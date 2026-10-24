// satellite/satellite_variable_file/file_moves_cases.cpp -- satellite.file.delete and what it asks with, with no
// console and no other account around them (FILE_AND_DRIVE_PLAN.md FD-2): a file deleted, a write-protected one
// too, a link and never what it points to, a folder and a missing file refused with why; one the person may not
// delete, with no console to ask on, left where it is; su and sudo run with a password through stand-ins of the
// cases' own -- the right one, a wrong one, an account that may not use sudo, and words passed whole -- and the
// hidden question and the yes-or-no asked on a pty, with nothing typed drawn and the terminal put back. AND FD-3's
// copy, paste and move: the clipboard in $TMPDIR/.satl/tmp, with TMPDIR pointed into the room, refused when its
// .satl is a link or others may write it; a name already there kept with no console, and the replace question
// answered n and y on a pty; a link moved itself; across drives to /dev/shm when it is another one.
//
//     make build/file_moves_cases && build/file_moves_cases "${TMPDIR:-/tmp}"    (check.sh runs it)
//
// IT WRITES ONLY INSIDE ONE FRESH FOLDER, made in the folder it is given and removed at the end. NO REAL su OR
// sudo IS EVER RUN: the stand-ins are scripts it writes in that folder, and it hands their paths over. The case
// that needs a folder it cannot write is skipped as root.
//
// `ok <name>` for each case that passes, `FAIL <name>: <what happened>` for each that does not; the exit status is
// 0 only when none failed.

#include "file_moves.hpp"

#include "../machine/as_another_account.hpp"
#include "../machine/ask_at_the_console.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <pwd.h>
#include <pty.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace satellite004;

namespace {

int failed = 0;
int passed = 0;
std::string room;

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

std::string at(const std::string &name) { return room + "/" + name; }

bool there(const std::string &path)
{
    struct stat about;
    return ::lstat(path.c_str(), &about) == 0;
}

void write_file(const std::string &path, const std::string &text, mode_t mode = 0644)
{
    std::FILE *file = std::fopen(path.c_str(), "w");
    if (file != nullptr) {
        std::fputs(text.c_str(), file);
        std::fclose(file);
    }
    ::chmod(path.c_str(), mode);
}

std::string read_file(const std::string &path)
{
    std::string text;
    std::FILE *file = std::fopen(path.c_str(), "r");
    if (file == nullptr)
        return "(none)";
    int c;
    while ((c = std::fgetc(file)) != EOF)
        text += static_cast<char>(c);
    std::fclose(file);
    return text;
}

bool holds(const std::string &text, const std::string &part) { return text.find(part) != std::string::npos; }

int remove_one(const char *path, const struct stat *, int, struct FTW *)
{
    ::chmod(path, 0700);
    return ::remove(path);
}

// THE STAND-INS: su asks on its terminal with ECHO off, as PAM does, then runs the command through sh as su runs
// it through the account's shell; sudo reads the password from its input, as -S makes it.
const char kSu[] =
    "#!/bin/sh\n"
    "[ \"$1\" = -c ] || exit 2\n"
    "cmd=$2; shift 2\n"
    "[ \"$1\" = -- ] && shift\n"
    "account=$1; shift\n"
    "printf 'Password: '\n"
    "stty -echo\n"
    "IFS= read -r pw\n"
    "stty echo\n"
    "echo\n"
    "[ \"$pw\" = right ] || { echo 'su: Authentication failure'; exit 1; }\n"
    "exec sh -c \"$cmd\" \"$@\"\n";
const char kSudo[] =
    "#!/bin/sh\n"
    "while [ \"$1\" != -- ]; do shift; done; shift\n"
    "IFS= read -r pw\n"
    "case \"$pw\" in\n"
    "right) exec \"$@\" ;;\n"
    "outsider) echo 'madness is not in the sudoers file.'; exit 1 ;;\n"
    "*) echo 'Sorry, try again.'; echo 'sudo: no password was provided'; echo 'sudo: 1 incorrect password attempt'; exit 1 ;;\n"
    "esac\n";

// su ASKING ONE MORE THING after the password, as an expired password does, and waiting for ever on it; and a
// sudo that, being root's stand-in, makes the folder writable first so its unlink can do what root's would.
const char kSuAsksMore[] =
    "#!/bin/sh\n"
    "printf 'Password: '\n"
    "stty -echo\n"
    "IFS= read -r pw\n"
    "stty echo\n"
    "echo\n"
    "printf 'Current password: '\n"
    "stty -echo\n"
    "IFS= read -r again\n"
    "exit 0\n";
const char kSudoAsRoot[] =
    "#!/bin/sh\n"
    "while [ \"$1\" != -- ]; do shift; done; shift\n"
    "IFS= read -r pw\n"
    "[ \"$pw\" = right ] || { echo 'Sorry, try again.'; echo 'sudo: no password was provided'; exit 1; }\n"
    "for last; do :; done\n"
    "chmod u+w \"$(dirname \"$last\")\"\n"
    "exec \"$@\"\n";

void deleting()
{
    write_file(at("plain"), "x\n");
    DiskAnswer answer = delete_a_file(at("plain"));
    check(answer.done && answer.why.empty() && !there(at("plain")), "delete: a file of the person's own is gone",
          answer.why);

    write_file(at("protected"), "x\n", 0444);
    answer = delete_a_file(at("protected"));
    check(answer.done && !there(at("protected")), "delete: a write-protected file goes too, with no question",
          answer.why);

    write_file(at("target"), "kept\n");
    ::symlink("target", at("link").c_str());
    answer = delete_a_file(at("link"));
    check(answer.done && !there(at("link")) && read_file(at("target")) == "kept\n",
          "delete: a link is deleted itself, and what it points to is kept", answer.why);

    ::mkdir(at("folder").c_str(), 0755);
    answer = delete_a_file(at("folder"));
    check(!answer.done && there(at("folder")) && answer.why == at("folder") + " is a folder, and satellite.file.delete deletes files",
          "delete: a folder is refused, by name", answer.why);

    answer = delete_a_file(at("nowhere"));
    check(!answer.done && answer.why == "there is no file " + at("nowhere"), "delete: nothing there says so",
          answer.why);

    write_file(at("keep"), "x\n");
    answer = delete_a_file(at("keep") + std::string(1, '\0') + ".bak");
    check(!answer.done && there(at("keep")) && answer.why == "the path holds a NUL, which no file's name can: nothing was deleted",
          "delete: a path holding a NUL deletes nothing, not the file named by the part before it", answer.why);

    if (::geteuid() == 0) {
        skipped("delete: a file in a folder the person may not write, with no console, stays", "root may write any folder");
        return;
    }
    ::mkdir(at("locked").c_str(), 0755);
    write_file(at("locked/kept"), "x\n");
    ::chmod(at("locked").c_str(), 0555);
    const int input = ::dup(STDIN_FILENO);
    const int nothing = ::open("/dev/null", O_RDONLY);
    ::dup2(nothing, STDIN_FILENO);
    answer = delete_a_file(at("locked/kept"));
    ::dup2(input, STDIN_FILENO);
    ::close(input);
    ::close(nothing);
    ::chmod(at("locked").c_str(), 0755);
    check(!answer.done && there(at("locked/kept")) && holds(answer.why, "there is no console to ask") &&
              holds(answer.why, "so it was not deleted"),
          "delete: a file in a folder the person may not write, with no console, stays and says why", answer.why);
}

void accounts()
{
    write_file(at("su"), kSu, 0755);
    write_file(at("sudo"), kSudo, 0755);
    const std::string said = "a; b $(touch " + at("never") + ") 'c'";
    const std::vector<std::string> words{"/bin/sh", "-c", "printf '%s' \"$1\" > \"$2\"", "sh", said, at("su_wrote")};

    AccountRun run = as_owner("someone", "right", words, at("su"));
    check(run.done && !run.wrong_password && read_file(at("su_wrote")) == said && !there(at("never")),
          "su: the right password runs the command, its words whole", run.why + " / " + read_file(at("su_wrote")));

    ::unlink(at("su_wrote").c_str());
    run = as_owner("someone", "wrong", words, at("su"));
    check(!run.done && run.wrong_password && run.why == "that password was not someone's" && !there(at("su_wrote")),
          "su: a wrong password runs nothing, and says whose it was not", run.why);

    const std::vector<std::string> sudo_words{"/bin/sh", "-c", "printf '%s' \"$1\" > \"$2\"", "sh", said,
                                              at("sudo_wrote")};
    run = with_sudo("right", sudo_words, at("sudo"));
    check(run.done && read_file(at("sudo_wrote")) == said && !there(at("never")),
          "sudo: the right password runs the command, its words whole", run.why);

    ::unlink(at("sudo_wrote").c_str());
    run = with_sudo("wrong", sudo_words, at("sudo"));
    check(!run.done && run.wrong_password && !run.not_allowed && run.why == "that password was not right" &&
              !there(at("sudo_wrote")),
          "sudo: a wrong password runs nothing, and says so", run.why);

    run = with_sudo("outsider", sudo_words, at("sudo"));
    check(!run.done && run.not_allowed && !run.wrong_password &&
              run.why == "this account may not use sudo: madness is not in the sudoers file.",
          "sudo: an account that may not use sudo is told so", run.why);

    run = with_sudo("right", {"/bin/sh", "-c", "echo it could not; exit 3"}, at("sudo"));
    check(!run.done && !run.wrong_password && run.why == "it could not", "sudo: a command that fails says its own last line",
          run.why);

    write_file(at("su_more"), kSuAsksMore, 0755);
    run = as_owner("someone", "right", words, at("su_more"));
    check(!run.done && !run.wrong_password && holds(run.why, "su asked for more than someone's password") &&
              holds(run.why, "Current password:"),
          "su: a second question after the password ends su, rather than waiting for ever", run.why);

    run = with_sudo("right", {"/bin/sh", "-c", "echo \"unlink: cannot unlink 'Sorry, try again': Permission denied\"; exit 1"},
                    at("sudo"));
    check(!run.done && !run.wrong_password && !run.not_allowed && holds(run.why, "cannot unlink"),
          "sudo: a command's own line quoting \"Sorry, try again\" is not a wrong password", run.why);

    run = as_owner("someone", "right", words, at("no_such_su"));
    check(!run.done && !run.wrong_password && holds(run.why, "could not be started"),
          "su: a su that is not there says it could not be started", run.why);
}

// WHO IS ASKED, from two stats alone: the folder's owner, or in a sticky folder the file's -- through su when that
// account can run a command and is not the person's own, through sudo otherwise.
void who_is_asked()
{
    const uid_t me = ::getuid();
    struct stat file{}, folder{};
    file.st_uid = me;
    folder.st_uid = me;
    folder.st_mode = S_IFDIR | 0555;
    DeleteAsker asker = who_may_delete(file, folder, me);
    check(asker.owner == me && !asker.through_su, "who: a read-only folder of the person's own asks their sudo password",
          asker.account);

    folder.st_uid = 0;
    folder.st_mode = S_IFDIR | 0755;
    const passwd *root = ::getpwuid(0);
    const bool root_logs_in = root != nullptr && root->pw_shell != nullptr &&
                              std::string(root->pw_shell).find("nologin") == std::string::npos &&
                              std::string(root->pw_shell).find("false") == std::string::npos;
    asker = who_may_delete(file, folder, me);
    check(asker.owner == 0 && asker.account == "root" && asker.through_su == (root_logs_in && me != 0),
          "who: a folder of root's asks root's password, through su", asker.account);

    folder.st_mode = S_IFDIR | S_ISVTX | 0777;
    file.st_uid = 0;
    asker = who_may_delete(file, folder, me);
    check(asker.owner == 0, "who: in a sticky folder, a file of the folder's owner's is the owner's", asker.account);

    const passwd *nobody = ::getpwnam("nobody");
    if (nobody == nullptr || nobody->pw_shell == nullptr || std::string(nobody->pw_shell).find("nologin") == std::string::npos) {
        skipped("who: in a sticky folder, the file's owner -- and one who cannot log in is asked through sudo",
                "no nobody account with nologin here");
    } else {
        file.st_uid = nobody->pw_uid;
        asker = who_may_delete(file, folder, me);
        check(asker.owner == nobody->pw_uid && asker.account == "nobody" && !asker.through_su,
              "who: in a sticky folder, the file's owner -- and one who cannot log in is asked through sudo",
              asker.account);
    }

    folder.st_mode = S_IFDIR | 0755;
    folder.st_uid = 4000123;
    asker = who_may_delete(file, folder, me);
    check(asker.owner == 4000123 && asker.account == "4000123" && !asker.through_su,
          "who: an owner with no account is named by number and asked through sudo", asker.account);
}

// DELETE ON A PTY, as at the prompt: a file in a read-only folder of the person's own asks for their sudo password
// and goes through the stand-in with the right one; a wrong one leaves it, and says so.
std::string delete_on_a_pty(const std::string &path, const char *password, std::string &drawn)
{
    int master = -1, terminal = -1, answers[2];
    if (::openpty(&master, &terminal, nullptr, nullptr, nullptr) != 0 || ::pipe(answers) != 0)
        return "(no pty)";
    const pid_t child = ::fork();
    if (child == 0) {
        ::close(master);
        ::close(answers[0]);
        ::setsid();
        ::ioctl(terminal, TIOCSCTTY, 0);
        ::dup2(terminal, STDIN_FILENO);
        ::dup2(terminal, STDOUT_FILENO);
        const DiskAnswer answer = delete_a_file(path, at("su"), at("sudo_as_root"));
        const std::string report = std::string(answer.done ? "done" : "kept") + "|" + answer.why;
        (void)!::write(answers[1], report.data(), report.size());
        ::_exit(0);
    }
    ::close(terminal);
    ::close(answers[1]);
    drawn.clear();
    for (int tries = 0; tries < 100 && !holds(drawn, "[sudo] password for "); ++tries) {
        pollfd waiting{master, POLLIN, 0};
        if (::poll(&waiting, 1, 50) > 0) {
            char buffer[512];
            const ssize_t got = ::read(master, buffer, sizeof buffer);
            if (got > 0)
                drawn.append(buffer, static_cast<std::size_t>(got));
        }
    }
    for (int tries = 0; tries < 100; ++tries) {
        termios now{};
        if (::tcgetattr(master, &now) == 0 && (now.c_lflag & ECHO) == 0)
            break;
        ::usleep(10000);
    }
    const std::string line = std::string(password) + "\n";
    (void)!::write(master, line.data(), line.size());
    std::string report;
    char buffer[512];
    ssize_t got;
    while ((got = ::read(answers[0], buffer, sizeof buffer)) > 0)
        report.append(buffer, static_cast<std::size_t>(got));
    int status = 0;
    ::waitpid(child, &status, 0);
    ::close(master);
    ::close(answers[0]);
    return report;
}

void deleting_with_a_password()
{
    if (::geteuid() == 0) {
        skipped("delete on a pty: a read-only folder of the person's own asks their sudo password", "root may write any folder");
        return;
    }
    write_file(at("sudo_as_root"), kSudoAsRoot, 0755);
    ::mkdir(at("shut").c_str(), 0755);
    write_file(at("shut/kept"), "x\n");
    ::chmod(at("shut").c_str(), 0555);
    std::string drawn;
    std::string report = delete_on_a_pty(at("shut/kept"), "wrong", drawn);
    check(holds(report, "kept|") && holds(report, "was not deleted: that password was not right") && there(at("shut/kept")) &&
              !holds(drawn, "wrong"),
          "delete on a pty: a wrong sudo password leaves the file, and says so", report + " / " + drawn);
    ::chmod(at("shut").c_str(), 0555);
    report = delete_on_a_pty(at("shut/kept"), "right", drawn);
    check(report == "done|" && !there(at("shut/kept")) && !holds(drawn, "right") &&
              holds(drawn, "is in a folder of yours that will not let it go -- [sudo] password for "),
          "delete on a pty: a read-only folder of the person's own asks their sudo password, and the right one deletes",
          report + " / " + drawn);
    ::chmod(at("shut").c_str(), 0755);
}

// THE QUESTIONS ON A PTY, asked in a child whose terminal it is: what was drawn, what was answered, and whether
// ECHO came back.
void asking()
{
    int master = -1, terminal = -1;
    if (::openpty(&master, &terminal, nullptr, nullptr, nullptr) != 0) {
        check(false, "asking: a pty to ask on", std::strerror(errno));
        return;
    }
    int answers[2];
    if (::pipe(answers) != 0) {
        check(false, "asking: a pipe for the answers", std::strerror(errno));
        return;
    }
    const pid_t child = ::fork();
    if (child == 0) {
        ::close(master);
        ::close(answers[0]);
        ::setsid();
        ::ioctl(terminal, TIOCSCTTY, 0);
        ::dup2(terminal, STDIN_FILENO);
        ::dup2(terminal, STDOUT_FILENO);
        std::string answer;
        const bool typed = hidden_line("secret word: ", answer);
        termios after{};
        ::tcgetattr(STDIN_FILENO, &after);
        const bool yes = yes_or_no("go on?");
        const bool no = yes_or_no("again?");
        const std::string report = std::string(typed ? "typed" : "not") + "|" + answer + "|" +
                                   ((after.c_lflag & ECHO) != 0 ? "echo" : "silent") + "|" + (yes ? "yes" : "no") +
                                   "|" + (no ? "yes" : "no");
        (void)!::write(answers[1], report.data(), report.size());
        ::_exit(0);
    }
    ::close(terminal);
    ::close(answers[1]);
    std::string drawn;
    const auto read_until = [&](const char *words) {
        for (int tries = 0; tries < 100 && !holds(drawn, words); ++tries) {
            pollfd waiting{master, POLLIN, 0};
            if (::poll(&waiting, 1, 50) > 0) {
                char buffer[512];
                const ssize_t got = ::read(master, buffer, sizeof buffer);
                if (got > 0)
                    drawn.append(buffer, static_cast<std::size_t>(got));
            }
        }
    };
    read_until("secret word: ");
    // TYPED ONLY ONCE ECHO IS OFF, as a person's keys would come long after the question.
    for (int tries = 0; tries < 100; ++tries) {
        termios now{};
        if (::tcgetattr(master, &now) == 0 && (now.c_lflag & ECHO) == 0)
            break;
        ::usleep(10000);
    }
    (void)!::write(master, "s3cret\n", 7);
    read_until("go on? [y/N] ");
    (void)!::write(master, "Y\n", 2);
    read_until("again? [y/N] ");
    (void)!::write(master, "\n", 1);
    std::string report;
    char buffer[512];
    ssize_t got;
    while ((got = ::read(answers[0], buffer, sizeof buffer)) > 0)
        report.append(buffer, static_cast<std::size_t>(got));
    int status = 0;
    ::waitpid(child, &status, 0);
    read_until("\n");
    ::close(master);
    ::close(answers[0]);
    check(report == "typed|s3cret|echo|yes|no", "asking: the hidden answer is read, ECHO comes back, Y is yes and Enter no",
          report);
    check(holds(drawn, "secret word: ") && !holds(drawn, "s3cret"), "asking: the hidden answer is never drawn", drawn);
    check(holds(drawn, "Y"), "asking: a yes-or-no answer is drawn as it is typed", drawn);

    std::string answer = "left over";
    const int input = ::dup(STDIN_FILENO);
    const int nothing = ::open("/dev/null", O_RDONLY);
    ::dup2(nothing, STDIN_FILENO);
    const bool console = a_console_to_ask_at();
    const bool asked = hidden_line("never seen: ", answer) || yes_or_no("never seen");
    ::dup2(input, STDIN_FILENO);
    ::close(input);
    ::close(nothing);
    check(!asked && answer.empty() && !console, "asking: with no terminal nothing is asked, and the answer is no",
          answer);
}

// THE CLIPBOARD AND ITS WORDS (FD-3): TMPDIR is pointed into the room, so the person's /tmp/.satl is never touched.
std::string clip_at(const std::string &name) { return room + "/clip/.satl/tmp/" + name; }

std::string mode_of(const std::string &path)
{
    struct stat about{};
    if (::lstat(path.c_str(), &about) != 0)
        return "(none)";
    char text[16];
    std::snprintf(text, sizeof text, "%o", static_cast<unsigned>(about.st_mode & 07777));
    return text;
}

// A MODE AS A COPY MAKES IT, as cp does: the nine permission bits through this process's umask.
std::string through_umask(mode_t mode)
{
    const mode_t mask = ::umask(0);
    ::umask(mask);
    char text[16];
    std::snprintf(text, sizeof text, "%o", static_cast<unsigned>(mode & 0777 & ~mask));
    return text;
}

// A FOLDER'S NAMES, sorted, one space between: a copy left half way would show as a name beside the file.
std::string listing_of(const std::string &path)
{
    std::vector<std::string> names;
    if (DIR *listing = ::opendir(path.c_str())) {
        while (const dirent *entry = ::readdir(listing))
            if (std::strcmp(entry->d_name, ".") != 0 && std::strcmp(entry->d_name, "..") != 0)
                names.emplace_back(entry->d_name);
        ::closedir(listing);
    }
    std::sort(names.begin(), names.end());
    std::string text;
    for (const std::string &name : names)
        text += (text.empty() ? "" : " ") + name;
    return text;
}

// A WORD RUN WITH NOTHING TO ASK ON: stdin from /dev/null, as delete's no-console case.
template <typename Word>
DiskAnswer with_no_console(Word word)
{
    const int input = ::dup(STDIN_FILENO);
    const int nothing = ::open("/dev/null", O_RDONLY);
    ::dup2(nothing, STDIN_FILENO);
    const DiskAnswer answer = word();
    ::dup2(input, STDIN_FILENO);
    ::close(input);
    ::close(nothing);
    return answer;
}

const std::string kNoConsole = " is already there, and there is no console to ask whether to replace it, so it was kept";

void copying()
{
    ::mkdir(at("clip").c_str(), 0755);
    ::setenv("TMPDIR", at("clip").c_str(), 1);
    write_file(at("a.txt"), "one\n", 0640);
    write_file(at("b.txt"), "two\n");
    DiskAnswer answer = copy_a_file(at("a.txt"));
    check(answer.done && read_file(clip_at("file-current")) == "one\n" && mode_of(clip_at("file-current")) == through_umask(0640) &&
              mode_of(at("clip/.satl")) == "700" && mode_of(at("clip/.satl/tmp")) == "700" &&
              read_file(clip_at("names")) == "file-current\ta.txt\n",
          "copy: the bytes and the mode go to file-current, in folders that are 0700, and its first name to names",
          answer.why + " / " + read_file(clip_at("names")));
    answer = copy_a_file(at("b.txt"));
    check(answer.done && read_file(clip_at("file-current")) == "two\n" &&
              read_file(clip_at("file-previous-001")) == "one\n" &&
              read_file(clip_at("names")) == "file-previous-001\ta.txt\nfile-current\tb.txt\n" &&
              listing_of(at("clip/.satl/tmp")) == "file-current file-previous-001 names",
          "copy: a second copy moves the first to file-previous-001 with its name, and leaves nothing else",
          answer.why + " / " + listing_of(at("clip/.satl/tmp")));
    ::rename(clip_at("file-previous-001").c_str(), clip_at("file-previous-007").c_str());
    answer = copy_a_file(at("a.txt"));
    check(answer.done && read_file(clip_at("file-previous-008")) == "two\n" && read_file(clip_at("file-current")) == "one\n",
          "copy: the next previous number is one past the highest there, so a gap the cleaning left is never refilled",
          answer.why + " / " + listing_of(at("clip/.satl/tmp")));
    ::mkdir(at("a_folder").c_str(), 0755);
    answer = copy_a_file(at("a_folder"));
    check(!answer.done && answer.why == at("a_folder") + " is a folder, and satellite.file.copy copies files" &&
              read_file(clip_at("file-current")) == "one\n",
          "copy: a folder is refused, and the clipboard is as it was", answer.why);
    answer = copy_a_file(at("nowhere"));
    check(!answer.done && answer.why == "there is no file " + at("nowhere"), "copy: nothing there says so", answer.why);
    answer = copy_a_file(at("a.txt") + std::string(1, '\0') + "x");
    check(!answer.done && answer.why == "the path holds a NUL, which no file's name can: nothing was copied" &&
              read_file(clip_at("file-current")) == "one\n",
          "copy: a path holding a NUL copies nothing, not the file named by the part before it", answer.why);
    ::mkfifo(at("a_fifo").c_str(), 0600);
    answer = copy_a_file(at("a_fifo"));   // a FIFO's open would wait for a writer: it must answer, not hang
    check(!answer.done && answer.why == at("a_fifo") + " is not a file, and satellite.file.copy copies files",
          "copy: a FIFO is refused at once, never opened", answer.why);
    write_file(at("setuid"), "#!/bin/sh\n", 0755);
    ::chmod(at("setuid").c_str(), 06755);
    answer = copy_a_file(at("setuid"));
    check(answer.done && mode_of(clip_at("file-current")) == through_umask(0755),
          "copy: a setuid and setgid file's copy has neither bit -- only the nine, through the umask, as cp gives them",
          answer.why + " / " + mode_of(clip_at("file-current")));
    copy_a_file(at("a.txt"));   // file-current is a.txt again for the pastes below
    ::chmod(at("clip/.satl").c_str(), 0755);
    ::chmod(at("clip/.satl/tmp").c_str(), 0755);
    answer = copy_a_file(at("a.txt"));
    check(answer.done && mode_of(at("clip/.satl")) == "700" && mode_of(at("clip/.satl/tmp")) == "700",
          "copy: a clipboard folder of the person's own that others may read is made 0700 again", answer.why);

    ::mkdir(at("clip_link").c_str(), 0755);
    ::mkdir(at("elsewhere").c_str(), 0700);
    ::symlink(at("elsewhere").c_str(), at("clip_link/.satl").c_str());
    ::setenv("TMPDIR", at("clip_link").c_str(), 1);
    answer = copy_a_file(at("a.txt"));
    check(!answer.done && answer.why == at("clip_link") + "/.satl is not a folder, so nothing can be kept on the clipboard" &&
              listing_of(at("elsewhere")).empty(),
          "copy: a .satl that is a link is refused, and nothing is written where it points", answer.why);
    ::mkdir(at("clip_open").c_str(), 0755);
    ::mkdir(at("clip_open/.satl").c_str(), 0700);
    ::chmod(at("clip_open/.satl").c_str(), 0777);
    ::setenv("TMPDIR", at("clip_open").c_str(), 1);
    answer = copy_a_file(at("a.txt"));
    check(!answer.done &&
              answer.why == at("clip_open") + "/.satl is not a folder of yours alone, so nothing can be kept on the clipboard" &&
              listing_of(at("clip_open/.satl")).empty(),
          "copy: a .satl others may write is refused, by the same test as another account's", answer.why);
    ::mkdir(at("clip_empty").c_str(), 0755);
    ::setenv("TMPDIR", at("clip_empty").c_str(), 1);
    answer = paste_a_file(at("a_folder"));
    check(!answer.done && answer.why == "nothing has been copied to paste" && listing_of(at("a_folder")).empty(),
          "paste: with nothing copied, says so", answer.why);
    ::setenv("TMPDIR", at("clip").c_str(), 1);
}

void pasting()
{
    ::mkdir(at("there").c_str(), 0755);
    DiskAnswer answer = paste_a_file(at("there"));
    check(answer.done && read_file(at("there/a.txt")) == "one\n" && mode_of(at("there/a.txt")) == through_umask(0640) &&
              listing_of(at("there")) == "a.txt" && read_file(clip_at("file-current")) == "one\n",
          "paste: file-current goes into the folder under its first name and its mode, and stays on the clipboard",
          answer.why + " / " + listing_of(at("there")));
    write_file(at("there/a.txt"), "mine\n");
    answer = with_no_console([] { return paste_a_file(at("there")); });
    check(!answer.done && read_file(at("there/a.txt")) == "mine\n" && answer.why == at("there") + "/a.txt" + kNoConsole,
          "paste: onto a name already there, with no console, keeps the file and says so", answer.why);
    ::mkdir(at("there2").c_str(), 0755);
    ::mkdir(at("there2/a.txt").c_str(), 0755);
    answer = paste_a_file(at("there2"));
    check(!answer.done && answer.why == at("there2") + "/a.txt is a folder, so no file can take its place",
          "paste: a folder of that name is never replaced", answer.why);
    answer = paste_a_file(at("nowhere"));
    check(!answer.done && answer.why == "there is no folder " + at("nowhere"), "paste: into no folder says so",
          answer.why);
    answer = paste_a_file(at("b.txt"));
    check(!answer.done && answer.why == at("b.txt") + " is a file, not a folder", "paste: into a file says so",
          answer.why);
    ::mkdir(at("working").c_str(), 0755);
    const int back = ::open(".", O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    answer = ::chdir(at("working").c_str()) == 0 ? paste_a_file(".") : DiskAnswer{};
    if (back >= 0) {
        (void)!::fchdir(back);
        ::close(back);
    }
    check(answer.done && read_file(at("working/a.txt")) == "one\n", "paste(): into the working directory", answer.why);
    answer = paste_a_file(std::string());
    check(!answer.done && answer.why == "no folder was named to paste into; satellite.file.paste() pastes into the folder you are in",
          "paste(\"\"): an empty name is no folder, and is not taken for paste()", answer.why);
    answer = paste_a_file(at("there") + std::string(1, '\0') + "x");
    check(!answer.done && answer.why == "the folder holds a NUL, which no folder's name can: nothing was pasted",
          "paste: a folder holding a NUL pastes nothing", answer.why);
}

void moving()
{
    ::mkdir(at("from").c_str(), 0755);
    ::mkdir(at("into").c_str(), 0755);
    write_file(at("from/m.txt"), "move me\n");
    DiskAnswer answer = move_a_file(at("from/m.txt"), at("into"));
    check(answer.done && !there(at("from/m.txt")) && read_file(at("into/m.txt")) == "move me\n",
          "move: the file goes into the folder, keeping its name", answer.why);
    write_file(at("from/m.txt"), "second\n");
    answer = with_no_console([] { return move_a_file(at("from/m.txt"), at("into")); });
    check(!answer.done && read_file(at("from/m.txt")) == "second\n" && read_file(at("into/m.txt")) == "move me\n" &&
              answer.why == at("into") + "/m.txt" + kNoConsole,
          "move: never over a file already there unasked", answer.why);
    answer = move_a_file(at("into/m.txt"), at("into"));
    check(answer.done && read_file(at("into/m.txt")) == "move me\n",
          "move: into the folder it is in already is done, and nothing changes", answer.why);
    write_file(at("pointed"), "pointed\n");
    ::symlink(at("pointed").c_str(), at("from/ln").c_str());
    answer = move_a_file(at("from/ln"), at("into"));
    struct stat moved{};
    check(answer.done && ::lstat(at("into/ln").c_str(), &moved) == 0 && S_ISLNK(moved.st_mode) && !there(at("from/ln")) &&
              read_file(at("pointed")) == "pointed\n",
          "move: a link moves itself, never what it points to", answer.why);
    answer = move_a_file(at("a_folder"), at("into"));
    check(!answer.done && there(at("a_folder")) && answer.why == at("a_folder") + " is a folder, and satellite.file.move moves files",
          "move: a folder is refused", answer.why);
    answer = move_a_file(at("from/none.txt"), at("into"));
    check(!answer.done && answer.why == "there is no file " + at("from/none.txt"), "move: nothing there says so",
          answer.why);
    answer = move_a_file(at("from/m.txt"), at("nowhere"));
    check(!answer.done && there(at("from/m.txt")) && answer.why == "there is no folder " + at("nowhere"),
          "move: into no folder says so, and the file stays", answer.why);
    answer = move_a_file(at("from/m.txt"), std::string());
    check(!answer.done && there(at("from/m.txt")) && answer.why == "no folder was named to move " + at("from/m.txt") + " into",
          "move: an empty folder name is refused, not taken for the working directory", answer.why);
    answer = move_a_file(at("from/m.txt") + std::string(1, '\0') + "x", at("into"));
    check(!answer.done && there(at("from/m.txt")) &&
              answer.why == "the path holds a NUL, which no file's name can: nothing was moved",
          "move: a path holding a NUL moves nothing, not the file named by the part before it", answer.why);
    ::mkdir(at("linked").c_str(), 0755);
    write_file(at("from/h.txt"), "hard\n");
    ::link(at("from/h.txt").c_str(), at("linked/h.txt").c_str());
    answer = move_a_file(at("from/h.txt"), at("linked"));
    check(!answer.done && there(at("from/h.txt")) && there(at("linked/h.txt")) &&
              answer.why == at("from/h.txt") + " and " + at("linked") + "/h.txt are the same file, so nothing was moved",
          "move: onto a hard link of the same file is refused, as mv refuses it -- never done with both left", answer.why);

    // ACROSS DRIVES, when /dev/shm is another one.
    struct stat here{}, shm{};
    if (::stat(room.c_str(), &here) != 0 || ::stat("/dev/shm", &shm) != 0 || here.st_dev == shm.st_dev) {
        skipped("move: across drives the file is copied with its mode and the first one removed", "no other drive at /dev/shm");
        return;
    }
    char made[] = "/dev/shm/file_moves_cases.XXXXXX";
    if (::mkdtemp(made) == nullptr) {
        skipped("move: across drives the file is copied with its mode and the first one removed", "no folder in /dev/shm");
        return;
    }
    const std::string far = made;
    write_file(at("from/far.txt"), "far\n", 0604);
    answer = move_a_file(at("from/far.txt"), far);
    check(answer.done && !there(at("from/far.txt")) && read_file(far + "/far.txt") == "far\n" &&
              mode_of(far + "/far.txt") == through_umask(0604) && listing_of(far) == "far.txt",
          "move: across drives the file is copied with its mode and the first one removed", answer.why);
    write_file(at("from/far.txt"), "again\n");
    answer = with_no_console([&far] { return move_a_file(at("from/far.txt"), far); });
    check(!answer.done && read_file(at("from/far.txt")) == "again\n" && read_file(far + "/far.txt") == "far\n" &&
              answer.why == far + "/far.txt" + kNoConsole,
          "move: across drives, never over a file already there unasked", answer.why);
    ::nftw(far.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
}

// TWO COPYING AT ONCE (the review, 2026-10-23): two processes each copy their own file 25 times into one
// clipboard. The lock makes them take turns, so all 50 copies are there -- file-current and 49 previous -- each
// number once, and `names` holds one line for each.
void copying_at_once()
{
    ::mkdir(at("clip_two").c_str(), 0755);
    ::setenv("TMPDIR", at("clip_two").c_str(), 1);
    write_file(at("first.txt"), "first\n");
    write_file(at("second.txt"), "second\n");
    pid_t children[2];
    for (int i = 0; i < 2; ++i) {
        children[i] = ::fork();
        if (children[i] == 0) {
            bool every = true;
            for (int n = 0; n < 25; ++n)
                every = copy_a_file(at(i == 0 ? "first.txt" : "second.txt")).done && every;
            ::_exit(every ? 0 : 1);
        }
    }
    bool both = true;
    for (const pid_t child : children) {
        int status = 1;
        ::waitpid(child, &status, 0);
        both = both && WIFEXITED(status) && WEXITSTATUS(status) == 0;
    }
    const std::string clip = room + "/clip_two/.satl/tmp/";
    int previous = 0, firsts = 0;
    for (int n = 1; n <= 60; ++n) {
        char name[32];
        std::snprintf(name, sizeof name, "file-previous-%03d", n);
        if (there(clip + name)) {
            ++previous;
            firsts += read_file(clip + name) == "first\n";
        }
    }
    const std::string names = read_file(clip + "names");
    const long lines = std::count(names.begin(), names.end(), '\n');
    firsts += read_file(clip + "file-current") == "first\n";
    check(both && previous == 49 && there(clip + "file-current") && firsts == 25 && lines == 50 &&
              listing_of(room + "/clip_two/.satl/tmp").find(".file-incoming") == std::string::npos,
          "copy: two copying at once take turns -- all 50 kept, each number once, a name for each",
          std::to_string(previous) + " previous, " + std::to_string(firsts) + " of the first, " + std::to_string(lines) +
              " names");
    ::setenv("TMPDIR", at("clip").c_str(), 1);
}

// THE REPLACE QUESTION ON A PTY (rule 6), asked in a child whose terminal it is: `typed` is answered at "[y/N] ".
std::string replace_on_a_pty(DiskAnswer (*word)(), const char *typed, std::string &drawn)
{
    int master = -1, terminal = -1, answers[2];
    if (::openpty(&master, &terminal, nullptr, nullptr, nullptr) != 0 || ::pipe(answers) != 0)
        return "(no pty)";
    const pid_t child = ::fork();
    if (child == 0) {
        ::close(master);
        ::close(answers[0]);
        ::setsid();
        ::ioctl(terminal, TIOCSCTTY, 0);
        ::dup2(terminal, STDIN_FILENO);
        ::dup2(terminal, STDOUT_FILENO);
        const DiskAnswer answer = word();
        const std::string report = std::string(answer.done ? "done" : "kept") + "|" + answer.why;
        (void)!::write(answers[1], report.data(), report.size());
        ::_exit(0);
    }
    ::close(terminal);
    ::close(answers[1]);
    drawn.clear();
    for (int tries = 0; tries < 100 && !holds(drawn, "[y/N] "); ++tries) {
        pollfd waiting{master, POLLIN, 0};
        if (::poll(&waiting, 1, 50) > 0) {
            char buffer[512];
            const ssize_t got = ::read(master, buffer, sizeof buffer);
            if (got > 0)
                drawn.append(buffer, static_cast<std::size_t>(got));
        }
    }
    const std::string line = std::string(typed) + "\n";
    (void)!::write(master, line.data(), line.size());
    std::string report;
    char buffer[512];
    ssize_t got;
    while ((got = ::read(answers[0], buffer, sizeof buffer)) > 0)
        report.append(buffer, static_cast<std::size_t>(got));
    int status = 0;
    ::waitpid(child, &status, 0);
    ::close(master);
    ::close(answers[0]);
    return report;
}

void replacing_on_a_pty()
{
    ::mkdir(at("asked").c_str(), 0755);
    write_file(at("asked/a.txt"), "mine\n");
    std::string drawn;
    std::string report = replace_on_a_pty([] { return paste_a_file(at("asked")); }, "n", drawn);
    check(report == "kept|" + at("asked") + "/a.txt is already there, and it was kept" &&
              read_file(at("asked/a.txt")) == "mine\n" && holds(drawn, "\"a.txt\" is already in " + at("asked")) &&
              holds(drawn, "-- replace it? [y/N] "),
          "paste on a pty: the question names the file and the folder, and n keeps the file", report + " / " + drawn);
    report = replace_on_a_pty([] { return paste_a_file(at("asked")); }, "y", drawn);
    check(report == "done|" && read_file(at("asked/a.txt")) == "one\n" && listing_of(at("asked")) == "a.txt",
          "paste on a pty: y replaces it with the copy", report + " / " + drawn);
    write_file(at("asked/m2.txt"), "old\n");
    write_file(at("from/m2.txt"), "new\n");
    report = replace_on_a_pty([] { return move_a_file(at("from/m2.txt"), at("asked")); }, "y", drawn);
    check(report == "done|" && read_file(at("asked/m2.txt")) == "new\n" && !there(at("from/m2.txt")),
          "move on a pty: y puts the file in the place of the one there", report + " / " + drawn);

    struct stat here{}, shm{};
    char made[] = "/dev/shm/file_moves_cases.XXXXXX";
    if (::stat(room.c_str(), &here) != 0 || ::stat("/dev/shm", &shm) != 0 || here.st_dev == shm.st_dev ||
        ::mkdtemp(made) == nullptr) {
        skipped("move on a pty: across drives, y replaces the one there once asked", "no other drive at /dev/shm");
        return;
    }
    static std::string far;
    far = made;
    write_file(far + "/m3.txt", "old\n");
    write_file(at("from/m3.txt"), "new\n");
    report = replace_on_a_pty([] { return move_a_file(at("from/m3.txt"), far); }, "y", drawn);
    std::size_t questions = 0;
    for (std::size_t at_ = drawn.find("[y/N] "); at_ != std::string::npos; at_ = drawn.find("[y/N] ", at_ + 1))
        ++questions;
    check(report == "done|" && read_file(far + "/m3.txt") == "new\n" && !there(at("from/m3.txt")) && questions == 1,
          "move on a pty: across drives, y replaces the one there, asked once", report + " / " + drawn);
    ::nftw(far.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc != 2) {
        std::fprintf(stderr, "usage: file_moves_cases <a folder to work in>\n");
        return 2;
    }
    std::string pattern = std::string(argv[1]) + "/file_moves_cases.XXXXXX";
    std::vector<char> made(pattern.begin(), pattern.end());
    made.push_back('\0');
    if (::mkdtemp(made.data()) == nullptr) {
        std::fprintf(stderr, "file_moves_cases: could not make a folder in %s: %s\n", argv[1], std::strerror(errno));
        return 2;
    }
    room = made.data();
    std::setvbuf(stdout, nullptr, _IOLBF, 0);
    deleting();
    accounts();
    who_is_asked();
    deleting_with_a_password();
    asking();
    copying();
    pasting();
    moving();
    copying_at_once();
    replacing_on_a_pty();
    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    std::printf("%d ok, %d failed: %s\n", passed, failed, failed == 0 ? "every file-moves case passed" : "a file-moves case FAILED");
    return failed == 0 ? 0 : 1;
}
