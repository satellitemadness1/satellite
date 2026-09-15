// One "command" = one call into a function the compiler cannot paste in, doing a little math,
// like a satellite-numbers library call. Three ways to use a 256-thread pool for 10,000,000 of them.
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>
using hrc = std::chrono::high_resolution_clock;

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

int main()
{
    const unsigned long long commands = 10000000;
    Pool pool(256);
    auto seconds = [](hrc::time_point t) { return std::chrono::duration<double>(hrc::now() - t).count(); };

    auto t = hrc::now(); unsigned long long a = 0;
    for (unsigned long long i = 0; i < commands; i++) a += command(i);
    double inline_s = seconds(t);
    std::printf("A. every command on the main thread:              %7.3f s   (%.1f ns per command)\n", inline_s, inline_s * 1e9 / commands);

    const unsigned long long sample = 200000; unsigned long long b = 0; std::mutex bm;
    t = hrc::now();
    for (unsigned long long i = 0; i < sample; i++) { pool.submit([&, i] { std::lock_guard<std::mutex> l(bm); b += command(i); }); pool.wait(); }
    double per = seconds(t) / sample;
    std::printf("B. recall a thread for EACH command:              %7.1f s   (%.0f ns per command, measured on %llu, x%.0f slower)\n",
                per * commands, per * 1e9, sample, per * commands / inline_s);

    for (unsigned batches : {24u, 256u}) {
        std::vector<unsigned long long> part(batches, 0);
        t = hrc::now();
        for (unsigned k = 0; k < batches; k++)
            pool.submit([&part, k, batches, commands] {
                unsigned long long from = commands / batches * k, to = k + 1 == batches ? commands : commands / batches * (k + 1), s = 0;
                for (unsigned long long i = from; i < to; i++) s += command(i);
                part[k] = s; });
        pool.wait();
        double s = seconds(t); unsigned long long c = 0; for (auto p : part) c += p;
        std::printf("C. %3u batches, one recall per batch:             %7.3f s   (x%.1f faster than A, same answer: %s)\n",
                    batches, s, inline_s / s, c == a ? "yes" : "NO");
    }
}
