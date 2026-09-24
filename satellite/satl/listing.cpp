// The prompt's directory table. See listing.hpp.

#include "listing.hpp"

#include "../machine/shown.hpp"
#include "../prompt/render.hpp"

#include <ctime>
#include <fcntl.h>
#include <limits>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite004 {

namespace {

constexpr std::size_t columns = 7;
const char *const headings[columns] = {"name", "type", "size", "permissions", "owner", "created", "modified"};
// SIZE IS WRITTEN FROM THE RIGHT, so the decimal points of every kb and mb line up.
constexpr bool from_the_right[columns] = {false, false, true, false, false, false, false};

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
// ONLY A FILE HAS A SIZE HERE. A directory's own st_size is the block holding its
// names, not what is in it, and adding up what is in it is a walk where a listing
// costs one stat an entry (D0.6.7); a link's is the length of the path it holds.
// Both show a dash, as a name that cannot be stat'ed does.
std::string size_of(unsigned long long int bytes)
{
    if (bytes < 1024)
        return std::to_string(bytes) + (bytes == 1 ? " byte" : " bytes");
    static const char *const units[] = {"kb", "mb", "gb", "tb", "pb", "eb"};
    constexpr std::size_t unit_count = sizeof units / sizeof units[0];
    std::size_t which = 0;
    unsigned long long int unit = 1024;
    while (which + 1 < unit_count && bytes / unit >= 1024) {
        unit *= 1024;
        ++which;
    }
    unsigned long long int whole = bytes / unit;
    unsigned long long int thousandths = static_cast<unsigned long long int>(
        (static_cast<unsigned __int128>(bytes % unit) * 1000 + unit / 2) / unit);
    if (thousandths == 1000) {
        ++whole;
        thousandths = 0;
    }
    if (whole == 1024 && which + 1 < unit_count) {
        whole = 1;
        ++which;
    }
    std::string places = std::to_string(thousandths);
    places.insert(0, 3 - places.size(), '0');
    return std::to_string(whole) + "." + places + " " + units[which];
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

std::string listing_table(const std::string &directory, const std::vector<std::string> &names)
{
    std::vector<std::vector<std::string>> rows;
    rows.reserve(names.size() + 1);
    rows.push_back({headings[0], headings[1], headings[2], headings[3], headings[4], headings[5], headings[6]});

    for (const std::string &name : names) {
        const std::string path = directory + "/" + name;
        struct stat about;
        // lstat, so a link lists as a link rather than as what it points at --
        // and a dangling link lists at all, which stat would refuse.
        if (::lstat(path.c_str(), &about) != 0) {
            rows.push_back({shown(name), "-", "-", "-", "-", "-", "-"});
            continue;
        }
        const std::string size =
            S_ISREG(about.st_mode) ? size_of(static_cast<unsigned long long int>(about.st_size)) : std::string("-");
        rows.push_back({shown(name), type_of(about.st_mode), size, permissions_of(about.st_mode),
                        shown(owner_of(about.st_uid)), created_at(path), moment(about.st_mtime)});
    }

    std::vector<std::size_t> widths(columns, 0);
    std::vector<std::vector<std::size_t>> cells(rows.size(), std::vector<std::size_t>(columns, 0));
    for (std::size_t row = 0; row < rows.size(); ++row)
        for (std::size_t column = 0; column < columns; ++column) {
            cells[row][column] = cells_of(rows[row][column]);
            if (cells[row][column] > widths[column])
                widths[column] = cells[row][column];
        }

    std::string out;
    for (std::size_t row = 0; row < rows.size(); ++row) {
        for (std::size_t column = 0; column < columns; ++column) {
            if (from_the_right[column])
                pad_to(out, cells[row][column], widths[column]);
            out += rows[row][column];
            if (column + 1 < columns) {
                if (!from_the_right[column])
                    pad_to(out, cells[row][column], widths[column]);
                out += "  ";
            }
        }
        out += '\n';
    }
    return out;
}

} // namespace satellite004
