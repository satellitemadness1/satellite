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

        // A word of the language: its code IS the function table's index.
        if (word::is_word_code(code)) {
            const NumberRow *library = functions[code];
            std::size_t k = at + 1;
            if (code_at(row, k) == token::left_parenthesis_token) {
                ++k;

                // THE ARGUMENT CHOOSES THE SCENARIO, which is the design
                // number_row.hpp already committed to: a library exports one
                // function per KIND of value, so the kind of the argument --
                // which its TOKEN already says -- picks which one runs. This is
                // the smallest thing that is not a value type, and it is the
                // seam where a real one will go in (PROGRESS §6.5).
                const Code argument_code = code_at(row, k);
                const Scenarios *scenarios = library != nullptr ? &library->scenarios : nullptr;

                // THE LIBRARY'S ANSWER IS THE PROGRAM'S ANSWER. A refused write
                // (/dev/full) answers display_error, and throwing that away was
                // a real defect -- check.sh caught it the moment this path ran
                // the checks, 2026-09-16.
                signed long long int answer = success;

                if (argument_code == token::string_token) {
                    const std::string argument = text_at(row, k);
                    if (scenarios != nullptr && scenarios->text != nullptr)
                        answer = scenarios->text(argument, true);
                    else
                        report_error(std::string("satl(run): ") + word::spelling_of(code) +
                                         " has no library built yet", not_built_yet);
                } else if (argument_code == token::number_token) {
                    const std::string digits = text_at(row, k);
                    if (scenarios != nullptr && scenarios->count != nullptr) {
                        unsigned long long int value = 0;
                        bool whole = !digits.empty();
                        for (char d : digits) {
                            if (d < '0' || d > '9') { whole = false; break; }
                            value = value * 10 + static_cast<unsigned long long int>(d - '0');
                        }
                        if (whole)
                            answer = scenarios->count(value, true);
                        else
                            report_error("satl(run): " + digits + " is not a whole number yet",
                                         not_built_yet);
                    } else {
                        report_error(std::string("satl(run): ") + word::spelling_of(code) +
                                         " has no library built yet", not_built_yet);
                    }
                } else if (argument_code == word::code_of(1, 17, 1) ||
                           argument_code == word::code_of(1, 17, 2)) {
                    // satellite.bool.false and satellite.bool.true are WORDS,
                    // so the bool arrives as a code and never as text.
                    const bool value = argument_code == word::code_of(1, 17, 2);
                    if (scenarios != nullptr && scenarios->flag != nullptr)
                        answer = scenarios->flag(value, true);
                    else
                        report_error(std::string("satl(run): ") + word::spelling_of(code) +
                                         " has no library built yet", not_built_yet);
                    ++k;
                }
                while (k < row.size() && code_at(row, k) != token::right_parenthesis_token) {
                    if (token::carries_a_count(code_at(row, k))) { text_at(row, k); continue; }
                    ++k;
                }
                if (k < row.size()) ++k;
                if (stops_the_program(answer))
                    return answer;
            }
            at = k;
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
