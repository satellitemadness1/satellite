// A pool of N threads started once, then "recalled" with work whenever it is needed.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
#include <sys/resource.h>

using hrc = std::chrono::high_resolution_clock;

class Pool {
public:
    explicit Pool(unsigned threads) { for (unsigned i = 0; i < threads; i++) workers_.emplace_back([this] { loop(); }); }
    ~Pool() { { std::lock_guard<std::mutex> l(m_); stop_ = true; } wake_.notify_all(); for (auto &w : workers_) w.join(); }
    void submit(std::function<void()> job) { { std::lock_guard<std::mutex> l(m_); jobs_.push_back(std::move(job)); pending_++; } wake_.notify_one(); }
    void wait() { std::unique_lock<std::mutex> l(m_); done_.wait(l, [this] { return pending_ == 0; }); }
private:
    void loop() {
        for (;;) {
            std::function<void()> job;
            { std::unique_lock<std::mutex> l(m_); wake_.wait(l, [this] { return stop_ || !jobs_.empty(); });
              if (stop_ && jobs_.empty()) return; job = std::move(jobs_.front()); jobs_.pop_front(); }
            job();
            { std::lock_guard<std::mutex> l(m_); if (--pending_ == 0) done_.notify_all(); }
        }
    }
    std::vector<std::thread> workers_; std::deque<std::function<void()>> jobs_;
    std::mutex m_; std::condition_variable wake_, done_; long pending_ = 0; bool stop_ = false;
};

static unsigned long long math(unsigned long long seed, int steps)
{
    unsigned long long z = seed, total = 0;
    for (int s = 0; s < steps; s++) { z += 0x9e3779b97f4a7c15ULL; unsigned long long x = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL; total += x ^ (x >> 31); }
    return total;
}
static double cpu_seconds() { rusage u; getrusage(RUSAGE_SELF, &u); return u.ru_utime.tv_sec + u.ru_stime.tv_sec + (u.ru_utime.tv_usec + u.ru_stime.tv_usec) / 1e6; }

int main()
{
    // 1. starting the pool
    auto t = hrc::now();
    Pool *pool = new Pool(256);
    std::printf("start 256 threads:                     %.2f ms\n", std::chrono::duration<double, std::milli>(hrc::now() - t).count());

    // 2. one recall: hand a tiny job to the pool and wait for it to finish
    const int recalls = 100000; std::atomic<unsigned long long> sink{0};
    t = hrc::now();
    for (int i = 0; i < recalls; i++) { pool->submit([&sink, i] { sink += i; }); pool->wait(); }
    std::printf("one recall (hand over + wait):         %.1f ns   vs the job done inline: ~1 ns\n",
                std::chrono::duration<double, std::nano>(hrc::now() - t).count() / recalls);

    // 3. real work: 256 big jobs, 20,000,000 math steps each, with 256 threads vs 24 vs 1
    for (unsigned threads : {1u, 24u, 256u}) {
        Pool *p = threads == 256 ? pool : new Pool(threads);
        std::vector<unsigned long long> answers(256);
        double cpu0 = cpu_seconds(); t = hrc::now();
        for (int j = 0; j < 256; j++) p->submit([&answers, j] { answers[j] = math(j, 20000000); });
        p->wait();
        double wall = std::chrono::duration<double>(hrc::now() - t).count(), cpu = cpu_seconds() - cpu0;
        unsigned long long check = 0; for (auto a : answers) check ^= a;
        std::printf("%3u threads, 256 jobs x 20M steps:     %6.2f s wall, CPU busy %5.0f%% of one thread (%4.1f of 24 hardware threads)  check %llx\n",
                    threads, wall, 100 * cpu / wall, cpu / wall, check);
        if (p != pool) delete p;
    }
    delete pool;
}
