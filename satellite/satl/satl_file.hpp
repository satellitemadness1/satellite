#pragma once
// A .satl file: loaded, checked for the three lines every program needs, turned
// into a list of calls whose functions are chosen BEFORE anything runs, and run.
//
// This first runner understands one statement, `satellite.console.display(x)`,
// where x is a string literal, a whole number, or satellite.bool.true/false.
// Any other line is satl_line_not_understood (13), with its line number.

#include "../arguments/arguments.hpp"
#include "../../satellite-numbers/call_number.hpp"

#include <string>
#include <vector>

namespace satellite004 {

struct MachineState;

struct Call {
    const NumberRow *row = nullptr;
    ArgumentKind kind = ArgumentKind::text;
    std::string text;
    unsigned long long int count = 0;
    bool flag = false;
    unsigned int line = 0;
};

// missing_satl_file (8) or successfully_loaded_satl_file (9).
signed long long int load_satl(const std::string &path, std::string &source, MachineState &state);

// success, or 10 / 11 / 12 for the first required line that is missing.
signed long long int check_satl(const std::string &source, MachineState &state);

// Choose every call's function now. success, int_error, string_error or 13.
signed long long int compile_satl(const std::string &source, const NumberIndex &index,
                                  std::vector<Call> &calls, MachineState &state);

// Run the calls in order. success or the first failing call's code.
signed long long int run_calls(const std::vector<Call> &calls, MachineState &state);

} // namespace satellite004
