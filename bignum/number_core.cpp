// Number: construction, the small form, parsing, normalisation.
//
// Part of bignum/, split from an 855-line bignum.cpp.

#include "bignum_internal.hpp"

namespace satellite {

namespace {
bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}
} // namespace

int Number::sign() const
{
    if (big_)
        return sig_ < 0 ? -1 : 1;
    return sig_ < 0 ? -1 : (sig_ > 0 ? 1 : 0);
}

BigInt Number::magnitude() const
{
    if (big_)
        return *big_;
    // Taken in unsigned, because -LLONG_MIN has no long long to land in. The
    // constructor keeps LLONG_MIN out of sig_, so this is belt and braces
    // rather than the load-bearing guard.
    const unsigned long long wide =
        sig_ < 0 ? 0ULL - static_cast<unsigned long long>(sig_)
                 : static_cast<unsigned long long>(sig_);
    return BigInt::from_u64(wide);
}

BigInt Number::scaled_magnitude(int target) const
{
    const BigInt mag = magnitude();
    if (exp_ == target || mag.is_zero())
        return mag;
    return BigInt::mul_pow10(mag, static_cast<unsigned>(exp_ - target));
}

Number Number::make(int sign, const BigInt &magnitude, long long exponent)
{
    Number out;
    if (magnitude.is_zero())
        return out;   // zero has no sign, and no exponent worth keeping

    // The exponent is an int, by §8.1's 32-byte layout. Nothing a program can
    // write reaches the ends of it — a literal is bounded by the length of the
    // source and a division by division_digits — so this clamps rather than
    // wraps, and being wrong at 10^2147483647 is a trade worth making.
    if (exponent > INT_MAX)
        exponent = INT_MAX;
    if (exponent < INT_MIN)
        exponent = INT_MIN;
    out.exp_ = static_cast<int>(exponent);

    if (magnitude.fits_ll()) {
        const long long value = magnitude.to_ll();
        out.sig_ = sign < 0 ? -value : value;
        return out;
    }
    out.sig_ = sign;
    out.big_ = std::make_shared<const BigInt>(magnitude);
    return out;
}

bool Number::small_parts(long long &sig, int &exp) const
{
    // The one test that matters. Above, make() stores the SIGN in sig_ when it
    // stores a BigInt, so a caller reading sig_ without checking big_ would get
    // 1 or -1 as the value of every large number.
    if (big_)
        return false;
    sig = sig_;
    exp = exp_;
    return true;
}

Number Number::from_small(long long sig, int exp)
{
    // LLONG_MIN has no positive counterpart, so negated() on it is undefined.
    // The integral constructor sends it big for exactly this reason, and this
    // function has to agree: it takes a caller-supplied long long, so it can be
    // handed a value make() would never produce (fits_ll() caps the magnitude
    // at LLONG_MAX, so make() cannot).
    //
    // Being the inverse of small_parts is not enough here. Round-tripping is
    // preserved either way, and the invariant that every other operation relies
    // on — that sig_ can be negated — is only preserved by promoting.
    if (sig == LLONG_MIN)
        return make(-1,
                    BigInt::from_u64(static_cast<unsigned long long>(LLONG_MAX) + 1),
                    exp);

    Number out;
    out.sig_ = sig;
    // Zero carries no exponent. make() drops it ("zero has no sign, and no
    // exponent worth keeping"), so a from_small that kept one would build a
    // Number that no other path can produce, and compare() and to_string()
    // would both then be reasoning about a shape they have never seen.
    out.exp_ = sig == 0 ? 0 : exp;
    return out;
}

Number Number::from_u64(unsigned long long value)
{
    return make(1, BigInt::from_u64(value), 0);
}

bool Number::parse(const std::string &text, Number &out)
{
    size_t i = 0;
    int sign = 1;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        sign = text[i] == '-' ? -1 : 1;
        i++;
    }

    // The digits of the significand, with the decimal point turned into an
    // exponent: "3.14" is 314 times 10^-2, which is already the representation.
    // Nothing is converted and nothing is rounded, which is the whole point.
    std::string digits;
    long long exponent = 0;
    bool any = false;

    for (; i < text.size() && is_digit(text[i]); i++, any = true)
        digits += text[i];

    if (i < text.size() && text[i] == '.') {
        i++;
        for (; i < text.size() && is_digit(text[i]); i++, any = true) {
            digits += text[i];
            exponent--;
        }
    }
    if (!any)
        return false;

    if (i < text.size() && (text[i] == 'e' || text[i] == 'E')) {
        i++;
        int exp_sign = 1;
        if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
            exp_sign = text[i] == '-' ? -1 : 1;
            i++;
        }
        if (i >= text.size() || !is_digit(text[i]))
            return false;
        long long value = 0;
        for (; i < text.size() && is_digit(text[i]); i++) {
            value = value * 10 + (text[i] - '0');
            if (value > INT_MAX)
                value = INT_MAX;      // make() clamps the total anyway
        }
        exponent += exp_sign * value;
    }
    if (i != text.size())
        return false;

    out = make(sign, BigInt::from_digits(digits.data(), digits.size()),
               exponent);
    return true;
}

void Number::normalized(std::string &digits, long long &exponent) const
{
    if (big_) {
        digits = big_->to_digits();
    } else {
        const unsigned long long wide =
            sig_ < 0 ? 0ULL - static_cast<unsigned long long>(sig_)
                     : static_cast<unsigned long long>(sig_);
        digits = std::to_string(wide);
    }
    exponent = exp_;

    if (digits == "0") {
        exponent = 0;
        return;
    }

    // Trailing zeros are representation, not value: 2.50 and 2.5 are the same
    // number and only one of them should print. Stripping here rather than
    // after every operation is what keeps add and multiply allocation-free.
    size_t end = digits.size();
    while (end > 1 && digits[end - 1] == '0') {
        end--;
        exponent++;
    }
    digits.resize(end);
}

} // namespace satellite
