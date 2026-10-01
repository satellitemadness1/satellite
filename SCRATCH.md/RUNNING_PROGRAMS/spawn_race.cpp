// SCRATCH.md/RUNNING_PROGRAMS/spawn_race.cpp -- 2026-10-01: three ways to start a program, raced from a
// process shaped like satl (1,024 threads, 2 GiB touched, its stack widened as widen_the_stack() does),
// and what stack limit each child is handed. The reason satellite_variable_program/program_spawn.cpp is
// clone(CLONE_VM | CLONE_VFORK) with one step posix_spawn has no room for -- machine/stack_share.hpp's
// rule that a child gets back the stack satl was given.
//
// Build: clang++ -std=c++20 -O2 -pthread SCRATCH.md/RUNNING_PROGRAMS/spawn_race.cpp -o /tmp/spawn_race
// Run:   /tmp/spawn_race 2000
//
// On this machine (2026-10-01): posix_spawn 565 us a run, child stack 1,986,560 KiB; fork + the hook
// 8,481 us; clone + the hook 594 us, child stack 8,192 KiB.
#include <fcntl.h>
#include <sched.h>
#include <signal.h>
#include <spawn.h>
#include <sys/mman.h>
#include <sys/resource.h>
#include <sys/wait.h>
#include <unistd.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <thread>
#include <vector>
#include <atomic>
extern char **environ;
static rlimit given;                       // what the shell gave us, as satl keeps it
struct Plan { const char *path; char *const *argv; int out; volatile int error; };
static int child(void *p) {
    Plan *plan = static_cast<Plan *>(p);
    struct sigaction dfl{}; dfl.sa_handler = SIG_DFL;
    for (int s = 1; s < NSIG; ++s) { struct sigaction now{}; if (s != SIGKILL && s != SIGSTOP && sigaction(s, nullptr, &now) == 0 && now.sa_handler != SIG_DFL) sigaction(s, &dfl, nullptr); }
    setrlimit(RLIMIT_STACK, &given);
    if (plan->out >= 0) dup2(plan->out, 1);
    sigset_t none; sigemptyset(&none); sigprocmask(SIG_SETMASK, &none, nullptr);
    execve(plan->path, plan->argv, environ);
    plan->error = errno; _exit(127);
}
static pid_t clone_spawn(const char *path, char *const *argv, int out) {
    Plan plan{path, argv, out, 0};
    sigset_t all, old; sigfillset(&all); pthread_sigmask(SIG_BLOCK, &all, &old);
    const size_t size = 64 * 1024;
    void *stack = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_STACK, -1, 0);
    pid_t pid = clone(child, static_cast<char *>(stack) + size, CLONE_VM | CLONE_VFORK | SIGCHLD, &plan);
    munmap(stack, size);
    pthread_sigmask(SIG_SETMASK, &old, nullptr);
    if (pid > 0 && plan.error != 0) { int st; waitpid(pid, &st, 0); errno = plan.error; return -1; }
    return pid;
}
static pid_t fork_spawn(const char *path, char *const *argv) {
    pid_t pid = fork();
    if (pid == 0) { setrlimit(RLIMIT_STACK, &given); execve(path, argv, environ); _exit(127); }
    return pid;
}
static double us(std::chrono::steady_clock::time_point a, std::chrono::steady_clock::time_point b, int n) { return std::chrono::duration<double, std::micro>(b - a).count() / n; }
int main(int argc, char **argv) {
    const int n = argc > 1 ? atoi(argv[1]) : 2000;
    getrlimit(RLIMIT_STACK, &given);
    rlimit wide = given; wide.rlim_cur = 1940ULL << 20; setrlimit(RLIMIT_STACK, &wide);   // satl's widen_the_stack()
    signal(SIGPIPE, SIG_IGN);
    std::atomic<bool> stop{false}; std::vector<std::thread> pool;                          // satl's 1,024 start-up threads
    for (int i = 0; i < 1024; ++i) pool.emplace_back([&stop] { while (!stop.load()) std::this_thread::sleep_for(std::chrono::milliseconds(200)); });
    std::vector<char> held(2ULL << 30); memset(held.data(), 1, held.size());              // 2 GiB touched
    char *t[] = {const_cast<char *>("true"), nullptr};
    using clk = std::chrono::steady_clock; int st;
    auto a = clk::now();
    for (int i = 0; i < n; ++i) { pid_t p; posix_spawn(&p, "/usr/bin/true", nullptr, nullptr, t, environ); waitpid(p, &st, 0); }
    auto b = clk::now();
    for (int i = 0; i < n; ++i) { pid_t p = fork_spawn("/usr/bin/true", t); waitpid(p, &st, 0); }
    auto c = clk::now();
    for (int i = 0; i < n; ++i) { pid_t p = clone_spawn("/usr/bin/true", t, -1); waitpid(p, &st, 0); }
    auto d = clk::now();
    printf("per run, %d runs each, 1024 threads + 2 GiB held:\n  posix_spawn       %6.0f us\n  fork + hook       %6.0f us\n  clone VFORK+hook  %6.0f us\n", n, us(a,b,n), us(b,c,n), us(c,d,n));
    // does the child get the shell's stack back?
    for (int which = 0; which < 2; ++which) {
        int fds[2]; pipe2(fds, O_CLOEXEC);
        char *sh[] = {const_cast<char *>("sh"), const_cast<char *>("-c"), const_cast<char *>("ulimit -s; trap -p PIPE; echo pipe-default-if-blank"), nullptr};
        pid_t p;
        if (which == 0) { posix_spawn_file_actions_t fa; posix_spawn_file_actions_init(&fa); posix_spawn_file_actions_adddup2(&fa, fds[1], 1); posix_spawn(&p, "/usr/bin/sh", &fa, nullptr, sh, environ); }
        else p = clone_spawn("/usr/bin/sh", sh, fds[1]);
        close(fds[1]); char buf[256]{}; ssize_t got = read(fds[0], buf, sizeof buf - 1); (void)got; close(fds[0]); waitpid(p, &st, 0);
        for (char *q = buf; *q; ++q) if (*q == '\n') *q = ' ';
        printf("  %s child: stack KiB %s\n", which == 0 ? "posix_spawn" : "clone+hook ", buf);
    }
    char *nope[] = {const_cast<char *>("nope"), nullptr};
    pid_t bad = clone_spawn("/usr/bin/no-such-program", nope, -1);
    printf("  missing program: clone_spawn answered %d, errno %s\n", bad, strerror(errno));
    stop = true; for (auto &th : pool) th.join();
}
