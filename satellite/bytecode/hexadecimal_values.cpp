// satellite/bytecode/hexadecimal_values.cpp -- what the checker and the walker ask about
// a hex (hexadecimal_values.hpp says why this is its own file).
//
// (the author, 2026-09-22) "similar thing for hex, which has an x in front of it,
// with hex numbers only for it 0 - 9 A - F". And the same evening, of the binary's
// b: "just make the prefix b mandatory and move on". THE x IS MANDATORY BY THE SAME
// READING -- his words were about the b, and a hex is "similar" -- so a value
// given to a satellite.variable.hex without its x is refused before anything runs,
// exactly as a binary without its b is. If he rules the x optional, it is
// hexadecimal_is_written_right below answering success, and nothing else.

#include "hexadecimal_values.hpp"

#include "file_calls.hpp"
#include "program_walk.hpp"
#include "word_codes.hpp"

#include <string>
#include <utility>

namespace satellite004 {
namespace {

bool a_hex_digit(char c) { return (c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'); }

bool all_hex_digits(const std::string &text)
{
    if (text.empty()) return false;
    for (const char c : text)
        if (!a_hex_digit(c)) return false;
    return true;
}

// WHAT A HEX HAS, said whole in every refusal of a method it has not got, so a
// person reads the answer where they read the question.
const char *const kWhatAHexHas = "a hex has .number, .string, .binary, .hex, .width, .add and .reverse()";

// The hex a number would be written as, for a suggestion: 32 is x20, -31 is -x1F.
std::string as_a_hex(const satellite_number &value)
{
    return (value.negative() ? "-x" : "x") + (value.negative() ? -value : value).to_radix_text(16);
}

} // namespace

// AN x LITERAL IS A satellite.variable.hex, its width kept: x00FF is four digits
// and displays as x00FF, not as 255 (003 DESIGN 8.5). The lexer only makes
// hexadecimal_token out of digits 0-9 and A-F, so from_digits refusing here means
// the lexer and this disagree. It was the number 31 for x1F until 2026-09-22 --
// and it still is worth 31 in every sum (object_hexadecimal.cpp).
signed long long int hexadecimal_literal(const std::string &digits, Value &out, std::string &why)
{
    satellite_hexadecimal_number hex;
    const signed long long int held = satellite_hexadecimal_number::from_digits(digits, hex);
    if (held != success) {
        why = "x" + digits + " is not a hex this can read -- a hex digit is 0 to 9 or A to F";
        return held;
    }
    out = Value::of_hexadecimal(std::move(hex));
    return success;
}

// A HEX IS WRITTEN WITH ITS x, BEFORE ANYTHING RUNS, in binary_is_written_with_b's
// words (program_check.cpp says why each clause there is): `= 1F` is ERROR:
// expected x1F. Only the first value, inside any brackets, its minus signs turning
// the suggestion over as they would the value.
//
// THE LEXER HAS ALREADY TAKEN THE MISTAKE APART, so it is put back together here.
// `1F` is the number 1 and then the name F; `FF` is a name; `10` is a number;
// `0x1F` is the number 0 and then an x literal. A number straight after which a
// name is written is never a sum, so reading the two as one run of digits is the
// only reading that sends a person anywhere useful.
signed long long int hexadecimal_is_written_right(const std::vector<std::bitset<16>> &row, std::size_t at,
                                                  const std::unordered_map<std::string, token::Code> &declared,
                                                  std::string &why)
{
    bool below_zero = false;
    for (;; ++at) {
        const token::Code ahead = code_at(row, at);
        if (ahead == token::tight_minus_token || ahead == token::minus_token)
            below_zero = !below_zero;
        else if (ahead != token::left_parenthesis_token)
            break;
    }
    const std::string sign = below_zero ? "-" : "";
    const token::Code code = code_at(row, at);
    if (code != token::number_token && code != token::name_token)
        return success;
    std::size_t k = at;
    std::string entered = text_at(row, k);

    // 0x1F AND 0b1010, the C and Python spellings.
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::hexadecimal_token) {
        std::size_t digits = k;
        why = "ERROR: expected " + sign + "x" + text_at(row, digits);
        return types_do_not_meet;
    }
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::binary_token) {
        std::size_t digits = k;
        why = "ERROR: " + sign + "0b" + text_at(row, digits) +
              " is not hex -- a hex is x and then 0 to 9 and A to F, like x1F";
        return types_do_not_meet;
    }

    if (code == token::number_token) {
        if (code_at(row, k) == token::name_token) {
            std::size_t rest = k;
            entered += text_at(row, rest);
        }
        why = all_hex_digits(entered) ? "ERROR: expected " + sign + "x" + entered
                                      : "ERROR: " + sign + entered + " is not hex -- a hex digit is 0 to 9 or A to F";
        return types_do_not_meet;
    }

    // A NAME: a declared one is a variable, and the walker judges what it holds.
    if (declared.find(entered) != declared.end())
        return success;
    if (all_hex_digits(entered)) {
        why = "ERROR: expected " + sign + "x" + entered;
        return types_do_not_meet;
    }
    // x AND THEN SOMETHING THAT IS NOT HEX -- x1G -- which the lexer made a name.
    // Only letters and digits after the x, and at least one digit, so `xray` is
    // still told it has no satellite.variable line, which is the truer sentence.
    if (entered.size() >= 2 && entered[0] == 'x') {
        bool a_digit = false;
        for (std::size_t i = 1; i < entered.size(); ++i) {
            const char c = entered[i];
            if (!((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')))
                return success;
            a_digit = a_digit || (c >= '0' && c <= '9');
        }
        if (a_digit) {
            why = "ERROR: " + sign + entered + " is not hex -- a hex digit is 0 to 9 or A to F";
            return types_do_not_meet;
        }
    }
    return success;
}

// THE CHECKER'S ANSWER IS FINAL FOR A HEX NAME (program_check.cpp's method_right_for),
// so this is the whole list: the four conversions, `.width`, and `.add`, which is
// `+` and reaches object_hexadecimal.cpp. `.reverse()` is allowed before this is
// asked, as it is on every type with an order.
signed long long int hexadecimal_method_check(token::Code method, const std::string &spelling, std::string &why)
{
    if (method == token::to_string_token || method == token::to_number_token || method == token::to_binary_token ||
        method == token::to_hexadecimal_token || method == token::width_token || method == token::add_token)
        return success;
    why = spelling + " -- " + kWhatAHexHas;
    return types_do_not_meet;
}

// A HEX GIVEN TO A satellite.variable.number NAME KEEPS WHAT IT IS WORTH, which is
// what `satellite.variable.number n = x1F` did when an x literal was a number --
// program_walk.cpp does the same for a binary. A hex given to a hex name stays a hex.
//
// A NUMBER GIVEN TO A HEX NAME IS REFUSED, as a number given to a binary name is:
// `h = h + x01` answers a number (object_hexadecimal.cpp), and making it a hex
// again would be choosing a width the program never wrote. The sentence says both
// ways out, where the binary's says only what the name holds. If the author rules
// that a sum on a hex stays a hex, this is where the number is wrapped back up.
signed long long int hexadecimal_on_store(token::Code holds, Value &value, std::string &why)
{
    if (holds == word::code_of(1, 6, 4) && value.is_hexadecimal()) {
        value = Value::of_number(value.as_hexadecimal()->worth);
        return success;
    }
    if (holds == word::code_of(1, 6, 11) && value.is_number()) {
        why = "it was given the number " + value.as_number()->to_text() +
              " -- a hex name holds only a hex, and arithmetic on a hex answers a number, so keep that "
              "in a satellite.variable.number or write the hex it should be: " + as_a_hex(*value.as_number());
        return types_do_not_meet;
    }
    return success;
}

// A HEX'S METHODS. The four conversions and `.width` are answered here and never
// by the conversions every type shares, which know nothing of a width: `.hex` is
// the digits as wide as written, as a binary's `.bin` is (object_hexadecimal.cpp).
//
// `.width` COUNTS BITS, FOUR TO THE DIGIT -- x00FF is 16 -- which is the author's
// call in 003 (DESIGN 8.5, 2026-09-09: "width() COUNTS BITS ON BOTH AND digits()
// COUNTS DIGITS"), so that a width means the same thing on a binary and a hex.
// 004 has no `.digits` method token yet; `.hex`'s text is as long as the digits.
Value hexadecimal_method(token::Code method, const Value &receiver, Value *, const std::vector<Value> &arguments,
                         bool, const std::string &name, ExpressionContext &context, bool &answered)
{
    answered = false;
    const satellite_hexadecimal_number *hex = receiver.as_hexadecimal();
    if (hex == nullptr || method == token::add_token)
        return Value();                      // `.add` IS `+`: the shared path asks the object model

    const std::string spelled = name + "." + method_spelling(method);
    const bool built = method == token::to_string_token || method == token::to_number_token ||
                       method == token::to_binary_token || method == token::to_hexadecimal_token ||
                       method == token::width_token;
    if (!built) {
        context.refuse(types_do_not_meet, spelled + " -- " + kWhatAHexHas);
        return Value();
    }
    if (!arguments.empty()) {
        context.refuse(satl_line_not_understood,
                       spelled + " takes no argument, and was given " + std::to_string(arguments.size()));
        return Value();
    }

    answered = true;
    if (method == token::width_token)
        return Value::of_number(satellite_number(hex->width) * satellite_number(4ull));
    if (method == token::to_number_token)
        return Value::of_number(hex->worth);

    satellite_string text;
    std::size_t bad_offset = 0;
    const std::string written = method == token::to_string_token ? hex->written()
                                : method == token::to_binary_token ? hex->bits()
                                                                    : hex->digits();
    const signed long long int made = satellite_string::from_utf8(written, text, bad_offset);
    if (made != success) {
        answered = false;
        context.refuse(made, spelled + " could not be written as text");
        return Value();
    }
    return Value::of_string(std::move(text));
}

} // namespace satellite004
