// Three ways to pick the "likely scenario" function for `left + right`,
// where each side is one of satellite's 16 kinds of value (value.hpp's variant).
#include <chrono>
#include <cstdio>
#include <vector>

using Fn = long long (*)(long long);
static long long total = 0;
[[gnu::noinline]] long long most_likely(long long x) { return total += x; }       // string + string
[[gnu::noinline]] long long other(long long x) { return total += x + 1; }          // every other pair

struct Entry { int left, right; Fn fn; };

int main()
{
    const int kinds = 16, count = 10000000;
    const int STR = 3;                                   // value.hpp: Str is arm 3

    // The input: 90% string + string, the rest spread over all pairs.
    std::vector<int> lefts(count), rights(count);
    unsigned seed = 12345;
    for (int i = 0; i < count; i++) {
        seed = seed * 1103515245 + 12345;
        bool likely = (seed >> 16) % 10 != 0;
        lefts[i] = likely ? STR : (seed >> 8) % kinds;
        rights[i] = likely ? STR : (seed >> 20) % kinds;
    }

    // A: the most likely function first, then search a vector of the others.
    std::vector<Entry> others;
    for (int l = 0; l < kinds; l++)
        for (int r = 0; r < kinds; r++)
            if (!(l == STR && r == STR)) others.push_back({l, r, other});

    // B: a 16 x 16 table -- the two kinds ARE the position, so there is no search.
    Fn table[16][16];
    for (int l = 0; l < kinds; l++)
        for (int r = 0; r < kinds; r++)
            table[l][r] = (l == STR && r == STR) ? most_likely : other;

    auto time = [&](const char *name, auto &&body) {
        total = 0;
        auto start = std::chrono::high_resolution_clock::now();
        body();
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::high_resolution_clock::now() - start).count();
        std::printf("%-58s %6.2f ns per +\n", name, double(ns) / count);
    };

    time("A: check most likely, then search the vector", [&] {
        for (int i = 0; i < count; i++) {
            if (lefts[i] == STR && rights[i] == STR) { most_likely(i); continue; }
            for (const Entry &e : others)
                if (e.left == lefts[i] && e.right == rights[i]) { e.fn(i); break; }
        }
    });
    time("B: table[left kind][right kind], no search", [&] {
        for (int i = 0; i < count; i++) table[lefts[i]][rights[i]](i);
    });
    time("C: baseline, C++ already knows the types (call directly)", [&] {
        for (int i = 0; i < count; i++) (lefts[i] == STR && rights[i] == STR) ? most_likely(i) : other(i);
    });
}
