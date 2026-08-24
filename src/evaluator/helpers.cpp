// Small helpers shared across the evaluator.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

#include <climits>

namespace satellite {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Bounds the C++ recursion the tree walk costs, so a pathological tree raises
// a satellite error instead of segfaulting the C++ stack (§6).
// satellite.library.main.x -> {"satellite", "library", "main", "x"}.
// False for anything that is not a pure SatelliteLit/Member chain, which is
// what separates a language path from an expression with members on it.
bool flatten_path(const Expr &expr, std::vector<std::string> &out)
{
    if (std::holds_alternative<SatelliteLit>(expr)) {
        out.push_back("satellite");
        return true;
    }
    if (const Member *m = std::get_if<Member>(&expr)) {
        if (!m->target || !flatten_path(*m->target, out))
            return false;
        out.push_back(m->name);
        return true;
    }
    return false;
}

std::string duration_misuse(const std::string &text)
{
    // Names the working form rather than only refusing: a duration is legal in
    // exactly one position, so "not here" without "there" leaves the reader
    // nothing to do. §8.2 is cited because the absence of a duration TYPE is a
    // decision with a reason, not an omission.
    return "a duration is not a value: " + text +
           " is a length of time, and satellite.console.display(" + text +
           ") — which sets the pause the printer takes between two displayed "
           "lines — is the only place the language asks for one (§8.2: there "
           "is no satellite.variable.duration)";
}

std::string join_path(const std::vector<std::string> &path)
{
    std::string out;
    for (size_t i = 0; i < path.size(); i++) {
        if (i)
            out += ".";
        out += path[i];
    }
    return out;
}

// A declaration with no initialiser still gets a value of its declared type,
// so `satellite.container.list<...> l` is an empty list you can append to
// rather than a nil you cannot. The bare `satellite` type has no such value
// and stays nil.
//
// A SPACESUIT type is nil here, and the declaration statement builds the
// instance instead (exec, below). The split is not arbitrary: a spacesuit's
// default is an ALLOCATION, so this function would have to be able to fail and
// to run satellite code, and a field of a suit's own type would default-
// construct forever. A variable is not part of any object's layout, so it has
// no such regress and gets a real instance; a field is, so it starts as nil and
// a method fills it with my_class_name().
Value default_of(const Type &type)
{
    if (type.is_singleton() || type.is_spacesuit())
        return std::monostate{};
    if (type.space == "variable") {
        if (type.name == "bool")
            return false;
        if (type.name == "number")
            return Number();
        if (type.name == "string")
            return make_string(SatString{});
        // The epoch. A time has no "empty" the way a string does, and the epoch
        // is the one instant that is a fact rather than a choice.
        if (type.name == "time")
            return Time{};
        // A file is nil until it is opened, and nil is a real answer rather
        // than a placeholder: satellite.variable.file is a reference type
        // (§8.3), and a declaration cannot open anything because it has no path
        // to open and nowhere to report that opening failed.
    }
    if (type.space == "container" && type.name == "list")
        return make_list(List{});
    // An empty map, not nil, for the same reason a list starts empty: a
    // declaration you cannot immediately .set() into would be useless, and nil
    // would make every map variable need an initialiser the language has no
    // syntax for.
    if (type.space == "container" && type.name == "map")
        return make_map(MapBody{});
    return std::monostate{};
}

// Structural, not pointer, equality: two lists holding equal values are equal
// even when they share no children. The variant's own operator== would compare
// the shared_ptrs inside a List, so the List arm has to come first.
//
// An OBJECT is the exception, and it needs no arm: the variant's own operator==
// compares the ObjectPtrs, which is identity, and identity is what equality
// means for a reference type. Two instances with equal fields are two
// instances — a spacesuit that wants them equal says so with a method, because
// only it knows which of its fields are part of what it means to be equal.
bool value_equals(const Value &a, const Value &b)
{
    if (a.index() != b.index())
        return false;
    if (const List *la = as_list(a)) {
        const List &lb = *as_list(b);
        if (la->size() != lb.size())
            return false;
        for (size_t i = 0; i < la->size(); i++) {
            const ValuePtr &x = (*la)[i];
            const ValuePtr &y = lb[i];
            if (!x || !y) {
                if (x != y)
                    return false;
                continue;
            }
            if (!value_equals(*x, *y))
                return false;
        }
        return true;
    }
    // Strings compare by CONTENT, and this arm is not optional.
    //
    // The variant's own operator== compares alternatives, and the string
    // alternative is a shared_ptr since strings moved behind a handle. That
    // compares POINTERS, so "x" == "x" was false whenever the two sides were
    // built separately — which is every interesting case. It shipped, and all
    // eleven test binaries passed, because nothing in the suite compared two
    // independently constructed strings. The satellite lexer written in
    // satellite is what caught it: its whitespace test stopped matching and
    // spaces started coming out as punctuation tokens.
    //
    // Any future alternative that is a handle to something with value semantics
    // needs an arm here too. The fallback below is correct only for
    // alternatives whose own operator== already means what the language means.
    if (const SatString *sa = as_string(a))
        return *sa == *as_string(b);

    // §21, and exactly the case the paragraph above warns about: a Bits is
    // behind a shared_ptr, so the variant's own operator== would compare
    // pointers and `x00FF == x00FF` would be false whenever the two sides were
    // written separately.
    //
    // Equal means SAME RADIX AND SAME DIGITS, so `x0009` and `x9` are not
    // equal. That follows from the width being part of the value rather than
    // being a separate decision: if they compared equal, one of them would have
    // to be a valid substitute for the other, and it is not — they pack to a
    // different number of bytes and arrive off a socket as different values.
    // A program that wants the numeric comparison asks for it: a.to_number()
    // .equals(b.to_number()) says which question is being asked.
    if (const Bits *ba = as_bits(a)) {
        const Bits *bb = as_bits(b);
        return bb && ba->radix == bb->radix && ba->digits == bb->digits;
    }

    // A map is exactly the shape the comment above warns about: a handle to
    // something with value semantics, whose fallback would compare MapRefs and
    // therefore pointers. Two independently built maps holding the same entries
    // must be equal.
    //
    // Order-INSENSITIVE, which is deliberate and is the one place a map's
    // insertion order does not count. Order is how a map is PRINTED and WALKED,
    // because those need to be deterministic; it is not part of what a map IS.
    // Two symbol tables that disagree only about which name was seen first hold
    // the same symbols.
    //
    // Looked up through b's index, so this is O(n) rather than O(n^2).
    if (const MapBody *ma = as_map(a)) {
        const MapBody *mb = as_map(b);
        if (ma->entries.size() != mb->entries.size())
            return false;
        for (const MapEntry &entry : ma->entries) {
            std::string key;
            if (!entry.key || !map_key_of(*entry.key, key))
                return false;
            auto found = mb->index.find(key);
            if (found == mb->index.end())
                return false;
            const ValuePtr &other = mb->entries[found->second].value;
            if (!entry.value || !other) {
                if (static_cast<bool>(entry.value) != static_cast<bool>(other))
                    return false;
                continue;
            }
            if (!value_equals(*entry.value, *other))
                return false;
        }
        return true;
    }

    return static_cast<const ValueBase &>(a) == static_cast<const ValueBase &>(b);
}

// An index must be a whole number: 1.5 is a bug in the program, not a
// silently truncated 1.
bool as_index(const Value &v, long long &out)
{
    // Number::to_integer answers both halves at once — is it whole, and does it
    // fit — so there is no isfinite/floor dance any more, and no way for a
    // value that merely rounds to an integer to pass as one.
    const Number *n = std::get_if<Number>(&v);
    return n && n->to_integer(out);
}

// Half-open, negative bounds Python-style, out-of-range clamps, hi < lo is
// empty (§7).
void clamp_range(long long &lo, long long &hi, long long len)
{
    if (lo < 0)
        lo += len;
    if (hi < 0)
        hi += len;
    lo = std::max(0LL, std::min(lo, len));
    hi = std::max(0LL, std::min(hi, len));
    if (hi < lo)
        hi = lo;
}

ValuePtr make_value(Value v)
{
    return std::make_shared<const Value>(std::move(v));
}

// A string and a list are stored behind a handle (value.hpp), so neither
// converts to a Value implicitly any more. These two overloads keep that a
// detail of the value model rather than something every call site restates.
ValuePtr make_value(SatString s)
{
    return make_value(make_string(std::move(s)));
}

ValuePtr make_value(List items)
{
    return make_value(make_list(std::move(items)));
}

ValuePtr make_value(MapBody body)
{
    return make_value(make_map(std::move(body)));
}


// The whole language, on one screen.
//
// That is only possible because the language IS one screen: two dozen methods,
// seven module functions, four statement forms. A reference that fits is worth
// more than a reference that is complete, and here they are the same thing --
// so this is checked against the code below rather than written once and left
// to rot.

ValuePtr module_constant(const std::vector<std::string> &path)
{
    // Deliberately reachable WITHOUT parentheses. Someone who needs help is by
    // definition someone who may not remember the calling syntax, and making
    // them get it right first is the one place a language can least afford to.
    if (path.size() == 2 && path[0] == "satellite" && path[1] == "help")
        return make_value(encode_raw(help_overview()));

    // A module does NOT answer here, and the reason is worth keeping: making
    // `satellite.directory` a value broke `satellite.directory.list()`, which
    // the evaluator reads as that value with `.list()` called on it -- so the
    // whole module disappeared behind "satellite.variable.string has no method
    // list". satellite.help can be bare because nothing is nested under it;
    // every module has its commands nested under it, so the parenthesised
    // satellite.directory() in modules.cpp is the only spelling that can work.

    if (path.size() == 3 && path[0] == "satellite" && path[1] == "bool") {
        if (path[2] == "true")
            return make_value(true);
        if (path[2] == "false")
            return make_value(false);
    }
    return nullptr;
}

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

static bool ends_with_ci(const std::string &name, const char *suffix)
{
    const size_t n = strlen(suffix);
    if (name.size() < n)
        return false;
    for (size_t i = 0; i < n; i++) {
        char a = name[name.size() - n + i];
        if (a >= 'A' && a <= 'Z')
            a = static_cast<char>(a - 'A' + 'a');
        if (a != suffix[i])
            return false;
    }
    return true;
}

static bool named_in(const std::string &name, const char *const *table)
{
    for (const char *const *p = table; *p; p++)
        if (ends_with_ci(name, *p))
            return true;
    return false;
}

// Archives. What is inside one is a question this listing does not open a file
// to answer, so it says what the thing IS and stops there.
static const char *const COMPRESSED[] = {
    ".tar", ".gz", ".tgz", ".xz", ".txz", ".bz2", ".tbz", ".tbz2", ".zst",
    ".tzst", ".zip", ".7z", ".rar", ".lz", ".lz4", ".lzma", ".z", ".jar",
    ".war", ".apk", ".deb", ".rpm", ".cab", ".iso", ".dmg", ".whl", ".xpi",
    ".cpio", ".ar", ".shar", ".sit", ".arj", ".lzh", nullptr
};

// Text BY NAME, so that a file too big to read to the end, or one whose first
// megabyte happens to hold something unusual, is still called what it is.
// Nothing here is a guess about content: every one of these is a format whose
// entire point is that a person can read it. A .sh is listed here rather than
// under exe on purpose -- it carries the execute bit and it is still source.
static const char *const TEXTUAL[] = {
    ".satl", ".txt", ".text", ".md", ".markdown", ".rst", ".log", ".csv",
    ".tsv", ".ini", ".cfg", ".conf", ".json", ".yaml", ".yml", ".toml",
    ".xml", ".html", ".htm", ".css", ".scss", ".less", ".svg", ".tex", ".bib",
    ".diff", ".patch", ".c", ".h", ".cc", ".hh", ".cpp", ".hpp", ".cxx",
    ".hxx", ".c++", ".h++", ".ipp", ".inl", ".m", ".mm", ".cs", ".java",
    ".js", ".mjs", ".cjs", ".ts", ".tsx", ".jsx", ".py", ".pyi", ".rb", ".pl",
    ".pm", ".php", ".go", ".rs", ".swift", ".kt", ".kts", ".scala", ".clj",
    ".cljs", ".lisp", ".el", ".scm", ".rkt", ".hs", ".ml", ".mli", ".ex",
    ".exs", ".erl", ".hrl", ".lua", ".r", ".jl", ".d", ".zig", ".nim", ".v",
    ".sh", ".bash", ".zsh", ".fish", ".ksh", ".csh", ".bat", ".cmd", ".ps1",
    ".vim", ".sql", ".asm", ".s", ".f", ".f90", ".f95", ".for", ".pas",
    ".vb", ".def", ".mk", ".cmake", ".gradle", ".spec", ".rules", ".desktop",
    ".service", ".in", ".am", ".ac", ".po", ".pot", ".gitignore", ".env",
    ".dockerfile", ".editorconfig", ".properties", ".gradle", ".sbt", nullptr
};

// rwxr-xr-x, the nine bits everyone can already read. The leading type
// character every ls prints is left off, because the type column beside this
// one says the same thing in a word.
static std::string permission_bits(mode_t mode)
{
    static const char *const rwx[] = {"---", "--x", "-w-", "-wx",
                                      "r--", "r-x", "rw-", "rwx"};
    std::string out = rwx[(mode >> 6) & 7];
    out += rwx[(mode >> 3) & 7];
    out += rwx[mode & 7];
    return out;
}

// CREATED, where the kernel and the filesystem are willing to say: statx(2)'s
// birth time, which ext4, xfs and btrfs record and which stat(2) has never
// carried at all. Where it is missing -- an old kernel, or a filesystem that
// never kept it -- the row falls back to the modification time, because the
// nearest true thing beats an empty column, and because a listing whose date
// column is blank on half the rows is a listing nobody trusts on the other
// half. The two cannot be told apart from the row, which is the honest cost.
static std::string created_on(const std::string &name, const struct stat &info)
{
    time_t when = info.st_mtime;
#if defined(__linux__) && defined(STATX_BTIME)
    struct statx sx;
    if (::statx(AT_FDCWD, name.c_str(), AT_STATX_SYNC_AS_STAT, STATX_BTIME,
                &sx) == 0 &&
        (sx.stx_mask & STATX_BTIME) != 0)
        when = static_cast<time_t>(sx.stx_btime.tv_sec);
#endif
    struct tm parts;
    if (!localtime_r(&when, &parts))
        return "";
    char out[32];
    if (strftime(out, sizeof out, "%Y-%m-%d %H:%M", &parts) == 0)
        return "";
    return out;
}

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

// Methods that write back through their receiver. They are the reason a
// receiver has to name a storage slot.
bool is_mutator(const std::string &name)
{
    return name == "append" || name == "set" || name == "remove";
}

// What the depth guard is protecting, measured rather than guessed.
//
// One capsule activation costs exactly 3 depth units — eval(Call) -> eval_call
// -> call_capsule -> exec(body) -> exec_block -> exec(return) -> eval — and
// 3169 bytes of C++ stack at -O2 (clang 24, x86-64). Three units is the fewest
// any body can cost, so that is the WORST ratio the guard has to survive: a
// longer body spends more units per byte, not fewer. The cost does not grow
// with the number of locals either, because a frame's slots are a vector.
//
// Where the C++ stack actually ends, bisected on an 8 MB stack with the guard
// disabled:
//
//     -O2    2600 levels return, 2800 segfault   -> ~7950 units
//     -O0    1200 levels return, 1300 segfault   -> ~3750 units
//
// §6's suggested default of 10000 was written when nothing recursed, and it is
// past BOTH cliffs — so the guard could never have fired, and a runaway
// recursion was a segfault rather than the error the guard exists to produce.
// 2000 units is a quarter of the optimised stack and half the unoptimised one.
// It is also what the resolver walks to (MAX_RESOLVE_DEPTH, env.cpp), so
// neither pass is the one that dies first.
constexpr int DEFAULT_MAX_DEPTH = 2000;

// The ceiling on satellite.library.system.max_depth, DERIVED FROM THE STACK
// THE PROCESS ACTUALLY HAS rather than fixed at one number.
//
// It used to be a hard 3000, and the reasoning for that number was sound while
// every run had the ordinary 8 MB: the knob raises the limit toward the cliff
// and must not raise it past, because above the cliff a larger setting does not
// buy deeper recursion, it buys a segfault instead of an error message. 3000
// also stayed under the -O0 cliff, an unoptimised build being exactly where
// losing the error message costs most.
//
// What that constant got wrong is that the cliff is not a constant. It is the
// stack, and the stack is `ulimit -s`. On 8 MB this formula still answers 3000,
// so nothing about a default run changes; on a 64 MB stack it answers ~24,000,
// and on a 64 GB one ~24,500,000 — about 8 million capsule activations, which
// is a depth no program reaches without meaning to.
//
// The divisor is the whole calibration and it is not a guess. One activation
// costs 3 units and ~3169 bytes at -O2, so 8 MB is ~7950 units of cliff; the
// old 3000 was 38% of that, and 8388608/2796 reproduces 3000 exactly. Keeping
// the RATIO rather than the number is what carries the -O0 margin along with
// it: an unoptimised activation costs about 2.1x more stack, and 38% of the
// -O2 cliff stays under the -O0 one at every stack size, not just at 8 MB.
//
// STACK_LIMIT_UNKNOWN — getrlimit failed, or said RLIM_INFINITY — is read as
// the ordinary 8 MB. "Unlimited" is not unbounded: the main thread's stack
// still stops where the next mapping begins, and believing the word would put
// the guard back past the cliff.
constexpr unsigned long long CEILING_BYTES_PER_UNIT = 2796;
constexpr unsigned long long ASSUMED_STACK_BYTES = 8ull << 20;

int max_max_depth()
{
    unsigned long long bytes = stack_limit_bytes();
    if (bytes == STACK_LIMIT_UNKNOWN)
        bytes = ASSUMED_STACK_BYTES;

    unsigned long long units = bytes / CEILING_BYTES_PER_UNIT;

    // A floor of 3, because one activation costs exactly that and a ceiling
    // below it would make every capsule call an error — a stack small enough
    // to justify that is one the process could not have started on.
    if (units < 3)
        units = 3;
    if (units > static_cast<unsigned long long>(INT_MAX))
        units = static_cast<unsigned long long>(INT_MAX);
    return static_cast<int>(units);
}

int read_max_depth()
{
    const int ceiling = max_max_depth();

    ValuePtr v = Library::instance().get("system", "max_depth");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(std::min<long long>(set, ceiling));

    // The DEFAULT is clamped by the ceiling too, which the fixed-constant
    // version never had to think about. `ulimit -s 1024` gives a cliff of ~370
    // units, and handing that run the unconditional 2000 would segfault it
    // before the guard ever looked.
    return std::min(DEFAULT_MAX_DEPTH, ceiling);
}

// The significant digits a non-terminating division keeps (§8.1). A knob for
// the same reason max_depth is one: the right answer depends on the program,
// and the default is only a default.
int read_division_digits()
{
    ValuePtr v = Library::instance().get("system", "division_digits");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(
                    std::min<long long>(set, Number::MAX_DIVISION_DIGITS));
    return Number::DEFAULT_DIVISION_DIGITS;
}

} // namespace satellite
