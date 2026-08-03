// Does the dispatch table actually beat the if-chain?  This is the design
// decision at the heart of satellite_container, so it gets measured rather
// than assumed.  Both versions compute exactly the same results.
#include "satellite/container.hpp"

#include <chrono>
#include <cstdio>
#include <random>
#include <vector>

using namespace satellite;
using Clock = std::chrono::steady_clock;

// The straightforward version: check the types, branch to the right code.
static Container add_ifchain(const Container& a, const Container& b) {
    if (a.type == Type::Int && b.type == Type::Int) {
        int64_t r;
        if (!__builtin_add_overflow(a.i, b.i, &r)) return Container::integer(r);
        BigInt x, y;
        return big::add(x, y);
    } else if (a.type == Type::Real && b.type == Type::Real) {
        return Container::real(a.d + b.d);
    } else if (a.type == Type::Int && b.type == Type::Real) {
        return Container::real((double)a.i + b.d);
    } else if (a.type == Type::Real && b.type == Type::Int) {
        return Container::real(a.d + (double)b.i);
    } else if (a.type == Type::Str || b.type == Type::Str) {
        return Container::str(a.to_string() + b.to_string());
    } else if (a.type == Type::Bool && b.type == Type::Bool) {
        return Container::integer((a.b ? 1 : 0) + (b.b ? 1 : 0));
    } else if (a.type == Type::List && b.type == Type::List) {
        return add(a, b);
    } else if (a.type == Type::Big || b.type == Type::Big) {
        return add(a, b);
    }
    return Container::nil();
}

int main() {
    init_tables();

    std::mt19937_64 rng(12345);
    const int N = 200000;

    // Two workloads: one pure-int (the common case), one type-mixed (where
    // the branch predictor has nothing to learn).
    std::vector<Container> pure, mixed;
    for (int k = 0; k < N; ++k) {
        pure.push_back(Container::integer((int64_t)(rng() % 1000)));
        switch (rng() % 4) {
            case 0:  mixed.push_back(Container::integer((int64_t)(rng() % 1000))); break;
            case 1:  mixed.push_back(Container::real((double)(rng() % 1000)));     break;
            case 2:  mixed.push_back(Container::boolean(rng() & 1));               break;
            default: mixed.push_back(Container::integer((int64_t)(rng() % 1000))); break;
        }
    }

    auto run = [&](const char* label, std::vector<Container>& v, bool table) {
        double best = 1e18;
        volatile int64_t sink = 0;
        for (int rep = 0; rep < 5; ++rep) {
            auto t0 = Clock::now();
            for (size_t k = 0; k + 1 < v.size(); ++k) {
                Container r = table ? add(v[k], v[k + 1]) : add_ifchain(v[k], v[k + 1]);
                sink += (r.type == Type::Int) ? r.i : 1;
            }
            double ms = std::chrono::duration<double, std::milli>(Clock::now() - t0).count();
            if (ms < best) best = ms;
        }
        double ns_per_op = best * 1e6 / (double)(v.size() - 1);
        printf("  %-34s %8.2f ms   %6.2f ns/op\n", label, best, ns_per_op);
        return ns_per_op;
    };

    printf("dispatch: %d ops per pass, best of 5\n\n", N);
    printf("PURE INT (predictable -- best case for the if-chain)\n");
    double a1 = run("if-chain", pure, false);
    double a2 = run("dispatch table", pure, true);
    printf("  -> table is %.2fx %s\n\n", a1 > a2 ? a1 / a2 : a2 / a1,
           a1 > a2 ? "faster" : "slower");

    printf("MIXED TYPES (what real satellite code looks like)\n");
    double b1 = run("if-chain", mixed, false);
    double b2 = run("dispatch table", mixed, true);
    printf("  -> table is %.2fx %s\n\n", b1 > b2 ? b1 / b2 : b2 / b1,
           b1 > b2 ? "faster" : "slower");

    // How much does adding types to the language cost each design?  The table
    // is O(1) forever; the if-chain grows a branch per type pair.
    printf("note: the if-chain above handles 8 cases.  every new satellite type\n");
    printf("      adds more branches to it; the table stays one indexed call.\n");
    return 0;
}
