// M20 -- the arguments object's ten selectors `1 4 3 1`-`1 4 3 10`, and
// `satellite.variable.string.resolved` `1 6 1 17`.
//
// WHAT THIS SECTION IS FOR, AND IT IS NOT THE FACTS. The thirty-six facts under
// `arguments` are this machine's -- a suite asserting that `machine.threads` is
// 24 would be a suite that fails on somebody else's laptop, which is the rule
// v1's own full_test.satl states and keeps. What IS checkable everywhere is the
// SHAPE: which road a spelling takes, which number it lands on, and that the
// two halves of the object answer about their own half.
//
// SO EVERY CLAUSE BELOW IS ABOUT THE COMMAND LINE OR ABOUT A REFUSAL, and the
// command line is one this file sets: arguments::start() takes the words, so a
// fixture can say what was typed and then assert the count.
//
// AND `count()` AGAINST `length()` IS THE ONE THE MILESTONE ALMOST GOT WRONG.
// Both were spelled `count` on 2026-09-11 -- the object's own row and the
// selector -- and off `satellite.main`'s parameter one silently answered the
// other's question. The author split the words. These clauses are what stop
// them growing back together.

#include "eval_test.hpp"

#include "satellite_arguments/arguments.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_value/render.hpp"

#include <string>
#include <vector>

using namespace satellite;

namespace {

// A program with a `satellite.main` that takes the object under one of its
// seven spellings, and one line of body.
std::string over(const std::string &spelling, const std::string &body)
{
    return "satellite.capsule satellite.main("
           "satellite.container.list<satellite.variable.string> " + spelling +
           ")\n{\n    satellite.return(" + body + ")\n}\n";
}

} // namespace

namespace eval_test {

void section_arguments()
{
    // THE COMMAND LINE THIS SECTION ASSERTS ABOUT. Three words, so `length()`
    // is 3 and `first()` and `last()` are two different strings -- a one-word
    // line would let a `first` that answered `last` pass.
    arguments::start({"satl", "alpha", "beta"});

    // THE ROWS, INSTALLED THE WAY programs/run_command.cpp INSTALLS THEM. The
    // selectors are in the same table as every other module's and are reached
    // through the same dispatch, so a section that forgot this line would see
    // S0721 -- "a path satellite has a number for and nothing behind yet" --
    // for all ten and would be testing the table rather than the object.
    // `scalars` is here for `resolved` at the bottom of this section.
    scalars::install_handlers();
    containers::install_handlers();
    arguments::install_handlers();

    // --- the command line half ------------------------ 1 4 3 1, 8, 9, 10
    {
        Run run;
        build(over("argz", "argz.length()"), run);
        check(run.built, "the selector compiles");
        check(answer_of(run, "main", {}) == "3",
              "length() is how many words were typed, argv[0] included");
    }
    {
        Run run;
        build(over("argz", "argz.first()"), run);
        check(answer_of(run, "main", {}) == "satl",
              "first() is position 0, which is the program");
    }
    {
        Run run;
        build(over("argz", "argz.last()"), run);
        check(answer_of(run, "main", {}) == "beta",
              "last() is the end of the command line");
    }
    {
        Run run;
        build(over("argz", "argz.contains(\"alpha\")"), run);
        check(answer_of(run, "main", {}) == "true",
              "contains(x) finds a word that was typed");
    }
    {
        Run run;
        build(over("argz", "argz.contains(\"machine.threads\")"), run);
        check(answer_of(run, "main", {}) == "false",
              "and does NOT look through the machine's facts -- the command "
              "line is the half this word asks about");
    }

    // --- every entry ----------------------------------- 1 4 3 2, 3, 6, 7
    //
    // THE COUNT IS COMPARED AGAINST keys(), NOT AGAINST A NUMBER. How many
    // facts the object holds is a property of words.def and moves the day a
    // row is appended; that the two agree is a property of the code and never
    // moves. MILESTONES/M20.md §3 is where the first shape is recorded.
    {
        Run run;
        build(over("argz",
                   "argz.count() > argz.length()"), run);
        check(answer_of(run, "main", {}) == "true",
              "count() holds the machine's facts as well as the command line");
    }
    {
        Run run;
        build("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.container.list<satellite.variable.string> named = "
              "argz.keys()\n"
              "    satellite.return(named.size() == argz.count())\n"
              "}\n", run);
        check(answer_of(run, "main", {}) == "true",
              "keys() names every entry count() counts");
    }
    {
        Run run;
        build(over("argz", "argz.has(\"machine.threads\")"), run);
        check(answer_of(run, "main", {}) == "true",
              "has(k) finds a fact under its dotted name");
    }
    {
        Run run;
        build(over("argz", "argz.has(\"no.such.fact\")"), run);
        check(answer_of(run, "main", {}) == "false",
              "and answers false rather than stopping -- the question get(k) "
              "refuses");
    }
    {
        Run run;
        build(over("argz", "argz.get(\"no.such.fact\")"), run);
        call(run, "main", {});
        check(ran_into(errors::Code::EVAL_NO_SUCH_ENTRY),
              "S0731: a miss is an error and not nothing, because two of the "
              "object's rows are M25's and refuse");
    }
    {
        Run run;
        build(over("argz", "argz.get(3)"), run);
        call(run, "main", {});
        check(ran_into(errors::Code::EVAL_WRONG_TYPE),
              "S0711: a name is a string -- a number is the wrong kind of "
              "question rather than a key that is missing");
    }

    // --- the whole of it as text ------------------------------ 1 4 3 4, 5
    {
        Run run;
        build("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.variable.string a = argz.to_string()\n"
              "    satellite.variable.string b = argz.lines()\n"
              "    satellite.return(a == b)\n"
              "}\n", run);
        check(answer_of(run, "main", {}) == "true",
              "to_string() and lines() are two numbers and one answer");
    }

    // --- `count` AND `length` ARE DIFFERENT WORDS AND STAY THAT WAY ----------
    //
    // `argz.length` is the object's own row `1 14 1 1 9`, which takes no
    // receiver; `argz.count()` falls through to the selector `1 4 3 2`. Before
    // the author split them on 2026-09-11 both were `count` and the first one
    // found won, silently.
    {
        Run run;
        build(over("argz", "argz.length"), run);
        check(answer_of(run, "main", {}) == "3",
              "the bare `length` is the object's row and counts the command "
              "line");
    }
    {
        Run run;
        build(over("argz", "argz.count() == argz.length"), run);
        check(answer_of(run, "main", {}) == "false",
              "and `count` is the OTHER number -- the two words do not answer "
              "each other's question");
    }

    // --- a fact is not a method, and a method is not a fact ------------------
    //
    // The clause in compile_expressions.cpp's method_receiver(), from both
    // sides. `argz.machine.threads()` is a module constant and must NOT arrive
    // with the object as argument 0 (which was S0722 until this milestone's
    // third commit); `argz.length()` must.
    {
        Run run;
        build(over("argz", "argz.machine.threads() > 0"), run);
        check(answer_of(run, "main", {}) == "true",
              "a fact called with parentheses is still a fact");
    }
    {
        Run run;
        build(over("argz", "argz.nosuchthing"), run);
        // ASKED OF `resolved` AND NOT OF `program`, because this refusal is
        // RESOLVE's and build() stops before compiling when resolve fails --
        // so `raised()`, which reads the compiler's list, would never see it.
        check(!run.built, "a word the object does not have stops the program "
                          "before it is compiled");
        bool said = false;
        for (const errors::Diagnostic &at : run.resolved.problems)
            said = said || at.code == errors::Code::RESOLVE_NO_SUCH_ARGUMENT_FIELD;
        check(said,
              "S0532: a word the object does not have is refused at resolve, "
              "and the sentence lists the selectors beside the facts");
    }

    // --- the live codes, read rather than printed ------------------ 1 6 1 17
    //
    // WHAT `resolved` IS FOR. A live code is one stored character that the
    // machine answers at DISPLAY time (DESIGN §8.3), so `"\threads"` has size
    // 1 and `to_number()` refuses on the placeholder text -- the six live
    // values could be printed and never read. The two clauses below are the
    // boundary and the door through it.
    {
        Run run;
        build("satellite.capsule satellite.main()\n{\n"
              "    satellite.variable.string live = \"\\threads\"\n"
              "    satellite.return(live.size())\n}\n", run);
        check(answer_of(run, "main", {}) == "1",
              "every other method sees the STORED code, which is one character");
    }
    {
        Run run;
        build("satellite.capsule satellite.main()\n{\n"
              "    satellite.variable.string live = \"\\threads\"\n"
              "    satellite.variable.string said = live.resolved()\n"
              "    satellite.return(said.size() > 1)\n}\n", run);
        check(answer_of(run, "main", {}) == "true",
              "resolved() answers the machine's own text, which is longer than "
              "the code it replaces");
    }
    {
        Run run;
        build("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n{\n"
              "    satellite.variable.string live = \"\\threads\"\n"
              "    satellite.variable.string said = live.resolved()\n"
              "    satellite.variable.number code = said.to_number()\n"
              "    satellite.return(code == argz.machine.threads)\n}\n", run);
        check(answer_of(run, "main", {}) == "true",
              "DESIGN §7.7's three surfaces, asserted rather than looked at: "
              "live code 97 and the object are one fact");
    }
    {
        Run run;
        build("satellite.capsule satellite.main()\n{\n"
              "    satellite.variable.string plain = \"ordinary\"\n"
              "    satellite.variable.string said = plain.resolved()\n"
              "    satellite.return(said == plain)\n}\n", run);
        check(answer_of(run, "main", {}) == "true",
              "a string with no live code in it makes the round trip "
              "unchanged, which is what lets a program call this without "
              "asking first");
    }
}

} // namespace eval_test
