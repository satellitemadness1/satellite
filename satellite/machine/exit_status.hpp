#pragma once
// The exit status for a machine code -- never the code cut to 8 bits (ERROR #7,
// brought forward to PLAN M0.5).
//
// A machine code is a signed long long; an exit status is 8 bits. Cut, 256 exits
// 0, and satl-term closes a tab on 0 -- so a failure would read as a clean run
// and take its own explanation off the screen. So:
//
//     0            exits 0
//     1 to 254     exits as itself
//     anything else (255, 256, -1, 4294967298 ...) exits 255, and the full code is
//                  written on stderr first
//
// 255 IS NEVER GIVEN TO A CODE (D0.5.2, recommended in PLAN and taken here; the
// author may rule otherwise, and then it is this one number). machine_codes.hpp
// says so beside the list, so a status of 255 always means "read stderr".

#include "machine_codes.hpp"
#include "machine_state.hpp"

#include <string>

namespace satellite004 {

constexpr int status_for_a_code_that_does_not_fit = 255;

inline int exit_status_of(signed long long int code)
{
    if (code >= 0 && code < status_for_a_code_that_does_not_fit)
        return static_cast<int>(code);
    report_error("satl(exit): machine code " + std::to_string(code) +
                     " does not fit an exit status, which holds 1 to 254, so satl exits 255",
                 code);
    return status_for_a_code_that_does_not_fit;
}

} // namespace satellite004
