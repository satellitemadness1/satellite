// satellite/bytecode/program_check.cpp -- NOTHING RUNS BEFORE THE WHOLE PROGRAM
// IS CHECKED. Every capsule body is walked and every statement judged before
// main is entered, so a program that cannot finish does not half-print first --
// which check.sh asserts in as many words ("nothing ran before the refusal").
//
// IT CHECKS SHAPE AND NAMES, NOT TYPES, and the line between those is the point
// of this file. A statement's SHAPE is knowable without running: whether a word
// is a call or a declaration, whether a library exists for it, whether a name
// was ever declared, whether a while has a body. A statement's TYPES are not --
// `n = a + b` depends on what a and b hold, and working that out here would mean
// running the program to check the program. So the kinds are judged where they
// are known, at the moment the operator meets them (expression.cpp), and this
// pass guarantees only that every line is one the walker recognises.
//
// WHAT THAT BUYS, EXACTLY: a program whose fifth line names a word with no
// library, or a variable nothing declared, prints nothing at all rather than
// four lines and then a refusal. What it does not buy is catching `1 + "a"` in an
// unrun branch, and this file does not pretend to.
//
// TWO TYPE RULES ARE CHECKED HERE, and only because each is a SPELLING and not a
// type: a satellite.variable.binary given digits with no b in front of them (the
// author, 2026-09-16), and a satellite.variable.percentage given digits with no %
// after them. The b and the % are visible in the text, so neither needs anything
// to run -- see binary_is_written_with_b and percentage_is_written_with_percent.
//
// THE DECLARED NAMES ARE TRACKED PER CAPSULE, which is the same rule run_body
// enforces by handing each body its own table: there are no globals, so a name
// declared in one capsule is not declared in another.

#include "program_walk.hpp"

#include "file_calls.hpp"
#include "container_calls.hpp"
#include "word_codes.hpp"
#include "../machine/s_codes.hpp"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace satellite004 {
namespace {

using token::Code;
// Each declared name and the word that declared it -- the TYPE is kept so that a
// later `bits = 1010` can be judged by the same rule as the declaration was.
using DeclaredNames = std::unordered_map<std::string, Code>;

// A for's NUMBER OUTLIVES NOTHING (MILESTONES M20.A: it "exists while the for
// loop is running"), so the checker has to forget it where the walker erases it,
// or `i` written after the loop would pass the check and be refused at run time --
// with the loop's own output already printed, which is the one thing this whole
// file exists to prevent. Each entry is the code just past a for's `}` and the
// name that dies there; forget_the_finished below is called before every
// statement, so a name is gone by the first statement at or after that point.
//
// It is a list and not a stack because it is read by POSITION: nested loops end
// in the order they must, and two loops that use `i` one after the other are both
// allowed -- the first has forgotten it before the second declares it.
using EndingNames = std::vector<std::pair<std::size_t, std::string>>;

void forget_the_finished(EndingNames &ending, std::size_t at, DeclaredNames &declared)
{
    for (std::size_t which = ending.size(); which > 0; --which) {
        if (ending[which - 1].first > at)
            continue;
        declared.erase(ending[which - 1].second);
        ending.erase(ending.begin() + static_cast<std::ptrdiff_t>(which - 1));
    }
}

// A BINARY IS WRITTEN WITH ITS b (the author, 2026-09-16): "if the user doesn't
// enter "b" and enters satellite.variable.binary just require them to enter the
// b, spit out an ERROR: expected "b"+whatever they entered".
//
// `at` is the first code of the value, straight after the `=`. A bare number
// there is the mistake, and the answer is what they wrote with a b in front:
//
//     satellite.variable.binary my_number = 10101010
//     ERROR: expected b10101010
//
// THE SUGGESTION IS NOT QUOTED, though the author's sentence quotes the b. In
// satellite a quote makes a STRING, so `expected "b10101010"` would point at the
// one spelling that is still wrong. If the author wants the quotes, it is this
// one string.
//
// ONLY THE FIRST VALUE IS JUDGED, which is the author's case -- the value they
// entered. `bits = b1010 * 2` is not this mistake (the 2 is a count, not bits)
// and is left to the walker, which refuses the number the arithmetic answers.
//
// DIGITS THAT ARE NOT ALL 0 AND 1 GET THEIR OWN SENTENCE, because `expected b12`
// would send a person to write b12, which is not binary either. The same goes
// for `b12` itself: the lexer makes a NAME of it (b and 0s and 1s is the only
// binary it knows), and "b12 has no satellite.variable line" is true and useless.
signed long long int binary_is_written_with_b(const std::vector<std::bitset<16>> &row, std::size_t at,
                                              const DeclaredNames &declared, std::string &why)
{
    // A BRACKET IS NOT A VALUE, so `= (10101010)` is judged by what is inside it.
    // Without this the brackets hid the mistake and the program ran first.
    //
    // A MINUS SIGN IS PART OF THE VALUE, as it is for a percentage: a binary keeps
    // its sign (the author, 2026-09-17: "give it a different number and keep a
    // sign"), so -b1010 declares, and `= -1010` is ERROR: expected -b1010. Each
    // minus turns the suggestion over, as it would the value. (For one commit the
    // minus was refused outright -- "a binary has no minus sign" -- which is what
    // that ruling answered.)
    bool below_zero = false;
    for (;; ++at) {
        const Code ahead = code_at(row, at);
        if (ahead == token::tight_minus_token || ahead == token::minus_token)
            below_zero = !below_zero;
        else if (ahead != token::left_parenthesis_token)
            break;
    }
    const std::string sign = below_zero ? "-" : "";
    const Code code = code_at(row, at);
    if (code != token::number_token && code != token::name_token)
        return success;
    std::size_t k = at;
    const std::string entered = text_at(row, k);

    // 0b10101010 AND 0x1F, the C and Python spellings. The lexer reads the 0 as a
    // number and the rest as a b or x literal of its own, so without this the
    // answer was `expected b0` -- a real binary, and the wrong one.
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::binary_token) {
        std::size_t digits = k;
        why = "ERROR: expected " + sign + "b" + text_at(row, digits);
        return types_do_not_meet;
    }
    if (code == token::number_token && entered == "0" && code_at(row, k) == token::hexadecimal_token) {
        std::size_t digits = k;
        why = "ERROR: " + sign + "0x" + text_at(row, digits) + " is not binary -- binary is b and then 0s and 1s, like b1010";
        return types_do_not_meet;
    }

    bool only_bits = true;
    for (const char c : entered) only_bits = only_bits && (c == '0' || c == '1');

    if (code == token::number_token) {
        why = only_bits ? "ERROR: expected " + sign + "b" + entered
                        : "ERROR: " + sign + entered + " is not binary -- binary is b and then 0s and 1s, like b1010";
        return types_do_not_meet;
    }

    // A NAME: only `b` and then digits, and only when nothing declared it.
    if (declared.find(entered) != declared.end() || entered.size() < 2 || entered[0] != 'b')
        return success;
    for (std::size_t i = 1; i < entered.size(); ++i)
        if (entered[i] < '0' || entered[i] > '9')
            return success;
    why = "ERROR: " + sign + entered + " is not binary -- a binary digit is 0 or 1";
    return types_do_not_meet;
}

// A PERCENTAGE IS WRITTEN WITH ITS %, by the same rule as a binary's b (the
// author's for binary, 2026-09-16, applied here on 2026-09-17 because a
// percentage has the same shape of mistake): `satellite.variable.percentage p = 50`
// is ERROR: expected 50%. The % is in the text, so nothing has to run to see it
// is missing. Only the first value, inside any brackets, as for binary.
//
// A MINUS SIGN IS PART OF THE VALUE, so `p = -50` is ERROR: expected -50% (the
// review of ab01a74, 2026-09-17: the - was not skipped, so the program printed
// first and was refused at run time). -50% is a real percentage, and the
// suggestion keeps the sign: each minus turns it over, as it would the value.
signed long long int percentage_is_written_with_percent(const std::vector<std::bitset<16>> &row, std::size_t at,
                                                        std::string &why)
{
    bool below_zero = false;
    for (;; ++at) {
        const Code code = code_at(row, at);
        if (code == token::tight_minus_token || code == token::minus_token)
            below_zero = !below_zero;
        else if (code != token::left_parenthesis_token)
            break;
    }
    if (code_at(row, at) != token::number_token)
        return success;
    std::size_t k = at;
    why = "ERROR: expected " + std::string(below_zero ? "-" : "") + text_at(row, k) + "%";
    return types_do_not_meet;
}

// AFTER A DECLARED NAME, ONLY `=` -- or the line's end, for a declaration with no
// value yet. `n += 1` was a line the walker skipped without a word: n kept its
// value and the program exited 0, because run_assignment read anything that was
// not `=` as "a declaration with no value" (found by the binary review,
// 2026-09-16, and true of every type). `+=` and its family are real tokens with
// no scenario, so they are not_built_yet by name; anything else is not a
// statement a name can start. `k` is the code straight after the name.
signed long long int after_the_name(const std::vector<std::bitset<16>> &row, std::size_t k,
                                    const std::string &name, bool declaring, std::string &why)
{
    const Code code = code_at(row, k);
    if (code == token::assign_token)
        return success;
    const bool ends = code == token::line_end_token || code == token::comment_token ||
                      code == token::end_of_file_token;
    if (ends && declaring)
        return success;

    const char *sign = code == token::plus_assign_token ? "+"
                     : code == token::minus_assign_token ? "-"
                     : code == token::times_assign_token ? "*"
                     : code == token::divide_assign_token ? "/"
                     : code == token::modulus_assign_token ? "%" : nullptr;
    if (sign != nullptr) {
        why = name + " " + sign + "= ... is not built yet -- write " + name + " = " + name + " " + sign + " ...";
        return not_built_yet;
    }
    why = ends ? name + " on its own line does nothing -- give it a value with ="
               : name + " is followed by something that is not = , and there is no statement of that shape";
    return satl_line_not_understood;
}

// THE BRACKETS THAT OPEN AT `open` -- a `(` or a `[` -- and how many arguments
// they hold: 0 for empty ones, otherwise the commas at their own depth plus one.
// `close` is left on the bracket that closes them. False when they never close on
// this line. A payload is skipped, never read, so a comma or a bracket inside a
// string is not one of theirs.
bool brackets_at(const std::vector<std::bitset<16>> &row, std::size_t open, std::size_t &close, std::size_t &count)
{
    std::size_t depth = 0, commas = 0;
    // A BRACED LIST IS ONE ARGUMENT, HOWEVER MANY COMMAS IT HOLDS. Counted apart
    // from `depth` rather than folded into it, because a `}` must never be able
    // to close the brackets: depth reaching 0 is what ends this loop, and a
    // stray brace driving it there would make `f({a, b}` look closed.
    //
    // Without this, `satellite.feedback({"a", "b"})` is refused before it runs,
    // by a checker counting two arguments in a call that takes one -- which is
    // exactly the message a person would get for their own mistake, so it must
    // not be given for the language's.
    std::size_t braces = 0;
    bool any = false;
    std::size_t at = open;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { any = true; text_at(row, at); continue; }
        if (code == token::line_end_token || code == token::end_of_file_token) break;
        if (code == token::left_brace_token) {
            any = true;
            ++braces;
        } else if (code == token::right_brace_token) {
            any = true;
            if (braces > 0) --braces;
        } else if (code == token::left_parenthesis_token || code == token::left_square_bracket_token) {
            if (depth > 0) any = true;
            ++depth;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token) {
            if (--depth == 0) { close = at; count = any ? commas + 1 : 0; return true; }
        } else {
            any = true;
            if (code == token::comma_token && depth == 1 && braces == 0) ++commas;
        }
        ++at;
    }
    close = at;
    count = 0;
    return false;
}

// A METHOD ON A DECLARED NAME, judged by the name's declared TYPE -- which the
// checker has, since DeclaredNames keeps the declaring word (the review,
// 2026-09-18: `n.append("x")` on a number and `f.replace(1)` on a file passed the
// check and were refused after earlier lines had printed). `k` is the code after
// the name. Only the first method is judged: what a method answers is a run-time
// fact, so a chain's later segments are left to the walker.
signed long long int method_on_a_name(const std::vector<std::bitset<16>> &row, std::size_t k,
                                      const std::string &name, Code declared_as, std::string &why)
{
    if (code_at(row, k) != token::method_token || !token::is_method_code(code_at(row, k + 1)))
        return success;
    const Code method = code_at(row, k + 1);
    const std::string spelling = std::string(name) + "." + method_spelling(method);
    const int arity = file_method_arity(method);
    const bool of_a_string_or_number = method == token::find_token || method == token::add_token ||
                                       method == token::to_string_token || method == token::to_number_token ||
                                       method == token::to_binary_token || method == token::to_hexadecimal_token;

    // A CONTAINER'S OWN METHODS (the author, 2026-09-18). `.reverse()` is not in
    // this list because it is not a container's: it is on every type that has an
    // order -- a string, a number, a binary -- so it is allowed on anything here
    // and refused at the value, where the kind is actually known.
    // ASKED, NEVER COPIED: container_arity IS the list of container methods, and
    // it lives beside the code that implements them (container_calls.hpp). The
    // hand-written set that used to be here went stale the same afternoon it was
    // written, refusing `n.first` before the program ran while the walker had it.
    const bool of_a_container = container_arity(method) >= 0;
    const bool a_container = declared_as == word::code_of(1, 4, 2) || declared_as == word::code_of(1, 4, 5) ||
                             declared_as == word::code_of(1, 4, 6);
    if (method == token::reverse_token)
        return success;                  // every type with an order has one
    if (a_container) {
        if (of_a_container || of_a_string_or_number) return success;
        why = spelling + " is not built for " + word::spelling_of(declared_as) +
              " yet -- a container has .append, .size, .contains, .sort().by_name(), "
              ".sort().by_value() and .reverse()";
        return not_built_yet;
    }

    if (declared_as != word::code_of(1, 6, 2)) {
        if (of_a_string_or_number) return success;
        why = spelling + " is not built for " + word::spelling_of(declared_as) + " yet -- " + so_far_whose(method);
        return not_built_yet;
    }
    if (arity < 0) {
        why = spelling + " -- a file has no " + method_spelling(method) +
              " (SATELLITE_FILE_OPERATIONS Part 3 lists what a file does)";
        return types_do_not_meet;
    }
    std::size_t close = k + 2, given = 0;
    const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
    if (bracketed && !brackets_at(row, k + 2, close, given)) {
        why = spelling + "( is never closed on its line";
        return satl_line_not_understood;
    }
    if (given != static_cast<std::size_t>(arity) || (arity > 0 && !bracketed)) {
        why = spelling + " takes " + std::to_string(arity) + (arity == 1 ? " argument" : " arguments") +
              (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
        return satl_line_not_understood;
    }
    return success;
}

// WHAT A METHOD-CALL STATEMENT MAY BE, WHOLE (the review: `f.size = 3`, `f.`,
// `f[` and `f.append("x") f.append("y")` passed the check and failed after
// earlier lines had printed). From `k`, any run of `.method`, `.method(...)` and
// `[...]`, and then the line's end: nothing a call answers can be given a value,
// and one statement is one line.
signed long long int a_call_to_its_end(const std::vector<std::bitset<16>> &row, std::size_t k,
                                       const std::string &name, std::string &why)
{
    // WHAT THE RUN ENDED ON, which is the whole of how `a[1] = x` is told from
    // `f.size = 3`. Both are a name, a run of somethings, and an `=`. The first
    // is an assignment into a list and the second is giving a value to a call's
    // answer, which is meaningless -- and the difference is only that one ended
    // on `]` and the other on a method.
    bool ended_on_an_index = false;
    for (;;) {
        std::size_t close = k, count = 0;
        if (code_at(row, k) == token::method_token && token::is_method_code(code_at(row, k + 1))) {
            k += 2;
            if (code_at(row, k) == token::left_parenthesis_token) {
                if (!brackets_at(row, k, close, count)) break;
                k = close + 1;
            }
            ended_on_an_index = false;
            continue;
        }
        if (code_at(row, k) == token::left_square_bracket_token) {
            if (!brackets_at(row, k, close, count)) break;
            k = close + 1;
            ended_on_an_index = true;
            continue;
        }
        break;
    }
    const Code code = code_at(row, k);
    if (code == token::line_end_token || code == token::comment_token || code == token::end_of_file_token)
        return success;
    // `a[i] = v`, and `a[i][j] = v` (the author, 2026-09-18). The walker's
    // run_indexed_assignment does the work; here it is only a shape to allow.
    if (code == token::assign_token && ended_on_an_index)
        return success;
    why = name + " is followed by something that is not a method call -- a call's answer cannot be given a "
                 "value, a bracket must close on its line, and one statement is one line";
    return satl_line_not_understood;
}

// Every name a statement USES as a value -- so a name with no declaration is
// caught before anything runs. A name followed by `(` is a capsule and is
// checked against the capsule table instead.
signed long long int names_in_statement(const std::vector<std::bitset<16>> &row,
                                        std::size_t from,
                                        std::size_t stop,
                                        const DeclaredNames &declared,
                                        const CapsuleTable &capsules,
                                        const FunctionTable &functions,
                                        std::string &why)
{
    for (std::size_t at = from; at < stop && at < row.size(); ) {
        const Code code = code_at(row, at);

        if (code == token::name_token) {
            std::size_t k = at;
            const std::string name = text_at(row, k);
            if (code_at(row, k) == token::left_parenthesis_token) {
                if (capsules.find(name) == capsules.end()) {
                    why = "no capsule named " + name;
                    return satl_line_not_understood;
                }
            } else if (declared.find(name) == declared.end()) {
                why = name + " has no satellite.variable line declaring it";
                return name_not_declared;
            } else {
                const signed long long int judged = method_on_a_name(row, k, name, declared.find(name)->second, why);
                if (judged != success) return judged;
            }
            at = k;
            continue;
        }

        // A WORD USED AS A CALL MUST HAVE A LIBRARY. A word with none is
        // not_built_yet (14) with its own name, which is what 003 did and what a
        // person can act on (function_table.hpp).
        // satellite.file's words are the object model's and have none (file_calls.hpp).
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token &&
            functions[code] == nullptr && !is_file_word(code)) {
            why = std::string(word::spelling_of(code)) + " has no library built for it yet";
            return not_built_yet;
        }

        // HOW MANY ARGUMENTS A WORD WAS GIVEN, judged here and not after the lines
        // above it have run (the review, 2026-09-18). A file word takes its own
        // count; a library takes one -- call_word hands it one value, and
        // `satellite.variable.string.replace("a", "b")` lexed to a two-parameter row
        // with a library once the lexer began counting commas.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            std::size_t close = at + 1, given = 0;
            if (brackets_at(row, at + 1, close, given)) {
                if (is_file_word(code) && given != file_word_arity(code)) {
                    why = file_word_takes(code) + ", and was given " + std::to_string(given) + " arguments";
                    return satl_line_not_understood;
                }
                if (!is_file_word(code) && given > 1) {
                    const std::string spelled(word::spelling_of(code));
                    why = spelled.substr(0, spelled.find('(')) + " takes one argument, and was given " +
                          std::to_string(given);
                    return satl_line_not_understood;
                }
            }
        }

        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        ++at;
    }
    return success;
}

// One statement, judged without running it. `at` is left on the code after it.
signed long long int check_statement(const std::vector<std::bitset<16>> &row,
                                     std::size_t &at,
                                     const CapsuleTable &capsules,
                                     const FunctionTable &functions,
                                     DeclaredNames &declared,
                                     EndingNames &ending,
                                     std::string &why)
{
    forget_the_finished(ending, at, declared);
    const Code code = code_at(row, at);

    if (code == token::line_end_token || code == token::left_brace_token ||
        code == token::right_brace_token) {
        ++at;
        return success;
    }

    if (code == word::code_of(1, 15)) {                  // satellite.return
        at = past_the_statement(row, at);
        return success;
    }

    // satellite.statement.if -- the same shape as while below, judged the same way.
    if (code == word::code_of(1, 13, 1)) {
        const std::size_t stop = past_the_statement(row, at);
        const signed long long int held =
            names_in_statement(row, at + 1, stop, declared, capsules, functions, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.if has no body";
            at = stop;
            return satl_line_not_understood;
        }
        at = brace;                 // left ON the brace, as while is: the loop counts it
        return success;
    }

    // satellite.statement.else, which has no condition of its own. A REAL ONE
    // FOLLOWS AN if's `}` -- looking back one code refuses one standing alone here,
    // where nothing has run yet, instead of at the moment the walker reaches it.
    // (A `}` that closed a while's body slips through this and is refused when it
    // runs; both say the same sentence.)
    if (code == word::code_of(1, 13, 4)) {
        std::size_t back = at;
        while (back > 0 && code_at(row, back - 1) == token::line_end_token) --back;
        if (back == 0 || code_at(row, back - 1) != token::right_brace_token) {
            why = "satellite.statement.else with no satellite.statement.if before it";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t after_else = brace_after(row, at + 1);
        if (code_at(row, after_else) == word::code_of(1, 13, 1)) {
            at = after_else;        // else written onto another if: that if is the statement
            return success;
        }
        if (code_at(row, after_else) != token::left_brace_token) {
            why = "satellite.statement.else has no body";
            at = at + 1;
            return satl_line_not_understood;
        }
        at = after_else;
        return success;
    }

    if (code == word::code_of(1, 13, 3)) {               // satellite.statement.while
        const std::size_t stop = past_the_statement(row, at);
        const signed long long int held =
            names_in_statement(row, at + 1, stop, declared, capsules, functions, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.while has no body";
            at = stop;
            return satl_line_not_understood;
        }
        // Left ON the brace, not past it: check_program's own loop counts braces,
        // and stepping over this one would make the body's `}` read as the
        // capsule's and end the check early.
        at = brace;
        return success;
    }

    // satellite.statement.for -- the only statement with three parts, and the
    // only one that DECLARES in its own header (MILESTONES M20.A). Its shape is
    // knowable without running and every piece of it is judged here: the two
    // semicolons, a satellite.variable.number with a name and a value, a
    // condition that is not empty, and a body.
    if (code == word::code_of(1, 13, 2)) {
        const std::size_t stop = past_the_statement(row, at);
        const ForHeader parts = for_header(row, at);
        if (!parts.ok) {
            why = "satellite.statement.for is written (satellite.variable.number <name> = <value>; "
                  "<condition>; <step>), with both semicolons";
            at = stop;
            return satl_line_not_understood;
        }
        // "you must declare a number here" (the author, M20.A). Not a string and
        // not a name already declared elsewhere: the header owns this one.
        std::size_t k = parts.declaration;
        if (code_at(row, k) != word::code_of(1, 6, 4) || code_at(row, k + 1) != token::name_token) {
            why = "satellite.statement.for begins with satellite.variable.number <name> = <value>";
            at = stop;
            return satl_line_not_understood;
        }
        ++k;
        const std::string name = text_at(row, k);
        if (code_at(row, k) != token::assign_token) {
            why = "satellite.statement.for's " + name + " needs a value: satellite.variable.number " + name + " = 0";
            at = stop;
            return satl_line_not_understood;
        }
        // THE VALUE IS READ BEFORE THE NAME IS DECLARED, so `for(number i = i; ...)`
        // is the same "no satellite.variable line" it would be anywhere else.
        signed long long int held =
            names_in_statement(row, k, parts.condition - 1, declared, capsules, functions, why);
        if (held != success) { at = stop; return held; }
        if (!declared.emplace(name, word::code_of(1, 6, 4)).second) {
            why = name + " is declared twice in the same capsule";
            at = stop;
            return name_declared_twice;
        }
        // THE STEP IS OPTIONAL AND THE CONDITION IS NOT: M20.A gives the middle
        // part no choice ("a place to declare a condition that evaluates to true
        // or to false") and marks only the third "optionally".
        if (parts.condition == parts.step - 1) {
            why = "satellite.statement.for has nothing between its semicolons, and it needs a condition there";
            at = stop;
            return satl_line_not_understood;
        }
        held = names_in_statement(row, parts.condition, parts.closing, declared, capsules, functions, why);
        if (held != success) { at = stop; return held; }
        // THE STEP'S SHAPE, which is `i++`, `i--` or a math operation and nothing
        // else. The move itself is a run-time fact; which of the three it is, is
        // not, so it is refused here rather than after the loop's first turn has
        // printed (program_walk.cpp, for_step_moves_by).
        int moves_by = 0;
        held = for_step_moves_by(row, parts, name, moves_by, why);
        if (held != success) { at = stop; return held; }
        const std::size_t brace = brace_after(row, stop);
        if (code_at(row, brace) != token::left_brace_token) {
            why = "satellite.statement.for has no body";
            at = stop;
            return satl_line_not_understood;
        }
        ending.push_back({past_matching_brace(row, brace), name});
        at = brace;                 // ON the brace, as if and while are
        return success;
    }

    // A DECLARATION WITH TYPES BETWEEN < AND > -- read here so the checker
    // refuses a malformed one BEFORE the program prints anything, which is the
    // whole point of the checker. read_type_shape is the walker's own parser, so
    // the two cannot come to disagree about what `<a, b>` means.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::less_than_token) {
        const std::size_t stop = past_the_statement(row, at);
        std::size_t k = at;
        TypeShape shape;
        unsigned int pending = 0;
        if (!read_type_shape(row, k, shape, pending, why) || pending != 0) {
            if (pending != 0)
                why = "there is a > here with nothing left for it to close";
            at = stop;
            return satl_line_not_understood;
        }
        if (code_at(row, k) != token::name_token) {
            why = std::string(word::spelling_of(code)) +
                  "<...> declares a name, and there is no name after the >";
            at = stop;
            return satl_line_not_understood;
        }
        const std::string name = text_at(row, k);
        if (!declared.emplace(name, code).second) {
            why = name + " is declared twice in the same capsule";
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        const signed long long int held = names_in_statement(row, k, stop, declared, capsules, functions, why);
        at = stop;
        return held;
    }

    // A DECLARATION IS A WORD FOLLOWED BY A NAME. Only the type that is finished
    // is accepted: satellite.variable.number is built and checked against Python
    // across 477,253 cases, while satellite.variable.string's 23 method
    // libraries still run on the old 32-bit string (PROGRESS §5). Saying so is
    // the honest refusal; accepting it would be a declaration that does nothing.
    if (word::is_word_code(code) && code_at(row, at + 1) == token::name_token) {
        std::size_t k = at + 1;
        const std::string name = text_at(row, k);
        const std::size_t stop = past_the_statement(row, at);
        // satellite.variable.number (1 6 4) and satellite.variable.string (1 6 1):
        // both types the object model carries as arms and both checked against
        // Python. The string joined the list on 2026-09-16, when satelliteObject
        // made satellite_string the interpreter's own string -- before that a
        // declaration of one would have been a declaration that did nothing.
        // satellite.variable.binary (1 6 5) joined the same day, as the arm
        // satellite_binary_number.
        // satellite.variable.file (1 6 2) and satellite.variable.bool (1 6 6) joined on
        // 2026-09-18 with the file type (SATELLITE_FILE_OPERATIONS FO-1, FO-2): most
        // of a file's words answer true or false, and a program has to keep them.
        // WHICH WORDS DECLARE A TYPE IS type_shape.hpp's LIST, and asking it is
        // the point rather than the tidiness. This was a chain of `code != this
        // && code != that`, and adding satellite.container.index to the language
        // did not add it here -- so `satellite.container.index s` was refused as
        // "not built yet" while the very same declaration WITH <> worked, which
        // is the sort of contradiction a hand-kept second list always grows.
        if (!is_a_type_word(code)) {
            why = std::string(word::spelling_of(code)) + " " + name +
                  " is a declaration, and only satellite.variable.number, .string, .binary, "
                  ".percentage, .file, .bool and satellite.container.list, .index and .multiple "
                  "are built yet";
            at = stop;
            return satl_line_not_understood;
        }
        if (!declared.emplace(name, code).second) {
            why = name + " is declared twice in the same capsule";
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        if (code == word::code_of(1, 6, 5) && code_at(row, k) == token::assign_token) {
            const signed long long int written = binary_is_written_with_b(row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (code == word::code_of(1, 6, 16) && code_at(row, k) == token::assign_token) {
            const signed long long int written = percentage_is_written_with_percent(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        const signed long long int held = names_in_statement(row, k, stop, declared, capsules, functions, why);
        at = stop;
        return held;
    }

    if (word::is_word_code(code)) {
        // A WORD FOLLOWED BY `=` IS A SETTING BEING WRITTEN -- the third shape a
        // statement can start with, checked here so the walker's arm for it is
        // ever reached. The checker runs first and refuses what it has no shape
        // for, so a shape added to program_walk.cpp and not to this file is a
        // shape no program can get to.
        //
        // THE LIBRARY DECIDES WHETHER THE WORD IS A SETTING, the same way
        // expression.cpp decides it: `flag_setting` filled in. A word with `=`
        // after it and no setting library falls through to the refusal below and
        // is told it is not a call -- which is true, and is what it was told
        // before settings existed.
        if (code_at(row, at + 1) == token::assign_token) {
            const NumberRow *library = functions[code];
            if (library != nullptr && library->scenarios.flag_setting != nullptr) {
                const std::size_t stop = past_the_statement(row, at);
                const signed long long int held =
                    names_in_statement(row, at, stop, declared, capsules, functions, why);
                at = stop;
                return held;
            }
        }

        // A WORD NOT FOLLOWED BY ( IS NOT A CALL, and with a name after it, it
        // was a declaration above. Anything else has no shape yet.
        if (code_at(row, at + 1) != token::left_parenthesis_token) {
            why = std::string(word::spelling_of(code)) + " is not a call, and there is no scenario for it yet";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t stop = past_the_statement(row, at);
        // A WORD'S CALL MAY BE CALLED ON (`satellite.file.open("t.se").append("x")`)
        // and must then reach the line's end, as a name's method call must.
        std::size_t close = at + 1, count = 0;
        if (brackets_at(row, at + 1, close, count)) {
            const signed long long int shaped =
                a_call_to_its_end(row, close + 1, std::string(word::spelling_of(code)) + "(...)", why);
            if (shaped != success) { at = stop; return shaped; }
        }
        const signed long long int held = names_in_statement(row, at, stop, declared, capsules, functions, why);
        at = stop;
        return held;
    }

    if (code == token::name_token) {
        std::size_t k = at;
        const std::string name = text_at(row, k);
        const std::size_t stop = past_the_statement(row, at);
        // `name(` is a capsule call; `name =` is an assignment to a declared name.
        const DeclaredNames::const_iterator found = declared.find(name);
        if (code_at(row, k) != token::left_parenthesis_token && found == declared.end()) {
            why = name + " has no satellite.variable line declaring it";
            at = stop;
            return name_not_declared;
        }
        // A METHOD CALL IS A STATEMENT OF ITS OWN (SATELLITE_FILE_OPERATIONS FO-1):
        // `my_file.append("line_1")` does its work and its answer is not kept. So is
        // one on a line read by number, `my_file[2].find("x")`. Everything else a
        // name can start is `=` or a capsule's `(`.
        const bool a_method_call = code_at(row, k) == token::method_token ||
                                   code_at(row, k) == token::left_square_bracket_token;
        if (code_at(row, k) != token::left_parenthesis_token && !a_method_call) {
            const signed long long int shaped = after_the_name(row, k, name, false, why);
            if (shaped != success) { at = stop; return shaped; }
        }
        if (a_method_call && found != declared.end()) {
            const signed long long int shaped = a_call_to_its_end(row, k, name, why);
            if (shaped != success) { at = stop; return shaped; }
        }
        // The b is required on every value a binary is GIVEN, not only the first.
        if (found != declared.end() && found->second == word::code_of(1, 6, 5) &&
            code_at(row, k) == token::assign_token) {
            const signed long long int written = binary_is_written_with_b(row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (found != declared.end() && found->second == word::code_of(1, 6, 16) &&
            code_at(row, k) == token::assign_token) {
            const signed long long int written = percentage_is_written_with_percent(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        const signed long long int held = names_in_statement(row, at, stop, declared, capsules, functions, why);
        at = stop;
        return held;
    }

    // ONE CODE, NOT THE LINE: run_statements steps over only this code (its payload
    // with it) and reads the rest of the line as a statement, so the check judges that
    // same statement. Skipping the line let a stray character before a declaration hide
    // it -- a no-break space pasted as indentation made a declared n "not declared" --
    // and let `undeclared = 5` after one run past the check (the payload sweep,
    // 2026-09-17). A comment line still passes: its token steps to the line's end.
    if (token::carries_a_count(code)) { text_at(row, at); return success; }
    ++at;
    return success;
}

} // namespace

signed long long int check_typed_line(const BytecodeRegistry &registry,
                                      const FunctionTable &functions,
                                      MachineState &state)
{
    static const CapsuleTable none;
    DeclaredNames declared;
    EndingNames ending;
    const std::vector<std::bitset<16>> &row = registry.front();
    for (std::size_t at = 0; at < row.size() && code_at(row, at) != token::end_of_file_token; ) {
        const std::size_t was = at;
        std::string why;
        const signed long long int stopped = check_statement(row, at, none, functions, declared, ending, why);
        if (stops_the_program(stopped))
            return report_error("satl(prompt): " + why, stopped);
        if (at <= was)                  // a statement must always move forward
            ++at;
    }
    (void)state;
    return success;
}

signed long long int check_program(const BytecodeRegistry &registry,
                                   const CapsuleTable &capsules,
                                   const FunctionTable &functions,
                                   MachineState &state)
{
    for (const std::pair<const std::string, CapsuleSite> &entry : capsules) {
        const std::vector<std::bitset<16>> &row = registry[entry.second.row];
        // One set a capsule: there are no globals, so a name declared elsewhere
        // is not declared here.
        DeclaredNames declared;
        EndingNames ending;
        std::size_t depth = 0;
        for (std::size_t at = entry.second.body; at < row.size(); ) {
            const Code code = code_at(row, at);
            if (code == token::right_brace_token) {
                if (depth == 0) break;      // the capsule's own closing brace
                --depth;
                ++at;
                continue;
            }
            if (code == token::left_brace_token) { ++depth; ++at; continue; }
            const std::size_t was = at;
            std::string why;
            const signed long long int code_of_line =
                check_statement(row, at, capsules, functions, declared, ending, why);
            if (stops_the_program(code_of_line))
                // THE STATEMENT'S OWN START, AND NOT WHERE `at` ENDED UP. A
                // refusal leaves `at` wherever check_statement stopped reading,
                // which is not reliably the thing that was wrong -- so the caret
                // goes under the start of the statement, which always is. The
                // LINE is exact either way, and that is what a person looks for
                // first.
                return raise_at(code_of_line, why, entry.first, state, row, was, "satl(check)");
            if (at <= was)                  // a statement must always move forward
                ++at;
        }
    }
    state.set("program(checked): " + std::to_string(capsules.size()) + " capsules", success);
    return success;
}

} // namespace satellite004
