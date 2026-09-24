// time_test/locks/main.cpp -- how threads share one thing: a bool, two bools, a lock, and
// "doubling" (copy, change, swap in), each raced by four threads adding 1 to one counter.
//
// The author, 2026-09-23, designing satellite's locks: "we attach a bool to the special
// object, if it's true, then we cannot write to that name", then "let's double literally
// everything in the language", then "try it with two bools". Each idea is here as he
// said it, and beside it the version that is right. RIGHT or WRONG does not depend on
// the clock: four threads each add 1 a million times, so the answer is 4,000,000.
//
// BUILD:  clang++ -std=c++20 -O2 -pthread main.cpp -o locks     (or g++)
// RUN:    ./locks
//
// Four threads for about two seconds -- not a stress test.

#include <atomic>
#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <thread>
#include <vector>
using clk = std::chrono::steady_clock;
constexpr int THREADS = 4, EACH = 1000000;

// ONE COUNTER, FOUR THREADS: the first family.
template <class F> static void race(const char *name, F body, auto answer) {
    auto t = clk::now(); std::vector<std::thread> v;
    for (int i = 0; i < THREADS; i++) v.emplace_back(body);
    for (auto &x : v) x.join();
    double ms = std::chrono::duration<double, std::milli>(clk::now() - t).count();
    long got = answer();
    std::printf("%-62s %9ld of %9ld  %s  %7.1f ns a write\n", name, got, (long)THREADS * EACH,
                got == (long)THREADS * EACH ? "RIGHT" : "WRONG", ms * 1e6 / (THREADS * EACH));
}
// ANY NUMBER OF THREADS, EACH HANDED ITS NUMBER: the two-bool family.
template <class F> static double run(int threads, F body) {
    auto t = clk::now(); std::vector<std::thread> v;
    for (int i = 0; i < threads; i++) v.emplace_back(body, i);
    for (auto &x : v) x.join();
    return std::chrono::duration<double, std::milli>(clk::now() - t).count();
}
static void say(const char *name, long got, long want, double ms, long writes) {
    std::printf("%-62s %9ld of %9ld  %s  %7.1f ns a write\n", name, got, want, got == want ? "RIGHT" : "WRONG", ms * 1e6 / writes);
}

int main() {
    std::printf("ONE BOOL, A LOCK, AND DOUBLING -- 4 threads, one counter:\n");

    // 1. THE BOOL, CHECKED AND THEN SET: two separate steps. (atomic loads and stores so the
    //    C++ itself is well defined -- the race is in the logic, which is the point.)
    { std::atomic<bool> busy{false}; std::atomic<long> counter{0};
      race("1. a bool: wait while true, set true, write, set false", [&] {
          for (int i = 0; i < EACH; i++) {
              while (busy.load()) {}                 // step 1: is anyone writing?
              busy.store(true);                      // step 2: then it's mine -- but another thread saw false too
              counter.store(counter.load() + 1);     // the write (read, add, store)
              busy.store(false);
          } }, [&] { return counter.load(); }); }
    // 2. THE SAME BOOL, FLIPPED IN ONE INDIVISIBLE STEP: exchange answers the OLD value, so
    //    exactly one thread ever sees "it was false" -- this is a spinlock.
    { std::atomic<bool> busy{false}; long counter = 0;
      race("2. the same bool, flipped in one step (test-and-set)", [&] {
          for (int i = 0; i < EACH; i++) {
              while (busy.exchange(true, std::memory_order_acquire)) {}
              counter++;
              busy.store(false, std::memory_order_release);
          } }, [&] { return counter; }); }
    // 3. AN ORDINARY LOCK
    { std::mutex m; long counter = 0;
      race("3. a mutex", [&] { for (int i = 0; i < EACH; i++) { std::lock_guard g(m); counter++; } }, [&] { return counter; }); }
    // 4. "DOUBLE EVERYTHING": read it, make the new value on the side, and swap it in only if
    //    nobody changed it meanwhile; otherwise try again. Nobody ever waits on a lock.
    { std::atomic<long> counter{0}; std::atomic<long> retries{0};
      race("4. copy, change, swap in -- retry if someone beat us", [&] {
          for (int i = 0; i < EACH; i++) {
              long seen = counter.load();
              while (!counter.compare_exchange_weak(seen, seen + 1)) retries++;
          } }, [&] { return counter.load(); });
      std::printf("   (retries: %ld -- the times another thread swapped first)\n", retries.load()); }
    // 5. THE SAME ON A LIST of 1,000 numbers: every write copies the whole list.
    { auto list = std::make_shared<const std::vector<long>>(1000, 0); std::atomic<long> retries{0};
      std::atomic<std::shared_ptr<const std::vector<long>>> shared{list};
      constexpr int LIST_EACH = 20000;
      auto t = clk::now(); std::vector<std::thread> v;
      for (int k = 0; k < THREADS; k++) v.emplace_back([&] {
          for (int i = 0; i < LIST_EACH; i++) {
              auto seen = shared.load();
              while (true) {
                  auto next = std::make_shared<std::vector<long>>(*seen);   // the double
                  (*next)[0]++;
                  std::shared_ptr<const std::vector<long>> done = next;
                  if (shared.compare_exchange_weak(seen, done)) break;
                  retries++;
              } } });
      for (auto &x : v) x.join();
      double ms = std::chrono::duration<double, std::milli>(clk::now() - t).count();
      long got = (*shared.load())[0];
      say("5. copy-and-swap on a 1,000-number list", got, (long)THREADS * LIST_EACH, ms, (long)THREADS * LIST_EACH);
      std::printf("   (right answer %d; retries: %ld)\n", THREADS * LIST_EACH, retries.load()); }

    std::printf("\nTWO BOOLS -- the idea, and the two classic ways two-bool locks are made right:\n");

    // A. TWO BOOLS, EACH CHECKED AND THEN SET -- the author's idea as first said.
    { const int T = 4, N = 1000000; std::atomic<bool> first{false}, second{false}; std::atomic<long> counter{0};
      double ms = run(T, [&](int) { for (int i = 0; i < N; i++) {
          while (first.load() || second.load()) {}    // both must be false
          first.store(true); second.store(true);        // flip both
          counter.store(counter.load() + 1);
          second.store(false); first.store(false); } });
      say("A. two bools, 4 threads: wait while either is true, set both", counter.load(), (long)T * N, ms, (long)T * N); }
    // B. PETERSON (1981): a bool PER THREAD ("I want in") and one TURN ("you go first").
    //    Only two threads. Every step in order (seq_cst).
    { const int N = 2000000; std::atomic<bool> want[2] = {false, false}; std::atomic<int> turn{0}; long counter = 0;
      double ms = run(2, [&](int me) { int other = 1 - me; for (int i = 0; i < N; i++) {
          want[me].store(true); turn.store(other);
          while (want[other].load() && turn.load() == other) {}
          counter++;
          want[me].store(false); } });
      say("B. two bools + whose turn (Peterson), 2 threads, steps in order", counter, 2L * N, ms, 2L * N); }
    // C. THE SAME, but the processor may let "I want in" sit in its store buffer while it
    //    reads the other thread's bool -- x86 does exactly that unless told not to.
    { const int N = 2000000; std::atomic<bool> want[2] = {false, false}; std::atomic<int> turn{0}; long counter = 0;
      double ms = run(2, [&](int me) { int other = 1 - me; for (int i = 0; i < N; i++) {
          want[me].store(true, std::memory_order_release); turn.store(other, std::memory_order_release);
          while (want[other].load(std::memory_order_acquire) && turn.load(std::memory_order_acquire) == other) {}
          counter++;
          want[me].store(false, std::memory_order_release); } });
      say("C. Peterson with the processor free to reorder, 2 threads", counter, 2L * N, ms, 2L * N); }
    // D. LAMPORT'S BAKERY (1974): any number of threads -- a bool and a ticket EACH.
    { const int T = 4, N = 200000; std::atomic<bool> choosing[T]; std::atomic<long> ticket[T]; long counter = 0;
      for (int i = 0; i < T; i++) { choosing[i] = false; ticket[i] = 0; }
      double ms = run(T, [&](int me) { for (int i = 0; i < N; i++) {
          choosing[me] = true; long top = 0; for (int j = 0; j < T; j++) top = std::max(top, ticket[j].load());
          ticket[me] = top + 1; choosing[me] = false;
          for (int j = 0; j < T; j++) { if (j == me) continue; while (choosing[j]) {}
              while (ticket[j] != 0 && (ticket[j] < ticket[me] || (ticket[j] == ticket[me] && j < me))) {} }
          counter++;
          ticket[me] = 0; } });
      say("D. a bool + a ticket per thread (bakery), 4 threads", counter, (long)T * N, ms, (long)T * N); }

    // WHEN NOBODY ELSE WANTS IT -- the usual case: one thread, 20 million writes.
    std::printf("\nONE THREAD, nobody contending, 20,000,000 writes:\n");
    const int M = 20000000;
    { volatile long c = 0; auto t = clk::now(); for (int i = 0; i < M; i++) c = c + 1;
      std::printf("  a plain write, no lock at all                    %5.2f ns\n", std::chrono::duration<double, std::nano>(clk::now() - t).count() / M); }
    { std::atomic<bool> busy{false}; volatile long c = 0; auto t = clk::now();
      for (int i = 0; i < M; i++) { while (busy.exchange(true, std::memory_order_acquire)) {} c = c + 1; busy.store(false, std::memory_order_release); }
      std::printf("  one bool, flipped in one step                    %5.2f ns\n", std::chrono::duration<double, std::nano>(clk::now() - t).count() / M); }
    { std::atomic<bool> a{false}, b{false}; volatile long c = 0; auto t = clk::now();
      for (int i = 0; i < M; i++) { while (a.exchange(true, std::memory_order_acquire)) {} while (b.exchange(true, std::memory_order_acquire)) {}
                                    c = c + 1; b.store(false, std::memory_order_release); a.store(false, std::memory_order_release); }
      std::printf("  two bools, each flipped in one step              %5.2f ns\n", std::chrono::duration<double, std::nano>(clk::now() - t).count() / M); }
    { std::mutex m; volatile long c = 0; auto t = clk::now(); for (int i = 0; i < M; i++) { std::lock_guard g(m); c = c + 1; }
      std::printf("  a mutex                                          %5.2f ns\n", std::chrono::duration<double, std::nano>(clk::now() - t).count() / M); }
}
