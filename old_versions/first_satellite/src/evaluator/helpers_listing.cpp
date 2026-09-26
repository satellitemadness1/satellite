// The listing, in columns: one row per element, and the header only where there
// is a table to head.
//
// Moved verbatim out of helpers.cpp, which was 779 lines. What each file IS --
// the suffix tables, the permission bits, the creation date -- is next door in
// helpers_file_facts.cpp; this file is the rendering, and the byte scan that
// only a reader of the whole file can do.
//
// Part of src/evaluator/. See eval_internal.hpp for what these pieces share.

#include "evaluator/eval_internal.hpp"

#include "evaluator/helpers_file_facts.hpp"

namespace satellite {

// The listing, in columns.
//
//   name        type       | permissions | size     | lines | date
//
// A list does not know that its strings are filenames -- nothing in a List
// records where they came from -- so this asks the disk rather than assuming:
// an element that names something gets a row, and an element that names
// nothing is printed as itself, bare, with no table built around it. That is
// what makes one rendering right for satellite.directory.list() and harmless
// for a list of words, and it is why the listing itself can go on handing back
// plain names that satellite.file.open still opens.
//
// gtk HAS a control for the type column, and it is better than this one:
// g_content_type_guess() answers from shared-mime-info's database, which knows
// formats no table here will. It is deliberately not used. §9 is that satl
// links six shared objects and satl-term links 119, and that the split exists
// so `satl --run` never pays the dynamic loader for a GUI stack it does not
// touch -- 25.9 ms against 2.5 ms. Linking glib to name a file type would put
// all of that back for one column of one rendering. So the type comes from the
// name first and the bytes second, which is what file(1) does and needs
// nothing linked at all.
static const long READ_CAP = 1L << 20;

std::string list_lines(const List &items)
{
    struct Row {
        std::string name;
        bool on_disk = false;
        std::string type;
        std::string perms;
        std::string size;
        std::string lines;
        std::string date;
    };

    std::vector<Row> rows;
    rows.reserve(items.size());
    bool any_on_disk = false;

    for (const ValuePtr &item : items) {
        Row row;
        row.name = item ? to_string(*item) : "satellite";

        struct stat info;
        if (::stat(row.name.c_str(), &info) != 0) {
            rows.push_back(std::move(row));
            continue;
        }
        row.on_disk = true;
        any_on_disk = true;
        // root_only, spelled out, rather than the nine bits that mean it.
        // `-r--------` owned by root is a fact about who may read the file, and
        // the reader of a listing has to translate three octal digits and an
        // owner column to get there. The word is the answer to the question
        // they were actually asking, and it is the case that comes up: it is
        // what /sys/firmware/dmi/entries looks like, which is why
        // satellite.system.memory.frequency() answers 0 for anybody but root.
        //
        // The test is deliberately narrow -- owned by root AND nothing granted
        // to group or other -- so a file that merely happens to be root's, with
        // ordinary r--r--r-- on it, still shows its bits.
        if (info.st_uid == 0 &&
            (info.st_mode & (S_IRWXG | S_IRWXO)) == 0)
            row.perms = "root_only";
        else
            row.perms = permission_bits(info.st_mode);
        // Rounded UP, so a file with one byte in it is not <0 kb>. Spelled the
        // way a person reading a listing says it, not the way a standards
        // document says KiB.
        row.size = std::to_string((info.st_size + 1023) / 1024) + " kb";
        row.date = created_on(row.name, info);

        if (S_ISDIR(info.st_mode)) {
            row.type = "dir";
            rows.push_back(std::move(row));
            continue;
        }
        if (!S_ISREG(info.st_mode)) {
            // A socket, a fifo, a device: it has a size, no contents worth
            // counting, and reading one can block until the machine is
            // rebooted.
            row.type = "special";
            rows.push_back(std::move(row));
            continue;
        }
        if (named_in(row.name, COMPRESSED)) {
            row.type = "compressed";
            rows.push_back(std::move(row));
            continue;
        }

        const bool by_name = named_in(row.name, TEXTUAL);

        // The bytes, up to READ_CAP. Neither "is it text", "is every byte
        // ascii" nor "how many lines" can be had from stat(2), and past the cap
        // the line count is left off rather than guessed: a listing that reads
        // a gigabyte to print one number is a listing nobody asks for twice.
        bool binary = false;
        bool ascii = true;
        bool elf = false;
        bool opened = false;
        long lines = 0;
        long seen = 0;
        unsigned char last = '\n';
        if (FILE *f = fopen(row.name.c_str(), "rb")) {
            opened = true;
            char buf[65536];
            size_t got;
            bool first = true;
            while (seen < READ_CAP && (got = fread(buf, 1, sizeof buf, f)) > 0) {
                if (first && got >= 4)
                    elf = buf[0] == '\x7f' && buf[1] == 'E' && buf[2] == 'L' &&
                          buf[3] == 'F';
                first = false;
                for (size_t i = 0; i < got; i++) {
                    last = static_cast<unsigned char>(buf[i]);
                    // A NUL is the one byte no text file has, and finding one
                    // settles the question: nothing later in the file can make
                    // it text again.
                    if (last == 0)
                        binary = true;
                    else if (last >= 0x80)
                        ascii = false;
                    if (last == '\n')
                        lines++;
                }
                seen += static_cast<long>(got);
                if (binary && !by_name)
                    break;
            }
            fclose(f);
        }
        // A last line with no newline on the end of it is still a line.
        if (seen > 0 && last != '\n')
            lines++;
        const bool counted = info.st_size <= READ_CAP;

        if (!opened) {
            // The file is there and this process may not read it -- /etc/shadow
            // from anybody but root. Everything below is a claim about bytes
            // nobody here has seen, and the empty read would otherwise report
            // it as an empty ascii file: a listing that says something false
            // and quietly, which is worse than one that says nothing. The name
            // is still evidence, so a .txt stays text; anything else says so.
            row.type = by_name ? "text" : "";
        } else if (elf) {
            row.type = "exe";
        } else if (by_name) {
            // The name wins over the bytes here, and it is allowed to: these
            // are formats that are text by definition. ascii or text is still
            // decided by what was actually read.
            row.type = counted && ascii && !binary ? "ascii" : "text";
        } else if (binary) {
            // No name to go on and a NUL in the bytes. An execute bit is the
            // only thing left that says what it is FOR.
            row.type = (info.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) ? "exe"
                                                                     : "binary";
        } else {
            row.type = counted && ascii ? "ascii" : "text";
        }

        if (opened && counted && row.type != "binary" && row.type != "exe")
            row.lines = std::to_string(lines);

        rows.push_back(std::move(row));
    }

    // Every cell carries something. A column that is blank on some rows reads
    // as a column that failed, and the reader cannot tell "this file has no
    // line count" from "the listing broke" -- so what does not apply says so:
    // an exe has no lines, a filesystem that never recorded a birth time has
    // no date, and an element that is not a path on this machine has none of
    // it. The dash is not a value and does not line up like one on purpose.
    const std::string none = "- -";
    for (Row &row : rows) {
        if (!row.on_disk && any_on_disk) {
            // A word in a list of paths still gets its row rather than
            // dropping out of the table it is sitting in.
            row.type = row.perms = row.size = row.lines = row.date = none;
            continue;
        }
        if (!row.on_disk)
            continue;
        if (row.type.empty())
            row.type = none;
        if (row.perms.empty())
            row.perms = none;
        if (row.size.empty())
            row.size = none;
        if (row.lines.empty())
            row.lines = none;
        if (row.date.empty())
            row.date = none;
    }

    // Widths from the rows themselves, so the columns are as narrow as the
    // listing allows and no narrower. A name is never truncated to fit: a name
    // with the end cut off is one that cannot be typed back in, which is the
    // one thing every other column exists to support.
    size_t w_name = 4, w_type = 4, w_size = 4, w_lines = 5, w_date = 4;
    for (const Row &row : rows) {
        w_name = std::max(w_name, row.name.size());
        w_type = std::max(w_type, row.type.size());
        w_size = std::max(w_size, row.size.size());
        w_lines = std::max(w_lines, row.lines.size());
        w_date = std::max(w_date, row.date.size());
    }

    const auto pad = [](std::string &out, const std::string &text, size_t width,
                        bool right) {
        if (right)
            out.append(width - text.size(), ' ');
        out += text;
        if (!right)
            out.append(width - text.size(), ' ');
    };

    std::string out;
    // The header appears only where there is a table to head. A list of words
    // stays a list of words.
    if (any_on_disk) {
        pad(out, "name", w_name, false);
        out += "  ";
        pad(out, "type", w_type, false);
        out += " | ";
        pad(out, "permissions", 11, false);
        out += " | ";
        pad(out, "size", w_size, true);
        out += " | ";
        pad(out, "lines", w_lines, true);
        out += " | date\n";
    }

    for (const Row &row : rows) {
        if (!any_on_disk) {
            // Nothing in this list was a path, so there is no table to be a
            // row of: a list of words prints as a list of words.
            out += row.name;
            out += "\n";
            continue;
        }
        pad(out, row.name, w_name, false);
        out += "  ";
        pad(out, row.type, w_type, false);
        out += " | ";
        pad(out, row.perms, 11, false);
        out += " | ";
        pad(out, row.size, w_size, true);
        out += " | ";
        pad(out, row.lines, w_lines, true);
        out += " | ";
        out += row.date;
        out += "\n";
    }
    return out;
}

} // namespace satellite
