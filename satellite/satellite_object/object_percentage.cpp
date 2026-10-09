// satellite/satellite_object/object_percentage.cpp -- WHICH PAIR FILE A
// PERCENTAGE GOES TO. satelliteObject's methods send every pair holding a
// percentage here (satellite_object.cpp), and this file says which of the nine
// <left>_and_<right>_<operation>.hpp files does the work. It is its own file
// because satellite_object.cpp was already past the author's 300-line target, and
// "a percentage met something" is one subject.
//
// (the author, 2026-09-17) "take 1% and 100% as meaning a percentage and
// 1000000000000% as something you can multiply by".
//
// WHAT ANSWERS, AND WHAT IT ANSWERS:
//
//     50% + 25%     75%            a percentage and a percentage stay a percentage
//     50% * 50%     25%            ... a percentage OF a percentage
//     50% / 4       12.5%          ... a share of it
//     200 * 50%     100            a number and a percentage answer a number:
//     200 - 50%     100            reduced by it (the author's infinity - 50%)
//     200 + 50%     300            grown by it
//     200 / 50%     400            what 200 is 50% of
//
// A NUMBER ON THE RIGHT OF + OR - IS REFUSED: `50% + 5` has no reading the author
// has given, and a percentage is taken off a number rather than a number off a
// percentage. `%` and `^` with a percentage are refused the same way.

#include "fast_paths.hpp"

#include "number_and_percentage_add.hpp"
#include "number_and_percentage_divide.hpp"
#include "number_and_percentage_multiply.hpp"
#include "number_and_percentage_subtract.hpp"
#include "percentage_and_number_divide.hpp"
#include "percentage_and_percentage_add.hpp"
#include "percentage_and_percentage_compare.hpp"
#include "percentage_and_percentage_multiply.hpp"
#include "percentage_and_percentage_subtract.hpp"

namespace satellite004 {
namespace {

// A value as a person wrote it, for a refusal: 3, 50%, "text".
std::string shown(const satelliteObject &value)
{
    if (const satellite_number *number = value.as_number())
        return number->to_text();
    if (const satellite_percentage *percent = value.as_percentage())
        return percent->written();
    return value.kind_name();
}

} // namespace

signed long long int percentage_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                          satelliteObject &out, std::string &why)
{
    const satellite_percentage *left_percent = left.as_percentage();
    const satellite_percentage *right_percent = right.as_percentage();
    const satellite_number *left_number = left.as_number();
    const satellite_number *right_number = right.as_number();

    signed long long int code = types_do_not_meet;
    satellite_percentage percent;
    satellite_number number;
    bool answers_a_percentage = false;

    switch (sign) {
    case '+':
        if (left_percent != nullptr && right_percent != nullptr) {
            code = percentage_and_percentage_add(*left_percent, *right_percent, percent);
            answers_a_percentage = true;
        } else if (left_number != nullptr && right_percent != nullptr) {
            code = number_and_percentage_add(*left_number, *right_percent, number);
        }
        break;
    case '-':
        if (left_percent != nullptr && right_percent != nullptr) {
            code = percentage_and_percentage_subtract(*left_percent, *right_percent, percent);
            answers_a_percentage = true;
        } else if (left_number != nullptr && right_percent != nullptr) {
            code = number_and_percentage_subtract(*left_number, *right_percent, number);
        }
        break;
    case '*':
        if (left_percent != nullptr && right_percent != nullptr) {
            code = percentage_and_percentage_multiply(*left_percent, *right_percent, percent);
            answers_a_percentage = true;
        } else if (left_number != nullptr && right_percent != nullptr) {
            code = number_and_percentage_multiply(*left_number, *right_percent, number);
        } else if (left_percent != nullptr && right_number != nullptr) {
            code = number_and_percentage_multiply(*right_number, *left_percent, number);
        }
        break;
    case '/':
        if (left_percent != nullptr && right_number != nullptr) {
            code = percentage_and_number_divide(*left_percent, *right_number, percent);
            answers_a_percentage = true;
        } else if (left_number != nullptr && right_percent != nullptr) {
            code = number_and_percentage_divide(*left_number, *right_percent, number);
        }
        break;
    default:
        break;
    }

    if (code == success) {
        out = answers_a_percentage ? satelliteObject::of_percentage(std::move(percent))
                                   : satelliteObject::of_number(std::move(number));
        return success;
    }

    const std::string both = shown(left) + " " + sign + " " + shown(right);
    // THE FLOAT EXISTS SINCE 2026-09-22, and a percentage answering one is the
    // author's grand finale -- the new types mixed together -- so `3 - 50%` says
    // that, and is still refused until then (SATELLITE_INFINITY.md FLT-2).
    if (code == answer_is_not_whole)
        why = both + " is not a whole number, and a percentage answering a float is not built yet -- "
                     "the new types are mixed together later";
    else if (code == division_by_zero)
        why = both + " is a division by zero";
    else if (left_percent != nullptr && right_number != nullptr && (sign == '+' || sign == '-'))
        why = both + ": a percentage is taken off or added onto a number, so the number goes first -- " +
              shown(right) + " " + sign + " " + shown(left);
    else
        why = std::string(1, sign) + " was given " + left.kind_name() + " and " + right.kind_name() +
              ", and there is no scenario for that pair";
    return code;
}

signed long long int percentage_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                        std::string &why)
{
    const satellite_percentage *left_percent = left.as_percentage();
    const satellite_percentage *right_percent = right.as_percentage();
    if (left_percent != nullptr && right_percent != nullptr) {
        order = percentage_and_percentage_compare(*left_percent, *right_percent);
        return success;
    }
    why = std::string("a comparison was given ") + left.kind_name() + " and " + right.kind_name() +
          ", and a percentage only compares with a percentage";
    return types_do_not_meet;
}

} // namespace satellite004
