// The Float value itself: construction, the invariants, rendering, rounding.
// See satellite_float.hpp; the four operations are float_arith.cpp and the
// class-3 answers are float_power.cpp.

#include "satellite_float/float_internal.hpp"
#include "satellite_float/satellite_float.hpp"

namespace satellite {

namespace floats {

Decimal decompose(const Number &magnitude)
{
    // to_string() is fixed notation until the padding would outnumber the
    // information, then `<digits>e<sign><exp>` -- satellite_number/render.cpp
    // owns the boundary and this parse accepts both sides of it, so a value
    // crossing the boundary cannot change what it decomposes to.
    const std::string text = magnitude.to_string();

    Decimal out;
    long long exponent = 0;
    long long int_digits = 0;
    bool seen_point = false;

    for (size_t i = 0; i < text.size(); ++i) {
        const char c = text[i];
        if (c == '-')
            continue; // a magnitude; the sign is the caller's
        if (c == '.') {
            seen_point = true;
            continue;
        }
        if (c == 'e' || c == 'E') {
            exponent = std::stoll(text.substr(i + 1));
            break;
        }
        out.digits.push_back(c);
        if (!seen_point)
            ++int_digits;
    }

    // Strip leading zeros, moving the point left past each one: "0.014" read
    // as digits "0014" with one integer digit is 0.digits x 10^1 until the
    // zeros come off the front.
    size_t lead = 0;
    while (lead < out.digits.size() && out.digits[lead] == '0')
        ++lead;
    out.digits.erase(0, lead);
    out.point = int_digits - static_cast<long long>(lead) + exponent;

    if (out.digits.empty())
        out.point = 0; // zero, canonically
    return out;
}

unsigned places_of(const Decimal &d)
{
    const long long below = static_cast<long long>(d.digits.size()) - d.point;
    return below > 0 ? static_cast<unsigned>(below) : 0;
}

Number compose(const Decimal &d)
{
    if (d.digits.empty())
        return Number();

    // Rebuild the text form and let parse() -- the one reader of numeric text
    // in the tree -- do the digits-to-limbs work. Exponent notation keeps the
    // string proportional to the digits even when `point` is far away.
    std::string text = d.digits;
    text += "e";
    text += std::to_string(d.point - static_cast<long long>(d.digits.size()));

    Number out;
    const bool parsed = Number::parse(text, out);
    // Unreachable: the text above is digits-e-integer by construction. Falls
    // back to zero rather than to an uninitialised read if it ever is not.
    return parsed ? out : Number();
}

Number truncate_places(const Number &magnitude, unsigned n)
{
    Decimal d = decompose(magnitude);
    if (places_of(d) <= n)
        return magnitude;

    const long long keep = d.point + static_cast<long long>(n);
    if (keep <= 0)
        return Number(); // everything is below the kept places
    d.digits.resize(static_cast<size_t>(keep));
    while (!d.digits.empty() && d.digits.back() == '0')
        d.digits.pop_back(); // recanonicalize: 0.50 truncated is 0.5
    if (d.digits.empty())
        d.point = 0;
    return compose(d);
}

Number place_step(unsigned n)
{
    return Number::from_small(true, 1, -static_cast<int>(n));
}

long long floor_log10(const Number &magnitude)
{
    // 0.D x 10^point with D's first digit non-zero puts the value in
    // [10^(point-1), 10^point), so the floor is point - 1. The one caller
    // contract is a POSITIVE magnitude; zero answers 0 and the digit budgets
    // built on it stay finite rather than trusting every caller's guard.
    const Decimal d = decompose(magnitude);
    return d.digits.empty() ? 0 : d.point - 1;
}

} // namespace floats

Float Float::from_number(const Number &value)
{
    const Number magnitude = value.abs();
    const Number whole = magnitude.floor();
    return assemble(!value.is_negative(), whole,
                    Number::sub(magnitude, whole));
}

Float Float::assemble(bool positive, const Number &whole,
                      const Number &fraction)
{
    Float out;
    // The halves are taken as magnitudes -- invariant 1 by construction
    // rather than by trust, so a signed Number handed in cannot smuggle a
    // second sign into a representation that holds it once.
    out.left_ = whole.abs();
    out.right_ = fraction.abs();

    // §8.6's normalize, step one: carry. Exact, and it cannot round. One
    // floor/sub pass settles any finite fraction, but the loop is the
    // invariant speaking -- the exit condition IS `R < 1`.
    while (Number::compare(out.right_, Number(1)) >= 0) {
        const Number carried = out.right_.floor();
        out.left_ = Number::add(out.left_, carried);
        out.right_ = Number::sub(out.right_, carried);
    }

    // Step two: invariant 3. There is no negative zero.
    out.positive_ = positive || out.is_zero();
    return out;
}

unsigned Float::places() const
{
    return floats::places_of(floats::decompose(right_));
}

Number Float::to_number() const
{
    const Number joined = Number::add(left_, right_);
    return positive_ ? joined : joined.negated();
}

std::string Float::to_string() const
{
    // sign, L, point, R's digits -- and AT LEAST ONE fractional digit, so a
    // whole float prints "4.0" and never impersonates the number 4. DESIGN
    // §1.1: the reader is told which type answered.
    std::string out;
    if (!positive_)
        out += '-';
    std::string whole = left_.to_string();
    if (whole.find('e') != std::string::npos) {
        // Number's renderer goes scientific when padding zeros would
        // outnumber information, which is right for a number standing alone
        // and wrong in the middle of `L.R` -- "1e+40.5" reads as nonsense.
        // An integer's decomposition has point >= digits, so writing the
        // zeros out is exact.
        const floats::Decimal d = floats::decompose(left_);
        whole = d.digits;
        whole.append(static_cast<size_t>(d.point) - d.digits.size(), '0');
    }
    out += whole;
    out += '.';

    const floats::Decimal fraction = floats::decompose(right_);
    if (fraction.digits.empty()) {
        out += '0';
        return out;
    }
    // 0.D x 10^point with point <= 0: the zeros between the decimal point and
    // the first significant digit are information here, not padding.
    for (long long i = fraction.point; i < 0; ++i)
        out += '0';
    out += fraction.digits;
    return out;
}

Float Float::negated() const
{
    Float out = *this;
    if (!out.is_zero())
        out.positive_ = !out.positive_;
    return out;
}

Float Float::abs() const
{
    Float out = *this;
    out.positive_ = true;
    return out;
}

Float Float::rounded_to(unsigned n) const
{
    const floats::Decimal d = floats::decompose(right_);
    if (floats::places_of(d) <= n)
        return *this;

    const Number kept = floats::truncate_places(right_, n);
    const Number dropped = Number::sub(right_, kept);

    // HALF AWAY FROM ZERO -- the rule, at the one place the float applies it.
    // The dropped tail is compared against half an ulp of the kept places;
    // at or above is away, below is toward. The sign was decided before this
    // ran and is not consulted: rounding a magnitude is what makes half-up
    // and half-away one motion (satellite_float.hpp's note).
    const Number half = Number::shift_right(floats::place_step(n), 1);
    const bool away = Number::compare(dropped, half) >= 0;

    // assemble() re-runs the carry, so 0.996 rounded to two places becomes
    // right 1.00 here and (L+1, 0.0) there.
    return assemble(positive_, left_,
                    away ? Number::add(kept, floats::place_step(n)) : kept);
}

int Float::compare(const Float &a, const Float &b)
{
    // The join is exact, so this IS §8.6's sign-then-L-then-R ordering, run
    // through the compare M8 already proved over signed values -- one
    // ordering, written once, true of both types.
    return Number::compare(a.to_number(), b.to_number());
}

} // namespace satellite
