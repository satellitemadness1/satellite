#pragma once
// The vector number index: every word built into satl, filed once at start-up and found by name
// before a program runs -- never while it runs.
//
// UNTIL 2026-10-07 THIS LOADED .so FILES: every numbered library beside the binary, dlopened and
// asked to describe itself. The author that day: "We were supposed to have built the
// satellite-numbers directly into the interpreter, so they are not external" -- so the libraries
// are files of satl's own now (satellite-numbers/words/), and the table that calls each one is
// satellite-numbers/word_table.hpp. The 33 arguments words went first (2026-10-03).

#include "number_row.hpp"

#include <string>
#include <vector>

namespace satellite004 {

struct MachineState;

struct NumberRow {
    std::string name;                               // "satellite.console.display"
    std::vector<unsigned long long int> numbers;    // {1, 5, 1}
    Scenarios scenarios;
    std::string file;                               // "(built into satl)" -- every row, since 2026-10-07
};

class NumberIndex {
public:
    // File every word built into satl: the arguments words (argument_words.hpp), then the table
    // (word_table.hpp). Answers number_vector_defined, or vector_loading_error when a word
    // described itself wrongly, twice, or not as words/words.tsv has it.
    signed long long int load(MachineState &state);

    // Before a program runs: the row for a name, or nullptr.
    const NumberRow *find(const std::string &name) const;

    const std::vector<NumberRow> &rows() const { return rows_; }

private:
    std::vector<NumberRow> rows_;
};

// "1 5 1"
std::string numbers_text(const std::vector<unsigned long long int> &numbers);

} // namespace satellite004
