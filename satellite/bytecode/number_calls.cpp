// satellite/bytecode/number_calls.cpp -- what a number answers (MILESTONES.md M17), reached BY ITS NUMBER
// since 2026-10-11. number_calls.hpp says what each method is and why an infinity runs a number's words.
// Each method's own code is a file under satellite-numbers/words/ named by its number -- 1.6.4.7.cpp is
// floor -- and this file is what they share: which method runs which number, how many arguments each
// takes, the whole number they work on, and run_number_number, which calls the one a method's number names.
//
// number_methods.cpp, written 2026-09-16 and never wired (in no make file, called by nothing), is gone: its
// methods are those files now, each with its code, and .power and .modulus ask the operators' own
// satelliteObject::power and ::modulus -- its own power refused 2 ^ -1, which ^ answers as 0.5.

#include "number_calls.hpp"

#include "file_calls.hpp"
#include "word_codes.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

// EVERY NUMBER BELOW IS IN words/words.tsv, OR THIS FILE DOES NOT COMPILE: a fixed_code of 0 would quietly
// send that method down the refusals in expression.cpp instead of to its number. 1 6 4 6 (to_string) is
// 17's second name and runs nothing, and 1 6 4 16 is number(x), which makes a number and is no method.
template <int... Last>
inline constexpr bool every_number_word_numbered = ((word::fixed_code<1, 6, 4, Last> != 0) && ...);
static_assert(every_number_word_numbered<1, 2, 3, 4, 5, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 18, 19, 20, 21, 22, 23>,
              "a number method's number is missing from words/words.tsv");
template <int... Last>
inline constexpr bool every_infinity_word_numbered = ((word::fixed_code<1, 6, 17, Last> != 0) && ...);
static_assert(every_infinity_word_numbered<1, 2, 3, 4>, "an infinity method's number is missing from words/words.tsv");

token::Code number_number_of(token::Code method)
{
    // ONE METHOD, ONE NUMBER, read off words/words.tsv -- 003's twenty keep theirs, and the three a number
    // answered or was owed with no number took the next free ones, 1 6 4 21 to 23 (add, reverse, bytes).
    // `to_string`, `str` and `string` are one code by the time anything here sees them, as a string's are,
    // and it runs 1 6 4 17, satellite.variable.number.string: the four conversions are 17 to 20, as a
    // string's are 1 6 1 19 to 22, and 1 6 4 6, satellite.variable.number.to_string, is 17's second name.
    switch (method) {
    case token::shift_left_method_token: return word::fixed_code<1, 6, 4, 1>;
    case token::max_token: return word::fixed_code<1, 6, 4, 2>;
    case token::min_token: return word::fixed_code<1, 6, 4, 3>;
    case token::abs_token: return word::fixed_code<1, 6, 4, 4>;
    case token::clamp_token: return word::fixed_code<1, 6, 4, 5>;
    case token::floor_token: return word::fixed_code<1, 6, 4, 7>;
    case token::ceil_token: return word::fixed_code<1, 6, 4, 8>;
    case token::round_token: return word::fixed_code<1, 6, 4, 9>;
    case token::power_of_token: return word::fixed_code<1, 6, 4, 10>;
    case token::shift_right_method_token: return word::fixed_code<1, 6, 4, 11>;
    case token::modulus_method_token: return word::fixed_code<1, 6, 4, 12>;
    case token::truncate_token: return word::fixed_code<1, 6, 4, 13>;
    case token::sqrt_token: return word::fixed_code<1, 6, 4, 14>;
    case token::digits_token: return word::fixed_code<1, 6, 4, 15>;
    case token::to_string_token: return word::fixed_code<1, 6, 4, 17>;
    case token::to_number_token: return word::fixed_code<1, 6, 4, 18>;
    case token::to_binary_token: return word::fixed_code<1, 6, 4, 19>;
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 4, 20>;
    case token::add_token: return word::fixed_code<1, 6, 4, 21>;
    case token::reverse_token: return word::fixed_code<1, 6, 4, 22>;
    case token::bytes_token: return word::fixed_code<1, 6, 4, 23>;
    default: return 0;
    }
}

token::Code infinity_number_of(token::Code method)
{
    // THE INFINITY'S OWN, from 1 6 17 1: its display, and the three whose tokens INF-1 made and whose
    // milestones build them -- .power_of (spelled power_of, to_the_power_of and power; SATELLITE_INFINITY.md
    // Part 5 calls it "new, and distinct from" the number's 1 6 4 10), .resize and .nines.
    switch (method) {
    case token::to_string_token: return word::fixed_code<1, 6, 17, 1>;
    case token::power_of_token: return word::fixed_code<1, 6, 17, 2>;
    case token::resize_token: return word::fixed_code<1, 6, 17, 3>;
    case token::nines_token: return word::fixed_code<1, 6, 17, 4>;
    default: return 0;
    }
}

int number_method_arity(token::Code method)
{
    switch (method) {
    case token::clamp_token: return 2;
    case token::max_token:
    case token::min_token:
    case token::power_of_token:
    case token::modulus_method_token:
    case token::shift_left_method_token:
    case token::shift_right_method_token: return 1;
    case token::abs_token:
    case token::floor_token:
    case token::ceil_token:
    case token::round_token:
    case token::truncate_token:
    case token::sqrt_token:
    case token::digits_token:
    case token::bytes_token: return 0;
    default: return -1;
    }
}

bool named_until_numbered(token::Code method)
{
    switch (method) {
    case token::abs_token:
    case token::floor_token:
    case token::ceil_token:
    case token::round_token:
    case token::sqrt_token:
    case token::digits_token:
    case token::modulus_method_token:
    case token::shift_left_method_token:
    case token::shift_right_method_token:
    case token::clamp_token:
    case token::bytes_token: return true;
    default: return false;
    }
}

std::string number_method_not_built(token::Code method)
{
    return method == token::sqrt_token ? "is numbered (1 6 4 14) and not built yet" : "";
}

signed long long int number_method_check(bool bracketed, std::size_t given, token::Code method,
                                         const std::string &spelling, std::string &why)
{
    // A METHOD THAT TAKES NOTHING MAY BE WRITTEN WITH OR WITHOUT ITS BRACKETS, as a string's and a list's
    // are: `n.satellite.floor` and `n.satellite.floor()` are one call. One that takes something needs them.
    const int wanted = number_method_arity(method);
    if (wanted >= 0 && (given != static_cast<std::size_t>(wanted) || (wanted > 0 && !bracketed))) {
        why = spelling + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
              (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
        return satl_line_not_understood;
    }
    const std::string missing = number_method_not_built(method);
    if (!missing.empty()) {
        why = spelling + " " + missing;
        return not_built_yet;
    }
    return success;
}

Value run_number_number(token::Code method, token::Code number, const Value &receiver,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context)
{
    // HOW MANY, AGAIN, for a receiver the checker could not see: a literal in brackets, a chain, an item of
    // a list. The same sentence number_method_check says before the run. AN INFINITY'S OWN ROW says its own
    // -- each is refused with the milestone that builds it, whatever it was given, as before it had a number.
    const int wanted = receiver.is_infinity() && number == infinity_number_of(method) ? -1 : number_method_arity(method);
    if (wanted >= 0 &&
        (arguments.size() != static_cast<std::size_t>(wanted) || (wanted > 0 && !had_parentheses))) {
        const std::string call = name + ".satellite." + token::method_name_of(method);
        context.refuse(satl_line_not_understood,
                       call + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
                           (had_parentheses ? ", and was given " + std::to_string(arguments.size())
                                            : ", in brackets after it"));
        return Value();
    }

    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the function its
    // file under satellite-numbers/words/ put in the row at start-up. A number's method changes nothing it
    // is called on, so it is handed no home and no shape.
    const NumberRow *row = context.functions.method(number);
    if (row == nullptr || row->scenarios.method_call == nullptr) {
        context.refuse(not_built_yet, name + ".satellite." + token::method_name_of(method) + " is not built for " +
                                          receiver.kind_name() + " yet");
        return Value();
    }
    return row->scenarios.method_call(receiver, nullptr, nullptr, arguments, had_parentheses, name, context);
}

namespace number_method_parts {

const satellite_number *the_number(const Value &receiver, const std::string &call, token::Code method,
                                   ExpressionContext &context)
{
    if (const satellite_number *number = receiver.as_number())
        return number;
    // AN INFINITY HAS NO WHOLE NUMBER TO WORK ON, and is told so as expression.cpp's last refusal tells any
    // type a word it does not have yet -- the words an infinity was told before a number's had numbers.
    context.refuse(not_built_yet, call + " is not built for " + receiver.kind_name() + " yet -- " + so_far_whose(method));
    return nullptr;
}

const satellite_number *number_argument(const Value &value, const std::string &call, const std::string &what,
                                        ExpressionContext &context)
{
    if (const satellite_number *number = value.as_number())
        return number;
    context.refuse(types_do_not_meet, call + " takes " + what + ", and was given " + value.kind_name());
    return nullptr;
}

} // namespace number_method_parts

} // namespace satellite004
