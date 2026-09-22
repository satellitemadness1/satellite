// satellite/satellite_object/object_hexadecimal.cpp -- what a hex answers when it
// meets something (object_hexadecimal.hpp says why this is its own file).
//
// (the author, 2026-09-22) "we are just building the individual
// satellite.variable.binary/bin my_number = b110101010100101 ... similar thing for
// hex". The individual type, and not yet what it does beside the others.
//
// A HEX IS READ BY WHAT ITS DIGITS ARE WORTH, and an arithmetic answer is a NUMBER
// -- exactly what a binary does (satellite_object.cpp's read_by_worth says why at
// length), and exactly what every x literal did before the hex was a type of its
// own, when x1F WAS the number 31. So making it a type changed what a hex DISPLAYS
// and nothing it computes:
//
//     x1F + 1        32            a hex and a number, by worth
//     x1F == 31      true          ... and ordered by worth
//     x10 * x10      256           two hexes, by worth, answering a number
//     x00FF == xFF   false         two hexes compare by worth AND width
//     b1100 + xFF    267           a binary and a hex, by worth -- see below
//
// 003 DESIGN 8.5 RECORDS `+` ON TWO BIT RUNS AS DECIDED AND UNBUILT: it ADDS, the
// width grows to fit, and with mixed types the left operand's type wins. That is
// the author's "grand finale" for these types, and when it is built the worth path
// below wraps its answer back up as a hex; nothing else here moves.
//
// A BINARY MEETS A HEX BY WORTH, AND THAT IS KEPT RATHER THAN REFUSED, on purpose.
// The brief for this run said a hex meeting a binary waits for the finale -- but
// `b1100 + xFF` is 267 in tests/arithmetic.satl and check.sh pins it, and the rule
// above that one is that no x literal that works today stops working. So the pair
// answers what it always did. If the author rules that it refuses until the
// finale, it is the one `|| right.is_binary()` in by_worth() below.
//
// THE OTHER NEW TYPES, A PERCENTAGE AND AN INFINITY ARE REFUSED, BY NAME, as the
// finale's: a float, a colour, a fraction, a percentage or an infinity meeting a
// hex is "not built yet", which is true, and not "no scenario", which would not be.

#include "object_hexadecimal.hpp"

#include <utility>

namespace satellite004 {
namespace {

// A NUMBER, A BINARY OR A HEX: the three that meet by worth.
bool by_worth(const satelliteObject &value)
{
    return value.is_hexadecimal() || value.is_number() || value.is_binary();
}

satelliteObject worth_of(const satelliteObject &value)
{
    if (const satellite_hexadecimal_number *hex = value.as_hexadecimal())
        return satelliteObject::of_number(hex->worth);
    if (const satellite_binary_number *bits = value.as_binary())
        return satelliteObject::of_number(bits->bits);
    return value;
}

// THE FINALE'S PAIRS, said as that. Anything else a hex meets is a pair with no
// scenario at all, and is told so in satellite_object.cpp's own words.
bool the_finale(const satelliteObject &value)
{
    return value.is_float() || value.is_color() || value.is_fraction() || value.is_percentage() ||
           value.is_infinity();
}

signed long long int refuse(const char *what, const satelliteObject &left, const satelliteObject &right,
                            std::string &why)
{
    const satelliteObject &other = left.is_hexadecimal() ? right : left;
    if (the_finale(other)) {
        why = std::string(what) + " was given " + left.kind_name() + " and " + right.kind_name() + " -- " +
              other.kind_name() + " meeting a hex is not built yet: the new types are mixed together later";
        return not_built_yet;
    }
    why = std::string(what) + " was given " + left.kind_name() + " and " + right.kind_name() +
          ", and there is no scenario for that pair";
    return types_do_not_meet;
}

} // namespace

signed long long int hexadecimal_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                           satelliteObject &out, std::string &why)
{
    const char spelled[2] = {sign, '\0'};
    if (!by_worth(left) || !by_worth(right))
        return refuse(spelled, left, right, why);

    // TWO NUMBERS NOW, and the number's own arithmetic answers -- the same answer,
    // and the same refusal (a division by zero, a result that is not whole), that
    // the x literal met when it was a number.
    const satelliteObject left_worth = worth_of(left);
    const satelliteObject right_worth = worth_of(right);
    switch (sign) {
    case '+': return left_worth.add(right_worth, out, why);
    case '-': return left_worth.subtract(right_worth, out, why);
    case '*': return left_worth.multiply(right_worth, out, why);
    case '/': return left_worth.divide(right_worth, out, why);
    case '%': return left_worth.modulus(right_worth, out, why);
    case '^': return left_worth.power(right_worth, out, why);
    default: break;
    }
    why = std::string(spelled) + " is not an operation a hex has";
    return types_do_not_meet;
}

// BY WORTH, AND THEN BY WIDTH, for two hexes -- the binary's rule, and for its
// reason: one integer answers all six comparisons, and it cannot be 0 for two
// values that are not equal, so `x00FF == xFF` is false and `xFF < x00FF`.
// A hex against a number or a binary is by worth alone: `x1F == 31` is true.
signed long long int hexadecimal_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                         std::string &why)
{
    const satellite_hexadecimal_number *left_hex = left.as_hexadecimal();
    const satellite_hexadecimal_number *right_hex = right.as_hexadecimal();
    if (left_hex != nullptr && right_hex != nullptr) {
        order = satellite_number::compare(left_hex->worth, right_hex->worth);
        if (order == 0 && left_hex->width != right_hex->width)
            order = left_hex->width < right_hex->width ? -1 : 1;
        return success;
    }
    if (!by_worth(left) || !by_worth(right))
        return refuse("a comparison", left, right, why);
    order = satellite_number::compare(*worth_of(left).as_number(), *worth_of(right).as_number());
    return success;
}

bool hexadecimal_same(const satellite_hexadecimal_number &left, const satellite_hexadecimal_number &right)
{
    return left == right;
}

// "x00FF": exactly what display prints.
signed long long int hexadecimal_to_string(const satellite_hexadecimal_number &value, satellite_string &out,
                                           std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.written(), out, bad_offset);
}

// What the digits are worth; the width does not survive, as it does not for a
// binary (003 DESIGN 8.5: "No conversion to a satellite.variable.number can keep a
// leading zero").
signed long long int hexadecimal_to_number(const satellite_hexadecimal_number &value, satellite_number &out,
                                           std::string &)
{
    out = value.worth;
    return success;
}

// Four bits to the digit, the width kept: "0000000011111111" for x00FF.
signed long long int hexadecimal_to_binary(const satellite_hexadecimal_number &value, satellite_string &out,
                                           std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.bits(), out, bad_offset);
}

// ITS OWN DIGITS, THE WIDTH KEPT -- "00FF" -- and not the hub's "FF". This is the
// binary's `.bin` rule (object_convert.cpp: a type's own text does not go round
// the number, "because the hub is a number and a number has no width").
signed long long int hexadecimal_to_hexadecimal(const satellite_hexadecimal_number &value, satellite_string &out,
                                                std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(value.digits(), out, bad_offset);
}

} // namespace satellite004
