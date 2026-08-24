// The two limits the evaluator reads out of satellite.library.system: how deep
// the tree walk may recurse, and how many significant digits a non-terminating
// division keeps.
//
// Moved verbatim out of helpers.cpp, which was 779 lines. The measurements the
// depth ceiling is calibrated from are in the comments below, and they are the
// reason the numbers are the numbers they are -- they move with the code.
//
// Part of src/evaluator/. See eval_internal.hpp for what these pieces share.

#include "evaluator/eval_internal.hpp"

#include <climits>

namespace satellite {

// What the depth guard is protecting, measured rather than guessed.
//
// One capsule activation costs exactly 3 depth units — eval(Call) -> eval_call
// -> call_capsule -> exec(body) -> exec_block -> exec(return) -> eval — and
// 3169 bytes of C++ stack at -O2 (clang 24, x86-64). Three units is the fewest
// any body can cost, so that is the WORST ratio the guard has to survive: a
// longer body spends more units per byte, not fewer. The cost does not grow
// with the number of locals either, because a frame's slots are a vector.
//
// Where the C++ stack actually ends, bisected on an 8 MB stack with the guard
// disabled:
//
//     -O2    2600 levels return, 2800 segfault   -> ~7950 units
//     -O0    1200 levels return, 1300 segfault   -> ~3750 units
//
// §6's suggested default of 10000 was written when nothing recursed, and it is
// past BOTH cliffs — so the guard could never have fired, and a runaway
// recursion was a segfault rather than the error the guard exists to produce.
// 2000 units is a quarter of the optimised stack and half the unoptimised one.
// It is also what the resolver walks to (MAX_RESOLVE_DEPTH, env.cpp), so
// neither pass is the one that dies first.
constexpr int DEFAULT_MAX_DEPTH = 2000;

// The ceiling on satellite.library.system.max_depth, DERIVED FROM THE STACK
// THE PROCESS ACTUALLY HAS rather than fixed at one number.
//
// It used to be a hard 3000, and the reasoning for that number was sound while
// every run had the ordinary 8 MB: the knob raises the limit toward the cliff
// and must not raise it past, because above the cliff a larger setting does not
// buy deeper recursion, it buys a segfault instead of an error message. 3000
// also stayed under the -O0 cliff, an unoptimised build being exactly where
// losing the error message costs most.
//
// What that constant got wrong is that the cliff is not a constant. It is the
// stack, and the stack is `ulimit -s`. On 8 MB this formula still answers 3000,
// so nothing about a default run changes; on a 64 MB stack it answers ~24,000,
// and on a 64 GB one ~24,500,000 — about 8 million capsule activations, which
// is a depth no program reaches without meaning to.
//
// The divisor is the whole calibration and it is not a guess. One activation
// costs 3 units and ~3169 bytes at -O2, so 8 MB is ~7950 units of cliff; the
// old 3000 was 38% of that, and 8388608/2796 reproduces 3000 exactly. Keeping
// the RATIO rather than the number is what carries the -O0 margin along with
// it: an unoptimised activation costs about 2.1x more stack, and 38% of the
// -O2 cliff stays under the -O0 one at every stack size, not just at 8 MB.
//
// STACK_LIMIT_UNKNOWN — getrlimit failed, or said RLIM_INFINITY — is read as
// the ordinary 8 MB. "Unlimited" is not unbounded: the main thread's stack
// still stops where the next mapping begins, and believing the word would put
// the guard back past the cliff.
constexpr unsigned long long CEILING_BYTES_PER_UNIT = 2796;
constexpr unsigned long long ASSUMED_STACK_BYTES = 8ull << 20;

int max_max_depth()
{
    unsigned long long bytes = stack_limit_bytes();
    if (bytes == STACK_LIMIT_UNKNOWN)
        bytes = ASSUMED_STACK_BYTES;

    unsigned long long units = bytes / CEILING_BYTES_PER_UNIT;

    // A floor of 3, because one activation costs exactly that and a ceiling
    // below it would make every capsule call an error — a stack small enough
    // to justify that is one the process could not have started on.
    if (units < 3)
        units = 3;
    if (units > static_cast<unsigned long long>(INT_MAX))
        units = static_cast<unsigned long long>(INT_MAX);
    return static_cast<int>(units);
}

int read_max_depth()
{
    const int ceiling = max_max_depth();

    ValuePtr v = Library::instance().get("system", "max_depth");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(std::min<long long>(set, ceiling));

    // The DEFAULT is clamped by the ceiling too, which the fixed-constant
    // version never had to think about. `ulimit -s 1024` gives a cliff of ~370
    // units, and handing that run the unconditional 2000 would segfault it
    // before the guard ever looked.
    return std::min(DEFAULT_MAX_DEPTH, ceiling);
}

// The significant digits a non-terminating division keeps (§8.1). A knob for
// the same reason max_depth is one: the right answer depends on the program,
// and the default is only a default.
int read_division_digits()
{
    ValuePtr v = Library::instance().get("system", "division_digits");
    long long set = 0;
    if (v)
        if (const Number *n = std::get_if<Number>(v.get()))
            if (n->floor().to_integer(set) && set >= 1)
                return static_cast<int>(
                    std::min<long long>(set, Number::MAX_DIVISION_DIGITS));
    return Number::DEFAULT_DIVISION_DIGITS;
}

} // namespace satellite
