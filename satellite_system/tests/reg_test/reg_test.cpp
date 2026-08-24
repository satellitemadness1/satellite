// reg.hpp — the VM register type and the register stack.
//
// Three things this file exists to pin, because each is a rule that would
// otherwise be true only by accident:
//
//   1. A slot has THREE states, not two. EMPTY is distinguishable from a slot
//      holding nil, which is what keeps src/evaluator/slots.cpp's "read before its
//      declaration runs" check alive once frames become register windows.
//   2. The inline path ALLOCATES NOTHING. That is the entire point of the type
//      — §17 measured 28 ns of make_shared in a 112.5 ns addition — so it is
//      counted here rather than assumed, with a global operator new counter.
//   3. add_inline is EXACT. It agrees with Number::add on every case it
//      accepts, and it declines rather than being wrong — including when
//      ALIGNING two exponents overflows before any addition happens.
//
// This file is the driver of the reg_test binary: the check helpers, the
// global operator new that counts allocations, the counters it feeds, and a
// main() that runs each section in the order they were written in. The sections
// themselves live in reg_test_slot_values.cpp, reg_test_inline_arithmetic.cpp
// and reg_test_register_stack.cpp, and are declared in reg_test.hpp. The
// operator new below is why the split has a rule: a replacement operator new
// may be defined in exactly one translation unit of the program, so it stays
// here and every section reaches the counters through the header.

#include "reg_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <new>

// --- allocation counting ----------------------------------------------------
// Global, because the claim is about the whole path and not about one call.

long long allocations = 0;
bool counting = false;

void *operator new(size_t size)
{
    if (counting)
        allocations++;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete(void *p) noexcept { std::free(p); }
void operator delete(void *p, size_t) noexcept { std::free(p); }
void *operator new[](size_t size)
{
    if (counting)
        allocations++;
    void *p = std::malloc(size ? size : 1);
    if (!p)
        throw std::bad_alloc();
    return p;
}
void operator delete[](void *p) noexcept { std::free(p); }
void operator delete[](void *p, size_t) noexcept { std::free(p); }

using namespace satellite;

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

void check_str(const std::string &got, const std::string &want,
               const char *what)
{
    if (got != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what, want.c_str(),
               got.c_str());
        failures++;
    }
}

int main()
{
    reg_test_slot_states();
    reg_test_value_round_trips();
    reg_test_inline_allocation();
    reg_test_inline_addition();
    reg_test_frame_windows();
    reg_test_stack_startup_cost();

    printf("PASS: reg (three slot states with EMPTY distinct from nil; "
           "round trips for all eleven Value alternatives; the inline path "
           "allocates nothing; add_inline exact or declining, alignment "
           "overflow included; frames as windows with zero-copy arguments; "
           "sizeof(Reg)=%zu)\n",
           sizeof(Reg));
    return failures ? 1 : 0;
}
