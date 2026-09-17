#include "machine_state.hpp"

#include "machine_codes.hpp"
#include "shown.hpp"

#include <iostream>

namespace satellite004 {

signed long long int display_machine_state(const std::string &current_machine_state,
                                           signed long long int machine_code_input)
{
    std::cout << "[satellite] " << shown(current_machine_state) << " (machine_code: "
              << machine_code_input << " " << machine_code_name(machine_code_input) << ")\n";
    if (!std::cout)
        return display_error;
    return success;
}

signed long long int report_error(const std::string &what, signed long long int machine_code)
{
    std::cerr << "[satellite] " << shown(what) << " (machine_code: " << machine_code << " "
              << machine_code_name(machine_code) << ")\n";
    return machine_code;
}

} // namespace satellite004
