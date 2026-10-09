// Runs a program STRAIGHT OUT OF bytecode_registry: no AST, no per-call object.
// A call is a POSITION in the row; the arguments are the codes between ( and ).
#include "bytecode/bytecode_registry.hpp"
#include "bytecode/function_table.hpp"
#include "bytecode/word_codes.hpp"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

using namespace satellite004;
using token::Code;

// The whole machine. One index into one row -- nothing is allocated to run a line.
static signed long long int run_row(const std::vector<std::bitset<16>> &row,
                                    const FunctionTable &functions)
{
    for (std::size_t at = 0; at < row.size(); ) {
        const Code code = static_cast<Code>(row[at].to_ulong());

        if (!word::is_word_code(code)) {          // not a call: step over it
            if (token::carries_a_count(code)) { std::size_t k = at; text_at(row, k); at = k; continue; }
            ++at;
            continue;
        }

        const NumberRow *library = functions[code];
        const char *spelling = word::spelling_of(code);
        ++at;

        if (at < row.size() && static_cast<Code>(row[at].to_ulong()) == token::left_parenthesis_token) {
            ++at;
            if (at < row.size() && static_cast<Code>(row[at].to_ulong()) == token::string_token) {
                const std::string argument = text_at(row, at);
                if (library == nullptr || library->scenarios.text == nullptr) {
                    printf("  %s(\"%s\") -- numbered, no library built yet (machine code 14)\n",
                           spelling, argument.c_str());
                } else {
                    printf("  %s -> table[%u] -> %s\n     the program prints: ",
                           spelling, static_cast<unsigned>(code), library->file.c_str());
                    fflush(stdout);
                    library->scenarios.text(argument, true);
                }
            }
            while (at < row.size() && static_cast<Code>(row[at].to_ulong()) != token::right_parenthesis_token) ++at;
            if (at < row.size()) ++at;
        }
    }
    return success;
}

int main(int argc, char **argv)
{
    MachineState state;
    state.debug_mode = false;
    StartupThreads threads;
    threads.start(64, state);

    NumberIndex index;
    const std::string folder = argc > 1 ? argv[1] : "build/satellite-numbers";
    if (stops_the_program(index.load(folder, state))) {
        printf("could not load %s -- run `make` first, or pass the folder\n", folder.c_str());
        return 1;
    }

    FunctionTable functions;
    functions.build(index, state);
    printf("function table: %llu of %u words have a library, %u slots (%zu KB)\n\n",
           functions.filled(), word::kWordsInTable,
           word::kLast - word::kBase + 1,
           (word::kLast - word::kBase + 1) * sizeof(void *) / 1024);

    const std::string path = argc > 2 ? argv[2] : "test_programs/hello_world.satl";
    std::ifstream file(path);
    if (!file) { printf("cannot read %s\n", path.c_str()); return 1; }
    std::ostringstream held;
    held << file.rdbuf();
    const std::string source = held.str();

    BytecodeRegistry registry;
    BytecodeFilenames filenames;
    build_bytecode_registry(path, source, threads, 64, registry, filenames, state);
    printf("%s: %zu codes\n\n", filenames[0].c_str(), registry[0].size());

    return static_cast<int>(run_row(registry[0], functions));
}
