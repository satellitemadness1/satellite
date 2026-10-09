#pragma once
// satellite/satellite_test/test_run.hpp -- one of satellite.test's programs, run in a child satl.
//
// A CHILD, AS THE PROMPT RUNS A FILE (satl/prompt_run.cpp): `satl --run <file>`, this same
// binary, so the program runs exactly as `satl file.satl` runs it -- its own main, its own
// names, its own refusals -- and a program that crashes cannot take the test with it.
//
// IN A PLACE OF ITS OWN (TestPlace): a test makes one folder when it starts (mkdtemp, under
// $TMPDIR or /tmp) and removes it when it ends. In it:
//   home/        the HOME every child of the test is given: a config.ini of satl's built-in
//                defaults, made by `satl --rebuild` as check.sh makes its own -- so a setting of
//                the person's cannot make a right satl look wrong (their infinity_display = 8
//                turned a right answer WRONG in the review, 2026-10-06), none of their rows
//                reaches a child (an absolute log_path sent 124 refusals a round into their
//                log), and the refusals the refused programs exist to make land in
//                home/.satl/satellite.log, which goes with the folder. Nothing a child does can
//                write the person's config.ini either. One home for the whole test, so a setting
//                that leaked out of one run would show in the next (full_structure_3 looks).
//   run/         the folder each program is written into, empty but for it, and the child's
//                working directory -- what it writes beside itself lands there, and it is
//                emptied before the next program.
//   errors       the child's stderr, read once it has ended.
// Every child is also told SATL_NO_WINDOW=1, as the programs' writers ran them.
//
// ITS OUTPUT AND ITS ERRORS APART (program_spawn.hpp's err): stdout on a pipe -- which also
// keeps the child from opening a console of its own (window_run.cpp) -- and stderr into the
// errors file. And what it wrote to satellite.log, which a refused program must and nothing
// else may: a warning goes there and nowhere else (critical_report.hpp's log_only_warning).
//
// IT CAN ALWAYS BE STOPPED. The wait is a poll, a tenth of a second at a time, and between them
// it asks whether to stop: a Ctrl-C (the prompt's stop flag, or CtrlCDuringATest's below), the
// thread's own stop() when a test runs on a thread, the whole program ending -- or the program
// running past its limit. Then the child and everything it started are killed, so nothing is
// left running and the place can go.
//
// TIMED FROM OUTSIDE, steady_clock from before the start to after the wait: the seconds a
// person waited for it, start-up included.

#include "test_programs.hpp"

#include <string>

namespace satellite004 {

struct TestRun {
    bool started = false;    // false: `why` says what stopped it before it ran
    std::string why;
    std::string out;         // its stdout
    std::string err;         // its stderr: the start-up block, and a report when it stopped on one
    std::string logged;      // what it added to satellite.log
    long long code = 0;      // its exit status; 128 + the signal when a signal ended it
    bool interrupted = false;   // a Ctrl-C, a stop() or the program's end stopped it -- the test stops
    bool timed_out = false;     // it ran past its limit, and was stopped
    double seconds = 0.0;
};

// THE FOLDER A TEST RUNS ITS PROGRAMS IN, made when it is made and gone when it goes.
class TestPlace {
public:
    TestPlace();
    ~TestPlace();
    TestPlace(const TestPlace &) = delete;
    TestPlace &operator=(const TestPlace &) = delete;

    // `passes` 0 runs the program as written; anything else is written into its passes line.
    // A program still running after `limit` seconds is stopped, and the run says so.
    TestRun run(const TestProgram &program, long long passes, double limit);

private:
    std::string folder_;   // "" when it could not be made: why_ says why, and every run says it
    std::string why_;
};

// A CTRL-C WHILE A TEST RUNS FROM A FILE. A file run leaves SIGINT as it is, so a Ctrl-C would
// end satl on the spot -- with the test's place on the disk and its child running on (the
// review, 2026-10-06). So while one of these lives, a Ctrl-C only asks the test to stop; when
// the last one goes, the place is gone, and satl ends as a Ctrl-C ends it. At the prompt it does
// nothing: the prompt's own Ctrl-C (the stop flag) is already asked.
class CtrlCDuringATest {
public:
    CtrlCDuringATest();
    ~CtrlCDuringATest();
    CtrlCDuringATest(const CtrlCDuringATest &) = delete;
    CtrlCDuringATest &operator=(const CtrlCDuringATest &) = delete;

private:
    bool installed_ = false;
};

// WHETHER A TEST SHOULD STOP NOW: a Ctrl-C, this thread's stop(), or the program's end.
bool test_stop_wanted();

// SATL_TEST_PASSES, when it holds a whole number above 0: every program runs that many passes,
// and full() runs its set once. check.sh's quick run (a whole test is ~60 s by design);
// 0 when it is not set.
long long test_passes_override();

} // namespace satellite004
