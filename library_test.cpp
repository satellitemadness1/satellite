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
               std::make_shared<const Value>(
                   satellite::make_string(satellite::encode("two")))};
    List outer{std::make_shared<const Value>(
                   satellite::make_list(std::move(inner))),
               std::make_shared<const Value>(true)};
    lib.set("main", "things", satellite::make_list(std::move(outer)));
    auto things = lib.get("main", "things");
    if (!things || satellite::to_string(*things) != "[[1, two], true]") {
        printf("FAIL: nested container, got %s\n",
               things ? satellite::to_string(*things).c_str() : "null");
        return 1;
    }

    // A map, through the same publish protocol.
    //
    // This is the ONLY place the MapRef alternative comes under
    // ThreadSanitizer. The evaluator's own write paths do not: this file drives
    // the Library directly from C++ and never constructs an Evaluator, so
    // call_mutator's field arm has no TSan coverage either. Anything that made
    // a MapBody mutable after publication — a lazily built index, a memoised
    // hash, a lookup cache — would be a data race, and this is what would see
    // it.
    {
        satellite::MapBody body;
        body.entries.push_back(
            {std::make_shared<const Value>(
                 satellite::make_string(satellite::encode("bolt"))),
             std::make_shared<const Value>(satellite::Number(40))});
        body.index.emplace("sbolt", 0);
        lib.set("main", "parts", satellite::make_map(std::move(body)));
        auto parts = lib.get("main", "parts");
        if (!parts || satellite::to_string(*parts) != "{bolt: 40}") {
            printf("FAIL: map through the library, got %s\n",
                   parts ? satellite::to_string(*parts).c_str() : "null");
            return 1;
        }
    }

    // Concurrent increments through update() must not lose a single write,
    // while lock-free readers hammer the same variable and new variables
    // are interned underneath them.
    const int WRITERS = 8, INCREMENTS = 20000, READERS = 4;
    lib.set("counters", "shared", satellite::Number(0));
    lib.set("counters", "map", satellite::make_map(satellite::MapBody{}));
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
                // Read the map the writers are republishing. A reader walks the
                // entries and the index of a body another thread may be
                // replacing wholesale, which is exactly the race the frozen-
                // after-construction rule exists to make safe.
                auto m = lib.get("counters", "map");
                if (m) {
                    const satellite::MapBody *body = satellite::as_map(*m);
                    if (body && body->entries.size() != body->index.size())
                        n--;   // torn: entries and index disagreed
                }
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
                if (n % 1000 == 0) {
                    lib.set("writer" + std::to_string(i),
                            "n" + std::to_string(n), satellite::Number(n));
                    // Republish a whole map body, copy-on-write, exactly as
                    // call_mutator's global arm does for .set().
                    lib.update("counters", "map",
                               [&](const Value *current) -> Value {
                                   const satellite::MapBody *old =
                                       current ? satellite::as_map(*current)
                                               : nullptr;
                                   satellite::MapBody next;
                                   if (old)
                                       next = *old;
                                   std::string key =
                                       "s" + std::to_string(i) + "_" +
                                       std::to_string(n);
                                   if (!next.index.count(key)) {
                                       next.index.emplace(key,
                                                          next.entries.size());
                                       next.entries.push_back(
                                           {std::make_shared<const Value>(
                                                satellite::make_string(
                                                    satellite::encode(key))),
                                            std::make_shared<const Value>(
                                                satellite::Number(n))});
                                   }
                                   return satellite::make_map(std::move(next));
                               });
                }
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
