// satellite/satellite_random/random_draw.cpp -- the uniform draw and the three shapes.
// random_draw.hpp says why it rejects rather than divides, and why a shape is worked out once.

#include "random_draw.hpp"

#include "../machine/machine_codes.hpp"

#include <utility>
#include <vector>

namespace satellite004 {

namespace {

// How many bits `value` needs, value >= 1: whole limbs below the top, then the top's own.
unsigned long long int bit_length(const satellite_number &value)
{
    const std::size_t count = value.limb_count();
    const unsigned long long int top = value.limb(count - 1);
    return 64ull * (count - 1) + (top == 0 ? 0u : 64u - static_cast<unsigned>(__builtin_clzll(top)));
}

// ONE LIMB, NO ALLOCATION: a bound that fits a limb is every dice roll, every index and
// every range a program is likely to write, and the one-limb satellite_number is inline.
satellite_number draw_one_limb(LimbSource &limbs, unsigned long long int largest, unsigned long long int mask)
{
    for (;;) {
        const unsigned long long int drawn = limbs.next_limb() & mask;
        if (drawn <= largest)
            return satellite_number(drawn);
    }
}

const satellite_number &one()
{
    static const satellite_number kOne(1ull);
    return kOne;
}

} // namespace

satellite_number random_below(LimbSource &limbs, const satellite_number &bound)
{
    if (bound.negative() || bound.is_zero())
        return satellite_number();
    // THE LARGEST VALUE A DRAW MAY BE, and its bit length k: 2^(k-1) <= largest < 2^k, so
    // the k-bit values number at most twice the bound and at least half of them are kept.
    // A bound that is a power of two makes largest all ones below it: nothing is rejected.
    const satellite_number largest = bound - one();
    if (largest.is_zero())
        return satellite_number();
    const unsigned long long int bits = bit_length(largest);
    const std::size_t count = static_cast<std::size_t>((bits + 63) / 64);
    const unsigned spare = static_cast<unsigned>(count * 64 - bits);     // top-limb bits above k
    const unsigned long long int top_mask = spare == 0 ? ~0ull : (~0ull >> spare);
    if (count == 1)
        return draw_one_limb(limbs, largest.limb(0), top_mask);

    std::vector<unsigned long long int> drawn(count);
    for (;;) {
        for (std::size_t i = 0; i < count; ++i)
            drawn[i] = limbs.next_limb();
        drawn[count - 1] &= top_mask;
        // drawn <= largest, compared from the top; equal is in, it IS bound - 1.
        bool kept = true;
        for (std::size_t i = count; i-- > 0;) {
            const unsigned long long int theirs = largest.limb(i);
            if (drawn[i] != theirs) {
                kept = drawn[i] < theirs;
                break;
            }
        }
        if (kept)
            return satellite_number::from_limbs(std::move(drawn));
    }
}

DrawShape digits_shape(const satellite_number &digits)
{
    DrawShape shape{one(), satellite_number(), one()};
    if (!digits.negative())
        satellite_number::power(satellite_number(10ull), digits, shape.count);
    return shape;
}

DrawShape range_shape(const satellite_number &least, const satellite_number &most)
{
    DrawShape shape{one(), least, one()};
    // Both ends in, and the + 1 is the whole of what makes `most` reachable.
    if (!(most < least))
        shape.count = most - least + one();
    return shape;
}

DrawShape stepped_shape(const satellite_number &least, const satellite_number &most, const satellite_number &step)
{
    DrawShape shape{one(), least, one()};
    if (most < least || step.negative() || step.is_zero())
        return shape;
    // How many values the step lands on: (most - least) / step, counted from 0, plus one.
    // The quotient is whole when the step divides the span, which the caller checked; when
    // it does not, the floor still never reaches past `most`.
    satellite_number places, remainder;
    if (satellite_number::divide(most - least, step, places, remainder) != success)
        return shape;
    shape.count = places + one();
    shape.step = step;
    return shape;
}

satellite_number draw(LimbSource &limbs, const DrawShape &shape)
{
    return shape.base + shape.step * random_below(limbs, shape.count);
}

} // namespace satellite004
