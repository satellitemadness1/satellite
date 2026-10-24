// satellite/satellite_variable_file/file_moves.cpp -- see file_moves.hpp.

#include "file_moves.hpp"

#include "../machine/as_another_account.hpp"
#include "../machine/ask_at_the_console.hpp"
#include "../machine/stop_flag.hpp"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <linux/fs.h>
#include <pwd.h>
#include <string>
#include <utility>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {
namespace {

// strerror_r COMES IN TWO SHAPES (satellite_file.cpp says the same): whichever this library has, one takes it.
[[maybe_unused]] const char *reason_text(const char *gnu, const char *) { return gnu; }
[[maybe_unused]] const char *reason_text(int xsi, const char *buffer) { return xsi == 0 ? buffer : "an unknown error"; }

std::string system_reason(int number)
{
    char buffer[256] = {};
    return reason_text(::strerror_r(number, buffer, sizeof buffer), buffer);
}

std::string folder_of(const std::string &path)
{
    const std::size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return ".";
    return slash == 0 ? "/" : path.substr(0, slash);
}

std::string name_of(const std::string &path)
{
    const std::size_t slash = path.find_last_of('/');
    return slash == std::string::npos ? path : path.substr(slash + 1);
}

// THE PATH FROM THE ROOT, its folder's links worked out and its own name kept -- so a link is named and never
// what it points to, and su and sudo, which may start somewhere else, delete the same file.
std::string whole_path(const std::string &path)
{
    char resolved[PATH_MAX];
    if (::realpath(folder_of(path).c_str(), resolved) == nullptr)
        return path;
    const std::string folder(resolved);
    return (folder == "/" ? folder : folder + "/") + name_of(path);
}

// AN ACCOUNT A COMMAND CAN BE RUN AS through su: one with a shell that runs commands. /usr/sbin/nologin and
// /bin/false refuse every command, so the password would be asked for nothing.
bool can_run_commands(const passwd *who)
{
    if (who == nullptr || who->pw_shell == nullptr || who->pw_shell[0] == '\0')
        return false;
    const std::string shell(who->pw_shell);
    const std::string last = name_of(shell);
    return last != "nologin" && last != "false";
}

// IMMUTABLE OR APPEND-ONLY (chattr +i, +a), on the file or on its folder: the kernel refuses even root. Asked of
// a regular file only, because opening a device to ask may do something to it; a file this account may not read
// cannot be asked, and is left to the password.
const char *marked_against_deleting(const std::string &whole, bool regular)
{
    const auto marks = [](int fd) {
        unsigned int flags = 0;
        const bool known = fd >= 0 && ::ioctl(fd, FS_IOC_GETFLAGS, &flags) == 0;
        if (fd >= 0)
            ::close(fd);
        return known ? flags : 0u;
    };
    const unsigned int folder = marks(::open(folder_of(whole).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC));
    if ((folder & FS_IMMUTABLE_FL) != 0)
        return "its folder is marked immutable";
    if ((folder & FS_APPEND_FL) != 0)
        return "its folder is marked append-only";
    const unsigned int file =
        regular ? marks(::open(whole.c_str(), O_RDONLY | O_NOFOLLOW | O_NONBLOCK | O_NOCTTY | O_CLOEXEC)) : 0u;
    if ((file & FS_IMMUTABLE_FL) != 0)
        return "it is marked immutable";
    if ((file & FS_APPEND_FL) != 0)
        return "it is marked append-only";
    return nullptr;
}

std::string my_name()
{
    const passwd *me = ::getpwuid(::getuid());
    return me != nullptr && me->pw_name != nullptr ? std::string(me->pw_name) : std::to_string(::getuid());
}

} // namespace

DeleteAsker who_may_delete(const struct stat &file, const struct stat &folder, uid_t me)
{
    DeleteAsker asker;
    asker.owner = (folder.st_mode & S_ISVTX) != 0 && file.st_uid != folder.st_uid ? file.st_uid : folder.st_uid;
    const passwd *who = ::getpwuid(asker.owner);
    asker.account = who != nullptr && who->pw_name != nullptr ? std::string(who->pw_name) : std::to_string(asker.owner);
    asker.through_su = asker.owner != me && can_run_commands(who);
    return asker;
}

DiskAnswer delete_a_file(const std::string &path, const std::string &su, const std::string &sudo)
{
    DiskAnswer answer;
    if (path.find('\0') != std::string::npos) {
        answer.why = "the path holds a NUL, which no file's name can: nothing was deleted";
        return answer;
    }
    // ONE PATH FROM HERE ON, its folder's links worked out now: the stat that says whose it is, the question and
    // the command all name the same file, whatever a link in the path is changed to while the password is typed
    // (FD-2's reader).
    const std::string whole = whole_path(path);
    struct stat about;
    if (::lstat(whole.c_str(), &about) != 0) {
        const int why = errno;
        answer.why = why == ENOENT || why == ENOTDIR ? "there is no file " + path
                                                     : "could not reach " + path + ": " + system_reason(why);
        return answer;
    }
    if (S_ISDIR(about.st_mode)) {
        answer.why = path + " is a folder, and satellite.file.delete deletes files";
        return answer;
    }
    if (::unlink(whole.c_str()) == 0) {
        answer.done = true;
        return answer;
    }
    const int refused = errno;
    if (refused != EACCES && refused != EPERM) {
        answer.why = "could not delete " + path + ": " + system_reason(refused);
        return answer;
    }
    // MARKED SO THAT NOBODY MAY, root included: no password would help, so none is asked (rule 3).
    if (const char *mark = marked_against_deleting(whole, S_ISREG(about.st_mode)); mark != nullptr) {
        answer.why = std::string(mark) + ", so nobody may delete " + path + " -- root neither";
        return answer;
    }

    // WHO MAY: the folder's owner, or in a sticky folder the file's.
    struct stat folder;
    if (::stat(folder_of(whole).c_str(), &folder) != 0) {
        answer.why = "could not delete " + path + ": " + system_reason(refused);
        return answer;
    }
    const DeleteAsker asker = who_may_delete(about, folder, ::getuid());
    const std::string asked_for = asker.through_su ? asker.account : my_name();
    if (!a_console_to_ask_at()) {
        answer.why = path + " is not yours to delete, and there is no console to ask " + asked_for +
                     "'s password on, so it was not deleted";
        return answer;
    }
    // THE QUESTION SAYS WHOSE PASSWORD, AND WHY THAT ONE.
    const std::string &account = asker.account;
    const std::string question = asker.through_su ? path + " is " + account + "'s to delete -- password for " + account
                                 : asker.owner == ::getuid()
                                     ? path + " is in a folder of yours that will not let it go -- [sudo] password for " +
                                           asked_for
                                     : path + " is " + account + "'s to delete, and " + account +
                                           " cannot log in -- [sudo] password for " + asked_for;
    std::string password;
    const bool typed = hidden_line(question + ": ", password);
    if (!typed || password.empty()) {
        explicit_bzero(password.data(), password.size());
        answer.why = "no password was typed, so " + path + " was not deleted";
        return answer;
    }
    const std::vector<std::string> unlink_it{"/usr/bin/unlink", "--", whole};
    const AccountRun run = asker.through_su ? as_owner(account, password, unlink_it, su)
                                            : with_sudo(password, unlink_it, sudo);
    explicit_bzero(password.data(), password.size());
    struct stat still;
    answer.done = run.done && ::lstat(whole.c_str(), &still) != 0;
    if (!answer.done)
        answer.why = path + " was not deleted: " + (run.why.empty() ? std::string("it is still there") : run.why);
    return answer;
}

// ---------------------------------------------------------------------------------------------------------------
// THE CLIPBOARD (FD-3, rule 5): $TMPDIR/.satl/tmp, which is /tmp/.satl/tmp with TMPDIR unset -- the author's
// "transfer the file to /tmp/.satl/tmp/file-current and keep a /tmp/.satl/tmp/file-previous-001 with no
// extension, just a number", every copy kept ("just keep every file in /tmp"). Each folder is opened, never
// followed, and must be the person's own: a /tmp/.satl another account made, or a link put there, is refused.
// ---------------------------------------------------------------------------------------------------------------

namespace {

constexpr char kCurrent[] = "file-current";
constexpr char kNames[] = "names";

std::string temporary_base()
{
    const char *given = std::getenv("TMPDIR");
    return given != nullptr && given[0] == '/' ? std::string(given) : std::string("/tmp");
}

// ONE FOLDER OF THE CLIPBOARD'S, made 0700 if it is not there, opened without following a link, and refused
// unless it is a folder of the person's own that nobody else may write. ONE THAT IS THEIRS BUT OPEN TO BE READ
// (made 0755 by hand, or by an older satl) IS MADE 0700 (the review, 2026-10-23): a copy keeps its file's mode,
// and the folder is what keeps a copy of a private file private.
int own_folder(int parent, const std::string &name, const std::string &shown_as, std::string &why)
{
    if (::mkdirat(parent, name.c_str(), 0700) != 0 && errno != EEXIST) {
        why = "could not make " + shown_as + ": " + system_reason(errno);
        return -1;
    }
    const int fd = ::openat(parent, name.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) {
        why = shown_as + (errno == ELOOP || errno == ENOTDIR ? " is not a folder" : " cannot be opened: " +
                                                                                       system_reason(errno)) +
              ", so nothing can be kept on the clipboard";
        return -1;
    }
    struct stat about;
    if (::fstat(fd, &about) != 0 || about.st_uid != ::getuid() || (about.st_mode & 022) != 0) {
        ::close(fd);
        why = shown_as + " is not a folder of yours alone, so nothing can be kept on the clipboard";
        return -1;
    }
    if ((about.st_mode & 077) != 0 && ::fchmod(fd, 0700) != 0) {
        why = "could not make " + shown_as + " yours alone (0700): " + system_reason(errno) +
              ", so nothing can be kept on the clipboard";
        ::close(fd);
        return -1;
    }
    return fd;
}

int clipboard(std::string &why)
{
    const std::string base = temporary_base();
    const int top = ::open(base.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (top < 0) {
        why = "could not open " + base + ": " + system_reason(errno);
        return -1;
    }
    const int satl = own_folder(top, ".satl", base + "/.satl", why);
    ::close(top);
    if (satl < 0)
        return -1;
    const int tmp = own_folder(satl, "tmp", base + "/.satl/tmp", why);
    ::close(satl);
    return tmp;
}

// A NAME KEPT ON ONE LINE: a newline, a tab and a backslash in it are written \n, \t and \\.
std::string kept_as(const std::string &name)
{
    std::string out;
    for (const char c : name)
        out += c == '\n' ? std::string("\\n") : c == '\t' ? std::string("\\t") : c == '\\' ? std::string("\\\\")
                                                                                              : std::string(1, c);
    return out;
}

std::string read_back(const std::string &kept)
{
    std::string out;
    for (std::size_t i = 0; i < kept.size(); ++i) {
        if (kept[i] != '\\' || i + 1 == kept.size()) {
            out += kept[i];
            continue;
        }
        const char next = kept[++i];
        out += next == 'n' ? '\n' : next == 't' ? '\t' : next;
    }
    return out;
}

std::string whole_of(int fd)
{
    std::string text;
    char buffer[4096];
    ssize_t got;
    while ((got = ::read(fd, buffer, sizeof buffer)) > 0)
        text.append(buffer, static_cast<std::size_t>(got));
    return text;
}

// `names`: one line each, `<its file><TAB><its first name>`.
std::vector<std::pair<std::string, std::string>> names_in(int folder)
{
    std::vector<std::pair<std::string, std::string>> names;
    const int fd = ::openat(folder, kNames, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0)
        return names;
    const std::string text = whole_of(fd);
    ::close(fd);
    std::size_t at = 0;
    while (at < text.size()) {
        std::size_t end = text.find('\n', at);
        if (end == std::string::npos)
            end = text.size();
        const std::string line = text.substr(at, end - at);
        const std::size_t tab = line.find('\t');
        if (tab != std::string::npos)
            names.emplace_back(line.substr(0, tab), read_back(line.substr(tab + 1)));
        at = end + 1;
    }
    return names;
}

// WRITTEN BESIDE AND RENAMED INTO PLACE, so `names` is the old one or the new one, never half. 0, or the errno.
int write_names(int folder, const std::vector<std::pair<std::string, std::string>> &names)
{
    std::string text;
    for (const auto &[file, name] : names)
        text += file + "\t" + kept_as(name) + "\n";
    const std::string beside = std::string(".") + kNames + "." + std::to_string(::getpid());
    const int fd = ::openat(folder, beside.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_NOFOLLOW | O_CLOEXEC, 0600);
    if (fd < 0)
        return errno;
    int failed = 0;
    for (std::size_t at = 0; failed == 0 && at < text.size();) {
        const ssize_t put = ::write(fd, text.data() + at, text.size() - at);
        if (put < 0 && errno == EINTR)
            continue;
        if (put <= 0)
            failed = put < 0 ? errno : ENOSPC;
        else
            at += static_cast<std::size_t>(put);
    }
    if (::close(fd) != 0 && failed == 0)
        failed = errno;
    if (failed == 0 && ::renameat(folder, beside.c_str(), folder, kNames) != 0)
        failed = errno;
    if (failed != 0)
        ::unlinkat(folder, beside.c_str(), 0);
    return failed;
}

std::string name_for(const std::vector<std::pair<std::string, std::string>> &names, const std::string &file)
{
    for (const auto &[kept, name] : names)
        if (kept == file)
            return name;
    return std::string();
}

// THE NEXT PREVIOUS NUMBER: one past the highest file-previous-NNN there, so the highest is always the newest and
// a gap the cleaning left is never filled.
unsigned long next_previous(int folder)
{
    unsigned long highest = 0;
    const int fd = ::dup(folder);
    DIR *listing = fd >= 0 ? ::fdopendir(fd) : nullptr;
    if (listing == nullptr) {
        if (fd >= 0)
            ::close(fd);
        return 1;
    }
    static const std::string prefix = "file-previous-";
    while (const dirent *entry = ::readdir(listing)) {
        const std::string name(entry->d_name);
        if (name.compare(0, prefix.size(), prefix) != 0 || name.size() == prefix.size())
            continue;
        const std::string digits = name.substr(prefix.size());
        if (digits.find_first_not_of("0123456789") != std::string::npos || digits.size() > 9)
            continue;
        highest = std::max(highest, std::stoul(digits));
    }
    ::closedir(listing);
    return highest + 1;
}

std::string previous_name(unsigned long number)
{
    char text[32];
    std::snprintf(text, sizeof text, "file-previous-%03lu", number);
    return text;
}

bool asked_to_stop()
{
    const volatile sig_atomic_t *flag = stop_flag();
    return flag != nullptr && *flag != 0;
}

// THE BYTES OF ONE OPEN FILE INTO ANOTHER, by copy_file_range where the kernel can, by read and write where it
// cannot; Ctrl-C (the session's flag) stops it between pieces. 0, or the errno that stopped it (EINTR for Ctrl-C).
int copy_bytes(int from, int to)
{
    bool ranged = true;
    for (;;) {
        if (asked_to_stop())
            return EINTR;
        if (ranged) {
            const ssize_t moved = ::copy_file_range(from, nullptr, to, nullptr, 8 << 20, 0);
            if (moved > 0)
                continue;
            if (moved == 0)
                return 0;
            if (errno == EINTR)
                continue;
            if (errno != EXDEV && errno != EINVAL && errno != ENOSYS && errno != EOPNOTSUPP && errno != EBADF)
                return errno;
            ranged = false;
        }
        char buffer[1 << 16];
        const ssize_t got = ::read(from, buffer, sizeof buffer);
        if (got < 0 && errno == EINTR)
            continue;
        if (got < 0)
            return errno;
        if (got == 0)
            return 0;
        for (ssize_t at = 0; at < got;) {
            const ssize_t put = ::write(to, buffer + at, static_cast<std::size_t>(got - at));
            if (put < 0 && errno == EINTR)
                continue;
            if (put <= 0)
                return put < 0 ? errno : EIO;
            at += put;
        }
    }
}

// A FILE WRITTEN BESIDE ITS PLACE AND RENAMED INTO IT, so a stopped or failed copy leaves what was there: `from`
// copied into `folder_fd`/`name`. 0, or the errno. ITS MODE IS THE ONE cp GIVES (the review, 2026-10-23): the
// source's nine permission bits, made with the file and so through the umask -- never setuid, setgid or sticky,
// so a program copied from somebody else never runs as the person who pasted it.
int place_a_copy(int from, int folder_fd, const std::string &name, mode_t mode)
{
    const std::string beside = "." + (name.size() > 100 ? name.substr(0, 100) : name) + ".satl-" +
                               std::to_string(::getpid());
    ::unlinkat(folder_fd, beside.c_str(), 0);
    const int to = ::openat(folder_fd, beside.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                            mode & 0777);
    if (to < 0)
        return errno;
    int failed = copy_bytes(from, to);
    if (::close(to) != 0 && failed == 0)
        failed = errno;
    if (failed == 0 && ::renameat(folder_fd, beside.c_str(), folder_fd, name.c_str()) != 0)
        failed = errno;
    if (failed != 0)
        ::unlinkat(folder_fd, beside.c_str(), 0);
    return failed;
}

std::string why_copy_stopped(int failed, const std::string &what)
{
    return failed == EINTR ? what + " was stopped, and nothing was changed" : what + ": " + system_reason(failed);
}

// A FILE OPENED FOR ITS BYTES ONLY AS THE FILE IT WAS SEEN TO BE (the review, 2026-10-23): the caller has looked at
// what the path names, because a FIFO's open waits for somebody to write and a device's open can do something.
// O_NONBLOCK and O_NOCTTY cover the moment between, and what was opened must still be that regular file.
// `no_follow` opens a link itself. -1 when it is not, with `why` said.
int open_for_its_bytes(const std::string &path, bool no_follow, const struct stat &seen, std::string &why)
{
    const int fd = ::open(path.c_str(), O_RDONLY | O_NONBLOCK | O_NOCTTY | O_CLOEXEC | (no_follow ? O_NOFOLLOW : 0));
    if (fd < 0) {
        why = "could not read " + path + ": " + system_reason(errno);
        return -1;
    }
    struct stat now{};
    if (::fstat(fd, &now) != 0 || !S_ISREG(now.st_mode) || now.st_dev != seen.st_dev || now.st_ino != seen.st_ino) {
        ::close(fd);
        why = path + " changed as it was opened, so nothing was done";
        return -1;
    }
    ::fcntl(fd, F_SETFL, ::fcntl(fd, F_GETFL) & ~O_NONBLOCK);
    return fd;
}

// A FOLDER TO PUT A FILE IN, opened.
int folder_to_put_in(const std::string &written, std::string &why)
{
    const int fd = ::open(written.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (fd >= 0)
        return fd;
    const int reason = errno;
    why = reason == ENOENT ? "there is no folder " + written
          : reason == ENOTDIR ? written + " is a file, not a folder"
                              : "could not open " + written + ": " + system_reason(reason);
    return -1;
}

// WHETHER TO REPLACE A FILE ALREADY THERE (rule 6): asked, and only y replaces it. A folder of that name is
// never replaced.
bool may_replace(int folder_fd, const std::string &name, const std::string &folder_shown, DiskAnswer &answer)
{
    struct stat there;
    if (::fstatat(folder_fd, name.c_str(), &there, AT_SYMLINK_NOFOLLOW) != 0)
        return true;
    const std::string where = folder_shown + "/" + name;
    if (S_ISDIR(there.st_mode)) {
        answer.why = where + " is a folder, so no file can take its place";
        return false;
    }
    if (!a_console_to_ask_at()) {
        answer.why = where + " is already there, and there is no console to ask whether to replace it, so it was kept";
        return false;
    }
    if (!yes_or_no("\"" + name + "\" is already in " + folder_shown + " -- replace it?")) {
        answer.why = where + " is already there, and it was kept";
        return false;
    }
    return true;
}

// A NUL ENDS A NAME FOR THE SYSTEM, so the part before it would be taken -- refused, as delete refuses it.
bool holds_a_nul(const std::string &text, bool a_folder, const char *nothing_was, DiskAnswer &answer)
{
    if (text.find('\0') == std::string::npos)
        return false;
    answer.why = a_folder ? std::string("the folder holds a NUL, which no folder's name can: nothing was ") + nothing_was
                          : std::string("the path holds a NUL, which no file's name can: nothing was ") + nothing_was;
    return true;
}

} // namespace

DiskAnswer copy_a_file(const std::string &path)
{
    DiskAnswer answer;
    if (holds_a_nul(path, false, "copied", answer))
        return answer;
    struct stat about{};
    if (::stat(path.c_str(), &about) != 0) {
        const int why = errno;
        answer.why = why == ENOENT || why == ENOTDIR ? "there is no file " + path
                                                     : "could not read " + path + ": " + system_reason(why);
        return answer;
    }
    if (!S_ISREG(about.st_mode)) {
        answer.why = path + (S_ISDIR(about.st_mode) ? " is a folder, and satellite.file.copy copies files"
                                                    : " is not a file, and satellite.file.copy copies files");
        return answer;
    }
    const int from = open_for_its_bytes(path, false, about, answer.why);
    if (from < 0)
        return answer;
    const int folder = clipboard(answer.why);
    if (folder < 0) {
        ::close(from);
        return answer;
    }
    // THE COPY FIRST, under a name of this satl's own: until it is whole, the clipboard is as it was.
    const std::string incoming = ".file-incoming-" + std::to_string(::getpid());
    const int failed = place_a_copy(from, folder, incoming, about.st_mode);
    ::close(from);
    if (failed != 0) {
        ::close(folder);
        answer.why = why_copy_stopped(failed, "the copy of " + path);
        return answer;
    }
    // THEN ONE satl AT A TIME (the review, 2026-10-23): file-current, the next previous number and `names` are read
    // and changed under the clipboard folder's own lock, so a second satl copying at the same moment waits for this
    // one, and no copy is lost or numbered twice. Closing the folder lets it go. Each step that fails puts back
    // the ones before it, so a copy is all done or not at all.
    ::flock(folder, LOCK_EX);
    std::vector<std::pair<std::string, std::string>> names = names_in(folder);
    names.erase(std::remove_if(names.begin(), names.end(),
                               [folder](const auto &kept) {   // the ones the cleaning removed
                                   struct stat there;
                                   return ::fstatat(folder, kept.first.c_str(), &there, AT_SYMLINK_NOFOLLOW) != 0;
                               }),
                names.end());
    std::string previous;
    struct stat current;
    if (::fstatat(folder, kCurrent, &current, AT_SYMLINK_NOFOLLOW) == 0) {
        previous = previous_name(next_previous(folder));
        if (::renameat2(folder, kCurrent, folder, previous.c_str(), RENAME_NOREPLACE) != 0) {
            answer.why = "could not keep the copy before it as " + previous + ": " + system_reason(errno) +
                         ", so nothing was copied";
            ::unlinkat(folder, incoming.c_str(), 0);
            ::close(folder);
            return answer;
        }
    }
    if (::renameat(folder, incoming.c_str(), folder, kCurrent) != 0) {
        answer.why = "could not keep the copy of " + path + ": " + system_reason(errno);
        if (!previous.empty())
            ::renameat(folder, previous.c_str(), folder, kCurrent);
        ::unlinkat(folder, incoming.c_str(), 0);
        ::close(folder);
        return answer;
    }
    for (auto &kept : names)
        if (kept.first == kCurrent)
            kept.first = previous;
    names.erase(std::remove_if(names.begin(), names.end(), [](const auto &kept) { return kept.first.empty(); }),
                names.end());
    names.emplace_back(kCurrent, name_of(path));
    if (const int unwritten = write_names(folder, names); unwritten != 0) {
        answer.why = "could not keep the name of the copy of " + path + ": " + system_reason(unwritten) +
                     ", so nothing was copied";
        ::renameat(folder, kCurrent, folder, incoming.c_str());
        if (!previous.empty())
            ::renameat(folder, previous.c_str(), folder, kCurrent);
        ::unlinkat(folder, incoming.c_str(), 0);
        ::close(folder);
        return answer;
    }
    ::close(folder);
    answer.done = true;
    return answer;
}

DiskAnswer paste_a_file(const std::string &into)
{
    DiskAnswer answer;
    if (into.empty()) {
        answer.why = "no folder was named to paste into; satellite.file.paste() pastes into the folder you are in";
        return answer;
    }
    if (holds_a_nul(into, true, "pasted", answer))
        return answer;
    const int folder = clipboard(answer.why);
    if (folder < 0)
        return answer;
    ::flock(folder, LOCK_SH);   // file-current and its name, as one copy left them
    const int from = ::openat(folder, kCurrent, O_RDONLY | O_NOFOLLOW | O_CLOEXEC);
    std::string name = name_for(names_in(folder), kCurrent);
    ::close(folder);
    if (from < 0) {
        answer.why = "nothing has been copied to paste";
        return answer;
    }
    if (name.empty() || name == "." || name == ".." || name.find('/') != std::string::npos)
        name = kCurrent;
    struct stat about{};
    if (::fstat(from, &about) != 0)
        about.st_mode = 0600;
    const int to = folder_to_put_in(into, answer.why);
    if (to < 0) {
        ::close(from);
        return answer;
    }
    if (may_replace(to, name, into, answer)) {
        const int failed = place_a_copy(from, to, name, about.st_mode);
        answer.done = failed == 0;
        if (!answer.done)
            answer.why = why_copy_stopped(failed, "the paste of " + name + " into " + into);
    }
    ::close(from);
    ::close(to);
    return answer;
}

DiskAnswer move_a_file(const std::string &path, const std::string &into)
{
    DiskAnswer answer;
    if (holds_a_nul(path, false, "moved", answer) || holds_a_nul(into, true, "moved", answer))
        return answer;
    if (into.empty()) {
        answer.why = "no folder was named to move " + path + " into";
        return answer;
    }
    struct stat about;
    if (::lstat(path.c_str(), &about) != 0) {
        const int why = errno;
        answer.why = why == ENOENT || why == ENOTDIR ? "there is no file " + path
                                                     : "could not reach " + path + ": " + system_reason(why);
        return answer;
    }
    if (S_ISDIR(about.st_mode)) {
        answer.why = path + " is a folder, and satellite.file.move moves files";
        return answer;
    }
    const int to = folder_to_put_in(into, answer.why);
    if (to < 0)
        return answer;
    const std::string name = name_of(path);
    // IN THAT FOLDER ALREADY when its own folder is the one named -- the folders compared, not the files (the
    // review, 2026-10-23): a hard link of the same name in another folder is the same file there, and the move
    // refuses it as mv does, rather than answer done and leave both.
    struct stat its_folder{}, target{}, there{};
    if (::stat(folder_of(path).c_str(), &its_folder) == 0 && ::fstat(to, &target) == 0 &&
        its_folder.st_dev == target.st_dev && its_folder.st_ino == target.st_ino) {
        ::close(to);
        answer.done = true;
        return answer;
    }
    if (::fstatat(to, name.c_str(), &there, AT_SYMLINK_NOFOLLOW) == 0 && there.st_dev == about.st_dev &&
        there.st_ino == about.st_ino) {
        ::close(to);
        answer.why = path + " and " + into + "/" + name + " are the same file, so nothing was moved";
        return answer;
    }
    // NEVER OVER A FILE UNASKED: the move refuses one already there, and asks.
    if (::renameat2(AT_FDCWD, path.c_str(), to, name.c_str(), RENAME_NOREPLACE) == 0) {
        ::close(to);
        answer.done = true;
        return answer;
    }
    int refused = errno;
    bool asked = false;
    if (refused == EEXIST || refused == EINVAL) {
        asked = true;
        if (!may_replace(to, name, into, answer)) {
            ::close(to);
            return answer;
        }
        if (::renameat(AT_FDCWD, path.c_str(), to, name.c_str()) == 0) {
            ::close(to);
            answer.done = true;
            return answer;
        }
        refused = errno;
    }
    if (refused != EXDEV) {
        ::close(to);
        answer.why = "could not move " + path + " into " + into + ": " + system_reason(refused);
        return answer;
    }
    // ACROSS DRIVES: copied as paste copies, then the first one goes.
    if (!S_ISREG(about.st_mode)) {
        ::close(to);
        answer.why = path + " is not a file, and only a file can be moved to another drive";
        return answer;
    }
    if (!asked && !may_replace(to, name, into, answer)) {
        ::close(to);
        return answer;
    }
    const int from = open_for_its_bytes(path, true, about, answer.why);
    if (from < 0) {
        ::close(to);
        return answer;
    }
    const int failed = place_a_copy(from, to, name, about.st_mode);
    ::close(from);
    ::close(to);
    if (failed != 0) {
        answer.why = why_copy_stopped(failed, "the move of " + path + " to " + into);
        return answer;
    }
    if (::unlink(path.c_str()) != 0) {
        answer.why = path + " was copied into " + into + ", but could not be removed (" + system_reason(errno) +
                     "), so it is now in both places";
        return answer;
    }
    answer.done = true;
    return answer;
}

} // namespace satellite004
