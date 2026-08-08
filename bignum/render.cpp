// §8.1.1: print the VALUE, never N significant digits.
//
// Part of bignum/, split from an 855-line bignum.cpp.

#include "bignum_internal.hpp"

namespace satellite {

std::string Number::to_string() const
{
    // The hot path: an integer in the small representation, which is every
    // counter and every small literal, and the one this is asked for a million
    // times in a printing loop.
    if (!big_ && exp_ == 0)
        return std::to_string(sig_);

    std::string digits;
    long long exponent = 0;
    normalized(digits, exponent);
    if (digits == "0")
        return "0";

    const std::string sign = is_negative() ? "-" : "";

    // How many digits sit before the decimal point. The value's magnitude is in
    // [10^(point-1), 10^point).
    const long long point = static_cast<long long>(digits.size()) + exponent;

    // §8.1.1 fixed the rule as "print the value", and set the notation boundary
    // at [1e-6, 1e21) because outside it "digits stop being informative — 1e308
    // in fixed notation is 309 characters". That reasoning was written when a
    // number was a double, where a 33-digit integer HAD no 33 informative
    // digits. It does now, so the boundary is restated in the terms the
    // reasoning was always really about:
    //
    //     switch to scientific when the PADDING ZEROS would outnumber the
    //     information, never when the information itself is long.
    //
    // 1e308 is one significant digit and 308 zeros, so it is still scientific.
    // 30! is 26 significant digits and 7 zeros, so it prints whole — which
    // under the old phrasing it would not have, and an exact type that renders
    // an exact integer as 2.6525285981219105863630848e+32 is hiding the answer
    // it went to the trouble of computing.
    //
    // Both thresholds are the old boundaries restated, so every value that
    // printed fixed before still does: 20 trailing zeros is 1e20, and 5 leading
    // zeros is 1e-6.
    constexpr long long MAX_TRAILING_ZEROS = 20;
    constexpr long long MAX_LEADING_ZEROS = 5;

    const long long width = static_cast<long long>(digits.size());
    const bool fixed = point <= 0 ? -point <= MAX_LEADING_ZEROS
                                  : point - width <= MAX_TRAILING_ZEROS;
    if (fixed) {
        if (point <= 0)
            return sign + "0." + std::string(static_cast<size_t>(-point), '0') +
                   digits;
        if (static_cast<size_t>(point) >= digits.size())
            return sign + digits +
                   std::string(static_cast<size_t>(point) - digits.size(), '0');
        return sign + digits.substr(0, static_cast<size_t>(point)) + "." +
               digits.substr(static_cast<size_t>(point));
    }

    std::string out = sign + digits.substr(0, 1);
    if (digits.size() > 1)
        out += "." + digits.substr(1);

    const long long e = point - 1;
    out += "e";
    out += e < 0 ? "-" : "+";
    out += std::to_string(e < 0 ? -e : e);
    return out;
}

} // namespace satellite
