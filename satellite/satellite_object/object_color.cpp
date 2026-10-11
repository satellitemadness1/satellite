// satellite/satellite_object/object_color.cpp -- what a color answers when it
// meets something (object_color.hpp says why this is its own file).
//
// (the author, 2026-09-22) "satellite.variable.color my_color = 000000 which is
// just a hexadecimal number of a mandatory width, 6 digits".
//
// A COLOUR HAS NO ARITHMETIC AND NO ORDER, and both are refused by name:
//
//     c + x000001      refused     the author asked for a colour, not for sums on one
//     c < d            refused     is red before blue? Every answer is a guess
//     c == d           true/false  the same digits AND the same transparency
//     c.number         65280       what the six digits are worth, to do sums on
//
// `==` AND `!=` ON TWO COLOURS DO NOT COME HERE: expression.cpp's apply() asks
// color_same for them before it asks for an order, exactly as it does for two
// files and two lists -- one ordering answers all six spellings, and a colour has
// sameness without having an order. So color_compare refuses everything it is
// asked, and the only thing that reaches it with two colours is something that
// really wants an order, like sorting a list of them.
//
// THE OTHER NEW TYPES, A BINARY, A PERCENTAGE AND AN INFINITY ARE THE FINALE'S: a
// hex, a float or a fraction meeting a colour is "not built yet", which is true
// (the author is mixing them together later), and not "no scenario", which would
// not be. `c == x00FF00` is one of those: a hex meeting a colour.

#include "object_color.hpp"

#include <string>

namespace satellite004 {
namespace {

// THE FINALE'S PAIRS, said as that.
bool the_finale(const satelliteObject &value)
{
    return value.is_hexadecimal() || value.is_float() || value.is_fraction() || value.is_binary() ||
           value.is_percentage() || value.is_infinity();
}

// ONE SENTENCE FOR BOTH HALVES: `what` is "+" or "a comparison", and the reason
// is said after the pair, so a person reads which two things met first.
signed long long int refuse(const std::string &what, const satelliteObject &left, const satelliteObject &right,
                            const char *reason, std::string &why)
{
    const satelliteObject &other = left.is_color() ? right : left;
    why = what + " was given " + left.kind_name() + " and " + right.kind_name();
    if (!other.is_color() && the_finale(other)) {
        why += " -- " + std::string(other.kind_name()) +
               " meeting a color is not built yet: the new types are mixed together later";
        return not_built_yet;
    }
    why += reason;
    return types_do_not_meet;
}

} // namespace

// NOT ASKED, SO NOT GUESSED. Adding two colours could mean mixing paint, mixing
// light or adding the digits, and the author has named none of them -- so `+` and
// every other operator is refused, and the sentence says where the number is.
signed long long int color_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                     satelliteObject &, std::string &why)
{
    // `"my colour is " + c` IS JOINING, NOT ARITHMETIC, and is told so: the colour's
    // text is one method away.
    if (sign == '+' && (left.is_string() || right.is_string()))
        return refuse("+", left, right, " -- a color is not text; .string on it is its text, to join", why);
    return refuse(std::string(1, sign), left, right,
                  " -- a color has no arithmetic: it is six hex digits and a transparency. "
                  "Its .number is what the digits are worth, to do sums on",
                  why);
}

signed long long int color_compare(const satelliteObject &left, const satelliteObject &right, int &,
                                   std::string &why)
{
    // TWO COLOURS HERE WANT AN ORDER (a sort); anything else wanted sameness with
    // something that is not a colour. Each is told the half that is its own.
    const bool two_colors = left.is_color() && right.is_color();
    return refuse("a comparison", left, right,
                  two_colors ? " -- colors have no order, so only == and != compare them"
                             : " -- a color is compared only with another color, with == and !=",
                  why);
}

bool color_same(const satellite_color &left, const satellite_color &right)
{
    return left == right;
}

// "x00FF00", or "x00FF00, 50": exactly what display prints.
signed long long int color_to_string(const satellite_color &value, satellite_string &out, std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.written(), out, bad_offset);
}

// WHAT THE SIX DIGITS ARE WORTH: x00FF00 is 65280. The transparency is not part of
// the worth, as a hex's width is not -- a conversion keeps what the target can hold.
signed long long int color_to_number(const satellite_color &value, satellite_number &out, std::string &)
{
    out = satellite_number(static_cast<unsigned long long int>(value.rgb));
    return success;
}

// Twenty-four bits, four to the digit, the width kept.
signed long long int color_to_binary(const satellite_color &value, satellite_string &out, std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.bits(), out, bad_offset);
}

// THE SIX DIGITS, "00FF00", and not the number hub's "FF00": a colour is always six
// wide, so its own digits are the answer, as a hex's are.
signed long long int color_to_hexadecimal(const satellite_color &value, satellite_string &out, std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.digits(), out, bad_offset);
}

} // namespace satellite004
