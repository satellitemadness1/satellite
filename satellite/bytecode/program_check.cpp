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

#include "word_codes.hpp"

#include <string>
#include <unordered_map>

namespace satellite004 {
namespace {

using token::Code;
// Each declared name and the word that declared it -- the TYPE is kept so that a
// later `bits = 1010` can be judged by the same rule as the declaration was.
using DeclaredNames = std::unordered_map<std::string, Code>;

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
            }
            at = k;
            continue;
        }

        // A WORD USED AS A CALL MUST HAVE A LIBRARY. A word with none is
        // not_built_yet (14) with its own name, which is what 003 did and what a
        // person can act on (function_table.hpp).
        if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token &&
            functions[code] == nullptr) {
            why = std::string(word::spelling_of(code)) + " has no library built for it yet";
            return not_built_yet;
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
                                     std::string &why)
{
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
        if (code != word::code_of(1, 6, 4) && code != word::code_of(1, 6, 1) && code != word::code_of(1, 6, 5) &&
            code != word::code_of(1, 6, 16)) {
            why = std::string(word::spelling_of(code)) + " " + name +
                  " is a declaration, and only satellite.variable.number, .string, .binary and "
                  ".percentage are built yet";
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
        // A WORD NOT FOLLOWED BY ( IS NOT A CALL, and with a name after it, it
        // was a declaration above. Anything else has no shape yet.
        if (code_at(row, at + 1) != token::left_parenthesis_token) {
            why = std::string(word::spelling_of(code)) + " is not a call, and there is no scenario for it yet";
            at = past_the_statement(row, at);
            return satl_line_not_understood;
        }
        const std::size_t stop = past_the_statement(row, at);
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
        if (code_at(row, k) != token::left_parenthesis_token) {
            const signed long long int shaped = after_the_name(row, k, name, false, why);
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

    at = past_the_statement(row, at);
    return success;
}

} // namespace

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
                check_statement(row, at, capsules, functions, declared, why);
            if (stops_the_program(code_of_line))
                return report_error("satl(check): in " + entry.first + ", " + why, code_of_line);
            if (at <= was)                  // a statement must always move forward
                ++at;
        }
    }
    state.set("program(checked): " + std::to_string(capsules.size()) + " capsules", success);
    return success;
}

} // namespace satellite004
