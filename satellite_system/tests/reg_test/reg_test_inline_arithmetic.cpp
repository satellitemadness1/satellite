// reg_test_inline_arithmetic.cpp — the inline path: that it allocates nothing,
// and that add_inline is exact on every case it accepts and declines the rest.
// Part of the reg_test binary; the harness and main() are in reg_test.cpp.
//
// The allocation section reads `allocations` and `counting`, which the global
// operator new in reg_test.cpp maintains. Nothing here may allocate outside the
// counted loops or the number it prints stops meaning anything.

#include "reg_test.hpp"

#include <cstdio>
#include <memory>
#include <random>

using namespace satellite;

void reg_test_inline_allocation()
{
    // --- the inline path allocates nothing ----------------------------------
    // The claim §17 rests on. Counted, not assumed.
    Reg a = Reg::small(2, 0);
    Reg b = Reg::small(3, 0);
    Reg out;

    counting = true;
    allocations = 0;
    for (int i = 0; i < 100000; i++)
        if (!add_inline(a, b, out))
            failures++;
    long long inline_allocs = allocations;
    counting = false;

    check(inline_allocs == 0, "100000 inline additions allocate nothing");
    if (inline_allocs != 0)
        printf("  (allocated %lld times)\n", inline_allocs);
    check_str(out.to_string(), "5", "and the answer is right");

    // The comparison that makes the number mean something: the same
    // addition through Number and a ValuePtr, which is what the tree walker
    // does today.
    counting = true;
    allocations = 0;
    for (int i = 0; i < 100000; i++) {
        volatile auto v = std::make_shared<const Value>(
            Number::add(Number(2), Number(3)));
        (void)v;
    }
    long long boxed_allocs = allocations;
    counting = false;
    check(boxed_allocs >= 100000,
          "the boxed path allocates at least once per addition");
    printf("  inline: %lld allocations   boxed: %lld allocations "
           "(100000 additions each)\n",
           inline_allocs, boxed_allocs);
}

void reg_test_inline_addition()
{
    // --- add_inline is exact, or declines -----------------------------------
    // Aligning exponents can overflow before a digit is added. 1 + 1e-30
    // needs 10^30 to line the decimal points up, and that is the case a
    // scheme which only checks the ADDITION gets wrong.
    Reg one = Reg::small(1, 0);
    Reg tiny = Reg::small(1, -30);
    Reg out;
    check(!add_inline(one, tiny, out),
          "alignment overflow declines rather than being wrong");

    // A modest exponent difference is fine and must be exact.
    Reg tenth = Reg::small(1, -1);
    check(add_inline(one, tenth, out), "1 + 0.1 stays inline");
    check_str(out.to_string(), "1.1", "and is exact, not 1.1000000000000001");

    // Significand overflow declines.
    Reg big = Reg::small(INT64_MAX, 0);
    check(!add_inline(big, one, out), "significand overflow declines");

    // A non-inline operand declines.
    Reg boxed = Reg::boxed(std::make_shared<const Value>(Number(1)));
    check(!add_inline(one, boxed, out), "a boxed operand declines");
    check(!add_inline(one, Reg::nil(), out), "nil declines");
    check(!add_inline(one, Reg(), out), "an EMPTY slot declines");

    // Fuzz against Number::add. Every case add_inline ACCEPTS must agree
    // with the arbitrary-precision answer exactly; the cases it declines
    // are the caller's bignum path and are correct by construction.
    std::mt19937_64 rng(20260808);
    std::uniform_int_distribution<long long> sig(-1000000000LL, 1000000000LL);
    std::uniform_int_distribution<int> ex(-12, 12);
    int accepted = 0;
    for (int i = 0; i < 200000; i++) {
        long long sa = sig(rng), sb = sig(rng);
        int ea = ex(rng), eb = ex(rng);
        Reg x = Reg::small(sa, ea), y = Reg::small(sb, eb), z;
        if (!add_inline(x, y, z))
            continue;
        accepted++;
        Number want = Number::add(Number::from_small(sa, ea),
                                  Number::from_small(sb, eb));
        if (z.to_string() != want.to_string()) {
            printf("FAIL: %lld e%d + %lld e%d -> %s, want %s\n", sa, ea, sb,
                   eb, z.to_string().c_str(), want.to_string().c_str());
            failures++;
            break;
        }
    }
    check(accepted > 100000, "the fuzz accepted a useful fraction inline");
    printf("  fuzz: %d of 200000 random additions stayed inline, all exact\n",
           accepted);
}
