// satellite/bytecode/fraction_check.cpp -- a fraction where none can go, said before anything runs.
// fraction_check.hpp says what is read and what is refused.

#include "fraction_check.hpp"

#include "bytecode_registry.hpp"
#include "convert_calls.hpp"
#include "word_codes.hpp"
#include "../machine/machine_codes.hpp"

#include <utility>

namespace satellite004 {
namespace {

using token::Code;

// WHAT A VALUE IS, AS FAR AS THE READER CAN SAY. by_worth is a binary or a hex, which meets a number by
// its worth and divides whole (his 9f); other is a kind that is surely no fraction -- a string, a bool.
enum class Is { unknown, number, floating, fraction, by_worth, other };

struct Read {
    Is is = Is::unknown;
    std::string text;   // the value as written, for the sentence: a / b, (a + 1) / 2, 5/4
};

// expression.cpp's precedence_of, the same numbers, so a / b + 1 is read here as it is worked out there.
int level_of(Code op)
{
    switch (op) {
    case token::power_token: return 7;
    case token::times_token:
    case token::divide_token:
    case token::modulus_token: return 6;
    case token::plus_token:
    case token::minus_token: return 5;
    case token::less_than_token:
    case token::greater_than_token:
    case token::less_or_equal_token:
    case token::greater_or_equal_token: return 4;
    case token::equals_token:
    case token::not_equals_token: return 3;
    case token::and_token: return 2;
    case token::or_token: return 1;
    default: return 0;
    }
}

const char *sign_of(Code op)
{
    switch (op) {
    case token::power_token: return "^";
    case token::times_token: return "*";
    case token::divide_token: return "/";
    case token::modulus_token: return "%";
    case token::plus_token: return "+";
    case token::minus_token: return "-";
    case token::less_than_token: return "<";
    case token::greater_than_token: return ">";
    case token::less_or_equal_token: return "<=";
    case token::greater_or_equal_token: return ">=";
    case token::equals_token: return "==";
    case token::not_equals_token: return "!=";
    case token::and_token: return "&&";
    case token::or_token: return "||";
    default: return "?";
    }
}

struct Reader {
    const std::vector<std::bitset<16>> &row;
    const LegalNames &declared;
    std::size_t at;
    bool lost = false;                       // a shape this reader does not read: it says nothing more
    signed long long int refused = success;  // and what it refused, when it did
    std::string why;

    Code code() const { return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0; }
};

Read expression(Reader &r, int lowest);

// A NAME'S KIND, BY THE WORD THAT DECLARED IT.
Is kind_declared(Code type)
{
    if (type == word::code_of(1, 6, 4)) return Is::number;
    if (type == word::code_of(1, 6, 10)) return Is::floating;
    if (type == word::code_of(1, 6, 20)) return Is::fraction;
    if (type == word::code_of(1, 6, 5) || type == word::code_of(1, 6, 11)) return Is::by_worth;
    return Is::unknown;
}

// ONE OPERAND, or lost.
Read operand(Reader &r)
{
    const Code code = r.code();
    if (code == token::number_token) {
        const std::string digits = text_at(r.row, r.at);
        if (r.code() == token::fraction_token) {             // 5/4, the literal: a fraction as written
            ++r.at;
            if (r.code() != token::number_token) {
                r.lost = true;
                return {};
            }
            const std::string bottom = text_at(r.row, r.at);
            return {Is::fraction, digits + "/" + bottom};
        }
        return {digits.find('.') == std::string::npos ? Is::number : Is::floating, digits};
    }
    if (code == token::binary_token || code == token::hexadecimal_token) {
        const std::string digits = text_at(r.row, r.at);
        const char marker = code == token::binary_token ? 'b' : 'x';
        return {Is::by_worth, !digits.empty() && digits[0] == marker ? digits : std::string(1, marker) + digits};
    }
    if (code == token::name_token) {
        const std::string name = text_at(r.row, r.at);
        const Code next = r.code();
        if (next == token::method_token || next == token::satellite_method_token ||
            next == token::left_square_bracket_token || next == token::left_parenthesis_token) {
            r.lost = true;                                    // an item, a method, a call: the run's to say
            return {};
        }
        const auto found = r.declared.find(name);
        return {found == r.declared.end() ? Is::unknown : kind_declared(found->second), name};
    }
    if (code == token::tight_minus_token) {                  // -5, -n: the same kind, turned over
        ++r.at;
        Read inside = operand(r);
        if (r.lost) return {};
        if (inside.is == Is::fraction) inside.is = Is::unknown;
        inside.text = "-" + inside.text;
        return inside;
    }
    if (code == token::left_parenthesis_token) {
        ++r.at;
        Read inside = expression(r, 1);
        if (r.lost || r.refused != success) return {};
        if (r.code() != token::right_parenthesis_token) {
            r.lost = true;
            return {};
        }
        ++r.at;
        inside.text = "(" + inside.text + ")";
        return inside;
    }
    if (is_convert_word(code) && r.at + 1 < r.row.size() &&
        static_cast<Code>(r.row[r.at + 1].to_ulong()) == token::left_parenthesis_token) {
        const std::string spelled(word::spelling_of(code));
        const Is is = code == word::code_of(1, 35, 2) ? Is::floating : Is::number;   // .float, else .number / .whole
        r.at += 2;
        Read inside = expression(r, 1);                       // a fraction inside is the point of the word
        if (r.lost || r.refused != success) return {};
        if (r.code() != token::right_parenthesis_token) {
            r.lost = true;
            return {};
        }
        ++r.at;
        return {is, spelled.substr(0, spelled.find('(')) + "(" + inside.text + ")"};
    }
    if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) {   // satellite.bool.false, .true
        ++r.at;
        return {Is::other, code == word::code_of(1, 17, 1) ? "satellite.bool.false" : "satellite.bool.true"};
    }
    if (code == token::string_token) {
        string_at(r.row, r.at);
        return {Is::other, "\"...\""};
    }
    r.lost = true;
    return {};
}

// TWO KINDS MET BY ONE OPERATOR -- what satellite_object.cpp answers for them, where this reader can say.
Is kind_of(Code op, Is left, Is right)
{
    if (level_of(op) <= 4)
        return Is::other;                                     // a comparison, && or ||: a bool
    if (left == Is::unknown || right == Is::unknown || left == Is::other || right == Is::other)
        return Is::unknown;
    const bool numbers = (left == Is::number || left == Is::by_worth) && (right == Is::number || right == Is::by_worth);
    // A FLOAT MEETS A NUMBER OR A FLOAT; a binary or a hex meeting one is the finale, refused when it runs.
    const bool floats = (left == Is::floating || right == Is::floating) && left != Is::by_worth &&
                        right != Is::by_worth;
    if (!numbers && !floats)
        return Is::unknown;
    if (op == token::divide_token)
        return left == Is::number && right == Is::number ? Is::fraction   // his 9a
             : numbers ? Is::number                                        // a binary or a hex: whole (9f)
             : Is::floating;
    if (op == token::power_token)
        return left == Is::floating ? Is::floating : Is::unknown;          // 2 ^ -1 is a float
    return numbers ? Is::number : Is::floating;
}

Read expression(Reader &r, int lowest)
{
    Read left = operand(r);
    while (!r.lost && r.refused == success) {
        const Code op = r.code();
        const int level = level_of(op);
        if (level == 0 || level < lowest)
            break;
        ++r.at;
        Read right = expression(r, op == token::power_token ? level : level + 1);
        if (r.lost || r.refused != success)
            break;
        // A FRACTION UNDER + - * / % ^ -- FRACTION ARITHMETIC IS NOT BUILT (object_fraction.cpp), in its words.
        if (level >= 5 && (left.is == Is::fraction || right.is == Is::fraction)) {
            const std::string &fraction = left.is == Is::fraction ? left.text : right.text;
            r.refused = not_built_yet;
            r.why = fraction + " is a fraction, and the " + sign_of(op) +
                    " of a fraction is fraction arithmetic, which is not built yet -- write satellite.convert.number(" +
                    fraction + ") for the whole number nearest it, or satellite.convert.float(" + fraction +
                    ") for the float";
            break;
        }
        left = {kind_of(op, left.is, right.is), left.text + " " + sign_of(op) + " " + right.text};
    }
    return left;
}

} // namespace

signed long long int fraction_given_where_none_goes(token::Code type, const std::string &name,
                                                    const std::vector<std::bitset<16>> &row, std::size_t value_at,
                                                    const LegalNames &declared, std::string &why)
{
    const bool a_number = type == word::code_of(1, 6, 4);
    const bool a_float = type == word::code_of(1, 6, 10);
    if (!a_number && !a_float && type != word::code_of(1, 6, 20))
        return success;
    Reader r{row, declared, value_at, false, success, std::string()};
    const Read value = expression(r, 1);
    if (r.refused != success) {
        why = r.why;
        return r.refused;
    }
    const Code after = r.code();
    if (r.lost || (after != token::line_end_token && after != token::comment_token && after != token::end_of_file_token))
        return success;                                       // not read to its end: the walker's to judge
    if (value.is != Is::fraction || (!a_number && !a_float))
        return success;
    why = name + " was declared " + std::string(word::spelling_of(type)) + ", and " + value.text +
          " is a fraction -- declare a fraction, or write satellite.convert." + (a_number ? "number(" : "float(") +
          value.text + ")";
    return types_do_not_meet;
}

} // namespace satellite004
