// satellite/satellite_variable_file/file_moves.cpp -- see file_moves.hpp.

#include "file_moves.hpp"

#include "../machine/as_another_account.hpp"
#include "../machine/ask_at_the_console.hpp"

#include <cerrno>
#include <climits>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <linux/fs.h>
#include <pwd.h>
#include <string>
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

} // namespace satellite004
