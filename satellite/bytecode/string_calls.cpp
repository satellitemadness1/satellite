// satellite/bytecode/string_calls.cpp -- what a string answers (M16). string_calls.hpp
// says what each method is and why positions count from 1; string_pieces.hpp holds the
// loops over the characters. This file is the arguments and the refusals.

#include "string_calls.hpp"

#include "bytecode_registry.hpp"
#include "program_walk.hpp"
#include "../machine/s_codes.hpp"
#include "../satellite_object/fast_paths.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/string_pieces.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <limits>
#include <utility>

namespace satellite004 {
namespace {

namespace fast = number_fast_path;
namespace pieces = string_pieces;

// WHICH ARGUMENTS ARE POSITIONS. Every other argument of a string's method is text.
bool takes_a_position(token::Code method)
{
    return method == token::at_token || method == token::substring_token;
}

std::string a_count_of(std::size_t n, const char *one, const char *many)
{
    return std::to_string(n) + " " + (n == 1 ? one : many);
}

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
    if (position == 0) {
        refuse(position_past_the_end,
               open + "0" + close + ": characters count from 1, so the first is " + open + "1" + close);
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

// `s.substring(start, end)`: both count from 1 and both are kept. 003's checks in 003's
// order -- a negative, then backwards, then past the end -- with 0 said as the one
// position a count from 1 does not have.
Value piece_of(const satellite_string &text, const std::vector<Value> &arguments, const std::string &call,
               ExpressionContext &context)
{
    unsigned long long int start = 0, end = 0;
    std::string start_written, end_written;
    if (!position_argument(arguments[0], start, start_written, call, context) ||
        !position_argument(arguments[1], end, end_written, call, context))
        return Value();
    const std::string spelled = call + "(" + start_written + ", " + end_written + ")";
    if (start == 0) {
        context.refuse(position_past_the_end, spelled + ": characters count from 1, so the first is 1");
        return Value();
    }
    if (start - 1 > end) {
        // end < start - 1 here, so end + 1 is a real count and cannot wrap.
        context.refuse(positions_backwards, spelled + " starts after it ends -- an empty piece is written with "
                                                     "its end one before its start, as " + call + "(" +
                                                     std::to_string(end + 1) + ", " + end_written + ")");
        return Value();
    }
    if (end > text.size()) {
        context.refuse(position_past_the_end,
                       spelled + ": there is no character " + end_written + " -- " + what_it_holds(text));
        return Value();
    }
    satellite_string out;
    text.substring(start - 1, end, out);
    return Value::of_string(std::move(out));
}

} // namespace

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

signed long long int string_method_check(const std::vector<std::bitset<16>> &row, std::size_t open,
                                         bool bracketed, std::size_t given, token::Code method,
                                         const std::string &spelling, std::string &why)
{
    const int wanted = string_method_arity(method);
    // A METHOD THAT TAKES NOTHING MAY BE WRITTEN WITH OR WITHOUT ITS BRACKETS, as a
    // container's is: `s.size` and `s.size()` are one read. One that takes something needs
    // them, and the count is judged here, before the line above it prints.
    if (given != static_cast<std::size_t>(wanted) || (wanted > 0 && !bracketed)) {
        why = spelling + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
              (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
        return satl_line_not_understood;
    }
    if (!bracketed || wanted == 0)
        return success;
    // A LITERAL THAT CAN NEVER BE RIGHT, one argument at a time: an argument is a lone
    // literal when a `,` or the `)` comes straight after it.
    std::size_t at = open + 1;
    for (int n = 0; n < wanted; ++n) {
        const token::Code code = code_at(row, at);
        std::size_t past = at;
        if (token::carries_a_count(code))
            skip_payload(row, past);
        else
            ++past;
        const token::Code after = code_at(row, past);
        const bool lone = after == token::comma_token || after == token::right_parenthesis_token;
        if (lone && takes_a_position(method) && code == token::string_token) {
            why = spelling + " takes a character's position -- a number, counting from 1 -- and was given text";
            return types_do_not_meet;
        }
        if (lone && !takes_a_position(method) &&
            (code == token::binary_token || code == token::hexadecimal_token || code == token::percentage_token)) {
            why = spelling + " takes text, and was given " +
                  (code == token::binary_token ? "a binary" : code == token::hexadecimal_token ? "a hex number" : "a percentage");
            return types_do_not_meet;
        }
        // On to the next argument: past this one's own brackets and payloads, to its comma.
        std::size_t depth = 0;
        while (at < row.size()) {
            const token::Code here = code_at(row, at);
            if (token::carries_a_count(here)) { skip_payload(row, at); continue; }
            if (here == token::line_end_token || here == token::end_of_file_token) return success;
            if (here == token::left_parenthesis_token || here == token::left_square_bracket_token ||
                here == token::left_brace_token)
                ++depth;
            else if (here == token::right_parenthesis_token || here == token::right_square_bracket_token ||
                     here == token::right_brace_token) {
                if (depth == 0) return success;
                --depth;
            } else if (here == token::comma_token && depth == 0) {
                ++at;
                break;
            }
            ++at;
        }
    }
    return success;
}

Value call_string_method(token::Code method, const Value &receiver, Value *home, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const satellite_string &text = *receiver.as_string();
    const char *spelled = token::method_name_of(method);
    const std::string call = name + "." + spelled;

    // HOW MANY, AGAIN, for a receiver the checker could not see: a literal, a chain, an
    // item of a list. The same sentence string_method_check says before the run.
    const int wanted = string_method_arity(method);
    if (arguments.size() != static_cast<std::size_t>(wanted) || (wanted > 0 && !had_parentheses)) {
        context.refuse(satl_line_not_understood,
                       call + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
                           (had_parentheses ? ", and was given " + std::to_string(arguments.size())
                                            : ", in brackets after it"));
        return Value();
    }

    switch (method) {
    case token::size_token:
        return Value::of_number(satellite_number(static_cast<unsigned long long int>(text.size())));
    case token::empty_token:
        return Value::of_bool(text.empty());
    case token::trim_token:
        return Value::of_string(pieces::trimmed(text));
    case token::resolved_token:
        // 003's live escapes (\threads, \home ...) were values and 004 has none (its
        // escapes are characters, worked out as the program is read), so there is nothing
        // left to resolve and the string answers itself -- the library's answer too.
        return receiver;
    case token::at_token:
        return one_character(text, arguments[0], call + "(", ")", 0, false, context);
    case token::substring_token:
        return piece_of(text, arguments, call, context);
    default:
        break;
    }

    // append AND clear CHANGE THE STRING, so they need the variable's own: a literal or a
    // piece of a chain is a string nothing holds, and changing it is a line that does
    // nothing (container_calls.cpp's words for `{1}.append(2)`). What they answer is not
    // used: the chain stays on the name, so `s.append("a").append("b")` appends twice.
    if (changes_a_string(method)) {
        satellite_string *own = home == nullptr ? nullptr : home->as_string();
        if (own == nullptr) {
            context.refuse(satl_line_not_understood, call + " changes a string, and this one has no name to change");
            return Value();
        }
        if (method == token::clear_token) {
            own->clear();
            return Value();
        }
        satellite_string made;
        const satellite_string *added = text_argument(arguments[0], made, call, context);
        if (added != nullptr)
            own->append(*added);            // an argument is its own copy, never the string it goes onto
        return Value();
    }

    // EVERY OTHER ONE TAKES TEXT: a string, or a number's digits (S020).
    satellite_string made_first, made_second;
    const satellite_string *first = text_argument(arguments[0], made_first, call, context);
    if (first == nullptr)
        return Value();

    switch (method) {
    case token::contains_token:
        return Value::of_bool(pieces::contains(text, *first));
    case token::starts_with_token:
        return Value::of_bool(pieces::starts_with(text, *first));
    case token::ends_with_token:
        return Value::of_bool(pieces::ends_with(text, *first));
    case token::find_token: {
        // str_find_str IS find, and has been since 2026-09-16 -- this only hands it text.
        Value answer;
        const signed long long int code =
            str_find_str(receiver, arguments[0].is_string() ? arguments[0] : Value::of_string(*first), answer);
        if (code != success) {
            context.refuse(code, call + " did not find it");   // 003's S0716: not found is not -1
            return Value();
        }
        return answer;
    }
    case token::split_token: {
        std::vector<satellite_string> parts = pieces::split(text, *first);
        std::vector<Value> items;
        items.reserve(parts.size());
        for (satellite_string &part : parts)
            items.push_back(Value::of_string(std::move(part)));
        return Value::of_list(make_list(std::move(items)));
    }
    case token::replace_token: {
        const satellite_string *second = text_argument(arguments[1], made_second, call, context);
        if (second == nullptr)
            return Value();
        if (first->empty()) {
            context.refuse(empty_search_text, call + " was given \"\" to look for, and an empty text is found "
                                                     "everywhere -- there is nothing to replace");
            return Value();
        }
        return Value::of_string(pieces::replace_all(text, *first, *second));
    }
    default:
        context.refuse(not_built_yet, call + " is not built for a string yet");
        return Value();
    }
}

Value character_of(const Value &text, const Value &index, const std::string &what, std::size_t where,
                   ExpressionContext &context)
{
    return one_character(*text.as_string(), index, what + "[", "]", where, true, context);
}

} // namespace satellite004
