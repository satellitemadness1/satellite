#pragma once
// satellite/satellite_variable_binary/satellite_binary_number.hpp --
// `satellite.variable.binary` `1 6 5`. The author's name for the type, reserved
// as an arm of satelliteObject before it was built.
//
// (the author, 2026-09-16) "satellite.variable.binary my_number = b10101010 and
// "b" is the designation for binary". The b is how a binary is WRITTEN; the
// lexer takes it off (bytecode_registry.cpp) and binary_token is what remembers
// it, so the digits arrive here bare.
//
// THE WIDTH IS PART OF THE VALUE, which is the author's ruling from 003's M19.5
// (003 DESIGN 8.5) carried across: `b0010` and `b10` are two values, and a
// binary displays exactly as it was written, leading zeros and all. That clause
// is the one a representation can break without any other test noticing -- a
// number with nothing beside it passes every arithmetic check and still prints
// `b10` for `b0010` -- so the width is stored OUT LOUD beside the bits rather
// than inferred from them.
//
// THE BITS ARE A satellite_number, so a binary has no ceiling on its length
// (DESIGN 1.2) and everything that asks what it is WORTH -- arithmetic, `.number`,
// `.hex` -- reaches the number hub in one step with nothing re-parsed.
//
// A BINARY KEEPS A SIGN (the author, 2026-09-17: "give it a different number and
// keep a sign with all of these things a satellite.variable.bool with percentages
// and with infinities keep satellite.variable.bool with them"). -b0101 is a
// binary, not a refusal and not the number -5: it displays as -b0101, is worth -5,
// and keeps its width. The sign is the bool the bits' satellite_number already
// carries -- the author's own rule for that type, "the sign is carried as a bool
// with the object" -- so there is one bool and nothing to keep in step with it.
// negative() reads it. Zero is never negative, as for satellite_number: -b0000 is
// b0000. If the author wants the sign as a field of its own beside `width`, that
// is this struct and nothing that reads it.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {

struct satellite_binary_number {
    satellite_number bits;              // what the bits are worth, sign and all: -b0101 holds -5
    unsigned long long int width = 0;   // how many digits were written, leading zeros included

    // The sign, a bool held with the value: the one bits carries.
    bool negative() const { return bits.negative(); }

    // The same bits and width with the sign turned over: -b0101 from b0101.
    satellite_binary_number negated() const { return satellite_binary_number{-bits, width}; }

    // The digits of a literal, with no b on them. Only 0 and 1; anything else,
    // or no digits at all, is int_error (3) and `out` is untouched.
    static signed long long int from_digits(const std::string &digits, satellite_binary_number &out)
    {
        if (digits.empty())
            return int_error;
        for (const char c : digits)
            if (c != '0' && c != '1')
                return int_error;
        satellite_number worth;
        std::size_t bad_offset = 0;
        const signed long long int code = satellite_number::from_radix_text(digits, 2, worth, bad_offset);
        if (code != success)
            return code;
        out.bits = std::move(worth);
        out.width = digits.size();
        return success;
    }

    // The digits exactly as written, WITHOUT the b: "00101010", and "-0101" for
    // -b0101, the sign in front as a number's base 2 text has it. The leading
    // zeros come back from the width, which is the whole reason it is kept.
    std::string digits() const { return (negative() ? "-" : "") + unsigned_digits(); }

    // What `satellite.console.display` prints: the sign, the b and the digits,
    // exactly as the program wrote them.
    std::string written() const { return (negative() ? "-b" : "b") + unsigned_digits(); }

    // BITS, SIGN AND WIDTH, all (003 DESIGN 8.5): `b0010 == b10` is false, and so
    // is `-b0010 == b0010`.
    friend bool operator==(const satellite_binary_number &l, const satellite_binary_number &r)
    {
        return l.width == r.width && l.bits == r.bits;
    }

private:
    std::string unsigned_digits() const
    {
        std::string text = (negative() ? -bits : bits).to_radix_text(2);
        if (text.size() < width)
            text.insert(0, static_cast<std::size_t>(width - text.size()), '0');
        return text;
    }
};

} // namespace satellite004
