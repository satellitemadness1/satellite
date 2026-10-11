// Every satellite.something.something: its 16-bit code, its numbers, its library.
#include "bytecode/function_table.hpp"
#include "bytecode/word_codes.hpp"
#include <cstdio>
using namespace satellite004;
int main() {
    MachineState state; state.debug_mode = false;
    NumberIndex index; index.load("build/satellite-numbers", state);
    FunctionTable functions; functions.build(index, state);
    printf("%-8s %-16s %-52s %s\n", "code", "numbers", "word", "library");
    printf("%.110s\n", "--------------------------------------------------------------------------------------------------------------");
    unsigned have = 0;
    for (token::Code c = word::kFirst; c < word::kFirst + word::kWordsInTable; ++c) {
        unsigned int depth = 0; const int *n = word::numbers_of(c, depth);
        char numbers[64] = {0}; int at = 0;
        for (unsigned i = 0; i < depth; ++i) at += snprintf(numbers + at, sizeof numbers - at, "%s%d", i ? " " : "", n[i]);
        const NumberRow *row = functions[c];
        if (row) ++have;
        printf("%-8u %-16s %-52s %s\n", c, numbers, word::spelling_of(c), row ? "YES" : "");
    }
    printf("\n%u words wired to a 16-bit code, %u have a function, %u owed\n",
           word::kWordsInTable, have, word::kWordsInTable - have);
    return 0;
}
