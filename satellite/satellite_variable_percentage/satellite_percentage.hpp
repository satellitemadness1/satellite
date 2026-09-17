#pragma once
// satellite/satellite_variable_percentage/satellite_percentage.hpp --
// `satellite.variable.percentage` `1 6 16`, the first word 004 added of its own.
//
// (the author, 2026-09-17) "build a satellite.variable.percentage that rounds up
// to 100.0000000000000000000000000000000 and only keeps that many digits... let's
// just keep 32 digit precision on the percentage we will keep the infinity at 4096
// precision but take 1% and 100% as meaning a percentage and 1000000000000% as
// something you can multiply by"
//
// THIRTY-TWO DIGITS AFTER THE POINT. The author's example has 31 zeros and "32
// digit precision" says 32, and 32 is read as the digits after the point, not the
// digits in all -- so 1000000000000% keeps exactly as many as 50% does. If the
// ruling is 32 in all, kDigits is the one number that changes, and the rounding
// moves with it.
//
// HELD AS A WHOLE NUMBER OF 10^-32 PERCENTS, in a satellite_number, so every value
// is exact: 12.5% is 125 followed by 31 zeros, and there is no binary fraction
// anywhere to turn 0.1% into 0.0999... There is no ceiling above either -- the
// scaled value is a satellite_number, so 1000000000000% and far larger are only
// more limbs.
//
// ROUNDED HALF AWAY FROM ZERO at the 33rd digit, which is "rounds up" the way a
// person means it: a 33rd digit of 5 or more carries into the 32nd, 4 or less is
// dropped, and a negative percentage rounds the same distance the other way.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {

struct satellite_percentage {
    static constexpr unsigned int kDigits = 32;   // the author: "let's just keep 32 digit precision"

    satellite_number scaled;   // the percentage times 10^32: 50% holds 50 * 10^32

    // 10^32, which is 1% scaled -- and 100 of them, which is 100%, the whole.
    static const satellite_number &unit()
    {
        static const satellite_number value =
            satellite_number(10000000000000000ull) * satellite_number(10000000000000000ull);
        return value;
    }
    static const satellite_number &whole()
    {
        static const satellite_number value = unit() * satellite_number(100ull);
        return value;
    }

    // THE DIGITS BEFORE THE %, as the lexer hands them over: "50", "12.5",
    // "1000000000000". Digits with at most one point, a digit on both sides of it;
    // anything else is int_error (3) and `out` is untouched. More than 32 digits
    // after the point are rounded, never refused -- the author asked for rounding.
    static signed long long int from_digits(const std::string &digits, satellite_percentage &out)
    {
        const std::size_t point = digits.find('.');
        std::string whole_part = point == std::string::npos ? digits : digits.substr(0, point);
        std::string fraction = point == std::string::npos ? std::string() : digits.substr(point + 1);
        if (whole_part.empty() || (point != std::string::npos && fraction.empty()))
            return int_error;
        for (const char c : whole_part + fraction)
            if (c < '0' || c > '9')
                return int_error;

        const bool carries = fraction.size() > kDigits && fraction[kDigits] >= '5';
        if (fraction.size() > kDigits)
            fraction.resize(kDigits);
        else
            fraction.append(kDigits - fraction.size(), '0');

        satellite_number value;
        std::size_t bad_offset = 0;
        const signed long long int code = satellite_number::from_text(whole_part + fraction, value, bad_offset);
        if (code != success)
            return code;
        if (carries)
            value += satellite_number(1ull);
        out.scaled = std::move(value);
        return success;
    }

    // numerator / denominator, ROUNDED HALF AWAY FROM ZERO -- the one rounding every
    // percentage answer goes through. The denominator is never 0 here; the callers
    // answer division_by_zero before they get this far.
    static satellite_number rounded_divide(const satellite_number &numerator, const satellite_number &denominator)
    {
        satellite_number quotient, remainder;
        satellite_number::divide(numerator, denominator, quotient, remainder);
        satellite_number twice = remainder + remainder;
        if (twice.negative())
            twice = -twice;
        const satellite_number size = denominator.negative() ? -denominator : denominator;
        if (satellite_number::compare(twice, size) >= 0) {
            if (numerator.negative() != denominator.negative())
                quotient -= satellite_number(1ull);
            else
                quotient += satellite_number(1ull);
        }
        return quotient;
    }

    // numerator / denominator when that is a WHOLE number, which is what a
    // satellite.variable.number can hold: answer_is_not_whole (24) otherwise, and
    // `out` untouched. 3 * 50% is 1.5, and 1.5 waits for satellite_float.
    static signed long long int whole_divide(const satellite_number &numerator, const satellite_number &denominator,
                                             satellite_number &out)
    {
        satellite_number quotient, remainder;
        const signed long long int code = satellite_number::divide(numerator, denominator, quotient, remainder);
        if (code != success)
            return code;
        if (!remainder.is_zero())
            return answer_is_not_whole;
        out = std::move(quotient);
        return success;
    }

    // "50", "12.5", "-33.33333333333333333333333333333333": every digit it holds,
    // with the zeros after the last one that matters left off.
    std::string to_text() const
    {
        const bool below_zero = scaled.negative();
        std::string digits = (below_zero ? -scaled : scaled).to_text();
        if (digits.size() <= kDigits)
            digits.insert(0, kDigits + 1 - digits.size(), '0');
        const std::string whole_part = digits.substr(0, digits.size() - kDigits);
        std::string fraction = digits.substr(digits.size() - kDigits);
        while (!fraction.empty() && fraction.back() == '0')
            fraction.pop_back();
        return (below_zero ? "-" : "") + whole_part + (fraction.empty() ? "" : "." + fraction);
    }

    // What satellite.console.display prints: the number and its %.
    std::string written() const { return to_text() + "%"; }

    friend bool operator==(const satellite_percentage &l, const satellite_percentage &r) { return l.scaled == r.scaled; }
};

} // namespace satellite004
