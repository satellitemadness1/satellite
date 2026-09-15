// N threads alive at the same moment, each doing 1,000 steps of math; every
// result is then checked against a single-thread recount.
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <mutex>
#include <vector>
#include <pthread.h>
#include <sched.h>

using hrc = std::chrono::high_resolution_clock;
static std::mutex gate;
static std::condition_variable opened;
static bool go = false;
static std::vector<unsigned long long> results;

static unsigned long long math(unsigned long long seed)
{
    unsigned long long z = seed, total = 0;
    for (int step = 0; step < 1000; step++) {
        z += 0x9e3779b97f4a7c15ULL;
        unsigned long long x = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        total += x ^ (x >> 31);
    }
    return total;
}

static void *worker(void *argument)
{
    const unsigned long long index = (unsigned long long)(intptr_t)argument;
    { std::unique_lock<std::mutex> lock(gate); opened.wait(lock, [] { return go; }); }
    results[index] = math(index);
    return nullptr;
}

static long status_value(const char *key)
{
    std::FILE *status = std::fopen("/proc/self/status", "r");
    char line[256]; long value = 0; size_t n = std::strlen(key);
    while (std::fgets(line, sizeof line, status)) if (std::strncmp(line, key, n) == 0) value = std::atol(line + n);
    std::fclose(status);
    return value;
}

int main(int argc, char **argv)
{
    const long wanted = std::strtol(argv[1], nullptr, 10);
    sched_param idle{}; sched_setscheduler(0, SCHED_IDLE, &idle);   // lowest priority: the desktop stays first
    results.assign(wanted, 0);

    pthread_attr_t attributes;
    pthread_attr_init(&attributes);
    pthread_attr_setstacksize(&attributes, 64 * 1024);
    std::vector<pthread_t> threads;
    threads.reserve(wanted);

    auto start = hrc::now();
    int failure = 0;
    for (long i = 0; i < wanted; i++) {
        pthread_t thread;
        failure = pthread_create(&thread, &attributes, worker, (void *)(intptr_t)i);
        if (failure != 0) break;
        threads.push_back(thread);
    }
    double create_s = std::chrono::duration<double>(hrc::now() - start).count();
    long alive = status_value("Threads:") - 1, rss_kb = status_value("VmRSS:");

    auto released = hrc::now();
    { std::lock_guard<std::mutex> lock(gate); go = true; }
    opened.notify_all();
    for (pthread_t thread : threads) pthread_join(thread, nullptr);
    double work_s = std::chrono::duration<double>(hrc::now() - released).count();

    unsigned long long wrong = 0;
    for (size_t i = 0; i < threads.size(); i++) if (results[i] != math(i)) wrong++;

    std::printf("%8ld asked | %8zu started%s | %8ld alive at once | %7.1f MB | start %5.2f s | math + join %5.2f s | wrong results: %llu\n",
                wanted, threads.size(), failure ? " (EAGAIN: out of slots)" : "", alive, rss_kb / 1024.0, create_s, work_s, wrong);
    return failure != 0 || wrong != 0;
}
