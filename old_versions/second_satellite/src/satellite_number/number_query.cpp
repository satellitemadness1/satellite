// Number: sign and integrality questions, rounding, comparison, and the three
// methods M8 added.
//
// Part of src/satellite_number/, split from the first satellite's 855-line
// bignum.cpp.

#include "satellite_number/bignum_internal.hpp"

namespace satellite {

bool Number::is_integer() const
{
    if (exp_ >= 0)
        return true;
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    return exponent >= 0 || digits == "0";
}

bool Number::to_integer(long long &out) const
{
    if (!is_integer())
        return false;

    unsigned long long mag = 0;
    if (!big_ && exp_ == 0) {
        // The fast path is every index, every slice bound and every loop
        // counter a program actually writes.
        mag = sig_;
    } else {
        std::string digits;
        long long exponent = 0;
        normalized(digits, exponent);
        if (digits == "0") {
            out = 0;
            return true;
        }
        if (exponent > 32)      // far past anything a long long holds
            return false;

        BigInt whole = BigInt::from_digits(digits.data(), digits.size());
        if (exponent > 0)
            whole = BigInt::mul_pow10(whole, static_cast<unsigned>(exponent));
        if (!whole.fits_u64())
            return false;
        mag = whole.to_u64();
    }

    // THE SIGNED RANGE IS NOT SYMMETRIC AND THIS IS WHERE THAT IS PAID FOR,
    // once, instead of in three places. v1 asked BigInt::fits_ll(), which
    // capped the magnitude at LLONG_MAX -- so `Number(LLONG_MIN).to_integer()`
    // came back FALSE, refusing to hand back the exact value its own
    // constructor had accepted. tests/number_test/sign.cpp asserts the round
    // trip, because a bug that only shows at one input is one a test with small
    // numbers never finds.
    constexpr unsigned long long kMostNegative =
        static_cast<unsigned long long>(LLONG_MAX) + 1;
    if (positive_) {
        if (mag > static_cast<unsigned long long>(LLONG_MAX))
            return false;
        out = static_cast<long long>(mag);
        return true;
    }
    if (mag > kMostNegative)
        return false;
    out = mag == kMostNegative ? LLONG_MIN : -static_cast<long long>(mag);
    return true;
}

size_t Number::digit_count() const
{
    // The digits the value is WRITTEN with, from its most significant down to
    // its least: 100 has three, 1230 has four, 12.5 has three, and zero has
    // one because "0" is a digit.
    //
    // normalized() answers a different question -- it strips trailing zeros,
    // because 2.50 and 2.5 are the same number and only one of them should
    // print -- so its count is the SIGNIFICAND's, and 100 comes back as 1e2
    // with a single digit. That is the right answer for rendering and the
    // wrong one here, so the stripped zeros are added back: a positive exponent
    // is exactly the run normalized() removed, and every one of them is a digit
    // position the written form has to fill.
    //
    // A negative exponent is the fractional side, where the digits normalized()
    // kept are already all of them -- the zeros in 0.001 sit BEFORE the first
    // significant digit, and a leading zero is padding, not a digit of the
    // value, so 0.001 has one.
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (exponent <= 0)
        return digits.size();
    return digits.size() + static_cast<size_t>(exponent);
}

size_t Number::payload_bytes() const
{
    // sizeof(unsigned) rather than a literal 4, because the limb type is
    // BigInt's choice: DESIGN §8.1 picked base 10^9 to fit a limb in 32 bits,
    // and a model that hard-coded the width would go quietly wrong if that ever
    // moved.
    return big_ ? big_->limb_count() * sizeof(unsigned) : 0;
}

Number Number::abs() const
{
    // ONE BOOL, which is what DESIGN §8.6 promised sign-magnitude would make
    // this. v1 had to reach into the significand and branch on whether the
    // magnitude was boxed.
    Number out = *this;
    out.positive_ = true;
    return out;
}

Number Number::negated() const
{
    // Zero has no sign to flip. Without this line negating zero would build the
    // one value DESIGN §8.6 invariant 3 forbids, from inside the module whose
    // make() exists to prevent it.
    if (is_zero())
        return *this;
    Number out = *this;
    out.positive_ = !out.positive_;
    return out;
}

// `1 6 4 2`, `1 6 4 3`, `1 6 4 5`. New at M8 -- the first satellite had none of
// the three.
//
// BY VALUE AND NOT BY const Number &, WHICH IS THE ONE PLACE THIS FILE REFUSES
// TO COPY std::max. A reference-returning max binds happily to a temporary and
// dangles the moment the full expression ends, which is the oldest trap in the
// standard library and one this language's own tie-breaker (DESIGN §1.1: do
// everything for the user, pay for it in performance rather than in their
// attention) says to buy out. It costs one shared_ptr increment, and only when
// the magnitude is boxed -- these are not on any hot path; add() is.
Number Number::max(const Number &a, const Number &b)
{
    return compare(a, b) < 0 ? b : a;
}

Number Number::min(const Number &a, const Number &b)
{
    return compare(a, b) > 0 ? b : a;
}

Number Number::clamp(const Number &a, const Number &low, const Number &high)
{
    // WHEN low IS ABOVE high THE ANSWER IS low, deterministically, and it is
    // written down rather than left to be discovered. std::clamp calls that
    // case undefined behaviour; this language does not have one of those, and
    // the alternative -- refusing -- needs a span, which is the caller's and
    // arrives with the path at M11.
    return max(low, min(a, high));
}

Number Number::floor() const { return rounded(Rounding::Floor); }
Number Number::ceil() const { return rounded(Rounding::Ceil); }
Number Number::round() const { return rounded(Rounding::HalfAwayFromZero); }

Number Number::rounded(Rounding mode) const
{
    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (exponent >= 0 || digits == "0")
        return *this;

    // Past this line the value has a fractional part, so it is not zero and
    // `positive_` IS its sign -- which is why nothing below calls sign().
    //
    // The fractional digits are exactly the last (-exponent) of the
    // significand, so dropping the fraction is a decimal-string split and the
    // three modes differ only in whether what was dropped bumps the magnitude.
    const size_t drop = static_cast<size_t>(-exponent);

    std::string whole;
    std::string fraction;
    if (drop >= digits.size()) {
        whole = "0";
        fraction = std::string(drop - digits.size(), '0') + digits;
    } else {
        whole = digits.substr(0, digits.size() - drop);
        fraction = digits.substr(digits.size() - drop);
    }

    const bool has_fraction =
        fraction.find_first_not_of('0') != std::string::npos;

    bool bump = false;
    switch (mode) {
    case Rounding::Floor:
        // Toward negative infinity, so it is the NEGATIVE side that grows:
        // floor(-2.5) is -3 and floor(2.5) is 2.
        bump = has_fraction && !positive_;
        break;
    case Rounding::Ceil:
        bump = has_fraction && positive_;
        break;
    case Rounding::HalfAwayFromZero:
        bump = !fraction.empty() && fraction[0] >= '5';
        break;
    }

    BigInt mag = BigInt::from_digits(whole.data(), whole.size());
    if (bump)
        mag = BigInt::add(mag, BigInt::from_u64(1));
    // ceil(-0.5) is zero and make() gives it a positive sign, which is the
    // invariant doing its job on the one operation that can reach it.
    return make(positive_, mag, 0);
}

int Number::compare(const Number &a, const Number &b)
{
    // The hot path: two small numbers at the same scale, which is every
    // `i < rounds` in every loop. v1 compared the significands directly,
    // because they carried the sign; a magnitude does not, so the flag is
    // consulted first and the magnitudes are compared under it.
    //
    // This IS DESIGN §8.6's comparison rule -- "compare `positive` first, a
    // true sorts above a false, reversing the result when both are negative" --
    // and invariant 3 is what makes it safe, since there is no -0 to be unequal
    // to 0. The float inherits this and does not write a second one.
    if (!a.big_ && !b.big_ && a.exp_ == b.exp_) {
        if (a.positive_ != b.positive_)
            return a.positive_ ? 1 : -1;
        const int c = a.sig_ < b.sig_ ? -1 : (a.sig_ > b.sig_ ? 1 : 0);
        return a.positive_ ? c : -c;
    }

    const int sa = a.sign();
    const int sb = b.sign();
    if (sa != sb)
        return sa < sb ? -1 : 1;
    if (sa == 0)
        return 0;

    const int target = std::min(a.exp_, b.exp_);

    // Still small, just at different scales -- `x < 2.5`. Aligning in an
    // unsigned long long keeps it allocation-free.
    if (!a.big_ && !b.big_) {
        unsigned long long left = 0;
        unsigned long long right = 0;
        if (scale_u64(a.sig_, a.exp_ - target, left) &&
            scale_u64(b.sig_, b.exp_ - target, right)) {
            const int c = left < right ? -1 : (left > right ? 1 : 0);
            return sa < 0 ? -c : c;
        }
    }

    const int c = BigInt::compare(a.scaled_magnitude(target),
                                  b.scaled_magnitude(target));
    return sa < 0 ? -c : c;
}

} // namespace satellite
