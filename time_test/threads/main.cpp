// time_test/threads/main.cpp -- what a thread costs, and when splitting a big number across
// threads pays.
//
// The author, 2026-09-23: "Add can be split for large numbers ... divide the numbers at 10
// digits per thread". This measures the three things that decide it: adding two numbers on
// one thread (100 digits to 100 million), handing work to another thread and back, and a
// real threaded add -- each thread adds its piece, then one pass carries between pieces --
// checked against the one-thread answer. satl's limbs: 64-bit, least significant first,
// carried through unsigned __int128.
//
// BUILD:  clang++ -std=c++20 -O2 -pthread main.cpp -o threads     (or g++)
// RUN:    ./threads
//
// At most 12 threads for a few milliseconds -- not a stress test. About 400 MB of memory
// at the 100-million-digit size.
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <mutex>
#include <random>
#include <thread>
#include <vector>
using limb = unsigned long long; using clk = std::chrono::steady_clock;

// one run of the add: out = a + b over [from, to), carry in, carry out
static limb add_run(const limb *a, const limb *b, limb *out, size_t from, size_t to, limb carry) {
    for (size_t i = from; i < to; i++) { unsigned __int128 s = (unsigned __int128)a[i] + b[i] + carry; out[i] = (limb)s; carry = (limb)(s >> 64); }
    return carry;
}
static double best_ns(auto f, int reps) { double best = 1e30; for (int r = 0; r < reps; r++) { auto t = clk::now(); f(); best = std::min(best, (double)std::chrono::duration_cast<std::chrono::nanoseconds>(clk::now() - t).count()); } return best; }

// THE THREADED ADD: each thread adds its piece with carry-in 0 and reports its carry out;
// then one pass walks the pieces in order and pushes each carry into the next piece
// (it stops at the first limb that does not overflow -- almost always the first).
static void add_threaded(const limb *a, const limb *b, limb *out, size_t n, int threads) {
    std::vector<limb> carry(threads, 0); std::vector<std::thread> pool; size_t piece = (n + threads - 1) / threads;
    for (int t = 0; t < threads; t++) pool.emplace_back([&, t] { size_t f = t * piece, e = std::min(n, f + piece); if (f < e) carry[t] = add_run(a, b, out, f, e, 0); });
    for (auto &th : pool) th.join();
    limb c = 0;
    for (int t = 0; t < threads; t++) {
        size_t f = t * piece, e = std::min(n, f + piece);
        for (size_t i = f; c && i < e; i++) { out[i] += 1; c = (out[i] == 0); }   // push the carry in
        c += carry[t];                                                            // this piece's own carry out
    }
}

int main() {
    std::mt19937_64 rng(7);
    std::printf("ONE THREAD, adding two numbers:\n");
    for (size_t digits : {100ul, 10000ul, 1000000ul, 100000000ul}) {
        size_t n = digits * 3.3219280948873623 / 64 + 1; std::vector<limb> a(n), b(n), o(n); for (auto &x : a) x = rng(); for (auto &x : b) x = rng();
        int reps = digits > 1e7 ? 5 : 2000;
        double ns = best_ns([&] { add_run(a.data(), b.data(), o.data(), 0, n, 0); asm volatile("" ::"r"(o.data()) : "memory"); }, reps);
        std::printf("  %11zu digits (%9zu limbs): %12.0f ns   (%.2f ns a limb)\n", digits, n, ns, ns / n);
    }
    // THE HANDOFF: wake a parked thread, it does nothing, wake us back. Condition variable, as a parked pool waits.
    { std::mutex m; std::condition_variable cv; int turn = 0; bool stop = false; const int trips = 20000;
      std::thread w([&] { std::unique_lock l(m); while (true) { cv.wait(l, [&] { return turn == 1 || stop; }); if (stop) return; turn = 0; cv.notify_one(); } });
      auto t = clk::now();
      for (int i = 0; i < trips; i++) { std::unique_lock l(m); turn = 1; cv.notify_one(); cv.wait(l, [&] { return turn == 0; }); }
      double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clk::now() - t).count() / (double)trips;
      { std::lock_guard l(m); stop = true; } cv.notify_one(); w.join();
      std::printf("HANDOFF to a parked thread and back (condition variable): %.0f ns\n", ns); }
    { std::atomic<int> turn{0}; const int trips = 200000; std::atomic<bool> stop{false};
      std::thread w([&] { while (!stop.load(std::memory_order_relaxed)) { if (turn.load(std::memory_order_acquire) == 1) turn.store(0, std::memory_order_release); } });
      auto t = clk::now(); for (int i = 0; i < trips; i++) { turn.store(1, std::memory_order_release); while (turn.load(std::memory_order_acquire) != 0) {} }
      double ns = std::chrono::duration_cast<std::chrono::nanoseconds>(clk::now() - t).count() / (double)trips; stop = true; w.join();
      std::printf("HANDOFF to a thread that never sleeps (spinning, burns a core): %.0f ns\n", ns); }
    // THE THREADED ADD, checked against one thread, threads started per add (a pool would save that part)
    for (size_t digits : {1000000ul, 10000000ul, 100000000ul}) {
        size_t n = digits * 3.3219280948873623 / 64 + 1; std::vector<limb> a(n), b(n), one(n), many(n);
        for (auto &x : a) x = rng();
        for (auto &x : b) x = rng();
        add_run(a.data(), b.data(), one.data(), 0, n, 0);
        std::printf("THREADED ADD, %zu digits:", digits);
        for (int t : {1, 2, 4, 8, 12}) { double ns = best_ns([&] { add_threaded(a.data(), b.data(), many.data(), n, t); }, 5);
            std::printf("  %d thr %.2f ms%s", t, ns / 1e6, many == one ? "" : " WRONG"); }
        std::printf("\n");
    }
}
