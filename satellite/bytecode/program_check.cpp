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

#include "suit_run.hpp"
#include "file_calls.hpp"
#include "color_values.hpp"
#include "console_calls.hpp"
#include "container_calls.hpp"
#include "main_arguments.hpp"
#include "float_values.hpp"
#include "fraction_values.hpp"
#include "hexadecimal_values.hpp"
#include "infinity_calls.hpp"
#include "library_values.hpp"
#include "window_calls.hpp"
#include "thread_calls.hpp"
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

// AND EACH NAME THAT HOLDS AN OBJECT, WITH THE SPACESUIT IT IS AN OBJECT OF (2026-09-22).
// Beside DeclaredNames rather than in it, because five other files are handed that map
// and none of them has any business with spacesuits.
using DeclaredObjects = std::unordered_map<std::string, std::size_t>;

inline constexpr std::size_t kNowhere = static_cast<std::size_t>(-1);

// WHERE A STATEMENT STANDS: the scope table and the scope, the capsule whose body it is,
// and what that body has declared that holds an object. One of these a body, handed to
// every judgement of it -- it replaced the three separate arguments every one of them took.
struct Where {
    Where(const BytecodeRegistry &its_registry, const CapsuleTable &its_capsules, std::size_t its_scope,
          const FunctionTable &its_functions)
        : registry(its_registry), capsules(its_capsules), scope(its_scope), functions(its_functions) {}

    const BytecodeRegistry &registry;
    const CapsuleTable &capsules;
    std::size_t scope;
    const FunctionTable &functions;
    const CapsuleSite *site = nullptr;   // the capsule whose body this is; null for a field, and at the prompt
    bool field = false;                  // a spacesuit's field: its value is worked out before there is an object
    DeclaredObjects objects;
    // AND EACH LIST DECLARED TO HOLD A SPACESUIT'S OBJECTS, with that spacesuit -- so
    // `units[i].call_x()` is judged by what the list was declared to hold.
    DeclaredObjects lists;
    std::size_t statement = kNowhere;    // where the statement being judged starts
    const Arguments *arguments = nullptr;  // the rows satl holds, which `argz.row = x` may not write
};

// A NAME THAT HOLDS OBJECTS, OR A LIST OF THEM, FROM ITS DECLARED SHAPE -- kept or
// forgotten, so a name declared again as something else is not judged as the old one.
void remember_shape(Where &where, const std::string &name, const TypeShape &shape)
{
    if (shape.is_a_suit())
        where.objects[name] = shape.suit;
    else
        where.objects.erase(name);
    if (shape.word == word::code_of(1, 4, 2) && shape.parameters.size() == 1 && shape.parameters[0].is_a_suit())
        where.lists[name] = shape.parameters[0].suit;
    else
        where.lists.erase(name);
}

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

// THE FOUR TYPES OF 2026-09-22 JUDGE THEIR OWN VALUES AND THEIR OWN METHODS, each
// in bytecode/<name>_values.cpp, so the four could be built side by side without
// every one of them editing this file. `type` is the declared word; any other
// word is none of theirs.
bool one_of_the_four(Code type)
{
    return type == word::code_of(1, 6, 10) || type == word::code_of(1, 6, 11) ||
           type == word::code_of(1, 6, 19) || type == word::code_of(1, 6, 20);
}

signed long long int written_right_for(Code type, const std::vector<std::bitset<16>> &row, std::size_t at,
                                       const DeclaredNames &declared, std::string &why)
{
    if (type == word::code_of(1, 6, 10)) return float_is_written_right(row, at, declared, why);
    if (type == word::code_of(1, 6, 11)) return hexadecimal_is_written_right(row, at, declared, why);
    if (type == word::code_of(1, 6, 19)) return color_is_written_right(row, at, declared, why);
    if (type == word::code_of(1, 6, 20)) return fraction_is_written_right(row, at, declared, why);
    return success;
}

signed long long int method_right_for(Code type, Code method, const std::string &spelling, std::string &why)
{
    if (type == word::code_of(1, 6, 10)) return float_method_check(method, spelling, why);
    if (type == word::code_of(1, 6, 11)) return hexadecimal_method_check(method, spelling, why);
    if (type == word::code_of(1, 6, 19)) return color_method_check(method, spelling, why);
    return fraction_method_check(method, spelling, why);
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
                             declared_as == word::code_of(1, 4, 6) ||
                             declared_as == word::code_of(1, 6, 21);   // the arguments: an index

    if (method == token::reverse_token)
        return success;                  // every type with an order has one
    if (one_of_the_four(declared_as))
        return method_right_for(declared_as, method, spelling, why);
    // AN INFINITY'S OWN METHODS, numbered at INF-1 and built from INF-4, each named
    // with the milestone that builds it -- before anything runs.
    if (declared_as == word::code_of(1, 6, 17)) {
        const std::string missing = infinity_method_not_built(method);
        if (!missing.empty()) {
            why = spelling + " " + missing;
            return not_built_yet;
        }
    }
    // A THREAD'S OWN METHODS (2026-09-23), asked of thread_calls.hpp: start, stop, join and
    // wait, each taking nothing. Before the containers: a list has a .join of its own.
    if (declared_as == word::code_of(1, 6, 13)) {
        if (thread_method_arity(method) < 0) {
            why = spelling + " -- " + thread_methods_are();
            return types_do_not_meet;
        }
        std::size_t close = k + 2, given = 0;
        if (code_at(row, k + 2) != token::left_parenthesis_token || !brackets_at(row, k + 2, close, given) ||
            given != 0) {
            why = spelling + "() takes nothing, in its brackets";
            return satl_line_not_understood;
        }
        return success;
    }
    if (a_container) {
        if (of_a_container) {
            // HOW MANY IT WAS GIVEN, judged here as a file's and a window's are (the
            // review, 2026-09-23): `a.reserve()` and `a.sum(1)` printed whatever came
            // before them and then stopped. Brackets are needed only by a method that
            // takes something -- `a.size` and `a.size()` are one read.
            const int wanted = container_arity(method);
            const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
            std::size_t close = k + 2, given = 0;
            if (bracketed && !brackets_at(row, k + 2, close, given)) {
                why = spelling + "( is never closed on its line";
                return satl_line_not_understood;
            }
            if (given != static_cast<std::size_t>(wanted) || (wanted > 0 && !bracketed)) {
                why = spelling + " takes " + std::to_string(wanted) + (wanted == 1 ? " argument" : " arguments") +
                      (bracketed ? ", and was given " + std::to_string(given) : ", in brackets after it");
                return satl_line_not_understood;
            }
            // A NAME DECLARED AN INDEX is told which half to ask, before the run, in
            // the walker's own sentence (container_calls.hpp).
            if (declared_as == word::code_of(1, 4, 5)) {
                const std::string refused = index_refuses(method, name);
                if (!refused.empty()) {
                    why = spelling + " -- " + refused;
                    return types_do_not_meet;
                }
            }
            return success;
        }
        if (of_a_string_or_number) return success;
        why = spelling + " is not built for " + word::spelling_of(declared_as) +
              " yet -- a container has .append, .size, .contains, .sum, .max, .min, .join, .reserve, "
              ".sort().by_name(), .sort().by_value() and .reverse()";
        return not_built_yet;
    }

    // A WINDOW'S OWN METHODS, asked of window_calls.hpp and never copied here --
    // the hand-written container set that used to sit above went stale the same
    // afternoon it was written, and one list is the fix for that.
    if (declared_as == word::code_of(1, 6, 18)) {
        if (window_method_arity(method) < 0) {
            // ASKED OF window_calls.hpp AND NOT WRITTEN HERE, which is the same
            // rule the line above already follows for the arity: this file kept
            // its own copy of a method list once and it was stale by the
            // afternoon. A widget added to window_calls.cpp's table appears in
            // this sentence without anybody coming back here.
            why = spelling + " -- " + window_methods_are();
            return types_do_not_meet;
        }
        // HOW `.press` AND `.pressed` ARE SPELLED IS NOT CHECKED HERE, and the
        // reason is a defect this file had for a day (found by a fresh reader,
        // 2026-09-21). This function sees only the FIRST method of a chain on a
        // DECLARED name, so a rule written here holds for exactly one spelling:
        // `satellite.window.button("x").pressed("nosuch")` has no declared
        // receiver and `b.title("t").pressed("nosuch")` is not the first method,
        // and BOTH escaped -- drew a window, and failed at the moment somebody
        // pressed the button. That is precisely the refusal WIN-11 claims to
        // have moved earlier. It now lives in names_in_statement, which walks
        // the WHOLE statement, so every spelling passes through it.
        //
        // HOW MANY IT WAS GIVEN *IS* CHECKED HERE, and it is the RECEIVER's
        // business: `w.title("a", "b")` is a window being asked something a
        // window does not do, and the receiver is what says so. Added
        // 2026-09-21, because `.pressed` had this and its neighbours did not --
        // one method refused before the run and the rest at it, for no reason a
        // person could see.
        //
        // ONLY WITH BRACKETS. `w.title`, `b.pressed` and `w.ok` written bare are
        // a READ, not a call with no arguments, so a missing `(` is not a
        // missing argument -- and `.close` written bare is already told to put
        // its brackets on, in the sentence that says a window DOES it.
        const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
        std::size_t close = k + 2, given = 0;
        if (bracketed && !brackets_at(row, k + 2, close, given)) {
            why = spelling + "( is never closed on its line";
            return satl_line_not_understood;
        }
        // `.press` AND `.pressed` SAY IT BETTER THEMSELVES, wherever they are
        // written, so they are not counted twice and given the duller sentence.
        // `.append` HAS TWO RIGHT COUNTS AND THE CHECKER LETS BOTH THROUGH
        // (GTK-7). It cannot do better: which one is right depends on what the
        // receiver turned out to BE, and a satellite.variable.window name may
        // hold a window or a row. window_append() names the wrong one at the
        // moment it knows, with the piece it actually got.
        if (bracketed && method != token::press_token && !window_method_takes_a_capsule_name(method) &&
            given != static_cast<std::size_t>(window_method_arity(method)) &&
            static_cast<int>(given) != window_method_also_takes(method)) {
            why = spelling + " takes " + std::to_string(window_method_arity(method)) +
                  (window_method_arity(method) == 1 ? " argument, and was given " : " arguments, and was given ") +
                  std::to_string(given);
            return satl_line_not_understood;
        }
        return success;
    }

    // A STRING'S COLOUR (console_style.hpp): one colour, in brackets, and a literal that
    // cannot be one is refused now.
    if (declared_as == word::code_of(1, 6, 1) &&
        (method == token::foreground_token || method == token::background_token)) {
        std::size_t close = k + 2, given = 0;
        const bool bracketed = code_at(row, k + 2) == token::left_parenthesis_token;
        if (!bracketed || !brackets_at(row, k + 2, close, given) || given != 1) {
            why = spelling + " takes one colour, in brackets: " + spelling + "(xFF8800)";
            return satl_line_not_understood;
        }
        why = colour_literal_refused(row, k + 3, spelling);
        return why.empty() ? success : types_do_not_meet;
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
        // A METHOD, OR A NAME AFTER A DOT: an object's capsule (2026-09-22), judged by
        // names_in_statement against the spacesuit it belongs to.
        if (code_at(row, k) == token::method_token &&
            (token::is_method_code(code_at(row, k + 1)) || code_at(row, k + 1) == token::name_token)) {
            if (code_at(row, k + 1) == token::name_token) {
                ++k;
                skip_payload(row, k);
            } else {
                k += 2;
            }
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

// HOW MANY A CAPSULE CALL WAS GIVEN, before anything runs (2026-09-21). Until
// capsules took arguments there was nothing to count. `open` is the call's `(`, and
// `written` is the name as the call wrote it -- `greet`, or `other.greet`.
signed long long int given_what_it_takes(const std::vector<std::bitset<16>> &row, std::size_t open,
                                         const CapsuleSite &site, const std::string &written, std::string &why)
{
    std::size_t close = open, given = 0;
    if (!brackets_at(row, open, close, given)) {
        why = written + "( is never closed on its line";
        return satl_line_not_understood;
    }
    const std::size_t takes = site.parameters.size();
    if (given != takes) {
        why = written + " takes " + std::to_string(takes) +
              (takes == 1 ? " argument, and was given " : " arguments, and was given ") + std::to_string(given);
        return satl_line_not_understood;
    }
    return success;
}

// AND WHAT A PRESS CAN HAND IT (2026-09-21). A press has nobody to write its
// arguments -- the program said `.pressed(name)` and walked away -- so the
// capsule's own declaration is what says what it wants, and there are only three
// things a press has to give: nothing, the piece, and the window it is in.
// `method` is the one the program wrote -- .pressed, .changed, .every -- because
// being told about presses after writing `.changed` is being told about somebody
// else's line ("a press" was the truth until GTK-9).
signed long long int a_press_can_run(const CapsuleSite &site, const std::string &name, Code method, std::string &why)
{
    const std::vector<CapsuleParameter> &wants = site.parameters;
    if (wants.size() > 2) {
        why = name + " takes " + std::to_string(wants.size()) + " arguments, and ." +
              std::string(token::method_name_of(method)) + " has only two to give: write " + name + "(), " + name +
              "(satellite.variable.window the_piece), or " + name +
              "(satellite.variable.window the_piece, satellite.variable.window its_window)";
        return satl_line_not_understood;
    }
    for (const CapsuleParameter &takes : wants) {
        if (takes.declared() == word::code_of(1, 6, 18))
            continue;
        why = name + "'s " + takes.name + " is declared " + word::spelling_of(takes.declared()) + ", and ." +
              std::string(token::method_name_of(method)) +
              " hands it the piece it happened to and the window it happened in -- "
              "both are satellite.variable.window";
        return types_do_not_meet;
    }
    return success;
}

// A VARIABLE MAY NOT TAKE THE NAME OF A FILE OR A SPACE IT CAN SEE (2026-09-22). A
// variable called `other`, in a file that includes other.satl, would make
// `other.greet()` a method on the variable and hide the file -- so the name is
// refused where it is declared, which is the one line that can change. 003 refused
// the same collision the other way round (its S1602): a name is declared once.
signed long long int a_name_it_may_take(const CapsuleTable &capsules, std::size_t scope, const std::string &name,
                                        std::string &why)
{
    const std::string taken = capsules.already_names(scope, name);
    if (taken.empty())
        return success;
    why = name + " is already " + taken + ", so a variable cannot be named " + name + " -- " + name +
          ".something() could then mean either";
    return name_declared_twice;
}

// A NAME DECLARED A SECOND TIME, said for what it already is: a field of the object the
// capsule runs on, or another variable of the capsule.
std::string declared_twice(const Where &where, const std::string &name)
{
    if (where.site != nullptr && where.site->suit != kNoScope) {
        const CapsuleScope &suit = where.capsules.scopes[where.site->suit];
        if (suit.layout->slot_of(name) != kNoSlot)
            return name + " is a field of " + suit.layout->shown + ", and a name is declared once -- a variable or a "
                          "parameter of its capsule cannot be named " + name + " as well";
    }
    return name + " is declared twice in the same capsule";
}

// A SPACESUIT'S CAPSULE CALLED BY ITS BARE NAME runs on the object the calling capsule
// runs on -- so the line must stand in a capsule of THAT spacesuit (2026-09-22). Not in
// a field's value, which is worked out before there is an object, and not in a spacesuit
// declared inside it, whose capsules run on an object of their own spacesuit.
signed long long int a_capsule_with_an_object(const CapsuleSite &site, const Where &where,
                                              const std::string &written, std::string &why)
{
    if (site.suit == kNoScope)
        return success;
    if (where.field) {
        why = written + " is a capsule of the spacesuit " + where.capsules.scopes[site.suit].within + ", and a field's "
              "value is worked out before there is an object to run it on";
        return satl_line_not_understood;
    }
    // A SUPERTYPE'S CAPSULE runs on an object of a spacesuit that extends it just as well.
    const bool extends_it = where.site != nullptr && where.site->suit != kNoScope &&
                            where.capsules.scopes[where.site->suit].layout->is_a(site.suit);
    if (where.site == nullptr || (where.site->suit != site.suit && !extends_it)) {
        why = written + " is a capsule of the spacesuit " + where.capsules.scopes[site.suit].within + ", and it runs "
              "on an object of it -- call it on one: an_object." + site.name + "(...)";
        return satl_line_not_understood;
    }
    return success;
}

// IS `name` A CAPSULE OF ANY SPACESUIT -- asked of a name after a `.` whose object is an
// answer, known only running: `shards[t].call_live()`, `make().call_x()`.
bool some_suit_has(const CapsuleTable &capsules, const std::string &name)
{
    for (const CapsuleScope &scope : capsules.scopes)
        if (scope.is_a_suit() && scope.capsules.count(name) != 0)
            return true;
    return false;
}

// A CAPSULE CALLED WHERE ITS ANSWER IS USED (2026-09-22): given what it takes, reachable
// with an object when it is a spacesuit's, and able to hand a value back at all.
// `open` is the call's `(`.
signed long long int a_call_for_its_answer(const std::vector<std::bitset<16>> &row, std::size_t open,
                                           const CapsuleSite &site, const std::string &written, const Where &where,
                                           std::string &why)
{
    signed long long int held = given_what_it_takes(row, open, site, written, why);
    if (held != success)
        return held;
    held = a_capsule_with_an_object(site, where, written, why);
    if (held != success)
        return held;
    if (!hands_back_a_value(where.registry, site)) {
        why = written + "() is used where its answer would be, and it never hands one back -- no satellite.return(...) "
                        "in it has a value";
        return capsule_gave_no_answer;
    }
    return success;
}

// `obj.name(...)` ON A NAME THAT HOLDS AN OBJECT OF `suit` (2026-09-22). `k` is on the
// `.` and is left on the call's `(`, so the arguments are judged as the loop goes on.
// `receiver_at` is where `obj` stands: a call that IS the whole statement lets its
// answer go, and anywhere else the answer is used.
signed long long int member_of_an_object(const std::vector<std::bitset<16>> &row, std::size_t &k,
                                         std::size_t receiver_at, const std::string &receiver, std::size_t suit,
                                         const Where &where, std::string &why)
{
    std::size_t m = k + 1;
    std::string member;
    if (code_at(row, m) == token::name_token) {
        member = text_at(row, m);
    } else if (token::is_method_code(code_at(row, m))) {
        member = token::method_name_of(code_at(row, m));
        ++m;
    } else {
        why = receiver + " is followed by a . and nothing an object of " +
              where.capsules.scopes[suit].layout->shown + " has";
        return satl_line_not_understood;
    }
    const std::string written = receiver + "." + member;
    // THE AUTHOR'S LOCK (satellite_object/object_lock.hpp): every object has .lock() and
    // .unlock(), whatever its spacesuit declares, and each takes nothing.
    if (code_at(row, k + 1) == token::lock_token || code_at(row, k + 1) == token::unlock_token) {
        if (code_at(row, m) != token::left_parenthesis_token || code_at(row, m + 1) != token::right_parenthesis_token) {
            why = written + "() takes nothing, in its brackets";
            return satl_line_not_understood;
        }
        k = m;
        return success;
    }
    signed long long int code = success;
    const CapsuleSite *site = where.capsules.member(suit, member, where.scope, code, why);
    if (site == nullptr)
        return code;
    if (code_at(row, m) != token::left_parenthesis_token) {
        why = written + " is a capsule of " + where.capsules.scopes[suit].layout->shown +
              ", and a capsule is called with its brackets: " + written + "()";
        return satl_line_not_understood;
    }
    const signed long long int held = given_what_it_takes(row, m, *site, written, why);
    if (held != success)
        return held;
    std::size_t close = m, given = 0;
    brackets_at(row, m, close, given);
    const Code after = code_at(row, close + 1);
    const bool stands_alone = where.statement == receiver_at &&
                              (after == token::line_end_token || after == token::comment_token ||
                               after == token::end_of_file_token);
    if (!stands_alone && !hands_back_a_value(where.registry, *site)) {
        why = written + "() is used where its answer would be, and it never hands one back -- no satellite.return(...) "
                        "in it has a value";
        return capsule_gave_no_answer;
    }
    k = m;
    return success;
}

// Every name a statement USES as a value -- so a name with no declaration is
// caught before anything runs. A name followed by `(` is a capsule and is
// checked against the capsule table instead.
signed long long int names_in_statement(const std::vector<std::bitset<16>> &row,
                                        std::size_t from,
                                        std::size_t stop,
                                        const DeclaredNames &declared,
                                        const Where &where,
                                        std::string &why)
{
    const CapsuleTable &capsules = where.capsules;
    const std::size_t scope = where.scope;
    const FunctionTable &functions = where.functions;
    // THE TWO CODES THIS LOOP LAST VISITED, and NOT row[at - 1] and row[at - 2]
    // (WIN-11). `.pressed(when_pressed)` is recognised by what stands before the
    // name, and a raw index backwards can land INSIDE A PAYLOAD -- where a
    // string's characters are their own Unicode numbers and one of them may
    // equal a token's code exactly. That is not a hypothetical in this tree:
    // capsules_in() carries the same warning, for a string ending in U+1006 that
    // made the next body a second satellite.main. This loop already SKIPS every
    // payload, so the codes it visited are the only ones that are really tokens.
    Code one_back = 0, two_back = 0;
    std::vector<std::size_t> judged;     // members judged through a list of objects
    std::vector<std::size_t> option_names;   // `foreground` in foreground=..., judged by its word
    for (std::size_t at = from; at < stop && at < row.size(); ) {
        const Code code = code_at(row, at);
        const Code before_this = one_back, and_before_that = two_back;
        two_back = one_back;
        one_back = code;

        // HOW `.pressed(...)` IS SPELLED, WHEREVER IT STANDS AND WHATEVER IT IS
        // WRITTEN ON (WIN-11). A method is judged by its RECEIVER in
        // method_on_a_name, and that is the wrong place for this: a receiver
        // that is a word's answer has no declared name, and a chain's second
        // method is never reached. Here there is no receiver to be gated on --
        // this loop walks every statement and skips every payload, so a
        // `pressed_token` it visits is a real one, wherever it was written.
        if (window_method_takes_a_capsule_name(code) &&
            code_at(row, at + 1) == token::left_parenthesis_token) {
            const std::string spelled = std::string(".") + token::method_name_of(code);
            std::size_t argument = at + 2;
            // TEXT IS NOT A NAME. It would lex, check, run, open the window, and
            // fail at the moment of the press -- window already up.
            if (code_at(row, argument) != token::name_token) {
                why = spelled + " takes the NAME of a capsule, written as it is written: " + spelled +
                      "(when_pressed) -- not text and not a value worked out, because satl proves "
                      "the capsule is there before your program runs";
                return types_do_not_meet;
            }
            // AND ONE NAME, NOT A NAME AND THEN ANYTHING. Looking only at the
            // code after the `(` let `.pressed(when_pressed, 5)` through to be
            // refused at run time, which is a refusal this checker owes earlier.
            // ONE NAME MAY BE DOTTED (2026-09-22): `.pressed(other.go)` names the
            // capsule go in the file other.satl, and is still one name.
            std::vector<std::string> dotted;
            dotted_names_at(row, argument, dotted);
            // AND WHAT MAY FOLLOW IT IS THE METHOD'S BUSINESS, asked of
            // window_calls.hpp (GTK-13). `.pressed`, `.changed` and `.closed`
            // take one name and nothing else; `.every` takes the name and then
            // how often, so a comma is what it wants there.
            const bool more_may_follow = window_method_takes_more_after_the_name(code);
            const token::Code after = code_at(row, argument);
            if (!more_may_follow && after != token::right_parenthesis_token) {
                why = spelled + " takes one capsule's name and nothing else";
                return satl_line_not_understood;
            }
            if (more_may_follow && after != token::comma_token) {
                why = spelled + " takes a capsule's NAME first and then the rest: " +
                      (code == token::ask_token    ? spelled + "(when_answered, \"delete it?\")"
                       : code == token::item_token ? spelled + "(when_open, \"Open\")"
                                                   : spelled + "(when_it_ticks, 1000)");
                return satl_line_not_understood;
            }
        }
        // `.press()` IS THE OTHER ONE, AND THEY ARE ONE LETTER APART -- so the
        // refusal for either given the other's argument names the other out
        // loud, rather than leaving a person with "when_pressed has no
        // satellite.variable line declaring it": a true sentence about the wrong
        // half of a typo.
        if (code == token::press_token && code_at(row, at + 1) == token::left_parenthesis_token &&
            code_at(row, at + 2) != token::right_parenthesis_token) {
            why = ".press() is the PROGRAM pressing it, and a click has nothing to say, so it takes "
                  "nothing. To name what a press RUNS, that is .pressed(a_capsule)";
            return satl_line_not_understood;
        }

        // A satellite.library VALUE READ IN THE LINE (library_values.hpp): its names judged
        // here and stepped over, so a method after them is judged as the loop goes on.
        if (is_library_word(code) && code_at(row, at + 1) == token::method_token) {
            std::string written;
            Code type = 0;
            const signed long long int read = library_read_is_right(capsules, where.registry, row, at, written, type, why);
            if (read != success) return read;
            const signed long long int method = method_on_a_name(row, at, written, type, why);
            if (method != success) return method;
            continue;
        }

        if (code == token::name_token) {
            std::size_t k = at;
            const std::string name = text_at(row, k);

            // A NAMED OPTION'S NAME (console_calls.hpp) is the option and not a variable:
            // judged by name with its word, below, and stepped over here with its `=`, so
            // its value is judged as the loop goes on.
            bool an_option = false;
            for (const std::size_t each : option_names) an_option = an_option || each == at;
            if (an_option) {
                at = k + 1;
                continue;
            }
            // AND ONE NOBODY TAKES: `twice(x=2)` gives a capsule an option, and a capsule's
            // arguments are given in order. Said as that, not as a variable x nobody declared
            // -- and ONLY straight after a call's `(`, a name's or a method's: after a comma
            // it may be `satellite.variable.number a = 1, b = 2`, and after a bare `(` it may
            // be `while((i = 3) > 0)`, and both are judged as they always were (the review).
            if (before_this == token::left_parenthesis_token &&
                (and_before_that == token::name_token || token::is_method_code(and_before_that)) &&
                code_at(row, k) == token::assign_token) {
                why = name + "= is a named option, and only satellite.console.display and satellite.console.input "
                             "take them -- a capsule is given its arguments in order, without names";
                return satl_line_not_understood;
            }

            // A MEMBER ALREADY JUDGED, through the list its object came out of (below).
            bool already_judged = false;
            for (const std::size_t each : judged) already_judged = already_judged || each == at;
            if (already_judged && before_this == token::method_token) {
                at = k;
                continue;
            }

            // A CAPSULE'S NAME INSIDE A LINE is a call for its answer (2026-09-22) -- a
            // statement that is only a call is judged by check_statement, which hands this
            // function its arguments alone -- or, after `.pressed(`, the capsule a button
            // runs, written as a name and never called here.
            const bool names_what_a_press_runs =
                before_this == token::left_parenthesis_token && window_method_takes_a_capsule_name(and_before_that);
            const bool after_a_dot = before_this == token::method_token;
            std::vector<std::string> names;
            std::size_t past = at;
            dotted_names_at(row, past, names);
            std::string written = names.front();
            for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];

            // WHAT A PRESS RUNS IS ONE CAPSULE, AND THE WHOLE NAME MUST REACH IT --
            // `.pressed(go)`, `.pressed(other.go)`. Judging only the first name let
            // `.pressed(go.reverse)` through to fail at the press (the review).
            if (names_what_a_press_runs) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr) {
                    why = reached.through_a_scope || reached.why.rfind("no capsule named ", 0) != 0
                              ? reached.why
                              : "no capsule named " + written + " -- ." +
                                    std::string(token::method_name_of(and_before_that)) +
                                    "(...) names a capsule to run, and there is no satellite.capsule " + written +
                                    "() in this program";
                    return satl_line_not_understood;
                }
                // A SPACESUIT'S CAPSULE RUNS ON AN OBJECT, and a press has none to give it
                // (the review, 2026-09-22: it passed here and would have run with no object).
                if (reached.site->suit != kNoScope) {
                    why = written + " is a capsule of the spacesuit " + capsules.scopes[reached.site->suit].within +
                          ", and it runs on an object -- ." + std::string(token::method_name_of(and_before_that)) +
                          "(...) names a capsule of a file or a satellite.namespace, which a press runs with none";
                    return satl_line_not_understood;
                }
                const signed long long int fits = a_press_can_run(*reached.site, written, and_before_that, why);
                if (fits != success)
                    return fits;
                at = past;
                continue;
            }

            // A MEMBER OF AN OBJECT -- `log.call_open()`, `plan.call_threads()` -- judged by
            // the spacesuit the name holds an object of (2026-09-22). `k` comes back on the
            // call's `(`, so its arguments are judged as the loop goes on.
            if (!after_a_dot && code_at(row, k) == token::method_token && declared.count(name) != 0) {
                const DeclaredObjects::const_iterator object = where.objects.find(name);
                if (object != where.objects.end()) {
                    const signed long long int judged =
                        member_of_an_object(row, k, at, name, object->second, where, why);
                    if (judged != success) return judged;
                    at = k;
                    continue;
                }
                // `.call_x(` ON A NAME THAT HOLDS NO OBJECT: the name after its dot is not
                // one of its methods -- the lexer made no method code of it -- and only an
                // object has capsules to call.
                if (code_at(row, k + 1) == token::name_token &&
                    declared.find(name)->second != word::code_of(1, 6, 21)) {
                    std::size_t m = k + 1;
                    const std::string member = text_at(row, m);
                    why = name + " is " + word::spelling_of(declared.find(name)->second) + ", and " + member +
                          " is not one of its methods -- only an object of a satellite.spacesuit has capsules to call";
                    return satl_line_not_understood;
                }
            }

            // A CAPSULE CALLED INSIDE A LINE, FOR ITS ANSWER (2026-09-22) -- `other.greet(`,
            // `tools.x(`. Asked only of a name no variable has -- a variable is a method's
            // receiver -- and never of a name after a `.`. When the first name is neither a
            // file nor a space this says nothing, and the name is judged below as it always was.
            if (!after_a_dot && names.size() >= 2 && declared.find(name) == declared.end() &&
                code_at(row, past) == token::left_parenthesis_token) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site != nullptr) {
                    const signed long long int judged = a_call_for_its_answer(row, past, *reached.site, written, where, why);
                    if (judged != success) return judged;
                    at = past;
                    continue;
                }
                if (reached.through_a_scope) {
                    why = reached.why;
                    return reached.code;
                }
            }

            if (code_at(row, k) == token::left_parenthesis_token) {
                // AFTER A `.`, A NAME IS A MEMBER of what came before it. A declared name's
                // members were judged above; this one's object is an ANSWER --
                // `shards[t].call_live()`, `make().call_x()` -- whose spacesuit is known only
                // running, so here it must be a capsule of SOME spacesuit, and the walker
                // judges the rest. Anything else is told what it was always told.
                if (after_a_dot && some_suit_has(capsules, name)) {
                    at = k;
                    continue;
                }
                const Reached reached = capsules.reach(scope, {name});
                if (reached.site == nullptr) {
                    // A SPACESUIT'S NAME WITH BRACKETS AFTER IT: an object made inside a
                    // line, which neither 003 nor 004 has -- one is made by declaring it.
                    std::string unused;
                    if (!after_a_dot && capsules.suit_named(scope, {name}, unused) != kNoScope) {
                        why = name + " is a spacesuit, and an object of it is made by declaring one on a line of its "
                                     "own -- " + name + " a_name(...) -- and then using a_name";
                        return satl_line_not_understood;
                    }
                    why = after_a_dot ? "no capsule named " + name : reached.why;
                    return satl_line_not_understood;
                }
                if (after_a_dot) {
                    why = "." + name + "(...) is not a method -- " + name + " is a capsule, and a capsule is called "
                          "by its own name, as " + reached.site->shown + "()";
                    return satl_line_not_understood;
                }
                const signed long long int judged = a_call_for_its_answer(row, k, *reached.site, name, where, why);
                if (judged != success) return judged;
                at = k;
                continue;
            } else if (declared.find(name) == declared.end()) {
                // A FIELD'S VALUE NAMING ANOTHER FIELD: there is no object yet (003's S0511).
                const bool a_field = where.field && capsules.scopes[scope].layout != nullptr &&
                                     capsules.scopes[scope].layout->slot_of(name) != kNoSlot;
                why = a_field ? name + " is a field of " + capsules.scopes[scope].layout->shown +
                                    ", and a field's value is worked out before there is an object -- it cannot name "
                                    "another field; give it its value in the satellite.constructor instead"
                              : name + " has no satellite.variable line declaring it";
                return name_not_declared;
            } else if (declared.find(name)->second == word::code_of(1, 6, 21) &&
                       past_the_argument_names(row, k) != k) {
                // THE ARGUMENTS VARIABLE'S ROWS ARE NAMES, NOT VARIABLES:
                // `arguments.memory.total` is one row (main_arguments.hpp), and which
                // rows there are is the machine's to say, so the walker says it.
                k = past_the_argument_names(row, k);
            } else if (where.lists.count(name) != 0 && code_at(row, k) == token::left_square_bracket_token) {
                // `units[i].call_x()` -- AN ITEM'S MEMBER, judged by the spacesuit the list was
                // declared to hold (the review, 2026-09-22: it was judged only running). The
                // index's own names are judged as the loop goes on; the member's name, when
                // the loop reaches it, is skipped as judged.
                std::size_t close = k, count = 0;
                if (brackets_at(row, k, close, count) && code_at(row, close + 1) == token::method_token &&
                    (code_at(row, close + 2) == token::name_token || token::is_method_code(code_at(row, close + 2)))) {
                    std::size_t dot = close + 1;
                    const signed long long int member =
                        member_of_an_object(row, dot, at, name + "[...]", where.lists.at(name), where, why);
                    if (member != success) return member;
                    judged.push_back(close + 2);
                }
            } else if (where.objects.count(name) == 0) {
                const signed long long int judged_here = method_on_a_name(row, k, name, declared.find(name)->second, why);
                if (judged_here != success) return judged_here;
            }
            at = k;
            continue;
        }

        // satellite.thread.new(capsule(args)) (2026-09-23, thread_calls.hpp): what is inside
        // is a CALL to one of the program's own capsules, KEPT and not run -- so it is judged
        // as a call (it reaches a capsule; it is given what it takes) and never as a call for
        // its answer: a capsule that hands nothing back is what a thread most often runs.
        // Its arguments are judged as the loop goes on, from the call's `(`.
        if (is_thread_word(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            std::size_t past = at + 2;
            if (code_at(row, past) != token::name_token) {
                why = "satellite.thread.new runs a capsule of your own on a thread, so what goes inside it is a "
                      "call: my_capsule() or my_capsule(x)";
                return thread_needs_a_capsule_call;
            }
            std::vector<std::string> names;
            dotted_names_at(row, past, names);
            std::string written = names.front();
            for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];
            if (code_at(row, past) != token::left_parenthesis_token) {
                why = "satellite.thread.new(" + written + ") names a capsule and does not call it -- write " +
                      written + "(), with what it takes inside the brackets";
                return thread_needs_a_capsule_call;
            }
            // `obj.call_x(...)` -- A CAPSULE OF AN OBJECT, run on a thread on that object (T2:
            // shared, and made safe by the object's own .lock()).
            const CapsuleSite *site = nullptr;
            const DeclaredObjects::const_iterator object =
                names.size() == 2 ? where.objects.find(names.front()) : where.objects.end();
            if (object != where.objects.end()) {
                signed long long int refused = success;
                site = capsules.member(object->second, names.back(), scope, refused, why);
                if (site == nullptr)
                    return refused;
            } else {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr) {
                    why = reached.why;
                    return satl_line_not_understood;
                }
                site = reached.site;
                // A SPACESUIT'S CAPSULE BY ITS BARE NAME runs on this body's object, so the
                // line must stand in a capsule of that spacesuit, as a call must.
                const signed long long int with_an_object = a_capsule_with_an_object(*site, where, written, why);
                if (with_an_object != success)
                    return with_an_object;
            }
            const signed long long int given = given_what_it_takes(row, past, *site, written, why);
            if (given != success)
                return given;
            std::size_t close = past, count = 0;
            brackets_at(row, past, close, count);
            if (code_at(row, close + 1) != token::right_parenthesis_token) {
                why = "satellite.thread.new takes one capsule call and nothing after it";
                return satl_line_not_understood;
            }
            at = past;
            continue;
        }

        // satellite.console.width AND .height ARE READ, NOT CALLED (console_calls.hpp),
        // so `width()` is told that rather than that it has no library.
        if (is_console_fact(code) && code_at(row, at + 1) == token::left_parenthesis_token) {
            why = std::string(word::spelling_of(code)) + " is read with no brackets: " + word::spelling_of(code);
            return satl_line_not_understood;
        }

        // A WORD USED AS A CALL MUST HAVE A LIBRARY. A word with none is
        // not_built_yet (14) with its own name, which is what 003 did and what a
        // person can act on (function_table.hpp).
        // satellite.file's words, satellite.infinity(), satellite.window's and
        // satellite.container.list() are the object model's and have none
        // (file_calls.hpp, infinity_calls.hpp, window_calls.hpp, container_calls.hpp)
        // -- each of them answers a HANDLE, which is the one thing a library cannot
        // make.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token &&
            functions[code] == nullptr && !is_file_word(code) && !is_infinity_word(code) &&
            !is_window_word(code) && !is_container_word(code) && !is_console_word(code)) {
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
                const std::string spelled_word(word::spelling_of(code));
                // NAMED OPTIONS, `foreground=xFF8800` (console_calls.hpp): judged by name
                // and by literal here, never counted as arguments, and their names
                // stepped over as options rather than judged as variables nobody declared.
                // Not in satellite.statement's own brackets, where `(d = 0; ...)` starts a loop.
                if (spelled_word.rfind("satellite.statement.", 0) != 0) {
                    std::vector<WrittenOption> options;
                    if (!options_written_in(row, at + 1, options, why)) {
                        why = spelled_word.substr(0, spelled_word.find('(')) + " -- " + why;
                        return satl_line_not_understood;
                    }
                    for (std::size_t n = 0; n < options.size(); ++n) {
                        const signed long long int refused = option_refused(code, row, options, n, why);
                        if (refused != success) return refused;
                        option_names.push_back(options[n].name_at);
                    }
                    given -= options.size();
                }
                // A COLOUR WORD GIVEN A LITERAL THAT CANNOT BE A COLOUR, said now.
                if (a_colour_word_given_one(code) && given == 1) {
                    const std::string refused =
                        colour_literal_refused(row, at + 2, spelled_word.substr(0, spelled_word.find('(')));
                    if (!refused.empty()) {
                        why = refused;
                        return types_do_not_meet;
                    }
                }
                // satellite.infinity(x) is infinity ** x, and INF-5 builds it.
                const std::string not_yet = infinity_word_not_built(code, given);
                if (!not_yet.empty()) {
                    why = not_yet;
                    return not_built_yet;
                }
                // satellite.container.list() takes nothing: a list that holds
                // something is written with braces.
                const std::string no_arguments = container_word_refused(code, given);
                if (!no_arguments.empty()) {
                    why = no_arguments;
                    return satl_line_not_understood;
                }
                if (is_file_word(code) && given != file_word_arity(code)) {
                    why = file_word_takes(code) + ", and was given " + std::to_string(given) + " arguments";
                    return satl_line_not_understood;
                }
                // A WINDOW WORD TAKES ITS OWN COUNT TOO -- three for new, one for
                // button -- so it is refused here rather than by the one-argument
                // rule below, which would tell a person the wrong thing.
                if (is_window_word(code) && given != window_word_arity(code)) {
                    // "1 argument", NOT "1 arguments". A switch is the first
                    // window word that takes NOTHING, so it is the first one a
                    // person can get wrong by exactly one -- and the sentence
                    // that tells them so reading like a machine wrote it is a
                    // small thing that is entirely avoidable.
                    why = window_word_takes(code) + ", and was given " + std::to_string(given) +
                          (given == 1 ? " argument" : " arguments");
                    return satl_line_not_understood;
                }
                if (!is_file_word(code) && !is_window_word(code)) {
                    const std::string spelled(word::spelling_of(code));
                    // A WORD THAT TAKES ONE, GIVEN NONE (GTK-12). Since the lexer
                    // stopped answering nothing for empty brackets, `display()`
                    // lexes as display(text) with 0 arguments; it is refused
                    // here by name rather than at run time by a value that is
                    // not there. A word whose row IS `path()` takes none and is
                    // not this.
                    const bool takes_one = spelled.find('(') != std::string::npos &&
                                           spelled.find("()") == std::string::npos;
                    if (given > 1 || (given == 0 && takes_one)) {
                        why = spelled.substr(0, spelled.find('(')) + " takes one argument, and was given " +
                              std::to_string(given);
                        return satl_line_not_understood;
                    }
                    // A WORD WHOSE ONLY ROW IS `path()`, GIVEN SOMETHING: the lexer hands
                    // it that row now (bytecode_registry.cpp), so `satellite.console.home(5)`
                    // is told it takes nothing, not that satellite.console is not a call.
                    if (spelled.size() > 2 && spelled.compare(spelled.size() - 2, 2, "()") == 0 && given > 0) {
                        why = spelled + " takes nothing, and was given " + std::to_string(given) +
                              (given == 1 ? " argument" : " arguments");
                        return satl_line_not_understood;
                    }
                }
            }
        }

        // A CONTAINER'S METHOD ON A LITERAL (the review, 2026-09-23): `", ".join({"a",
        // "b"})` -- Python's spelling -- and `3.max(5)`. A literal has no declaration
        // for method_on_a_name to judge it by, and until join and max were method
        // names this was caught by accident, as a call to a capsule nobody wrote;
        // after, only once the line ran, with output already printed. The literal's
        // kind is known right here, so it is refused before anything runs, in the
        // walker's own words. `.reverse()` is every ordered type's and is left alone.
        if (code == token::string_token || code == token::number_token || code == token::binary_token ||
            code == token::hexadecimal_token || code == token::percentage_token) {
            std::size_t past = at;
            text_at(row, past);
            const Code method = code_at(row, past + 1);
            if (code_at(row, past) == token::method_token &&
                (method == token::foreground_token || method == token::background_token)) {
                const std::string spelled = std::string("that ") + (code == token::string_token ? "string" : "literal") +
                                            "." + method_spelling(method);
                if (code != token::string_token) {
                    why = spelled + " -- a colour is given to text: \"...\"." + method_spelling(method) + "(xFF8800)";
                    return types_do_not_meet;
                }
                std::size_t close = past + 2, given = 0;
                if (code_at(row, past + 2) != token::left_parenthesis_token ||
                    !brackets_at(row, past + 2, close, given) || given != 1) {
                    why = spelled + " takes one colour, in brackets";
                    return satl_line_not_understood;
                }
                why = colour_literal_refused(row, past + 3, spelled);
                if (!why.empty()) return types_do_not_meet;
            }
            if (code_at(row, past) == token::method_token && container_arity(method) >= 0 &&
                method != token::reverse_token) {
                const char *kind = code == token::string_token   ? "a string"
                                   : code == token::number_token ? "a number"
                                   : code == token::binary_token ? "a binary"
                                   : code == token::hexadecimal_token ? "a hex"
                                                                      : "a percentage";
                const char *bare = kind + 2;    // "string" out of "a string"
                why = std::string("that ") + bare + "." + method_spelling(method) + " is not built for " + kind +
                      " yet -- " + so_far_whose(method);
                return not_built_yet;
            }
        }
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        ++at;
    }
    return success;
}

// AN OBJECT BEING DECLARED (2026-09-22): `run_log log`, `tagged_report.run_log log(path)`,
// `run_log also = log`. `at` is on the first name and left past the statement.
//
// The spacesuit must be one this line can reach; the name may not be a file's, a space's
// or a field's; what follows the name is `=` and a value, the constructor's arguments in
// brackets, or nothing -- and those arguments are counted against the constructor before
// anything runs (003 counted them only running, its S0722).
//
// DECLARED AGAIN IS A NEW OBJECT, not a second declaration to refuse, when it is the
// same spacesuit: 003's DESIGN 7.4 ("a redeclaration therefore takes a fresh slot"),
// and the author's declaration files do it 14,704 times.
signed long long int check_object_declaration(const std::vector<std::bitset<16>> &row, std::size_t &at,
                                              Where &where, DeclaredNames &declared, std::string &why)
{
    const std::size_t stop = past_the_statement(row, at);
    std::size_t k = at;
    std::vector<std::string> names;
    dotted_names_at(row, k, names);
    const std::string name = text_at(row, k);
    std::string written;
    for (const std::string &each : names) written += (written.empty() ? "" : ".") + each;

    const std::size_t suit = where.capsules.suit_named(where.scope, names, why);
    if (suit == kNoScope) { at = stop; return name_not_declared; }
    {
        const signed long long int named = a_name_it_may_take(where.capsules, where.scope, name, why);
        if (named != success) { at = stop; return named; }
    }
    const DeclaredNames::const_iterator already = declared.find(name);
    if (already != declared.end()) {
        const DeclaredObjects::const_iterator object = where.objects.find(name);
        const bool a_field = where.site != nullptr && where.site->suit != kNoScope &&
                             where.capsules.scopes[where.site->suit].layout->slot_of(name) != kNoSlot;
        if (a_field || object == where.objects.end() || object->second != suit) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
    }
    const CapsuleScope &of = where.capsules.scopes[suit];
    const std::string shown = of.layout->shown;

    if (code_at(row, k) == token::assign_token) {
        const signed long long int held = names_in_statement(row, k + 1, stop, declared, where, why);
        if (held != success) { at = stop; return held; }
    } else {
        const Code after = code_at(row, k);
        const bool bracketed = after == token::left_parenthesis_token;
        if (!bracketed && after != token::line_end_token && after != token::comment_token &&
            after != token::end_of_file_token) {
            why = name + " is followed by something that is not =, its arguments in brackets, or the line's end";
            at = stop;
            return satl_line_not_understood;
        }
        std::size_t given = 0, close = k;
        if (bracketed) {
            if (!brackets_at(row, k, close, given)) {
                why = written + " " + name + "( is never closed on its line";
                at = stop;
                return satl_line_not_understood;
            }
            const Code end = code_at(row, close + 1);
            if (end != token::line_end_token && end != token::comment_token && end != token::end_of_file_token) {
                why = written + " " + name + "(...) is the whole statement, and something follows its )";
                at = stop;
                return satl_line_not_understood;
            }
        }
        // A FIELD WITH NOTHING AFTER ITS NAME IS AN EMPTY SLOT (suit_run.hpp): no
        // constructor runs, so there is nothing to count.
        if (!(where.field && !bracketed)) {
            const std::size_t receiving = where.capsules.constructor_of(suit);
            if (receiving == kNoSite) {
                if (given != 0) {
                    why = written + " " + name + " is declared with arguments, and " + shown +
                          " has no satellite.constructor to take them";
                    at = stop;
                    return satl_line_not_understood;
                }
            } else {
                const CapsuleSite &constructor = where.capsules.sites[receiving];
                if (given != constructor.parameters.size()) {
                    const std::size_t takes = constructor.parameters.size();
                    why = constructor.shown + " takes " + std::to_string(takes) +
                          (takes == 1 ? " argument" : " arguments") + ", and " + name + " was given " +
                          std::to_string(given) + (bracketed ? "" : " -- they go in brackets after the name");
                    at = stop;
                    return satl_line_not_understood;
                }
            }
        }
        if (bracketed) {
            const signed long long int held = names_in_statement(row, k + 1, close, declared, where, why);
            if (held != success) { at = stop; return held; }
        }
    }
    declared[name] = word::code_of(1, 10);
    where.objects[name] = suit;
    where.lists.erase(name);
    at = stop;
    return success;
}

// One statement, judged without running it. `at` is left on the code after it.
signed long long int check_statement(const std::vector<std::bitset<16>> &row,
                                     std::size_t &at,
                                     Where &where,
                                     DeclaredNames &declared,
                                     EndingNames &ending,
                                     std::string &why)
{
    const CapsuleTable &capsules = where.capsules;
    const std::size_t scope = where.scope;
    const FunctionTable &functions = where.functions;
    where.statement = at;
    forget_the_finished(ending, at, declared);
    const Code code = code_at(row, at);

    if (code == token::line_end_token || code == token::left_brace_token ||
        code == token::right_brace_token) {
        ++at;
        return success;
    }

    // satellite.return, AND WHAT IT HANDS BACK (2026-09-22). Its value is judged like any
    // expression's, and a constructor hands back nothing -- "what a constructor produces
    // is the object" (003's S0525). Any other capsule answers whatever it hands back:
    // nothing in its header says what that is (satellite.returns, taken out 2026-09-24).
    if (code == word::code_of(1, 15)) {
        const std::size_t stop = past_the_statement(row, at);
        const std::size_t value_at = return_value_at(row, at);
        if (value_at != 0) {
            if (where.site != nullptr && where.site->constructor) {
                why = where.site->shown + " hands nothing back -- what a constructor produces is the object, so its "
                                          "satellite.return is written satellite.return()";
                at = stop;
                return satl_line_not_understood;
            }
            std::size_t close = at + 1, given = 0;
            if (!brackets_at(row, at + 1, close, given)) {
                why = "satellite.return( is never closed on its line";
                at = stop;
                return satl_line_not_understood;
            }
            const Code after = code_at(row, close + 1);
            if (after != token::line_end_token && after != token::comment_token && after != token::end_of_file_token) {
                why = "satellite.return(...) is the whole statement, and something follows its )";
                at = stop;
                return satl_line_not_understood;
            }
            if (given > 1) {
                why = "satellite.return(...) hands back one value, and was given " + std::to_string(given);
                at = stop;
                return satl_line_not_understood;
            }
            const signed long long int held = names_in_statement(row, value_at, close, declared, where, why);
            at = stop;
            return held;
        }
        at = stop;
        return success;
    }

    // A SPACESUIT, OR ONE OF ITS SECTIONS, WRITTEN INSIDE A CAPSULE (2026-09-22).
    // A spacesuit that comes to exist when a capsule runs is POLYMORPH M1, and what it
    // means is the author's to rule: where it lives once the capsule has run, and what
    // a second run makes (M1's D1 and D2).
    if (code == word::code_of(1, 10)) {
        why = "a satellite.spacesuit declared inside a capsule, that comes to exist when the capsule runs, is not "
              "built yet (POLYMORPH M1) -- declare it at the top of a file, in a satellite.namespace, or inside "
              "another spacesuit";
        at = past_the_statement(row, at);
        return not_built_yet;
    }
    if (code == word::code_of(1, 11) || code == word::code_of(1, 12) || code == word::code_of(1, 24)) {
        why = std::string(word::spelling_of(code)) + " is a section of a satellite.spacesuit, and goes inside one "
                                                     "beside the others -- not inside a capsule";
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }

    // satellite.statement.if -- the same shape as while below, judged the same way.
    if (code == word::code_of(1, 13, 1)) {
        const std::size_t stop = past_the_statement(row, at);
        const signed long long int held =
            names_in_statement(row, at + 1, stop, declared, where, why);
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
            names_in_statement(row, at + 1, stop, declared, where, why);
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
            names_in_statement(row, k, parts.condition - 1, declared, where, why);
        if (held != success) { at = stop; return held; }
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (!declared.emplace(name, word::code_of(1, 6, 4)).second) {
            why = declared_twice(where, name);
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
        held = names_in_statement(row, parts.condition, parts.closing, declared, where, why);
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

    // A CAPSULE OR A SPACE DECLARED INSIDE A CAPSULE (2026-09-22). capsules_in steps
    // over every capsule's body whole, so neither is declared there -- and without
    // these two they were told "is a declaration, and only ... are built yet", which
    // is about variables. A space that comes to exist when its capsule runs is the
    // author's idea (POLYMORPH M1) and is not built; a capsule inside a capsule has
    // never been one.
    if (code == word::code_of(1, 2)) {                   // satellite.capsule
        why = "satellite.capsule goes at the top of a file or inside a satellite.namespace, not inside another "
              "capsule";
        at = past_the_statement(row, at);
        return satl_line_not_understood;
    }
    if (code == word::code_of(1, 28)) {                  // satellite.namespace, and satellite.space
        why = "satellite.namespace goes at the top of a file or inside another satellite.namespace -- a space "
              "declared inside a capsule, that comes to exist when the capsule runs, is not built yet";
        at = past_the_statement(row, at);
        return not_built_yet;
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
        // `satellite.container.list<data_unit>` NAMES A SPACESUIT, which must be one this
        // line can reach (2026-09-22).
        if (names_a_suit(shape) && !resolve_shape(capsules, scope, shape, why)) {
            at = stop;
            return name_not_declared;
        }
        const std::string name = text_at(row, k);
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (!declared.emplace(name, code).second) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        remember_shape(where, name, shape);
        const signed long long int held = names_in_statement(row, k, stop, declared, where, why);
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
                  ".percentage, .file, .bool, .infinity, .float, .hex, .color, .fraction, .window, "
                  ".thread and satellite.container.list, .index and .multiple are built yet";
            at = stop;
            return satl_line_not_understood;
        }
        {
            const signed long long int named = a_name_it_may_take(capsules, scope, name, why);
            if (named != success) { at = stop; return named; }
        }
        if (!declared.emplace(name, code).second) {
            why = declared_twice(where, name);
            at = stop;
            return name_declared_twice;
        }
        const signed long long int shaped = after_the_name(row, k, name, true, why);
        if (shaped != success) { at = stop; return shaped; }
        remember_shape(where, name, plain_shape(code));
        if (code == word::code_of(1, 6, 5) && code_at(row, k) == token::assign_token) {
            const signed long long int written = binary_is_written_with_b(row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        if (code == word::code_of(1, 6, 16) && code_at(row, k) == token::assign_token) {
            const signed long long int written = percentage_is_written_with_percent(row, k + 1, why);
            if (written != success) { at = stop; return written; }
        }
        if (one_of_the_four(code) && code_at(row, k) == token::assign_token) {
            const signed long long int written = written_right_for(code, row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        const signed long long int held = names_in_statement(row, k, stop, declared, where, why);
        at = stop;
        return held;
    }

    // A LINE THAT STARTS WITH A satellite.library VALUE (library_values.hpp): writing one is
    // S250, and a line that only reads one does nothing -- both refused before anything runs.
    if (is_library_word(code) && code_at(row, at + 1) == token::method_token)
        return library_statement(capsules, where.registry, row, at, why);

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
                    names_in_statement(row, at, stop, declared, where, why);
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
        const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
        at = stop;
        return held;
    }

    // AN OBJECT BEING DECLARED -- a spacesuit's name and then a name (suit_run.hpp).
    if (code == token::name_token && an_object_declaration_at(row, at))
        return check_object_declaration(row, at, where, declared, why);

    if (code == token::name_token) {
        std::size_t k = at;
        const std::string name = text_at(row, k);
        const std::size_t stop = past_the_statement(row, at);
        // `name(` is a capsule call; `name =` is an assignment to a declared name.
        const DeclaredNames::const_iterator found = declared.find(name);
        // A CAPSULE CALL STANDING AS ITS OWN STATEMENT -- `greet(1)`, `other.greet()`,
        // `tools.x(1)` -- the one place a capsule may be called (names_in_statement
        // says why). The walker takes `name(` as a capsule whatever else the name is,
        // and `a.b(` as one when no variable is named `a` (a_name_it_may_take keeps a
        // variable from ever sharing a file's or a space's name); this reads them the
        // same way, and hands names_in_statement only the ARGUMENTS.
        {
            std::vector<std::string> names;
            std::size_t open = at;
            dotted_names_at(row, open, names);
            const bool called = code_at(row, open) == token::left_parenthesis_token;
            const bool bare = called && names.size() == 1;
            const bool dotted = called && names.size() >= 2 && found == declared.end();
            if (bare || dotted) {
                const Reached reached = capsules.reach(scope, names);
                if (reached.site == nullptr && (bare || reached.through_a_scope)) {
                    why = reached.why;
                    at = stop;
                    return reached.code;
                }
                if (reached.site != nullptr) {
                    std::string written = names.front();
                    for (std::size_t n = 1; n < names.size(); ++n) written += "." + names[n];
                    signed long long int held = given_what_it_takes(row, open, *reached.site, written, why);
                    if (held != success) { at = stop; return held; }
                    held = a_capsule_with_an_object(*reached.site, where, written, why);
                    if (held != success) { at = stop; return held; }
                    // NOTHING AFTER ITS `)`. A call standing as a statement is walked as a
                    // statement, which steps from the `)` to the next line: `other.greet().reverse()`
                    // ran the capsule and dropped the rest without a word (the review,
                    // 2026-09-22, and as true of `greet().reverse()` before scopes).
                    std::size_t close = open, given = 0;
                    brackets_at(row, open, close, given);      // given_what_it_takes proved it closes
                    const Code after = code_at(row, close + 1);
                    if (after != token::line_end_token && after != token::comment_token &&
                        after != token::end_of_file_token) {
                        why = written + "(...) standing as a statement is the whole statement -- to use what it "
                                        "answers, declare a name with it first, as in <type> answer = " +
                              written + "(...)";
                        at = stop;
                        return satl_line_not_understood;
                    }
                    held = names_in_statement(row, open + 1, close, declared, where, why);
                    at = stop;
                    return held;
                }
            }
        }
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
        // `argz.some_var = value` -- A ROW OF THE ARGUMENTS VARIABLE BEING WRITTEN (2026-09-23).
        // A row satl holds is refused here, before anything runs, by the same words the
        // walker asks (main_arguments.hpp); a name of the program's own is a shape to allow,
        // and its value is judged. `argz.n += 1` is refused as every name's `+=` is.
        if (a_method_call && found != declared.end() && found->second == word::code_of(1, 6, 21)) {
            std::string key;
            const std::size_t past = past_the_argument_names(row, k, key);
            const Code after = code_at(row, past);
            if (past != k && after >= token::assign_token && after <= token::modulus_assign_token) {
                signed long long int held = after_the_name(row, past, name + "." + key, false, why);
                if (held == success) {
                    why = why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions);
                    held = why.empty() ? names_in_statement(row, past + 1, stop, declared, where, why)
                                       : word_takes_no_assignment;
                }
                at = stop;
                return held;
            }
            // `argz.l[1] = v`, AN ITEM OF A ROW: allowed below as `a[i] = v` is, and refused
            // here only when the row is satl's -- which rows the program wrote, only the
            // walker knows.
            std::size_t end = past, close = 0, count = 0;
            while (code_at(row, end) == token::left_square_bracket_token && brackets_at(row, end, close, count))
                end = close + 1;
            if (past != k && end != past && code_at(row, end) == token::assign_token) {
                why = why_an_argument_is_not_written(key, name, nullptr, where.arguments, where.functions);
                if (!why.empty()) { at = stop; return word_takes_no_assignment; }
            }
        }
        // AN OBJECT'S MEMBER IS JUDGED FIRST (2026-09-22), so `log.path = x` is told that a
        // field is reached from inside its spacesuit only, and not that a call's answer
        // cannot be given a value -- true, and about the wrong thing.
        if (a_method_call && found != declared.end() && where.objects.count(name) != 0) {
            const signed long long int held = names_in_statement(row, at, stop, declared, where, why);
            if (held != success) { at = stop; return held; }
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
        if (found != declared.end() && one_of_the_four(found->second) && code_at(row, k) == token::assign_token) {
            const signed long long int written = written_right_for(found->second, row, k + 1, declared, why);
            if (written != success) { at = stop; return written; }
        }
        // satellite.variable.color (2026-09-22): `c = ff00aa` IS SIX HEX DIGITS AND NOT A
        // NAME (color_check.cpp's color_names_start), so the names are looked for after it.
        std::size_t names_from = at;
        if (found != declared.end() && found->second == word::code_of(1, 6, 19) && code_at(row, k) == token::assign_token)
            names_from = color_names_start(row, k + 1, at, declared);
        const signed long long int held = names_in_statement(row, names_from, stop, declared, where, why);
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
    Where where{registry, none, kNoScope, functions};
    const std::vector<std::bitset<16>> &row = registry.front();
    for (std::size_t at = 0; at < row.size() && code_at(row, at) != token::end_of_file_token; ) {
        const std::size_t was = at;
        std::string why;
        const signed long long int stopped = check_statement(row, at, where, declared, ending, why);
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
    // WHAT THE SCAN FOUND COMES FIRST, AND THE EARLIEST OF IT. capsules_in() has no
    // program to stop and no sentence to print, so a header it could not read, a
    // name declared twice in one scope, a variable inside a satellite.namespace or
    // a spacesuit nobody can declare yet is refused here -- with a caret now, which
    // the header refusals never had. EARLIEST BY FILE AND POSITION, and not in the
    // order the passes happened to find them: nobody reads a program by pass.
    //
    // AND THEN THE CAPSULES IN FILE ORDER. The table was an unordered_map until
    // scopes, and which of two wrong capsules a person was told about depended on
    // which one the hash put first.
    if (!capsules.troubles.empty()) {
        const ScopeTrouble *first = &capsules.troubles.front();
        for (const ScopeTrouble &each : capsules.troubles)
            if (each.row < first->row || (each.row == first->row && each.at < first->at))
                first = &each;
        return raise_at(first->code, first->why, std::string(), state, registry[first->row], first->at,
                        "satl(check)");
    }

    // EVERY satellite.library VALUE, READ ONCE (library_values.hpp) -- before any capsule
    // is judged, and so before anything runs. Values, not globals: nothing changes one.
    {
        const signed long long int read = read_library_values(registry, capsules, functions, state);
        if (stops_the_program(read))
            return read;
    }

    for (const CapsuleSite &site : capsules.sites) {
        const std::vector<std::bitset<16>> &row = registry[site.row];
        // One set a capsule: there are no globals, so a name declared elsewhere
        // is not declared here.
        DeclaredNames declared;
        Where where{registry, capsules, site.scope, functions};
        where.site = &site;
        where.arguments = state.arguments;
        // A SPACESUIT'S CAPSULE SEES ITS OBJECT'S FIELDS BY THEIR BARE NAMES (2026-09-22):
        // the author's `path = path_input`, `satellite.return(spacesuit_name)`. They are
        // its first declared names, as its parameters are, and a field that holds an
        // object is one whose members are judged against that object's spacesuit.
        // In order, so a field its spacesuit declared again over a supertype's is the one
        // the name means here.
        if (site.suit != kNoScope)
            for (const SuitField &field : capsules.scopes[site.suit].layout->fields) {
                declared[field.name] = field.shape.word;
                remember_shape(where, field.name, field.shape);
            }
        // ITS PARAMETERS ARE ITS FIRST DECLARED NAMES (2026-09-21). They are
        // declared by the header rather than by a satellite.variable line, and
        // without this the body that uses one is refused with "has no
        // satellite.variable line declaring it" -- a true sentence about a name
        // that really was declared, just not where the checker was looking.
        //
        // satellite.main's TOO, SINCE 2026-09-22. It was left out on purpose while
        // run_main bound nothing -- a declared name that would not be there moves
        // the refusal from the checker to the walker. run_main binds it now: it is
        // the arguments variable, every row satl holds (main_arguments.hpp).
        for (const CapsuleParameter &takes : site.parameters) {
            std::string why;
            const signed long long int named = a_name_it_may_take(capsules, site.scope, takes.name, why);
            if (named != success)
                return raise_at(named, why, site.shown, state, row, site.declared_at, "satl(check)");
            if (declared.count(takes.name) != 0)
                return raise_at(name_declared_twice, declared_twice(where, takes.name), site.shown, state, row,
                                site.declared_at, "satl(check)");
            // main's is the arguments variable, whatever type it was written with.
            declared[takes.name] = site.name == "satellite.main" ? word::code_of(1, 6, 21) : takes.declared();
            remember_shape(where, takes.name, takes.shape);
        }
        EndingNames ending;
        std::size_t depth = 0;
        for (std::size_t at = site.body; at < row.size(); ) {
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
            const signed long long int code_of_line = check_statement(row, at, where, declared, ending, why);
            if (stops_the_program(code_of_line))
                // THE STATEMENT'S OWN START, AND NOT WHERE `at` ENDED UP. A
                // refusal leaves `at` wherever check_statement stopped reading,
                // which is not reliably the thing that was wrong -- so the caret
                // goes under the start of the statement, which always is. The
                // LINE is exact either way, and that is what a person looks for
                // first.
                return raise_at(code_of_line, why, site.shown, state, row, was, "satl(check)");
            if (at <= was)                  // a statement must always move forward
                ++at;
        }
    }

    // EVERY FIELD'S OWN STATEMENT (2026-09-22), judged as the declaration it is -- each
    // alone, as 003 made them: a field's value is worked out before there is an object,
    // so it cannot name another field or call its spacesuit's capsules (003's S0511).
    for (std::size_t s = 0; s < capsules.scopes.size(); ++s) {
        const CapsuleScope &suit = capsules.scopes[s];
        if (!suit.is_a_suit())
            continue;
        const std::vector<std::bitset<16>> &row = registry[suit.row];
        // ITS OWN FIELDS: a supertype's were judged where they are declared.
        for (std::size_t slot = suit.layout->own_fields; slot < suit.layout->fields.size(); ++slot) {
            const SuitField &field = suit.layout->fields[slot];
            DeclaredNames declared;
            EndingNames ending;
            Where where{registry, capsules, s, functions};
            where.field = true;
            std::size_t at = field.at;
            std::string why;
            const signed long long int code_of_line = check_statement(row, at, where, declared, ending, why);
            if (stops_the_program(code_of_line))
                return raise_at(code_of_line, why, suit.layout->shown, state, row, field.at, "satl(check)");
        }
        // A SUPERTYPE'S CONSTRUCTOR THAT WANTS ARGUMENTS, when this spacesuit has one of its
        // own: the object's arguments go to the nearest constructor, and every one above it
        // runs with none -- 003's order, which had no super(...) to hand them over either.
        const std::size_t receiving = capsules.constructor_of(s);
        for (std::size_t n = 1; n < suit.layout->lineage.size(); ++n) {
            const std::size_t above = capsules.scopes[suit.layout->lineage[n]].constructor;
            if (above == kNoSite || above == receiving || capsules.sites[above].parameters.empty())
                continue;
            return raise_at(satl_line_not_understood,
                            suit.layout->shown + " extends " + capsules.scopes[suit.layout->lineage[n]].layout->shown +
                                ", whose satellite.constructor takes " +
                                std::to_string(capsules.sites[above].parameters.size()) +
                                (capsules.sites[above].parameters.size() == 1 ? " argument" : " arguments") +
                                ", and nothing can hand them over: an object's arguments go to " +
                                suit.layout->shown + "'s own constructor, and every constructor above it runs with none",
                            std::string(), state, row, suit.declared_at, "satl(check)");
        }
    }
    state.set("program(checked): " + std::to_string(capsules.sites.size()) + " capsules", success);
    return success;
}

} // namespace satellite004
