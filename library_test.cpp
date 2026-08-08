// Stress test for satellite.library's concurrency contract:
// writers lock, readers never do, and no increment may be lost.

#include "library.hpp"

#include <atomic>
#include <cstdio>
#include <thread>
#include <vector>

using satellite::Library;
using satellite::List;
using satellite::Value;

int main()
{
    auto &lib = Library::instance();

    // Basic set/get and dotted-path resolution.
    lib.set("main", "x", satellite::Number(42));
    auto x = lib.get_path("satellite.library.main.x");
    if (!x || !(std::get<satellite::Number>(*x) == satellite::Number(42))) {
        printf("FAIL: path lookup\n");
        return 1;
    }

    // A container holding pointers to other values, nested.
    List inner{std::make_shared<const Value>(satellite::Number(1)),
               std::make_shared<const Value>(satellite::encode("two"))};
    List outer{std::make_shared<const Value>(std::move(inner)),
               std::make_shared<const Value>(true)};
    lib.set("main", "things", std::move(outer));
    auto things = lib.get("main", "things");
    if (!things || satellite::to_string(*things) != "[[1, two], true]") {
        printf("FAIL: nested container, got %s\n",
               things ? satellite::to_string(*things).c_str() : "null");
        return 1;
    }

    // Concurrent increments through update() must not lose a single write,
    // while lock-free readers hammer the same variable and new variables
    // are interned underneath them.
    const int WRITERS = 8, INCREMENTS = 20000, READERS = 4;
    lib.set("counters", "shared", satellite::Number(0));
    std::atomic<bool> stop{false};
    std::atomic<long long> reads{0};

    std::vector<std::thread> readers;
    for (int i = 0; i < READERS; i++)
        readers.emplace_back([&] {
            long long n = 0;
            while (!stop.load(std::memory_order_relaxed)) {
                auto snapshot = lib.get("counters", "shared");
                if (snapshot && !std::get<satellite::Number>(*snapshot).is_negative())
                    n++;
            }
            reads += n;
        });

    std::vector<std::thread> writers;
    for (int i = 0; i < WRITERS; i++)
        writers.emplace_back([&, i] {
            for (int n = 0; n < INCREMENTS; n++) {
                lib.update("counters", "shared", [](const Value *current) {
                    return Value(satellite::Number::add(std::get<satellite::Number>(*current),
                                                 satellite::Number(1)));
                });
                if (n % 1000 == 0)
                    lib.set("writer" + std::to_string(i),
                            "n" + std::to_string(n), satellite::Number(n));
            }
        });

    for (auto &t : writers)
        t.join();
    stop = true;
    for (auto &t : readers)
        t.join();

    satellite::Number got = std::get<satellite::Number>(*lib.get("counters", "shared"));
    satellite::Number want = satellite::Number::mul(
        satellite::Number(WRITERS), satellite::Number(INCREMENTS));
    if (!(got == want)) {
        printf("FAIL: shared counter got %s, want %s (lost writes)\n",
               got.to_string().c_str(), want.to_string().c_str());
        return 1;
    }

    printf("PASS: %s increments intact, %lld lock-free reads, "
           "%zu variables in satellite.library\n",
           got.to_string().c_str(), reads.load(), lib.list().size());
    return 0;
}
