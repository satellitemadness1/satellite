#pragma once
// satellite/satellite_test/test_programs.hpp -- the programs satellite.test runs, built into satl.
//
// Each is a whole satl program. make_test_programs.py writes them into test_programs.cpp from
// programs/manifest.tsv, and test_run.hpp runs one in a child satl. THE CONTRACT EVERY ONE KEEPS:
//   - the first line inside satellite.main is `    satellite.variable.number passes = <N>`, and
//     every repeat count comes from it -- so a run can be made shorter (test_run.hpp) and still
//     be the same program;
//   - stdout carries only its result lines (the start-up block goes to stderr, kept apart), and
//     an answers program prints the same lines at any passes;
//   - no input, no window, no network; files only beside itself, and none left behind.

#include <cstddef>
#include <string>

namespace satellite004 {

enum class TestKind {
    loss,       // prints one number: 547311173 when nothing was lost (satellite.test.loss)
    speed,      // prints one checksum line; its C++ twin must print the same (satellite.test.speed)
    answers,    // prints `ok <capability>` a check, or `WRONG <capability>: ...` (satellite.test.full)
    refused,    // satl must refuse it: exit_code, and phrase in its report (satellite.test.full)
};

struct TestProgram {
    const char *name;        // its file name: what the child is given, and what a report names
    TestKind kind;
    const char *what;        // one line, what it covers -- speed prints it beside its seconds
    const char *text;        // the program
    const char *expected;    // answers: every line a right run prints, exactly -- one a check.
                             // Exactly, because some lines are their own subject and cannot print
                             // WRONG: a display line, a thread's, a console's (programs/full/)
    long long exit_code;     // refused: the code satl stops it with
    const char *phrase;      // refused: words its report must hold -- and it prints nothing
    std::string (*twin)(long long passes);   // speed: the same work in C++, its checksum line
};

extern const TestProgram kTestPrograms[];
extern const std::size_t kTestProgramCount;

} // namespace satellite004
