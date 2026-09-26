#pragma once

// The shared harness for the format test binary, and one declaration per
// section it runs. The section bodies live one file per topic --
// format_test_kinds.cpp, format_test_registry.cpp, format_test_selectors.cpp,
// format_test_paths.cpp and format_test_ops.cpp -- while format_test.cpp holds
// the check and check_str pair, the failure counter, the printed figures and
// main().
//
// This header includes format.hpp rather than leaving that to each section,
// because including it is the larger half of the test: the static_asserts at
// the bottom of format.hpp fire at COMPILE time, so every translation unit in
// this binary pays for the structural invariants, and a file that fails to
// build IS the test failing. It is also the only thing this binary includes
// from src, and that is a property worth keeping -- format.hpp links against
// nothing, so nothing here may pull the interpreter in behind it.

#include "bytecode_format/format.hpp"

// ONE counter for the whole binary. It stopped being static when the sections
// moved into their own translation units: main() reads it after the last
// section has run, so there has to be exactly one of it. Defined in
// format_test.cpp, named here so every section can reach the same object -- a
// per-file copy would leave a failing check invisible to the exit status.
extern int failures;

void check(bool ok, const char *what);
void check_str(const char *got, const char *want, const char *what);

// The sections, in the order main() runs them. That order is not incidental:
// each one reads the tables the ones before it have already vouched for, so a
// break in the kind space is reported before the selector counts that assume
// it. Adding a section means a declaration here and a call in main().
void test_kinds();
void test_registry();
void test_selectors();
void test_paths();
void test_variadic();
void test_ops();
