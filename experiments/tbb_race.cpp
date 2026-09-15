// One command = a call the compiler cannot paste in, ~28 ns of math, like a satellite library call.
// Races: one thread, our 256-thread pool (192 runners, one recall per batch), and TBB parallel_for.
#include <oneapi/tbb/blocked_range.h>
#include <oneapi/tbb/parallel_for.h>
#include <oneapi/tbb/task_arena.h>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
using hrc = std::chrono::high_resolution_clock;
static double since(hrc::time_point t) { return std::chrono::duration<double>(hrc::now() - t).count(); }

[[gnu::noinline]] unsigned long long command(unsigned long long i)
{
    unsigned long long z = i, total = 0;
    for (int s = 0; s < 20; s++) { z += 0x9e3779b97f4a7c15ULL; z ^= z >> 31; z *= 0xbf58476d1ce4e5b9ULL; total += z; }
    return total;
}

class Pool {
public:
    explicit Pool(unsigned n) { for (unsigned i = 0; i < n; i++) w_.emplace_back([this] { loop(); }); }
    ~Pool() { { std::lock_guard<std::mutex> l(m_); stop_ = true; } wake_.notify_all(); for (auto &t : w_) t.join(); }
    void submit(std::function<void()> j) { { std::lock_guard<std::mutex> l(m_); q_.push_back(std::move(j)); pending_++; } wake_.notify_one(); }
    void wait() { std::unique_lock<std::mutex> l(m_); done_.wait(l, [this] { return pending_ == 0; }); }
private:
    void loop() { for (;;) { std::function<void()> j;
        { std::unique_lock<std::mutex> l(m_); wake_.wait(l, [this] { return stop_ || !q_.empty(); }); if (stop_ && q_.empty()) return; j = std::move(q_.front()); q_.pop_front(); }
        j(); { std::lock_guard<std::mutex> l(m_); if (--pending_ == 0) done_.notify_all(); } } }
    std::vector<std::thread> w_; std::deque<std::function<void()>> q_; std::mutex m_; std::condition_variable wake_, done_; long pending_ = 0; bool stop_ = false;
};

static std::vector<unsigned long long> out;

static void one_thread(unsigned long long n) { for (unsigned long long i = 0; i < n; i++) out[i] = command(i); }
static void our_pool(Pool &pool, unsigned long long n, unsigned batches)
{
    for (unsigned k = 0; k < batches; k++)
        pool.submit([k, n, batches] { unsigned long long a = n / batches * k, b = k + 1 == batches ? n : n / batches * (k + 1);
                                      for (unsigned long long i = a; i < b; i++) out[i] = command(i); });
    pool.wait();
}
static void tbb_for(unsigned long long n)
{
    oneapi::tbb::parallel_for(oneapi::tbb::blocked_range<unsigned long long>(0, n), [](const oneapi::tbb::blocked_range<unsigned long long> &r) {
        for (unsigned long long i = r.begin(); i != r.end(); i++) out[i] = command(i); });
}

int main()
{
    Pool pool(192);
    std::printf("TBB will use %d threads\n\n", oneapi::tbb::info::default_concurrency());

    const unsigned long long big = 100000000;
    out.assign(big, 0);
    auto t = hrc::now(); one_thread(big); double a = since(t);
    t = hrc::now(); our_pool(pool, big, 192); double b = since(t);
    t = hrc::now(); tbb_for(big); double c = since(t);
    std::printf("ONE HUGE LOOP, 100,000,000 commands\n");
    std::printf("  one thread:                 %6.3f s\n  our pool, 192 batches:      %6.3f s  (x%.1f)\n  TBB parallel_for:           %6.3f s  (x%.1f)\n\n", a, b, a / b, c, a / c);

    const unsigned long long small = 10000, loops = 10000;
    out.assign(small, 0);
    t = hrc::now(); for (unsigned long long l = 0; l < loops; l++) one_thread(small); a = since(t);
    t = hrc::now(); for (unsigned long long l = 0; l < loops; l++) our_pool(pool, small, 192); b = since(t);
    t = hrc::now(); for (unsigned long long l = 0; l < loops; l++) tbb_for(small); c = since(t);
    std::printf("THE SAME LOOP RUN 10,000 TIMES, 10,000 commands each (100,000,000 in total)\n");
    std::printf("  one thread:                 %6.3f s\n  our pool, 192 batches:      %6.3f s  (x%.2f)\n  TBB parallel_for:           %6.3f s  (x%.1f)\n", a, b, a / b, c, a / c);
}
