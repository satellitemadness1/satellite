#pragma once

// reg_test.hpp — the shared harness for the reg test binary.
//
// The test is split across several translation units, and everything more than
// one of them needs is declared here and defined exactly once in reg_test.cpp.
// Two of those things carry a rule worth stating rather than discovering:
//
//   - `failures` is a single count for the whole binary, not one per file. main
//     returns on it after every section has run, so a section that fails does
//     not stop the ones after it from reporting.
//   - `allocations` and `counting` are written by the global operator new that
//     reg_test.cpp defines. That definition must exist in EXACTLY ONE
//     translation unit — two would be a duplicate symbol at link time and there
//     is no way to have one per section — so it stays in the driver and the
//     counters it feeds are reached from here.
//
// Each section of main() is a function of its own, declared below in the order
// main() calls them. Adding a section means declaring it here, defining it in
// the file its topic belongs to, and calling it from main() — in that order,
// because a section nobody calls compiles and passes.

#include "register_file/reg.hpp"

#include <string>

// --- the harness ------------------------------------------------------------

extern int failures;

void check(bool ok, const char *what);
void check_str(const std::string &got, const std::string &want,
               const char *what);

// --- the allocation counter -------------------------------------------------
// Defined in reg_test.cpp alongside the operator new that maintains them. Set
// `counting` around the region under test and read `allocations` after; the
// counter is global because the claim is about the whole path and not about one
// call.

extern long long allocations;
extern bool counting;

// --- the sections, in the order main() runs them ----------------------------

void reg_test_slot_states();
void reg_test_value_round_trips();
void reg_test_inline_allocation();
void reg_test_inline_addition();
void reg_test_frame_windows();
void reg_test_stack_startup_cost();
