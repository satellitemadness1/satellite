// satellite/satellite_variable_file/file_moves_cases.cpp -- satellite.file.delete and what it asks with, with no
// console and no other account around them (FILE_AND_DRIVE_PLAN.md FD-2): a file deleted, a write-protected one
// too, a link and never what it points to, a folder and a missing file refused with why; one the person may not
// delete, with no console to ask on, left where it is; su and sudo run with a password through stand-ins of the
// cases' own -- the right one, a wrong one, an account that may not use sudo, and words passed whole -- and the
// hidden question and the yes-or-no asked on a pty, with nothing typed drawn and the terminal put back.
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

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
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
    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    std::printf("%d ok, %d failed: %s\n", passed, failed, failed == 0 ? "every file-moves case passed" : "a file-moves case FAILED");
    return failed == 0 ? 0 : 1;
}
