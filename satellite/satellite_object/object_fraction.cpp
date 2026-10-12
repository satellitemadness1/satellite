// satellite/satellite_object/object_fraction.cpp -- what a fraction answers when it
// meets something (object_fraction.hpp says why this is its own file).
//
// (the author, 2026-09-22) "let's just start with satellite.variable.fraction
// my_number = satellite_number1/satellite_number2 simply two numbers that are tied
// together, and when the user declares them, we will automatically make them
// floats with decimal points, but display them as just the whole number".
//
// WHAT ANSWERS, AND WHAT IT ANSWERS:
//
//     1/3            1/3       two floats, each shown as the whole number it is
//     2/4            2/4       tied, not simplified: kept as it was written
//     2/4 == 1/2     true      but compared BY VALUE, by cross-multiplying
//     1/3 < 1/2      true      the same ordering, six spellings
//     1/2 == 1       false     a plain number is read as itself over 1
//     6 / 4          6/4       two numbers divided, kept as written (the author, 2026-10-12:
//                              "what about always a fraction? 4 / 2 is the fraction")
//
// AND WHAT DOES NOT, YET: + - * / % ^ with a fraction on either side. "let's just
// start with" is two numbers tied together, so fraction arithmetic is refused by
// name rather than guessed at. A fraction meeting a float, a hex, a colour, an
// infinity, a percentage or a binary is the author's "grand finale", the new types
// mixed together, and says so.

#include "object_fraction.hpp"

#include "number_and_number_divide_rounded.hpp"
#include "../satellite_variable_float/float_scaled.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

// A FRACTION, OR A PLAIN NUMBER READ AS ONE OVER 1 -- the two kinds a fraction is
// compared with today. nullptr for anything else. `made` holds the number's
// fraction, so a fraction that is already one is never copied to be looked at.
const satellite_fraction *fraction_of(const satelliteObject &value, satellite_fraction &made)
{
    if (const satellite_fraction *held = value.as_fraction())
        return held;
    if (const satellite_number *held = value.as_number()) {
        made = fraction_of_number(*held);
        return &made;
    }
    return nullptr;
}

// A value as a person wrote it, for a refusal: 1/3, 3, or what kind it is.
std::string shown(const satelliteObject &value)
{
    if (const satellite_fraction *held = value.as_fraction())
        return fraction_written(*held);
    if (const satellite_number *held = value.as_number())
        return held->to_text();
    return value.kind_name();
}

// THE PAIRS THAT WAIT FOR THE FINALE, and the sentence that says so. Anything
// else a fraction meets has no scenario at all, and is named as that pair.
signed long long int refuse_meeting(const std::string &what, const satelliteObject &left,
                                    const satelliteObject &right, std::string &why)
{
    const satelliteObject &other = left.is_fraction() ? right : left;
    const std::string both = what + " was given " + left.kind_name() + " and " + right.kind_name();
    if (other.is_float() || other.is_hexadecimal() || other.is_color() || other.is_infinity() ||
        other.is_percentage() || other.is_binary()) {
        why = both + ", and a fraction meeting " + other.kind_name() +
              " is not built yet -- the new types are mixed together later";
        return not_built_yet;
    }
    if (what == "+" && other.is_string()) {
        why = both + " -- with a number first, + adds; to join them, put the text first (\"total: \" + n) or "
                     "write n.satellite.string";
        return types_do_not_meet;
    }
    why = both + ", and there is no scenario for that pair";
    return types_do_not_meet;
}

// A/B AGAINST C/D IS A*D AGAINST C*B, since B and D are above zero -- and each of
// the four is a float, its digits over 10^places, so each product is two whole
// numbers multiplied and its places added (float_scaled.hpp). The side with fewer
// places is brought up to the other's, and then two whole numbers are compared.
//
// EXACT, NEVER ROUNDED. Multiplying through satelliteObject would round each
// product to arguments.float.decimal places, and two fractions that differ past
// that place would compare equal -- a comparison that is wrong and does not say so.
int fraction_order(const satellite_fraction &left, const satellite_fraction &right)
{
    satellite_number across = float_scaled(left.numerator, left.numerator.places) *
                              float_scaled(right.denominator, right.denominator.places);
    satellite_number back = float_scaled(right.numerator, right.numerator.places) *
                            float_scaled(left.denominator, left.denominator.places);
    const unsigned long long int across_places = left.numerator.places + right.denominator.places;
    const unsigned long long int back_places = right.numerator.places + left.denominator.places;
    if (across_places < back_places)
        across = across * ten_to_the(back_places - across_places);
    else if (back_places < across_places)
        back = back * ten_to_the(across_places - back_places);
    const int order = satellite_number::compare(across, back);
    // A DENOMINATOR BELOW ZERO turns the inequality over. Nothing makes one today
    // (satellite_fraction.hpp), and this line keeps the order true if anything does.
    return left.denominator.negative != right.denominator.negative ? -order : order;
}

} // namespace

std::string fraction_part_written(const satellite_float &part)
{
    if (part.fraction.is_zero())
        return (part.negative ? "-" : "") + part.whole.to_text();
    return float_written(part);
}

std::string fraction_written(const satellite_fraction &value)
{
    return fraction_part_written(value.numerator) + "/" + fraction_part_written(value.denominator);
}

satellite_fraction fraction_of_number(const satellite_number &value)
{
    return satellite_fraction{float_of_number(value), float_of_number(satellite_number(1ull))};
}

// THE SIGN RIDES ON THE NUMERATOR (satellite_fraction.hpp), so a bottom number below zero turns both
// over: 5 / -4 is -5/4 and -6 / -4 is 6/4. Nothing else is changed -- 6 / 4 stays 6/4.
signed long long int fraction_of_division(const satellite_number &above, const satellite_number &below,
                                          satelliteObject &out, std::string &why)
{
    if (below.is_zero()) {
        why = "the / of " + above.to_text() + " and " + below.to_text() + " is a division by zero";
        return division_by_zero;
    }
    if (below.negative())
        out = satelliteObject::of_fraction(satellite_fraction{float_of_number(-above), float_of_number(-below)});
    else
        out = satelliteObject::of_fraction(satellite_fraction{float_of_number(above), float_of_number(below)});
    return success;
}

// A/B IS (A's digits * 10^B's places) / (B's digits * 10^A's places), and the division of two whole numbers
// cuts toward zero (satellite_number.hpp) -- so 1.5/2 is 15 / 20, which is 0, and -7/2 is -3. The bottom
// number is above zero, so only the top one's sign reaches the answer.
satellite_number fraction_cut_toward_zero(const satellite_fraction &value)
{
    satellite_number above = float_scaled(value.numerator, value.numerator.places);
    satellite_number below = float_scaled(value.denominator, value.denominator.places);
    if (value.denominator.places != 0)
        above = above * ten_to_the(value.denominator.places);
    if (value.numerator.places != 0)
        below = below * ten_to_the(value.numerator.places);
    satellite_number quotient, remainder;
    satellite_number::divide(above, below, quotient, remainder);
    return quotient;
}

// THE SAME TWO WHOLE NUMBERS, DIVIDED AND ROUNDED as two numbers are (number_and_number_divide_rounded.hpp),
// so satellite.convert.number(f) of a fraction f = 7 / 2 is what satellite.convert.number(7 / 2) is: 4.
satellite_number fraction_rounded(const satellite_fraction &value, bool &lost)
{
    satellite_number above = float_scaled(value.numerator, value.numerator.places);
    satellite_number below = float_scaled(value.denominator, value.denominator.places);
    if (value.denominator.places != 0)
        above = above * ten_to_the(value.denominator.places);
    if (value.numerator.places != 0)
        below = below * ten_to_the(value.numerator.places);
    satellite_number quotient;
    number_and_number_divide_rounded(above, below, quotient, lost);   // below is never zero (satellite_fraction.hpp)
    return quotient;
}

// FRACTION ARITHMETIC IS NOT BUILT, and says so by name. `1/3 + 1/3` has one right
// answer (2/3) and several right spellings of it (2/3, 6/9, 0.666...), and which
// one a program gets back is the author's to say -- "let's just start with" two
// numbers tied together.
signed long long int fraction_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                     satelliteObject &, std::string &why)
{
    satellite_fraction left_made, right_made;
    if (fraction_of(left, left_made) == nullptr || fraction_of(right, right_made) == nullptr)
        return refuse_meeting(std::string(1, sign), left, right, why);
    // AND WHAT TO WRITE INSTEAD, since every a / b is a fraction now (2026-10-12): the whole number or
    // the float it is worth, made first.
    why = std::string("the ") + sign + " of " + shown(left) + " and " + shown(right) +
          " is fraction arithmetic, and fraction arithmetic is not built yet -- so far a fraction is two numbers "
          "tied together, written, shown and compared; " + kConvertTheFraction;
    return not_built_yet;
}

signed long long int fraction_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                   std::string &why)
{
    satellite_fraction left_made, right_made;
    const satellite_fraction *a = fraction_of(left, left_made);
    const satellite_fraction *b = fraction_of(right, right_made);
    if (a == nullptr || b == nullptr)
        return refuse_meeting("a comparison", left, right, why);
    order = fraction_order(*a, *b);
    return success;
}

// BY VALUE, as `==` is: 2/4 and 1/2 are the same fraction written two ways, so a
// list holding 2/4 contains 1/2. The display still keeps what was written.
bool fraction_same(const satellite_fraction &left, const satellite_fraction &right)
{
    return fraction_order(left, right) == 0;
}

signed long long int fraction_to_string(const satellite_fraction &value, satellite_string &out, std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(fraction_written(value), out, bad_offset);
}

// NOT BUILT HERE: 4/2 is 2 and 1/3 is no whole number, so a number made of a fraction is
// said out loud -- satellite.convert.number(f), rounded with a warning when it was not
// even (the author, 2026-10-12; fraction_rounded) -- and a fraction's own numbers are its
// .satellite.numerator and .satellite.denominator.
signed long long int fraction_to_number(const satellite_fraction &value, satellite_number &, std::string &why)
{
    why = "a number made of the fraction " + fraction_written(value) +
          " is written satellite.convert.number(...), the whole number nearest it -- its two numbers are "
          ".satellite.numerator and .satellite.denominator";
    return not_built_yet;
}

signed long long int fraction_to_binary(const satellite_fraction &value, satellite_string &, std::string &why)
{
    why = "base 2 text of a fraction (" + fraction_written(value) +
          ") is not built yet -- a fraction meeting a binary is one of the new types mixed together later";
    return not_built_yet;
}

signed long long int fraction_to_hexadecimal(const satellite_fraction &value, satellite_string &, std::string &why)
{
    why = "base 16 text of a fraction (" + fraction_written(value) +
          ") is not built yet -- a fraction meeting a hex is one of the new types mixed together later";
    return not_built_yet;
}

} // namespace satellite004
