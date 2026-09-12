// The twelve shapes' arithmetic, and the spin wrapped around it. See
// satellite_random/tiers.hpp for the two layers and which one a test drives.
//
// NOTHING HERE NAMES A PCG ENTITY. The generator arrives as random.hpp's
// Source behind its pointer, so this file adds no Apache-2.0 include -- the
// rule random.cpp states for the whole module.

#include "satellite_random/tiers.hpp"

#include <algorithm>
#include <memory>
#include <string>

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

// The seeded tier's stream -- M21, and DELIBERATELY NOT shared_source(). That
// one is seeded from the kernel on first use and a program can neither read
// nor set its state, which is exactly the property a replayable draw cannot
// have. Held behind a pointer because a Source is neither copyable nor
// movable, and reseeding is therefore a replacement rather than an assignment.
std::unique_ptr<Source> &seeded_source()
{
    static thread_local std::unique_ptr<Source> stream;
    return stream;
}

// How many digits a Number carries to the right of the point. Number has no
// places() of its own -- Float does, because there the right half IS the
// precision -- so it is read off the rendered form, which is canonical: a
// trailing zero does not survive it, so 0.50 answers 1 and not 2.
unsigned places_of(const Number &value)
{
    if (value.is_integer())
        return 0;
    const std::string text = value.to_string();
    const std::string::size_type point = text.find('.');
    if (point == std::string::npos)
        return 0;
    return static_cast<unsigned>(text.size() - point - 1);
}

} // namespace

void seed_stream(unsigned long long seed)
{
    seeded_source() = std::make_unique<Source>(static_cast<std::uint64_t>(seed));
}

bool stream_is_seeded() { return seeded_source() != nullptr; }

bool draw_grid(Bits32 &bits, const Number &low, const Number &high,
               unsigned digits, Number &out)
{
    // THE GRID MUST CONTAIN ITS OWN BOUNDS. A caller asking for 2 digits
    // between 0.125 and 0.875 would otherwise be answered on a grid neither
    // end sits on, and "inclusive at both ends" would quietly stop being
    // true -- so the spacing is the finer of what was asked for and what the
    // bounds already carry, never the coarser.
    unsigned fine = digits;
    fine = std::max(fine, places_of(low));
    fine = std::max(fine, places_of(high));

    const Number scale = power_of_ten(static_cast<long long>(fine));

    // Both bounds onto the integers, exactly: `fine` is at least each one's
    // own precision, so neither multiplication can leave a fraction behind.
    const Number lo = Number::mul(low, scale);
    const Number hi = Number::mul(high, scale);

    Number index;
    if (!draw_range(bits, lo, hi, index))
        return false;

    // Exact: the divisor is a power of ten and the dividend is an integer, so
    // the quotient terminates within `fine` places and nothing is rounded.
    out = Number::divide(index, scale, fine);
    return true;
}

bool seeded_grid(const Number &low, const Number &high, unsigned digits,
                 Number &out)
{
    return draw_grid(*seeded_source(), low, high, digits, out);
}

bool seeded_step(const Number &low, const Number &high, const Number &step,
                 Number &out)
{
    // draw_step needs no change to take a FRACTIONAL step: member_count
    // divides an exact span by an exact step and handlers.cpp has already
    // refused a step that does not divide it, and `low + step * index` is
    // exact multiplication either way. The only thing that was ever whole
    // about it was the check at the door.
    return draw_step(*seeded_source(), low, high, step, out);
}

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
