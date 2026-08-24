#pragma once

// The shared harness of the evaluator test, and one declaration per section.
//
// The test is a single binary built from every .cpp in this folder -- the
// Makefile picks them up with a wildcard -- so a section is just a function in
// a file of its own, named for what it tests. eval_test.cpp owns main() and
// calls the sections below in the order the checks were originally written in;
// keeping that order matters, because a reader following a failure back to the
// source should find the cases in the sequence the output reported them, and
// because a few sections leave state behind on purpose -- the file sections
// share one path on disk, and satellite.directory puts the process back where
// it found it.

#include <string>

// How many checks have failed so far. One counter for the whole binary, not
// one per file: a per-file copy would let a section count its own failures
// while main() still saw zero and printed PASS. Defined in eval_test.cpp.
extern int failures;

// Reports `what` and counts a failure when `ok` is false.
void check(bool ok, const std::string &what);

// A fresh namespace name, so one case's declarations cannot reach another's.
// See the comment on its definition in eval_test.cpp.
std::string fresh_ns();

// Runs `source` and compares everything it printed against `want`.
void check_output(const std::string &source, const std::string &want,
                  const std::string &what);

// Runs `source` expecting it to fail, with `fragment` somewhere in the report.
void check_error(const std::string &source, const std::string &fragment,
                 const std::string &what);

// The sections, in the order main() runs them.

// eval_test_values.cpp
void eval_test_spine();
void eval_test_arithmetic();
void eval_test_number_rendering();
void eval_test_number_methods();
void eval_test_bool_values();

// eval_test_file.cpp
void eval_test_file_basics();

// eval_test_filesystem.cpp
void eval_test_file_new();

// eval_test_file.cpp -- runs after the block above, and on the same path
void eval_test_file_reopen();

// eval_test_filesystem.cpp
void eval_test_directory();
void eval_test_delete();

// eval_test_console.cpp
void eval_test_help();

// eval_test_values.cpp
void eval_test_strings();
void eval_test_comparison();

// eval_test_capsules.cpp
void eval_test_control_flow();

// eval_test_lists.cpp
void eval_test_lists();
void eval_test_list_literals();
void eval_test_list_assignment();

// eval_test_values.cpp
void eval_test_true_and_false();

// eval_test_console.cpp
void eval_test_display_end();

// eval_test_maps.cpp
void eval_test_maps();

// eval_test_console.cpp
void eval_test_display();
void eval_test_pace();

// eval_test_capsules.cpp
void eval_test_errors();
void eval_test_capsules();

// eval_test_size.cpp
void eval_test_size_and_digits();
void eval_test_spacesuit_size();

// eval_test_values.cpp
void eval_test_binary_and_hex();

// eval_test_capsules.cpp
void eval_test_eval_line();

// eval_test_arguments.cpp
void eval_test_arguments();
