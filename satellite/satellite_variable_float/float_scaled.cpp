// satellite/satellite_variable_float/float_scaled.cpp -- a float worked out as
// one whole number and a count of places (float_scaled.hpp says why, and what
// the one form is).

#include "float_scaled.hpp"

#include <bit>
#include <climits>
#include <utility>

namespace satellite004 {
namespace {

constexpr unsigned long long int kTenToTheNineteen = 10000000000000000000ull;

// How many bits the magnitude takes; 0 for zero.
unsigned long long int bits_of(const satellite_number &value)
{
    const std::size_t count = value.limb_count();
    const unsigned long long int top = value.limb(count - 1);
    return (count - 1) * 64ull + (top == 0 ? 0 : 64 - std::countl_zero(top));
}

// a + b, or every count there is when that does not fit. A row past one limb is
// more digits than a machine has memory for, so "all of them" is exactly what it
// means -- the sum saturating is that, and never a cap on anything that exists.
unsigned long long int sum_or_every(unsigned long long int a, unsigned long long int b)
{
    return a > ULLONG_MAX - b ? ULLONG_MAX : a + b;
}

// HOW MANY DIGITS THE WHOLE SIDE HAS PAST arguments.float.whole, or 0 --
// `magnitude` being held at `places` places. The bits answer it without dividing
// nearly every time: 8^k is below 10^k, so a number of fewer than 3 * (places +
// whole) bits is below 10^(places + whole), and its whole side fits.
unsigned long long int whole_digits_over(const satellite_number &magnitude, unsigned long long int places)
{
    const unsigned long long int room = sum_or_every(places, float_precision_in_use.whole);
    if (bits_of(magnitude) / 3 < room)
        return 0;
    // A NUMBER IN MEMORY HAS A DIGIT COUNT THAT FITS ONE LIMB, so limb(0) is all of it.
    const unsigned long long int digits = magnitude.digits().limb(0);
    return digits > room ? digits - room : 0;
}

// THE ONE FORM: no zero at the end of the fraction. Nineteen at a time while they
// come off whole -- one limb's worth of tens -- and then one at a time. An odd
// number ends in no zero, which is most answers that did not come out even, and
// they leave on the first test.
void without_end_zeros(satellite_number &magnitude, unsigned long long int &places)
{
    if (magnitude.is_zero()) {
        places = 0;
        return;
    }
    satellite_number quotient, remainder;
    const satellite_number nineteen_tens(kTenToTheNineteen), ten(10ull);
    while (places >= 19 && (magnitude.limb(0) & 1) == 0) {
        satellite_number::divide(magnitude, nineteen_tens, quotient, remainder);
        if (!remainder.is_zero())
            break;
        magnitude = std::move(quotient);
        places -= 19;
    }
    while (places > 0 && (magnitude.limb(0) & 1) == 0) {
        satellite_number::divide(magnitude, ten, quotient, remainder);
        if (!remainder.is_zero())
            break;
        magnitude = std::move(quotient);
        --places;
    }
}

// HOW MANY PLACES END rest / denominator, or every count there is when nothing
// does. It ends exactly when what is left of the denominator, its twos and fives
// divided out, divides the rest -- 1 / 8 ends, 2 / 3 never does -- and then the
// places it needs are at most the larger of the two counts. Twos come off a
// limb's worth at a time and fives twenty-seven at a time (5^27 is the largest
// that fits one limb), so a denominator that is mostly tens is not walked digit
// by digit.
unsigned long long int places_that_end(const satellite_number &rest, satellite_number denominator)
{
    constexpr unsigned long long int kFiveToTheTwentySeven = 7450580596923828125ull;
    unsigned long long int twos = 0, fives = 0;
    satellite_number quotient, remainder;
    while ((denominator.limb(0) & 1) == 0) {                 // the denominator is never zero here
        const unsigned long long int low = denominator.limb(0);
        const int shift = low == 0 ? 63 : std::countr_zero(low);
        satellite_number::divide(denominator, satellite_number(1ull << shift), quotient, remainder);
        denominator = std::move(quotient);
        twos += static_cast<unsigned long long int>(shift);
    }
    const satellite_number many_fives(kFiveToTheTwentySeven), five(5ull);
    for (bool many = true;;) {
        satellite_number::divide(denominator, many ? many_fives : five, quotient, remainder);
        if (!remainder.is_zero()) {
            if (!many)
                break;
            many = false;
            continue;
        }
        denominator = std::move(quotient);
        fives += many ? 27 : 1;
    }
    satellite_number::divide(rest, denominator, quotient, remainder);
    if (!remainder.is_zero())
        return ULLONG_MAX;
    return twos > fives ? twos : fives;
}

// The magnitude at `places` places cut into the author's two numbers, and the sign.
satellite_float split(satellite_number magnitude, unsigned long long int places, bool negative)
{
    satellite_float out;
    if (places == 0)
        out.whole = std::move(magnitude);
    else
        satellite_number::divide(magnitude, ten_to_the(places), out.whole, out.fraction);
    out.places = places;
    out.negative = negative && !(out.whole.is_zero() && out.fraction.is_zero());
    return out;
}

} // namespace

// THE LAST FEW LARGE POWERS OF TEN ARE KEPT. A float held at arguments.float.decimal
// places asks for the same two or three over and over -- 10^128 to divide, 10^96 to
// show 32 of the places -- and building 10^4096 costs more than the division it
// serves (measured, 2026-09-22: 2.0 / 3 at 4096 places, displayed, a warm median of
// 36.4 us building them every time and 6.1 us keeping them).
// Four a thread, the oldest replaced, so it never grows and no thread waits on
// another's.
satellite_number ten_to_the(unsigned long long int count)
{
    if (count <= 19) {
        unsigned long long int value = 1;
        while (count-- > 0)
            value *= 10;
        return satellite_number(value);
    }
    struct kept_power {
        unsigned long long int count = 0;    // 0 is never asked for here, so it means empty
        satellite_number value;
    };
    thread_local kept_power kept[4];
    thread_local unsigned int oldest = 0;
    for (const kept_power &one : kept)
        if (one.count == count)
            return one.value;
    satellite_number out;
    satellite_number::power(satellite_number(10ull), satellite_number(count), out);
    kept[oldest] = kept_power{count, out};
    oldest = (oldest + 1) % 4;
    return out;
}

satellite_number divided_rounded(const satellite_number &dividend, const satellite_number &divisor)
{
    satellite_number quotient, remainder;
    satellite_number::divide(dividend, divisor, quotient, remainder);   // toward zero; the remainder has the dividend's sign
    const satellite_number twice = remainder.negative() ? -(remainder + remainder) : remainder + remainder;
    if (satellite_number::compare(twice, divisor) >= 0)
        quotient += satellite_number(1ull, dividend.negative());
    return quotient;
}

satellite_number float_scaled(const satellite_float &value, unsigned long long int places)
{
    satellite_number scaled = places == 0 ? value.whole : value.whole * ten_to_the(places);
    if (!value.fraction.is_zero())
        scaled += places == value.places ? value.fraction : value.fraction * ten_to_the(places - value.places);
    return value.negative ? -scaled : scaled;
}

satellite_float float_from_scaled(satellite_number scaled, unsigned long long int places)
{
    const bool negative = scaled.negative();
    satellite_number magnitude = negative ? -scaled : scaled;
    // THE WHOLE SIDE FIRST: past arguments.float.whole digits the low ones round to
    // zeros, and the fraction goes with them -- one rounding, at the last digit kept.
    if (const unsigned long long int over = whole_digits_over(magnitude, places)) {
        magnitude = divided_rounded(magnitude, ten_to_the(places + over)) * ten_to_the(over);
        places = 0;
    } else if (places > float_precision_in_use.decimal) {
        magnitude = divided_rounded(magnitude, ten_to_the(places - float_precision_in_use.decimal));
        places = float_precision_in_use.decimal;
    }
    without_end_zeros(magnitude, places);
    return split(std::move(magnitude), places, negative);
}

satellite_float float_from_quotient(const satellite_number &numerator, const satellite_number &denominator,
                                    bool negative)
{
    satellite_number whole, rest;
    satellite_number::divide(numerator, denominator, whole, rest);
    // PAST THE WHOLE SIDE'S DIGITS, one rounding at the last one kept, worked from
    // the quotient itself and not from a rounded one.
    if (const unsigned long long int over = whole_digits_over(whole, 0))
        return split(divided_rounded(numerator, denominator * ten_to_the(over)) * ten_to_the(over), 0, negative);
    if (rest.is_zero())
        return split(std::move(whole), 0, negative);

    // THE FEWEST PLACES THAT END IT, or arguments.float.decimal when none do --
    // one division either way. Worked at the most places every time, 1 / 4 would be
    // 25 followed by 126 zeros to take back off, and at a million places a million.
    const unsigned long long int ends = places_that_end(rest, denominator);
    const unsigned long long int most = float_precision_in_use.decimal;
    unsigned long long int places = ends < most ? ends : most;
    const satellite_number scale = ten_to_the(places);
    satellite_number fraction, left_over;
    satellite_number::divide(rest * scale, denominator, fraction, left_over);
    if (!left_over.is_zero() && satellite_number::compare(left_over + left_over, denominator) >= 0) {
        fraction += satellite_number(1ull);
        if (fraction == scale) {             // .999...9 rounded up is the next whole number
            fraction = satellite_number();
            whole += satellite_number(1ull);
        }
    }
    without_end_zeros(fraction, places);
    // BUILT AS THE TWO NUMBERS IT ALREADY IS, never joined into one and divided back.
    satellite_float out;
    out.negative = negative && !(whole.is_zero() && fraction.is_zero());   // -1 / 10^200 at 128 places is 0.0
    out.whole = std::move(whole);
    out.fraction = std::move(fraction);
    out.places = places;
    return out;
}

satellite_float float_of_number(const satellite_number &value)
{
    return float_from_scaled(value, 0);
}

std::string float_written(const satellite_float &value)
{
    const unsigned long long int shown = float_precision_in_use.shown;
    satellite_number whole = value.whole, fraction = value.fraction;
    unsigned long long int places = value.places;
    if (places > shown) {
        fraction = divided_rounded(fraction, ten_to_the(places - shown));
        places = shown;
        if (fraction == ten_to_the(shown)) {   // .99999... shown to fewer places is the next whole number
            fraction = satellite_number();
            whole += satellite_number(1ull);
        }
    }
    std::string after = fraction.to_text();
    if (after.size() < places)
        after.insert(0, static_cast<std::size_t>(places - after.size()), '0');
    while (after.size() > 1 && after.back() == '0')
        after.pop_back();
    // A NEGATIVE FLOAT THAT SHOWS AS ZERO SHOWS NO SIGN: -0.0000...1 to 32 places
    // is 0.0, and "-0.0" would be a number that does not exist.
    const bool below_zero = value.negative && !(whole.is_zero() && fraction.is_zero());
    return (below_zero ? "-" : "") + whole.to_text() + "." + after;
}

} // namespace satellite004
