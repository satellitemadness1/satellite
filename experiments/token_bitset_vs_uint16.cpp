// Is 8 bytes a token a bottleneck? Flat arrays, same count, same work.
#include <algorithm>
#include <bitset>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <vector>

template <class F> static double best_ms(F f, int runs = 5) {
    double b = 1e18;
    for (int i = 0; i < runs; ++i) {
        auto a = std::chrono::steady_clock::now(); f();
        auto z = std::chrono::steady_clock::now();
        b = std::min(b, std::chrono::duration<double, std::milli>(z - a).count());
    }
    return b;
}

int main() {
    printf("%12s %14s %14s %14s %14s %9s\n",
           "tokens", "bitset MB", "uint16 MB", "bitset ms", "uint16 ms", "ratio");
    for (size_t n : {100000ull, 1000000ull, 10000000ull, 50000000ull}) {
        std::vector<std::bitset<16>> b(n);
        std::vector<uint16_t> w(n);
        for (size_t i = 0; i < n; ++i) { uint16_t v = uint16_t(256 + (i * 2654435761u) % 300); b[i] = v; w[i] = v; }

        const std::bitset<16> want_b(512); const uint16_t want_w = 512;
        volatile size_t sink = 0;

        double tb = best_ms([&]{ size_t c = 0; for (size_t i = 0; i < n; ++i) if (b[i] == want_b) ++c; sink = c; });
        double tw = best_ms([&]{ size_t c = 0; for (size_t i = 0; i < n; ++i) if (w[i] == want_w) ++c; sink = c; });

        printf("%12zu %14.1f %14.1f %14.2f %14.2f %8.2fx\n", n,
               n * sizeof(std::bitset<16>) / 1048576.0, n * sizeof(uint16_t) / 1048576.0,
               tb, tw, tb / tw);
    }
    printf("\nL2/L3 on this machine:\n");
    return 0;
}
