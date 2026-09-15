// THE MILESTONE. DESIGN §7.5: the language has no depth limit, and until M8.5
// four static passes broke it. This is the same claim for the evaluator.
//
// EVERY FIXTURE HERE RUNS AGAINST 8 MiB OF C++ STACK, because 065-tests.mk does
// not name machine_limits among this binary's sources and so nothing raises
// RLIMIT_STACK. MILESTONES/M8.5.md §4.1 is why that is written down rather than
// left to be true by accident: a 20,000-deep resolve fixture passed for a day
// against a raise it had never been given, and a test run against the 1.9 GiB
// `satl` asks for would prove nothing at all.
//
// AND THE NUMBERS ARE DELIBERATELY ABSURD, which is NO_LIMITS §7's rule. A
// 100,000-deep recursion is machine-generated and no person will write one; the
// point is that the interpreter's answer does not depend on who generated the
// file.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "satellite_value/render.hpp"

#include <string>

namespace eval_test {

namespace {

// A capsule that calls itself `n` times and answers 0. The body is deliberately
// cheap -- one comparison and one subtraction -- because what is being measured
// is the stack and not the arithmetic.
const char *kCountDown = R"(
satellite.capsule deep(satellite.variable.number n)
{
    satellite.statement.if (n < 1)
    {
        satellite.return(0)
    }
    satellite.return(deep(n - 1))
}
)";

// `1 + 1 + 1 + ...`, which DESIGN §6.6 makes left-associative -- so the tree is
// a chain `n` deep and the evaluator has to walk all of it before the first
// addition can happen.
std::string long_sum(int terms)
{
    std::string out = "satellite.capsule chain()\n{\n    satellite.return(1";
    for (int i = 1; i < terms; i++)
        out += " + 1";
    out += ")\n}\n";
    return out;
}

std::string many_iterations(long long rounds)
{
    return "satellite.capsule loop()\n"
           "{\n"
           "    satellite.variable.number i = 0\n"
           "    satellite.statement.while (i < " + std::to_string(rounds) + ")\n"
           "    {\n"
           "        i = i + 1\n"
           "    }\n"
           "    satellite.return(i)\n"
           "}\n";
}

} // namespace

void section_depth()
{
    using namespace satellite;

    // --- a recursion no C++ stack could hold ---------------------------------
    {
        Run run;
        build(kCountDown, run);
        check(run.built, "the count-down fixture compiles");

        check(answer_of(run, "deep", {10}) == "0", "ten deep answers");

        // 100,000 FRAMES. On the C++ stack this is where every walker in this
        // tree died before M8.5 -- NO_LIMITS §2.4's table starts its segfaults
        // at 19,000 -- and the frames here are the evaluator's own, on the heap.
        check(answer_of(run, "deep", {100000}) == "0",
              "DESIGN §7.5: 100,000 frames deep answers, at 8 MiB of C++ stack");
        check(last_ending == eval::Ending::Finished,
              "and it FINISHED rather than being refused");

        // A million, because 100,000 could still be a large fixed number
        // somewhere. NO_LIMITS §4.1.1 is the argument this fixture is built
        // against: "it is a bigger number and not the absence of one."
        check(answer_of(run, "deep", {1000000}) == "0",
              "a million frames deep answers too");
    }

    // --- the ceiling, and it refuses in words about RECURSION ----------------
    {
        Run run;
        build("satellite.capsule forever(satellite.variable.number n)\n"
              "{\n"
              "    satellite.return(forever(n + 1))\n"
              "}\n",
              run);
        check(run.built, "the runaway fixture compiles");

        call(run, "forever", {1}, 8ull * 1024 * 1024);

        check(ran_into(errors::Code::EVAL_TOO_DEEP),
              "S0701: a runaway recursion is refused at max_depth");
        check(last_ending == eval::Ending::Stopped,
              "and it STOPPED rather than being called malformed -- the program "
              "is correct and asked for more than it was allowed");

        // THE SENTENCE IS THE POINT AND NOT THE STOP. M6's watchdog would have
        // ended this run too; what it could not say is what was growing.
        // SCRATCH.md/NO_LIMITS.md §8's first question is exactly that gap.
        check(!last_problems.empty(), "it said something");
        if (!last_problems.empty()) {
            const std::string said = errors::sentence(last_problems.front());
            check(holds(said, "recursion"),
                  "the sentence names RECURSION, which is what a memory ceiling "
                  "one layer out cannot do");
            check(holds(said, "max_depth"),
                  "and it names the dial the user can turn");
            check(!last_problems.front().frames.empty(),
                  "DESIGN §9's fourth field is filled -- M9 is FrameRef's first "
                  "real producer, and report.hpp said so at M5");
        }
    }

    // --- a loop is not a depth ----------------------------------------------
    //
    // NO_LIMITS §2.2 IN A FIXTURE. "A loop costs zero stack depth -- the frame
    // is reused every iteration." The answers below are different and the peak
    // is the SAME, which is the only way to check a claim about a stack from
    // outside.
    {
        Run few;
        build(many_iterations(10), few);
        check(answer_of(few, "loop", {}) == "10", "ten rounds answer 10");
        const unsigned long long small = last_peak;

        Run many;
        build(many_iterations(100000), many);
        check(answer_of(many, "loop", {}) == "100000",
              "a hundred thousand rounds answer 100000");
        check(last_peak == small,
              "and cost the SAME control stack -- a loop reuses one work item, "
              "so NO_LIMITS §2.2's 'a loop costs zero stack depth' is a fact "
              "about this machine and not only about the first satellite's");
    }

    // --- an expression with no capsule call in it at all ---------------------
    {
        Run run;
        build(long_sum(100000), run);
        check(run.built, "a 100,000-term sum compiles");
        check(answer_of(run, "chain", {}) == "100000",
              "and evaluates -- the expression tree is 100,000 deep and the "
              "walk is on the heap");
    }
}

} // namespace eval_test
