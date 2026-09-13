// The prompt's directory table. See satellite_directory/listing.hpp for why it
// is the prompt's and not the list's.

#include "satellite_directory/listing.hpp"

#include <fcntl.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <ctime>

namespace satellite::directory {

namespace {

thread_local bool listing_wanted = false;

// A cell that does not apply says so. A blank cell reads as a column that
// failed, and the reader cannot tell "no birth time was recorded" from "the
// listing broke" -- v1's rule, and its spelling.
constexpr const char *kNone = "- -";

// TEXT MEANS A PERSON CAN READ IT: no NUL byte, and the bytes are UTF-8 (ASCII
// is). Asked of the CONTENT and never the name, for the reason gzip.cpp sniffs
// its magic: a name is a claim and the bytes are the file.
//
// THE FIRST 64 KB ARE WHAT IS READ, which is a classification choice and not a
// limit on anything: file(1) decides the same question from the head of the
// file, and a listing that read every gigabyte to fill one cell would be a
// listing nobody typed twice. A file that is text for 64 KB and binary after it
// is called text, and this sentence is where that is admitted.
//
// A sequence cut in half by the end of the window is not held against the
// file -- that is the window's doing, not the content's.
constexpr size_t kSniff = 64 * 1024;

bool utf8_text(const unsigned char *bytes, size_t size, bool whole)
{
    size_t i = 0;
    while (i < size) {
        const unsigned char c = bytes[i];
        if (c == 0)
            return false;
        size_t follow = 0;
        if (c < 0x80)
            follow = 0;
        else if (c >= 0xc2 && c <= 0xdf)
            follow = 1;
        else if (c >= 0xe0 && c <= 0xef)
            follow = 2;
        else if (c >= 0xf0 && c <= 0xf4)
            follow = 3;
        else
            return false;
        if (i + follow >= size && follow != 0)
            return !whole;
        for (size_t k = 1; k <= follow; k++)
            if ((bytes[i + k] & 0xc0) != 0x80)
                return false;
        i += follow + 1;
    }
    return true;
}

struct Content {
    bool text = false;
    bool program = false;  // an ELF image or a #! script
};

// O_NONBLOCK and only ever on a REGULAR file, which the caller checks: a fifo
// or a device can block a read until the machine is rebooted, and the listing
// is not allowed to hang the prompt for a row.
Content sniff(const std::string &path)
{
    Content out;
    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOCTTY);
    if (fd < 0)
        return out;
    std::vector<unsigned char> buffer(kSniff);
    size_t got = 0;
    while (got < buffer.size()) {
        const ssize_t n = ::read(fd, buffer.data() + got, buffer.size() - got);
        if (n <= 0)
            break;
        got += static_cast<size_t>(n);
    }
    const bool whole = got < buffer.size();
    ::close(fd);

    out.program = (got >= 4 && std::memcmp(buffer.data(), "\x7f" "ELF", 4) == 0) ||
                  (got >= 2 && buffer[0] == '#' && buffer[1] == '!');
    out.text = utf8_text(buffer.data(), got, whole);
    return out;
}

// rwxr-xr-x, and ` exe` after it when the file is PROGRAM CODE the bits let
// somebody run. BOTH halves: a directory carries x and is not a program, and a
// shell script with no x bit is source nobody can execute by name.
std::string permissions(mode_t mode, bool program)
{
    static const char *const rwx[] = {"---", "--x", "-w-", "-wx",
                                      "r--", "r-x", "rw-", "rwx"};
    std::string out = rwx[(mode >> 6) & 7];
    out += rwx[(mode >> 3) & 7];
    out += rwx[mode & 7];
    if (S_ISREG(mode) && program && (mode & (S_IXUSR | S_IXGRP | S_IXOTH)))
        out += " exe";
    return out;
}

// The owner's NAME, or the number when the password database has none -- a
// file unpacked from another machine's archive, or a container's uid. The
// number is still the true answer; an empty cell would not be.
std::string owner(uid_t uid)
{
    std::vector<char> scratch(1024);
    for (;;) {
        passwd entry;
        passwd *found = nullptr;
        const int failed = ::getpwuid_r(uid, &entry, scratch.data(),
                                        scratch.size(), &found);
        if (failed == ERANGE) {
            scratch.resize(scratch.size() * 2);
            continue;
        }
        if (failed == 0 && found != nullptr && found->pw_name != nullptr)
            return found->pw_name;
        return std::to_string(uid);
    }
}

std::string date(time_t when)
{
    struct tm parts;
    char out[32];
    if (::localtime_r(&when, &parts) == nullptr ||
        std::strftime(out, sizeof out, "%Y-%m-%d %H:%M", &parts) == 0)
        return kNone;
    return out;
}

// CREATED IS statx(2)'s BIRTH TIME, which ext4, xfs and btrfs keep and stat(2)
// has never carried. WHERE IT IS MISSING THE CELL SAYS SO, and does NOT fall
// back to the modification time the way v1's single date column did: the row
// now has a modified column beside it, so the fallback would print the same
// instant twice under two names and call one of them a creation.
std::string created(const std::string &path, bool follow)
{
#if defined(__linux__) && defined(STATX_BTIME)
    struct statx sx;
    const int flags = AT_STATX_SYNC_AS_STAT | (follow ? 0 : AT_SYMLINK_NOFOLLOW);
    if (::statx(AT_FDCWD, path.c_str(), flags, STATX_BTIME, &sx) == 0 &&
        (sx.stx_mask & STATX_BTIME) != 0)
        return date(static_cast<time_t>(sx.stx_btime.tv_sec));
#endif
    (void)path;
    (void)follow;
    return kNone;
}

// Columns padded by CHARACTERS, not bytes, so a name with an accent in it does
// not push its row out of line. A continuation byte is not a character.
size_t width(const std::string &text)
{
    size_t n = 0;
    for (const char c : text)
        if ((static_cast<unsigned char>(c) & 0xc0) != 0x80)
            n++;
    return n;
}

} // namespace

void ask_for_listing(bool wanted) { listing_wanted = wanted; }

bool take_listing_request()
{
    const bool wanted = listing_wanted;
    listing_wanted = false;
    return wanted;
}

std::string listing(const std::string &directory,
                    const std::vector<std::string> &leaves)
{
    enum Column { NAME, TYPE, PERMISSIONS, OWNER, CREATED, MODIFIED, COLUMNS };
    static const char *const heading[COLUMNS] = {
        "name", "type", "permissions", "owner", "created", "modified"};

    std::vector<std::vector<std::string>> rows;
    rows.push_back(std::vector<std::string>(heading, heading + COLUMNS));

    const std::string base =
        directory.empty() || directory.back() == '/' ? directory
                                                     : directory + "/";
    for (const std::string &leaf : leaves) {
        const std::string path = base + leaf;
        std::vector<std::string> row(COLUMNS, kNone);
        row[NAME] = leaf;

        // stat FOLLOWS a link, so a link to a directory lists as the directory
        // it reaches -- which is what `change` into it will do. A DANGLING link
        // reaches nothing; lstat still describes the link itself, and its type
        // stays "- -" because there is no content to call text.
        struct stat info;
        bool follow = true;
        if (::stat(path.c_str(), &info) != 0) {
            follow = false;
            if (::lstat(path.c_str(), &info) != 0) {
                rows.push_back(std::move(row));
                continue;
            }
        }

        Content content;
        if (S_ISDIR(info.st_mode))
            row[TYPE] = "dir";
        else if (follow && S_ISREG(info.st_mode)) {
            content = sniff(path);
            if (content.text)
                row[TYPE] = "text";
        }
        row[PERMISSIONS] = permissions(info.st_mode, content.program);
        row[OWNER] = owner(info.st_uid);
        row[CREATED] = created(path, follow);
        row[MODIFIED] = date(info.st_mtime);
        rows.push_back(std::move(row));
    }

    // Widths from the rows themselves. A NAME IS NEVER TRUNCATED: a name with
    // its end cut off cannot be typed back in, which is what every other column
    // is there to help with.
    size_t widths[COLUMNS] = {};
    for (const auto &row : rows)
        for (int c = 0; c < COLUMNS; c++)
            widths[c] = std::max(widths[c], width(row[c]));

    std::string out;
    for (const auto &row : rows) {
        std::string line;
        for (int c = 0; c < COLUMNS; c++) {
            if (c == TYPE)
                line += "  ";
            else if (c > TYPE)
                line += " | ";
            line += row[c];
            if (c + 1 < COLUMNS)
                line.append(widths[c] - width(row[c]), ' ');
        }
        out += line;
        out += '\n';
    }
    // The last newline is display()'s to add.
    if (!out.empty())
        out.pop_back();
    return out;
}

} // namespace satellite::directory
