// satellite/bytecode/color_check.cpp -- WHAT THE CHECKER ASKS ABOUT A COLOR, BEFORE
// ANYTHING RUNS (color_values.hpp declares it; color_values.cpp is the walker's half).
//
// (the author, 2026-09-22) "satellite.variable.color my_color = x000000 or just
// 000000 without the x, but a variable.color is always width 6 hexadecimal number".
//
// THE x IS OPTIONAL ON A COLOUR, and that is his word, where a hex's x is required.
// ON THE LINE THAT DECLARES ONE the lexer already reads the value as hex digits
// (bytecode_registry.cpp, "A COLOUR WRITTEN WITHOUT ITS x"), so `000000`, `ff00aa`
// and `00ff00` all arrive as a hex. ON A LATER `c = ...` THE LEXER CANNOT HELP --
// it does not know what c was declared as -- so the digits arrive as whatever
// they look like, and this file and the walker read them back:
//
//     c = x00ff00     a hex, as always                              accepted
//     c = 000000      the number token keeps its six digits         accepted
//     c = ff00aa      a name no variable has, all hex digits        accepted
//     c = 00ff00      the number 00 and then the name ff00          ERROR: write x00ff00
//
// THE LAST IS REFUSED, NOT GUESSED, because the lexer keeps no blanks: `00ff00`
// and `00 ff00` are the same two tokens, and only one of them is a colour. The
// sentence names the one spelling that always works. If the author rules that
// every later line must write its x, it is color_names_start answering `from` and
// the two "accepted" rows below refusing with "write x...".

#include "color_values.hpp"
#include "color_reading.hpp"

#include <string>

namespace satellite004 {
namespace {

using namespace color_reading;

// THE COMMA AFTER THE VALUE, at the value's own depth -- a comma inside brackets
// belongs to a call -- or kNoComma when there is none. Payloads are skipped, never read.
constexpr std::size_t kNoComma = static_cast<std::size_t>(-1);

std::size_t the_comma_after(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    long long int depth = 0;
    while (at < row.size()) {
        const token::Code code = code_at(row, at);
        if (code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token)
            break;
        if (token::carries_a_count(code)) { skip_payload(row, at); continue; }
        if (code == token::left_parenthesis_token || code == token::left_square_bracket_token ||
            code == token::left_brace_token)
            ++depth;
        else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token ||
                 code == token::right_brace_token)
            --depth;
        else if (code == token::comma_token && depth == 0)
            return at;
        ++at;
    }
    return kNoComma;
}

// THE DIGITS, BEFORE ANYTHING RUNS. `code` and `text` are the value's first token,
// `past` the code after it. A value that is more than one literal -- a sum, a
// method on it -- is left to the walker, which knows what it answers.
signed long long int the_digits_are_right(token::Code code, const std::string &text,
                                          const std::vector<std::bitset<16>> &row, std::size_t past,
                                          const std::unordered_map<std::string, token::Code> &declared,
                                          bool a_sign, std::string &why)
{
    const token::Code next = code_at(row, past);
    const bool a_variable = declared.find(text) != declared.end();

    // A NAME THAT IS A VARIABLE IS THE VARIABLE, and the walker judges what it holds.
    if (code == token::name_token && a_variable)
        return success;

    // HEX DIGITS THAT ARE ALSO A VARIABLE'S NAME. On the line that declares a colour
    // the lexer reads any run of hex digits as the colour's digits, so
    // `satellite.variable.color d = facade` would give d xFACADE and not what the
    // variable facade holds -- a wrong answer without a word. The token cannot say
    // whether an x was typed, so this is refused, and both ways out always work: the
    // variable on the next line, where a name is a name, or the digits in the other
    // case, which no variable is spelled as (names are matched exactly).
    if (code == token::hexadecimal_token && a_variable) {
        bool has_lower = false;
        for (const char c : text) has_lower = has_lower || (c >= 'a' && c <= 'f');
        std::string other_case = text;
        for (char &c : other_case)
            c = has_lower ? (c >= 'a' && c <= 'f' ? static_cast<char>(c - 'a' + 'A') : c)
                          : (c >= 'A' && c <= 'F' ? static_cast<char>(c - 'A' + 'a') : c);
        why = "ERROR: " + text + " is read here as hex digits, and it is also a variable -- on the line that "
              "declares a color, a run of hex digits is the color's digits. For the variable, declare the color "
              "first and give it " + text + " on the next line; " +
              (text.size() == 6 ? "for the digits, write x" + other_case
                                : "as digits it is " + how_many_digits(text.size()) + ", and " + kSixDigits);
        return types_do_not_meet;
    }
    if (code == token::name_token && !all_hex_digits(text)) {
        // x AND THEN SOMETHING THAT IS NOT HEX -- xGG0000 -- which the lexer made a
        // name. The hex's rule: letters and digits after the x, at least one digit.
        bool a_digit = false, only_letters_and_digits = text.size() >= 2 && text[0] == 'x';
        for (std::size_t i = 1; i < text.size() && only_letters_and_digits; ++i) {
            const char c = text[i];
            only_letters_and_digits = (c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
            a_digit = a_digit || (c >= '0' && c <= '9');
        }
        if (only_letters_and_digits && a_digit) {
            why = "ERROR: " + text + " is not hex -- a hex digit is 0 to 9 or A to F";
            return types_do_not_meet;
        }
        return success;                  // names_in_statement says it has no declaration
    }

    if (code == token::number_token) {
        // 0x00FF00, THE C SPELLING: the number 0 and then an x literal.
        if (text == "0" && next == token::hexadecimal_token) {
            std::size_t digits = past;
            why = "ERROR: expected x" + text_at(row, digits);
            return types_do_not_meet;
        }
        // 00ff00: THE NUMBER 00 AND THEN THE NAME ff00. Put back together, to say
        // what to write -- a number straight after which a name is written is never
        // a sum, so one run of digits is the only reading that helps.
        if (next == token::name_token) {
            std::size_t rest = past;
            const std::string joined = text + text_at(row, rest);
            why = all_hex_digits(joined)
                      ? "ERROR: write x" + joined + " -- on a line after the one that declares a color, digits "
                                                   "that start with a number and hold a letter need their x"
                      : "ERROR: " + joined + " is not hex -- a hex digit is 0 to 9 or A to F";
            return types_do_not_meet;
        }
        if (the_value_ends(next) && !all_digits(text)) {
            why = "ERROR: " + text + " is not a color -- " + kSixDigits;
            return types_do_not_meet;
        }
    }

    if (!the_value_ends(next))
        return success;
    if (a_sign) {
        why = "ERROR: a color has no sign, and this one has a minus in front of it -- x000000 is black, and "
              "there is no color below it";
        return types_do_not_meet;
    }
    if (text.size() != 6) {
        why = "ERROR: " + text + " is " + how_many_digits(text.size()) + " -- " + kSixDigits;
        return types_do_not_meet;
    }
    return success;
}

// THE AUTHOR'S SECOND WAY OF WRITING A TRANSPARENCY: "my_color = x000000, 0-99 for
// transparency". Only a number written out is judged here; a name or a sum is
// judged by the walker, which knows what it holds.
signed long long int the_transparency_is_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                               std::string &why)
{
    const std::size_t comma = the_comma_after(row, at);
    if (comma == kNoComma)
        return success;
    std::size_t k = comma + 1;
    const token::Code first = code_at(row, k);
    if (first == token::line_end_token || first == token::comment_token || first == token::end_of_file_token) {
        why = "ERROR: a comma after a color is its transparency, 0 to 99, and nothing is written after this one";
        return satl_line_not_understood;
    }
    if (the_comma_after(row, k) != kNoComma) {
        why = "ERROR: a color takes one transparency after its comma, 0 to 99, and this line has two commas";
        return satl_line_not_understood;
    }
    bool a_sign = false;
    for (; code_at(row, k) == token::tight_minus_token || code_at(row, k) == token::minus_token; ++k)
        a_sign = true;
    if (code_at(row, k) != token::number_token)
        return success;
    std::size_t past = k;
    const std::string text = text_at(row, past);
    const token::Code next = code_at(row, past);
    if (next != token::line_end_token && next != token::comment_token && next != token::end_of_file_token)
        return success;                  // a sum: the walker works it out
    std::size_t first_digit = 0;
    while (first_digit + 1 < text.size() && text[first_digit] == '0') ++first_digit;
    if (a_sign || !all_digits(text) || text.size() - first_digit > 2) {
        why = "ERROR: " + std::string(kWhatATransparencyIs) + ", and this is " + (a_sign ? "-" : "") + text;
        return types_do_not_meet;
    }
    return success;
}

} // namespace

// THE VALUE AND THEN THE TRANSPARENCY, each judged where a literal is written.
// Only the first value, inside any brackets, its minus signs noted for the
// refusal -- a colour has no sign.
signed long long int color_is_written_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                            const std::unordered_map<std::string, token::Code> &declared,
                                            std::string &why)
{
    bool a_sign = false;
    std::size_t k = at;
    for (;; ++k) {
        const token::Code ahead = code_at(row, k);
        if (ahead == token::tight_minus_token || ahead == token::minus_token)
            a_sign = true;
        else if (ahead != token::left_parenthesis_token)
            break;
    }
    const token::Code code = code_at(row, k);
    if (code == token::hexadecimal_token || code == token::number_token || code == token::name_token) {
        std::size_t past = k;
        const std::string text = text_at(row, past);
        const signed long long int judged = the_digits_are_right(code, text, row, past, declared, a_sign, why);
        if (judged != success)
            return judged;
    }
    return the_transparency_is_right(row, at, why);
}

// `c = ff00aa` IS SIX HEX DIGITS AND NOT A NAME, when no variable has that name and
// nothing but a comma or the line's end follows -- the same test the walker reads
// it by (color_values.cpp). A declared name always wins: it is the variable.
std::size_t color_names_start(const std::vector<std::bitset<16>> &row, std::size_t at, std::size_t from,
                              const std::unordered_map<std::string, token::Code> &declared)
{
    if (code_at(row, at) != token::name_token)
        return from;
    std::size_t past = at;
    const std::string text = text_at(row, past);
    if (text.size() != 6 || !all_hex_digits(text) || declared.find(text) != declared.end() ||
        !the_value_ends(code_at(row, past)))
        return from;
    return past;
}

// THE CHECKER'S ANSWER IS FINAL FOR A COLOUR NAME (program_check.cpp's
// method_right_for), so this is the whole list: `.transparency`, and the four
// conversions. `.reverse()` is let through before this is asked, as it is on every
// type, and a colour refuses it where it runs: a colour has no order to reverse.
signed long long int color_method_check(token::Code method, const std::string &spelling, std::string &why)
{
    if (method == token::transparency_token || method == token::to_string_token ||
        method == token::to_number_token || method == token::to_binary_token ||
        method == token::to_hexadecimal_token)
        return success;
    why = spelling + " -- " + kWhatAColorHas;
    return types_do_not_meet;
}

} // namespace satellite004
