// satellite/bytecode/function_table.cpp -- see the header for why the code is
// the index and not a name to look up.

#include "function_table.hpp"

#include "../machine/machine_codes.hpp"

#include <string>

namespace satellite004 {

signed long long int FunctionTable::build(const NumberIndex &index, MachineState &state)
{
    rows_.assign(static_cast<std::size_t>(word::kLast - word::kBase) + 1, nullptr);
    filled_ = 0;

    // BY SPELLING, ONCE, HERE. The word table and the libraries agree on a
    // word's name, so the name is what joins them -- and doing it now, at
    // start-up, is exactly what keeps names out of the running program.
    for (token::Code code = word::kFirst; code < word::kFirst + word::kWordsInTable; ++code) {
        const char *spelling = word::spelling_of(code);
        if (spelling == nullptr) continue;
        const NumberRow *row = index.find(spelling);
        if (row == nullptr) continue;
        rows_[static_cast<std::size_t>(code) - word::kBase] = row;
        ++filled_;
    }

    state.set("function_table(built): " + std::to_string(filled_) + " of " +
                  std::to_string(word::kWordsInTable) + " words have a library",
              success);
    return success;
}

} // namespace satellite004
