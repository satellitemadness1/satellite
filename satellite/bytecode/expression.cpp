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

#include "file_calls.hpp"
#include "../machine/stop_flag.hpp"

#include "word_codes.hpp"
#include "word_counts.hpp"
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_object/fast_paths.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/satellite_index.hpp"

#include <limits>
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
// `op_at` IS WHERE THE OPERATOR IS, and it is carried in for one reason: the
// caret. `"a" - "b"` is the refusal a person meets most, and a report that
// points at the `-` says which operation was asked for -- where a caret on the
// start of the statement leaves them to find it on a line that may hold three.
Value apply(Code op, std::size_t op_at, const Value &left, const Value &right, ExpressionContext &context)
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
                           std::string(spelling_of(op)) + " was given two bools, and only == and != order those",
                           op_at);
            return Value();
        }
        // TWO FILES ARE THE SAME FILE OR NOT, and have no order: == and != ask
        // whether two names hold ONE handle (satellite_object.cpp's identity).
        if (left.is_file() && right.is_file()) {
            if (an_ordering(op)) {
                context.refuse(types_do_not_meet,
                               std::string(spelling_of(op)) + " was given two files, and only == and != compare those",
                               op_at);
                return Value();
            }
            return Value::of_bool((left == right) == (op == token::equals_token));
        }
        int order = 0;
        const signed long long int code = left.compare(right, order, why);
        if (code != success) {
            context.refuse(code, why, op_at);
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
        context.refuse(code, why, op_at);
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
    default: return method_spelling(method);
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
        // THE ARGUMENTS ARE A LIST, divided by commas at their own depth
        // (2026-09-18): a file's `insert(n, x)` and `replace(a, b)` are the first
        // methods that take two. The methods that take one still refuse two below.
        std::vector<Value> arguments;
        if (code_at(row, at) == token::left_parenthesis_token) {
            had_parentheses = true;
            ++at;
            if (code_at(row, at) != token::right_parenthesis_token) {
                for (;;) {
                    arguments.push_back(evaluate_at(row, at, 1, context));
                    if (context.code != success || code_at(row, at) != token::comma_token)
                        break;
                    ++at;
                }
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

        // A FILE ANSWERS ITS OWN METHODS (file_calls.cpp). The receiver is a
        // handle, so the method acts on the one open file every name for it shares.
        if (satellite_file *file = receiver.as_file()) {
            Value answer = call_file_method(method, *file, arguments, had_parentheses, name, context);
            if (context.code != success)
                return Value();
            receiver = std::move(answer);
            continue;
        }

        // A NAME DECLARED WITH NO VALUE YET, said as that: `satellite.variable.file f`
        // and then `f.append("x")` is not a method that is missing.
        if (receiver.is_nothing()) {
            context.refuse(satl_line_not_understood, name + " has no value yet -- give it one with = before calling ." +
                                                         spelling + " on it");
            return Value();
        }

        // THE REST ARE THE STRING'S AND THE NUMBER'S, which take one argument or none.
        // A file's method names on anything else are not built for it yet.
        if (conversion == nullptr && method != token::find_token && method != token::add_token) {
            context.refuse(not_built_yet, name + "." + spelling + " is not built for " + receiver.kind_name() +
                                              " yet -- so far it is a file's");
            return Value();
        }
        if (arguments.size() > 1) {
            context.refuse(satl_line_not_understood, name + "." + spelling + " takes one argument, and was given " +
                                                         std::to_string(arguments.size()));
            return Value();
        }
        const bool had_argument = !arguments.empty();
        const Value argument = had_argument ? arguments.front() : Value();

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
// WHICH ITEM A NUMBER NAMES, refused in the same words a file's line is.
//
// ONE SET OF RULES FOR ONE BRACKET. A file's line and a list's item are reached
// by the same `[ ]`, so they count the same way (from 1), refuse a negative the
// same way, and say "counting from 1" in the same sentence. Two rules for one
// bracket would be the language contradicting itself in the space of one line.
bool position_of(const Value &index, unsigned long long int &out, const std::string &what,
                 std::size_t where, ExpressionContext &context)
{
    const satellite_number *number = index.as_number();
    if (number == nullptr) {
        context.refuse(types_do_not_meet, what + "[...] takes an item number, and was given " + index.kind_name(),
                       where);
        return false;
    }
    if (number->negative()) {
        context.refuse(not_a_position, what + "[" + fast::to_text(*number) + "] -- items count from 1", where);
        return false;
    }
    // TOO LARGE TO BE ANY POSITION is not an error of its own: it is a position
    // no list has, and `item_at` says so in the words that name the size.
    out = fast::fits_a_count(*number) ? fast::as_count(*number) : 0;
    if (out == 0 && !fast::fits_a_count(*number))
        out = std::numeric_limits<unsigned long long int>::max();
    return true;
}

// ONE `[n]` APPLIED TO WHATEVER THE LAST ONE ANSWERED. This is the whole of
// nesting: `a[1][2]` is this twice, and neither call knows it is in a chain.
Value index_into(const Value &current, const Value &index, const std::string &what, std::size_t where,
                 ExpressionContext &context)
{
    if (const ListHandle *handle = current.as_list()) {
        unsigned long long int position = 0;
        if (!position_of(index, position, what, where, context))
            return Value();
        const satelliteList *list = handle->get();
        const satelliteObject *item = list == nullptr ? nullptr : item_at(*list, position);
        if (item == nullptr) {
            const std::size_t held = list == nullptr ? 0 : list->items.size();
            context.refuse(line_past_the_end,
                           what + "[" + std::to_string(position) + "]: " +
                               (held == 0 ? std::string("the list is empty")
                                          : "there is no such item -- the list holds " + std::to_string(held) +
                                                (held == 1 ? " item" : " items") + ", counting from 1"),
                           where);
            return Value();
        }
        return *item;
    }
    // AN INDEX IS REACHED BY ITS KEY, not by a position -- so it does NOT go
    // through position_of, and `scores["alice"]` is not a number anywhere.
    if (const IndexHandle *handle = current.as_index()) {
        std::string key_name;
        if (!key_name_of(index, key_name)) {
            context.refuse(types_do_not_meet,
                           what + "[...] was given " + index.kind_name() +
                               " as a key, and a key must be a number, a string, a bool, a binary or a "
                               "percentage -- something that cannot change after it is filed under",
                           where);
            return Value();
        }
        const satelliteIndex *held = handle->get();
        const satelliteObject *found = held == nullptr ? nullptr : value_at(*held, key_name);
        if (found == nullptr) {
            satellite_string spelled;
            std::string ignored;
            index.to_string(spelled, ignored);
            context.refuse(line_past_the_end,
                           what + "[" + spelled.to_utf8() + "]: there is no such key in it" +
                               (held == nullptr || held->entries.empty()
                                    ? " -- the index is empty"
                                    : " -- it holds " + std::to_string(held->entries.size()) +
                                          (held->entries.size() == 1 ? " key" : " keys")),
                           where);
            return Value();
        }
        return *found;
    }

    if (satellite_file *file = current.as_file())
        return read_file_line(*file, index, what, context);

    context.refuse(types_do_not_meet, what + " is " + current.kind_name() +
                                          ", and [ ] reads a line of a file, an item of a list, or a key of an index",
                   where);
    return Value();
}

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
        const std::size_t sign_at = at;
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
                context.refuse(types_do_not_meet, std::string("a minus sign was put in front of ") + inner.kind_name(),
                               sign_at);
            return Value();
        }
        return Value::of_number(-*inner.as_number());
    }
    if (code == token::not_token) {
        const std::size_t not_at = at;
        ++at;
        const Value inner = one_operand(row, at, context);
        if (!inner.is_bool()) {
            if (context.code == success)
                context.refuse(types_do_not_meet, std::string("a ! was put in front of ") + inner.kind_name(),
                               not_at);
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

    // A BRACE WHERE A VALUE BELONGS IS A LIST (the author, 2026-09-18: "we need
    // to build satellite object definitions to be this: = {series_of_objects,
    // another_object}").
    //
    // THIS NEEDED NO NEW TOKEN AND NO LOOKAHEAD, and that is worth a sentence
    // because it looks like it should have. `{` is already the block opener, so
    // the obvious fear is that `if x {` and `x = {` now collide. They cannot:
    // one_operand is only ever reached WHERE A VALUE BELONGS, and a block's `{`
    // never stands in that position -- it follows a condition, a capsule's
    // header or an `else`, each of which is read by its own shape before an
    // expression is asked for. The POSITION decides, which is the same thing
    // that already tells a unary minus from a subtraction.
    //
    // EACH ITEM IS A WHOLE EXPRESSION, so {1 + 1, x, {2, 3}} is a list of a sum,
    // a variable and a list. Nesting costs nothing here: an item is an object,
    // and a list IS an object.
    if (code == token::left_brace_token) {
        const std::size_t brace_at = at;
        ++at;
        std::vector<satelliteObject> items;
        // `{}` IS A LIST OF NOTHING, not a refusal and not `nothing`. A list that
        // may be empty is what makes a loop that fills one legal to write.
        if (code_at(row, at) == token::right_brace_token) {
            ++at;
            return Value::of_list(make_list());
        }
        for (;;) {
            Value item = evaluate_at(row, at, 1, context);
            if (context.code != success)
                return Value();
            items.push_back(std::move(item));

            const Code next = code_at(row, at);
            if (next == token::comma_token) {
                ++at;
                // A TRAILING COMMA IS ALLOWED: `{1, 2,}` is two items. It costs
                // one test, and the alternative is refusing a line that says
                // exactly what it means for the sake of tidiness.
                if (code_at(row, at) == token::right_brace_token) {
                    ++at;
                    break;
                }
                continue;
            }
            if (next == token::right_brace_token) {
                ++at;
                break;
            }
            // THE UNCLOSED LIST, named at the brace that opened it rather than
            // at the end of the line -- the `{` is what the person has to look
            // at, and the caret should be under it.
            context.refuse(satl_line_not_understood,
                           "this list was opened with { and never closed with } -- items are "
                           "separated by commas, as in {\"one\", \"two\"}",
                           brace_at);
            return Value();
        }
        return Value::of_list(make_list(std::move(items)));
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

    // A WORD'S ANSWER CAN BE CALLED ON, as a name's can (the review, 2026-09-18):
    // `satellite.file.open("t.se").append("x")` was dropped after the word without
    // a word said, because nothing here looked for the `.` after the `)`.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
        const std::string spelled(word::spelling_of(code));
        Value answer = call_word(row, at, context);
        if (context.code != success)
            return Value();
        if (code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1)))
            return call_method(row, at, answer, spelled.substr(0, spelled.find('(')), context);
        return answer;
    }

    // A SETTING READ BY ITS BARE NAME -- `arguments.access`, no parentheses.
    //
    // THIS IS WHY THE TEST ABOVE IS NOT ENOUGH. Every other word in the language
    // is a CALL and a call is spelled with brackets, so `is_word_code` and a `(`
    // told words apart from everything else with no list of types. A setting is
    // the first word that is a VALUE, and a value has no brackets -- so it looks
    // like a word the expression reader has no arm for, and before this it was
    // one.
    //
    // THE LIBRARY DECIDES, NOT A LIST OF CODES HERE. A word is a setting exactly
    // when its library filled in `flag_setting`, which keeps this arm from
    // becoming the hand-written table of names that the code-is-the-index
    // dispatch exists to avoid. A word with no library falls through to the
    // refusals below and still says `not_built_yet` with its own name.
    if (word::is_word_code(code)) {
        const NumberRow *library = context.functions[code];
        if (library != nullptr && library->scenarios.flag_setting != nullptr) {
            const SettingReply said = library->scenarios.flag_setting(false, false);
            ++at;
            if (said.code != success) {
                context.refuse(said.code, std::string(word::spelling_of(code)) +
                                              " could not be read" +
                                              (said.reason.empty() ? "" : " -- " + said.reason));
                return Value();
            }
            return Value::of_bool(said.flag);
        }

        // A FACT ABOUT THE MACHINE, read the same way and for the same reason:
        // it is a VALUE with no brackets, so without this arm it looks like a
        // word the expression reader has no scenario for. SATELLITE_ARGUMENTS
        // B7-B11.
        //
        // THE LIBRARY DECIDES AGAIN. A word is a fact exactly when its library
        // filled in `fact`, so no list of codes is kept here and a fact added
        // later needs no line in this file.
        if (library != nullptr && library->scenarios.fact != nullptr) {
            const FactReply said = library->scenarios.fact();
            ++at;
            if (said.code != success) {
                context.refuse(said.code, std::string(word::spelling_of(code)) +
                                              " could not be read" +
                                              (said.reason.empty() ? "" : " -- " + said.reason),
                               at - 1);
                return Value();
            }
            if (said.is_text == true) {
                Value answer;
                std::size_t bad = 0;
                const signed long long int made = Value::of_utf8(said.text, answer, bad);
                if (made != success) {
                    context.refuse(made, std::string(word::spelling_of(code)) +
                                             " answered bytes that are not text",
                                   at - 1);
                    return Value();
                }
                return answer;
            }
            return Value::of_number(satellite_number(said.count));
        }
    }

    // A NAME IS A VARIABLE, and a name with no declaration is name_not_declared
    // (25) rather than a silent nothing -- which is what 003 does and what a
    // person can act on.
    if (code == token::name_token) {
        std::size_t k = at;
        const std::string name = text_at(row, k);
        const std::size_t name_at = at;   // text_at walked past it; the caret wants the name
        at = k;
        const VariableTable::const_iterator found = context.variables.find(name);
        if (found == context.variables.end()) {
            context.refuse(name_not_declared, name + " has no satellite.variable line declaring it", name_at);
            return Value();
        }
        // `f[n]` -- LINE n OF A FILE, and `a[n]` -- ITEM n OF A LIST, both
        // counting from 1 (the author, 2026-09-18: "we'll build it so you can
        // iterate over the lines as if they were objects").
        //
        // A LOOP AND NOT ONE BRACKET, so `a[1][2]` reads the item of an item.
        // The author asked for it in the same breath as the index itself: "we
        // must build it to be able to access lists inside of lists". It costs a
        // while instead of an if, because each step just indexes whatever the
        // last one answered -- a list of lists is not a second kind of thing.
        if (code_at(row, at) == token::left_square_bracket_token) {
            Value current = found->second.value;
            std::string what = name;
            while (code_at(row, at) == token::left_square_bracket_token) {
                const std::size_t opened_at = at;
                ++at;
                const Value index = evaluate_at(row, at, 1, context);
                if (context.code != success)
                    return Value();
                if (code_at(row, at) != token::right_square_bracket_token) {
                    context.refuse(satl_line_not_understood,
                                   what + "[...] was given something it could not read to the end of", opened_at);
                    return Value();
                }
                ++at;
                if (current.is_nothing()) {
                    context.refuse(satl_line_not_understood, name + " has no value yet -- give it one with = before "
                                                                 "reading an item of it", opened_at);
                    return Value();
                }
                current = index_into(current, index, what, opened_at, context);
                if (context.code != success)
                    return Value();
                what += "[...]";
            }
            if (code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1)))
                return call_method(row, at, current, name, context);
            return current;
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
        const std::size_t op_at = at;
        ++at;
        // LEFT-ASSOCIATIVE at level + 1, so a - b - c is (a - b) - c. Power is
        // the exception and climbs at its OWN level, which is what makes
        // 2 ^ 3 ^ 2 read as 2 ^ (3 ^ 2). The header says why it is the one.
        const int next = (op == token::power_token) ? level : level + 1;
        const Value right = evaluate_at(row, at, next, context);
        if (context.code != success)
            return Value();
        left = apply(op, op_at, left, right, context);
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

// A DIRECTORY WORD ANSWERS A VALUE, where every other word so far answers a
// machine code (number_row.hpp): `change(d)` is true or false and is never an
// error, and `list()` is names. The names have nowhere to go inside a program
// until satellite has a list type (MILESTONES M14); at the PROMPT they are drawn
// as the table, which the session does with the library's own answer rather than
// through 003's one-shot global request (PLAN M0.6).
Value call_directory_word(token::Code code, const Scenarios &scenarios, const Value &argument,
                          ExpressionContext &context)
{
    const std::string spelling(word::spelling_of(code));
    std::string path;
    const bool given = !argument.is_nothing();
    if (given && !argument.is_string()) {
        context.refuse(types_do_not_meet,
                       spelling + " takes a directory written as a string, and was given " + argument.kind_name());
        return Value();
    }
    if (given)
        path = argument.text_utf8();

    const DirectoryReply reply = scenarios.directory(path, given, stop_flag());
    if (stops_the_program(reply.code)) {
        context.refuse(reply.code, spelling + (reply.reason.empty() ? std::string() : ": " + reply.reason));
        return Value();
    }
    if (code == word::code_of(1, 18, 1))
        return Value::of_bool(reply.flag);

    context.refuse(not_built_yet, spelling + " read " + std::to_string(reply.names.size()) +
                                      " names, and satellite has no list type built yet to hold them "
                                      "(MILESTONES M14) -- at the prompt the same line draws the table");
    return Value();
}

Value call_word(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    const Code code = code_at(row, at);

    // THE `word_counts` BIT, AND THIS IS THE ONE PLACE IT IS READ. Every word in
    // the language arrives here, so counting here counts everything once and
    // needs no second site to keep in step.
    //
    // THE TEST IS NOT HOISTED YET AND THAT IS THE AUTHOR'S ORDER, not an
    // oversight: "we can leave optimizing it to another milestone later, so it
    // doesn't have to be optimized yet". MILESTONES M35 and SATELLITE_ERROR F5b
    // are where `RunPlan::plain` gets a call_word with this line compiled out.
    if (context.state.features.on(Feature::word_counts))
        word_counts().saw(code);

    const NumberRow *library = context.functions[code];
    const Scenarios *scenarios = library != nullptr ? &library->scenarios : nullptr;
    ++at;

    // THE ARGUMENTS ARE A LIST (2026-09-18), divided by commas at their own depth:
    // `satellite.file.new(path, "text")` is the first word a program can call with
    // two. A library still takes one, and says so below when it is given more.
    std::vector<Value> arguments;
    if (code_at(row, at) == token::left_parenthesis_token) {
        ++at;
        if (code_at(row, at) != token::right_parenthesis_token) {
            for (;;) {
                arguments.push_back(evaluate_at(row, at, 1, context));
                if (context.code != success || code_at(row, at) != token::comma_token)
                    break;
                ++at;
            }
        }
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

    // satellite.file's words answer a HANDLE, which no library can (file_calls.hpp).
    if (is_file_word(code))
        return call_file_word(code, arguments, row, context);

    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood, std::string(word::spelling_of(code)) +
                                                     " takes one argument, and was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    const Value argument = arguments.empty() ? Value() : arguments.front();

    signed long long int answer = success;
    if (scenarios == nullptr) {
        context.refuse(not_built_yet, std::string(word::spelling_of(code)) + " has no library built yet");
        return Value();
    }
    if (scenarios->directory != nullptr)
        return call_directory_word(code, *scenarios, argument, context);

    // A FACT, WRITTEN WITH ITS BRACKETS. `arguments.memory()` is the author's own
    // spelling in the second brief -- "arguments.memory() (alias of
    // arguments.memory.total())" -- and it reaches HERE rather than the bare-word
    // arm above, because brackets make it a call. Same answer either way.
    //
    // IT TAKES NOTHING, and says so when given something: a fact is what the
    // machine has, so there is nothing to hand it.
    if (scenarios->fact != nullptr) {
        if (!arguments.empty()) {
            context.refuse(satl_line_not_understood,
                           std::string(word::spelling_of(code)) +
                               " is a fact about this machine and takes nothing");
            return Value();
        }
        const FactReply said = scenarios->fact();
        if (said.code != success) {
            context.refuse(said.code, std::string(word::spelling_of(code)) + " could not be read" +
                                          (said.reason.empty() ? "" : " -- " + said.reason));
            return Value();
        }
        if (said.is_text == true) {
            Value made;
            std::size_t bad = 0;
            const signed long long int built = Value::of_utf8(said.text, made, bad);
            if (built != success) {
                context.refuse(built, std::string(word::spelling_of(code)) + " answered bytes that are not text");
                return Value();
            }
            return made;
        }
        return Value::of_number(satellite_number(said.count));
    }
    // A VALUE LEAVES AS BYTES HERE, and only here: a library's text scenario
    // takes a std::string (number_row.hpp), so the satellite_string goes back
    // out through to_utf8 at the boundary and nowhere inside the interpreter.
    // A LIST GOES TO THE WORD THAT KNOWS WHAT A LIST MEANS, and falls back to
    // its own spelling when the word has no such meaning -- so
    // `satellite.feedback({"a","b"})` is two messages, while
    // `satellite.console.display({1, 2})` prints {1, 2} without every library
    // having to grow a list scenario it does not want.
    if (argument.is_list() && scenarios->list != nullptr) {
        std::vector<std::string> items;
        const satelliteList *held = argument.as_list()->get();
        if (held != nullptr) {
            items.reserve(held->items.size());
            for (const satelliteObject &item : held->items) {
                satellite_string one;
                std::string why;
                // AN ITEM THAT HAS NO TEXT STOPS THE CALL, naming the item. A
                // list with a file in it reaching a word that expects words
                // would otherwise arrive as an empty string among real ones,
                // which is a lie the person cannot see.
                const signed long long int made = item.to_string(one, why);
                if (made != success) {
                    context.refuse(made, std::string(word::spelling_of(code)) + " was given a list holding " +
                                             item.kind_name() + ", and " + why);
                    return Value();
                }
                items.push_back(one.to_utf8());
            }
        }
        answer = scenarios->list(items, true);
    }
    else if (argument.is_string() && scenarios->text != nullptr)
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
    // A CONTAINER GIVEN TO A WORD THAT ONLY TAKES TEXT reads back as what was
    // typed: {1, "two"}, or {"zoe": 1, "al": 2} for an index. satellite_object.cpp's
    // to_string is the one spelling, so display and a refusal quote it the same way.
    else if ((argument.is_list() || argument.is_index()) && scenarios->text != nullptr) {
        satellite_string written;
        std::string why;
        const signed long long int made = argument.to_string(written, why);
        if (made != success) {
            context.refuse(made, std::string(word::spelling_of(code)) + " was given " +
                                     argument.kind_name() + ", and " + why);
            return Value();
        }
        answer = scenarios->text(written.to_utf8(), true);
    }
    else {
        context.refuse(not_built_yet, std::string(word::spelling_of(code)) + " has no scenario for " +
                                          argument.kind_name());
        return Value();
    }

    if (stops_the_program(answer))
        context.refuse(answer, std::string(word::spelling_of(code)) + " refused");
    return Value::of_code(answer);
}

// `a[i] = v`, AND `a[i][j] = v`, WITH COPY-ON-WRITE ALONG THE WHOLE PATH.
//
// IT LIVES BESIDE index_into ON PURPOSE. Reading `a[i]` and writing `a[i]` must
// agree about what `i` means -- counting from 1, what a bad index says, which
// kinds have a `[ ]` at all -- and the way to keep two functions agreeing is to
// let them call the same one (position_of) from the same file. The walker parses
// the statement and hands the pieces here.
//
// `root` IS A REFERENCE INTO THE VARIABLE TABLE, never a copy, and the whole
// copy-on-write scheme depends on that. satellite_list.hpp says why at length.
signed long long int write_through_index(Value &root, const std::vector<Value> &indices, Value value,
                                         const std::string &name, std::size_t where, const TypeShape &shape,
                                         ExpressionContext &context)
{
    Value *target = &root;
    std::string what = name;

    // THE DECLARED SHAPE IS WALKED DOWN BESIDE THE VALUE, and this is not
    // decoration: without it `<key, value>` is enforced when a WHOLE container is
    // assigned and silently ignored when one item is written, so
    // `satellite.container.index<...string, ...number> s` took `s[1] = 5` -- a
    // number key in an index declared to take strings. A type that holds until
    // you use it is worse than no type at all, because a person believes it.
    //
    // A SHAPE WITH NO WORD MEANS "ANYTHING", which is what a container declared
    // without <> is and what every level below one becomes.
    static const TypeShape kAnything;
    const TypeShape *here = &shape;

    for (std::size_t step = 0; step < indices.size(); ++step) {
        if (target->is_nothing()) {
            context.refuse(satl_line_not_understood,
                           name + " has no value yet -- give it one with = before changing an item of it", where);
            return context.code;
        }
        // A FILE'S LINES ARE NOT WRITTEN THIS WAY. `f[2] = "x"` looks like it
        // should work and must not quietly do nothing: a file has its own words
        // for changing a line, and `[ ]` on a file reads.
        if (target->is_file()) {
            context.refuse(types_do_not_meet,
                           what + "[...] = ... -- a file's line is not written with [ ], and " + name +
                               " is a file (SATELLITE_FILE_OPERATIONS Part 3 lists what a file does)",
                           where);
            return context.code;
        }
        // AN INDEX GROWS ON A WRITE. A new key is PUT IN rather than refused,
        // which is the opposite of a list and is not an inconsistency:
        // satellite_index.hpp has the reason -- a position is not something a
        // program invents, and a key is the only way a dict is ever filled.
        if (IndexHandle *keys = target->as_index()) {
            // THE KEY MUST FIT WHAT WAS DECLARED, checked before anything is put
            // in: an index that has already taken a key of the wrong type cannot
            // be un-taken, and the entry would sit there for the rest of the run.
            std::string unfit;
            if (here->word != 0 && !here->parameters.empty() &&
                !value_fits(here->parameters[0], indices[step], unfit)) {
                context.refuse(types_do_not_meet,
                               "the key does not fit: " + unfit, where);
                return context.code;
            }
            const TypeShape *inside = (here->word != 0 && here->parameters.size() > 1)
                                          ? &here->parameters[1] : &kAnything;

            std::string key_name;
            if (!key_name_of(indices[step], key_name)) {
                context.refuse(types_do_not_meet,
                               "the key is " + std::string(indices[step].kind_name()) +
                                   ", and a key must be a number, a string, a bool, a binary or a "
                                   "percentage -- something that cannot change after it is filed under",
                               where);
                return context.code;
            }
            satelliteIndex &body = about_to_change(*keys);
            satelliteObject &slot = value_for_writing(body, key_name, indices[step]);
            if (step + 1 == indices.size()) {
                if (inside->word != 0 && !value_fits(*inside, value, unfit)) {
                    context.refuse(types_do_not_meet,
                                   "the value does not fit: " + unfit, where);
                    return context.code;
                }
                slot = std::move(value);
                return success;
            }
            here = inside;
            target = &slot;
            satellite_string spelled;
            std::string ignored;
            indices[step].to_string(spelled, ignored);
            what += "[" + spelled.to_utf8() + "]";
            continue;
        }

        ListHandle *handle = target->as_list();
        if (handle == nullptr) {
            context.refuse(types_do_not_meet,
                           what + " is " + target->kind_name() +
                               ", and [ ] = ... changes an item of a list or a key of an index", where);
            return context.code;
        }

        unsigned long long int position = 0;
        if (!position_of(indices[step], position, what, where, context))
            return context.code;

        // MADE WRITABLE BEFORE THE ITEM IS FOUND, not after: the clone moves the
        // items, so a pointer taken first would point into the old vector.
        satelliteList &body = about_to_change(*handle);
        satelliteObject *item = item_at(body, position);
        if (item == nullptr) {
            const std::size_t held = body.items.size();
            // THE LIST DOES NOT GROW HERE, and this says so rather than leaving a
            // person to guess. Growing on a write to one past the end is a real
            // design -- it is just not one anybody has chosen, and choosing it in
            // an error path is how a language gets a rule nobody meant.
            context.refuse(line_past_the_end,
                           what + "[" + std::to_string(position) + "] = ...: " +
                               (held == 0 ? std::string("the list is empty")
                                          : "there is no such item -- the list holds " + std::to_string(held) +
                                                (held == 1 ? " item" : " items") + ", counting from 1") +
                               ". Writing past the end does not make the list longer",
                           where);
            return context.code;
        }

        // THE ITEM'S OWN SHAPE, for a list declared `<of what>`.
        const TypeShape *inside_list = (here->word != 0 && !here->parameters.empty())
                                           ? &here->parameters[0] : &kAnything;
        if (step + 1 == indices.size()) {
            std::string unfit;
            if (inside_list->word != 0 && !value_fits(*inside_list, value, unfit)) {
                context.refuse(types_do_not_meet,
                               what + "[" + std::to_string(position) + "] does not fit: " + unfit, where);
                return context.code;
            }
            *item = std::move(value);
            return success;
        }
        here = inside_list;
        target = item;
        what += "[" + std::to_string(position) + "]";
    }
    return success;
}

Value evaluate_expression(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context)
{
    return evaluate_at(row, at, 1, context);
}

} // namespace satellite004
