#pragma once
// satellite/satellite_variable_hex/satellite_hexadecimal_number.hpp --
// `satellite.variable.hex` `1 6 11`, second spelling `satellite.variable.hexadecimal`
// (words/aliases.tsv). Arm 15 of satelliteObject.
//
// (the author, 2026-09-22) "similar thing for hex, which has an x in front of it,
// with hex numbers only for it 0 - 9 A - F". The x is how a hex is WRITTEN; the
// lexer takes it off (bytecode_registry.cpp) and hexadecimal_token is what
// remembers it, so the digits arrive here bare.
//
// THE BINARY'S SHAPE, IN BASE 16 (satellite_binary_number.hpp says why each field
// is there): what it is worth, sign and all, in a satellite_number, and the width
// as written beside it, because x00FF and xFF are two values that display
// differently -- 003 DESIGN 8.5, "the width is part of the value: x0009 is not x9".
//
// CASE IS NOT PART OF THE VALUE (003 DESIGN 8.5, the author's): x00ff and x00FF
// are one value, so the case is not stored and a hex always displays upper case
// -- which is also what `.hex` on a number has always answered ("FF"). The width
// is what carries meaning, and it is kept.
//
// A HEX KEEPS A SIGN, as a binary does (the author, 2026-09-17: "keep a sign with
// all of these things"). -x1F displays as -x1F, is worth -31, and keeps its width.
// The sign is the bool the worth's satellite_number already carries, so there is
// one bool and nothing to keep in step with it. Zero is never negative: -x00 is x00.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {

struct satellite_hexadecimal_number {
    satellite_number worth;              // what the digits are worth, sign and all: -x1F holds -31
    unsigned long long int width = 0;    // how many digits were written, leading zeros included

    // The sign, a bool held with the value: the one the worth carries.
    bool negative() const { return worth.negative(); }

    // The same digits and width with the sign turned over: -x1F from x1F.
    satellite_hexadecimal_number negated() const { return satellite_hexadecimal_number{-worth, width}; }

    // The digits of a literal, with no x on them. Only 0-9, A-F and a-f; anything
    // else, or no digits at all, is int_error (3) and `out` is untouched. NO CEILING
    // ON THE LENGTH: the worth is a satellite_number (DESIGN 1.2).
    static signed long long int from_digits(const std::string &digits, satellite_hexadecimal_number &out)
    {
        if (digits.empty())
            return int_error;
        for (const char c : digits)
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')))
                return int_error;
        satellite_number value;
        std::size_t bad_offset = 0;
        const signed long long int code = satellite_number::from_radix_text(digits, 16, value, bad_offset);
        if (code != success)
            return code;
        out.worth = std::move(value);
        out.width = digits.size();
        return success;
    }

    // The digits exactly as wide as written, WITHOUT the x and upper case: "00FF",
    // and "-1F" for -x1F, the sign in front as a number's base 16 text has it.
    std::string digits() const { return (negative() ? "-" : "") + unsigned_digits(); }

    // What `satellite.console.display` prints: the sign, the x and the digits.
    std::string written() const { return (negative() ? "-x" : "x") + unsigned_digits(); }

    // FOUR BITS TO THE DIGIT, the width kept (003 DESIGN 8.5: "hex.to_binary() can
    // never fail, every digit being exactly four bits" -- x00FF is 0000000011111111).
    // Without the b, as every `.binary` answer is; the sign in front.
    std::string bits() const
    {
        std::string text = (negative() ? -worth : worth).to_radix_text(2);
        const unsigned long long int wanted = width * 4;
        if (text.size() < wanted)
            text.insert(0, static_cast<std::size_t>(wanted - text.size()), '0');
        return (negative() ? "-" : "") + text;
    }

    // WORTH, SIGN AND WIDTH, all: `x00FF == xFF` is false, as `b0010 == b10` is.
    friend bool operator==(const satellite_hexadecimal_number &l, const satellite_hexadecimal_number &r)
    {
        return l.width == r.width && l.worth == r.worth;
    }

private:
    std::string unsigned_digits() const
    {
        std::string text = (negative() ? -worth : worth).to_radix_text(16);
        if (text.size() < width)
            text.insert(0, static_cast<std::size_t>(width - text.size()), '0');
        return text;
    }
};

} // namespace satellite004
