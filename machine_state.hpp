#pragma once
// The interpreter's current state, shown every time it changes while
// arguments.debug_mode is true:
//
//     [satellite] vector.number.index(loading) (machine_code: 0 success)

#include <string>

namespace satellite004 {

signed long long int display_machine_state(const std::string &current_machine_state,
                                           signed long long int machine_code_input);

struct MachineState {
    bool debug_mode = false;
    std::string current = "satellite(starting)";
    signed long long int code = 0;

    // Record a new state and, in debug mode, display it. Answers the code, so
    // a caller can write `return state.set("...", code);`.
    signed long long int set(const std::string &state, signed long long int machine_code)
    {
        current = state;
        code = machine_code;
        if (debug_mode == true)
            display_machine_state(current, code);
        return code;
    }
};

// A failure is shown whether or not debug mode is on: an error nobody sees is
// an error that did not get reported.
signed long long int report_error(const std::string &what, signed long long int machine_code);

} // namespace satellite004
