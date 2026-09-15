// Number: construction, the small form, parsing, normalisation.
//
// Part of src/satellite_number/, split from the first satellite's 855-line
// bignum.cpp. THIS IS THE FILE THE SIGN CHANGE LANDED IN: v1's sign() read the
// significand and its make() took an int, and DESIGN §8.1 says the magnitude
// never carries a sign. Every function below that lost code lost it here.

#include "satellite_number/bignum_internal.hpp"

namespace satellite {

namespace {
bool is_digit(char c)
{
    return c >= '0' && c <= '9';
}
} // namespace

BigInt Number::magnitude() const
{
    // Three lines in v1, and two of them were about LLONG_MIN: the significand
    // was signed, so the magnitude had to be taken in unsigned to have anywhere
    // to land. It is unsigned already.
    return big_ ? *big_ : BigInt::from_u64(sig_);
}

BigInt Number::scaled_magnitude(int target) const
{
    const BigInt mag = magnitude();
    if (exp_ == target || mag.is_zero())
        return mag;
    return BigInt::mul_pow10(mag, static_cast<unsigned>(exp_ - target));
}

Number Number::small(bool positive, unsigned long long sig, int exp)
{
    Number out;
    if (sig == 0)
        return out;   // zero is positive and carries no exponent
    out.sig_ = sig;
    out.exp_ = exp;
    out.positive_ = positive;
    return out;
}

Number Number::make(bool positive, const BigInt &magnitude, long long exponent)
{
    Number out;

    // WHERE "THERE IS NO NEGATIVE ZERO" IS ENFORCED, and it is an early return
    // rather than a check. Every path in this module that builds a value from a
    // magnitude comes through here or through small() above, and both answer a
    // zero magnitude with a default-constructed Number -- which is positive,
    // because DESIGN §8.1 defaults the flag to true. So -0 is not normalised
    // away at comparison time; it is a state the representation cannot reach.
    if (magnitude.is_zero())
        return out;

    // The exponent is an int, by the layout bignum.hpp measured. Reaching
    // either end of it costs about two gigabytes of digits, so what stops a
    // program getting there is M6's watchdog rather than anything here -- and
    // this clamps rather than wraps, because being wrong at 10^2147483647 is a
    // trade worth making and a wrapped exponent is a value that is quietly
    // enormous instead of quietly saturated.
    //
    // IT USED TO SAY "a division by division_digits" was one of the bounds.
    // That stopped being true on 2026-08-31, when the division's ceiling was
    // deleted for being the kind of limit DESIGN §7.5 forbids; the sentence is
    // now about memory, which is what §7.5 says the bound always is.
    if (exponent > INT_MAX)
        exponent = INT_MAX;
    if (exponent < INT_MIN)
        exponent = INT_MIN;
    out.exp_ = static_cast<int>(exponent);
    out.positive_ = positive;

    if (magnitude.fits_u64()) {
        out.sig_ = magnitude.to_u64();
        return out;
    }
    // sig_ STAYS ZERO WHEN THE MAGNITUDE IS BOXED. v1 put the sign here, which
    // meant a caller reading the field without checking big_ saw 1 or -1 as the
    // value of every large number. Both mistakes are mistakes; this one reads
    // the same every time and cannot be confused for a plausible small value.
    // small_parts() is the sanctioned window and it checks big_ first.
    out.big_ = std::make_shared<const BigInt>(magnitude);
    return out;
}

bool Number::small_parts(bool &positive, unsigned long long &sig, int &exp) const
{
    // The one test that matters, and the reason this is a member.
    if (big_)
        return false;
    positive = positive_;
    sig = sig_;
    exp = exp_;
    return true;
}

Number Number::from_small(bool positive, unsigned long long sig, int exp)
{
    // v1 needed a paragraph here about LLONG_MIN -- it takes a caller-supplied
    // integer, so it could be handed a value make() would never produce, and
    // negating that one is undefined. An unsigned significand cannot be handed
    // one, so what is left is the two rules small() already applies.
    return small(positive, sig, exp);
}

Number Number::from_u64(unsigned long long value)
{
    return small(true, value, 0);
}

bool Number::parse(const std::string &text, Number &out)
{
    size_t i = 0;
    bool positive = true;
    if (i < text.size() && (text[i] == '+' || text[i] == '-')) {
        positive = text[i] == '+';
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

    // "-0" PARSES TO ZERO AND NOT TO A NEGATIVE ZERO, and it costs nothing to
    // say so: make() drops the sign with the magnitude. A program is allowed to
    // write it and the language is not allowed to keep it.
    out = make(positive, BigInt::from_digits(digits.data(), digits.size()),
               exponent);
    return true;
}

void Number::normalized(std::string &digits, long long &exponent) const
{
    digits = big_ ? big_->to_digits() : std::to_string(sig_);
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
