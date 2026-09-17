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

constexpr std::size_t columns = 6;
const char *const headings[columns] = {"name", "type", "permissions", "owner", "created", "modified"};

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
    rows.push_back({headings[0], headings[1], headings[2], headings[3], headings[4], headings[5]});

    for (const std::string &name : names) {
        const std::string path = directory + "/" + name;
        struct stat about;
        // lstat, so a link lists as a link rather than as what it points at --
        // and a dangling link lists at all, which stat would refuse.
        if (::lstat(path.c_str(), &about) != 0) {
            rows.push_back({shown(name), "-", "-", "-", "-", "-"});
            continue;
        }
        rows.push_back({shown(name), type_of(about.st_mode), permissions_of(about.st_mode),
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
            out += rows[row][column];
            if (column + 1 < columns) {
                pad_to(out, cells[row][column], widths[column]);
                out += "  ";
            }
        }
        out += '\n';
    }
    return out;
}

} // namespace satellite004
