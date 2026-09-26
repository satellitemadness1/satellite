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
#include "../satellite_object/string_case.hpp"

#include "capsule_calls.hpp"
#include "capsule_scopes.hpp"

#include "file_calls.hpp"
#include "info_calls.hpp"
#include "access_calls.hpp"
#include "color_values.hpp"
#include "container_calls.hpp"
#include "string_calls.hpp"
#include "console_calls.hpp"
#include "thread_calls.hpp"
#include "../satellite_object/object_lock.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/thread_stop.hpp"
#include "main_arguments.hpp"
#include "float_values.hpp"
#include "fraction_values.hpp"
#include "hexadecimal_values.hpp"
#include "infinity_calls.hpp"
#include "library_values.hpp"
#include "window_calls.hpp"
#include "../machine/stop_flag.hpp"

#include "word_codes.hpp"
#include "word_counts.hpp"
#include "../satellite_variable_number/number_conversions.hpp"
#include "../satellite_object/fast_paths.hpp"
#include "../satellite_object/satellite_list.hpp"
#include "../satellite_object/satellite_index.hpp"

#include <optional>
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
        // TWO CONTAINERS ARE EQUAL OR NOT, AND HAVE NO ORDER. `{1,2} < {3}` means
        // nothing -- is a list ordered by length, by its first item, by its
        // text? Every answer is a guess -- while `{1,2} == {1,2}` has exactly one
        // right answer, which satellite_object.cpp gives.
        if ((left.is_list() && right.is_list()) || (left.is_index() && right.is_index())) {
            if (an_ordering(op)) {
                context.refuse(types_do_not_meet,
                               std::string(spelling_of(op)) + " was given two " +
                                   (left.is_list() ? "lists" : "indexes") +
                                   ", and only == and != compare those -- there is no order between two containers",
                               op_at);
                return Value();
            }
            return Value::of_bool((left == right) == (op == token::equals_token));
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
        // satellite.variable.color (2026-09-22): TWO COLOURS ARE THE SAME OR NOT, AND HAVE
        // TWO THREADS ARE EQUAL WHEN THEY ARE ONE THREAD (003's M23 §6, "two names, one thread"),
        // and there is no order between two -- so == and != ask satelliteObject's identity, and
        // an ordering is refused by name, as for two colors below.
        if (left.is_thread() && right.is_thread()) {
            if (an_ordering(op)) {
                context.refuse(types_do_not_meet,
                               std::string(spelling_of(op)) + " was given two threads, and only == and != compare "
                                                              "those -- one thread or two",
                               op_at);
                return Value();
            }
            return Value::of_bool((left == right) == (op == token::equals_token));
        }
        // NO ORDER -- is red before blue? -- so == and != ask color_same (the digits AND
        // the transparency), and an ordering is refused by name, as for two files.
        if (left.is_color() && right.is_color()) {
            if (an_ordering(op)) {
                context.refuse(types_do_not_meet,
                               std::string(spelling_of(op)) + " was given two colors, and only == and != compare "
                                                              "those -- there is no order between two colors",
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
bool refuse_if_reserved(const std::vector<std::bitset<16>> &row, Code code, std::size_t &at,
                        ExpressionContext &context)
{
    // 5/4 -- the fraction (the author, 2026-09-16: "when you encounter
    // number/number with NO space -- that becomes a fraction"). A NUMBER WRITTEN
    // OUT before the slash never reaches here: the number literal's arm reads 1/3
    // as the fraction (satellite.variable.fraction, 2026-09-22). What does is a
    // touching slash after a name, a hex or a binary that ends in a digit --
    // `n1/2`, `x1F2/3` -- and a fraction of one of those is the author's later
    // step ("we auto convert for the "another" type"), so it is named as not built.
    if (code == token::fraction_token) {
        context.refuse(not_built_yet,
                       "a touching / makes a fraction only between two numbers written out, like 1/3 -- a fraction "
                       "of a name, a hex or a binary is not built yet; write a space on both sides for division");
        ++at;
        return true;
    }
    const char *sign = code == token::tight_plus_token ? "+"
                     : code == token::tight_times_token ? "*"
                     : code == token::tight_modulus_token ? "%"
                     : code == token::tight_power_token ? "^" : nullptr;
    if (sign == nullptr)
        return false;
    // A TOUCHING `**` IS NAMED (INF-1, SATELLITE_INFINITY.md). A spaced `**` is power,
    // so a person writing `2**3` meant power -- and the generic answer below would
    // send them to write `2 * * 3`, which is two operators and is refused too. The
    // for step names the same mistake (program_walk.cpp, for_step_moves_by).
    if (code == token::tight_times_token && code_at(row, at + 1) == token::tight_times_token) {
        context.refuse(satl_line_not_understood,
                       "power is written with a space on both sides -- 2 ** 3 or 2 ^ 3, never 2**3", at);
        ++at;
        return true;
    }
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
    return method_spelling(method);                         // the registry's one table (INF-1)
}

// WHETHER THE STATEMENT ENDS HERE, so nothing after this point uses what was answered.
// capsule_calls.cpp's ends_the_line asks the same of a capsule's answer.
bool the_line_ends_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    const Code code = code_at(row, at);
    return code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token;
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
                  const std::string &root, ExpressionContext &context, Value *home = nullptr,
                  const TypeShape *shape = nullptr)
{
    // NOTHING IS COPIED WHILE THE CHAIN IS STILL ON THE VARIABLE, and that one
    // sentence is the difference between an append that is instant and one that
    // is quadratic.
    //
    // `Value (*live) = start;` USED TO BE THE FIRST LINE HERE, and it made
    // 20,000 appends take THIRTEEN TIMES what 5,000 did -- copy-on-write asks
    // `use_count() == 1`, and that copy made the answer no on every single call,
    // so each append duplicated the whole list. It is precisely the bug 003
    // shipped for months (satellite_list.hpp tells that story), rebuilt here by
    // accident and caught only by measuring it.
    //
    // So `held` stays EMPTY until a method answers something new, and `live`
    // points at the variable's own object until then. `on_the_name` is what
    // `.append` needs: a list is a value, so appending anywhere but the
    // variable itself changes a copy nobody will ever read.
    Value held;
    Value *live = home;
    bool on_the_name = home != nullptr;
    if (live == nullptr) {
        held = start;
        live = &held;
    }

    // THE LOOP IS WHAT MAKES THEM STRING TOGETHER (the author, 2026-09-16: "so we
    // can string operations together"). One turn is one `.segment`, the answer
    // becomes the next turn's (*live), and `s.bin.find("1010111")` is two turns
    // with nothing in this file knowing that pairing exists. A chain of any
    // length costs one local.
    //
    // A REFUSAL NAMES THE PIECE IT REFUSED (the M16 review, 2026-09-26). `name` was the
    // chain's root on every turn, so `s.trim.append("d")` said "s.append changes a string,
    // and this one has no name to change" -- of s, which has one. Once a turn has taken the
    // chain off the name, the next turn is called what was written up to it,
    // `s.trim.append`, as `s[...][2]` already was. A chain one turn long, the common case,
    // spells nothing.
    std::string chain;
    const std::string *shown = &root;
    std::size_t link_at = 0;
    bool after_a_link = false;
    while (code_at(row, at) == token::method_token &&
           (token::is_method_code(code_at(row, at + 1)) || a_member_next(row, at, *live))) {
        if (after_a_link && !on_the_name) {
            chain = *shown + link_spelled(row, link_at);
            shown = &chain;
        }
        link_at = at;
        after_a_link = true;
        const std::string &name = *shown;
        // AN OBJECT'S MEMBER (2026-09-22, capsule_calls.hpp): its spacesuit says what the
        // name is, whether or not the lexer made it a method code -- an object's capsule
        // may be called `size` as well as `call_name`. What it answers goes on down the chain.
        if ((*live).is_user_defined()) {
            Value answer = call_member(row, at, *live, name, context);
            if (context.code != success)
                return Value();
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }
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
                    // A CAPSULE'S NAME IS READ AS WRITTEN, NEVER WORKED OUT
                    // (SATELLITE_WINDOW.md WIN-11). `my_button.pressed(when_pressed)`
                    // names a capsule to run, and a capsule is not a value any
                    // expression can answer yet -- worked out, `when_pressed` is
                    // "a name with no satellite.variable line declaring it",
                    // which is a refusal of the program that is right.
                    //
                    // ASKED OF window_calls.hpp AND NOT LISTED HERE, so there is
                    // one place that says which methods are spelled this way.
                    // THE CHECKER HAS ALREADY PROVED the name is a real capsule
                    // and that this argument IS a name -- that is the whole
                    // reason a name may stand here at all.
                    //
                    // AND IT IS KEPT AS THE CAPSULE IT REACHED, NOT AS THE WORDS
                    // (2026-09-22). Two files may each have a `when_pressed`, and a
                    // press happens long after this line, with nothing left to say
                    // which file wrote it -- so the name is resolved HERE, from where
                    // it stands, and the window holds that capsule's key. A dotted
                    // name, `.pressed(other.go)`, is one name.
                    if (arguments.empty() && window_method_takes_a_capsule_name(method) &&
                        code_at(row, at) == token::name_token) {
                        std::size_t k = at;
                        std::vector<std::string> names;
                        dotted_names_at(row, k, names);
                        const std::string capsule =
                            capsule_key_at(context.state.capsules, context.state.program, row, at, names);
                        Value named;
                        std::size_t bad_offset = 0;
                        Value::of_utf8(capsule, named, bad_offset);
                        arguments.push_back(std::move(named));
                        at = k;
                    } else {
                        arguments.push_back(evaluate_at(row, at, 1, context));
                    }
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

        // `.reverse()` ON A STRING, A NUMBER OR A BINARY -- one command on every
        // type that has an order (the author, 2026-09-18). Asked before the
        // container branch because none of these is a container, and before the
        // string/number branch because that one answers "not built for" first.
        if (method == token::reverse_token && !(*live).is_list() && !(*live).is_index() &&
            !(*live).is_file()) {
            bool handled = false;
            std::string why;
            Value answer = reverse_of((*live), handled, why);
            if (!handled) {
                context.refuse(types_do_not_meet,
                               name + ".reverse() was written on " + (*live).kind_name() +
                                   (why.empty() ? std::string(", and that has no order to reverse")
                                                : ", and " + why));
                return Value();
            }
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }

        // A LIST'S AND AN INDEX'S OWN METHODS (container_calls.cpp).
        if ((*live).is_list() || (*live).is_index()) {
            const bool changes_it = method == token::append_token;
            Value answer = call_container_method(method, (*live), on_the_name ? live : nullptr,
                                                        on_the_name ? shape : nullptr, arguments,
                                                        had_parentheses, name, context);
            if (context.code != success)
                return Value();
            // A METHOD THAT ANSWERED SOMETHING NEW takes the chain off the
            // variable; one that changed it in place leaves the chain where it
            // is, so `names.append("a").append("b")` really appends twice.
            if (!changes_it) {
                held = std::move(answer);
                live = &held;
                on_the_name = false;
            }
            continue;
        }

        // A STRING IN CAPITALS OR SMALL LETTERS, .upper() and .lower() -- and .uppercase(),
        // .up() and .lowercase(), which the lexer makes the same two codes (the author,
        // 2026-09-25). A new string is answered (string_case.hpp); the name keeps its own.
        if ((*live).is_string() && (method == token::upper_token || method == token::lower_token)) {
            if (!arguments.empty()) {
                context.refuse(satl_line_not_understood, name + "." + spelling + "() takes nothing in its brackets");
                return Value();
            }
            satellite_string changed;
            const signed long long int made = string_case(*(*live).as_string(), method == token::upper_token, changed);
            if (made != success) {
                context.refuse(made, name + "." + spelling + "() could not change that text");
                return Value();
            }
            held = Value::of_string(std::move(changed));
            live = &held;
            on_the_name = false;
            continue;
        }

        // A STRING'S COLOUR, .foreground(c) and .background(c) (console_style.hpp):
        // the string answered in that colour, or unchanged where no colour is drawn.
        if ((*live).is_string() && (method == token::foreground_token || method == token::background_token)) {
            Value answer = string_coloured(*live, method, arguments, had_parentheses, name, context);
            if (context.code != success)
                return Value();
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }

        // A STRING'S OWN METHODS (M16, string_calls.cpp): .size .empty .contains .starts_with
        // .ends_with .find .at .substring .split .replace .trim .resolved answer something
        // new, and .append and .clear change the string on the name -- so, as a list's
        // .append does, those two leave the chain where it is.
        if ((*live).is_string() && string_method_arity(method) >= 0) {
            Value answer = call_string_method(method, *live, on_the_name ? live : nullptr, arguments,
                                              had_parentheses, name, context);
            if (context.code != success)
                return Value();
            if (!changes_a_string(method)) {
                held = std::move(answer);
                live = &held;
                on_the_name = false;
            }
            continue;
        }

        // A FILE ANSWERS ITS OWN METHODS (file_calls.cpp). The (*live) is a
        // handle, so the method acts on the one open file every name for it shares.
        if (satellite_file *file = (*live).as_file()) {
            // THE AUTHOR'S LOCK ON A FILE: .lock() turns it on and .unlock() off, and while it
            // is on every method called on the file holds it -- a file is one thing to every
            // name for it, and to every thread (satellite_object/object_lock.hpp).
            if (method == token::lock_token || method == token::unlock_token) {
                file->lock.on.store(method == token::lock_token, std::memory_order_release);
                held = Value();
                live = &held;
                on_the_name = false;
                continue;
            }
            const ObjectHold one_call(&file->lock, LockUse::writing);
            if (one_call.code() != success) {
                context.refuse(one_call.code(), name + " is a locked file held by a thread that is waiting for this one");
                return Value();
            }
            Value answer = call_file_method(method, *file, arguments, had_parentheses, name, context);
            if (context.code != success)
                return Value();
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }

        // THE FOUR TYPES OF 2026-09-22 ANSWER THEIR OWN METHODS, each in
        // bytecode/<name>_values.cpp. `answered` false leaves the method to the
        // conversions every type shares, below. `slot` is the variable itself, for
        // a method that changes the value -- the same rule as a list's `.append`.
        if ((*live).is_float() || (*live).is_hexadecimal() || (*live).is_color() || (*live).is_fraction()) {
            bool answered = false;
            Value *slot = on_the_name ? live : nullptr;
            Value answer = (*live).is_float()       ? float_method(method, *live, slot, arguments, had_parentheses, name, context, answered)
                         : (*live).is_hexadecimal() ? hexadecimal_method(method, *live, slot, arguments, had_parentheses, name, context, answered)
                         : (*live).is_color()       ? color_method(method, *live, slot, arguments, had_parentheses, name, context, answered)
                                                    : fraction_method(method, *live, slot, arguments, had_parentheses, name, context, answered);
            if (context.code != success)
                return Value();
            if (answered) {
                held = std::move(answer);
                live = &held;
                on_the_name = false;
                continue;
            }
        }

        // A THREAD ANSWERS ITS OWN (thread_calls.cpp): start and stop answer the thread,
        // so `t.start()` can go on to another method; join and wait answer what its
        // capsule handed back. Before the containers ever see it: a list has a .join too.
        if (const ThreadHandle *thread = (*live).thread_handle()) {
            Value answer = call_thread_method(method, *thread, arguments, had_parentheses, name, context);
            if (context.code != success)
                return Value();
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }

        // A WINDOW ANSWERS ITS OWN (window_calls.cpp), the same way and for the
        // same reason: the handle is what a method acts through, so two names for
        // one window move one window. What comes back IS the window, so
        // `w.title("x").focus()` strings together with no case of its own here.
        if (const WindowHandle *window = (*live).window_handle()) {
            Value answer = call_window_method(method, *window, arguments, had_parentheses, name, context);
            if (context.code != success)
                return Value();
            held = std::move(answer);
            live = &held;
            on_the_name = false;
            continue;
        }

        // A NAME DECLARED WITH NO VALUE YET, said as that: `satellite.variable.file f`
        // and then `f.append("x")` is not a method that is missing.
        if ((*live).is_nothing()) {
            context.refuse(satl_line_not_understood, name + " has no value yet -- give it one with = before calling ." +
                                                         spelling + " on it");
            return Value();
        }

        // AN INFINITY'S OWN METHODS ARE NUMBERED AND NOT BUILT YET (INF-1 numbered
        // them), and each says which milestone builds it. An infinity has no number,
        // no bits and no hex digits to be converted to -- it is larger than every
        // number -- so those three are refused by name; `.string` is its display, and
        // `.reverse()` was answered above.
        if ((*live).is_infinity()) {
            const std::string missing = infinity_method_not_built(method);
            if (!missing.empty()) {
                context.refuse(not_built_yet, name + "." + spelling + " " + missing);
                return Value();
            }
            if (conversion != nullptr && conversion != object_to_string) {
                context.refuse(types_do_not_meet, name + "." + spelling +
                                                      " was written on an infinity, and an infinity is larger than "
                                                      "every number -- there is no " + spelling + " to make of it");
                return Value();
            }
        }

        // THE REST ARE THE STRING'S AND THE NUMBER'S, which take one argument or none.
        // A file's method names on anything else are not built for it yet.
        if (conversion == nullptr && method != token::find_token && method != token::add_token) {
            context.refuse(not_built_yet, name + "." + spelling + " is not built for " + (*live).kind_name() +
                                              " yet -- " + so_far_whose(method));
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
            code = conversion((*live), answer);
            if (code == types_do_not_meet) {
                context.refuse(code, std::string(spelling) + " was written on " + (*live).kind_name() +
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
                code = str_find_str((*live), argument, answer);
            else if (method == token::add_token) {
                // `.add` IS `+`, and it is the object model's own add -- so it
                // joins two strings and sums two numbers without this file
                // knowing which, exactly as the operator does.
                std::string why;
                code = (*live).add(argument, answer, why);
                if (code != success && code != text_not_found) {
                    context.refuse(code, why);
                    return Value();
                }
            }
            if (code == types_do_not_meet) {
                context.refuse(code, std::string(spelling) + " was written on " + (*live).kind_name() +
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
        held = std::move(answer);
        live = &held;
        on_the_name = false;
    }
    // `s.split(",")[2]` -- [ ] STRAIGHT AFTER A METHOD'S ANSWER is not built (the M16
    // review): it stopped the expression at the `[` with a hint about spaces around math
    // signs, which does not apply. Said as what it is, with the way round it.
    if (code_at(row, at) == token::left_square_bracket_token) {
        context.refuse(satl_line_not_understood,
                       index_after_an_answer(*shown + (after_a_link ? link_spelled(row, link_at) : std::string())), at);
        return Value();
    }
    // A CHANGE THAT ENDS A STATEMENT ANSWERS NOTHING (the M16 review, 2026-09-26). The
    // chain still on the name means every turn changed the variable in place -- .append,
    // .clear -- and `return *live` then copied the variable to answer a line that lets
    // the answer go. A list is a handle, so that copy was a count; a string is held by
    // value, so it was every character, and `t.append("ab")` in a loop was quadratic:
    // 200,000 appends took 6.10 s where 50,000 took 0.35 s. Used in an expression --
    // `display(t.append("!"))` -- the answer is still the string.
    if (on_the_name && context.statement && the_line_ends_at(row, at))
        return Value();
    return *live;
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
    // TOO LARGE TO BE ANY POSITION IS SAID AS THAT, rather than being turned
    // into the largest number there is and reported as one. It used to become
    // 18446744073709551615 and the refusal printed it, so a person who typed
    // more digits than a machine has was told about a number they never wrote.
    if (!fast::fits_a_count(*number)) {
        context.refuse(line_past_the_end,
                       what + "[" + fast::to_text(*number) + "]: that is more items than anything could hold",
                       where);
        return false;
    }
    out = fast::as_count(*number);
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

    // A STRING'S CHARACTER, `s[n]` (M16) -- counting from 1, as an item and a line do, and
    // answered as a string of one character: satellite has no character type.
    if (current.is_string())
        return character_of(current, index, what, where, context);

    if (satellite_file *file = current.as_file()) {
        // A LOCKED FILE'S LINE IS READ UNDER ITS LOCK, as its methods are (the second review:
        // f[n] beside another thread's f.append(...) read a half-moved line, S514).
        const ObjectHold one_read(&file->lock, LockUse::reading);
        if (one_read.code() != success) {
            context.refuse(one_read.code(), what + " is a locked file held by a thread that is waiting for this one");
            return Value();
        }
        return read_file_line(*file, index, what, context);
    }

    context.refuse(types_do_not_meet, what + " is " + current.kind_name() +
                                          ", and [ ] reads a line of a file, an item of a list, a key of an index, "
                                          "or a character of a string",
                   where);
    return Value();
}

// A METHOD MAY FOLLOW A LITERAL, NOT ONLY A NAME.
//
// `"abc".reverse()` and `123.reverse()` and `{1, 2}.size` all failed with "could
// not be read to the end of" -- every literal branch below answered its value
// and returned, so the `.` after it was a token nothing expected. A method on a
// NAME worked, so the gap was invisible until somebody wrote the obvious thing:
// the author asked for `.reverse()` on strings and numbers, and
// `"abc".reverse()` is how a person would first try it.
//
// `home` IS nullptr HERE AND MUST BE. A literal has no name, so nothing can be
// changed in place -- `{1, 2}.append(3)` is refused for exactly that reason, and
// container_calls.cpp says so in its own words rather than silently appending to
// something about to be thrown away.
Value maybe_a_method(const std::vector<std::bitset<16>> &row, std::size_t &at, Value value,
                     const char *what, ExpressionContext &context)
{
    if ((code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1))) ||
        a_member_next(row, at, value))
        return call_method(row, at, value, what, context);
    // `"abc"[1]`: [ ] reads a name's item, and a literal has none yet (call_method says
    // the same of a method's answer). It stopped the expression with a hint about spaces.
    if (code_at(row, at) == token::left_square_bracket_token) {
        context.refuse(satl_line_not_understood, index_after_an_answer(what), at);
        return Value();
    }
    return value;
}

// THE SLOT AT `a[i][j]`, AS SOMETHING THAT CAN BE CHANGED.
//
// WHY THIS EXISTS: `grid[1].append(3)` was refused -- "this one has no name to
// change" -- so a list inside a list could never be appended to, in a language
// whose whole container design is "any container with any container". The read
// path hands a method a COPY of the item, and appending to a copy changes
// nothing, so refusing was right and the refusal was the symptom.
//
// IT IS ONLY EVER CALLED WHEN THE CHAIN REALLY ENDS IN A MUTATOR, and that
// matters: descending makes every handle on the way unique (copy-on-write), so
// doing it on a READ would clone a shared list every time anybody looked at an
// item of it. The caller checks the method first and walks this way only then.
//
// Answers nullptr and refuses through `context` when the path does not lead to a
// slot; `inner` is left pointing at the shape the final slot was declared with,
// so a method writing into it can be held to the same type `a[i][j] = v` is.
Value *slot_through_index(Value &root, const std::vector<Value> &indices, const std::string &name,
                          std::size_t where, const TypeShape *shape, const TypeShape **inner,
                          ExpressionContext &context)
{
    static const TypeShape kAnything;
    Value *target = &root;
    const TypeShape *here = shape != nullptr ? shape : &kAnything;
    std::string what = name;

    for (std::size_t step = 0; step < indices.size(); ++step) {
        if (target->is_nothing()) {
            context.refuse(satl_line_not_understood,
                           name + " has no value yet -- give it one with = before changing an item of it", where);
            return nullptr;
        }
        here = &arm_holding(*here, *target);          // through a multiple, the arm it is held as
        if (IndexHandle *keys = target->as_index()) {
            std::string key_name;
            if (!key_name_of(indices[step], key_name)) {
                context.refuse(types_do_not_meet,
                               what + ": the key is " + std::string(indices[step].kind_name()) +
                                   ", and a key must be a number, a string, a bool, a binary or a percentage",
                               where);
                return nullptr;
            }
            satelliteIndex &body = about_to_change(*keys);
            satelliteObject *found = value_at(body, key_name);
            if (found == nullptr) {
                context.refuse(line_past_the_end, what + ": there is no such key in it", where);
                return nullptr;
            }
            here = (is_an_index_word(here->word) && here->parameters.size() > 1)
                       ? &here->parameters[1] : &kAnything;
            target = found;
            what += "[key]";
            continue;
        }
        ListHandle *handle = target->as_list();
        if (handle == nullptr) {
            context.refuse(types_do_not_meet,
                           what + " is " + target->kind_name() + ", and [ ] reaches into a list or an index", where);
            return nullptr;
        }
        unsigned long long int position = 0;
        if (!position_of(indices[step], position, what, where, context))
            return nullptr;
        satelliteList &body = about_to_change(*handle);
        satelliteObject *item = item_at(body, position);
        if (item == nullptr) {
            const std::size_t held = body.items.size();
            context.refuse(line_past_the_end,
                           what + "[" + std::to_string(position) + "]: " +
                               (held == 0 ? std::string("the list is empty")
                                          : "there is no such item -- the list holds " + std::to_string(held) +
                                                (held == 1 ? " item" : " items") + ", counting from 1"),
                           where);
            return nullptr;
        }
        here = (here->word == word::code_of(1, 4, 2) && !here->parameters.empty())
                   ? &here->parameters[0] : &kAnything;
        target = item;
        what += "[" + std::to_string(position) + "]";
    }
    *inner = &arm_holding(*here, *target);
    return target;
}

// WHAT FOLLOWS A ROW OF THE ARGUMENTS VARIABLE (2026-09-23). A program's row can hold a
// container since `args.l = satellite.container.list()` was built, so it is read and
// changed the way a named container is: `args.l[1]`, `args.l[1][2]`, `args.l.append(5)`,
// `args.grid[1].append(3)`. `current` is the row as read, `key` its name.
//
// THE READ IS ON THE COPY AND ONLY A MUTATOR WALKS TO THE ROW ITSELF -- the bracket chain
// in one_operand says why, and why the copy is let go of first.
Value after_an_argument(const std::vector<std::bitset<16>> &row, std::size_t &at, Value current,
                        const std::string &name, const std::string &key, Value &arguments, ExpressionContext &context)
{
    std::string what = name + "." + key;
    std::vector<Value> used;
    const std::size_t chain_at = at;
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
        used.push_back(index);
        current = index_into(current, index, what, opened_at, context);
        if (context.code != success)
            return Value();
        what += "[...]";
    }
    const Code method = code_at(row, at) == token::method_token ? code_at(row, at + 1) : 0;
    if (token::is_method_code(method) && container_arity(method) >= 0 && changes_a_container(method) &&
        !a_member_next(row, at, current)) {
        current = Value();
        Value *slot = an_argument_to_change(key, name, arguments, context);
        const TypeShape *inner = nullptr;
        if (slot != nullptr && !used.empty())
            slot = slot_through_index(*slot, used, name + "." + key, chain_at, nullptr, &inner, context);
        if (slot == nullptr || context.code != success)
            return Value();
        return call_method(row, at, *slot, what, context, slot, inner);
    }
    return maybe_a_method(row, at, std::move(current), "that argument", context);
}

// `{"zoe": 30, "al": 4}` -- A MAP WRITTEN WHOLE (2026-09-26), `at` on the first `:`,
// its key already worked out. Before this a map could only be filled one key at a
// time, so a list of maps took a line a key and a name a map -- and satl printed every
// map in exactly this form, which could not be read back in. Each key and value is a
// whole expression, as a list's items are, so a map of lists of maps is one literal.
//
// A KEY WRITTEN TWICE KEEPS ITS FIRST PLACE AND ITS LAST VALUE, which is what
// `m[k] = v` twice does and what Python's {"a": 1, "a": 2} does.
Value map_literal(const std::vector<std::bitset<16>> &row, std::size_t &at, std::size_t brace_at, Value key,
                  ExpressionContext &context)
{
    IndexHandle made = make_index();
    for (;;) {
        std::string key_name;
        if (!key_name_of(key, key_name)) {
            context.refuse(types_do_not_meet,
                           "a key in this map is " + std::string(key.kind_name()) +
                               ", and a key must be a number, a string, a bool, a binary or a percentage -- "
                               "something that cannot change after it is filed under",
                           brace_at);
            return Value();
        }
        ++at;                                        // past the `:`
        Value value = evaluate_at(row, at, 1, context);
        if (context.code != success)
            return Value();
        value_for_writing(*made, key_name, key) = std::move(value);

        // WHETHER A COMMA WAS TAKEN IS KEPT, never read back from `at - 1`: the code before
        // `at` may be the last of a string's payload, and U+0700 has the comma's code (the
        // review, 2026-09-26: {"a": "\u0700" "b": 2} was taken without its comma).
        const bool comma = code_at(row, at) == token::comma_token;
        if (comma) ++at;                             // and a trailing one, as a list allows
        if (code_at(row, at) == token::right_brace_token) {
            ++at;
            break;
        }
        if (comma) {
            key = evaluate_at(row, at, 1, context);
            if (context.code != success)
                return Value();
            if (code_at(row, at) == token::colon_token)
                continue;
        }
        context.refuse(satl_line_not_understood,
                       "this map was opened with { and never closed with } -- each entry is a key, a : and "
                       "a value, and entries are separated by commas, as in {\"zoe\": 30, \"al\": 4}",
                       brace_at);
        return Value();
    }
    return maybe_a_method(row, at, Value::of_index(std::move(made)), "that map", context);
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
        // satellite.variable.hex (2026-09-22): A HEX KEEPS ITS SIGN and stays a hex, as a
        // binary does -- -x1F is -x1F, worth -31, width 2. It was the number -31 until then.
        if (const satellite_hexadecimal_number *hex = inner.as_hexadecimal())
            return Value::of_hexadecimal(hex->negated());
        // -50% is a percentage below zero: `200 - -50%` grows 200 by half.
        if (const satellite_percentage *percent = inner.as_percentage())
            return Value::of_percentage(satellite_percentage{-percent->scaled});
        // -infinity IS infinity * -1, ONE VALUE (SATELLITE_INFINITY.md Q20): every count
        // turned over, the exponents shared. A negative infinity is below every number.
        if (const satellite_infinity *infinite = inner.as_infinity())
            return Value::of_infinity(satellite_infinity::negated(infinite));
        // A FLOAT'S SIGN IS ITS OWN BOOL (the author, 2026-09-22: "plus a sign which
        // is positive by default"), so -12.5 turns the bool over and keeps the digits.
        if (const satellite_float *real = inner.as_float())
            return Value::of_float(real->negated());
        // A FRACTION'S SIGN RIDES ON ITS NUMERATOR (satellite.variable.fraction,
        // 2026-09-22): -1/3 is (-1, 3), shown -1/3.
        if (const satellite_fraction *tied = inner.as_fraction())
            return Value::of_fraction(tied->negated());
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

    if (refuse_if_reserved(row, code, at, context))
        return Value();

    if (code == token::left_parenthesis_token) {
        ++at;
        Value inside = evaluate_at(row, at, 1, context);
        if (code_at(row, at) == token::right_parenthesis_token)
            ++at;
        return maybe_a_method(row, at, std::move(inside), "that value", context);
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
            return maybe_a_method(row, at, Value::of_list(make_list()), "that list", context);
        }
        for (;;) {
            Value item = evaluate_at(row, at, 1, context);
            if (context.code != success)
                return Value();
            // A `:` AFTER THE FIRST ITEM MAKES THE BRACES A MAP (2026-09-26): {"zoe": 30,
            // "al": 4}, the way satl has always printed one and Python writes a dict.
            if (items.empty() && code_at(row, at) == token::colon_token)
                return map_literal(row, at, brace_at, std::move(item), context);
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
        return maybe_a_method(row, at, Value::of_list(make_list(std::move(items))), "that list", context);
    }

    // A STRING LITERAL BECOMES A satellite_string HERE, which is the one doorway
    // UTF-8 comes in through. Strict: a bad sequence is string_error (4) naming
    // the byte, rather than a string standing for bytes that could not be read.
    if (code == token::string_token) {
        const std::string utf8 = string_at(row, at);
        Value held;
        std::size_t bad_offset = 0;
        const signed long long int made = Value::of_utf8(utf8, held, bad_offset);
        if (made != success) {
            context.refuse(made, "that string holds a byte at " + std::to_string(bad_offset) +
                                     " that is not part of any character");
            return Value();
        }
        return maybe_a_method(row, at, std::move(held), "that string", context);
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
        return maybe_a_method(row, at, Value::of_binary(std::move(bits)), "that binary", context);
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
        return maybe_a_method(row, at, Value::of_percentage(std::move(percent)), "that percentage", context);
    }

    // THE NUMBER LITERALS. An x literal is the hex's own (2026-09-22), and a number
    // with a point in it the float's: each reads its literal in
    // bytecode/<name>_values.cpp. text_at MOVES `at` past the payload, so the digits
    // are read once, here, and the choice is made on them.
    if (code == token::number_token || code == token::hexadecimal_token) {
        const unsigned int radix = radix_of(code);
        const std::string digits = text_at(row, at);
        // A NUMBER WITH A TOUCHING SLASH AFTER IT IS A FRACTION -- 1/3, 1.5/2
        // (satellite.variable.fraction, 2026-09-22) -- read in fraction_values.cpp.
        if (code == token::number_token && code_at(row, at) == token::fraction_token) {
            Value made;
            std::string why;
            const signed long long int held = fraction_literal(digits, row, at, made, why);
            if (held != success) {
                context.refuse(held, why);
                return Value();
            }
            return maybe_a_method(row, at, std::move(made), "that fraction", context);
        }
        if (code == token::hexadecimal_token || digits.find('.') != std::string::npos) {
            const bool hex = code == token::hexadecimal_token;
            Value made;
            std::string why;
            const signed long long int held =
                hex ? hexadecimal_literal(digits, made, why) : float_literal(digits, made, why);
            if (held != success) {
                context.refuse(held, why);
                return Value();
            }
            return maybe_a_method(row, at, std::move(made), hex ? "that hex" : "that float", context);
        }
        satellite_number value;
        const signed long long int held = fast::from_token_text(digits, radix, value);
        if (held != success) {
            context.refuse(held, digits + " is not a number this can read");
            return Value();
        }
        return maybe_a_method(row, at, Value::of_number(std::move(value)), "that number", context);
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

    // A satellite.library VALUE (2026-09-23) -- `satellite.library.span`, or
    // `satellite.library.settings.span` from a file this one includes: written at the top of
    // its file, read here, changed nowhere (library_values.hpp). A method may follow it.
    if (is_library_word(code) && code_at(row, at + 1) == token::method_token) {
        Value held = library_value_at(row, at, context);
        if (context.code != success)
            return Value();
        return maybe_a_method(row, at, std::move(held), "that satellite.library value", context);
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

        // THE TERMINAL'S SIZE, satellite.console.width and .height (console_calls.hpp):
        // asked fresh every time, because a terminal is resized while a program runs.
        if (is_console_fact(code)) {
            ++at;
            return console_fact(code);
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

        // A CAPSULE, CALLED FOR ITS ANSWER (2026-09-22, capsule_calls.hpp) -- `five()`,
        // and `other.greet(x)` or `tools.x()` when the first name is not a variable of
        // this body's: a variable is a method's receiver, and the checker refuses a
        // variable named like a file or a space, so the two cannot meet.
        if (code_at(row, at) == token::left_parenthesis_token) {
            at = name_at;
            Value answer = call_capsule_for_its_answer(row, at, {name}, k, context);
            if (context.code != success)
                return Value();
            return maybe_a_method(row, at, std::move(answer), "that answer", context);
        }
        Seen found = context.variables.seen(name);
        if (!found && code_at(row, at) == token::method_token) {
            std::size_t past = name_at;
            std::vector<std::string> names;
            dotted_names_at(row, past, names);
            if (names.size() >= 2 && code_at(row, past) == token::left_parenthesis_token) {
                at = name_at;
                Value answer = call_capsule_for_its_answer(row, at, names, past, context);
                if (context.code != success)
                    return Value();
                return maybe_a_method(row, at, std::move(answer), "that answer", context);
            }
        }
        if (!found) {
            context.refuse(name_not_declared, name + " has no satellite.variable line declaring it", name_at);
            return Value();
        }
        // THE ARGUMENTS VARIABLE'S ROWS BY NAME (main_arguments.hpp):
        // `arguments.username`, `arguments.memory.total`, and whatever follows the
        // row is a method on it -- `arguments.username.upper()`.
        if (found.declared == word::code_of(1, 6, 21) && code_at(row, at) == token::method_token) {
            bool read = false;
            std::string key;
            Value answer = read_an_argument(row, at, name, *found.value, context, read, key);
            if (context.code != success)
                return Value();
            if (read)
                return after_an_argument(row, at, std::move(answer), name, key, *found.value, context);
        }
        // A MAP'S OWN NUMBERED WORDS, 003'S SPELLING (1 4 1 2 and 1 4 1 1): `m.get(k)` IS m[k]
        // AND `m.set(k, v)` IS m[k] = v, through the same two functions, so a missing key and a
        // value that does not fit are refused in the same words. The author found them refused
        // on 2026-09-26 (ERRORS2 1b A/D) and his programs call them seventy times. `.has(k)` is
        // contains's third spelling, in REGISTRY.satellite.
        if (found.value->is_index() && code_at(row, at) == token::method_token &&
            code_at(row, at + 1) == token::name_token) {
            std::size_t k = at + 1;
            const std::string member = text_at(row, k);
            if ((member == "get" || member == "set") && code_at(row, k) == token::left_parenthesis_token) {
                const std::size_t open = k;
                std::vector<Value> arguments;
                ++k;
                while (code_at(row, k) != token::right_parenthesis_token) {
                    arguments.push_back(evaluate_at(row, k, 1, context));
                    if (context.code != success) return Value();
                    if (code_at(row, k) != token::comma_token) break;
                    ++k;
                }
                const std::size_t wanted = member == "get" ? 1 : 2;
                if (code_at(row, k) != token::right_parenthesis_token || arguments.size() != wanted) {
                    context.refuse(satl_line_not_understood,
                                   name + (member == "get" ? ".get(key) takes one key, and reads the value under it"
                                                           : ".set(key, value) takes a key and a value, and files one under the other"),
                                   open);
                    return Value();
                }
                at = k + 1;
                if (member == "get") {
                    Value answer = index_into(*found.value, arguments[0], name, name_at, context);
                    if (context.code != success) return Value();
                    return maybe_a_method(row, at, std::move(answer), "that value", context);
                }
                static const TypeShape kAnything;
                write_through_index(*found.value, {arguments[0]}, std::move(arguments[1]), name, name_at,
                                    found.shape != nullptr ? *found.shape : kAnything, context);
                return Value();
            }
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
            // THE FIRST BRACKET READS THE VARIABLE WHERE IT IS, AND COPIES NOTHING (M16). A
            // string is held by value, so `Value current = *found.value` -- this line until
            // today -- copied all of it for every s[i]: a loop reading each character of a
            // string of 163,840 took 4.6 s, and four times the characters seventeen times as
            // long. A list or an index is a handle, so for them it only saves a count.
            Value current;
            const Value *reading = found.value;
            std::string what = name;
            // KEPT SO A MUTATOR AT THE END OF THE CHAIN CAN BE WALKED AGAIN,
            // this time reaching the real slot rather than a copy of it. Reading
            // is done on the copy, which is what keeps a read from cloning a
            // shared list.
            std::vector<Value> used;
            const std::size_t chain_at = at;
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
                if (reading->is_nothing()) {
                    context.refuse(satl_line_not_understood, name + " has no value yet -- give it one with = before "
                                                                 "reading an item of it", opened_at);
                    return Value();
                }
                used.push_back(index);
                current = index_into(*reading, index, what, opened_at, context);
                reading = &current;
                if (context.code != success)
                    return Value();
                what += "[...]";
            }
            // `list[i].call_x()` -- A CAPSULE OF THE OBJECT AN ITEM HOLDS (the author's
            // programs do it 48 times). An object is a reference, so the copy read above
            // IS the object, and nothing needs walking to again.
            if (a_member_next(row, at, current))
                return call_method(row, at, current, what, context);
            if (code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1))) {
                // A MUTATOR AT THE END OF A CHAIN GETS THE REAL SLOT.
                // `grid[1].append(3)` has to change grid, and `current` above is
                // a copy -- so the path is walked a second time, by reference,
                // and ONLY when the method really changes something.
                if (container_arity(code_at(row, at + 1)) >= 0 && changes_a_container(code_at(row, at + 1))) {
                    const Seen writable = context.variables.seen(name);
                    if (writable) {
                        // LET GO OF THE ITEM WE READ BEFORE WALKING TO IT AGAIN.
                        //
                        // `current` holds a handle to the very item about to be
                        // changed, so copy-on-write sees use_count() == 2 and
                        // clones the whole inner list -- on EVERY append.
                        // Measured: 10,000 nested appends 1.569s, 40,000 appends
                        // 23.667s. Fifteen times the work for four times the
                        // appends, which is the same quadratic trap as the outer
                        // .append and as 003's, arrived at by a third route.
                        //
                        // ONE LINE FIXES IT AND NOTHING SAYS SO IF IT IS REMOVED:
                        // the output is identical either way.
                        current = Value();
                        const TypeShape *inner = nullptr;
                        Value *slot = slot_through_index(*writable.value, used, name, chain_at,
                                                         writable.shape, &inner, context);
                        if (context.code != success)
                            return Value();
                        if (slot != nullptr)
                            return call_method(row, at, *slot, what, context, slot, inner);
                    }
                }
                // Called what it was written as, `w[...].at(9)`: the item, not the list.
                return call_method(row, at, current, what, context);
            }
            return current;
        }
        // THE THREE TOKENS TOGETHER (the author): a period, a method's own code,
        // and a `(`. call_method above says what happens then. After an OBJECT, the
        // name is one of its spacesuit's capsules, whatever it is spelled.
        //
        // THE VARIABLE'S OWN OBJECT -- or the field of this body's object -- so
        // `names.append("x")` changes names rather than a copy of it (value.hpp's Seen).
        if ((code_at(row, at) == token::method_token && token::is_method_code(code_at(row, at + 1))) ||
            a_member_next(row, at, *found.value))
            return call_method(row, at, *found.value, name, context, found.value, found.shape);
        return *found.value;
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
        if (refuse_if_reserved(row, op, at, context))
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

    // satellite.thread.new's ARGUMENT IS A CALL TO KEEP, NOT TO MAKE (thread_calls.hpp), so
    // it is taken before the loop below works it out -- which would run the capsule here,
    // on this thread, and hand the thread its answer. 003's first build did exactly that.
    if (is_thread_word(code) && code_at(row, at) == token::left_parenthesis_token)
        return call_thread_new(row, at, context);
    // ...and satellite.access's argument is a NAME, read by its declaration (access_calls.hpp).
    if (is_access_word(code) && code_at(row, at) == token::left_parenthesis_token)
        return call_access(row, at, context);

    // THE ARGUMENTS ARE A LIST (2026-09-18), divided by commas at their own depth:
    // `satellite.file.new(path, "text")` is the first word a program can call with
    // two. A library still takes one, and says so below when it is given more.
    std::vector<Value> arguments;
    // AND ITS NAMED OPTIONS, `foreground=xFF8800` (console_calls.hpp) -- which the
    // checker has already judged by name, so here they are only read and worked out.
    std::vector<NamedOption> options;
    if (code_at(row, at) == token::left_parenthesis_token) {
        ++at;
        if (code_at(row, at) != token::right_parenthesis_token) {
            for (;;) {
                // AN OPTION IS MADE ONLY WHEN THERE IS ONE: a NamedOption built and
                // thrown away for every argument was part of what a plain display paid.
                std::string option_name;
                std::size_t value_at = 0;
                if (an_option_at(row, at, option_name, value_at)) {
                    at = value_at;
                    options.push_back(NamedOption{std::move(option_name), evaluate_at(row, at, 1, context)});
                } else {
                    arguments.push_back(evaluate_at(row, at, 1, context));
                }
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
        //
        // UNLESS THE LINE SIMPLY ENDED: then the ) is missing, or a string with no closing "
        // swallowed it, and "check that every math sign has a space" sent a person looking
        // for a sign that was never there (the error sweep, 2026-09-25 -- the author's own
        // quad_main.satl line 6 was told exactly that).
        if (context.code == success && code_at(row, at) != token::right_parenthesis_token) {
            const Code stopped = code_at(row, at);
            const bool line_ended = stopped == token::line_end_token || stopped == token::comment_token ||
                                    stopped == token::end_of_file_token || at >= row.size();
            context.refuse(satl_line_not_understood,
                           std::string(word::spelling_of(code)) +
                               (line_ended ? "'s ( is never closed -- the line ends before its ), or a string on "
                                             "it has no closing \""
                                           : " was given something it could not read to the end of -- check that "
                                             "every math sign has a space on both sides"));
        }
        while (at < row.size() && code_at(row, at) != token::right_parenthesis_token) ++at;
        if (at < row.size()) ++at;
    }
    if (context.code != success)
        return Value();

    // .center() WRITTEN ON A DISPLAY (the author, 2026-09-25: satellite.console.display(
    // "something").center() "renders the text in the middle of the console window"). Taken
    // here, brackets and all, so the line is centred BEFORE it is printed -- a method on
    // display's answer would come after the line had already gone out.
    //
    // AND A DISPLAY ASKS NO OTHER FAMILY (the author, 2026-09-26: "we need to bypass those
    // 8,000 lines when we are printing at least"). One compare against a constant says it is
    // display, and it goes straight to its own path below: not one of the file, info,
    // infinity, window, container or console questions is asked of it. callgrind on build
    // 0108 had those questions at ~10,500 of the ~11,600 instructions a display cost.
    const bool to_the_screen = is_display_word(code);
    bool centred = false;
    if (to_the_screen && code_at(row, at) == token::method_token &&
        code_at(row, at + 1) == token::center_token) {
        centred = true;
        at += 2;
        if (code_at(row, at) == token::left_parenthesis_token && code_at(row, at + 1) == token::right_parenthesis_token)
            at += 2;
    }

    if (!to_the_screen) {
        // satellite.file's words answer a HANDLE, which no library can (file_calls.hpp).
        if (is_file_word(code))
            return call_file_word(code, arguments, row, context);
        // ...and satellite.info's words answer a list of indexes, which no library can
        // make either (info_calls.hpp).
        if (is_info_word(code))
            return call_info_word(code, arguments, context);
        // ...and so does satellite.infinity() (infinity_calls.hpp).
        if (is_infinity_word(code))
            return call_infinity_word(code, arguments, context);
        // ...and so does satellite.window.new() (window_calls.hpp). Three word
        // families now, which is why that header stops calling it a departure.
        //
        // A WINDOW IS THE MAIN THREAD'S (window_desk.hpp: one interpreter thread writes a
        // piece), so a program's thread may not open or build one yet (THREADS.md T3).
        // ASKED ONCE, and it is one read of window_calls.cpp's table.
        if (is_window_word(code)) {
            if (on_a_program_thread()) {
                context.refuse(thread_cannot_share_yet, std::string(word::spelling_of(code)) +
                                                            " -- a window belongs to the main thread, and a thread "
                                                            "the program started may not open or build one yet");
                return Value();
            }
            return call_window_word(code, arguments, context);
        }
        // ...and satellite.container.list(), a list of nothing (container_calls.hpp).
        if (is_container_word(code))
            return call_container_word(code, arguments, context);
    }
    // ONE LINE AT A TIME ONCE A THREAD EXISTS (machine/console_lock.hpp): the console's
    // words and every numbered library below are called holding the console lock, because
    // display writes std::cout from inside its library. NOT input, which waits for a person:
    // holding the lock there would stop every other thread's lines until somebody typed.
    // Before any start() the hold holds nothing, and a plain display pays one load.
    const bool waits_for_a_person = code == word::fixed_code<1, 5, 2> || code == word::fixed_code<1, 5, 3>;
    std::optional<ConsoleHold> one_line;
    if (!waits_for_a_person)
        one_line.emplace();

    // ...and satellite.console's own words and satellite.terminal's (console_calls.hpp).
    if (!to_the_screen && is_console_word(code))
        return call_console_word(code, arguments, options, context);

    if (arguments.size() > 1) {
        context.refuse(satl_line_not_understood, std::string(word::spelling_of(code)) +
                                                     " takes one argument, and was given " +
                                                     std::to_string(arguments.size()));
        return Value();
    }
    // MOVED, NOT COPIED: `arguments` is this call's own, and a copy here was a second
    // whole copy of every value displayed (the first attempt's review, 2026-09-26).
    Value argument = arguments.empty() ? Value() : std::move(arguments.front());

    signed long long int answer = success;
    if (scenarios == nullptr) {
        context.refuse(not_built_yet, std::string(word::spelling_of(code)) + " has no library built yet");
        return Value();
    }
    // display WITH OPTIONS, OR WITH THE CONSOLE'S COLOURS SET, is one styled line
    // (console_calls.hpp). Without either it takes the path below untouched, so a
    // plain display pays one test and nothing else.
    if (to_the_screen && (centred || !options.empty() || (console_colours_ever_set() && console_colours().any())))
        return display_with_options(code, *scenarios, argument, options, context, centred);
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
    // A COLOURED STRING'S CODES ARE LEFT OUT INTO A PIPE (console_style.hpp's
    // for_the_screen) -- by display only; any other word is handed the string whole.
    else if (argument.is_string() && scenarios->text != nullptr)
        answer = scenarios->text(to_the_screen ? screen_text(argument.text_utf8()) : argument.text_utf8(), true);
    else if (argument.is_number())
        answer = display_a_number(*scenarios, *argument.as_number());
    // A binary leaves as the text it was written as, b and leading zeros and all.
    else if (argument.is_binary() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_binary()->written(), true);
    // satellite.variable.hex (2026-09-22): a hex leaves as its x and its digits, upper
    // case and as wide as written -- x00FF, -x1F.
    else if (argument.is_hexadecimal() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_hexadecimal()->written(), true);
    // satellite.variable.color (2026-09-22): a colour leaves as the author writes one --
    // x00FF00, and x00FF00, 50 when it is see-through at all.
    else if (argument.is_color() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_color()->written(), true);
    // A percentage leaves as its digits and its %: 50%, 12.5%.
    else if (argument.is_percentage() && scenarios->text != nullptr)
        answer = scenarios->text(argument.as_percentage()->written(), true);
    // An infinity leaves as its one set of parentheses: (infinity), (-infinity).
    else if (argument.is_infinity() && scenarios->text != nullptr)
        answer = scenarios->text(satellite_infinity::display(argument.as_infinity()), true);
    else if (argument.is_bool() && scenarios->flag != nullptr)
        answer = scenarios->flag(*argument.as_bool(), true);
    // A FLOAT LEAVES AS ITS DIGITS ROUND ONE POINT -- 12.34, -0.5, 2.0 -- the same
    // text `.string` answers (object_float.cpp's float_to_string, 2026-09-22).
    else if (argument.is_float() && scenarios->text != nullptr) {
        satellite_string written;
        std::string why;
        const signed long long int made = argument.to_string(written, why);
        if (made != success) {
            context.refuse(made, std::string(word::spelling_of(code)) + " was given a float, and " + why);
            return Value();
        }
        answer = scenarios->text(written.to_utf8(), true);
    }
    // A FRACTION LEAVES AS ITS TWO NUMBERS ROUND THE SLASH -- 1/3, 2/4, 1.5/2 -- the
    // same text `.string` answers (object_fraction.cpp's fraction_to_string,
    // satellite.variable.fraction, 2026-09-22).
    else if (argument.is_fraction() && scenarios->text != nullptr) {
        satellite_string written;
        std::string why;
        argument.to_string(written, why);
        answer = scenarios->text(written.to_utf8(), true);
    }
    // A CONTAINER GIVEN TO A WORD THAT ONLY TAKES TEXT reads back as what was
    // typed: {1, "two"}, or {"zoe": 1, "al": 2} for an index. satellite_object.cpp's
    // to_string is the one spelling, so display and a refusal quote it the same way.
    // A WINDOW READS BACK AS WHICH WINDOW IT IS -- (window "my title") -- for the
    // same reason a container reads back as what was typed: the one thing a person
    // displays a window for is to see which one they have hold of. Same branch as
    // the containers, because satellite_object.cpp's to_string is the one spelling
    // for all of them.
    else if ((argument.is_list() || argument.is_index() || argument.is_window() || argument.is_thread()) &&
             scenarios->text != nullptr) {
        satellite_string written;
        std::string why;
        const signed long long int made = argument.to_string(written, why);
        if (made != success) {
            context.refuse(made, std::string(word::spelling_of(code)) + " was given " +
                                     argument.kind_name() + ", and " + why);
            return Value();
        }
        answer = scenarios->text(to_the_screen ? screen_text(written.to_utf8()) : written.to_utf8(), true);
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
        here = &arm_holding(*here, *target);          // through a multiple, the arm it is held as (type_shape.hpp)
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
            if (is_an_index_word(here->word) && !here->parameters.empty() &&
                !value_fits(here->parameters[0], indices[step], unfit)) {
                context.refuse(types_do_not_meet,
                               "the key does not fit: " + unfit, where);
                return context.code;
            }
            // A `multiple<A, B>` NAME CONSTRAINS ITSELF, NOT WHAT IS INSIDE IT.
            // Its parameters are the types the NAME may hold; reading them as an
            // index's <key, value> made `multiple<list, number> m = {1, 2}` then
            // `m[1] = "text"` refuse, because parameters[0] (a list) was being
            // asked to describe an item.
            const bool a_plain_index = is_an_index_word(here->word);
            const TypeShape *inside = (a_plain_index && here->parameters.size() > 1)
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
        // The same rule for a list: only a DECLARED LIST says what its items are.
        const TypeShape *inside_list = (here->word == word::code_of(1, 4, 2) && !here->parameters.empty())
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

std::string link_spelled(const std::vector<std::bitset<16>> &row, std::size_t dot)
{
    std::size_t k = dot + 1;
    std::string out = ".";
    if (code_at(row, k) == token::name_token) {
        out += text_at(row, k);
    } else {
        out += method_spelling(code_at(row, k));
        ++k;
    }
    if (code_at(row, k) == token::left_parenthesis_token)
        out += code_at(row, k + 1) == token::right_parenthesis_token ? "()" : "(...)";
    return out;
}

std::string index_after_an_answer(const std::string &spelled)
{
    return spelled + "[...]: [ ] is not built yet straight after a call's answer, a literal or a bracket -- give "
                     "the value a name first, and read [ ] of that name";
}

} // namespace satellite004
