// satellite/bytecode/string_check.cpp -- what a string's methods refuse BEFORE ANYTHING RUNS
// (M16). string_calls.cpp is the walker's half and says what each method does; this is the
// checker's, as color_check.cpp is a colour's: a count that is wrong, and a literal that can
// never be right in its place, refused in the walker's own sentence and with its machine
// code -- only sooner, so the lines above it never print first.
//
// A LITERAL'S KIND IS KNOWN FROM ITS TOKEN ALONE, which is the whole reason these can be
// judged here (ERRORS2 #8's rule, 2026-09-26: "a literal a name cannot take is refused
// before anything runs, not after the lines above it"). A value worked out -- a name, a
// call, a sum, a minus sign -- is still judged when it runs.
//
// NOT JUDGED HERE, AND ON PURPOSE: a float or a bool where TEXT goes. The walker refuses
// both (S301), but the author's ruling of 2026-09-16 -- "just convert the number to the
// string" -- may yet cover 4.5, so whether they are refused at all is his to say.

#include "string_calls.hpp"

#include "bytecode_registry.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/s_codes.hpp"

namespace satellite004 {
namespace {

// WHICH ARGUMENTS ARE POSITIONS. Every other argument of a string's method is text.
bool takes_a_position(token::Code method)
{
    return method == token::at_token || method == token::substring_token;
}

// WHAT A LONE LITERAL IS: one literal, and then `closer` or a comma straight after it.
enum class Lone { not_lone, text, whole, decimal, fraction, binary, hexadecimal, percentage, boolean };

Lone lone_literal_at(const std::vector<std::bitset<16>> &row, std::size_t at, token::Code closer, std::string &written)
{
    std::size_t k = at;
    const token::Code code = code_at(row, k);
    Lone kind = Lone::not_lone;
    if (code == token::string_token) {
        written = text_at(row, k);
        kind = Lone::text;
    } else if (code == token::number_token) {
        written = text_at(row, k);
        kind = written.find('.') == std::string::npos ? Lone::whole : Lone::decimal;
        if (code_at(row, k) == token::fraction_token) {        // 1/2, touching, is one fraction
            ++k;
            if (code_at(row, k) != token::number_token)
                return Lone::not_lone;
            text_at(row, k);
            kind = Lone::fraction;
        }
    } else if (code == token::binary_token || code == token::hexadecimal_token || code == token::percentage_token) {
        written = text_at(row, k);
        kind = code == token::binary_token ? Lone::binary
             : code == token::hexadecimal_token ? Lone::hexadecimal : Lone::percentage;
    } else if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) {   // satellite.bool.false, .true
        ++k;
        kind = Lone::boolean;
    }
    const token::Code after = code_at(row, k);
    return after == closer || after == token::comma_token ? kind : Lone::not_lone;
}

// What the walker calls it when a position was wanted (value.kind_name()).
const char *kind_named(Lone kind)
{
    switch (kind) {
    case Lone::text: return "a string";
    case Lone::decimal: return "a float";
    case Lone::fraction: return "a fraction";
    case Lone::binary: return "a binary";
    case Lone::hexadecimal: return "a hex";
    case Lone::percentage: return "a percentage";
    case Lone::boolean: return "a bool";
    default: return "a number";
    }
}

bool zero(Lone kind, const std::string &written)
{
    return kind == Lone::whole && written.find_first_not_of('0') == std::string::npos;
}

// THE BRACKET THAT CLOSES THE ONE AT `open`, or 0 when it never closes on its line.
// Payloads are skipped whole, so a bracket inside a string is not one.
std::size_t closing_bracket(const std::vector<std::bitset<16>> &row, std::size_t open)
{
    std::size_t depth = 0;
    for (std::size_t at = open; at < row.size();) {
        const token::Code here = code_at(row, at);
        if (token::carries_a_count(here)) { skip_payload(row, at); continue; }
        if (here == token::line_end_token || here == token::end_of_file_token) return 0;
        if (here == token::left_parenthesis_token || here == token::left_square_bracket_token ||
            here == token::left_brace_token)
            ++depth;
        else if ((here == token::right_parenthesis_token || here == token::right_square_bracket_token ||
                  here == token::right_brace_token) && --depth == 0)
            return at;
        ++at;
    }
    return 0;
}

// WHERE EACH ARGUMENT STARTS, from the `(` at `open`: past each one's own brackets and
// payloads to its comma at depth 0.
std::vector<std::size_t> argument_starts(const std::vector<std::bitset<16>> &row, std::size_t open, int wanted)
{
    std::vector<std::size_t> starts;
    std::size_t at = open + 1;
    for (int n = 0; n < wanted; ++n) {
        starts.push_back(at);
        std::size_t depth = 0;
        while (at < row.size()) {
            const token::Code here = code_at(row, at);
            if (token::carries_a_count(here)) { skip_payload(row, at); continue; }
            if (here == token::line_end_token || here == token::end_of_file_token) return starts;
            if (here == token::left_parenthesis_token || here == token::left_square_bracket_token ||
                here == token::left_brace_token)
                ++depth;
            else if (here == token::right_parenthesis_token || here == token::right_square_bracket_token ||
                     here == token::right_brace_token) {
                if (depth == 0) return starts;
                --depth;
            } else if (here == token::comma_token && depth == 0) {
                ++at;
                break;
            }
            ++at;
        }
    }
    return starts;
}

// THE LITERALS IN A METHOD'S BRACKETS, in the walker's order: every argument's kind first,
// left to right, then what the value itself cannot be -- a first character of 0, or an
// empty text to replace.
signed long long int literals_given(const std::vector<std::bitset<16>> &row, std::size_t open, int wanted,
                                    token::Code method, const std::string &spelling, std::string &why)
{
    const std::vector<std::size_t> starts = argument_starts(row, open, wanted);
    std::vector<Lone> kinds;
    std::vector<std::string> written(starts.size());
    for (std::size_t n = 0; n < starts.size(); ++n)
        kinds.push_back(lone_literal_at(row, starts[n], token::right_parenthesis_token, written[n]));
    for (const Lone kind : kinds) {
        if (takes_a_position(method) && kind == Lone::text) {
            why = spelling + " takes a character's position -- a number, counting from 1 -- and was given text";
            return types_do_not_meet;
        }
        if (takes_a_position(method) && kind != Lone::not_lone && kind != Lone::whole) {
            why = spelling + " takes a character's position -- a number, counting from 1 -- and was given " +
                  kind_named(kind);
            return types_do_not_meet;
        }
        if (!takes_a_position(method) &&
            (kind == Lone::binary || kind == Lone::hexadecimal || kind == Lone::percentage)) {
            why = spelling + " takes text, and was given " +
                  (kind == Lone::binary ? "a binary" : kind == Lone::hexadecimal ? "a hex number" : "a percentage");
            return types_do_not_meet;
        }
    }
    // `s.at(0)` and `s.substring(0, n)`: the one position a count from 1 does not have.
    if (takes_a_position(method) && !kinds.empty() && zero(kinds[0], written[0])) {
        if (method == token::at_token) {
            why = spelling + "(0): characters count from 1, so the first is " + spelling + "(1)";
        } else {
            const std::string end = kinds.size() > 1 && kinds[1] == Lone::whole ? written[1] : "...";
            why = spelling + "(0, " + end + "): characters count from 1, so the first is 1";
        }
        return position_past_the_end;
    }
    // `s.replace("", x)`: an empty text is found everywhere, so there is nothing to replace.
    if (method == token::replace_token && !kinds.empty() && kinds[0] == Lone::text && written[0].empty()) {
        why = spelling + " was given \"\" to look for, and an empty text is found everywhere -- there is "
                         "nothing to replace";
        return empty_search_text;
    }
    return success;
}

} // namespace

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
    if (bracketed && wanted > 0) {
        const signed long long int literals = literals_given(row, open, wanted, method, spelling, why);
        if (literals != success)
            return literals;
    }
    // `s.split(",")[2]` -- [ ] AFTER THE ANSWER (the M16 review). A string's methods answer a
    // string, a number, a bool or a list of strings, never an object, so every link after
    // this one is a method, and a `[` after the last is the not-built index the walker
    // refuses -- said here, before anything runs.
    std::string spelled = spelling;
    std::size_t k = open;
    if (bracketed) {
        spelled += given == 0 ? "()" : "(...)";
        const std::size_t close = closing_bracket(row, open);
        if (close == 0)
            return success;
        k = close + 1;
    }
    while (code_at(row, k) == token::method_token && token::is_method_code(code_at(row, k + 1))) {
        spelled += link_spelled(row, k);
        k += 2;
        if (code_at(row, k) == token::left_parenthesis_token) {
            const std::size_t close = closing_bracket(row, k);
            if (close == 0)
                return success;
            k = close + 1;
        }
    }
    if (code_at(row, k) == token::left_square_bracket_token) {
        why = index_after_an_answer(spelled);
        return satl_line_not_understood;
    }
    return success;
}

signed long long int string_index_check(const std::vector<std::bitset<16>> &row, std::size_t open,
                                        const std::string &name, std::string &why)
{
    std::string written;
    const Lone kind = lone_literal_at(row, open + 1, token::right_square_bracket_token, written);
    if (kind == Lone::not_lone || (kind == Lone::whole && !zero(kind, written)))
        return success;
    if (kind == Lone::whole) {
        why = name + "[0]: characters count from 1, so the first is " + name + "[1]";
        return position_past_the_end;
    }
    why = name + "[...] takes a character's position -- a number, counting from 1 -- and was given " + kind_named(kind);
    return types_do_not_meet;
}

} // namespace satellite004
