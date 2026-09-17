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
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_object/fast_paths.hpp"

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

// THE TOKEN -> FAST PATH TABLE USED TO BE HERE AND IS NOW IN THE OBJECT MODEL.
// It was a switch from `plus_token` to `number_fast_path::add`, which only ever
// worked because both sides of `+` were known to be numbers. satelliteValue::add
// decides that on the PAIR of kinds instead, and reaches
// number_and_number_add.hpp through a case label that reads like the filename.
// So this file kept the precedence and gave up the dispatch, which is the split
// the header describes: this file is the grammar, that one is the meaning.

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

// The radix a number literal's token already decided. The lexer strips the x
// (bytecode_registry.cpp), so the digits never carry a prefix and this is the
// only thing that knows which base they are in. binary_token is not here: a b
// literal is a satellite.variable.binary now and never becomes a bare number.
unsigned int radix_of(Code marker)
{
    if (marker == token::hexadecimal_token) return fast::kHexadecimal;
    return fast::kDecimal;
}

// THE OBJECT MODEL DECIDES WHAT TWO KINDS DO, and this function only chooses
// which of its methods to ask. Every branch that used to be here -- two numbers,
// two strings, two bools -- is now a `case pair_of(...)` in
// satellite_object/satellite_value.cpp, sitting above a call to the header named
// for that pair. Adding satellite_float changes those files and not this one.
Value apply(Code op, const Value &left, const Value &right, ExpressionContext &context)
{
    std::string why;

    // THE COMPARISONS, which all come off one ordering.
    //
    // AN ORDERING ON TWO BOOLS IS STILL REFUSED, and it is refused HERE rather
    // than in the object model, because the object model's job is to say how two
    // bools ORDER (false before true) and this file's job is to say which
    // spellings may ask. `a < b` on two bools means nothing in this language;
    // `a == b` does.
    if (precedence_of(op) >= 1 && precedence_of(op) <= 2) {
        if (an_ordering(op) && left.is_bool() && right.is_bool()) {
            context.refuse(types_do_not_meet,
                           std::string(spelling_of(op)) + " was given two bools, and only == and != order those");
            return Value();
        }
        int order = 0;
        const signed long long int code = left.compare(right, order, why);
        if (code != success) {
            context.refuse(code, why);
            return Value();
        }
        return Value::of_bool(holds(order, op));
    }

    Value answer;
    signed long long int code = satl_line_not_understood;
    switch (op) {
    case token::plus_token: code = left.add(right, answer, why); break;
    case token::minus_token: code = left.subtract(right, answer, why); break;
    case token::times_token: code = left.multiply(right, answer, why); break;
    case token::divide_token: code = left.divide(right, answer, why); break;
    case token::modulus_token: code = left.modulus(right, answer, why); break;
    case token::power_token: code = left.power(right, answer, why); break;
    default:
        why = std::string(spelling_of(op)) + " is not an operator this can work out";
        break;
    }
    if (code != success) {
        context.refuse(code, why);
        return Value();
    }
    return answer;
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

// A METHOD'S SPELLING, for a refusal a person has to act on. One name a TOKEN and
// not one a spelling, because the aliases collapsed at the lexer: `to_string`,
// `str` and `string` are one code by the time anything here sees them, so a
// refusal names the one the language thinks in.
const char *spelling_of_method(Code method)
{
    switch (method) {
    case token::find_token: return "find";
    case token::replace_token: return "replace";
    case token::add_token: return "add";
    case token::to_string_token: return "to_string";
    case token::to_number_token: return "number";
    case token::to_binary_token: return "binary";
    case token::to_hexadecimal_token: return "hex";
    default: return "that method";
    }
}

// A CONVERSION, OR nullptr FOR AN OPERATION. The whole difference between the two
// halves of a chain segment: a conversion changes what the receiver IS, an
// operation does something WITH it.
ObjectConversion conversion_of(Code method)
{
    switch (method) {
    case token::to_string_token: return object_to_string;
    case token::to_number_token: return object_to_number;
    case token::to_binary_token: return object_to_binary;
    case token::to_hexadecimal_token: return object_to_hexadecimal;
    default: return nullptr;
    }
}

// A METHOD ON A DECLARED NAME -- `s.find("str")`. The author's design, 2026-09-16,
// and the three tokens he named are the trigger: period, method name, `(`.
//
// WHAT IS IN THE PARENTHESES IS JUST AN EXPRESSION, and that is why the "look for
// quotes" step he described does not need a step of its own. The evaluator
// already tells the two apart, because the LEXER did: a quoted literal arrived as
// string_token with its characters counted behind it, and a bare name arrived as
// name_token. So `s.find("x")` evaluates a string, `s.find(other)` looks `other`
// up as a variable, and `s.find(nothing_declared)` is name_not_declared (25) --
// which is his "no object? ERROR", in the words a person can act on, with no code
// written here to produce it.
//
// THE RECEIVER'S ARM CHOOSES THE FAST PATH. `find` on a string is str_find_str;
// on anything else it is a refusal naming what it got. That is the object model's
// one hop: the variable's value knows its own kind, so the method resolves
// against that and nothing else.
Value call_method(const std::vector<std::bitset<16>> &row, std::size_t &at, const Value &start,
                  const std::string &name, ExpressionContext &context)
{
    Value receiver = start;

    // THE LOOP IS WHAT MAKES THEM STRING TOGETHER (the author, 2026-09-16: "so we
    // can string operations together"). One turn is one `.segment`, the answer
    // becomes the next turn's receiver, and `s.bin.find("1010111")` is two turns
    // with nothing in this file knowing that pairing exists. A chain of any
    // length costs one local.
    while (code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1))) {
        const Code method = code_at(row, at + 1);
        const char *spelling = spelling_of_method(method);
        at += 2;

        // A CONVERSION MAY BE WRITTEN WITH OR WITHOUT PARENTHESES, which is the
        // author's own spelling in both shapes: `n.to_string()` has them and
        // `s.bin.find(...)` does not. An OPERATION always has them, because it
        // takes an argument.
        const ObjectConversion conversion = conversion_of(method);
        bool had_parentheses = false;
        bool had_argument = false;
        Value argument;
        if (code_at(row, at) == token::left_parenthesis_token) {
            had_parentheses = true;
            ++at;
            if (code_at(row, at) != token::right_parenthesis_token) {
                argument = evaluate_at(row, at, 1, context);
                had_argument = true;
            }
            if (context.code != success)
                return Value();
            if (code_at(row, at) != token::right_parenthesis_token) {
                context.refuse(satl_line_not_understood,
                               name + "." + spelling + " was given something it could not read to the end of");
                return Value();
            }
            ++at;
        }

        Value answer;
        signed long long int code = not_built_yet;

        if (conversion != nullptr) {
            if (had_argument) {
                context.refuse(satl_line_not_understood,
                               std::string(spelling) + " is a conversion and takes no argument");
                return Value();
            }
            code = conversion(receiver, answer);
            if (code == types_do_not_meet) {
                context.refuse(code, std::string(spelling) + " was written on " + receiver.kind_name() +
                                         ", and there is no conversion from that");
                return Value();
            }
            if (code == int_error) {
                context.refuse(code, name + "." + spelling + ": that text is not a whole number this can read");
                return Value();
            }
        } else {
            if (!had_parentheses) {
                context.refuse(satl_line_not_understood,
                               name + "." + spelling + " takes an argument and needs a ( after it");
                return Value();
            }
            if (method == token::find_token)
                code = str_find_str(receiver, argument, answer);
            else if (method == token::add_token) {
                // `.add` IS `+`, and it is the object model's own add -- so it
                // joins two strings and sums two numbers without this file
                // knowing which, exactly as the operator does.
                std::string why;
                code = receiver.add(argument, answer, why);
                if (code != success && code != text_not_found) {
                    context.refuse(code, why);
                    return Value();
                }
            }
            if (code == types_do_not_meet) {
                context.refuse(code, std::string(spelling) + " was written on " + receiver.kind_name() +
                                         " and given " + argument.kind_name() + ", and there is no scenario for that");
                return Value();
            }
            // NOT FOUND IS A REFUSAL AND NOT -1 (003's S0716, machine code 15).
            if (code == text_not_found) {
                context.refuse(code, name + "." + spelling + " did not find it");
                return Value();
            }
        }

        if (code != success) {
            context.refuse(code, std::string("satellite.variable.string.") + spelling + " is not built yet");
            return Value();
        }
        receiver = std::move(answer);
    }
    return receiver;
}

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
        // A binary KEEPS ITS SIGN and stays a binary (the author, 2026-09-17: "keep
        // a sign with all of these things"): -b1010 is -b1010, worth -10, width 4.
        // It was the number -10 until then.
        if (const satellite_binary_number *bits = inner.as_binary())
            return Value::of_binary(bits->negated());
        // -50% is a percentage below zero: `200 - -50%` grows 200 by half.
        if (const satellite_percentage *percent = inner.as_percentage())
            return Value::of_percentage(satellite_percentage{-percent->scaled});
        if (!inner.is_number()) {
            if (context.code == success)
                context.refuse(types_do_not_meet, std::string("a minus sign was put in front of ") + inner.kind_name());
            return Value();
        }
        return Value::of_number(-*inner.as_number());
    }
    if (code == token::not_token) {
        ++at;
        const Value inner = one_operand(row, at, context);
        if (!inner.is_bool()) {
            if (context.code == success)
                context.refuse(types_do_not_meet, std::string("a ! was put in front of ") + inner.kind_name());
            return Value();
        }
        return Value::of_bool(!*inner.as_bool());
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

    // A STRING LITERAL BECOMES A satellite_string HERE, which is the one doorway
    // UTF-8 comes in through. Strict: a bad sequence is string_error (4) naming
    // the byte, rather than a string standing for bytes that could not be read.
    if (code == token::string_token) {
        const std::string utf8 = text_at(row, at);
        Value held;
        std::size_t bad_offset = 0;
        const signed long long int made = Value::of_utf8(utf8, held, bad_offset);
        if (made != success) {
            context.refuse(made, "that string holds a byte at " + std::to_string(bad_offset) +
                                     " that is not part of any character");
            return Value();
        }
        return held;
    }

    // A BINARY LITERAL IS A satellite.variable.binary, its width kept: b0010 is
    // four bits and displays as b0010, not as 2 (003 DESIGN 8.5, the author's
    // ruling). The lexer only makes binary_token out of b and 0s and 1s, so
    // from_digits refusing here would mean the lexer and this disagree.
    if (code == token::binary_token) {
        const std::string digits = text_at(row, at);
        satellite_binary_number bits;
        const signed long long int held = satellite_binary_number::from_digits(digits, bits);
        if (held != success) {
            context.refuse(held, "b" + digits + " is not binary this can read");
            return Value();
        }
        return Value::of_binary(std::move(bits));
    }

    // A PERCENTAGE LITERAL -- 50%, 12.5%, 1000000000000% -- is a
    // satellite.variable.percentage (the author, 2026-09-17), kept to 32 digits
    // after the point and rounded there. The lexer made the token only out of a
    // number's digits with a % pressed against them.
    if (code == token::percentage_token) {
        const std::string digits = text_at(row, at);
        satellite_percentage percent;
        const signed long long int held = satellite_percentage::from_digits(digits, percent);
        if (held != success) {
            context.refuse(held, digits + "% is not a percentage this can read");
            return Value();
        }
        return Value::of_percentage(std::move(percent));
    }

    // THE TWO NUMBER LITERALS, THROUGH ONE CONVERSION FAST PATH. 34587 and xFFAA
    // differ only by the radix their token names.
    if (code == token::number_token || code == token::hexadecimal_token) {
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
        return Value::of_bool(code == word::code_of(1, 17, 2));
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
        // THE THREE TOKENS TOGETHER (the author): a period, a method's own code,
        // and a `(`. call_method above says what happens then.
        if (code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1)))
            return call_method(row, at, found->second.value, name, context);
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
    // A VALUE LEAVES AS BYTES HERE, and only here: a library's text scenario
    // takes a std::string (number_row.hpp), so the satellite_string goes back
    // out through to_utf8 at the boundary and nowhere inside the interpreter.
    if (argument.is_string() && scenarios->text != nullptr)
        answer = scenarios->text(argument.text_utf8(), true);
    else if (argument.is_number())
        answer = display_a_number(*scenarios, *argument.as_number());
    // A binary leaves as the text it was written as, b and leading zeros and all.
    else if (argument.is_binary() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_binary()->written(), true);
    // A percentage leaves as its digits and its %: 50%, 12.5%.
    else if (argument.is_percentage() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_percentage()->written(), true);
    else if (argument.is_bool() && scenarios->flag != nullptr)
        answer = scenarios->flag(*argument.as_bool(), true);
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
