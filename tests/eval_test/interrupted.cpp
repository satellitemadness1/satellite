// Ctrl-C, as the machine sees it -- M11's fourth Ending, proved without a
// signal ever being raised.
//
// THE FLAG IS HANDED IN, WHICH IS WHAT MAKES THIS TESTABLE AT ALL. Policy
// carries `interrupted` as a function pointer for the same reason it carries
// the ceiling as a number: `satl` wires system_facts/interrupt.hpp in,
// tests hand in a counter, and the machine cannot tell the difference --
// which is the claim. A fixture that had to fork, sleep and kill would be
// timing-dependent and would be testing the KERNEL's delivery; what M11
// promises is the machine's half, "the walk stops itself at the next
// statement", and that half is deterministic.
//
// (The signal half -- no SA_RESTART, the escalation, exit 130 from a real
// SIGINT -- is v1's code ported whole, and the real-terminal run is in
// MILESTONES/M11.md §5 with its transcript.)

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"

#include <string>

namespace eval_test {

namespace {

// True after the Nth ask. Statement boundaries ask in order, so "the flag
// rose during iteration k" is a thing a fixture can say exactly.
int asks_before_interrupt = 0;
int asks_so_far = 0;

bool interrupt_after_n()
{
    asks_so_far++;
    return asks_so_far > asks_before_interrupt;
}

satellite::eval::Ending run_interrupted(const std::string &source,
                                        const std::string &capsule, int after,
                                        std::vector<satellite::errors::Diagnostic> *problems)
{
    using namespace satellite;

    Run run;
    build(source, run);
    if (!run.built) {
        check(false, "an interrupt fixture failed to compile");
        return eval::Ending::Refused;
    }

    words::PathId path = run.words.find(words::NodeId::LIBRARY, capsule);
    if (path == words::kNoPath)
        path = run.words.find(words::NodeId::SATELLITE, capsule);
    const int which = path == words::kNoPath ? -1 : run.program.find(path);
    if (which < 0) {
        check(false, "an interrupt fixture declares no capsule called `" + capsule + "`");
        return eval::Ending::Refused;
    }

    asks_before_interrupt = after;
    asks_so_far = 0;

    eval::Policy policy;
    policy.max_depth = 1024ull * 1024 * 1024;
    policy.interrupted = interrupt_after_n;

    eval::Machine machine(run.program.closures, run.parsed.ast, policy);
    machine.run_top_level();
    if (machine.ok())
        machine.call(static_cast<uint32_t>(which), {});
    if (problems != nullptr)
        *problems = machine.problems();

    // THE ONE PLACE ALL FOUR ENDINGS CAN BE TOLD APART, so the checks below
    // ask the machine and not a rendering of it.
    check(machine.ok() == (machine.ending() == eval::Ending::Finished),
          "ok() is Finished and nothing else");
    return machine.ending();
}

bool says(const std::vector<satellite::errors::Diagnostic> &problems,
          satellite::errors::Code code)
{
    for (const satellite::errors::Diagnostic &at : problems)
        if (at.code == code)
            return true;
    return false;
}

} // namespace

void section_interrupted()
{
    using namespace satellite;

    // --- a while loop that would never end, ended ----------------------------
    {
        std::vector<errors::Diagnostic> problems;
        const eval::Ending ending = run_interrupted(
            "satellite.capsule it()\n{\n"
            "    satellite.variable.number n = 0\n"
            "    satellite.statement.while (n < n + 1)\n    {\n"
            "        n = n + 1\n    }\n"
            "    satellite.return(n)\n}\n",
            "it", 25, &problems);
        check(ending == eval::Ending::Interrupted,
              "a `while` with no way out ends as Interrupted -- the first walk "
              "in this tree long enough to be stopped, stopped");
        check(says(problems, errors::Code::EVAL_INTERRUPTED),
              "S0730 is the sentence, with the line the run was on");
    }

    // --- Interrupted is its own ending and not a costume ---------------------
    {
        Run run;
        build("satellite.capsule it()\n{\n"
              "    satellite.statement.for (;;)\n    {\n    }\n"
              "    satellite.return(1)\n}\n",
              run);
        check(run.built, "`for (;;)` compiles -- the language's own forever");

        words::PathId path = run.words.find(words::NodeId::LIBRARY, "it");
        const int which = path == words::kNoPath ? -1 : run.program.find(path);

        asks_before_interrupt = 3;
        asks_so_far = 0;
        eval::Policy policy;
        policy.max_depth = 1024ull * 1024 * 1024;
        policy.interrupted = interrupt_after_n;
        eval::Machine machine(run.program.closures, run.parsed.ast, policy);
        machine.run_top_level();
        if (which >= 0 && machine.ok())
            machine.call(static_cast<uint32_t>(which), {});

        check(machine.ending() == eval::Ending::Interrupted,
              "a `for` with no condition is ended by the same boundary");
        check(!machine.ok() && !machine.at_the_ceiling(),
              "and it is neither Finished nor Stopped -- PLAN §8's M10 entry "
              "named the trap: without a fourth Ending a Ctrl-C reads as a "
              "machine limit, and a script testing for 4 reads a person's "
              "hand as a memory ceiling");
    }

    // --- the boundary is between statements, so finished work stands ---------
    {
        std::vector<errors::Diagnostic> problems;
        const eval::Ending ending = run_interrupted(
            "satellite.capsule it()\n{\n"
            "    satellite.variable.number n = 1\n"
            "    satellite.return(n + 1)\n}\n",
            "it", 1000, &problems);
        check(ending == eval::Ending::Finished,
              "a flag that never rises costs a program nothing but the asks");
    }

    // --- nobody listening is the default and costs nothing -------------------
    {
        Run run;
        build("satellite.capsule it()\n{\n"
              "    satellite.variable.number n = 0\n"
              "    satellite.statement.while (n < 100)\n    {\n"
              "        n = n + 1\n    }\n"
              "    satellite.return(n)\n}\n",
              run);
        check(run.built && answer_of(run, "it", {}) == "100",
              "a null `interrupted` -- the Policy default, and what every "
              "other section runs under -- never stops a walk");
    }
}

} // namespace eval_test
