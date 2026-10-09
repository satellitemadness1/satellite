// std::system against posix_spawnp, running the same program N times, 2026-09-26.
// Build: clang++ -std=c++20 -O2 system_vs_spawn.cpp -o system_vs_spawn
// Run:   ./system_vs_spawn 2000            (and with a 2nd argument: MiB to hold first, like a big satl)
#include <spawn.h>
#include <sys/wait.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
extern char **environ;
using clk = std::chrono::steady_clock;

static double per_run_us(clk::time_point a, clk::time_point b, int n)
{
    return std::chrono::duration<double, std::micro>(b - a).count() / n;
}

int main(int argc, char **argv)
{
    const int n = argc > 1 ? std::atoi(argv[1]) : 2000;
    std::vector<char> held;                      // a process holding memory, the way satl does
    if (argc > 2) { held.resize(std::size_t(std::atoi(argv[2])) << 20); std::memset(held.data(), 1, held.size()); }

    auto a = clk::now();
    for (int i = 0; i < n; ++i) if (std::system("/bin/true") != 0) return 1;
    auto b = clk::now();
    char *args[] = {const_cast<char *>("true"), nullptr};
    for (int i = 0; i < n; ++i) {
        pid_t pid; int status = 0;
        if (posix_spawnp(&pid, "true", nullptr, nullptr, args, environ) != 0) return 2;
        waitpid(pid, &status, 0);
    }
    auto c = clk::now();
    std::printf("holding %s MiB: std::system %.0f us a run, posix_spawnp %.0f us a run (x%.1f)\n",
                argc > 2 ? argv[2] : "0", per_run_us(a, b, n), per_run_us(b, c, n),
                per_run_us(a, b, n) / per_run_us(b, c, n));
}
