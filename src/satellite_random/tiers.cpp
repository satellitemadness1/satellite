// The twelve shapes' arithmetic, and the spin wrapped around it. See
// satellite_random/tiers.hpp for the two layers and which one a test drives.
//
// NOTHING HERE NAMES A PCG ENTITY. The generator arrives as random.hpp's
// Source behind its pointer, so this file adds no Apache-2.0 include -- the
// rule random.cpp states for the whole module.

#include "satellite_random/tiers.hpp"

namespace satellite {

namespace {

// One generator for the process, constructed on first draw. Source's own note
// prices a construction -- the whole 16384-word table from the kernel -- and
// that price is why this is a static and never a per-call local. thread_local
// because a Source is one generator's state and the pool's workers must not
// interleave draws through one stream unlocked; today the only caller is the
// walking thread, and this line is what keeps that true by construction
// rather than by remembering.
Bits32 &shared_source()
{
    static thread_local Source source;
    return source;
}

// 10^digits, exactly: a significand of 1 at exponent `digits` IS that power
// of ten, with no multiplication and no rounding anywhere.
Number power_of_ten(long long digits)
{
    return Number::from_small(true, 1, static_cast<int>(digits));
}

// How many members {low, low + step, ..., high} has, exactly. The step
// divides high - low -- handlers.cpp refused the call otherwise -- so the
// division terminates inside its own digit count and the quotient is the
// integer it must be. The count asked of divide() is the dividend's own
// rendered length plus one: an integer quotient never carries more digits
// than its dividend, so nothing is rounded away.
Number member_count(const Number &low, const Number &high, const Number &step)
{
    const Number span = Number::sub(high, low);
    if (span.is_zero())
        return Number(1);
    const unsigned digits =
        static_cast<unsigned>(span.to_string().size()) + 1;
    return Number::add(Number::divide(span, step, digits), Number(1));
}

// The spin's callbacks: draw the same shape the answer will have, and let it
// fall. The context is the arguments, because spin() speaks function pointer
// rather than capture -- random.hpp's signature.
struct DigitsWork {
    long long digits;
};

void discard_digits(Bits32 &bits, void *context)
{
    Number thrown;
    (void)draw_digits(bits, static_cast<DigitsWork *>(context)->digits, thrown);
}

struct RangeWork {
    const Number *low;
    const Number *high;
};

void discard_range(Bits32 &bits, void *context)
{
    RangeWork *work = static_cast<RangeWork *>(context);
    Number thrown;
    (void)draw_range(bits, *work->low, *work->high, thrown);
}

struct StepWork {
    const Number *low;
    const Number *high;
    const Number *step;
};

void discard_step(Bits32 &bits, void *context)
{
    StepWork *work = static_cast<StepWork *>(context);
    Number thrown;
    (void)draw_step(bits, *work->low, *work->high, *work->step, thrown);
}

} // namespace

bool draw_digits(Bits32 &bits, long long digits, Number &out)
{
    return Number::random_below(power_of_ten(digits), bits, out);
}

bool draw_range(Bits32 &bits, const Number &low, const Number &high,
                Number &out)
{
    // Inclusive at both ends, and the +1 is the whole of what makes `high`
    // reachable.
    const Number span = Number::add(Number::sub(high, low), Number(1));

    Number offset;
    if (!Number::random_below(span, bits, offset))
        return false;

    out = Number::add(low, offset);
    return true;
}

bool draw_step(Bits32 &bits, const Number &low, const Number &high,
               const Number &step, Number &out)
{
    Number index;
    if (!Number::random_below(member_count(low, high, step), bits, index))
        return false;

    out = Number::add(low, Number::mul(step, index));
    return true;
}

bool tier_digits(Tier tier, long long digits, Number &out, long long *discarded)
{
    Bits32 &bits = shared_source();
    DigitsWork work{digits};
    const long long thrown = spin(tier, bits, discard_digits, &work);
    if (discarded)
        *discarded = thrown;
    return draw_digits(bits, digits, out);
}

bool tier_range(Tier tier, const Number &low, const Number &high, Number &out,
                long long *discarded)
{
    Bits32 &bits = shared_source();
    RangeWork work{&low, &high};
    const long long thrown = spin(tier, bits, discard_range, &work);
    if (discarded)
        *discarded = thrown;
    return draw_range(bits, low, high, out);
}

bool tier_step(Tier tier, const Number &low, const Number &high,
               const Number &step, Number &out, long long *discarded)
{
    Bits32 &bits = shared_source();
    StepWork work{&low, &high, &step};
    const long long thrown = spin(tier, bits, discard_step, &work);
    if (discarded)
        *discarded = thrown;
    return draw_step(bits, low, high, step, out);
}

} // namespace satellite
