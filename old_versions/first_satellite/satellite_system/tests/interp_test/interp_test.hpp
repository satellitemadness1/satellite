#pragma once

// The shared harness of the interp test, and one declaration per section.
//
// The test is a single binary built from every .cpp in this folder -- the
// Makefile picks them up with a wildcard -- so a section is just a function in
// a file of its own, named for what it tests. interp_test.cpp owns main() and
// calls the sections below in the order the checks were originally written in;
// keeping that order matters, because a reader following a failure back to the
// source should find the cases in the sequence the output reported them.

#include <string>
#include <vector>

// How many checks have failed so far. One counter for the whole binary, not
// one per file: a per-file copy would let a section count its own failures
// while main() still saw zero and printed PASS. Defined in interp_test.cpp.
extern int failures;

// Reports `what` and counts a failure when `ok` is false.
void check(bool ok, const std::string &what);

// Runs a whole program with `args` bound to satellite.main's parameter and
// compares everything it displayed against `want`, printing both when they
// differ so the diff is readable from the test output alone.
void check_run(const std::string &source, const std::vector<std::string> &args,
               const std::string &want, const std::string &what);

// A fresh namespace name, so one case's declarations cannot reach another's.
// See the comment on its definition in interp_test.cpp.
std::string fresh_ns();

// The sections, in the order main() runs them.

// interp_test_arguments.cpp
void interp_test_argument_conversion();
void interp_test_main_capsule();

// interp_test_exit_status.cpp
void interp_test_exit_status();

// interp_test_run_file.cpp
void interp_test_run_file();
void interp_test_error_names_file();

// interp_test_multiline_entry.cpp
void interp_test_multiline_entry();
void interp_test_prompt_session();

// interp_test_run_command.cpp
void interp_test_run_command();
void interp_test_run_command_end_to_end();

// interp_test_console_pacing.cpp
void interp_test_console_pacing();
