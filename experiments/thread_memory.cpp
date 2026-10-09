#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>
#include <pthread.h>
static std::mutex gate; static std::condition_variable opened; static bool go = false;
static void *wait_here(void *) { std::unique_lock<std::mutex> lock(gate); opened.wait(lock, [] { return go; }); return nullptr; }
static long meminfo(const char *key)
{
    std::FILE *f = std::fopen("/proc/meminfo", "r"); char line[256]; long v = 0; size_t n = std::strlen(key);
    while (std::fgets(line, sizeof line, f)) if (std::strncmp(line, key, n) == 0) v = std::atol(line + n);
    std::fclose(f); return v;
}
static long self_rss() { std::FILE *f = std::fopen("/proc/self/status", "r"); char line[256]; long v = 0;
    while (std::fgets(line, sizeof line, f)) if (std::strncmp(line, "VmRSS:", 6) == 0) v = std::atol(line + 6); std::fclose(f); return v; }
int main(int, char **argv)
{
    const long n = std::strtol(argv[1], nullptr, 10);
    const long stack0 = meminfo("KernelStack:"), slab0 = meminfo("Slab:"), tables0 = meminfo("PageTables:"), rss0 = self_rss();
    pthread_attr_t a; pthread_attr_init(&a); pthread_attr_setstacksize(&a, 64 * 1024);
    std::vector<pthread_t> t(n);
    for (long i = 0; i < n; i++) if (pthread_create(&t[i], &a, wait_here, nullptr)) { std::printf("stopped at %ld\n", i); return 1; }
    const long stack = meminfo("KernelStack:") - stack0, slab = meminfo("Slab:") - slab0, tables = meminfo("PageTables:") - tables0, rss = self_rss() - rss0;
    { std::lock_guard<std::mutex> l(gate); go = true; } opened.notify_all();
    for (pthread_t &x : t) pthread_join(x, nullptr);
    const double each = double(stack + slab + tables + rss) / n;
    std::printf("%ld threads alive -- per thread: program %.1f KB, kernel stack %.1f KB, kernel slab %.1f KB, page tables %.1f KB  =  %.1f KB\n",
                n, double(rss) / n, double(stack) / n, double(slab) / n, double(tables) / n, each);
    std::printf("1,000,000 threads at that rate: %.1f GB   (MemAvailable now: %.1f GB)\n", each * 1e6 / 1048576, meminfo("MemAvailable:") / 1048576.0);
}
