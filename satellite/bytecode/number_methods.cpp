// satellite/bytecode/number_methods.cpp -- the header says why a method here is
// a name bound to a fast path and never a second implementation of one.
//
// EVERY METHOD BELOW IS BACKED BY CODE THAT ALREADY EXISTS AND IS ALREADY
// CHECKED. power, modulus and the comparisons are number_fast_path's; to_string,
// binary and hex are number_conversions'; digits and bytes are
// satellite_number's own. 482,465 cases in check_numbers.py prove the answers,
// and this file only chooses which one to ask for. Nothing here does arithmetic.
//
// THE FOUR THAT DO NOTHING ARE NOT PLACEHOLDERS. floor, ceil, round and truncate
// all answer the receiver unchanged, because satellite.variable.number is a
// WHOLE number -- 003 DESIGN §8.6 puts all four in its "exact and bounded" class
// and says trunc of a value with no fractional part is that value. They are here
// rather than refused so that a program written against the float can be moved
// to a number without its arithmetic silently changing meaning.

#include "number_methods.hpp"

#include "../satellite_variable_number/number_arithmetic.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

namespace satellite004 {
namespace {

namespace fast = number_fast_path;

// How many arguments a method takes. -1 for a name that is not a method here.
int arguments_wanted(const std::string &name)
{
    if (name == "power" || name == "modulus" || name == "max" || name == "min" ||
        name == "shift_left" || name == "shift_right")
        return 1;
    if (name == "abs" || name == "to_string" || name == "string" || name == "number" ||
        name == "binary" || name == "hex" || name == "digits" || name == "bytes" ||
        name == "floor" || name == "ceil" || name == "round" || name == "truncate" ||
        name == "sqrt")
        return 0;
    return -1;
}

// 2 to the power of n, for the two shifts. A shift is multiplication and
// division by a power of two -- 003 DESIGN §8.6 gave the spelling that meaning
// on 2026-08-31 -- so both are the fast paths with a power worked out first.
signed long long int two_to_the(const satellite_number &n, satellite_number &out)
{
    return satellite_number::power(satellite_number(2ull), n, out);
}

} // namespace

signed long long int call_number_method(const std::string &name,
                                        const satellite_number &receiver,
                                        const std::vector<Value> &arguments,
                                        Value &out,
                                        std::string &why)
{
    const int wanted = arguments_wanted(name);
    if (wanted < 0) {
        why = "satellite.variable.number has no method named " + name;
        return satl_line_not_understood;
    }
    if ((int)arguments.size() != wanted) {
        why = name + " takes " + std::to_string(wanted) + " argument" + (wanted == 1 ? "" : "s") +
              ", and was given " + std::to_string(arguments.size());
        return satl_line_not_understood;
    }
    // EVERY ONE-ARGUMENT METHOD HERE TAKES A NUMBER, so the kind is checked once
    // rather than in each branch, and the refusal names what arrived.
    if (wanted == 1 && arguments[0].kind != Value::Kind::number) {
        why = name + " takes a number, and was given " + arguments[0].kind_name();
        return types_do_not_meet;
    }
    const satellite_number &given = wanted == 1 ? arguments[0].number : receiver;

    // The two the author asked for by name, and they are the operators' own
    // fast paths: n.power(3) and n ^ 3 are one call.
    if (name == "power" || name == "modulus") {
        satellite_number answer;
        const signed long long int code = name == "power" ? fast::power(receiver, given, answer)
                                                          : fast::modulus(receiver, given, answer);
        if (code != success) {
            why = "the " + name + " of " + receiver.to_text() + " and " + given.to_text() + " is " +
                  (code == division_by_zero ? "a division by zero" : "not a whole number, and there is no float yet");
            return code;
        }
        out = Value::of_number(std::move(answer));
        return success;
    }

    if (name == "max" || name == "min") {
        const int order = fast::compare(receiver, given);
        const bool take_receiver = name == "max" ? order >= 0 : order <= 0;
        out = Value::of_number(take_receiver ? receiver : given);
        return success;
    }

    if (name == "shift_left" || name == "shift_right") {
        if (given.negative()) {
            why = name + " was given " + given.to_text() + ", and a shift is never negative";
            return not_a_position;
        }
        satellite_number scale;
        const signed long long int code = two_to_the(given, scale);
        if (code != success) {
            why = "2 to the power of " + given.to_text() + " could not be worked out";
            return code;
        }
        satellite_number answer;
        const signed long long int held = name == "shift_left" ? fast::multiply(receiver, scale, answer)
                                                               : fast::divide(receiver, scale, answer);
        if (held != success) {
            why = "the shift could not be worked out";
            return held;
        }
        out = Value::of_number(std::move(answer));
        return success;
    }

    if (name == "abs") {
        out = Value::of_number(receiver.negative() ? -receiver : receiver);
        return success;
    }

    // THE CONVERSIONS, which are number_conversions.hpp's fast paths under the
    // names words.tsv already gives them (1 6 4 6, 17, 19, 20).
    if (name == "to_string" || name == "string") {
        out = Value::of_text(fast::to_text(receiver, fast::kDecimal));
        return success;
    }
    if (name == "binary") {
        out = Value::of_text(fast::to_text(receiver, fast::kBinary));
        return success;
    }
    if (name == "hex") {
        out = Value::of_text(fast::to_text(receiver, fast::kHexadecimal));
        return success;
    }
    if (name == "number") {       // a number's own `number` is itself (the conversion set)
        out = Value::of_number(receiver);
        return success;
    }

    if (name == "digits") {
        out = Value::of_number(receiver.digits());
        return success;
    }
    if (name == "bytes") {
        out = Value::of_number(receiver.bytes());
        return success;
    }

    // A WHOLE NUMBER IS ALREADY FLOORED, CEILED, ROUNDED AND TRUNCATED. See the
    // note at the top: this is 003 DESIGN §8.6's rule, not a stub.
    if (name == "floor" || name == "ceil" || name == "round" || name == "truncate") {
        out = Value::of_number(receiver);
        return success;
    }

    // sqrt is the one method of this type with nothing behind it: an exact
    // integer square root is a real piece of work (Newton's method over limbs)
    // and 003 DESIGN §8.6 puts sqrt in the class that MUST round, which a whole
    // number cannot do. It is refused by name rather than left out of the list,
    // so `n.sqrt()` says what is missing instead of "no method named sqrt".
    why = "satellite.variable.number.sqrt is numbered (1 6 4 14) and not built yet";
    return not_built_yet;
}

} // namespace satellite004
