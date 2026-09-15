#pragma once
// The vector number index: every numbered library, loaded into memory once at
// start-up, and found by name before a program runs -- never while it runs.

#include "number_row.hpp"

#include <string>
#include <vector>

namespace satellite004 {

struct MachineState;

struct NumberRow {
    std::string name;                               // "satellite.console.display"
    std::vector<unsigned long long int> numbers;    // {1, 5, 1}
    Scenarios scenarios;
    std::string file;                               // the library it came from
};

class NumberIndex {
public:
    // Load every `*.so` in `folder`. Answers number_vector_defined, or
    // vector_loading_error when the folder or a library could not be read.
    signed long long int load(const std::string &folder, MachineState &state);

    // Before a program runs: the row for a name, or nullptr.
    const NumberRow *find(const std::string &name) const;

    const std::vector<NumberRow> &rows() const { return rows_; }

private:
    std::vector<NumberRow> rows_;
};

// "1 5 1"
std::string numbers_text(const std::vector<unsigned long long int> &numbers);

} // namespace satellite004
