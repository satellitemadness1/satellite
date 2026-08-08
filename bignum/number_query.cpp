// Number: sign and integrality questions, rounding, comparison.
//
// Part of bignum/, split from an 855-line bignum.cpp.

#include "bignum_internal.hpp"

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
