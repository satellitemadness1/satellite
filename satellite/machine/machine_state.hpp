#pragma once
// The interpreter's current state, shown every time it changes while
// arguments.debug_mode is true:
//
//     [satellite] vector.number.index(loading) (machine_code: 0 success)

#include "../config/feature_register.hpp"

#include <string>

namespace satellite004 {

signed long long int display_machine_state(const std::string &current_machine_state,
                                           signed long long int machine_code_input);

struct MachineState {
    bool debug_mode = false;
    std::string current = "satellite(starting)";
    signed long long int code = 0;

    // THE FEATURE REGISTER, AND THIS IS WHAT READS THE BITS. The register is
    // composed by `satl --rebuild` into one binary in config.ini and read once at
    // start-up; this is how it reaches the places that test it, because
    // `MachineState &state` is already threaded through the walker, the
    // expression reader and every library call. Nothing new has to be passed.
    //
    // ON MachineState AND NOT A GLOBAL, which is SATELLITE_ARGUMENTS A1's own
    // choice ("Add bool arguments_active = false to MachineState") made for the
    // reason that file gives: there are no globals in the interpreter, and a
    // register a thread could not see the right copy of is a register that turns
    // features on for some threads and not others.
    //
    // READ-ONLY ONCE A PROGRAM IS RUNNING. config/feature_register.hpp's rule 3
    // says why: `arguments.threads_startup` is 1,024 threads, and a value one
    // thread writes while a thousand read it is a data race. Start-up sets this
    // and nothing writes it again.
    FeatureRegister features;

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
