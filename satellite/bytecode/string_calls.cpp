// satellite/bytecode/string_calls.cpp -- what a string answers (M16), reached BY ITS NUMBER
// since 2026-10-10. string_calls.hpp says what each method is and why positions count from 1;
// string_pieces.hpp holds the loops over the characters. Each method's own code is a file under
// satellite-numbers/words/ named by its number -- 1.6.1.3.cpp is find -- and this file is what
// they share: the arguments going in, the refusals as the program runs, and run_string_number,
// which calls the one a method's number names. string_check.cpp is the refusals said before it
// runs.

#include "string_calls.hpp"

#include "bytecode_registry.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/s_codes.hpp"
#include "../satellite_object/fast_paths.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <limits>
#include <utility>

namespace satellite004 {
namespace {

namespace fast = number_fast_path;

std::string a_count_of(std::size_t n, const char *one, const char *many)
{
    return std::to_string(n) + " " + (n == 1 ? one : many);
}

} // namespace

namespace string_method_parts {

// TEXT GOING IN: the string itself, or a number's digits with S020 in satellite.log.
// `made` holds the digits when there are any, so a string argument is never copied.
const satellite_string *text_argument(const Value &value, satellite_string &made, const std::string &what,
                                      ExpressionContext &context)
{
    if (const satellite_string *text = value.as_string())
        return text;
    if (const satellite_number *number = value.as_number()) {
        const std::string digits = fast::to_text(*number);
        std::size_t bad_offset = 0;
        satellite_string::from_utf8(digits, made, bad_offset);   // digits and a minus are always characters
        warn_number_taken_as_text(context.state, what, digits);
        return &made;
    }
    context.refuse(types_do_not_meet, what + " takes text, and was given " + value.kind_name());
    return nullptr;
}

// A POSITION GOING IN, counting from 1. `written` is the number as the program wrote it,
// for the refusals; a number too large to be any position becomes the largest count,
// which every string is shorter than, so it is refused as past the end in its own digits.
bool position_argument(const Value &value, unsigned long long int &out, std::string &written,
                       const std::string &what, ExpressionContext &context)
{
    const satellite_number *number = value.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + " takes a character's position -- a number, counting from 1 -- "
                                                 "and was given " + value.kind_name());
        return false;
    }
    written = fast::to_text(*number);
    if (number->negative()) {
        context.refuse(not_a_position, what + " was given " + written + ", and characters count from 1");
        return false;
    }
    out = fast::fits_a_count(*number) ? fast::as_count(*number) : std::numeric_limits<unsigned long long int>::max();
    return true;
}

// "the string is empty" or "it holds 5 characters, counting from 1" -- the end of every
// refusal of a position, so a person is told what WAS there as well as what was not.
std::string what_it_holds(const satellite_string &text)
{
    return text.empty() ? "the string is empty"
                        : "it holds " + a_count_of(text.size(), "character", "characters") + ", counting from 1";
}

// ONE CHARACTER, for s.at(n) and s[n]: `open` and `close` spell the call as it was
// written ("s[" and "]", or "s.at(" and ")") so each refusal quotes the program.
Value one_character(const satellite_string &text, const Value &index, const std::string &open,
                    const std::string &close, std::size_t where, bool placed, ExpressionContext &context)
{
    auto refuse = [&](signed long long int code, std::string why) {
        if (placed) context.refuse(code, std::move(why), where);
        else context.refuse(code, std::move(why));
    };
    const satellite_number *number = index.as_number();
    if (number == nullptr) {
        refuse(types_do_not_meet, open + "..." + close + " takes a character's position -- a number, counting "
                                  "from 1 -- and was given " + index.kind_name());
        return Value();
    }
    const std::string written = fast::to_text(*number);
    if (number->negative()) {
        refuse(not_a_position, open + written + close + " -- characters count from 1");
        return Value();
    }
    const unsigned long long int position =
        fast::fits_a_count(*number) ? fast::as_count(*number) : std::numeric_limits<unsigned long long int>::max();
    if (position == 0) {   // S413: satellite counts from 1
        refuse(counts_from_one,
               open + "0" + close + ": satellite counts from 1, not 0, so the first character is " + open + "1" +
                   close);
        return Value();
    }
    char32_t code = 0;
    if (position > text.size() || text.code_at(position - 1, code) != success) {
        refuse(position_past_the_end, open + written + close + ": there is no such character -- " + what_it_holds(text));
        return Value();
    }
    satellite_string one;
    one.append_code(code);
    return Value::of_string(std::move(one));
}

} // namespace string_method_parts

int string_method_arity(token::Code method)
{
    switch (method) {
    case token::substring_token:
    case token::replace_token: return 2;
    case token::find_token:
    case token::contains_token:
    case token::starts_with_token:
    case token::ends_with_token:
    case token::split_token:
    case token::at_token:
    case token::append_token: return 1;
    case token::size_token:
    case token::empty_token:
    case token::trim_token:
    case token::resolved_token:
    case token::clear_token: return 0;
    default: return -1;
    }
}

bool changes_a_string(token::Code method)
{
    return method == token::append_token || method == token::clear_token;
}

// EVERY NUMBER BELOW IS IN words/words.tsv, OR THIS FILE DOES NOT COMPILE: a fixed_code of 0
// would quietly send that method down the refusals in expression.cpp instead of to its number.
template <int... Last>
inline constexpr bool every_string_word_numbered = ((word::fixed_code<1, 6, 1, Last> != 0) && ...);
static_assert(every_string_word_numbered<1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 14, 15, 16, 17, 19, 20, 21, 22, 23,
                                         24, 25, 26>,
              "a string method's number is missing from words/words.tsv");

token::Code string_number_of(token::Code method)
{
    // ONE METHOD, ONE NUMBER, read off words/words.tsv. `to_number`, `number` and `as_number` are
    // one code by the time anything here sees them (the lexer folds a second spelling into the
    // first), and the code's first spelling is `number` -- so it runs 1 6 1 20, and 1 6 1 13,
    // satellite.variable.string.to_number, is that number's second name.
    switch (method) {
    case token::size_token: return word::fixed_code<1, 6, 1, 1>;
    case token::empty_token: return word::fixed_code<1, 6, 1, 2>;
    case token::find_token: return word::fixed_code<1, 6, 1, 3>;
    case token::contains_token: return word::fixed_code<1, 6, 1, 4>;
    case token::substring_token: return word::fixed_code<1, 6, 1, 5>;
    case token::starts_with_token: return word::fixed_code<1, 6, 1, 6>;
    case token::ends_with_token: return word::fixed_code<1, 6, 1, 7>;
    case token::lower_token: return word::fixed_code<1, 6, 1, 8>;
    case token::upper_token: return word::fixed_code<1, 6, 1, 9>;
    case token::split_token: return word::fixed_code<1, 6, 1, 10>;
    case token::trim_token: return word::fixed_code<1, 6, 1, 11>;
    case token::replace_token: return word::fixed_code<1, 6, 1, 12>;
    case token::append_token: return word::fixed_code<1, 6, 1, 14>;
    case token::clear_token: return word::fixed_code<1, 6, 1, 15>;
    case token::at_token: return word::fixed_code<1, 6, 1, 16>;
    case token::resolved_token: return word::fixed_code<1, 6, 1, 17>;
    case token::to_string_token: return word::fixed_code<1, 6, 1, 19>;
    case token::to_number_token: return word::fixed_code<1, 6, 1, 20>;
    case token::to_binary_token: return word::fixed_code<1, 6, 1, 21>;
    case token::to_hexadecimal_token: return word::fixed_code<1, 6, 1, 22>;
    case token::reverse_token: return word::fixed_code<1, 6, 1, 23>;
    case token::foreground_token: return word::fixed_code<1, 6, 1, 24>;
    case token::background_token: return word::fixed_code<1, 6, 1, 25>;
    case token::add_token: return word::fixed_code<1, 6, 1, 26>;
    default: return 0;
    }
}

Value run_string_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context)
{
    // HOW MANY, AGAIN, for a receiver the checker could not see: a literal, a chain, an
    // item of a list. The same sentence string_method_check says before the run -- for the
    // methods string_method_arity counts; the others say their own, in their own files.
    const int wanted = string_method_arity(method);
    if (wanted >= 0 &&
        (arguments.size() != static_cast<std::size_t>(wanted) || (wanted > 0 && !had_parentheses))) {
        const std::string call = name + ".satellite." + token::method_name_of(method);
        context.refuse(satl_line_not_understood,
                       call + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
                           (had_parentheses ? ", and was given " + std::to_string(arguments.size())
                                            : ", in brackets after it"));
        return Value();
    }

    // THE NUMBER RUNS: table[code], one subtraction and one load (function_table.hpp), and the
    // function its file under satellite-numbers/words/ put in the row at start-up.
    const NumberRow *row = context.functions.method(number);
    if (row == nullptr || row->scenarios.string_call == nullptr) {
        context.refuse(not_built_yet, name + ".satellite." + token::method_name_of(method) + " is not built for a string yet");
        return Value();
    }
    return row->scenarios.string_call(receiver, home, arguments, had_parentheses, name, context);
}

Value character_of(const Value &text, const Value &index, const std::string &what, std::size_t where,
                   ExpressionContext &context)
{
    return string_method_parts::one_character(*text.as_string(), index, what + "[", "]", where, true, context);
}

} // namespace satellite004
