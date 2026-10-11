// The prompt's directory table. See listing.hpp.

#include "listing.hpp"
#include "listing_counts.hpp"
#include "listing_progress.hpp"

#include "../machine/filesystems.hpp"
#include "../machine/shown.hpp"
#include "../prompt/render.hpp"

#include <ctime>
#include <fcntl.h>
#include <limits>
#include <pwd.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/vfs.h>
#include <unistd.h>

namespace satellite004 {

namespace {

constexpr std::size_t columns = 9;
const char *const headings[columns] = {"name",        "type",  "size",    "files",   "sub",
                                       "permissions", "owner", "created", "modified"};
// THE NUMBERS ARE WRITTEN FROM THE RIGHT, so the decimal points of every kb and mb
// line up, and so do the counts.
constexpr bool from_the_right[columns] = {false, false, true, true, true, false, false, false, false};

std::size_t cells_of(const std::string &shown_text)
{
    // No width to wrap at: place() answers the column the text ends on, which is
    // its cells. The renderer counts a prompt line the same way.
    return prompt::place(shown_text, std::numeric_limits<std::size_t>::max(), prompt::Spot{}).column;
}

std::string permissions_of(mode_t mode)
{
    static const char *const rwx[] = {"---", "--x", "-w-", "-wx", "r--", "r-x", "rw-", "rwx"};
    std::string out = rwx[(mode >> 6) & 7];
    out += rwx[(mode >> 3) & 7];
    out += rwx[mode & 7];
    return out;
}

const char *type_of(mode_t mode)
{
    if (S_ISDIR(mode)) return "dir";
    if (S_ISLNK(mode)) return "link";
    if (S_ISFIFO(mode)) return "fifo";
    if (S_ISSOCK(mode)) return "sock";
    if (S_ISBLK(mode) || S_ISCHR(mode)) return "dev";
    if (S_ISREG(mode)) return "file";
    return "-";
}

// THE SIZE COLUMN, the author's rule of 2026-09-24. Under 1024 bytes a size is a
// whole number of bytes, the word written out in full. From 1024 bytes it is kb, and
// from 1024 kb it is mb: the bytes divided by 1024, or by 1024 and by 1024 again,
// rounded to the nearest thousandth -- so everything but bytes shows three places, and
// 1500 kb is 1.465 mb. The division is whole numbers, the whole part and then the
// remainder's thousandths rounded half up, so no size is ever off by a float.
//
// PAST mb THE SAME RULE GOES ON, as gb, tb, pb and eb, where a file's size ends. He
// named bytes, kb and mb; the rest is the guess that is easy to take back. A size
// that ROUNDS to 1024 of one unit is written as 1.000 of the next, so the column
// never shows 1024.000 mb.
//
// A DIRECTORY'S SIZE IS WHAT IS IN IT (the author, 2026-09-25): every file's size
// under it at every depth, added up -- see count_contents. Its own st_size, the block
// holding its names, is not shown anywhere. A link's is the length of the path it
// holds, so a link shows a dash, as a name that cannot be stat'ed does.
std::string size_of(unsigned long long int bytes)
{
    const SizeParts parts = size_parts(bytes);
    if (parts.unit == 0)
        return std::to_string(bytes) + (bytes == 1 ? " byte" : " bytes");
    std::string places = std::to_string(parts.thousandths);
    places.insert(0, 3 - places.size(), '0');
    return std::to_string(parts.whole) + "." + places + " " + size_unit_names[parts.unit];
}

// The name behind the number when the system knows one, and the number itself
// when it does not -- a listing of a directory owned by a deleted account still
// lists rather than stopping.
std::string owner_of(uid_t uid)
{
    const passwd *who = ::getpwuid(uid);
    return who != nullptr && who->pw_name != nullptr ? std::string(who->pw_name) : std::to_string(uid);
}

std::string moment(time_t at)
{
    tm broken{};
    if (::localtime_r(&at, &broken) == nullptr)
        return "-";
    char text[32];
    if (::strftime(text, sizeof text, "%Y-%m-%d %H:%M", &broken) == 0)
        return "-";
    return text;
}

// CREATED IS statx(2)'s BIRTH TIME, which ext4, xfs and btrfs keep and stat(2)
// does not answer. A filesystem that has none says so with a dash.
std::string created_at(const std::string &path)
{
    struct statx about;
    if (::statx(AT_FDCWD, path.c_str(), AT_SYMLINK_NOFOLLOW, STATX_BTIME, &about) == 0 &&
        (about.stx_mask & STATX_BTIME) != 0)
        return moment(static_cast<time_t>(about.stx_btime.tv_sec));
    return "-";
}

void pad_to(std::string &out, std::size_t cells, std::size_t width)
{
    out.append(width > cells ? width - cells : 0, ' ');
}

} // namespace

NameFacts facts_of(const std::string &path, const struct stat &about)
{
    return NameFacts{type_of(about.st_mode), permissions_of(about.st_mode), owner_of(about.st_uid), created_at(path),
                     moment(about.st_mtime)};
}

SizeParts size_parts(unsigned long long int bytes)
{
    SizeParts parts;
    if (bytes < 1024) {
        parts.whole = bytes;
        return parts;
    }
    constexpr std::size_t last_unit = sizeof size_unit_names / sizeof size_unit_names[0] - 1;
    parts.unit = 1;
    unsigned long long int unit = 1024;
    while (parts.unit < last_unit && bytes / unit >= 1024) {
        unit *= 1024;
        ++parts.unit;
    }
    parts.whole = bytes / unit;
    parts.thousandths = static_cast<unsigned long long int>(
        (static_cast<unsigned __int128>(bytes % unit) * 1000 + unit / 2) / unit);
    if (parts.thousandths == 1000) {
        ++parts.whole;
        parts.thousandths = 0;
    }
    if (parts.whole == 1024 && parts.unit < last_unit) {
        parts.whole = 1;
        ++parts.unit;
    }
    return parts;
}

std::string with_commas(unsigned long long int number)
{
    std::string digits = std::to_string(number);
    for (std::size_t at = digits.size(); at > 3; at -= 3)
        digits.insert(at - 3, 1, ',');
    return digits;
}

// ALWAYS mb: the bytes over 1024 twice, rounded to the nearest thousandth as size_of
// rounds, in whole numbers.
std::string megabytes_with_commas(unsigned long long int bytes)
{
    constexpr unsigned long long int megabyte = 1024ull * 1024ull;
    unsigned long long int whole = bytes / megabyte;
    unsigned long long int thousandths = ((bytes % megabyte) * 1000 + megabyte / 2) / megabyte;
    if (thousandths == 1000) {
        ++whole;
        thousandths = 0;
    }
    std::string places = std::to_string(thousandths);
    places.insert(0, 3 - places.size(), '0');
    return with_commas(whole) + "." + places + " mb";
}

std::string lined_up(const std::vector<std::vector<std::string>> &rows, const std::vector<bool> &from_the_right)
{
    const std::size_t count = from_the_right.size();
    std::vector<std::size_t> widths(count, 0);
    std::vector<std::vector<std::size_t>> cells(rows.size(), std::vector<std::size_t>(count, 0));
    for (std::size_t row = 0; row < rows.size(); ++row)
        for (std::size_t column = 0; column < count && column < rows[row].size(); ++column) {
            cells[row][column] = cells_of(rows[row][column]);
            if (cells[row][column] > widths[column])
                widths[column] = cells[row][column];
        }

    std::string out;
    for (std::size_t row = 0; row < rows.size(); ++row) {
        for (std::size_t column = 0; column < count && column < rows[row].size(); ++column) {
            if (from_the_right[column])
                pad_to(out, cells[row][column], widths[column]);
            out += rows[row][column];
            if (column + 1 < count) {
                if (!from_the_right[column])
                    pad_to(out, cells[row][column], widths[column]);
                out += "  ";
            }
        }
        out += '\n';
    }
    return out;
}

std::string free_space_line(const std::string &directory)
{
    struct statvfs about;
    if (::statvfs(directory.c_str(), &about) != 0)
        return "FREE SPACE IN DIRECTORY: -";
    const unsigned long long int block = about.f_frsize != 0 ? about.f_frsize : about.f_bsize;
    const unsigned __int128 bytes = static_cast<unsigned __int128>(about.f_bavail) * block;
    constexpr unsigned long long int most = std::numeric_limits<unsigned long long int>::max();
    return "FREE SPACE IN DIRECTORY: " + megabytes_with_commas(bytes > most ? most : static_cast<unsigned long long int>(bytes));
}

bool listing_table(const std::string &directory, const std::vector<std::string> &names,
                   const volatile sig_atomic_t *stop, bool at_a_terminal, std::string &table)
{
    table.clear();
    std::vector<std::vector<std::string>> rows;
    rows.reserve(names.size() + 1);
    rows.emplace_back(headings, headings + columns);
    std::vector<unsigned char> piece;   // count_lines' read buffer, one for the whole table

    // WHERE THE LISTED DIRECTORY IS. On a filesystem that stores nothing a file has no
    // size to show, except /proc/kcore at /proc's top (inode 1), which is the memory.
    // At the top of a filesystem that counts its names, that count is the walk's
    // total, less the rows themselves -- what the progress line measures against.
    struct statfs system {};
    struct stat here {};
    const bool known = ::statfs(directory.c_str(), &system) == 0;
    const bool nothing_stored = known && filesystems::stores_nothing(static_cast<unsigned long long int>(system.f_type));
    const bool at_the_top_of_proc = known &&
                                    static_cast<unsigned long long int>(system.f_type) == filesystems::proc_type &&
                                    ::stat(directory.c_str(), &here) == 0 && here.st_ino == 1;
    unsigned long long int total = 0;
    if (known && !nothing_stored && system.f_files > 0 && system.f_files >= system.f_ffree &&
        filesystems::is_a_mount_top(AT_FDCWD, directory.c_str())) {
        const unsigned long long int in_use = static_cast<unsigned long long int>(system.f_files - system.f_ffree);
        total = in_use > names.size() + 1 ? in_use - names.size() - 1 : 0;
    }
    ListingProgress progress(at_a_terminal, total);

    for (const std::string &name : names) {
        const std::string path = directory + "/" + name;
        struct stat about;
        // lstat, so a link lists as a link rather than as what it points at --
        // and a dangling link lists at all, which stat would refuse.
        if (::lstat(path.c_str(), &about) != 0) {
            rows.push_back({shown(name), "-", "-", "-", "-", "-", "-", "-", "-"});
            continue;
        }
        std::string size = "-", files = "-", sub = "-";
        if (S_ISREG(about.st_mode)) {
            if (!nothing_stored)
                size = size_of(static_cast<unsigned long long int>(about.st_size));
            else if (at_the_top_of_proc && name == "kcore")
                size = size_of(filesystems::memory_bytes());
            bool text = false;
            unsigned long long int lines = 0;
            if (!count_lines(path, stop, piece, text, lines))
                return false;
            if (text)
                sub = "(" + std::to_string(lines) + ")";
        } else if (S_ISDIR(about.st_mode)) {
            // A row that is a mount's top is another filesystem, so its names are
            // counted apart from the listed directory's total.
            const bool its_own = filesystems::is_a_mount_top(AT_FDCWD, path.c_str());
            Contents in;
            if (!count_contents(path, stop, its_own ? &progress.elsewhere : &progress.on_the_filesystem, in))
                return false;
            if (in.opened) {
                const char *const at_least = in.whole ? "" : "+";
                size = size_of(in.bytes) + (in.by_the_filesystem ? "" : at_least);
                files = std::to_string(in.files) + (in.files_whole ? "" : "+");
                if (in.about)
                    sub = "~" + std::to_string(in.below);
                else if (in.below > 0 || !in.whole)
                    sub = std::to_string(in.below) + at_least;
            }
        }
        const NameFacts facts = facts_of(path, about);
        rows.push_back({shown(name), facts.type, size, files, sub, facts.permissions, shown(facts.owner), facts.created,
                        facts.modified});
    }

    table = lined_up(rows, std::vector<bool>(from_the_right, from_the_right + columns));
    return true;
}

} // namespace satellite004
