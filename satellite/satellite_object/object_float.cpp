// satellite/satellite_object/object_float.cpp -- what a float answers when it
// meets something (object_float.hpp says why this is its own file).
//
// (the author, 2026-09-22) "a float is going to be two satellite_number's put
// together, one before the decimal, one after, plus a sign which is positive by
// default, and a precision that it gets from arguments".
//
// WHAT ANSWERS, AND WHAT IT ANSWERS (arguments.float.decimal at its 128):
//
//     1.5 + 2.25     3.75           a float and a float
//     1.5 * 2        3.0            a float and a plain number, either side:
//     10 - 0.5       9.5            the number is read as the float it is (7 is 7.0)
//     2.0 / 3        0.666...667    128 places, the last rounded half away from zero
//     5.5 % 2        1.5            the remainder, with the dividend's sign, as a number's
//     1.5 ^ 2        2.25           a whole-number power, exact until it is rounded
//     2 ^ -1         0.5            a number to a negative power is a float now (FLT-2)
//
// AND WHAT DOES NOT: a float meeting an infinity, a percentage, a binary, a hex, a
// colour or a fraction is the author's "grand finale", the new types mixed
// together, and until then it says that; anything else is a pair with no scenario,
// named as one. `4 / 3` is still 1 -- two plain numbers never come here -- and
// `4.0 / 3` is the float (SATELLITE_INFINITY.md Q13).

#include "object_float.hpp"

#include "../satellite_variable_float/float_scaled.hpp"

#include <climits>
#include <string>
#include <utility>

namespace satellite004 {
namespace {

// A FLOAT, OR A PLAIN NUMBER READ AS ONE -- the two kinds a float meets today.
// nullptr for anything else. `made` holds the number's float, so a float that
// is already one is never copied to be looked at.
const satellite_float *float_of(const satelliteObject &value, satellite_float &made)
{
    if (const satellite_float *held = value.as_float())
        return held;
    if (const satellite_number *held = value.as_number()) {
        made = float_of_number(*held);
        return &made;
    }
    return nullptr;
}

// A value as a person wrote it, for a refusal: 2.5, 3, "text".
std::string shown(const satelliteObject &value)
{
    if (const satellite_float *held = value.as_float())
        return float_written(*held);
    if (const satellite_number *held = value.as_number())
        return held->to_text();
    return value.kind_name();
}

// THE PAIRS THAT WAIT FOR THE FINALE, and the sentence that says so. Anything
// else a float meets has no scenario at all, and is named as that pair.
signed long long int refuse_meeting(const std::string &what, const satelliteObject &left,
                                    const satelliteObject &right, std::string &why)
{
    const satelliteObject &other = left.is_float() ? right : left;
    const std::string both = what + " was given " + left.kind_name() + " and " + right.kind_name();
    if (other.is_infinity() || other.is_percentage() || other.is_binary() || other.is_hexadecimal() ||
        other.is_color() || other.is_fraction()) {
        why = both + ", and a float meeting " + other.kind_name() +
              " is not built yet -- the new types are mixed together later";
        return not_built_yet;
    }
    if (what == "+" && other.is_string()) {
        why = both + ", and satellite converts nothing on its own -- write the conversion (.string) out loud";
        return types_do_not_meet;
    }
    why = both + ", and there is no scenario for that pair";
    return types_do_not_meet;
}

satellite_float float_add(const satellite_float &left, const satellite_float &right, bool take_away)
{
    const unsigned long long int places = left.places > right.places ? left.places : right.places;
    const satellite_number a = float_scaled(left, places), b = float_scaled(right, places);
    return float_from_scaled(take_away ? a - b : a + b, places);
}

satellite_float float_multiply(const satellite_float &left, const satellite_float &right)
{
    return float_from_scaled(float_scaled(left, left.places) * float_scaled(right, right.places),
                             left.places + right.places);
}

// A / B IS (A's digits * 10^B's places) / (B's digits * 10^A's places), with the
// tens both sides share left out.
signed long long int float_divide(const satellite_float &left, const satellite_float &right, satellite_float &out)
{
    satellite_number a = float_scaled(left, left.places), b = float_scaled(right, right.places);
    if (b.is_zero())
        return division_by_zero;
    if (a.negative()) a = -a;
    if (b.negative()) b = -b;
    const unsigned long long int shared = left.places < right.places ? left.places : right.places;
    out = float_from_quotient(a * ten_to_the(right.places - shared), b * ten_to_the(left.places - shared),
                              left.negative != right.negative);
    return success;
}

signed long long int float_modulus(const satellite_float &left, const satellite_float &right, satellite_float &out)
{
    const unsigned long long int places = left.places > right.places ? left.places : right.places;
    const satellite_number divisor = float_scaled(right, places);
    if (divisor.is_zero())
        return division_by_zero;
    satellite_number quotient, remainder;
    satellite_number::divide(float_scaled(left, places), divisor, quotient, remainder);
    out = float_from_scaled(std::move(remainder), places);
    return success;
}

// A WHOLE-NUMBER POWER. Upward it is exact and then rounded once: 1.5 ^ 2 is 225
// at 2 places. Downward it is 1 over that, a quotient like any division. A power
// with a fraction in it is a root, and is refused by the caller.
//
// NO CEILING ON THE EXPONENT, as satellite_number::power has none: the places are
// counted in a satellite_number too, and a power too large for the machine is the
// machine's limit reached, not the language's.
signed long long int float_power(const satellite_float &base, const satellite_number &exponent, satellite_float &out)
{
    const satellite_number digits = float_scaled(base, base.places);
    const bool upward = !exponent.negative();
    const satellite_number times = upward ? exponent : -exponent;

    // AN ANSWER BELOW HALF THE LAST PLACE KEPT IS 0.0, AND IS SAID WITHOUT BUILDING
    // THE NUMBER THAT PROVES IT. `2 ^ -1000000000` is 1 over a number of 300 million
    // digits, and `0.5 ^ 1000000000` the same number's reciprocal written out -- both
    // round to 0.0 at any arguments.float.decimal that fits in memory, and working
    // them out first would be the method's cost and not the machine's. A base of at
    // least 2 going down, or at most 0.5 going up, is at most 2^-times, which is
    // below 10^-(decimal + 1) once times passes 4 * (decimal + 1) (2^4 > 10).
    const unsigned long long int decimal = float_precision_in_use.decimal;
    const bool at_most_a_half = base.whole.is_zero() &&
                                satellite_number::compare(base.fraction + base.fraction, ten_to_the(base.places)) <= 0;
    const bool at_least_two = satellite_number::compare(base.whole, satellite_number(2ull)) >= 0;
    if (!digits.is_zero() && decimal < ULLONG_MAX / 8 && (upward ? at_most_a_half : at_least_two) &&
        satellite_number::compare(times, satellite_number(4 * (decimal + 1))) > 0) {
        out = satellite_float();
        return success;
    }
    satellite_number raised;
    satellite_number::power(digits, times, raised);
    const satellite_number places = satellite_number(base.places) * times;
    if (upward && places.fits_one_limb()) {
        out = float_from_scaled(std::move(raised), places.limb(0));
        return success;
    }
    satellite_number tens;
    satellite_number::power(satellite_number(10ull), places, tens);
    if (upward) {
        const bool below_zero = raised.negative();
        out = float_from_quotient(below_zero ? -raised : raised, tens, below_zero);
        return success;
    }
    if (raised.is_zero())
        return division_by_zero;
    const bool below_zero = raised.negative();
    out = float_from_quotient(tens, below_zero ? -raised : raised, below_zero);
    return success;
}

} // namespace

signed long long int float_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                     satelliteObject &out, std::string &why)
{
    satellite_float left_made, right_made;
    const satellite_float *a = float_of(left, left_made);
    const satellite_float *b = float_of(right, right_made);
    if (a == nullptr || b == nullptr)
        return refuse_meeting(std::string(1, sign), left, right, why);

    satellite_float answer;
    signed long long int code = success;
    switch (sign) {
    case '+': answer = float_add(*a, *b, false); break;
    case '-': answer = float_add(*a, *b, true); break;
    case '*': answer = float_multiply(*a, *b); break;
    case '/': code = float_divide(*a, *b, answer); break;
    case '%': code = float_modulus(*a, *b, answer); break;
    case '^':
        if (!b->fraction.is_zero()) {
            why = "the ^ of " + shown(left) + " and " + shown(right) +
                  " is a power that is not a whole number, and only a whole-number power is built -- "
                  "a fractional power is a root";
            return not_built_yet;
        }
        code = float_power(*a, b->negative ? -b->whole : b->whole, answer);
        break;
    default:
        why = std::string(1, sign) + " is not an operator a float has";
        return satl_line_not_understood;
    }
    if (code != success) {
        why = std::string("the ") + sign + " of " + shown(left) + " and " + shown(right) + " is a division by zero";
        return code;
    }
    out = satelliteObject::of_float(std::move(answer));
    return success;
}

signed long long int float_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                   std::string &why)
{
    satellite_float left_made, right_made;
    const satellite_float *a = float_of(left, left_made);
    const satellite_float *b = float_of(right, right_made);
    if (a == nullptr || b == nullptr)
        return refuse_meeting("a comparison", left, right, why);
    const unsigned long long int places = a->places > b->places ? a->places : b->places;
    order = satellite_number::compare(float_scaled(*a, places), float_scaled(*b, places));
    return success;
}

// THE FOUR FIELDS, because every float is held in the one form (float_scaled.hpp):
// 12.5 and 12.50 are one float, and 12.05 is another, whose `places` differ.
bool float_same(const satellite_float &left, const satellite_float &right)
{
    return left.negative == right.negative && left.places == right.places && left.whole == right.whole &&
           left.fraction == right.fraction;
}

signed long long int float_to_string(const satellite_float &value, satellite_string &out, std::string &)
{
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(float_written(value), out, bad_offset);
}

// THE WHOLE NUMBER WHEN THERE IS NO FRACTION, AND A REFUSAL WHEN THERE IS: 3.0 is
// 3, and 12.5 is not a whole number. Cutting it to 12 or rounding it to 13 would
// be choosing an answer behind the program's back; which of the two a program
// means is for it to say, once a float's .round and .floor are built. If the
// author rules that .number cuts, it is `out = whole` with the sign, here.
signed long long int float_to_number(const satellite_float &value, satellite_number &out, std::string &why)
{
    if (!value.fraction.is_zero()) {
        why = float_written(value) + " is not a whole number, so there is no number to make of it -- "
                                     "satellite rounds nothing on its own";
        return answer_is_not_whole;
    }
    out = value.negative ? -value.whole : value.whole;
    return success;
}

signed long long int float_to_binary(const satellite_float &value, satellite_string &, std::string &why)
{
    why = "base 2 text of a float (" + float_written(value) +
          ") is not built yet -- a float meeting a binary is one of the new types mixed together later";
    return not_built_yet;
}

signed long long int float_to_hexadecimal(const satellite_float &value, satellite_string &, std::string &why)
{
    why = "base 16 text of a float (" + float_written(value) +
          ") is not built yet -- a float meeting a hex is one of the new types mixed together later";
    return not_built_yet;
}

} // namespace satellite004
