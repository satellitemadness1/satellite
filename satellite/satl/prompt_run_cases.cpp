// satellite/satl/prompt_run_cases.cpp -- `interpret <file>` and `run <file>` with no screen around them (the
// author, 2026-10-20: "Just create another satellite, for each program"): where the prompt runs a program,
// and how a satellite of its own is started (satl/prompt_run.cpp, machine/satellite_of_its_own.hpp).
//
//     make build/prompt_run_cases && build/prompt_run_cases "${TMPDIR:-/tmp}"    (check.sh runs it)
//
// THE WHOLE OF IT NEEDS A SCREEN, AND THIS HAS NONE: a satellite is a console window. So the two halves are
// proved apart. THE DECISION -- satls_own_console_is_open(), which only a console on a screen answers true,
// and nobody_gave_satl_a_console(), which SATL_NO_WINDOW turns to no -- is answered by each case here, in
// place of window_run.cpp's, and the desk window_run.cpp hands the start to is this harness's own thread
// (start_a_satellite_on_the_desk; check.sh pins the hand-off). THE START is watched from its far end, by
// A STAND-IN FOR satl, which is this same binary: both ways a program is started run /proc/self/exe, and
// started with --run or --console first, this is not the harness but a satl that writes down what it was
// given -- its words, its folder, its session, its three streams, every other file it holds, its Ctrl-C and
// hangup, the signals it has blocked, and its stack -- and then ends with the code it is told, or waits until
// it is let go.
//
// satl'S OWN CONSOLE IS COPIED AS FAR AS IT GOES WITHOUT ONE: while the prompt runs a line, descriptors 0, 1
// and 2 are a pty (console_launch.cpp moves satl's onto its console's), the C stderr is a file of its own,
// "where satl was started" (keep_the_c_streams_off_the_pty), and the stack is widened as main() widens satl's.
//
// IT WRITES ONLY INSIDE ONE FRESH FOLDER, made in the folder it is given and removed at the end. `ok <name>`
// for each case that passes, `FAIL <name>: <what happened>` for each that does not; the exit status is 0
// only when none failed.

#include "prompt_run.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/satellite_of_its_own.hpp"
#include "../machine/stack_share.hpp"

#include <chrono>
#include <cerrno>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <ftw.h>
#include <poll.h>
#include <pthread.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace satellite004;

namespace {

bool the_console_is_open = false;   // what each case says satls_own_console_is_open() answers
bool nobody_gave_one = true;        // ...and nobody_gave_satl_a_console(): false is SATL_NO_WINDOW set
int handed_to_the_desk = 0;         // starts the prompt handed to start_a_satellite_on_the_desk

} // namespace

namespace satellite004 {

// THE DECISION'S QUESTIONS, ANSWERED BY THE CASE: a console of satl's own is a window on a screen, and a
// harness never has one (window_run.cpp asks the desk; here the case says); and in that console, where
// stdout is its pty, nobody_gave_satl_a_console() is no only under SATL_NO_WINDOW (window_run.cpp's rule).
bool satls_own_console_is_open() { return the_console_is_open; }
bool nobody_gave_satl_a_console() { return nobody_gave_one; }

// AND NO DESK: window_run.cpp runs the start on it, at the priority satl was started with; here it runs on
// the harness's own thread, which is all a stand-in can watch -- counted, so a prompt that started one
// itself, at the interpreter's priority, would be seen.
bool start_a_satellite_on_the_desk(const std::vector<std::string> &words, std::string &why)
{
    ++handed_to_the_desk;
    return start_a_satellite_of_its_own(words, why);
}

} // namespace satellite004

namespace {

// ---------------------------------------------------------------------------------------------------------
// THE STAND-IN FOR satl.
// ---------------------------------------------------------------------------------------------------------

// A FILE AS THE KERNEL KNOWS IT: two descriptors on the same file -- or the same device -- answer the same.
std::string identity(const struct stat &about)
{
    return std::to_string(about.st_dev) + ":" + std::to_string(about.st_ino) + ":" + std::to_string(about.st_rdev);
}

std::string identity_of(int fd)
{
    struct stat about {};
    return fstat(fd, &about) == 0 ? identity(about) : std::string("closed");
}

// WHAT IT WAS GIVEN, one "name value" a line, into the file PROMPT_RUN_RECORD names -- written whole under
// another name and renamed, so the harness never reads half of it. Then it waits for PROMPT_RUN_RELEASE to
// be there, when that is set (20 seconds at most), and ends with PROMPT_RUN_CODE.
int be_the_stand_in(int argc, char **argv)
{
    std::string text = "words ";
    for (int i = 1; i < argc; ++i)
        text += std::string(i > 1 ? "|" : "") + argv[i];
    char here[4096];
    text += "\nfolder " + std::string(getcwd(here, sizeof here) != nullptr ? here : "?");
    text += "\npid " + std::to_string(getpid());
    text += "\nparent " + std::to_string(getppid());
    text += "\nsession " + std::to_string(getsid(0));
    const int terminal = open("/dev/tty", O_RDWR | O_NOCTTY | O_CLOEXEC);
    text += std::string("\nterminal ") + (terminal >= 0 ? "yes" : "no");
    if (terminal >= 0)
        close(terminal);
    for (int fd = 0; fd < 3; ++fd)
        text += "\nfd" + std::to_string(fd) + " " + identity_of(fd);
    std::string others;
    if (DIR *open_ones = opendir("/proc/self/fd")) {
        const int own = dirfd(open_ones);
        while (const dirent *entry = readdir(open_ones)) {
            if (entry->d_name[0] == '.')
                continue;
            const int fd = std::atoi(entry->d_name);
            if (fd > 2 && fd != own)
                others += (others.empty() ? "" : ",") + std::to_string(fd);
        }
        closedir(open_ones);
    }
    text += "\nothers " + (others.empty() ? std::string("none") : others);
    struct rlimit stack {};
    getrlimit(RLIMIT_STACK, &stack);
    text += "\nstack " + std::to_string(static_cast<unsigned long long>(stack.rlim_cur));
    for (const int each : {SIGINT, SIGHUP}) {
        struct sigaction now {};
        sigaction(each, nullptr, &now);
        text += std::string(each == SIGINT ? "\nsigint " : "\nsighup ") + (now.sa_handler == SIG_IGN   ? "ignored"
                                                                           : now.sa_handler == SIG_DFL ? "default"
                                                                                                       : "caught");
    }
    sigset_t blocked;
    sigemptyset(&blocked);
    sigprocmask(SIG_BLOCK, nullptr, &blocked);
    bool any = false;
    for (int each = 1; each < NSIG; ++each)
        if (sigismember(&blocked, each) == 1)
            any = true;
    text += std::string("\nblocked ") + (any ? "some" : "none") + "\n";

    if (const char *record = std::getenv("PROMPT_RUN_RECORD")) {
        const std::string part = std::string(record) + ".part";
        std::ofstream(part) << text;
        std::rename(part.c_str(), record);
    }
    if (const char *release = std::getenv("PROMPT_RUN_RELEASE"))
        for (int waited = 0; waited < 2000 && access(release, F_OK) != 0; ++waited)
            usleep(10 * 1000);
    const char *code = std::getenv("PROMPT_RUN_CODE");
    return code != nullptr ? std::atoi(code) : 0;
}

// A satl WHOSE FILE IS GONE -- what a console opened before an install is, the installer having renamed a
// new satl over it -- starting a satellite: its /proc/self/exe ends "(deleted)", and the exec must fail
// where the start can say so. Writes "started yes|no" and "why <why>" into `record`.
int a_deleted_satl_starts_one(const char *record)
{
    char self[4096];
    const ssize_t length = readlink("/proc/self/exe", self, sizeof self - 1);
    if (length <= 0)
        return 2;
    self[length] = '\0';
    unlink(self);
    std::string why;
    const bool started = start_a_satellite_of_its_own({"--run", "prog.satl"}, why);
    std::ofstream(record) << "started " << (started ? "yes" : "no") << "\nwhy " << why << "\n";
    return 0;
}

// ---------------------------------------------------------------------------------------------------------
// THE HARNESS.
// ---------------------------------------------------------------------------------------------------------

int failed = 0;
int passed = 0;
std::string room;               // the fresh folder every case works in
std::string prompts_folder;     // where the prompt "is" when it runs a line: a folder of the room's
unsigned long long given = 0;   // the stack this harness was given, before it widened its own
bool widened = false;           // ...and whether it was widened, so that a child handed `given` shows
FILE *where_started = nullptr;  // the C stderr while a line runs: "where satl was started"
pid_t the_harness = 0;          // this process, the one whose exit handlers are its own to run
char exit_marker[4096] = {};    // made by any other process that runs them

void check(bool right, const std::string &name, const std::string &detail = "")
{
    if (right) {
        ++passed;
        std::printf("ok %s\n", name.c_str());
    } else {
        ++failed;
        std::printf("FAIL %s: %s\n", name.c_str(), detail.empty() ? "(no detail)" : detail.c_str());
    }
    std::fflush(stdout);
}

void skipped(const std::string &name, const char *why)
{
    ++passed;
    std::printf("ok %s (skipped: %s)\n", name.c_str(), why);
    std::fflush(stdout);
}

bool there(const std::string &path) { return access(path.c_str(), F_OK) == 0; }

// THE STACK A CHILD WAS HANDED, where it can be told apart: only a widened stack shows one that was not.
void check_the_stack(const std::string &stack, const std::string &name)
{
    if (widened)
        check(stack == std::to_string(given), name, "stack " + stack + ", given " + std::to_string(given));
    else
        skipped(name, "the stack was not widened, so a child cannot be told apart from one handed satl's stack");
}

// A FORK THAT RAN THIS PROCESS'S EXIT HANDLERS LEAVES A MARK. The middle one of a start is a fork of a satl
// with threads and ends by _exit: exit() there would run satl's handlers -- the destructor of the desk's
// joinable std::thread among them, which aborts.
void mark_a_foreign_exit()
{
    if (getpid() != the_harness && exit_marker[0] != '\0') {
        const int fd = open(exit_marker, O_WRONLY | O_CREAT | O_CLOEXEC, 0600);
        if (fd >= 0)
            close(fd);
    }
}

// A MIDDLE ONE ENDED BEFORE IT CAN FORK THE NEW satl: fork(), which makes it, runs pthread_atfork's child
// handlers in it, and _Fork, which makes the new satl, runs none -- so this ends it there, by SIGKILL, or by
// _exit(129), what satl's own hangup handler does in a fork of satl. 0 leaves it be.
int end_the_middle_one = 0;
void ends_the_middle_one()
{
    if (end_the_middle_one == SIGKILL)
        raise(SIGKILL);
    else if (end_the_middle_one != 0)
        _exit(end_the_middle_one);
}

// HOW MANY FILES THIS PROCESS HOLDS OPEN, as /proc counts them.
int open_descriptors()
{
    int count = 0;
    if (DIR *open_ones = opendir("/proc/self/fd")) {
        while (const dirent *entry = readdir(open_ones))
            if (entry->d_name[0] != '.')
                ++count;
        closedir(open_ones);
    }
    return count;
}

std::string identity_of_path(const std::string &path)
{
    struct stat about {};
    return stat(path.c_str(), &about) == 0 ? identity(about) : std::string("missing");
}

// A STAND-IN'S RECORD, read once it is there -- waiting `most_ms` for it. Empty when none came.
std::map<std::string, std::string> record_of(const std::string &path, int most_ms)
{
    std::map<std::string, std::string> read;
    for (int waited = 0; !there(path) && waited < most_ms; waited += 10)
        usleep(10 * 1000);
    std::ifstream file(path);
    std::string line;
    while (std::getline(file, line)) {
        const std::size_t space = line.find(' ');
        if (space != std::string::npos)
            read[line.substr(0, space)] = line.substr(space + 1);
    }
    return read;
}

// WHAT /proc SAYS OF A PROCESS: its state letter, or 0 when it is gone.
char state_of(pid_t pid)
{
    std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(file, line))
        return 0;
    const std::size_t close = line.rfind(')');
    return close != std::string::npos && close + 2 < line.size() ? line[close + 2] : 0;
}

// WHAT ONE LINE AT THE PROMPT DID.
struct Line {
    bool taken = false;              // run_a_file_from_the_prompt answered true
    long long answer = -1;
    double seconds = 0;              // how long the prompt took to come back
    std::string said;                // what reached the console's pty
    std::string console;             // the pty's identity, for the records to be held against
};

// ONE LINE AT THE PROMPT, IN satl'S OWN CONSOLE AS FAR AS IT GOES: descriptors 0, 1 and 2 on a pty and the C
// stderr on where_started, from prompts_folder. `call` is the line; what reached the pty is read before the
// pty is let go.
Line on_a_console(const std::function<bool(long long &)> &call)
{
    Line line;
    std::fflush(stdout);
    std::cout.flush();
    std::cerr.flush();
    const int master = posix_openpt(O_RDWR | O_NOCTTY | O_CLOEXEC);
    char name[256] = {};
    if (master < 0 || grantpt(master) != 0 || unlockpt(master) != 0 || ptsname_r(master, name, sizeof name) != 0) {
        line.said = std::string("(no pty: ") + std::strerror(errno) + ")";
        return line;
    }
    const int slave = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC);
    line.console = identity_of(slave);
    int saved[3];
    for (int fd = 0; fd < 3; ++fd) {
        saved[fd] = fcntl(fd, F_DUPFD_CLOEXEC, 10);
        dup2(slave, fd);
    }
    FILE *was = stderr;
    stderr = where_started;
    char before[4096];
    const bool moved = getcwd(before, sizeof before) != nullptr && chdir(prompts_folder.c_str()) == 0;

    const auto started = std::chrono::steady_clock::now();
    long long answer = -1;
    line.taken = call(answer);
    line.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - started).count();
    line.answer = answer;
    std::cerr.flush();

    if (moved)
        (void)!chdir(before);
    stderr = was;
    char buffer[4096];
    for (;;) {
        pollfd ready{master, POLLIN, 0};
        if (poll(&ready, 1, 200) <= 0 || (ready.revents & POLLIN) == 0)
            break;
        const ssize_t got = read(master, buffer, sizeof buffer);
        if (got <= 0)
            break;
        line.said.append(buffer, static_cast<std::size_t>(got));
    }
    for (int fd = 0; fd < 3; ++fd) {
        dup2(saved[fd], fd);
        close(saved[fd]);
    }
    close(slave);
    close(master);
    return line;
}

Line at_the_prompt(const std::string &typed)
{
    return on_a_console([&typed](long long &answer) {
        signed long long int ran = -1;
        const bool taken = run_a_file_from_the_prompt(typed, ran);
        answer = ran;
        return taken;
    });
}

// A CASE'S STAND-IN IS TOLD WHERE TO WRITE, WHETHER TO WAIT AND WHAT TO END WITH, through what every child
// inherits: the environment.
int cases_so_far = 0;
struct StandIn {
    std::string record;
    std::string release;
};
StandIn a_stand_in(bool waits, int code)
{
    ++cases_so_far;
    StandIn made{room + "/record-" + std::to_string(cases_so_far), room + "/release-" + std::to_string(cases_so_far)};
    setenv("PROMPT_RUN_RECORD", made.record.c_str(), 1);
    if (waits)
        setenv("PROMPT_RUN_RELEASE", made.release.c_str(), 1);
    else
        unsetenv("PROMPT_RUN_RELEASE");
    setenv("PROMPT_RUN_CODE", std::to_string(code).c_str(), 1);
    return made;
}

void let_go(const StandIn &stand_in) { std::ofstream(stand_in.release) << "go\n"; }

// NOTHING IS THIS HARNESS'S CHILD: no satellite, and no middle one left unreaped.
bool no_child_left(std::string &seen)
{
    int status = 0;
    const pid_t child = waitpid(-1, &status, WNOHANG);
    const int error = errno;
    seen = child < 0 ? std::string("waitpid: ") + std::strerror(error) : "a child " + std::to_string(child);
    return child < 0 && error == ECHILD;
}

// ---------------------------------------------------------------------------------------------------------
// ANYWHERE BUT satl'S OWN CONSOLE: run here, and waited for -- exactly as before 2026-10-20.
// ---------------------------------------------------------------------------------------------------------
void here_and_waited_for()
{
    the_console_is_open = false;
    const char *where = "away from satl's own console: ";

    StandIn stand_in = a_stand_in(false, 7);
    Line line = at_the_prompt("interpret prog.satl a b");
    std::map<std::string, std::string> got = record_of(stand_in.record, 0);
    check(line.taken && line.answer == 7, std::string(where) + "interpret answers the program's own exit status, so it waited for its end",
          "taken " + std::to_string(line.taken) + ", answer " + std::to_string(line.answer));
    check(got["words"] == "--run|prog.satl|a|b", std::string(where) + "... started as satl --run prog.satl a b",
          "words " + got["words"]);
    check(got["session"] == std::to_string(getsid(0)) && got["fd1"] == line.console && got["fd0"] == line.console,
          std::string(where) + "... in the prompt's own session, on the prompt's own terminal",
          "session " + got["session"] + ", fd0 " + got["fd0"] + ", fd1 " + got["fd1"] + ", the pty " + line.console);
    check(got["folder"] == prompts_folder, std::string(where) + "... from the folder the prompt is in", "folder " + got["folder"]);
    check_the_stack(got["stack"], std::string(where) + "... with the stack satl was given");
    check(line.said.find("satellite of its own") == std::string::npos,
          std::string(where) + "... and no satellite is spoken of", "said: " + line.said);

    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("run -dash.satl");
    got = record_of(stand_in.record, 0);
    check(line.taken && line.answer == 0 && got["words"] == "--run|-dash.satl",
          std::string(where) + "run -dash.satl: a file whose name begins with - stays a file", "words " + got["words"]);

    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("satellite.interpret \"two words.satl\" 'c d' e");
    got = record_of(stand_in.record, 0);
    check(line.taken && got["words"] == "--run|two words.satl|c d|e",
          std::string(where) + "satellite.interpret, its words split as a shell splits them", "words " + got["words"]);

    // A FILE THAT IS NOT THERE IS STILL THE PROGRAM'S satl'S TO REFUSE, with its code: the prompt asks nothing of
    // it here.
    stand_in = a_stand_in(false, missing_satl_file);
    line = at_the_prompt("interpret there-is-no-such-file.satl");
    got = record_of(stand_in.record, 0);
    check(line.taken && line.answer == missing_satl_file && got["words"] == "--run|there-is-no-such-file.satl" &&
              line.said.find("satellite of its own") == std::string::npos,
          std::string(where) + "a file that is not there is still refused by the satl that runs it",
          "answer " + std::to_string(line.answer) + ", words " + got["words"] + ", said: " + line.said);

    // AND IN satl'S OWN CONSOLE UNDER SATL_NO_WINDOW, which keeps a satl where it was pointed: the program never
    // opened a console of its own there, so it runs in this one and is waited for, as it always was.
    const char *no_window = "in satl's own console under SATL_NO_WINDOW: ";
    the_console_is_open = true;
    nobody_gave_one = false;
    stand_in = a_stand_in(false, 7);
    line = at_the_prompt("interpret prog.satl a b");
    got = record_of(stand_in.record, 0);
    the_console_is_open = false;
    nobody_gave_one = true;
    check(line.taken && line.answer == 7 && got["words"] == "--run|prog.satl|a|b",
          std::string(no_window) + "interpret runs satl --run prog.satl a b and answers its exit status, so it waited",
          "taken " + std::to_string(line.taken) + ", answer " + std::to_string(line.answer) + ", words " + got["words"]);
    check(got["session"] == std::to_string(getsid(0)) && got["fd1"] == line.console &&
              line.said.find("satellite of its own") == std::string::npos,
          std::string(no_window) + "... in this console's session and on its pty, and no satellite is spoken of",
          "session " + got["session"] + ", fd1 " + got["fd1"] + ", the pty " + line.console + ", said: " + line.said);
}

// ---------------------------------------------------------------------------------------------------------
// IN satl'S OWN CONSOLE: a satellite of its own, and the prompt back at once.
// ---------------------------------------------------------------------------------------------------------
void a_satellite_of_its_own()
{
    the_console_is_open = true;
    const char *where = "in satl's own console: ";
    const std::string started_here = identity_of(fileno(where_started));
    const std::string null_device = identity_of_path("/dev/null");

    // A PROMPT IN AN ODD STATE, which the new satl must not inherit: Ctrl-C ignored, a hangup ignored -- as a
    // console that holds ignores it (console_launch.cpp), where File > Open… still starts one -- a signal
    // blocked, and a file held open without close-on-exec.
    struct sigaction ignore {}, before {}, hangup_before {};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGINT, &ignore, &before);
    sigaction(SIGHUP, &ignore, &hangup_before);
    sigset_t one, mask_before;
    sigemptyset(&one);
    sigaddset(&one, SIGUSR2);
    sigprocmask(SIG_BLOCK, &one, &mask_before);
    const int held = open((room + "/held open").c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0600);

    StandIn stand_in = a_stand_in(true, 3);
    const int descriptors_before = open_descriptors();
    const int handed_before = handed_to_the_desk;
    Line line = at_the_prompt("interpret prog.satl a b");
    const int handed = handed_to_the_desk - handed_before;
    const int descriptors_after = open_descriptors();
    std::string seen;
    const bool none_left = no_child_left(seen);
    std::map<std::string, std::string> got = record_of(stand_in.record, 10000);
    const pid_t pid = static_cast<pid_t>(std::atol(got["pid"].c_str()));
    const char state = pid > 0 ? state_of(pid) : 0;

    sigaction(SIGINT, &before, nullptr);
    sigaction(SIGHUP, &hangup_before, nullptr);
    sigprocmask(SIG_SETMASK, &mask_before, nullptr);
    if (held >= 0)
        close(held);

    check(line.taken && line.answer == success && line.seconds < 5,
          std::string(where) + "interpret comes back at once, answering success, while the program runs on",
          "taken " + std::to_string(line.taken) + ", answer " + std::to_string(line.answer) + ", " +
              std::to_string(line.seconds) + " s");
    check(line.said.find("satl(prompt): prog.satl is running in a satellite of its own") != std::string::npos,
          std::string(where) + "... and says so on one line, on the console", "said: " + line.said);
    check(!got.empty() && pid > 0 && state != 0 && state != 'Z',
          std::string(where) + "... the program is still running when the prompt is back",
          got.empty() ? "no record" : "state " + std::string(1, state ? state : '-'));
    check(got["words"] == "--console|--run|prog.satl|a|b",
          std::string(where) + "... started as satl --console --run prog.satl a b", "words " + got["words"]);
    check(handed == 1, std::string(where) + "... handed to the desk to start, where File > Open… starts one",
          std::to_string(handed) + " handed to start_a_satellite_on_the_desk");
    check(got["folder"] == prompts_folder, std::string(where) + "... from the folder the prompt is in", "folder " + got["folder"]);
    check(got["session"] == got["pid"] && got["session"] != std::to_string(getsid(0)) && got["terminal"] == "no",
          std::string(where) + "... in a session of its own, with no terminal, so this console's Ctrl-C and hangup never reach it",
          "session " + got["session"] + " of pid " + got["pid"] + ", terminal " + got["terminal"]);
    check(got["fd0"] == null_device, std::string(where) + "... stdin /dev/null", "fd0 " + got["fd0"] + ", /dev/null " + null_device);
    check(got["fd1"] == started_here && got["fd2"] == started_here && got["fd1"] != line.console,
          std::string(where) + "... stdout and stderr where satl was started, never the console's pty",
          "fd1 " + got["fd1"] + ", fd2 " + got["fd2"] + ", started " + started_here + ", the pty " + line.console);
    check(got["others"] == "none", std::string(where) + "... and no other file of satl's", "others " + got["others"]);
    check(got["sigint"] == "default" && got["sighup"] == "default" && got["blocked"] == "none",
          std::string(where) + "... Ctrl-C and a hangup its own to take, and no signal blocked",
          "sigint " + got["sigint"] + ", sighup " + got["sighup"] + ", blocked " + got["blocked"]);
    check_the_stack(got["stack"], std::string(where) + "... with the stack satl was given");
    check(none_left && got["parent"] != std::to_string(getpid()),
          std::string(where) + "... never this satl's child, and nothing left to reap", seen + ", its parent " + got["parent"]);
    // AND NOTHING OF THE START LEFT IN THE PROMPT: 24 programs started must not be 24 files held for the
    // console's life, and the middle one, a fork of this process, ends without running its exit handlers.
    check(descriptors_before == descriptors_after, std::string(where) + "... and the prompt keeps no file of the start",
          std::to_string(descriptors_before) + " open before, " + std::to_string(descriptors_after) + " after");
    check(!there(exit_marker), std::string(where) + "... and no process but this one ran its exit handlers",
          std::string(exit_marker) + " was made");

    let_go(stand_in);
    char after = state;
    for (int waited = 0; waited < 10000 && after != 0 && after != 'Z'; waited += 10) {
        usleep(10 * 1000);
        after = state_of(pid);
    }
    check(pid > 0 && (after == 0 || after == 'Z') && no_child_left(seen),
          std::string(where) + "... and when it ends, it ends somewhere else", "state " + std::string(1, after ? after : '-') + ", " + seen);

    // THE SAME WORDS AS ANYWHERE ELSE, AFTER --console.
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("run -dash.satl");
    got = record_of(stand_in.record, 10000);
    check(line.taken && line.answer == success && got["words"] == "--console|--run|-dash.satl",
          std::string(where) + "run -dash.satl: a file whose name begins with - stays a file", "words " + got["words"]);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("satellite.run \"two words.satl\" 'c d' e");
    got = record_of(stand_in.record, 10000);
    check(line.taken && got["words"] == "--console|--run|two words.satl|c d|e",
          std::string(where) + "satellite.run, its words split as a shell splits them", "words " + got["words"]);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("satellite.interpret prog.satl");
    got = record_of(stand_in.record, 10000);
    check(line.taken && got["words"] == "--console|--run|prog.satl" &&
              line.said.find("satl(prompt): prog.satl is running in a satellite of its own") != std::string::npos,
          std::string(where) + "satellite.interpret too", "words " + got["words"] + ", said: " + line.said);

    // WHAT IS REFUSED IS REFUSED AS BEFORE, AND STARTS NOTHING.
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("interpret");
    check(line.taken && line.answer == satl_line_not_understood &&
              line.said.find("`interpret` needs a file after it") != std::string::npos && record_of(stand_in.record, 500).empty(),
          std::string(where) + "interpret with no file is refused as before, and starts nothing", "said: " + line.said);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("run \"never closed");
    check(line.taken && line.answer == satl_line_not_understood &&
              line.said.find("a quote on the `run` line is never closed") != std::string::npos &&
              record_of(stand_in.record, 500).empty(),
          std::string(where) + "a quote never closed is refused as before, and starts nothing", "said: " + line.said);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("satellite.console.display(\"x\")");
    check(!line.taken && record_of(stand_in.record, 300).empty(), std::string(where) + "any other line is not the prompt's word");

    // A FILE THAT IS NOT THERE, OR IS NOT A FILE, IS SAID ON THE LINE, with the code satl itself answers for
    // it, and starts nothing -- never "running" for a satellite that would open only to hold on it.
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("interpret there-is-no-such-file.satl");
    check(line.taken && line.answer == missing_satl_file &&
              line.said.find("satl(prompt): there-is-no-such-file.satl could not be started in a satellite of its own: "
                             "cannot locate file") != std::string::npos &&
              line.said.find("is running") == std::string::npos && record_of(stand_in.record, 500).empty(),
          std::string(where) + "a file that is not there is said on the line, answering missing_satl_file, and starts nothing",
          "answer " + std::to_string(line.answer) + ", said: " + line.said);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("run .");
    check(line.taken && line.answer == missing_satl_file &&
              line.said.find("satl(prompt): . could not be started in a satellite of its own: it is a directory, and a "
                             "program is a .satl file") != std::string::npos &&
              record_of(stand_in.record, 500).empty(),
          std::string(where) + "a folder is said on the line the same way, and starts nothing",
          "answer " + std::to_string(line.answer) + ", said: " + line.said);
    stand_in = a_stand_in(false, 0);
    line = at_the_prompt("interpret \"\"");
    check(line.taken && line.answer == missing_satl_file &&
              line.said.find("satl(prompt): (an empty name) could not be started in a satellite of its own: cannot "
                             "locate file") != std::string::npos &&
              record_of(stand_in.record, 500).empty(),
          std::string(where) + "an empty name is named as satl names one, and starts nothing",
          "answer " + std::to_string(line.answer) + ", said: " + line.said);

    // ONE THAT CANNOT BE STARTED IS SAID ON THE LINE: no process may be made (RLIMIT_NPROC at 0).
    struct rlimit processes {};
    getrlimit(RLIMIT_NPROC, &processes);
    struct rlimit none_at_all = processes;
    none_at_all.rlim_cur = 0;
    setrlimit(RLIMIT_NPROC, &none_at_all);
    const pid_t probe = fork();
    if (probe == 0)
        _exit(0);
    if (probe > 0) {
        waitpid(probe, nullptr, 0);
        setrlimit(RLIMIT_NPROC, &processes);
        skipped(std::string(where) + "a satellite that cannot be started is said on the line",
                "RLIMIT_NPROC does not stop this user making a process");
    } else {
        stand_in = a_stand_in(false, 0);
        line = at_the_prompt("interpret prog.satl");
        setrlimit(RLIMIT_NPROC, &processes);
        check(line.taken && line.answer == satl_line_not_understood &&
                  line.said.find("satl(prompt): prog.satl could not be started in a satellite of its own: " +
                                 std::string(std::strerror(EAGAIN))) != std::string::npos &&
                  record_of(stand_in.record, 500).empty(),
              std::string(where) + "a satellite that cannot be started is said on the line, answering satl_line_not_understood",
              "answer " + std::to_string(line.answer) + ", said: " + line.said);
    }
}

// ---------------------------------------------------------------------------------------------------------
// THE START ALONE, GIVEN WHAT THE CONSOLE MENU GIVES IT: New window's words, Open…'s, a satl whose file is
// gone, and a middle one ended before it could fork. console_menu.cpp is GTK's and is not built here;
// check.sh pins the words it passes.
// ---------------------------------------------------------------------------------------------------------
void the_menus_start()
{
    const char *where = "the shared start: ";
    StandIn stand_in = a_stand_in(false, 0);
    std::string why;
    bool started = false;
    Line line = on_a_console([&](long long &) { return started = start_a_satellite_of_its_own({}, why); });
    std::map<std::string, std::string> got = record_of(stand_in.record, 10000);
    check(started && got["words"] == "--console", std::string(where) + "given New window's words, nothing: satl --console",
          "started " + std::to_string(started) + " (" + why + "), words " + got["words"]);

    stand_in = a_stand_in(false, 0);
    const std::string chosen = prompts_folder + "/chosen.satl";
    line = on_a_console([&](long long &) { return started = start_a_satellite_of_its_own({chosen}, why); });
    got = record_of(stand_in.record, 10000);
    check(started && got["words"] == "--console|" + chosen && got["fd1"] != line.console && got["session"] == got["pid"],
          std::string(where) + "given Open…'s words, the file: satl --console <the file>, in a session of its own, off the console's pty",
          "words " + got["words"] + ", session " + got["session"] + ", fd1 " + got["fd1"]);

    // THE EXEC FAILS AND THE START SAYS SO -- never "running" for a satl that never ran.
    const std::string copy = room + "/deleted-satl";
    const std::string record = room + "/deleted-record";
    {
        std::ifstream from("/proc/self/exe", std::ios::binary);
        std::ofstream to(copy, std::ios::binary);
        to << from.rdbuf();
    }
    chmod(copy.c_str(), 0700);
    stand_in = a_stand_in(false, 0);
    const pid_t child = fork();
    if (child == 0) {
        char *const words[] = {const_cast<char *>(copy.c_str()), const_cast<char *>("--a-deleted-satl-starts-one"),
                               const_cast<char *>(record.c_str()), nullptr};
        execv(copy.c_str(), words);
        _exit(127);
    }
    int status = -1;
    if (child > 0)
        waitpid(child, &status, 0);
    got = record_of(record, 0);
    const std::string ending = "(deleted): " + std::string(std::strerror(ENOENT));
    check(child > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0 && got["started"] == "no" &&
              got["why"].size() >= ending.size() && got["why"].compare(got["why"].size() - ending.size(), ending.size(), ending) == 0 &&
              record_of(stand_in.record, 500).empty(),
          std::string(where) + "a satl whose file is gone cannot start one, and the start says why",
          "started " + got["started"] + ", why " + got["why"]);

    // A MIDDLE ONE ENDED BEFORE IT COULD FORK closes the pipe empty, as an exec that succeeded does: how it
    // ended tells the two apart, and the start says so -- never "running" for a satl that never ran.
    for (const int end : {SIGKILL, 128 + SIGHUP}) {
        stand_in = a_stand_in(false, 0);
        why.clear();
        end_the_middle_one = end;
        started = start_a_satellite_of_its_own({"--run", "prog.satl"}, why);
        end_the_middle_one = 0;
        check(!started && !why.empty() && record_of(stand_in.record, 500).empty(),
              std::string(where) + "a middle one ended before it could fork, " +
                  (end == SIGKILL ? "by a signal" : "by satl's hangup handler") + ": the start says so, and nothing runs",
              "started " + std::to_string(started) + ", why " + why);
    }
}

int remove_one(const char *path, const struct stat *, int, struct FTW *)
{
    return std::remove(path);
}

} // namespace

int main(int argc, char **argv)
{
    if (argc > 1 && (std::strcmp(argv[1], "--run") == 0 || std::strcmp(argv[1], "--console") == 0))
        return be_the_stand_in(argc, argv);
    if (argc == 3 && std::strcmp(argv[1], "--a-deleted-satl-starts-one") == 0)
        return a_deleted_satl_starts_one(argv[2]);
    if (argc != 2) {
        std::fprintf(stderr, "usage: prompt_run_cases <a folder to work in>\n");
        return 2;
    }
    // AS satl's main() DOES: std::cerr writes to descriptor 2 itself, which is the console's pty.
    std::ios::sync_with_stdio(false);

    std::string pattern = std::string(argv[1]) + "/prompt_run_cases.XXXXXX";
    std::vector<char> name(pattern.begin(), pattern.end());
    name.push_back('\0');
    if (::mkdtemp(name.data()) == nullptr) {
        std::printf("FAIL setup: could not make a folder in %s: %s\n", argv[1], std::strerror(errno));
        return 1;
    }
    char *whole = ::realpath(name.data(), nullptr);
    room = whole != nullptr ? whole : name.data();
    std::free(whole);
    prompts_folder = room + "/the prompt's folder";
    mkdir(prompts_folder.c_str(), 0700);
    // THE FILES THE LINES NAME ARE THERE, as a person's would be: in satl's own console the prompt starts a
    // satellite only for a file it can find. The stand-in never reads them.
    for (const char *file : {"prog.satl", "-dash.satl", "two words.satl"})
        std::ofstream(prompts_folder + "/" + file) << "satellite.include(satellite)\n";
    the_harness = getpid();
    std::snprintf(exit_marker, sizeof exit_marker, "%s/exit-handlers-ran", room.c_str());
    std::atexit(mark_a_foreign_exit);
    pthread_atfork(nullptr, nullptr, ends_the_middle_one);
    where_started = std::fopen((room + "/started.log").c_str(), "w");
    if (where_started == nullptr) {
        std::printf("FAIL setup: could not make %s/started.log: %s\n", room.c_str(), std::strerror(errno));
        return 1;
    }

    // THE STACK, AS satl's main() WIDENS IT (machine/stack_share.hpp): from 8 MiB where the hard limit lets
    // it be widened at all, so that a child handed the widened one would be seen.
    struct rlimit stack {};
    getrlimit(RLIMIT_STACK, &stack);
    const rlim_t eight = 8ULL * 1024 * 1024;
    if ((stack.rlim_max == RLIM_INFINITY || stack.rlim_max > eight) && (stack.rlim_cur == RLIM_INFINITY || stack.rlim_cur > eight)) {
        stack.rlim_cur = eight;
        setrlimit(RLIMIT_STACK, &stack);
    }
    getrlimit(RLIMIT_STACK, &stack);
    given = static_cast<unsigned long long>(stack.rlim_cur);
    widen_the_stack();
    getrlimit(RLIMIT_STACK, &stack);
    widened = static_cast<unsigned long long>(stack.rlim_cur) != given;
    if (widened)
        std::printf("note  the stack is widened as satl's is, from %llu bytes\n", given);
    else
        std::printf("note  the stack is not widened: this machine's limits do not let it be, so the stack cases are "
                    "skipped\n");

    here_and_waited_for();
    a_satellite_of_its_own();
    the_menus_start();

    std::fclose(where_started);
    ::nftw(room.c_str(), remove_one, 16, FTW_DEPTH | FTW_PHYS);
    check(!there(room), "cleanup: the folder it made is gone", room + " is still there");

    std::printf("%d ok, %d failed: %s\n", passed, failed,
                failed == 0 ? "every interpret and run case passed" : "an interpret or run case FAILED");
    return failed == 0 ? 0 : 1;
}
