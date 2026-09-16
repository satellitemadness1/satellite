// satellite/bytecode/program_walk.cpp -- the header says what the three pieces
// are for and why a call is a position rather than an object.

#include "program_walk.hpp"

#include "word_codes.hpp"
#include "../satl/satl_file.hpp"

#include <fstream>
#include <sstream>

namespace satellite004 {
namespace {

using token::Code;

Code code_at(const std::vector<std::bitset<16>> &row, std::size_t at)
{
    return at < row.size() ? static_cast<Code>(row[at].to_ulong()) : 0;
}

// A LEADING SLASH IS TRIED BOTH WAYS, and the author has to settle which it is.
// 003's rule is the filesystem root -- include("/home/me/ships/ship"). But the
// author wrote include("/test_dir/final_test_file.satl") and put that file at
// test_programs/test_dir/, meaning it relative to the program. So the root is
// tried first, and the main file's own directory second, and whichever exists
// wins. That is a fallback, which is not a rule; ERROR/PLAN should carry the
// question until it is answered.
std::string find_file(const std::string &resolved, const std::string &main_file)
{
    std::ifstream first(resolved);
    if (first)
        return resolved;
    if (resolved.empty() || resolved[0] != '/')
        return resolved;
    const std::string root = directory_of(main_file);
    const std::string beside = root.empty() ? resolved.substr(1) : root + resolved;
    std::ifstream second(beside);
    return second ? beside : resolved;
}

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

    std::vector<std::string> waiting{main_file};
    std::vector<std::string> loaded;

    while (!waiting.empty()) {
        const std::string path = waiting.front();
        waiting.erase(waiting.begin());

        bool already = false;
        for (const std::string &done : loaded) already = already || done == path;
        if (already)
            continue;   // a cycle of includes ends here instead of running forever
        loaded.push_back(path);

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
            waiting.push_back(find_file(shape.resolved, main_file));
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
                if (code_at(row, k) == token::string_token) {
                    const std::string argument = text_at(row, k);
                    if (library != nullptr && library->scenarios.text != nullptr)
                        library->scenarios.text(argument, true);
                    else
                        report_error(std::string("satl(run): ") + word::spelling_of(code) +
                                         " has no library built yet",
                                     not_built_yet);
                }
                while (k < row.size() && code_at(row, k) != token::right_parenthesis_token) {
                    if (token::carries_a_count(code_at(row, k))) { text_at(row, k); continue; }
                    ++k;
                }
                if (k < row.size()) ++k;
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
