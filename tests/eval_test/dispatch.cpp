// `handlers[path_id]`, the receiver-binding tag, and the inline cache.
//
// THE TABLE IS EMPTY IN `satl` AT M9 AND THIS IS ITS CONSUMER. PLAN §8's path
// ledger gives M9 none of the 223 numbered paths -- "M5 and M9 build the
// machinery every other row dispatches through" -- so the first real rows are
// M10's console. PLAN M2's rule is that a registry gets a reader in the
// milestone that writes it, and a test binary installing its own handlers is
// that reader: it is the only place the mechanism can be exercised before
// anything owns a row.
//
// AND THE TABLE IS PROCESS-WIDE, SO EVERY FIXTURE PUTS IT BACK. A test that
// passes in one order and fails in another is worse than one that fails.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"

#include <string>

namespace eval_test {

namespace {

int calls_made = 0;

bool count_and_answer(satellite::eval::Machine &, const satellite::Value *arguments,
                      uint32_t count, satellite::Value *answer)
{
    calls_made++;
    if (count == 1 && arguments[0].is_number())
        *answer = satellite::Value::number(
            satellite::Number::add(std::get<satellite::Number>(arguments[0]),
                                   satellite::Number(1)));
    else
        *answer = satellite::Value::nothing();
    return true;
}

bool always_refuses(satellite::eval::Machine &machine, const satellite::Value *,
                    uint32_t, satellite::Value *)
{
    // DESIGN §9.1: a handler that cannot do what it was asked reports through
    // the machine and answers false. It does not throw -- 8.5 ns returned as an
    // enum against 1537 ns thrown -- and it does not answer nullptr.
    machine.refuse(satellite::errors::make<satellite::errors::Code::EVAL_NOT_A_NUMBER>(
        machine.span_of(machine.here()), "a test handler", "nothing at all"));
    return false;
}

} // namespace

void section_dispatch()
{
    using namespace satellite;

    eval::Handlers &table = eval::Handlers::table();
    table.clear();
    calls_made = 0;

    // `satellite.console.display` `1 5 1` IS M10's ROW AND IS BORROWED HERE.
    // What is being checked is the mechanism, so the path only has to be a real
    // language path with a number -- and using one nobody has built yet is what
    // keeps this fixture from pinning a behaviour M10 gets to choose.
    const words::PathId display =
        static_cast<words::PathId>(words::NodeId::CONSOLE_DISPLAY);

    table.install(display, {count_and_answer, false, 1, "a test, not a milestone"});
    check(table.installed() == 1, "a handler installs into its own row");
    check(table.find(display) != nullptr, "and is found by its PathId");
    check(table.find(display)->binds_receiver == false,
          "DESIGN §6.4 qualification 2's tag is carried -- it is the only "
          "reason `satellite.file.new` `1 8 1` and `satellite.variable.file.new` "
          "`1 6 2 1` can coexist, and WORD_NUMBERS §4 calls that pair the kind "
          "of thing that gets decided by accident at M16");

    {
        Run run;
        build("satellite.capsule it(satellite.variable.number n)\n"
              "{\n"
              "    satellite.return(satellite.console.display(n))\n"
              "}\n",
              run);
        check(run.built, "a call through the table compiles");
        check(answer_of(run, "it", {41}) == "42", "and dispatches to the handler");
        check(calls_made == 1, "exactly once");

        // THE INLINE CACHE -- PLAN §2.4. "The second execution of a call site
        // does no lookup at all." What can be checked from outside is that the
        // answer does not change once the cell is filled, and that a call site
        // got a cell at compile time rather than on first execution -- which is
        // what keeps the op arena immutable for DESIGN §10.5's threads.
        check(run.program.closures.caches() == 1,
              "the call site was given a cache cell at COMPILE time, so the op "
              "stays a POD and the mutable half is a side table");
        check(answer_of(run, "it", {41}) == "42", "a second run answers the same");
    }

    // --- the arity is checked before the handler runs ------------------------
    {
        calls_made = 0;
        Run run;
        build("satellite.capsule it()\n"
              "{\n"
              "    satellite.return(satellite.console.display(1, 2))\n"
              "}\n",
              run);
        call(run, "it", {});
        check(ran_into(errors::Code::EVAL_ARGUMENT_COUNT),
              "S0722: PLAN §2.3's 'how many arguments, already checked'");
        check(calls_made == 0, "and the handler was never entered");
    }

    // --- a handler that refuses ---------------------------------------------
    {
        table.install(display, {always_refuses, false, eval::kAnyArity, "a test"});
        Run run;
        build("satellite.capsule it()\n"
              "{\n"
              "    satellite.return(satellite.console.display(1))\n"
              "}\n",
              run);
        call(run, "it", {});
        check(last_ending == eval::Ending::Refused,
              "DESIGN §9.1: a handler reports through the machine and answers "
              "false -- it does not throw and does not return nullptr");
    }

    // PUT IT BACK. See the file note.
    table.clear();
    check(table.installed() == 0, "the table is left as it was found");
}

} // namespace eval_test
