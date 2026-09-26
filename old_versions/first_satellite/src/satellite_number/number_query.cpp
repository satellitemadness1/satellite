// Number: sign and integrality questions, rounding, comparison.
//
// Part of src/satellite_number/, split from an 855-line bignum.cpp.

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

    // The fast path is every index, every slice bound and every loop counter a
    // program actually writes.
    if (!big_ && exp_ == 0) {
        out = sig_;
        return true;
    }

    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (digits == "0") {
        out = 0;
        return true;
    }
    if (exponent > 32)      // far past anything a long long holds
        return false;

    BigInt mag = BigInt::from_digits(digits.data(), digits.size());
    if (exponent > 0)
        mag = BigInt::mul_pow10(mag, static_cast<unsigned>(exponent));
    if (!mag.fits_ll())
        return false;

    const long long value = mag.to_ll();
    out = sign() < 0 ? -value : value;
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
    // wrong one for .digits(), so the stripped zeros are added back here: a
    // positive exponent is exactly the run normalized() removed, and every one
    // of them is a digit position the written form has to fill.
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
    // BigInt's choice: §8.1 picked base 10^9 to fit a limb in 32 bits, and a
    // model that hard-coded the width would go quietly wrong if that ever moved.
    return big_ ? big_->limb_count() * sizeof(unsigned) : 0;
}

Number Number::abs() const
{
    Number out = *this;
    if (big_)
        out.sig_ = 1;
    else if (out.sig_ < 0)
        out.sig_ = -out.sig_;
    return out;
}

Number Number::negated() const
{
    Number out = *this;
    out.sig_ = -out.sig_;
    return out;
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

    // The fractional digits are exactly the last (-exponent) of the
    // significand, so dropping the fraction is a decimal-string split and the
    // three modes differ only in whether what was dropped bumps the magnitude.
    const size_t drop = static_cast<size_t>(-exponent);
    const int s = sign();

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
        bump = has_fraction && s < 0;
        break;
    case Rounding::Ceil:
        bump = has_fraction && s > 0;
        break;
    case Rounding::HalfAwayFromZero:
        bump = !fraction.empty() && fraction[0] >= '5';
        break;
    }

    BigInt mag = BigInt::from_digits(whole.data(), whole.size());
    if (bump)
        mag = BigInt::add(mag, BigInt::from_u64(1));
    return make(s, mag, 0);
}

int Number::compare(const Number &a, const Number &b)
{
    // The hot path: two small numbers at the same scale, which is every
    // `i < rounds` in every loop.
    if (!a.big_ && !b.big_ && a.exp_ == b.exp_)
        return a.sig_ < b.sig_ ? -1 : (a.sig_ > b.sig_ ? 1 : 0);

    const int sa = a.sign();
    const int sb = b.sign();
    if (sa != sb)
        return sa < sb ? -1 : 1;
    if (sa == 0)
        return 0;

    const int target = std::min(a.exp_, b.exp_);

    // Still small, just at different scales — `x < 2.5`. Aligning in a long
    // long keeps it allocation-free.
    if (!a.big_ && !b.big_) {
        long long left = 0;
        long long right = 0;
        if (scale_ll(a.sig_, a.exp_ - target, left) &&
            scale_ll(b.sig_, b.exp_ - target, right))
            return left < right ? -1 : (left > right ? 1 : 0);
    }

    const int c = BigInt::compare(a.scaled_magnitude(target),
                                  b.scaled_magnitude(target));
    return sa < 0 ? -c : c;
}

} // namespace satellite
