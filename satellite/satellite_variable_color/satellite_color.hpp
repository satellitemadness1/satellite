#pragma once
// satellite/satellite_variable_color/satellite_color.hpp -- `satellite.variable.color`
// `1 6 19`, second spelling `satellite.variable.colour` (words/aliases.tsv). Arm 16
// of satelliteObject.
//
// (the author, 2026-09-22) "satellite.variable.color my_color = 000000 which is
// just a hexadecimal number of a mandatory width, 6 digits"; earlier the same day:
// "satellite.variable.color my_color = x000000 or just 000000 without the x ...
// and then we'll add transparency in two ways, my_color.transparency(0 - 99) and
// my_color = x000000, 0-99 for transparency".
//
// A MACHINE INTEGER, AND THAT IS NOT A LIMIT. Six hex digits are 24 bits, always:
// the width is the type, the author's "mandatory width". Nothing a person can
// write makes a colour wider, so a satellite_number would only be slower.
//
// THE TRANSPARENCY IS HOW SEE-THROUGH IT IS, 0 TO 99, the author's range -- a range
// he gave the value, like the six digits, and not a ceiling on anything a person
// can write. 0 IS NOT SEE-THROUGH AT ALL AND 99 IS ALMOST CLEAR, which is OUR
// reading of "0 - 99": a colour written with no transparency is a solid colour, so
// the number a person never writes has to mean solid. If the author rules it the
// other way round (0 clear, 99 solid), a colour with nothing after it would have
// to start at 99 -- `transparency = 99` below -- and written() would leave off
// ", 99" instead of ", 0". Nothing else reads the number's meaning.
//
// A COLOUR HAS NO SIGN. A hex keeps one (-x1F), a colour does not: there is no
// colour below black. `-x000000` given to a colour name is refused by name.

#include <cstddef>
#include <string>

namespace satellite004 {

struct satellite_color {
    unsigned int rgb = 0;                // the six digits: 0xRRGGBB
    unsigned int transparency = 0;       // 0 to 99 (the author's range): 0 solid, 99 almost clear

    // THE AUTHOR'S TWO ENDS, named once so every refusal says the same range.
    static constexpr unsigned int kSolid = 0;
    static constexpr unsigned int kMostSeeThrough = 99;

    // EXACTLY SIX HEX DIGITS, no x on them, upper or lower case -- the author's
    // "mandatory width, 6 digits". Anything else is false and `out` is untouched.
    static bool from_digits(const std::string &digits, satellite_color &out)
    {
        if (digits.size() != 6)
            return false;
        unsigned int value = 0;
        for (const char c : digits) {
            unsigned int digit = 0;
            if (c >= '0' && c <= '9') digit = static_cast<unsigned int>(c - '0');
            else if (c >= 'A' && c <= 'F') digit = static_cast<unsigned int>(c - 'A' + 10);
            else if (c >= 'a' && c <= 'f') digit = static_cast<unsigned int>(c - 'a' + 10);
            else return false;
            value = value * 16 + digit;
        }
        out = satellite_color{value, kSolid};
        return true;
    }

    // THE SIX DIGITS, upper case and no x: "00FF00". Case is not part of the value,
    // as it is not for a hex (003 DESIGN 8.5), so ff00aa shows as FF00AA.
    std::string digits() const
    {
        static const char kDigit[] = "0123456789ABCDEF";
        std::string text(6, '0');
        for (int at = 5, value = static_cast<int>(rgb); at >= 0; --at, value >>= 4)
            text[static_cast<std::size_t>(at)] = kDigit[value & 0xF];
        return text;
    }

    // WHAT `satellite.console.display` PRINTS, IN THE AUTHOR'S OWN SPELLING: x00FF00,
    // and x00FF00, 50 when it is see-through at all -- his "my_color = x000000, 0-99
    // for transparency", so a colour displayed reads straight back into a colour
    // name. A solid colour leaves the ", 0" off, as he writes it.
    std::string written() const
    {
        return "x" + digits() + (transparency == kSolid ? std::string() : ", " + std::to_string(transparency));
    }

    // TWENTY-FOUR BITS, FOUR TO THE DIGIT, the width kept -- a hex's `.binary` rule
    // (003 DESIGN 8.5: "every digit being exactly four bits"). No b, as every
    // `.binary` answer has none.
    std::string bits() const
    {
        std::string text(24, '0');
        for (int at = 23, value = static_cast<int>(rgb); at >= 0; --at, value >>= 1)
            text[static_cast<std::size_t>(at)] = (value & 1) ? '1' : '0';
        return text;
    }

    // THE SAME COLOUR IS THE SAME DIGITS AND THE SAME TRANSPARENCY: x00FF00 and
    // x00FF00, 50 are two colours, because one of them lets the screen through.
    friend bool operator==(const satellite_color &l, const satellite_color &r)
    {
        return l.rgb == r.rgb && l.transparency == r.transparency;
    }
};

} // namespace satellite004
