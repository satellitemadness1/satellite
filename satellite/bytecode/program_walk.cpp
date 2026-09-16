// satellite/bytecode/program_walk.cpp -- the header says what the three pieces
// are for and why a call is a position rather than an object.

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

static Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

namespace {

// THE FALLBACK IS GONE, because the leading slash is now a RULE (the author,
// 2026-09-16): include(/test) is ./test, and only a path under the user's home
// directory is truly absolute. include_shape.cpp resolves both, so nothing here
// has to guess which of two places a file might be in.

} // namespace

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

// One statement of a body, judged without running it. `at` is left on the code
// after the statement. See the header for which code means what.
static signed long long int check_statement(const std::vector<std::bitset<16>> &row,
                                            std::size_t &at,
                                            const CapsuleTable &capsules,
                                            const FunctionTable &functions,
                                            std::string &why)
{
    const Code code = code_at(row, at);

    if (code == word::code_of(1, 15)) {            // satellite.return
        while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
        return success;
    }

    if (word::is_word_code(code)) {
        const std::string spelling = word::spelling_of(code);
        std::size_t k = at + 1;

        // A WORD NOT FOLLOWED BY ( IS NOT A CALL. `satellite.variable.string s`
        // is a declaration, and there is no scenario for one yet.
        if (code_at(row, k) != token::left_parenthesis_token) {
            why = spelling + " is not a call, and there is no scenario for it yet";
            while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
            return satl_line_not_understood;
        }

        ++k;
        const Code first = code_at(row, k);
        const NumberRow *library = functions[code];
        const Scenarios *scenarios = library != nullptr ? &library->scenarios : nullptr;

        // What stands between ( and ) decides which scenario runs, so what is
        // THERE is what gets judged.
        std::size_t after = k;
        if (token::carries_a_count(first)) text_at(row, after);
        else ++after;
        const bool one_thing = code_at(row, after) == token::right_parenthesis_token;

        if (first == token::string_token && !one_thing) {
            why = spelling + " was given an expression, and there is no scenario for one yet";
            while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
            return string_error;
        }
        if (first == token::number_token) {
            std::size_t digits_at = k;
            const std::string digits = text_at(row, digits_at);
            unsigned long long int value = 0;
            for (char d : digits) {
                if (d < '0' || d > '9') break;
                if (value > (0xFFFFFFFFFFFFFFFFull - static_cast<unsigned long long int>(d - '0')) / 10) {
                    why = digits + " is too large to hold";
                    while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
                    return int_error;
                }
                value = value * 10 + static_cast<unsigned long long int>(d - '0');
            }
        }
        // AN ARGUMENT SHAPE WITH NO SCENARIO IS REFUSED, NOT SKIPPED. Three
        // shapes can run today -- a string, a whole number, and the two bool
        // words -- because number_row.hpp exports one function per KIND of
        // value and there is no value type yet to hold anything else.
        //
        // A NESTED CALL IS THE ONE THAT BITES: display(string.upper("x")) used
        // to be accepted here and then do NOTHING, exit 0, no output and no
        // error (found by running it, 2026-09-16). Its result has to become the
        // argument, which means evaluating inner to outer and putting the answer
        // somewhere -- and that somewhere is the value type this does not have.
        // Until then it is refused in as many words.
        const bool a_bool = first == word::code_of(1, 17, 1) || first == word::code_of(1, 17, 2);
        const bool nothing_at_all = first == token::right_parenthesis_token;
        const bool a_nested_call = word::is_word_code(first) &&
                                   code_at(row, k + 1) == token::left_parenthesis_token;
        if (!a_bool && !nothing_at_all && !a_nested_call &&
            first != token::string_token && first != token::number_token) {
            why = spelling + " was given " +
                  (word::is_word_code(first) ? std::string(word::spelling_of(first)) + " -- a word as an argument"
                                             : std::string("something")) +
                  ", and there is no value type to carry it yet";
            while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
            return satl_line_not_understood;
        }

        if (scenarios == nullptr ||
            (first == token::string_token && scenarios->text == nullptr) ||
            (first == token::number_token && scenarios->count == nullptr)) {
            why = spelling + " has no library built for that kind of value yet";
            while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
            return not_built_yet;
        }
        while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
        return success;
    }

    if (code == token::name_token) {               // a capsule the user owns
        std::size_t k = at;
        const std::string name = text_at(row, k);
        if (capsules.find(name) == capsules.end()) {
            why = "no capsule named " + name;
            while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
            return satl_line_not_understood;
        }
        while (at < row.size() && code_at(row, at) != token::line_end_token) ++at;
        return success;
    }

    ++at;
    return success;
}

signed long long int check_program(const BytecodeRegistry &registry,
                                   const CapsuleTable &capsules,
                                   const FunctionTable &functions,
                                   MachineState &state)
{
    for (const std::pair<const std::string, CapsuleSite> &entry : capsules) {
        const std::vector<std::bitset<16>> &row = registry[entry.second.row];
        for (std::size_t at = entry.second.body; at < row.size(); ) {
            if (code_at(row, at) == token::right_brace_token) break;
            std::string why;
            const signed long long int code = check_statement(row, at, capsules, functions, why);
            if (stops_the_program(code))
                return report_error("satl(check): in " + entry.first + ", " + why, code);
            if (at < row.size() && code_at(row, at) == token::line_end_token) ++at;
        }
    }
    state.set("program(checked): " + std::to_string(capsules.size()) + " capsules", success);
    return success;
}

namespace {

// ONE ARGUMENT, EVALUATED. `at` is left past whatever was read. A call runs here
// -- inner before outer, which is what makes display(display("x")) work -- and
// answers its machine code as a count.
Value evaluate(const std::vector<std::bitset<16>> &row,
               std::size_t &at,
               const CapsuleTable &capsules,
               const FunctionTable &functions,
               MachineState &state);

// A word's call: its arguments, its scenario, its answer.
Value call_word(const std::vector<std::bitset<16>> &row,
                std::size_t &at,
                const CapsuleTable &capsules,
                const FunctionTable &functions,
                MachineState &state)
{
    const Code code = code_at(row, at);
    const NumberRow *library = functions[code];
    const Scenarios *scenarios = library != nullptr ? &library->scenarios : nullptr;
    ++at;

    Value argument;
    if (code_at(row, at) == token::left_parenthesis_token) {
        ++at;
        if (code_at(row, at) != token::right_parenthesis_token)
            argument = evaluate(row, at, capsules, functions, state);
        while (at < row.size() && code_at(row, at) != token::right_parenthesis_token) ++at;
        if (at < row.size()) ++at;
    }

    signed long long int answer = success;
    if (scenarios != nullptr) {
        if (argument.kind == Value::Kind::text && scenarios->text != nullptr)
            answer = scenarios->text(argument.text, true);
        else if (argument.kind == Value::Kind::count && scenarios->count != nullptr)
            answer = scenarios->count(argument.count, true);
        else if (argument.kind == Value::Kind::flag && scenarios->flag != nullptr)
            answer = scenarios->flag(argument.flag, true);
    }
    if (stops_the_program(answer))
        state.set("satl(run): " + std::string(word::spelling_of(code)) + " refused", answer);

    Value result;
    result.kind = Value::Kind::count;
    result.count = static_cast<unsigned long long int>(answer);
    return result;
}

Value evaluate(const std::vector<std::bitset<16>> &row,
               std::size_t &at,
               const CapsuleTable &capsules,
               const FunctionTable &functions,
               MachineState &state)
{
    const Code code = code_at(row, at);
    Value value;

    if (code == token::string_token) {
        value.kind = Value::Kind::text;
        value.text = text_at(row, at);
        return value;
    }
    if (code == token::number_token) {
        const std::string digits = text_at(row, at);
        value.kind = Value::Kind::count;
        for (char d : digits) {
            if (d < '0' || d > '9') break;
            value.count = value.count * 10 + static_cast<unsigned long long int>(d - '0');
        }
        return value;
    }
    if (code == word::code_of(1, 17, 1) || code == word::code_of(1, 17, 2)) {
        value.kind = Value::Kind::flag;
        value.flag = code == word::code_of(1, 17, 2);
        ++at;
        return value;
    }
    if (word::is_word_code(code) && code_at(row, at + 1) == token::left_parenthesis_token)
        return call_word(row, at, capsules, functions, state);

    ++at;
    return value;
}

// One body, to its closing brace. Recurses into a capsule a line calls, which
// is where a program's own depth comes from -- and the walker keeps its own
// position, never a copy of the program.
signed long long int run_body(const BytecodeRegistry &registry,
                              const CapsuleTable &capsules,
                              const FunctionTable &functions,
                              const CapsuleSite &site,
                              MachineState &state)
{
    const std::vector<std::bitset<16>> &row = registry[site.row];

    for (std::size_t at = site.body; at < row.size(); ) {
        const Code code = code_at(row, at);

        if (code == token::right_brace_token)
            return success;

        if (code == word::code_of(1, 15)) {          // satellite.return
            state.set("satellite.return", success);
            return success;
        }

        // A word of the language: its code IS the function table's index, and
        // its arguments are evaluated inner to outer.
        if (word::is_word_code(code)) {
            std::size_t k = at;
            const Value answer = call_word(row, k, capsules, functions, state);
            at = k;
            if (answer.kind == Value::Kind::count &&
                stops_the_program(static_cast<signed long long int>(answer.count)))
                return static_cast<signed long long int>(answer.count);
            continue;
        }

        // A name the user owns, followed by (): one of their capsules.
        if (code == token::name_token) {
            std::size_t k = at;
            const std::string name = text_at(row, k);
            if (code_at(row, k) == token::left_parenthesis_token) {
                while (k < row.size() && code_at(row, k) != token::right_parenthesis_token) ++k;
                if (k < row.size()) ++k;
                const CapsuleTable::const_iterator found = capsules.find(name);
                if (found == capsules.end())
                    report_error("satl(run): no capsule named " + name, satl_line_not_understood);
                else
                    run_body(registry, capsules, functions, found->second, state);
            }
            at = k;
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
    return run_body(registry, capsules, functions, main->second, state);
}

} // namespace satellite004
