// satellite/bytecode/program_walk.cpp -- the header says what the three pieces
// are for and why a call is a position rather than an object. The checker moved
// to program_check.cpp when statements grew past calls, so both stay near the
// author's 300-line target.
//
// A STATEMENT IS ONE OF SIX SHAPES, and run_statements below is that list:
//
//     satellite.return(...)                      ends the body
//     satellite.variable.number <name> = <expr>  declares, and gives a value
//     <name> = <expr>                            gives a value to one declared
//     satellite.statement.while(<expr>) { ... }  runs the body while it holds
//     <word>(<expr>)                             a word of the language
//     <name>()                                   a capsule the user wrote
//
// NOTHING IS ALLOCATED TO RUN A LINE still holds, with one honest exception: a
// body's VariableTable. It is created when the body starts and destroyed when it
// ends, which is what makes "there are no globals" (the author, 2026-09-16) true
// by construction -- a capsule is handed a different table, so it CANNOT see its
// caller's variables even by accident.

#include "program_walk.hpp"

#include "word_codes.hpp"
#include "../satl/satl_file.hpp"

#include <fstream>
#include <utility>
#include <sstream>

namespace satellite004 {
namespace {

using token::Code;

} // namespace

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

std::size_t past_the_statement(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (at < row.size() && code_at(row, at) != token::line_end_token) {
        if (token::carries_a_count(code_at(row, at))) { text_at(row, at); continue; }
        ++at;
    }
    return at < row.size() ? at + 1 : at;
}

// The code just past the `}` that closes the `{` at `from`. Counts braces, and
// SKIPS a counted payload rather than reading it -- a string holding a `}` must
// not close a body, which is the same rule that keeps a string saying
// "satellite.main" from declaring one (PROGRESS §6.5).
std::size_t past_matching_brace(const std::vector<std::bitset<16>> &row, std::size_t from)
{
    std::size_t depth = 0;
    std::size_t at = from;
    while (at < row.size()) {
        const Code code = code_at(row, at);
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        if (code == token::left_brace_token) ++depth;
        else if (code == token::right_brace_token && --depth == 0) return at + 1;
        ++at;
    }
    return at;
}

// The `{` that opens a body after `at`, past any line ends. The author writes
// the brace on its own line, so this cannot simply be the next code.
std::size_t brace_after(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    while (at < row.size() && code_at(row, at) == token::line_end_token) ++at;
    return at;
}

signed long long int load_program(const std::string &main_file,
                                  StartupThreads &threads,
                                  unsigned long long int batches,
                                  BytecodeRegistry &registry,
                                  BytecodeFilenames &filenames,
                                  MachineState &state)
{
    registry.clear();
    filenames.clear();

    // Each file waits with the name of the file that asked for it, so a missing
    // one can say who wanted it.
    std::vector<std::pair<std::string, std::string>> waiting{{main_file, std::string()}};
    std::vector<std::string> loaded;

    while (!waiting.empty()) {
        const std::string path = waiting.front().first;
        const std::string asked_by = waiting.front().second;
        waiting.erase(waiting.begin());

        bool already = false;
        for (const std::string &done : loaded) already = already || done == path;
        if (already)
            continue;   // a cycle of includes ends here instead of running forever
        loaded.push_back(path);

        // CANNOT LOCATE FILE (the author, 2026-09-16), said before load_satl is
        // asked, so the message names the file the program meant rather than
        // whatever the operating system called the failure.
        std::ifstream there(path);
        if (!there)
            return report_error("cannot locate file: " + path +
                                    (asked_by.empty() ? std::string() : ", included by " + asked_by),
                                missing_satl_file);

        std::string source;
        const signed long long int code = load_satl(path, source, state);
        if (stops_the_program(code))
            return code;

        add_file_to_bytecode_registry(path, source, threads, batches, registry, filenames);

        // Only the includes are read out of a file at load time. There are no
        // globals, so nothing else in it can run before main does.
        const std::vector<std::bitset<16>> &row = registry.back();
        for (std::size_t i = 0; i < row.size(); ) {
            if (code_at(row, i) != word::code_of(1, 1)) { ++i; continue; }
            std::size_t k = i;
            const IncludeShape shape = include_at(row, k, path);
            i = (k > i) ? k : i + 1;
            if (shape.kind == IncludeShape::Kind::none ||
                shape.kind == IncludeShape::Kind::main_marker)
                continue;
            waiting.push_back({shape.resolved, path});
        }
    }

    state.set("program(loaded): " + std::to_string(registry.size()) + " files, " +
                  std::to_string(codes_in(registry)) + " codes", success);
    return success;
}

CapsuleTable capsules_in(const BytecodeRegistry &registry)
{
    CapsuleTable table;
    for (std::size_t r = 0; r < registry.size(); ++r) {
        const std::vector<std::bitset<16>> &row = registry[r];
        for (std::size_t i = 0; i < row.size(); ++i) {
            if (code_at(row, i) != word::code_of(1, 2))   // satellite.capsule
                continue;

            // The name is the next code: a word (satellite.main) or a name the
            // user owns. Then its arguments, then the `{` its body opens with.
            std::size_t k = i + 1;
            std::string name;
            if (word::is_word_code(code_at(row, k))) {
                name = word::spelling_of(code_at(row, k));
                ++k;
            } else if (code_at(row, k) == token::name_token) {
                name = text_at(row, k);
            } else {
                continue;
            }

            while (k < row.size() && code_at(row, k) != token::left_brace_token &&
                   code_at(row, k) != token::right_brace_token) {
                if (token::carries_a_count(code_at(row, k))) { text_at(row, k); continue; }
                ++k;
            }
            if (code_at(row, k) != token::left_brace_token)
                continue;

            table[name] = CapsuleSite{r, k + 1};
            i = k;
        }
    }
    return table;
}

namespace {

// Every statement from `from` until the `}` that closes the body, or the row's
// end. `variables` is THIS body's own table.
signed long long int run_statements(const BytecodeRegistry &registry,
                                    const CapsuleTable &capsules,
                                    const FunctionTable &functions,
                                    std::size_t which_row,
                                    std::size_t from,
                                    VariableTable &variables,
                                    MachineState &state);

// `satellite.statement.while(<expr>) { ... }`. `at` is on the word code and is
// left past the body's `}`.
//
// THE CONDITION IS RE-READ FROM THE SAME POSITION EVERY TURN, which is the whole
// loop: the walker keeps a position, so going round again is assigning one
// integer, not rebuilding anything. Nothing is allocated per iteration.
signed long long int run_while(const BytecodeRegistry &registry,
                               const CapsuleTable &capsules,
                               const FunctionTable &functions,
                               std::size_t which_row,
                               std::size_t &at,
                               VariableTable &variables,
                               MachineState &state)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];
    const std::size_t condition_at = at + 1;          // the `(`
    std::size_t after = past_the_statement(row, at);
    const std::size_t brace = brace_after(row, after);
    if (code_at(row, brace) != token::left_brace_token) {
        at = after;
        return report_error("satl(run): satellite.statement.while has no body", satl_line_not_understood);
    }
    const std::size_t past = past_matching_brace(row, brace);
    at = past;

    for (;;) {
        std::size_t here = condition_at;
        ExpressionContext context{variables, functions, state};
        if (code_at(row, here) == token::left_parenthesis_token) ++here;
        const Value holds = evaluate_expression(row, here, context);
        if (context.code != success)
            return report_error("satl(run): in satellite.statement.while, " + context.why, context.code);
        if (holds.kind != Value::Kind::flag)
            return report_error(std::string("satl(run): satellite.statement.while was given ") +
                                    holds.kind_name() + " and needs a true or false",
                                types_do_not_meet);
        if (!holds.flag)
            return success;

        // THE BODY SHARES THIS BODY'S VARIABLES. A while is not a capsule: the
        // author's own program writes `counter = counter + 1` inside one and
        // expects the counter outside it to move.
        const signed long long int code =
            run_statements(registry, capsules, functions, which_row, brace + 1, variables, state);
        if (stops_the_program(code))
            return code;
    }
}

// `satellite.variable.number <name> = <expr>`, and `<name> = <expr>`.
// `declared` is 0 for a plain assignment.
signed long long int run_assignment(const std::vector<std::bitset<16>> &row,
                                    std::size_t &at,
                                    Code declared,
                                    const std::string &name,
                                    const FunctionTable &functions,
                                    VariableTable &variables,
                                    MachineState &state)
{
    // A DECLARATION DOES NOT REFUSE A NAME THAT IS ALREADY THERE, and that is
    // not laxity -- it is the difference between a declaration being reached
    // TWICE IN THE TEXT and being reached twice IN TIME. A declaration inside a
    // while body is one declaration that runs once per turn of the loop:
    //
    //     satellite.statement.while(n < 3)
    //     {
    //         satellite.variable.number m = 0      <- once in the text, 3 times in time
    //
    // Refusing here made that program die on its second iteration, having
    // already printed its first (found by running it, 2026-09-16). The real
    // error -- two declarations of one name in one capsule -- is caught by
    // program_check.cpp BEFORE anything runs, which is the right place for it:
    // the checker walks the text, so it sees each declaration exactly once.
    const VariableTable::iterator found = variables.find(name);
    if (declared == 0 && found == variables.end()) {
        at = past_the_statement(row, at);
        return report_error("satl(run): " + name + " has no satellite.variable line declaring it", name_not_declared);
    }
    if (code_at(row, at) != token::assign_token) {
        // A declaration with no `=` is a variable with no value yet. Nothing can
        // read it before something writes it, which name_not_declared already says.
        at = past_the_statement(row, at);
        if (declared != 0)
            variables[name] = Variable{declared, Value()};
        return success;
    }
    ++at;

    ExpressionContext context{variables, functions, state};
    Value value = evaluate_expression(row, at, context);
    if (context.code != success) {
        at = past_the_statement(row, at);
        return report_error("satl(run): in " + name + " = ..., " + context.why, context.code);
    }

    // THE DECLARED TYPE OUTLIVES THE LINE THAT WROTE IT. `n = "text"` on a
    // number is refused rather than quietly making n a string (value.hpp).
    const Code holds = declared != 0 ? declared : found->second.declared;
    if (holds == word::code_of(1, 6, 4) && value.kind != Value::Kind::number) {
        at = past_the_statement(row, at);
        return report_error(std::string("satl(run): ") + name + " was declared satellite.variable.number and was given " +
                                value.kind_name(),
                            types_do_not_meet);
    }
    variables[name] = Variable{holds, std::move(value)};
    at = past_the_statement(row, at);
    return success;
}

signed long long int run_statements(const BytecodeRegistry &registry,
                                    const CapsuleTable &capsules,
                                    const FunctionTable &functions,
                                    std::size_t which_row,
                                    std::size_t from,
                                    VariableTable &variables,
                                    MachineState &state)
{
    const std::vector<std::bitset<16>> &row = registry[which_row];

    for (std::size_t at = from; at < row.size(); ) {
        const Code code = code_at(row, at);

        if (code == token::right_brace_token)
            return success;
        if (code == token::line_end_token) { ++at; continue; }

        if (code == word::code_of(1, 15)) {          // satellite.return
            state.set("satellite.return", success);
            return success;
        }

        if (code == word::code_of(1, 13, 3)) {       // satellite.statement.while
            const signed long long int stopped =
                run_while(registry, capsules, functions, which_row, at, variables, state);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        // A DECLARATION IS A WORD FOLLOWED BY A NAME; a call is a word followed
        // by `(`. That one test tells them apart with no list of types.
        if (word::is_word_code(code) && code_at(row, at + 1) == token::name_token) {
            std::size_t k = at + 1;
            const std::string name = text_at(row, k);
            at = k;
            const signed long long int stopped =
                run_assignment(row, at, code, name, functions, variables, state);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        // A word of the language: its code IS the function table's index, and
        // its arguments are evaluated inner to outer.
        if (word::is_word_code(code)) {
            ExpressionContext context{variables, functions, state};
            std::size_t k = at;
            call_word(row, k, context);
            at = past_the_statement(row, k);
            if (context.code != success)
                return report_error("satl(run): " + context.why, context.code);
            continue;
        }

        if (code == token::name_token) {
            std::size_t k = at;
            const std::string name = text_at(row, k);

            // A name followed by `(` is a capsule; a name followed by `=` is an
            // assignment. Nothing else is a statement a name can start.
            if (code_at(row, k) == token::left_parenthesis_token) {
                at = past_the_statement(row, k);
                const CapsuleTable::const_iterator found = capsules.find(name);
                if (found == capsules.end()) {
                    report_error("satl(run): no capsule named " + name, satl_line_not_understood);
                    continue;
                }
                // A NEW TABLE, so the capsule cannot see this body's variables.
                VariableTable theirs;
                const signed long long int stopped =
                    run_statements(registry, capsules, functions, found->second.row, found->second.body, theirs, state);
                if (stops_the_program(stopped))
                    return stopped;
                continue;
            }
            at = k;
            const signed long long int stopped =
                run_assignment(row, at, 0, name, functions, variables, state);
            if (stops_the_program(stopped))
                return stopped;
            continue;
        }

        if (token::carries_a_count(code)) { std::size_t k = at; text_at(row, k); at = k; continue; }
        ++at;
    }
    return success;
}

} // namespace

signed long long int run_main(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              MachineState &state)
{
    const CapsuleTable::const_iterator main = capsules.find("satellite.main");
    if (main == capsules.end())
        return report_error("satl(run): no satellite.main to begin in",
                            satl_file_missing_satellite_main);
    VariableTable variables;   // main's own, and the program's only frame to start
    return run_statements(registry, capsules, functions, main->second.row, main->second.body, variables, state);
}

} // namespace satellite004
