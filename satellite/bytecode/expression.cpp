// satellite/bytecode/expression.cpp -- the header says which token triggers
// which fast path. The author ruled on `^` on 2026-09-16: it is power, and every
// math sign needs a space on both sides.
//
// PRECEDENCE CLIMBING, not a tree. One function per expression, a loop per
// level, and the operands live in two locals: an expression of any depth costs
// the C++ stack and nothing else, which is the same promise run_body makes about
// a statement. 003 DESIGN §6 spells the grammar rule "precedence climbing over
// the operators in §6.6" in as many words, so this is the port, not an invention.

#include "expression.hpp"

#include "word_codes.hpp"
#include "../satellite_variable_number/number_arithmetic.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <utility>

namespace satellite004 {
namespace {

using token::Code;
namespace fast = number_fast_path;

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

// §6.6's table, plus power at the top. 0 means "not an operator", which is how
// an expression finds its own end: a `)`, a `,` or a line_end_token all land here.
int precedence_of(Code op)
{
    switch (op) {
    case token::power_token: return 5;              // `^`, ruled power by the author 2026-09-16
    case token::times_token:
    case token::divide_token:
    case token::modulus_token: return 4;
    case token::plus_token:
    case token::minus_token: return 3;
    case token::less_than_token:
    case token::greater_than_token:
    case token::less_or_equal_token:
    case token::greater_or_equal_token: return 2;
    case token::equals_token:
    case token::not_equals_token: return 1;
    default: return 0;
    }
}

// THE WIRING ITSELF: a token to one of the six fast paths. nullptr for the
// comparisons, which answer a flag rather than a number and are done below.
fast::Operation fast_path_of(Code op)
{
    switch (op) {
    case token::plus_token: return fast::add;
    case token::minus_token: return fast::subtract;
    case token::times_token: return fast::multiply;
    case token::divide_token: return fast::divide;
    case token::modulus_token: return fast::modulus;
    case token::power_token: return fast::power;
    default: return nullptr;
    }
}

const char *spelling_of(Code op)
{
    switch (op) {
    case token::plus_token: return "+";
    case token::minus_token: return "-";
    case token::times_token: return "*";
    case token::divide_token: return "/";
    case token::modulus_token: return "%";
    case token::power_token: return "^";
    case token::less_than_token: return "<";
    case token::greater_than_token: return ">";
    case token::less_or_equal_token: return "<=";
    case token::greater_or_equal_token: return ">=";
    case token::equals_token: return "==";
    case token::not_equals_token: return "!=";
    default: return "an operator";
    }
}

bool an_ordering(Code op)
{
    return op == token::less_than_token || op == token::greater_than_token ||
           op == token::less_or_equal_token || op == token::greater_or_equal_token;
}

bool holds(int order, Code op)
{
    switch (op) {
    case token::less_than_token: return order < 0;
    case token::greater_than_token: return order > 0;
    case token::less_or_equal_token: return order <= 0;
    case token::greater_or_equal_token: return order >= 0;
    case token::equals_token: return order == 0;
    case token::not_equals_token: return order != 0;
    default: return false;
    }
}

// The radix a literal token already decided. The lexer strips the b and the x
// (bytecode_registry.cpp:241), so the digits never carry a prefix and this is
// the only thing that knows which base they are in.
unsigned int radix_of(Code marker)
{
    if (marker == token::binary_token) return fast::kBinary;
    if (marker == token::hexadecimal_token) return fast::kHexadecimal;
    return fast::kDecimal;
}

Value apply(Code op, const Value &left, const Value &right, ExpressionContext &context)
{
    // TWO NUMBERS: the six fast paths, and the one comparison they all come off.
    if (left.kind == Value::Kind::number && right.kind == Value::Kind::number) {
        if (precedence_of(op) <= 2 && precedence_of(op) >= 1)
            return Value::of_flag(holds(fast::compare(left.number, right.number), op));
        satellite_number answer;
        const signed long long int code = fast_path_of(op)(left.number, right.number, answer);
        if (code != success) {
            context.refuse(code, std::string("the ") + spelling_of(op) + " of " + left.number.to_text() +
                                     " and " + right.number.to_text() + " is " +
                                     (code == division_by_zero ? "a division by zero"
                                                               : "not a whole number, and there is no float yet"));
            return Value();
        }
        return Value::of_number(std::move(answer));
    }

    // TWO STRINGS. `+` joins them -- 003 DESIGN §6.6, the author at M19: "it is
    // one operator over two types and not a second meaning for the character:
    // addition and joining are the same shape". == and != compare them. An
    // ordering does too, by the same byte order a sort would use.
    if (left.kind == Value::Kind::text && right.kind == Value::Kind::text) {
        if (op == token::plus_token)
            return Value::of_text(left.text + right.text);
        if (op == token::equals_token || op == token::not_equals_token || an_ordering(op)) {
            const int order = left.text.compare(right.text);
            return Value::of_flag(holds(order < 0 ? -1 : (order > 0 ? 1 : 0), op));
        }
    }

    // TWO BOOLS: only the two that mean something on them.
    if (left.kind == Value::Kind::flag && right.kind == Value::Kind::flag &&
        (op == token::equals_token || op == token::not_equals_token))
        return Value::of_flag(holds(left.flag == right.flag ? 0 : 1, op));

    // NOTHING IS CONVERTED (DESIGN §1.1). `"n = " + 4` is refused here and not
    // quietly turned into "n = 4": a program that wants that writes the
    // conversion out loud, which is what satellite.variable.number.to_string is
    // for. The refusal names both kinds, because that is what a person fixes.
    context.refuse(types_do_not_meet, std::string(spelling_of(op)) + " was given " + left.kind_name() +
                                          " and " + right.kind_name() + ", and there is no scenario for that pair");
    return Value();
}

// THE RESERVED HALF OF EACH OPERATOR, refused in its own words. The author's
// rule is that every math operation is written space-sign-space, so a TOUCHING
// sign is not that operation -- and a language that quietly answered 5 for `2+3`
// would be one that had decided its own rule did not matter.
//
// Asked in BOTH positions, which is why it is a function and not two branches: a
// touching sign can arrive where a value was expected (`display(+3)`) or where
// an operator was (`display(2+3)`, where `2` is read first and the `+` is next).
// The second is the common one and it is the one a generic "could not read to
// the end" message served badly.
bool refuse_if_reserved(Code code, std::size_t &at, ExpressionContext &context)
{
    // 5/4 -- the fraction (the author, 2026-09-16: "when you encounter
    // number/number with NO space -- that becomes a fraction"). The type is not
    // built, so this says so in the fraction's own words rather than letting the
    // slash fall through to something that would answer.
    if (code == token::fraction_token) {
        context.refuse(not_built_yet,
                       "a touching / between two numbers is a fraction, and the fraction type is not built yet "
                       "-- write a space on both sides for whole-number division");
        ++at;
        return true;
    }
    const char *sign = code == token::tight_plus_token ? "+"
                     : code == token::tight_times_token ? "*"
                     : code == token::tight_modulus_token ? "%"
                     : code == token::tight_power_token ? "^" : nullptr;
    if (sign == nullptr)
        return false;
    context.refuse(satl_line_not_understood,
                   std::string("every math operation is written with a space on both sides of the sign, and this ") +
                       sign + " has none");
    ++at;
    return true;
}

Value evaluate_at(const std::vector<std::bitset<16>> &row, std::size_t &at, int lowest, ExpressionContext &context);

// A literal, a name, a call, a bracketed expression, or a unary operator.
Value one_operand(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    const Code code = code_at(row, at);

    // A TOUCHING MINUS IS THE UNARY ONE, and that falls out of the author's
    // whitespace rule rather than being a special case bolted on: `5 - 4` has
    // blanks on both sides and is subtraction, `-5` does not and is a sign.
    // The spaced minus is accepted here too, so `( - 5)` still reads.
    //
    // UNARY MINUS BINDS TIGHTER THAN EVERY BINARY OPERATOR (003 §6.6), so it
    // takes another operand and not an expression. The consequence worth
    // knowing: `-2 ^ 2` is (-2) ^ 2 = 4, and not -(2 ^ 2) = -4 as mathematics
    // would read it. §6.6's rule is followed as written rather than carved out
    // for the one operator it predates.
    if (code == token::tight_minus_token || code == token::minus_token) {
        ++at;
        const Value inner = one_operand(row, at, context);
        if (inner.kind != Value::Kind::number) {
            if (context.code == success)
                context.refuse(types_do_not_meet, std::string("a minus sign was put in front of ") + inner.kind_name());
            return Value();
        }
        return Value::of_number(-inner.number);
    }
    if (code == token::not_token) {
        ++at;
        const Value inner = one_operand(row, at, context);
        if (inner.kind != Value::Kind::flag) {
            if (context.code == success)
                context.refuse(types_do_not_meet, std::string("a ! was put in front of ") + inner.kind_name());
            return Value();
        }
        return Value::of_flag(!inner.flag);
    }

    if (refuse_if_reserved(code, at, context))
        return Value();

    if (code == token::left_parenthesis_token) {
        ++at;
        Value inside = evaluate_at(row, at, 1, context);
        if (code_at(row, at) == token::right_parenthesis_token)
            ++at;
        return inside;
    }

    if (code == token::string_token)
        return Value::of_text(text_at(row, at));

    // THE THREE NUMBER LITERALS, THROUGH ONE CONVERSION FAST PATH. 34587, b1100
    // and xFFAA differ only by the radix their token names.
    if (code == token::number_token || code == token::binary_token || code == token::hexadecimal_token) {
        const unsigned int radix = radix_of(code);
        const std::string digits = text_at(row, at);
        satellite_number value;
        const signed long long int held = fast::from_token_text(digits, radix, value);
        if (held != success) {
            context.refuse(held, digits + " is not a number this can read");
            return Value();
        }
        return Value::of_number(std::move(value));
    }

    if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) {
        ++at;
        return Value::of_flag(code == word::code_of(1, 17, 2));
    }

    if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token)
        return call_word(row, at, context);

    // A NAME IS A VARIABLE, and a name with no declaration is name_not_declared
    // (25) rather than a silent nothing -- which is what 003 does and what a
    // person can act on.
    if (code == token::name_token) {
        std::size_t k = at;
        const std::string name = text_at(row, k);
        at = k;
        const VariableTable::const_iterator found = context.variables.find(name);
        if (found == context.variables.end()) {
            context.refuse(name_not_declared, name + " has no satellite.variable line declaring it");
            return Value();
        }
        return found->second.value;
    }

    if (context.code == success)
        context.refuse(satl_line_not_understood, "there is no value here to work with");
    ++at;
    return Value();
}

// One level of §6.6's table. `lowest` is the loosest level this call will take,
// which is what makes the climb.
Value evaluate_at(const std::vector<std::bitset<16>> &row, std::size_t &at, int lowest, ExpressionContext &context)
{
    Value left = one_operand(row, at, context);

    while (context.code == success) {
        const Code op = code_at(row, at);
        // A touching sign where an operator belongs is the common way the
        // whitespace rule is broken, so it is answered here rather than left to
        // whatever notices the expression stopped early.
        if (refuse_if_reserved(op, at, context))
            return Value();
        const int level = precedence_of(op);
        if (level == 0 || level < lowest)
            break;
        ++at;
        // LEFT-ASSOCIATIVE at level + 1, so a - b - c is (a - b) - c. Power is
        // the exception and climbs at its OWN level, which is what makes
        // 2 ^ 3 ^ 2 read as 2 ^ (3 ^ 2). The header says why it is the one.
        const int next = (op == token::power_token) ? level : level + 1;
        const Value right = evaluate_at(row, at, next, context);
        if (context.code != success)
            return Value();
        left = apply(op, left, right, context);
    }
    return left;
}

} // namespace

signed long long int display_a_number(const Scenarios &scenarios, const satellite_number &value)
{
    // A NUMBER LARGER THAN ONE LIMB CAN ONLY REACH THE OUTPUT AS TEXT, and so
    // can a negative one: number_row.hpp's count scenario takes an
    // `unsigned long long int`. Sending it there would be the silent truncation
    // this type exists to abolish, so the text scenario takes it instead and the
    // value survives exactly.
    if (fast::fits_a_count(value) && scenarios.count != nullptr)
        return scenarios.count(fast::as_count(value), true);
    if (scenarios.text != nullptr)
        return scenarios.text(value.to_text(), true);
    return not_built_yet;
}

Value call_word(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    const Code code = code_at(row, at);
    const NumberRow *library = context.functions[code];
    const Scenarios *scenarios = library != nullptr ? &library->scenarios : nullptr;
    ++at;

    Value argument;
    if (code_at(row, at) == token::left_parenthesis_token) {
        ++at;
        if (code_at(row, at) != token::right_parenthesis_token)
            argument = evaluate_at(row, at, 1, context);
        // AN EXPRESSION MUST REACH ITS OWN `)`. Skipping whatever is left over
        // is what let `display(5 -4)` print 5 and exit 0: the evaluator stopped
        // at the touching minus, and the skip swallowed `-4` without a word.
        // Anything still standing here is a refusal, and the whitespace rule is
        // the likeliest reason for one -- so the message says so.
        if (context.code == success && code_at(row, at) != token::right_parenthesis_token)
            context.refuse(satl_line_not_understood,
                           std::string(word::spelling_of(code)) +
                               " was given something it could not read to the end of -- check that every "
                               "math sign has a space on both sides");
        while (at < row.size() && code_at(row, at) != token::right_parenthesis_token) ++at;
        if (at < row.size()) ++at;
    }
    if (context.code != success)
        return Value();

    signed long long int answer = success;
    if (scenarios == nullptr) {
        context.refuse(not_built_yet, std::string(word::spelling_of(code)) + " has no library built yet");
        return Value();
    }
    if (argument.kind == Value::Kind::text && scenarios->text != nullptr)
        answer = scenarios->text(argument.text, true);
    else if (argument.kind == Value::Kind::number)
        answer = display_a_number(*scenarios, argument.number);
    else if (argument.kind == Value::Kind::flag && scenarios->flag != nullptr)
        answer = scenarios->flag(argument.flag, true);
    else {
        context.refuse(not_built_yet, std::string(word::spelling_of(code)) + " has no scenario for " +
                                          argument.kind_name());
        return Value();
    }

    if (stops_the_program(answer))
        context.refuse(answer, std::string(word::spelling_of(code)) + " refused");
    return Value::of_code(answer);
}

Value evaluate_expression(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    return evaluate_at(row, at, 1, context);
}

} // namespace satellite004
