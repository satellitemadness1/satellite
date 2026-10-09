// satellite/satellite_test/test_run.cpp -- a satellite.test program in a child satl. test_run.hpp
// says why a child, why its own place, why stdout and stderr apart, and how it is stopped.

#include "test_run.hpp"

#include "../machine/stop_flag.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_variable_program/program_spawn.hpp"

#include <cerrno>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <filesystem>
#include <fstream>
#include <mutex>
#include <poll.h>
#include <sstream>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {

namespace {

// A CTRL-C WHILE A TEST RUNS FROM A FILE (CtrlCDuringATest): the handler only says so.
volatile sig_atomic_t ctrl_c_asked = 0;
std::mutex guards_lock;
int guards = 0;
struct sigaction before_guards {};

void on_ctrl_c(int)
{
    ctrl_c_asked = 1;
}

// THIS SAME satl, by the path the kernel ran (prompt_run.cpp's this_satl): a build/satl runs its
// tests with build/satl, an installed one with itself. A satl installed again while a test runs
// has had its file replaced, and the kernel then names it "... (deleted)": the new one at the
// same path runs the rest (the review, 2026-10-06).
std::string this_satl()
{
    std::string path(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", path.data(), path.size());
    if (length <= 0)
        return "satl";
    path.resize(static_cast<std::size_t>(length));
    static const std::string gone = " (deleted)";
    if (path.size() > gone.size() && path.compare(path.size() - gone.size(), gone.size(), gone) == 0)
        path.resize(path.size() - gone.size());
    return path;
}

// THE PASSES LINE, rewritten: the first line that is `    satellite.variable.number passes = <N>`
// gets `passes` for its N. make_test_programs.py refuses a program without one.
std::string with_passes(const std::string &text, long long passes)
{
    if (passes <= 0)
        return text;
    static const std::string line = "\n    satellite.variable.number passes = ";
    const std::size_t at = text.find(line);
    if (at == std::string::npos)
        return text;
    const std::size_t number = at + line.size();
    std::size_t end = number;
    while (end < text.size() && text[end] >= '0' && text[end] <= '9')
        ++end;
    return text.substr(0, number) + std::to_string(passes) + text.substr(end);
}

std::string read_all(const std::string &path, std::uintmax_t from = 0)
{
    std::ifstream file(path, std::ios::binary);
    if (from > 0)
        file.seekg(static_cast<std::streamoff>(from));
    std::ostringstream text;
    text << file.rdbuf();
    return text.str();
}

std::uintmax_t size_of(const std::string &path)
{
    std::error_code missing;
    const std::uintmax_t size = std::filesystem::file_size(path, missing);
    return missing ? 0 : size;
}

// THE CHILD AND EVERYTHING IT STARTED: found first, then the child -- ours and not yet waited
// for, so its pid is still its own -- then the rest, each only while it is still that process.
void kill_everything_under(pid_t child)
{
    const std::vector<ProcessSeen> under = processes_under(child);
    ::kill(child, SIGKILL);
    for (const ProcessSeen &process : under)
        signal_if_still_there(process, SIGKILL);
}

} // namespace

bool test_stop_wanted()
{
    return (stop_flag() != nullptr && *stop_flag() != 0) || ctrl_c_asked != 0 ||
           (stop_of_this_thread != nullptr && stop_of_this_thread->load()) || program_quit().load();
}

CtrlCDuringATest::CtrlCDuringATest()
{
    if (stop_flag() != nullptr)
        return;
    const std::lock_guard<std::mutex> held(guards_lock);
    if (guards == 0) {
        // A SIGINT SATL WAS STARTED IGNORING stays ignored: a Ctrl-C would not have stopped it.
        struct sigaction now {};
        if (sigaction(SIGINT, nullptr, &now) != 0 || now.sa_handler == SIG_IGN)
            return;
        struct sigaction ask {};
        ask.sa_handler = on_ctrl_c;
        sigemptyset(&ask.sa_mask);
        ask.sa_flags = 0;   // no SA_RESTART: the wait's poll wakes at once
        sigaction(SIGINT, &ask, &before_guards);
    }
    ++guards;
    installed_ = true;
}

CtrlCDuringATest::~CtrlCDuringATest()
{
    if (!installed_)
        return;
    bool end_now = false;
    {
        const std::lock_guard<std::mutex> held(guards_lock);
        if (--guards == 0) {
            sigaction(SIGINT, &before_guards, nullptr);
            end_now = ctrl_c_asked != 0;
        }
    }
    // THE CTRL-C, AS IT WOULD HAVE BEEN: the test's place is gone and its lines are out.
    if (end_now)
        raise(SIGINT);
}

long long test_passes_override()
{
    const char *given = std::getenv("SATL_TEST_PASSES");
    if (given == nullptr || *given == '\0')
        return 0;
    char *end = nullptr;
    const long long passes = std::strtoll(given, &end, 10);
    return end != nullptr && *end == '\0' && passes > 0 ? passes : 0;
}

TestPlace::TestPlace()
{
    const char *tmp = std::getenv("TMPDIR");
    const std::string pattern = std::string(tmp != nullptr && *tmp != '\0' ? tmp : "/tmp") + "/satl-test-XXXXXX";
    std::vector<char> made(pattern.begin(), pattern.end());
    made.push_back('\0');
    if (mkdtemp(made.data()) == nullptr) {
        why_ = "satellite.test could not make a folder to run its programs in (" + pattern + "): " +
               std::strerror(errno);
        return;
    }
    folder_ = made.data();
    std::error_code failed;
    std::filesystem::create_directories(folder_ + "/home/.satl", failed);
    if (!failed)
        std::filesystem::create_directory(folder_ + "/run", failed);
    // satl's BUILT-IN DEFAULTS, written by `satl --rebuild` into the home, as check.sh makes its own.
    if (!failed) {
        const ProgramPlace place{folder_, {"HOME=" + folder_ + "/home", "SATL_NO_WINDOW=1"}};
        const ProgramStart start = start_a_program({this_satl(), "--rebuild"}, -1, -1, -1, place);
        if (start.pid > 0) {
            int status = 0;
            while (waitpid(start.pid, &status, 0) < 0 && errno == EINTR) {
            }
            if (start.pidfd >= 0)
                close(start.pidfd);
        }
        if (!std::filesystem::is_regular_file(folder_ + "/home/.satl/config.ini", failed) && !failed)
            failed = std::make_error_code(std::errc::no_such_file_or_directory);
    }
    if (failed) {
        why_ = "satellite.test could not make the folders and settings it runs its programs with, in " + folder_ +
               ": " + failed.message();
        std::error_code gone;
        std::filesystem::remove_all(folder_, gone);
        folder_.clear();
    }
}

TestPlace::~TestPlace()
{
    if (folder_.empty())
        return;
    std::error_code gone;
    std::filesystem::remove_all(folder_, gone);
}

TestRun TestPlace::run(const TestProgram &program, long long passes, double limit)
{
    TestRun run;
    if (folder_.empty()) {
        run.why = why_;
        return run;
    }
    // AN EMPTY FOLDER FOR EVERY PROGRAM: what the one before left behind is gone first.
    const std::string folder = folder_ + "/run";
    std::error_code failed;
    std::filesystem::remove_all(folder, failed);
    if (!failed)
        std::filesystem::create_directory(folder, failed);
    if (failed) {
        run.why = "satellite.test could not empty " + folder + " for " + program.name + ": " + failed.message();
        return run;
    }
    const std::string path = folder + "/" + program.name;
    const std::string errors_path = folder_ + "/errors";
    const std::string log_path = folder_ + "/home/.satl/satellite.log";
    {
        std::ofstream written(path, std::ios::binary);
        written << with_passes(program.text, passes);
        if (!written) {
            run.why = "satellite.test could not write " + path;
            return run;
        }
    }

    int ends[2] = {-1, -1};
    const int errors = open(errors_path.c_str(), O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC, 0600);
    if (errors < 0 || pipe2(ends, O_CLOEXEC) != 0) {
        run.why = "satellite.test could not make the pipe and file to read " + std::string(program.name) + " by: " +
                  std::strerror(errno);
        if (errors >= 0)
            close(errors);
        return run;
    }

    // BY ITS NAME, FROM ITS FOLDER: the program is started as `satl <file>` is typed beside it.
    const std::uintmax_t logged_before = size_of(log_path);
    const ProgramPlace place{folder, {"HOME=" + folder_ + "/home", "SATL_NO_WINDOW=1"}};
    const std::chrono::steady_clock::time_point began = std::chrono::steady_clock::now();
    const ProgramStart start = start_a_program({this_satl(), "--run", program.name}, ends[1], -1, errors, place);
    close(ends[1]);
    close(errors);
    if (start.pid <= 0) {
        close(ends[0]);
        run.why = "satellite.test could not start satl for " + std::string(program.name) + ": " + start.why;
        return run;
    }

    // ITS OUTPUT, READ TO THE END -- the pipe ends when the child does, and anything it started --
    // while the child is watched by its pidfd, and a tenth of a second at a time the stop and the
    // limit are asked.
    char piece[65536];
    int status = 0;
    bool reading = true, ended = false, killed = false;
    for (;;) {
        struct pollfd watch[2];
        nfds_t watching = 0;
        if (reading)
            watch[watching++] = {ends[0], POLLIN, 0};
        if (!ended && start.pidfd >= 0)
            watch[watching++] = {start.pidfd, POLLIN, 0};
        if (watching > 0 && poll(watch, watching, 100) > 0) {
            for (nfds_t n = 0; n < watching; ++n) {
                if (watch[n].revents == 0)
                    continue;
                if (watch[n].fd == ends[0]) {
                    const ssize_t got = read(ends[0], piece, sizeof piece);
                    if (got > 0)
                        run.out.append(piece, static_cast<std::size_t>(got));
                    else if (got == 0 || (errno != EINTR && errno != EAGAIN))
                        reading = false;
                } else if (waitpid(start.pid, &status, WNOHANG) == start.pid) {
                    ended = true;
                }
            }
        } else if (watching == 0) {
            // NO PIDFD ON THIS KERNEL: its end is asked for, a hundredth of a second at a time.
            const pid_t done = waitpid(start.pid, &status, WNOHANG);
            if (done == start.pid || (done < 0 && errno != EINTR))
                ended = true;
            else
                poll(nullptr, 0, 10);
        }
        if (ended && !reading)
            break;
        if (!killed) {
            const double so_far = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();
            const bool stop = test_stop_wanted();
            if (stop || (limit > 0.0 && so_far > limit)) {
                if (!ended)
                    kill_everything_under(start.pid);
                killed = true;
                run.interrupted = stop;
                run.timed_out = !stop;
                reading = false;
            }
        }
        if (killed && !ended) {
            while (waitpid(start.pid, &status, 0) < 0 && errno == EINTR) {
            }
            ended = true;
        }
        if (killed && ended)
            break;
    }
    close(ends[0]);
    if (start.pidfd >= 0)
        close(start.pidfd);
    run.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - began).count();

    run.started = true;
    if (WIFEXITED(status))
        run.code = WEXITSTATUS(status);
    else if (WIFSIGNALED(status))
        run.code = 128 + WTERMSIG(status);
    // CTRL-C: satl stops a line with machine code 130, interrupted (machine_codes.hpp) -- which is
    // also what a SIGINT that killed it outright reads as.
    if (run.code == 130)
        run.interrupted = true;
    run.err = read_all(errors_path);
    if (size_of(log_path) > logged_before)
        run.logged = read_all(log_path, logged_before);
    return run;
}

} // namespace satellite004
