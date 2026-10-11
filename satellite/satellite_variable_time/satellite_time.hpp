#pragma once
// satellite/satellite_variable_time/satellite_time.hpp -- `satellite.variable.time` `1 6 3`, a
// length of time. Arm 21 of satelliteObject.
//
// (the author, 2026-10-10) "we need to build satellite.time.sleep(1s or 1ms or 1ns) since we are
// taking in 3 types, we don't really need to figure out what the minimum time is, if the user
// enters 1ns and the function takes longer than that, then... oh well, but we need to accept s or
// ms or ns as a time, and a time object of those lengths ... satellite.variable.time my_time =
// satellite.time.second(1) or satellite.time.microsecond(1) or satellite.time.nanosecond(1) and
// accept here s and ms and ns as aliases of those time spans"
//
// HELD AS A WHOLE NUMBER OF NANOSECONDS, in a satellite_number, the way a percentage is held in
// 10^-32 percents: 1s holds 1000000000, 1.5s holds 1500000000, 250ms holds 250000000. Every value
// is exact -- there is no binary fraction to turn 0.1s into 0.0999... -- and there is no ceiling,
// because a satellite_number has none. The nanosecond is the smallest of the three units, so it is
// the grain: a digit finer than a nanosecond is let go -- cut off toward zero, so 1.5ns is 1ns and
// -1.5ns is -1ns -- as 003 did ("a fraction of a nanosecond is not a wait anybody can observe").
//
// AND THE UNIT IT WAS WRITTEN IN, which is only how it is SHOWN: 250ms displays as 250ms, not as
// 0.25s. Two times are the same time when they are the same length, so 1s == 1000ms.
//
// A TIME KEEPS A SIGN, as a percentage, a binary and a hex do (the author, 2026-09-17: "keep a
// sign with all of these things"): -1s is a time, and satellite.time.sleep refuses to wait one.
//
// ms IS THE MILLISECOND -- MY READING, AND HIS TO OVERRULE. He wrote "microsecond" beside "ms" on
// 2026-10-10, and every other time he has meant a thousandth: his 003 `sleep(90, "ms")` was QUAD's
// sleep_for(milliseconds(90)), and v1's one time literal was `100ms`. If ms is to be the
// microsecond, places() answers 3 for it, its word is renamed (words_004.tsv 1 9 6, aliases.tsv), and
// named(), the sentences that spell a unit out and the help pages say microsecond -- the arithmetic
// is unchanged.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <string>
#include <string_view>

namespace satellite004 {

struct satellite_time {
    enum class Unit : unsigned char { second, millisecond, nanosecond };

    satellite_number nanoseconds;   // the length: 1s holds 1000000000
    Unit unit = Unit::second;       // how it is shown, and nothing more

    bool negative() const { return nanoseconds.negative(); }

    // HOW MANY DECIMAL PLACES A UNIT SITS ABOVE A NANOSECOND, so every conversion is one exact
    // multiplication by a power of ten -- 003's own arrangement for its sleep units.
    static unsigned int places(Unit unit)
    {
        switch (unit) {
        case Unit::second: return 9;
        case Unit::millisecond: return 6;
        case Unit::nanosecond: return 0;
        }
        return 0;
    }
    static const satellite_number &one(Unit unit)   // 10^places: one of the unit, in nanoseconds
    {
        static const satellite_number second(1000000000ull), millisecond(1000000ull), nanosecond(1ull);
        return unit == Unit::second ? second : unit == Unit::millisecond ? millisecond : nanosecond;
    }
    static const char *suffix(Unit unit)
    {
        return unit == Unit::second ? "s" : unit == Unit::millisecond ? "ms" : "ns";
    }
    static const char *named(Unit unit)
    {
        return unit == Unit::second ? "second" : unit == Unit::millisecond ? "millisecond" : "nanosecond";
    }

    // THE UNIT A LITERAL ENDS IN -- s, ms or ns -- and how many characters it takes. False for
    // anything else. The lexer asks it of the letters touching a number, and the walker of the
    // literal's own text, so the two cannot come to disagree about what 5ms is.
    static bool unit_at_the_end(std::string_view written, Unit &unit, std::size_t &length)
    {
        const auto ends = [&](std::string_view tail) {
            return written.size() > tail.size() && written.substr(written.size() - tail.size()) == tail;
        };
        if (ends("ms")) { unit = Unit::millisecond; length = 2; return true; }
        if (ends("ns")) { unit = Unit::nanosecond; length = 2; return true; }
        if (ends("s")) { unit = Unit::second; length = 1; return true; }
        return false;
    }

    // DIGITS IN A UNIT: "1", "1.5", "250", "-0.25" -- digits with at most one point, a digit on both
    // sides of it, a minus allowed in front (a float's own text has one). int_error (3) for anything
    // else, and `out` untouched. Digits finer than a nanosecond are let go, never refused.
    static signed long long int from_digits(const std::string &written, Unit unit, satellite_time &out)
    {
        const bool minus = !written.empty() && written[0] == '-';
        const std::string digits = minus ? written.substr(1) : written;
        const std::size_t point = digits.find('.');
        const std::string whole_part = point == std::string::npos ? digits : digits.substr(0, point);
        std::string fraction = point == std::string::npos ? std::string() : digits.substr(point + 1);
        if (whole_part.empty() || (point != std::string::npos && fraction.empty()))
            return int_error;
        for (const char c : whole_part + fraction)
            if (c < '0' || c > '9')
                return int_error;
        const unsigned int keep = places(unit);
        if (fraction.size() > keep)
            fraction.resize(keep);
        else
            fraction.append(keep - fraction.size(), '0');
        satellite_number value;
        std::size_t bad_offset = 0;
        const signed long long int code = satellite_number::from_text(whole_part + fraction, value, bad_offset);
        if (code != success)
            return code;
        if (minus && !value.is_zero())
            value = -value;
        out.nanoseconds = std::move(value);
        out.unit = unit;
        return success;
    }

    // A WHOLE NUMBER OF THE UNIT: satellite.time.second(5) is 5 times 10^9 nanoseconds.
    static satellite_time of_count(const satellite_number &count, Unit unit)
    {
        satellite_time out;
        out.nanoseconds = count * one(unit);
        out.unit = unit;
        return out;
    }

    // THE LENGTH IN ITS OWN UNIT: "1", "1.5", "-250", "0" -- every digit it holds, with the zeros
    // after the last one that matters left off.
    std::string to_text() const
    {
        const bool below_zero = negative();
        std::string digits = (below_zero ? -nanoseconds : nanoseconds).to_text();
        const unsigned int keep = places(unit);
        if (keep == 0)
            return (below_zero ? "-" : "") + digits;
        if (digits.size() <= keep)
            digits.insert(0, keep + 1 - digits.size(), '0');
        const std::string whole_part = digits.substr(0, digits.size() - keep);
        std::string fraction = digits.substr(digits.size() - keep);
        while (!fraction.empty() && fraction.back() == '0')
            fraction.pop_back();
        return (below_zero ? "-" : "") + whole_part + (fraction.empty() ? "" : "." + fraction);
    }

    // What satellite.console.display prints: the length and its unit, exactly as it is written --
    // 1s, 250ms, 1.5s, 30ns.
    std::string written() const { return to_text() + suffix(unit); }

    // THE SAME TIME WITH ITS SIGN TURNED OVER, for a minus written in front of one: -1s.
    satellite_time negated() const
    {
        satellite_time out = *this;
        if (!nanoseconds.is_zero())
            out.nanoseconds = -nanoseconds;
        return out;
    }

    // THE SAME LENGTH IS THE SAME TIME, whatever unit shows it: 1s == 1000ms.
    friend bool operator==(const satellite_time &l, const satellite_time &r)
    {
        return satellite_number::compare(l.nanoseconds, r.nanoseconds) == 0;
    }
};

} // namespace satellite004
