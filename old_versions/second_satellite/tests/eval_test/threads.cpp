// M23's rows: the deferred call, the three verbs, and the five refusals.
//
// THE REFUSALS ARE THE REASON THIS FILE EXISTS. Each of S1401 through S1405
// stops the run, so `example/threads.satl` can demonstrate at most one of them
// and demonstrates none; a fixture can raise all five because it builds a fresh
// program per clause. That is the same division of labour M13's section draws
// between the tier refusals here and the distribution claims in a program.
//
// AND THE CONCURRENCY CLAIMS ARE NOT HERE, for the mirror-image reason. DESIGN
// §7.1's 1600-of-1600 is eight threads racing over a real program and belongs
// where a reader can see the program: `example/threads.satl` §3. What is here
// is everything a single walk can settle.
//
// THE TABLE IS PROCESS-WIDE, SO THIS SECTION PUTS IT BACK EMPTY -- the same
// contract every install-the-real-rows section keeps.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "error_reporter/warning_log.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_thread/handlers.hpp"
#include "satellite_thread/thread_handle.hpp"
#include "satellite_time/handlers.hpp"

#include <cstdio>
#include <string>
#include <unistd.h>
#include <vector>

namespace eval_test {

namespace {

using satellite::errors::Code;

// A whole program with one capsule to thread and one `it` to run it from.
std::string program(const std::string &body, const std::string &more = "")
{
    return more + "satellite.capsule work()\n"
           "{\n"
           "    satellite.return(7)\n"
           "}\n"
           "satellite.capsule twice(satellite.variable.number n)\n"
           "{\n"
           "    satellite.return(n + n)\n"
           "}\n"
           "satellite.capsule it()\n{\n" +
           body + "}\n";
}

std::string answers(const std::string &body, const std::string &more = "")
{
    Run run;
    build(program(body, more), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

bool refused_with(const std::string &body, Code code)
{
    Run run;
    build(program(body), run);
    if (!run.built)
        return raised(run, code);
    call(run, "it", {});
    return ran_into(code);
}

// --- the deferred call ------------------------------------------------------

void a_call_is_packaged_and_not_performed()
{
    // THE WHOLE OF DESIGN §13, AS ONE ANSWER. `work()` returns 7. If `new`
    // performed the call, this would be a thread built over the number 7 --
    // which is S1401 -- and if it packages it, the value is a capsule. Asking
    // the TYPE is what separates the two, and type_name is the same word every
    // S07xx refusal prints.
    check(holds(answers("satellite.variable.thread t = "
                        "satellite.thread.new(work())\n"
                        "satellite.return(t)\n"),
                "<thread work, not started>"),
          "new() answers a thread that has not started, over a capsule that "
          "has not run -- DESIGN §13's packaging");

    // AND THE ARGUMENTS ARE EVALUATED, which is the other half of §13's
    // sentence and is a separate claim. A refusal INSIDE an argument has to
    // happen at `new`, on this thread, because that is where the argument is
    // worked out -- so a division by zero in there is S0601 now and not a
    // thread that fails later.
    check(refused_with("satellite.variable.number zero = 0\n"
                       "satellite.variable.thread t = "
                       "satellite.thread.new(twice(1 / zero))\n",
                       Code::NUMBER_DIVIDE_BY_ZERO),
          "an argument to a packaged call is evaluated where it is written, "
          "so its refusal is raised there");
}

void a_packaged_call_checks_its_arity()
{
    // S0722 AND NOT A CODE OF ITS OWN -- errors.def's S14xx block note is the
    // argument: packaging a call asks the same question about the same capsule
    // that performing one does.
    check(refused_with("satellite.variable.thread t = "
                       "satellite.thread.new(work(1, 2))\n",
                       Code::EVAL_ARGUMENT_COUNT),
          "a packaged call with the wrong number of arguments is S0722, the "
          "same code op_call raises");
}

void only_a_capsule_of_your_own_can_be_threaded()
{
    // CAUGHT AT COMPILE AND NOT AT RUN, which is what makes this the important
    // clause in the file. `display` inside `new` must NOT print: falling
    // through to op_dispatch would perform the call at the moment `new` ran,
    // which is the one outcome worse than a refusal.
    check(refused_with("satellite.variable.thread t = "
                       "satellite.thread.new(satellite.console.display(\"x\"))\n",
                       Code::THREAD_NOT_A_CAPSULE_CALL),
          "a language call inside new() is S1401 and is refused before it can "
          "print");

    // AND A VALUE THAT IS NOT A CALL AT ALL takes the handler's arm, which is
    // the same code from the other end -- satellite_thread/handlers.cpp says
    // why one sentence serves both sites.
    check(refused_with("satellite.variable.thread t = satellite.thread.new(5)\n",
                       Code::THREAD_NOT_A_CAPSULE_CALL),
          "a number inside new() is S1401 from the handler");
}

// --- the three verbs --------------------------------------------------------

void a_thread_runs_and_answers()
{
    check(answers("satellite.variable.thread t = "
                  "satellite.thread.new(work())\n"
                  "t.start()\n"
                  "satellite.return(t.join())\n") == "7",
          "join() answers what the capsule returned -- the author's decision "
          "of 2026-09-12, and the one verb of the three that has an answer");
}

void the_three_verbs_refuse_in_the_wrong_order()
{
    check(refused_with("satellite.variable.thread t = "
                       "satellite.thread.new(work())\n"
                       "t.start()\n"
                       "t.start()\n",
                       Code::THREAD_ALREADY_STARTED),
          "starting twice is S1402 -- a thread runs once");

    check(refused_with("satellite.variable.thread t = "
                       "satellite.thread.new(work())\n"
                       "t.join()\n",
                       Code::THREAD_NOT_STARTED),
          "joining before starting is S1403");

}

void a_second_join_warns_and_answers_the_same()
{
    // S1404 IS A WARNING SINCE THREAD.md T1 -- the author's Q2. The thread is
    // done, so the second join is done too: the same answer, the run goes on,
    // the warning waits to be printed and is already in the log.
    const std::string log = satellite::errors::log::path();
    std::remove(log.c_str());
    (void)satellite::errors::log::take();
    check(answers("satellite.variable.thread t = "
                  "satellite.thread.new(work())\n"
                  "t.start()\n"
                  "satellite.variable.number first = t.join()\n"
                  "satellite.return(first + t.join())\n") == "14",
          "joining twice gives back the same answer twice");
    const std::vector<satellite::errors::Diagnostic> warned =
        satellite::errors::log::take();
    check(warned.size() == 1 && warned[0].code == Code::THREAD_ALREADY_JOINED,
          "and S1404 is waiting to be printed when the run ends");

    std::string kept;
    if (std::FILE *in = std::fopen(log.c_str(), "r")) {
        char chunk[512];
        size_t got = 0;
        while ((got = std::fread(chunk, 1, sizeof chunk, in)) > 0)
            kept.append(chunk, got);
        std::fclose(in);
    }
    check(kept.find("warning S1404") != std::string::npos,
          "and it is already written to satellite.log");
}

void a_global_counted_by_threads_is_exact()
{
    // THREAD.md T2 -- D10. Four threads adding one to a global 500 times each
    // lost more than half the updates before the access list; a statement now
    // keeps `satellite.library` until it ends, so the total is exact.
    check(answers("satellite.variable.thread a = satellite.thread.new(bump())\n"
                  "satellite.variable.thread b = satellite.thread.new(bump())\n"
                  "satellite.variable.thread c = satellite.thread.new(bump())\n"
                  "satellite.variable.thread d = satellite.thread.new(bump())\n"
                  "a.start()\nb.start()\nc.start()\nd.start()\n"
                  "a.join()\nb.join()\nc.join()\nd.join()\n"
                  "satellite.return(satellite.library.n)\n",
                  "satellite.library.n = 0\n"
                  "satellite.capsule bump() satellite.returns(satellite.variable.number)\n"
                  "{\n"
                  "    satellite.variable.number i = 0\n"
                  "    satellite.statement.for (i = 0; i < 500; i = i + 1)\n"
                  "    {\n"
                  "        satellite.library.n = satellite.library.n + 1\n"
                  "    }\n"
                  "    satellite.return(0)\n"
                  "}\n") == "2000",
          "four threads adding one 500 times each to a global give exactly 2000");
}

void a_thread_can_start_a_thread()
{
    // THREAD.md D8, which hung every time: the inner thread's interrupt hook
    // was the outer thread's `stopped_or_interrupted`, and it asked itself for
    // ever. The answer travels two joins back.
    check(answers("satellite.variable.thread t = "
                  "satellite.thread.new(outer())\n"
                  "t.start()\n"
                  "satellite.return(t.join())\n",
                  "satellite.capsule outer()\n"
                  "{\n"
                  "    satellite.variable.thread u = satellite.thread.new(work())\n"
                  "    u.start()\n"
                  "    satellite.return(u.join() + 1)\n"
                  "}\n") == "8",
          "a thread started by a thread runs, and both joins answer");
}

void a_declared_thread_holds_nothing()
{
    // DESIGN §6.4 QUALIFICATION 3, AND NOT A WRONG-TYPE SENTENCE. `satellite
    // .variable.thread t` with nothing on the right is a declared name holding
    // nothing, which is a state every type has since M12 -- so the refusal is
    // S0713's and says so, rather than telling somebody a thread is not a
    // thread.
    check(refused_with("satellite.variable.thread t\n"
                       "t.start()\n",
                       Code::EVAL_HOLDING_NOTHING),
          "a declared thread that was never given one holds nothing, and "
          "start() says so in §6.4's words");
}

// --- what a thread's refusal does -------------------------------------------

void a_thread_that_refuses_hands_its_own_sentence_back()
{
    // THE THREAD'S CODE, NOT A CODE ABOUT THREADS -- errors.def's S14xx note.
    // `boom` divides by zero on the thread; what the joining walk stops with
    // is S0601, the sentence about the division, with the caret on the line
    // that did it.
    Run run;
    build("satellite.capsule boom()\n"
          "{\n"
          "    satellite.variable.number zero = 0\n"
          "    satellite.return(1 / zero)\n"
          "}\n"
          "satellite.capsule it()\n"
          "{\n"
          "    satellite.variable.thread t = satellite.thread.new(boom())\n"
          "    t.start()\n"
          "    satellite.return(t.join())\n"
          "}\n",
          run);
    check(run.built, "the threaded division compiles");
    if (!run.built)
        return;
    call(run, "it", {});
    check(ran_into(Code::NUMBER_DIVIDE_BY_ZERO),
          "a capsule that refuses on a thread raises ITS OWN code in the walk "
          "that joined it -- the thread is how it ran, not what went wrong");
}

// --- closing ----------------------------------------------------------------

void close_all_reports_a_thread_nobody_waited_for()
{
    // THE OTHER HALF OF THE SENTENCE ABOVE. A thread that refused and was
    // never joined has a diagnostic with nowhere to go, and run_command asks
    // close_all() for exactly those -- a program losing an error silently is
    // what DESIGN §1.1 will not have.
    //
    // THE SLEEP IS WHAT MAKES THIS A TEST AND NOT A COIN TOSS, and finding out
    // why is worth more than the clause. Without it the capsule returns in
    // microseconds, `close_all()` raises `stop` before the child has reached
    // its first statement boundary, and the child stops having divided nothing
    // by nothing -- Ending::Interrupted, no diagnostic, and this clause failed.
    // That is the CORRECT behaviour and not a bug: "close everything" means a
    // thread started on the last line of a program may never run at all, which
    // is what the author's decision of 2026-09-12 costs and what M23.md §4
    // records. The sleep buys the thread the time to actually fail, so the
    // clause is about reporting rather than about scheduling.
    Run run;
    build("satellite.capsule boom()\n"
          "{\n"
          "    satellite.variable.number zero = 0\n"
          "    satellite.return(1 / zero)\n"
          "}\n"
          "satellite.capsule it()\n"
          "{\n"
          "    satellite.variable.thread t = satellite.thread.new(boom())\n"
          "    t.start()\n"
          "    satellite.time.sleep(0.05)\n"
          "    satellite.return(1)\n"
          "}\n",
          run);
    check(run.built, "the unjoined threaded division compiles");
    if (!run.built)
        return;
    call(run, "it", {});
    check(last_ending == satellite::eval::Ending::Finished,
          "the walk that started it finished -- the thread's refusal is not "
          "the caller's");

    // READ FROM THE HELPER AND NOT CLOSED AGAIN HERE. `call` closes the run's
    // threads before it returns -- thread_handle.hpp's rule -- so asking a
    // second time would answer an empty registry and this clause would pass by
    // testing nothing.
    bool found = false;
    for (const satellite::errors::Diagnostic &problem : last_abandoned)
        if (problem.code == Code::NUMBER_DIVIDE_BY_ZERO)
            found = true;
    check(found, "close_all() hands back the refusal of a thread nobody "
                 "joined, so run_command can print it and fail the run");
}

void close_all_keeps_nothing_from_a_thread_it_stopped()
{
    // STOPPING IS NOT FAILING. A thread the run closed was doing nothing
    // wrong, so its Interrupted ending produces no sentence -- which is the
    // difference between this clause and the one above it, and the reason
    // close_all() reads `finished` BEFORE it raises `stop`.
    Run run;
    build("satellite.capsule forever()\n"
          "{\n"
          "    satellite.variable.number i = 0\n"
          "    satellite.statement.while (i >= 0)\n"
          "    {\n"
          "        i = i + 1\n"
          "    }\n"
          "    satellite.return(0)\n"
          "}\n"
          "satellite.capsule it()\n"
          "{\n"
          "    satellite.variable.thread t = satellite.thread.new(forever())\n"
          "    t.start()\n"
          "    satellite.return(1)\n"
          "}\n",
          run);
    check(run.built, "the endless threaded loop compiles");
    if (!run.built)
        return;
    call(run, "it", {});

    // AND THIS IS THE CLAUSE THAT WOULD HANG IF THE STOP FLAG DID NOT WORK,
    // which is worth saying out loud: a suite that failed here would fail by
    // never finishing. The loop has a statement boundary every iteration, so
    // the flag reaches it at the first one after close_all() sets it -- and
    // `call` is what called it, before it returned.
    check(last_abandoned.empty(),
          "a thread the run stopped reports nothing -- the program was over, "
          "and it was not wrong");
}

} // namespace

void section_threads()
{
    auto &table = satellite::eval::Handlers::table();
    table.clear();

    // NO CONSOLE ROW, AND THE ONE CLAUSE THAT NAMES `display` IS WHY THAT IS
    // SAFE RATHER THAN LUCKY. eval_test links no console -- it never has --
    // and `satellite.thread.new(satellite.console.display("x"))` is refused by
    // the COMPILER, which reads resolve's answer and never asks the handler
    // table whether the row exists. A fixture that needed the console to prove
    // that would be proving it at the wrong layer.
    satellite::scalars::install_handlers();
    satellite::time::install_handlers();
    satellite::thread::install_handlers();

    // THE LOG GOES TO A FILE OF THIS SUITE'S OWN, so a fixture's S1404 does not
    // sit in the author's ~/.satl/satellite.log for ever.
    const std::string own_log =
        "/tmp/satellite_eval_test_" + std::to_string(getpid()) + ".log";
    satellite::errors::log::redirect(own_log);

    // THE COUNT IS HERE SO A ROW DROPPED FROM THE INSTALL LOOP CANNOT VANISH
    // QUIETLY -- the contract three other sections keep, and M21.md §5 records
    // that all three caught a row ARRIVING, which is the direction they were
    // not written for. M23 installs three.
    const size_t after = table.installed();

    a_call_is_packaged_and_not_performed();
    a_packaged_call_checks_its_arity();
    only_a_capsule_of_your_own_can_be_threaded();
    a_thread_runs_and_answers();
    the_three_verbs_refuse_in_the_wrong_order();
    a_second_join_warns_and_answers_the_same();
    a_thread_can_start_a_thread();
    a_global_counted_by_threads_is_exact();
    a_declared_thread_holds_nothing();
    a_thread_that_refuses_hands_its_own_sentence_back();
    close_all_reports_a_thread_nobody_waited_for();
    close_all_keeps_nothing_from_a_thread_it_stopped();

    check(table.installed() == after,
          "nothing installed a row while the section ran");
    table.clear();
    std::remove(own_log.c_str());
    satellite::errors::log::redirect("");
}

} // namespace eval_test
