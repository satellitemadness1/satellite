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
// THE DECLARED NAMES ARE TRACKED PER CAPSULE, which is the same rule run_body
// enforces by handing each body its own table: there are no globals, so a name
// declared in one capsule is not declared in another.

#include "program_walk.hpp"

#include "word_codes.hpp"

#include <string>
#include <unordered_set>

namespace satellite004 {
namespace {

using token::Code;
using DeclaredNames = std::unordered_set<std::string>;

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
        if (code != word::code_of(1, 6, 4)) {
            why = std::string(word::spelling_of(code)) + " " + name +
                  " is a declaration, and only satellite.variable.number is built yet";
            at = stop;
            return satl_line_not_understood;
        }
        if (!declared.insert(name).second) {
            why = name + " is declared twice in the same capsule";
            at = stop;
            return name_declared_twice;
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
        if (code_at(row, k) != token::left_parenthesis_token && declared.find(name) == declared.end()) {
            why = name + " has no satellite.variable line declaring it";
            at = stop;
            return name_not_declared;
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
