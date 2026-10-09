// satellite/satellite_variable_program/program_spawn.cpp -- the header says what this is, and
// what it was measured against.

#include "program_spawn.hpp"

#include "../machine/own_environment.hpp"
#include "../machine/stack_share.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <mutex>
#include <sstream>
#include <sys/resource.h>
#include <unordered_map>
#include <sched.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>

extern char **environ;

namespace satellite004 {
namespace {

// WHAT THE CHILD IS HANDED. It shares satl's memory until it execs (CLONE_VM), so it reads this
// where it lies, and writes `error` for satl to read once clone has returned.
struct ChildPlan {
    const char *path;
    char *const *arguments;
    char *const *environment;   // the machine's, not what satl set for itself (own_environment.hpp)
    int input;
    int out;
    int err;
    const char *folder;         // nullptr: it starts where satl is
    volatile int error;
    volatile int in_folder;     // 1 when it was entering `folder` that failed, not the exec
};

// THE CHILD, between clone and exec. Only calls that are safe there: no allocation, no lock.
int the_child(void *given)
{
    ChildPlan *plan = static_cast<ChildPlan *>(given);
    // A SIGNAL SATL CATCHES IS THE PROGRAM'S TO TAKE AS ANY PROGRAM WOULD: back to its default
    // before anything is unblocked, so satl's handler never runs in here. SIGPIPE too: satl
    // ignores it for itself (structured-library.cpp), and a program handed that ignore would
    // say "Broken pipe" where it would have ended quietly.
    struct sigaction plain {};
    plain.sa_handler = SIG_DFL;
    sigemptyset(&plain.sa_mask);
    for (int signal = 1; signal < NSIG; ++signal) {
        if (signal == SIGKILL || signal == SIGSTOP)
            continue;
        struct sigaction now {};
        if (sigaction(signal, nullptr, &now) != 0)
            continue;
        if ((now.sa_handler != SIG_DFL && now.sa_handler != SIG_IGN) || signal == SIGPIPE)
            sigaction(signal, &plain, nullptr);
    }
    hand_a_child_the_stack_satl_was_given();   // machine/stack_share.hpp's rule
    if (dup2(plan->input, 0) < 0 || dup2(plan->out, 1) < 0 || dup2(plan->err, 2) < 0) {
        plan->error = errno;
        syscall(SYS_exit_group, 127);
    }
    // ITS FOLDER BY THE RAW CALL: syscall is bound already (below), and chdir would be a first
    // call through the resolver in here.
    if (plan->folder != nullptr && syscall(SYS_chdir, plan->folder) != 0) {
        plan->error = errno;
        plan->in_folder = 1;
        syscall(SYS_exit_group, 127);
    }
    // EVERY OTHER FILE SATL HAS OPEN STAYS SATL'S: a window's pty, a file a program opened, a
    // pipe of another program's -- one left open in here would hold it open for as long as
    // this program runs.
#ifdef SYS_close_range
    syscall(SYS_close_range, 3U, ~0U, 0U);
#endif
    sigset_t none;
    sigemptyset(&none);
    sigprocmask(SIG_SETMASK, &none, nullptr);
    execve(plan->path, plan->arguments, plan->environment);
    plan->error = errno;
    syscall(SYS_exit_group, 127);
    return 127;
}

// THE CHILD'S CALLS, MADE ONCE HERE FIRST. satl is linked without -z now, so the first call of a
// libc function goes through the dynamic linker's resolver -- and glibc's own posix_spawn never
// lets its vfork child do that (the review, 2026-10-01). Each call below is harmless, and after it
// every call the child makes is already bound: sigaction, sigemptyset, sigprocmask, setrlimit
// (hand_a_child_the_stack_satl_was_given), dup2, syscall (close_range, exit_group), execve, errno.
void bind_the_childs_calls_once()
{
    static std::once_flag once;
    std::call_once(once, [] {
        struct sigaction now {};
        sigaction(SIGUSR1, nullptr, &now);
        sigset_t set;
        sigemptyset(&set);
        sigprocmask(SIG_BLOCK, nullptr, &set);
        rlimit stack {};
        if (getrlimit(RLIMIT_STACK, &stack) == 0)
            setrlimit(RLIMIT_STACK, &stack);       // the same limit back: changes nothing
        const int fd = open("/dev/null", O_RDONLY | O_CLOEXEC);
        if (fd >= 0) {
            dup2(fd, fd);                          // a descriptor onto itself: changes nothing
            close(fd);
        }
        syscall(SYS_getpid);
        char *none[] = {nullptr};
        execve("", none, environ);                 // ENOENT: binds execve, starts nothing
        errno = 0;
    });
}

// A NAME WITH NO SLASH IS LOOKED UP ON THE PATH, here, before anything starts -- so "there is no
// program named that" is known without a child. A path is taken as it is written.
std::string found_on_the_path(const std::string &name, int &error)
{
    const char *path = std::getenv("PATH");
    const std::string folders = path != nullptr ? path : "/usr/local/bin:/usr/bin:/bin";
    error = ENOENT;
    std::size_t from = 0;
    for (;;) {
        const std::size_t colon = folders.find(':', from);
        const std::string folder = folders.substr(from, colon == std::string::npos ? std::string::npos : colon - from);
        const std::string candidate = (folder.empty() ? std::string(".") : folder) + "/" + name;
        struct stat what {};
        if (stat(candidate.c_str(), &what) == 0 && S_ISREG(what.st_mode)) {
            if (access(candidate.c_str(), X_OK) == 0) {
                error = 0;
                return candidate;
            }
            error = EACCES;   // one that may not be run; keep looking for one that may
        }
        if (colon == std::string::npos)
            return std::string();
        from = colon + 1;
    }
}

void say_why(ProgramStart &start, const std::string &program, bool looked_up)
{
    start.code = start.error == ENOENT ? 127 : 126;
    if (start.error == ENOENT)
        start.why = looked_up ? "there is no program named " + program + " on the PATH"
                              : "there is no program at " + program;
    else if (start.error == EACCES)
        start.why = program + " may not be run: it is not marked as a program, or a folder on the way may not be "
                              "entered";
    else if (start.error == ENOEXEC)
        start.why = program + " is not a program this machine can run";
    else
        start.why = program + " could not be started: " + std::strerror(start.error);
}

} // namespace

ProgramStart start_a_program(const std::vector<std::string> &words, int out, int in, int err, const ProgramPlace &place)
{
    bind_the_childs_calls_once();
    ProgramStart start;
    if (words.empty() || words.front().empty()) {
        start.error = ENOENT;
        start.code = 127;
        start.why = "it holds no program to start";
        return start;
    }
    const std::string &program = words.front();
    const bool looked_up = program.find('/') == std::string::npos;
    std::string path = program;
    if (looked_up) {
        path = found_on_the_path(program, start.error);
        if (path.empty()) {
            say_why(start, program, true);
            return start;
        }
    }

    std::vector<char *> arguments;
    arguments.reserve(words.size() + 1);
    for (const std::string &word : words)
        arguments.push_back(const_cast<char *>(word.c_str()));
    arguments.push_back(nullptr);

    // EVERY DESCRIPTOR HANDED IN IS 3 OR ABOVE. A satl started with its input or output closed
    // gets 0, 1 or 2 back from open() and pipe(), and the child's dup2s onto 0, 1 and 2 would
    // then write over one of them before using it.
    const auto above_two = [](int fd) {
        if (fd < 0 || fd > 2)
            return fd;
        const int moved = fcntl(fd, F_DUPFD_CLOEXEC, 3);
        close(fd);
        return moved;
    };
    const int nothing = above_two(open("/dev/null", O_RDWR | O_CLOEXEC));
    if (nothing < 0) {
        start.error = errno;
        say_why(start, program, looked_up);
        return start;
    }
    const int stream = out > 2 ? out : out >= 0 ? fcntl(out, F_DUPFD_CLOEXEC, 3) : nothing;
    const int input = in > 2 ? in : in >= 0 ? fcntl(in, F_DUPFD_CLOEXEC, 3) : nothing;
    const int errors = err == kErrorsWithOutput ? stream : err > 2 ? err : err >= 0 ? fcntl(err, F_DUPFD_CLOEXEC, 3) : nothing;
    std::vector<std::string> environment_held;
    std::vector<char *> environment_pointers;
    char *const *environment = environment_for_a_program(environment_held, environment_pointers);
    // THE PLACE'S NAMES IN PLACE OF satl's: every entry but those, then those.
    std::vector<char *> placed;
    if (!place.environment.empty()) {
        const auto replaced = [&place](const char *entry) {
            for (const std::string &change : place.environment) {
                const std::size_t name = change.find('=');
                if (name != std::string::npos && std::strncmp(entry, change.c_str(), name + 1) == 0)
                    return true;
            }
            return false;
        };
        for (char *const *entry = environment; *entry != nullptr; ++entry)
            if (!replaced(*entry))
                placed.push_back(*entry);
        for (const std::string &change : place.environment)
            placed.push_back(const_cast<char *>(change.c_str()));
        placed.push_back(nullptr);
        environment = placed.data();
    }
    ChildPlan plan{path.c_str(), arguments.data(), environment, input, stream, errors,
                   place.folder.empty() ? nullptr : place.folder.c_str(), 0, 0};

    // EVERY SIGNAL BLOCKED FOR THE CLONE, as glibc's posix_spawn does: the child shares this
    // memory until it execs, and a handler run in it would run on satl's data.
    sigset_t all, before;
    sigfillset(&all);
    pthread_sigmask(SIG_BLOCK, &all, &before);
    constexpr std::size_t kChildStack = 128 * 1024;
    void *stack = mmap(nullptr, kChildStack, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    pid_t pid = -1;
    int clone_error = 0;
    if (stack == MAP_FAILED) {
        clone_error = errno;
    } else {
        pid = clone(the_child, static_cast<char *>(stack) + kChildStack, CLONE_VM | CLONE_VFORK | SIGCHLD, &plan);
        if (pid < 0)
            clone_error = errno;
        munmap(stack, kChildStack);   // CLONE_VFORK: the child has exec'd or ended by now
    }
    pthread_sigmask(SIG_SETMASK, &before, nullptr);
    close(nothing);
    if (stream != out && stream != nothing)
        close(stream);   // the copies made above; the caller still holds `out` and `in` itself
    if (input != in && input != nothing)
        close(input);
    if (errors != err && errors != nothing && errors != stream)
        close(errors);

    if (pid < 0) {
        start.error = clone_error;
        say_why(start, program, looked_up);
        return start;
    }
    if (plan.error != 0) {
        int status = 0;
        waitpid(pid, &status, 0);   // the child that could not exec, ended with 127
        start.error = plan.error;
        say_why(start, program, looked_up);
        if (plan.in_folder != 0)
            start.why = program + " could not be started in " + place.folder + ": " + std::strerror(plan.error);
        return start;
    }
    start.pid = pid;
#ifdef SYS_pidfd_open
    start.pidfd = static_cast<int>(syscall(SYS_pidfd_open, pid, 0));
#endif
    return start;
}

namespace {

// ONE LINE OF /proc/<pid>/stat: its parent, its state and when it started. The name in brackets
// may hold spaces and brackets of its own, so the fields are read after the LAST ')'.
bool read_stat(pid_t pid, pid_t &parent, char &state, unsigned long long &started)
{
    std::ifstream file("/proc/" + std::to_string(pid) + "/stat");
    std::string line;
    if (!std::getline(file, line))
        return false;
    const std::size_t close = line.rfind(')');
    if (close == std::string::npos)
        return false;
    std::istringstream fields(line.substr(close + 1));
    std::string field;
    long long parent_read = 0;
    fields >> state >> parent_read;
    for (int skip = 0; skip < 17 && fields >> field; ++skip) {
    }
    if (!(fields >> started))
        return false;
    parent = static_cast<pid_t>(parent_read);
    return true;
}

} // namespace

std::vector<ProcessSeen> processes_under(pid_t root)
{
    std::unordered_map<pid_t, std::vector<ProcessSeen>> children;
    if (DIR *proc = opendir("/proc")) {
        while (const dirent *entry = readdir(proc)) {
            char *end = nullptr;
            const long pid = std::strtol(entry->d_name, &end, 10);
            if (end == entry->d_name || *end != '\0' || pid <= 0)
                continue;
            pid_t parent = 0;
            char state = 0;
            unsigned long long started = 0;
            if (read_stat(static_cast<pid_t>(pid), parent, state, started) && state != 'Z')
                children[parent].push_back({static_cast<pid_t>(pid), started});
        }
        closedir(proc);
    }
    std::vector<ProcessSeen> under;
    std::vector<pid_t> next{root};
    while (!next.empty()) {
        const pid_t parent = next.back();
        next.pop_back();
        const auto found = children.find(parent);
        if (found == children.end())
            continue;
        for (const ProcessSeen &child : found->second) {
            under.push_back(child);
            next.push_back(child.pid);
        }
    }
    return under;
}

bool still_there(const ProcessSeen &process)
{
    pid_t parent = 0;
    char state = 0;
    unsigned long long started = 0;
    return read_stat(process.pid, parent, state, started) && state != 'Z' && started == process.started;
}

void signal_if_still_there(const ProcessSeen &process, int signal)
{
    if (still_there(process))
        kill(process.pid, signal);
}

ProcessSeen seen_now(pid_t pid)
{
    pid_t parent = 0;
    char state = 0;
    unsigned long long started = 0;
    if (!read_stat(pid, parent, state, started) || state == 'Z')
        return ProcessSeen{};
    return ProcessSeen{pid, started};
}

void wait_until_stopped(const ProcessSeen &process, int most_ms)
{
    for (int waited = 0; waited < most_ms; ++waited) {
        pid_t parent = 0;
        char state = 0;
        unsigned long long started = 0;
        if (!read_stat(process.pid, parent, state, started) || started != process.started || state == 'Z' ||
            state == 'T' || state == 't')
            return;
        usleep(1000);
    }
}

} // namespace satellite004
