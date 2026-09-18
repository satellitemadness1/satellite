// satellite/satellite_variable_file/file_cases.cpp -- satellite.variable.file's
// handle with no interpreter around it (SATELLITE_FILE_OPERATIONS Part 3):
// the list of lines counted from 1, the endings a file had, strict UTF-8, and
// the two ways a save lands.
//
//     make build/file_cases && build/file_cases "${TMPDIR:-/tmp}"    (check.sh runs it)
//
// IT WRITES ONLY INSIDE ONE FRESH FOLDER, made in the folder it is given and
// removed at the end. `ok <name>` for each case that passes, `FAIL <name>: <what
// happened>` for each that does not; the exit status is 0 only when none failed.
// The cases that need a permission refused are skipped as root, and say so.

#include "satellite_file.hpp"

#include "../machine/machine_codes.hpp"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

using namespace satellite004;

namespace {

using Kind = satellite_file::Kind;
using Handle = std::shared_ptr<satellite_file>;

int failed = 0;
int passed = 0;
std::string room;   // the fresh folder every case works in

void check(bool right, const std::string &name, const std::string &detail = "")
{
    if (right) {
        ++passed;
        std::printf("ok %s\n", name.c_str());
    } else {
        ++failed;
        std::printf("FAIL %s: %s\n", name.c_str(), detail.empty() ? "(no detail)" : detail.c_str());
    }
}

void skipped(const std::string &name, const char *why)
{
    ++passed;
    std::printf("ok %s (skipped: %s)\n", name.c_str(), why);
}

std::string at(const std::string &name)
{
    return room + "/" + name;
}

// Bytes as a person can read them in a FAIL line.
std::string shown(const std::string &bytes)
{
    std::string out = "\"";
    for (const char raw : bytes) {
        const unsigned char c = static_cast<unsigned char>(raw);
        if (out.size() > 160) {
            out += "...";
            break;
        }
        if (c == '\n') out += "\\n";
        else if (c == '\r') out += "\\r";
        else if (c == '\\') out += "\\\\";
        else if (c == '"') out += "\\\"";
        else if (c < 0x20 || c >= 0x7F) {
            char hex[8];
            std::snprintf(hex, sizeof hex, "\\x%02X", c);
            out += hex;
        } else out += static_cast<char>(c);
    }
    return out + "\"";
}

std::string said(const Handle &f)
{
    if (!f)
        return "no handle at all";
    return std::string("ok ") + (f->ok() ? "true" : "false") + ", code " + machine_code_name(f->code()) +
           ", error \"" + f->error() + "\"";
}

bool put(const std::string &path, const std::string &bytes)
{
    const int fd = ::open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0644);
    if (fd < 0)
        return false;
    const bool wrote = ::write(fd, bytes.data(), bytes.size()) == static_cast<ssize_t>(bytes.size());
    return ::close(fd) == 0 && wrote;
}

// The file's bytes, or "<unreadable: ...>" when there are none to read.
std::string bytes_of(const std::string &path)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_CLOEXEC | O_NONBLOCK);
    if (fd < 0)
        return std::string("<unreadable: ") + std::strerror(errno) + ">";
    std::string out;
    char buffer[65536];
    for (;;) {
        const ssize_t got = ::read(fd, buffer, sizeof buffer);
        if (got <= 0)
            break;
        out.append(buffer, static_cast<std::size_t>(got));
    }
    ::close(fd);
    return out;
}

void expect_bytes(const std::string &name, const std::string &path, const std::string &want)
{
    const std::string got = bytes_of(path);
    check(got == want, name, "the file holds " + shown(got) + ", not " + shown(want));
}

void expect_all(const std::string &name, const Handle &f, const std::string &want)
{
    const std::string got = f ? f->read_all() : std::string("<no handle>");
    check(got == want, name, "read_all() is " + shown(got) + ", not " + shown(want) + "; " + said(f));
}

void make_link(const std::string &target, const std::string &path)
{
    if (::symlink(target.c_str(), path.c_str()) != 0)
        check(false, "setup: a symlink at " + path, std::strerror(errno));
}

bool there(const std::string &path)   // anything at all, a dangling link included
{
    struct stat about {};
    return ::lstat(path.c_str(), &about) == 0;
}

ino_t inode_of(const std::string &path)
{
    struct stat about {};
    return ::stat(path.c_str(), &about) == 0 ? about.st_ino : 0;
}

mode_t mode_of(const std::string &path)
{
    struct stat about {};
    return ::stat(path.c_str(), &about) == 0 ? (about.st_mode & 07777) : 0;
}

long long size_on_disk(const std::string &path)
{
    struct stat about {};
    return ::stat(path.c_str(), &about) == 0 ? static_cast<long long>(about.st_size) : -1;
}

bool set_old_time(const std::string &path)
{
    const struct timespec times[2] = {{1000000000, 0}, {1000000000, 0}};
    return ::utimensat(AT_FDCWD, path.c_str(), times, AT_SYMLINK_NOFOLLOW) == 0;
}

bool has_old_time(const std::string &path)
{
    struct stat about {};
    return ::stat(path.c_str(), &about) == 0 && about.st_mtim.tv_sec == 1000000000 && about.st_mtim.tv_nsec == 0;
}

// The names in a folder that a save left behind.
std::vector<std::string> leftovers(const std::string &folder)
{
    std::vector<std::string> names;
    DIR *listing = ::opendir(folder.c_str());
    if (listing == nullptr)
        return names;
    while (const dirent *entry = ::readdir(listing))
        if (std::strstr(entry->d_name, ".saving.") != nullptr)
            names.push_back(entry->d_name);
    ::closedir(listing);
    return names;
}

std::string line_of(const Handle &f, std::size_t n)
{
    std::string out;
    if (!f || !f->line(n, out))
        return "<no line " + std::to_string(n) + ">";
    return out;
}

bool contains_text(const std::string &haystack, const std::string &needle)
{
    return haystack.find(needle) != std::string::npos;
}

const std::string kNul("\0", 1);

// ---------------------------------------------------------------------------
// 1. THE AUTHOR'S EXAMPLE, counted from 1
// ---------------------------------------------------------------------------
void the_authors_example()
{
    const std::string p = at("demo.se");
    const Handle f = satellite_file::make_new(p, Kind::text);
    check(f && f->ok() && f->size() == 0 && f->code() == success, "author: new makes an open, empty file", said(f));
    if (!f || !f->ok())
        return;
    check(bytes_of(p).empty() && satellite_file::exists(p), "author: ... and it is on the disk, empty", shown(bytes_of(p)));
    check(f->append("line_1"), "author: append(\"line_1\")", said(f));
    expect_all("author: ... line_1", f, "line_1\n");
    check(f->append("line_2"), "author: append(\"line_2\")", said(f));
    expect_all("author: ... line_1 line_2", f, "line_1\nline_2\n");
    check(f->insert(2, "line_2"), "author: insert(2, \"line_2\")", said(f));
    expect_all("author: ... line_1 line_2 line_2", f, "line_1\nline_2\nline_2\n");
    check(f->replace_line(2, "line_3"), "author: replace_line(2, \"line_3\")", said(f));
    expect_all("author: ... line_1 line_3 line_2", f, "line_1\nline_3\nline_2\n");
    check(f->replace_text("line_2", "line_4"), "author: replace_text(\"line_2\", \"line_4\")", said(f));
    expect_all("author: ... line_1 line_3 line_4", f, "line_1\nline_3\nline_4\n");
    check(f->close() && !f->ok(), "author: close() answers true and closes", said(f));
    expect_bytes("author: the disk holds exactly line_1 line_3 line_4", p, "line_1\nline_3\nline_4\n");
}

// ---------------------------------------------------------------------------
// 2. NEW NEVER CLOBBERS
// ---------------------------------------------------------------------------
void new_never_clobbers()
{
    const std::string p = at("there.txt");
    put(p, "keep me\n");
    set_old_time(p);
    Handle f = satellite_file::make_new(p, Kind::text);
    check(f && !f->ok() && f->code() == file_already_there && !f->error().empty(),
          "new over a file: not ok, file_already_there", said(f));
    check(bytes_of(p) == "keep me\n" && has_old_time(p), "new over a file: its bytes and mtime are unchanged",
          shown(bytes_of(p)));
    check(f && f->size() == 0 && f->code() == file_not_open, "new over a file: the handle has no lines", said(f));

    ::mkdir(at("a_folder").c_str(), 0755);
    f = satellite_file::make_new(at("a_folder"), Kind::text);
    check(f && !f->ok() && f->code() == not_a_file, "new over a directory: not_a_file", said(f));
    struct stat about {};
    check(::stat(at("a_folder").c_str(), &about) == 0 && S_ISDIR(about.st_mode), "new over a directory: it is still one");

    make_link("nowhere_yet", at("dangling"));
    f = satellite_file::make_new(at("dangling"), Kind::text);
    check(f && !f->ok() && f->code() == file_already_there, "new over a dangling symlink: file_already_there", said(f));
    check(!there(at("nowhere_yet")), "new over a dangling symlink: nothing made where it points");

    f = satellite_file::make_new(at("no_such_folder/x.txt"), Kind::text);
    check(f && !f->ok() && f->code() == file_unwritable && contains_text(f->error(), "No such file"),
          "new in a missing folder: file_unwritable, with the system's reason", said(f));
    check(!there(at("no_such_folder")), "new in a missing folder: no folder is made");

    f = satellite_file::make_new(at("binary.dat"), Kind::binary);
    check(f && !f->ok() && f->code() == not_built_yet &&
              f->error() == "binary files are not built yet (SATELLITE_FILE_OPERATIONS FO-8)",
          "new binary: not_built_yet, said by name", said(f));
    check(!there(at("binary.dat")), "new binary: nothing is made");

    f = satellite_file::make_new(at("nul") + kNul + "tail", Kind::text);
    check(f && !f->ok() && f->code() == path_holds_a_nul, "new with a NUL in the path: path_holds_a_nul", said(f));
    check(!there(at("nul")), "new with a NUL in the path: nothing made at the part before it");

    f = satellite_file::make_new(at("fresh.txt"), Kind::text);
    check(f && f->ok() && f->code() == success && f->error().empty() && f->path() == at("fresh.txt"),
          "new where nothing is: ok, no error, the path as given", said(f));
    check(f && f->close(), "new, then close with nothing added: true", said(f));
    expect_bytes("new, then close with nothing added: the file is there and empty", at("fresh.txt"), "");
}

// ---------------------------------------------------------------------------
// 3. OPEN NEVER CREATES, AND OPENS ONLY FILES
// ---------------------------------------------------------------------------
void open_never_creates()
{
    Handle f = satellite_file::open_existing(at("missing.txt"), Kind::text);
    check(f && !f->ok() && f->code() == file_not_found, "open of a missing path: file_not_found", said(f));
    check(!there(at("missing.txt")), "open of a missing path: nothing is made");

    f = satellite_file::open_existing(at("no_such_folder/deeper.txt"), Kind::text);
    check(f && f->code() == file_not_found, "open under a missing folder: file_not_found", said(f));

    ::mkdir(at("open_folder").c_str(), 0755);
    f = satellite_file::open_existing(at("open_folder"), Kind::text);
    check(f && !f->ok() && f->code() == not_a_file, "open of a directory: not_a_file", said(f));

    const std::string fifo = at("a_fifo");
    if (::mkfifo(fifo.c_str(), 0600) != 0) {
        check(false, "open of a FIFO", "mkfifo failed: " + std::string(std::strerror(errno)));
    } else {
        ::alarm(20);   // a watchdog: a FIFO opened for reading waits for a writer forever
        const auto started = std::chrono::steady_clock::now();
        f = satellite_file::open_existing(fifo, Kind::text);
        const auto took = std::chrono::steady_clock::now() - started;
        ::alarm(0);
        check(f && !f->ok() && f->code() == not_a_file, "open of a FIFO: not_a_file", said(f));
        check(took < std::chrono::seconds(2), "open of a FIFO: answers at once, without waiting for a writer");
    }

    if (::geteuid() == 0) {
        skipped("open of a file it may not read: file_unreadable", "running as root");
    } else {
        const std::string secret = at("secret.txt");
        put(secret, "hidden\n");
        ::chmod(secret.c_str(), 0000);
        f = satellite_file::open_existing(secret, Kind::text);
        check(f && !f->ok() && f->code() == file_unreadable && contains_text(f->error(), "Permission denied"),
              "open of a file it may not read: file_unreadable, with the system's reason", said(f));
        ::chmod(secret.c_str(), 0600);
    }

    put(at("plain.txt"), "one\ntwo\n");
    f = satellite_file::open_existing(at("plain.txt"), Kind::binary);
    check(f && !f->ok() && f->code() == not_built_yet, "open binary: not_built_yet", said(f));
    check(f && !f->append("x") && f->code() == file_has_no_lines, "a line word on a binary handle: file_has_no_lines",
          said(f));
    std::string out = "stale";
    check(f && !f->line(1, out) && out.empty() && f->code() == file_has_no_lines && f->size() == 0,
          "line() and size() on a binary handle: false and 0, file_has_no_lines", said(f));

    f = satellite_file::open_existing(at("plain.txt") + kNul, Kind::text);
    check(f && !f->ok() && f->code() == path_holds_a_nul, "open with a NUL in the path: path_holds_a_nul", said(f));

    make_link("plain.txt", at("plain_link"));
    f = satellite_file::open_existing(at("plain_link"), Kind::text);
    check(f && f->ok() && f->size() == 2 && line_of(f, 2) == "two", "open through a symlink to a file", said(f));

    put(at("empty.txt"), "");
    f = satellite_file::open_existing(at("empty.txt"), Kind::text);
    check(f && f->ok() && f->size() == 0 && f->empty(), "an empty file is 0 lines", said(f));
    put(at("newline.txt"), "\n");
    f = satellite_file::open_existing(at("newline.txt"), Kind::text);
    check(f && f->ok() && f->size() == 1 && line_of(f, 1).empty(), "a file of one \\n is 1 empty line", said(f));
    check(f && f->close(), "... closed with nothing changed", said(f));
    expect_bytes("... and still one \\n", at("newline.txt"), "\n");
}

// ---------------------------------------------------------------------------
// 4. A FILE IS WRITTEN BACK THE WAY IT WAS
// ---------------------------------------------------------------------------
void endings_are_kept()
{
    // \r\n STAYS \r\n
    std::string p = at("crlf.txt");
    put(p, "a\r\nb\r\n");
    Handle f = satellite_file::open_existing(p, Kind::text);
    check(f && f->ok() && f->size() == 2 && line_of(f, 1) == "a" && line_of(f, 2) == "b",
          "crlf: the \\r is part of the ending, not the line", said(f));
    check(f && f->insert(2, "x") && f->append("c") && f->close(), "crlf: insert, append, close", said(f));
    expect_bytes("crlf: an edited \\r\\n file stays \\r\\n", p, "a\r\nx\r\nb\r\nc\r\n");

    // MIXED: each line keeps its own
    p = at("mixed.txt");
    put(p, "a\nb\r\nc\n");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->replace_line(1, "A") && f->append("d") && f->close(), "mixed: replace, append, close", said(f));
    expect_bytes("mixed: each line keeps its own ending; a new one takes the first line's", p, "A\nb\r\nc\nd\n");
    p = at("mixed_crlf_first.txt");
    put(p, "a\r\nb\n");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->append("c") && f->close(), "mixed, \\r\\n first: append, close", said(f));
    expect_bytes("mixed, \\r\\n first: the new line ends \\r\\n", p, "a\r\nb\nc\r\n");

    // THE BYTE-ORDER MARK
    p = at("bom.txt");
    put(p, "\xEF\xBB\xBFone\ntwo\n");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->ok() && line_of(f, 1) == "one" && f->index_of("one") == 1,
          "bom: the mark is not part of line 1", said(f));
    expect_all("bom: read_all() has the mark", f, "\xEF\xBB\xBFone\ntwo\n");
    check(f && f->replace_line(2, "TWO") && f->close(), "bom: replace, close", said(f));
    expect_bytes("bom: the mark survives a rewrite", p, "\xEF\xBB\xBFone\nTWO\n");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->append("three") && f->close(), "bom: append, close", said(f));
    expect_bytes("bom: ... and an append", p, "\xEF\xBB\xBFone\nTWO\nthree\n");

    // NO FINAL NEWLINE
    p = at("no_final.txt");
    put(p, "a\nb");
    set_old_time(p);
    ino_t inode = inode_of(p);
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->ok() && f->size() == 2 && line_of(f, 2) == "b", "no final newline: the last piece is a line", said(f));
    check(f && f->save() && f->close(), "no final newline: save and close with nothing changed", said(f));
    check(bytes_of(p) == "a\nb" && has_old_time(p) && inode_of(p) == inode,
          "no final newline: unchanged, it is not written at all", shown(bytes_of(p)));
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->replace_line(1, "A") && f->close(), "no final newline: replace line 1, close", said(f));
    expect_bytes("no final newline: a rewrite keeps the last line unended", p, "A\nb");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->replace_line(2, "B") && f->close(), "no final newline: replace the last line, close", said(f));
    expect_bytes("no final newline: the replaced last line keeps having none", p, "A\nB");

    p = at("no_final_append.txt");
    put(p, "a\nb");
    inode = inode_of(p);
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->append("c"), "append after no final newline", said(f));
    expect_all("append after no final newline: the old last line is ended first", f, "a\nb\nc\n");
    check(f && f->save(), "append after no final newline: save", said(f));
    check(bytes_of(p) == "a\nb\nc\n" && inode_of(p) == inode,
          "append after no final newline: the append path writes the ending, then the line",
          shown(bytes_of(p)) + (inode_of(p) == inode ? "" : " (the inode changed)"));
    check(f && f->append("d") && f->close(), "append after no final newline: a second append, close", said(f));
    expect_bytes("append after no final newline: ... the ending is written once", p, "a\nb\nc\nd\n");

    p = at("no_final_rewrite.txt");
    put(p, "a\r\nb");
    inode = inode_of(p);
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->append("c") && f->replace_line(1, "A") && f->close(),
          "append after no final newline, then an edit: close", said(f));
    check(bytes_of(p) == "A\r\nb\r\nc\r\n" && inode_of(p) != inode,
          "append after no final newline, full rewrite: the old last line is ended in the file's ending",
          shown(bytes_of(p)));

    p = at("split_last.txt");
    put(p, "a\r\nb");
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->replace_line(2, "x\ny") && f->size() == 3 && f->close(),
          "replace_line splitting the unended last line", said(f));
    expect_bytes("replace_line split: earlier pieces take the file's ending, the last keeps the line's", p,
                 "a\r\nx\r\ny");

    // TEXT IN: '\n' makes lines; a trailing one adds none; "" is one empty line
    p = at("text_in.txt");
    f = satellite_file::make_new(p, Kind::text);
    check(f && f->append("p\nq") && f->size() == 2, "text in: \"p\\nq\" is two lines", said(f));
    check(f && f->append("r\n") && f->size() == 3, "text in: \"r\\n\" is one line", said(f));
    check(f && f->append("s\n\n") && f->size() == 5 && line_of(f, 4) == "s" && line_of(f, 5).empty(),
          "text in: \"s\\n\\n\" is \"s\" and \"\"", said(f));
    check(f && f->append("") && f->size() == 6 && line_of(f, 6).empty(), "text in: \"\" is one empty line", said(f));
    check(f && f->append("\n") && f->size() == 7 && line_of(f, 7).empty(), "text in: \"\\n\" is one empty line",
          said(f));
    check(f && f->append("t\r\nu") && f->size() == 9 && line_of(f, 8) == "t" && line_of(f, 9) == "u",
          "text in: \\r\\n inside the text is one line ending, not text", said(f));
    check(f && !f->append("a\rb") && f->code() == file_not_text && f->size() == 9,
          "text in: a \\r that ends no line is refused (file_not_text), nothing added", said(f));
    check(f && !f->append("z\r") && f->code() == file_not_text && f->size() == 9,
          "text in: a \\r at the very end is refused too", said(f));
    check(f && f->insert(1, "i1\ni2") && f->size() == 11 && line_of(f, 1) == "i1" && line_of(f, 2) == "i2",
          "text in: insert of two lines puts both in, in order", said(f));
    check(f && f->close(), "text in: close", said(f));
    expect_bytes("text in: the file", p, "i1\ni2\np\nq\nr\ns\n\n\n\nt\nu\n");
}

// ---------------------------------------------------------------------------
// 5. TEXT MEANS UTF-8, and a NUL is data
// ---------------------------------------------------------------------------
void only_text_opens()
{
    struct refusal {
        const char *name;
        std::string bytes;
        const char *must_say;
        const char *must_say_too;
    };
    const refusal refusals[] = {
        {"a lone 0xFF", std::string("ok\n\xFF\n"), "line 2", "offset 0 in the line"},
        {"an overlong 0xC0 0xAF", std::string("x\n\n\xC0\xAF\n"), "line 3", "0xC0"},
        {"a surrogate 0xED 0xA0 0x80", std::string("ab\xED\xA0\x80\n"), "line 1", "offset 2 in the line"},
        {"a code above U+10FFFF", std::string("\xF4\x90\x80\x80\n"), "line 1", "0xF4"},
        {"a character cut by the newline", std::string("fine\nab\xE2\x82\ncd\n"), "line 2", "offset 2 in the line"},
        {"a bad byte after a byte-order mark", std::string("\xEF\xBB\xBF\xFF\n"), "line 1", "offset 3 in the file"},
        {"a \\r-only file", std::string("a\rb\rc\r"), "line 1", "\\r"},
        {"a \\r inside a line", std::string("a\nb\rc\n"), "line 2", "\\r"},
    };
    int k = 0;
    for (const refusal &one : refusals) {
        const std::string p = at("not_text_" + std::to_string(k++) + ".txt");
        put(p, one.bytes);
        const Handle f = satellite_file::open_existing(p, Kind::text);
        check(f && !f->ok() && f->code() == file_not_text && contains_text(f->error(), one.must_say) &&
                  contains_text(f->error(), one.must_say_too),
              std::string("not text: ") + one.name + " is file_not_text, naming " + one.must_say, said(f));
        check(bytes_of(p) == one.bytes, std::string("not text: ") + one.name + " is left as it was");
    }

    std::string p = at("wide.txt");
    const std::string wide = "\xF0\x9F\x98\x80\n\xE9\xB1\x80 is U+9C40\n";
    put(p, wide);
    Handle f = satellite_file::open_existing(p, Kind::text);
    check(f && f->ok() && f->size() == 2 && line_of(f, 1) == "\xF0\x9F\x98\x80",
          "UTF-8: a 4-byte character and U+9C40 open", said(f));
    check(f && f->insert(1, "\xC3\xA9") && f->close(), "UTF-8: insert, close", said(f));
    expect_bytes("UTF-8: round-trips", p, "\xC3\xA9\n" + wide);

    p = at("nul_inside.txt");
    put(p, std::string("a\0b\nc\n", 6));
    f = satellite_file::open_existing(p, Kind::text);
    check(f && f->ok() && f->size() == 2 && line_of(f, 1) == std::string("a\0b", 3),
          "NUL inside a line: it is data", said(f));
    check(f && f->index_of(std::string("a\0b", 3)) == 1 && f->search(kNul) == 1, "NUL inside a line: found as data",
          said(f));
    check(f && f->replace_line(2, "C") && f->close(), "NUL inside a line: replace another, close", said(f));
    expect_bytes("NUL inside a line: round-trips", p, std::string("a\0b\nC\n", 6));
}

// ---------------------------------------------------------------------------
// 6. THE TWO SAVES
// ---------------------------------------------------------------------------
void the_two_saves()
{
    const std::string p = at("fast.txt");
    put(p, "a\n");
    ::chmod(p.c_str(), 0640);
    const ino_t first = inode_of(p);
    Handle f = satellite_file::open_existing(p, Kind::text);
    check(f && f->append("b") && f->save(), "append path: append, save", said(f));
    check(bytes_of(p) == "a\nb\n" && inode_of(p) == first, "append path: the same file, only added to (same inode)",
          shown(bytes_of(p)));
    check(f && f->insert(1, "z") && f->save(), "full rewrite: insert, save", said(f));
    check(bytes_of(p) == "z\na\nb\n" && inode_of(p) != first, "full rewrite: a new file renamed over (a new inode)",
          shown(bytes_of(p)));
    check(mode_of(p) == 0640, "full rewrite: the file's mode 0640 is kept",
          "mode is 0" + std::to_string(mode_of(p) >> 6) + std::to_string((mode_of(p) >> 3) & 7) +
              std::to_string(mode_of(p) & 7));
    check(f && f->close(), "full rewrite: close", said(f));
    check(leftovers(room).empty(), "full rewrite: no .saving. file is left beside it");

    const std::string target = at("target.txt");
    const std::string link = at("link.txt");
    put(target, "one\n");
    make_link("target.txt", link);
    f = satellite_file::open_existing(link, Kind::text);
    check(f && f->replace_line(1, "ONE") && f->close(), "symlink: replace through the link, close", said(f));
    struct stat about {};
    char pointed[256] = {};
    const ssize_t length = ::readlink(link.c_str(), pointed, sizeof pointed - 1);
    check(::lstat(link.c_str(), &about) == 0 && S_ISLNK(about.st_mode) && length > 0 &&
              std::string(pointed, static_cast<std::size_t>(length)) == "target.txt",
          "symlink: the link is still a link, to the same name");
    expect_bytes("symlink: its target was rewritten", target, "ONE\n");
    f = satellite_file::open_existing(link, Kind::text);
    check(f && f->append("two") && f->close(), "symlink: append through the link, close", said(f));
    check(::lstat(link.c_str(), &about) == 0 && S_ISLNK(about.st_mode) && bytes_of(target) == "ONE\ntwo\n",
          "symlink: an append lands in the target too", shown(bytes_of(target)));

    // A RELATIVE PATH KEEPS MEANING THE FILE IT OPENED when the program changes
    // directory before the save (satellite.directory.change).
    char *started = ::getcwd(nullptr, 0);
    const std::string elsewhere = at("elsewhere");
    ::mkdir(elsewhere.c_str(), 0755);
    if (started == nullptr || ::chdir(room.c_str()) != 0) {
        check(false, "relative path: setup", std::strerror(errno));
    } else {
        put("relative_edit.txt", "a\n");
        put("relative_append.txt", "a\n");
        const Handle edited = satellite_file::open_existing("relative_edit.txt", Kind::text);
        const Handle added = satellite_file::open_existing("relative_append.txt", Kind::text);
        const bool changed = edited->insert(1, "z") && added->append("b");
        const bool moved = ::chdir(elsewhere.c_str()) == 0;
        check(changed && moved && edited->close() && added->close() && edited->path() == "relative_edit.txt",
              "relative path: both saves land after the working directory moves", said(edited) + "; " + said(added));
        expect_bytes("relative path: the rewrite went to the file it opened", at("relative_edit.txt"), "z\na\n");
        expect_bytes("relative path: the append went to the file it opened", at("relative_append.txt"), "a\nb\n");
        check(!there(elsewhere + "/relative_edit.txt") && !there(elsewhere + "/relative_append.txt"),
              "relative path: nothing was made in the new working directory");
        if (::chdir(started) != 0)
            check(false, "relative path: back to the first working directory", std::strerror(errno));
    }
    std::free(started);
}

// ---------------------------------------------------------------------------
// 7. A LONG LOOP OF APPENDS STREAMS
// ---------------------------------------------------------------------------
void appends_stream()
{
    const std::string p = at("many.txt");
    const Handle f = satellite_file::make_new(p, Kind::text);
    if (!f || !f->ok()) {
        check(false, "100,000 appends", said(f));
        return;
    }
    const ino_t inode = inode_of(p);
    std::string expected;
    bool every = true;
    bool streamed = false;
    std::string at_20000;
    for (int i = 1; i <= 100000; ++i) {
        const std::string text = "line " + std::to_string(i);
        every = f->append(text) && every;
        expected += text;
        expected += '\n';
        if (i == 20000) {
            const std::string disk = bytes_of(p);
            streamed = !disk.empty() && disk.size() < expected.size() && disk.back() == '\n' &&
                       expected.compare(0, disk.size(), disk) == 0;
            at_20000 = std::to_string(disk.size()) + " bytes on the disk of " + std::to_string(expected.size());
        }
    }
    check(every && f->size() == 100000, "100,000 appends: every one true, 100,000 lines", said(f));
    check(streamed, "100,000 appends: past 64 KiB the lines are on the disk before close, whole", at_20000);
    const long long before_close = size_on_disk(p);
    check(before_close > 0 && before_close <= static_cast<long long>(expected.size()),
          "100,000 appends: the file is not empty before close", std::to_string(before_close));
    check(f->close(), "100,000 appends: close", said(f));
    check(bytes_of(p) == expected, "100,000 appends: the file holds every line, in order",
          std::to_string(bytes_of(p).size()) + " bytes, not " + std::to_string(expected.size()));
    check(inode_of(p) == inode, "100,000 appends: never rewritten (the same inode)");
    const Handle again = satellite_file::open_existing(p, Kind::text);
    check(again && again->size() == 100000 && line_of(again, 1) == "line 1" && line_of(again, 100000) == "line 100000",
          "100,000 appends: read back, line 1 and line 100000", said(again));
}

// ---------------------------------------------------------------------------
// 8. ASKING WHERE A LINE IS, and replacing text
// ---------------------------------------------------------------------------
void where_lines_are()
{
    const Handle f = satellite_file::make_new(at("search.txt"), Kind::text);
    check(f && f->append("alpha") && f->append("beta") && f->append("alphabet") && f->append("beta"),
          "search: four lines", said(f));
    if (!f || !f->ok())
        return;
    check(f->index_of("beta") == 2 && f->index_of("alphabet") == 3, "index_of: the first line that IS the text, from 1");
    check(f->index_of("gamma") == 0 && f->index_of("alph") == 0 && f->index_of("") == 0,
          "index_of: 0 when no line is exactly the text");
    check(f->search("pha") == 1 && f->search("bet") == 2 && f->search("habe") == 3,
          "search: the first line that CONTAINS the text, from 1");
    check(f->search("zzz") == 0 && f->search("") == 0 && f->search("a\nb") == 0, "search: 0 when none does, and for \"\"");
    check(f->contains("alphabet") && !f->contains("alph") && !f->contains("gamma"), "contains: exactly a line");
    check(f->code() == success && f->error().empty(), "a question with no answer is not a failure", said(f));

    check(!f->replace_text("", "x") && f->code() == empty_search_text, "replace_text of \"\": empty_search_text", said(f));
    check(!f->replace_text("zzz", "y") && f->code() == text_not_found, "replace_text of absent text: text_not_found",
          said(f));
    check(!f->replace_text("alpha\nbeta", "y") && f->code() == text_not_found,
          "replace_text across a line ending: text_not_found (a line holds no newline)", said(f));
    check(f->replace_text("beta", "beta beta") && line_of(f, 2) == "beta beta" && line_of(f, 4) == "beta",
          "replace_text: `to` holding `from` is replaced once, the first line only", line_of(f, 2) + " / " + line_of(f, 4));
    check(f->replace_text("beta", "x") && line_of(f, 2) == "x beta", "replace_text: the first occurrence in the line",
          line_of(f, 2));
    check(f->replace_text("alphabet", "one\ntwo") && f->size() == 5 && line_of(f, 3) == "one" &&
              line_of(f, 4) == "two" && line_of(f, 5) == "beta",
          "replace_text: `to` holding a newline makes lines", said(f));
    expect_all("replace_text: the whole", f, "alpha\nx beta\none\ntwo\nbeta\n");
    check(f->close(), "search: close", said(f));
}

// ---------------------------------------------------------------------------
// 9. THE LIST WORDS AND THEIR EDGES
// ---------------------------------------------------------------------------
void the_list_words()
{
    const std::string p = at("list.txt");
    const Handle f = satellite_file::make_new(p, Kind::text);
    for (const char *text : {"one", "two", "three", "four", "five"})
        f->append(text);
    check(f->size() == 5 && !f->empty(), "list: five lines", said(f));
    std::string out = "stale";
    check(!f->line(0, out) && out.empty() && f->code() == line_past_the_end && contains_text(f->error(), "line 0"),
          "line(0): false, line_past_the_end", said(f));
    out = "stale";
    check(!f->line(6, out) && out.empty() && f->code() == line_past_the_end, "line(size()+1): false, line_past_the_end",
          said(f));
    check(f->line(5, out) && out == "five" && f->line(1, out) && out == "one", "line(1) and line(size())", out);
    check(f->append("after") && f->code() == line_past_the_end, "a later success does not clear the last failure",
          said(f));
    check(f->remove_last(), "list: remove_last");
    check(!f->insert(0, "x") && f->code() == line_past_the_end, "insert(0): false", said(f));
    check(!f->insert(7, "x") && f->size() == 5, "insert(size()+2): false, nothing added", said(f));
    check(f->insert(6, "six") && f->size() == 6 && line_of(f, 6) == "six", "insert(size()+1): an append", said(f));
    check(f->first(out) && out == "one" && f->last(out) && out == "six", "first and last", out);
    check(!f->remove_at(0) && !f->remove_at(7) && f->code() == line_past_the_end && f->size() == 6,
          "remove_at(0) and remove_at(size()+1): false", said(f));
    check(f->remove_at(2) && line_of(f, 2) == "three" && f->size() == 5, "remove_at(2): the lines below move up",
          said(f));
    check(f->remove("four") && f->size() == 4 && f->index_of("four") == 0, "remove: the first line exactly the text",
          said(f));
    check(!f->remove("absent") && f->code() == text_not_found && f->size() == 4, "remove of absent text: text_not_found",
          said(f));
    check(f->remove_first() && line_of(f, 1) == "three", "remove_first", said(f));
    check(f->remove_last() && f->size() == 2 && line_of(f, 2) == "five", "remove_last", said(f));
    check(f->truncate(5) && f->truncate(2) && f->size() == 2, "truncate(n >= size()): true, nothing changes", said(f));
    check(f->truncate(1) && f->size() == 1 && line_of(f, 1) == "three", "truncate(1): keeps the first line", said(f));
    expect_all("read_all: every line with its ending", f, "three\n");
    check(f->clear_lines() && f->size() == 0 && f->empty(), "clear_lines: no lines", said(f));
    check(f->clear_lines(), "clear_lines on no lines: true");
    check(!f->first(out) && !f->last(out) && f->code() == line_past_the_end, "first and last of no lines: false",
          said(f));
    check(!f->remove_first() && !f->remove_last(), "remove_first and remove_last of no lines: false");
    check(f->replace_line(1, "x") == false && f->code() == line_past_the_end, "replace_line past the end: false",
          said(f));
    check(f->close(), "list: close", said(f));
    expect_bytes("list: the file is there and empty", p, "");
}

// ---------------------------------------------------------------------------
// 10. OPEN, CLOSED, AND A SAVE THAT FAILS
// ---------------------------------------------------------------------------
void open_and_closed()
{
    const std::string p = at("life.txt");
    put(p, "a\n");
    Handle f = satellite_file::open_existing(p, Kind::text);
    std::string out;
    f->line(0, out);
    check(f->close() && !f->ok() && f->close(), "close twice: true both times", said(f));
    check(f->code() == line_past_the_end && f->path() == p, "a closed handle keeps its path, code and error", said(f));
    check(f->size() == 0 && f->code() == file_not_open, "closed: size() is 0, file_not_open", said(f));
    check(!f->line(1, out) && f->code() == file_not_open, "closed: line() is false, file_not_open", said(f));
    check(!f->append("x") && !f->insert(1, "x") && !f->replace_line(1, "x") && !f->replace_text("a", "b") &&
              !f->remove_at(1) && !f->remove("a") && !f->truncate(0) && !f->clear_lines() && f->code() == file_not_open,
          "closed: every changing word is false, file_not_open", said(f));
    check(f->read_all().empty() && f->index_of("a") == 0 && f->search("a") == 0 && !f->contains("a"),
          "closed: read_all is \"\", index_of and search 0");
    check(!f->save() && f->code() == file_not_open, "closed: save() is false, file_not_open", said(f));
    expect_bytes("closed: the file is as it was", p, "a\n");

    put(p, "b\nc\n");
    check(f->reopen() && f->ok() && f->code() == success && f->error().empty(),
          "reopen: true, and the last failure is cleared", said(f));
    check(f->size() == 2 && line_of(f, 1) == "b", "reopen: the disk is read again", said(f));
    check(f->reopen() && f->size() == 2, "reopen of an open handle: true, nothing happens");
    check(f->close(), "reopen: close");
    ::unlink(p.c_str());
    check(!f->reopen() && !f->ok() && f->code() == file_not_found, "reopen of a removed file: file_not_found", said(f));
    check(!there(p), "reopen of a removed file: nothing is made");

    put(p, "b\nc\n");
    {
        const Handle g = satellite_file::open_existing(p, Kind::text);
        g->append("from the destructor");
    }
    expect_bytes("the destructor saves an appended handle", p, "b\nc\nfrom the destructor\n");
    {
        const Handle g = satellite_file::open_existing(p, Kind::text);
        g->remove_first();
    }
    expect_bytes("the destructor saves an edited handle", p, "c\nfrom the destructor\n");

    if (::geteuid() == 0) {
        skipped("a save that fails leaves the file and keeps the changes", "running as root");
        return;
    }
    const std::string folder = at("read_only_folder");
    const std::string inside = folder + "/f.txt";
    ::mkdir(folder.c_str(), 0755);
    put(inside, "a\nb\n");
    f = satellite_file::open_existing(inside, Kind::text);
    check(f->insert(1, "z"), "failed save: insert");
    ::chmod(folder.c_str(), 0555);
    check(!f->save() && f->code() == file_unwritable && contains_text(f->error(), "Permission denied"),
          "failed save: false, file_unwritable, with the system's reason", said(f));
    expect_bytes("failed save: the old file is intact", inside, "a\nb\n");
    expect_all("failed save: the changes are still held", f, "z\na\nb\n");
    check(!f->close() && f->ok() && f->code() == file_unwritable, "failed save: close() is false and the handle stays open",
          said(f));
    check(leftovers(folder).empty(), "failed save: no .saving. file is left");
    ::chmod(folder.c_str(), 0755);
    check(f->close() && !f->ok(), "failed save: once the folder is writable, close() lands", said(f));
    expect_bytes("failed save: ... with the changes", inside, "z\na\nb\n");

    const std::string locked = at("read_only_file.txt");
    put(locked, "a\n");
    ::chmod(locked.c_str(), 0444);
    f = satellite_file::open_existing(locked, Kind::text);
    check(f->ok() && f->replace_line(1, "b") && !f->save() && f->code() == file_unwritable,
          "a read-only file: a rewrite is refused, though its folder is writable", said(f));
    check(leftovers(room).empty(), "a read-only file: no .saving. file is left");
    const Handle g = satellite_file::open_existing(locked, Kind::text);
    check(g->append("more") && !g->save() && g->code() == file_unwritable, "a read-only file: an append is refused",
          said(g));
    expect_bytes("a read-only file: untouched", locked, "a\n");
    ::chmod(locked.c_str(), 0644);
    check(f->close() && g->close(), "a read-only file made writable: both close", said(f) + "; " + said(g));
    expect_bytes("a read-only file made writable: the rewrite, then the append, land", locked, "b\nmore\n");
}

// ---------------------------------------------------------------------------
// 11. THE WORDS BY NAME
// ---------------------------------------------------------------------------
void by_name()
{
    const std::string p = at("clear_me.txt");
    put(p, "a\nb\n");
    std::string reason = "stale";
    check(satellite_file::clear(p, reason) == success && reason.empty(), "clear: success", reason);
    check(satellite_file::exists(p) && bytes_of(p).empty(), "clear: the file is there and empty");
    check(satellite_file::clear(at("clear_missing.txt"), reason) == file_not_found && !reason.empty(),
          "clear of a missing path: file_not_found", reason);
    check(!there(at("clear_missing.txt")), "clear of a missing path: nothing is made");
    ::mkdir(at("clear_folder").c_str(), 0755);
    check(satellite_file::clear(at("clear_folder"), reason) == not_a_file, "clear of a directory: not_a_file", reason);
    ::mkfifo(at("clear_fifo").c_str(), 0600);
    check(satellite_file::clear(at("clear_fifo"), reason) == not_a_file, "clear of a FIFO: not_a_file", reason);
    check(satellite_file::clear(p + kNul, reason) == path_holds_a_nul, "clear with a NUL in the path: path_holds_a_nul",
          reason);

    make_link("clear_me.txt", at("exists_link"));
    make_link("nothing_here", at("exists_dangling"));
    check(satellite_file::exists(p), "exists: a file");
    check(satellite_file::exists(at("exists_link")), "exists: a symlink to a file");
    check(!satellite_file::exists(at("clear_folder")), "exists: a directory is false");
    check(!satellite_file::exists(at("not_here.txt")), "exists: a missing path is false");
    check(!satellite_file::exists(at("exists_dangling")), "exists: a dangling symlink is false");
    check(!satellite_file::exists(at("clear_fifo")), "exists: a FIFO is false");
    check(!satellite_file::exists(p + kNul), "exists: a NUL in the path is false");

    const Handle f = satellite_file::open_existing(p, Kind::text);
    check(f->path_exists(), "path_exists: true while the file is there");
    ::unlink(p.c_str());
    check(!f->path_exists(), "path_exists: false once it is gone");
    check(f->close(), "path_exists: close with nothing changed", said(f));
}

int remove_one(const char *path, const struct stat *, int, struct FTW *)
{
    return ::remove(path);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: file_cases <a folder to work in>\n");
        return 2;
    }
    std::string pattern = std::string(argv[1]) + "/file_cases.XXXXXX";
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    if (::mkdtemp(name.data()) == nullptr) {
        std::printf("FAIL setup: could not make a folder in %s: %s\n", argv[1], std::strerror(errno));
        return 1;
    }
    // ABSOLUTE, so the case that changes directory still finds what it made.
    char *whole = ::realpath(name.data(), nullptr);
    room = whole != nullptr ? whole : name.data();
    std::free(whole);

    the_authors_example();
    new_never_clobbers();
    open_never_creates();
    endings_are_kept();
    only_text_opens();
    the_two_saves();
    appends_stream();
    where_lines_are();
    the_list_words();
    open_and_closed();
    by_name();

    ::chmod(at("read_only_folder").c_str(), 0755);
    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    check(!there(room), "cleanup: the folder it made is gone", room + " is still there");

    std::printf("%d ok, %d failed: %s\n", passed, failed, failed == 0 ? "every file case passed" : "a file case FAILED");
    return failed == 0 ? 0 : 1;
}
