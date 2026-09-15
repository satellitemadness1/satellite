// M26's spacesuits through the machine: construction, reference semantics,
// inheritance, a mutating method on a field, and every refusal resolve raises
// about a suit. example/spacesuits.satl is the same story told to a person.
//
// PLAN §8's DONE-WHEN IS THE SPINE OF THIS FILE. It asks for a program that
// "declares a spacesuit with a protected field and a public accessor,
// constructs two of them, passes one into a capsule that mutates it, and
// proves the caller sees the mutation -- reference semantics, demonstrated
// rather than asserted", and for "reaching a protected field from outside" to
// be "refused by name ... rather than by silence". Both halves are fixtures
// below, and every other fixture is there because the first two lean on it.
//
// THE REFUSALS ARE ASSERTED BY CODE AND NOT BY SENTENCE. errors.def owns the
// wording and reporter_test owns checking it; what this section owns is that
// the right code reaches the right program.

#include "eval_test.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_scalars/handlers.hpp"

#include <string>

namespace eval_test {

namespace {

// ONE SUIT THE WHOLE SECTION SHARES: a protected number and the two public
// capsules that are the only way to reach it. It is the done-when's suit
// exactly, and nothing in it is clever.
const char *const kCounter =
    "satellite.spacesuit counter()\n"
    "{\n"
    "    satellite.protected\n"
    "    {\n"
    "        satellite.variable.number n = 0\n"
    "        satellite.capsule secret() satellite.returns(satellite.variable.number)\n"
    "        {\n"
    "            satellite.return(n)\n"
    "        }\n"
    "    }\n"
    "    satellite.public\n"
    "    {\n"
    "        satellite.variable.number shown = 1\n"
    "        satellite.capsule add(satellite.variable.number by)\n"
    "        {\n"
    "            n = n + by\n"
    "        }\n"
    "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
    "        {\n"
    "            satellite.return(n)\n"
    "        }\n"
    "    }\n"
    "}\n";

std::string answers(const std::string &suits, const std::string &body)
{
    Run run;
    build(suits + "satellite.capsule it()\n{\n" + body + "}\n", run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

// Whether parse or resolve refused the program with this code. Every suit
// refusal is resolve's, so a program that got as far as compiling was not
// refused and answers false.
bool refused_before_running(const std::string &source, satellite::errors::Code code)
{
    Run run;
    build(source, run);
    for (const satellite::errors::Diagnostic &at : run.resolved.problems)
        if (at.code == code)
            return true;
    return false;
}

std::string with_it(const std::string &suits, const std::string &body)
{
    return suits + "satellite.capsule it()\n{\n" + body + "}\n";
}

} // namespace

void section_suits()
{
    using namespace satellite;
    using errors::Code;

    scalars::install_handlers();
    containers::install_handlers();

    // --- a declaration with no `=` is the construction ----------------------

    check(answers(kCounter, "    counter c\n"
                            "    satellite.return(c.get())\n") == "0",
          "`counter c` BUILDS ONE, and its field starts at its initialiser -- "
          "compile_statements.cpp's note is why the declaration is the "
          "construction");

    check(answers(kCounter, "    counter c\n"
                            "    c.add(2)\n"
                            "    c.add(3)\n"
                            "    satellite.return(c.get())\n") == "5",
          "a public capsule writes a protected field and a second one reads it");

    check(answers(kCounter, "    counter a\n"
                            "    counter b\n"
                            "    a.add(1)\n"
                            "    b.add(5)\n"
                            "    satellite.return(a.get())\n") == "1",
          "TWO DECLARATIONS ARE TWO OBJECTS -- constructing is not sharing");

    // --- reference semantics: the done-when's central sentence --------------

    check(answers(std::string(kCounter) +
                      "satellite.capsule bump(counter target)\n"
                      "{\n"
                      "    target.add(10)\n"
                      "    satellite.return(satellite)\n"
                      "}\n",
                  "    counter mine\n"
                  "    counter other\n"
                  "    bump(mine)\n"
                  "    satellite.variable.number seen = mine.get()\n"
                  "    satellite.variable.number untouched = other.get()\n"
                  "    satellite.return(seen + untouched)\n") == "10",
          "PLAN §8's done-when: a capsule that mutates its spacesuit argument "
          "is seen by the caller -- 10 from the one passed and 0 from the one "
          "that was not. DESIGN §7.4: a spacesuit is a reference type");

    check(answers(kCounter, "    counter a\n"
                            "    counter b = a\n"
                            "    b.add(3)\n"
                            "    satellite.return(a.get())\n") == "3",
          "PLAIN ASSIGNMENT SHARES -- `b = a` is a second name for one object, "
          "which is why `.pointer()` copies nothing");

    // --- inheritance --------------------------------------------------------

    check(answers(std::string(kCounter) +
                      "satellite.spacesuit loud(counter)\n"
                      "{\n"
                      "    satellite.public\n"
                      "    {\n"
                      "        satellite.capsule twice(satellite.variable.number by)\n"
                      "        {\n"
                      "            add(by)\n"
                      "            add(by)\n"
                      "        }\n"
                      "    }\n"
                      "}\n",
                  "    loud l\n"
                  "    l.twice(4)\n"
                  "    satellite.return(l.get())\n") == "8",
          "a suit holds its superclass: its own capsule calls the parent's, and "
          "the parent's accessor answers from outside");

    // --- a mutating method on a field -- DESIGN §6.4's storage-slot rule ----

    check(answers("satellite.spacesuit notes()\n"
                  "{\n"
                  "    satellite.protected\n"
                  "    {\n"
                  "        satellite.container.list<satellite.variable.string> seen = "
                  "satellite.container.list()\n"
                  "    }\n"
                  "    satellite.public\n"
                  "    {\n"
                  "        satellite.capsule note(satellite.variable.string s)\n"
                  "        {\n"
                  "            seen.append(s)\n"
                  "        }\n"
                  "        satellite.capsule count() satellite.returns(satellite.variable.number)\n"
                  "        {\n"
                  "            satellite.return(seen.size())\n"
                  "        }\n"
                  "    }\n"
                  "}\n",
                  "    notes book\n"
                  "    book.note(\"one\")\n"
                  "    book.note(\"two\")\n"
                  "    satellite.return(book.count())\n") == "2",
          "`seen.append(s)` on a field writes back into the object -- "
          "op_method_field, the fourth answer to §6.4's storage-slot rule");

    // --- the refusals, every one by name ------------------------------------

    check(refused_before_running(with_it(kCounter, "    counter c\n"
                                                   "    satellite.return(c.secret())\n"),
                                 Code::RESOLVE_SUIT_MEMBER_PROTECTED),
          "S0516: a protected capsule called from outside is refused by name "
          "-- the done-when's second half");

    check(refused_before_running(with_it(kCounter, "    counter c\n"
                                                   "    satellite.return(c.shown)\n"),
                                 Code::RESOLVE_NO_BARE_FIELD),
          "S0517: a field reached with a dot is refused EVEN WHEN IT IS PUBLIC "
          "-- DESIGN §12, accessor methods only");

    check(refused_before_running(with_it(kCounter, "    counter c\n"
                                                   "    satellite.return(c.nope())\n"),
                                 Code::RESOLVE_NO_SUCH_MEMBER),
          "S0518: a member the suit does not have");

    check(refused_before_running("satellite.spacesuit twice()\n"
                                 "{\n"
                                 "    satellite.variable.number n = 0\n"
                                 "    satellite.variable.number n = 1\n"
                                 "}\n",
                                 Code::RESOLVE_SUIT_MEMBER_TWICE),
          "S0515: one name declared twice inside one suit");

    check(refused_before_running("satellite.spacesuit orphan(missing)\n"
                                 "{\n"
                                 "}\n",
                                 Code::RESOLVE_SUIT_NO_SUCH_SUPER),
          "S0519: extending a suit the file does not declare");

    check(refused_before_running("satellite.spacesuit a(b)\n{\n}\n"
                                 "satellite.spacesuit b(a)\n{\n}\n",
                                 Code::RESOLVE_SUIT_INHERITANCE_CYCLE),
          "S0520: two suits extending each other, refused rather than repaired");

    // --- constructors -- 2026-09-12, `satellite.constructor(args) { }` ------

    // ONE SUIT WHOSE CONSTRUCTOR DOES ARITHMETIC THAT SHOWS ITS ORDER. `n`
    // starts at 3; the constructor doubles it and adds its argument. So 7 means
    // the initialiser ran FIRST and the constructor saw it, and 1 would mean
    // the constructor ran on a field that had not been given its value yet.
    const std::string doubler =
        "satellite.spacesuit doubler()\n"
        "{\n"
        "    satellite.protected\n"
        "    {\n"
        "        satellite.variable.number n = 3\n"
        "    }\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
        "        {\n"
        "            satellite.return(n)\n"
        "        }\n"
        "    }\n"
        "    satellite.constructor(satellite.variable.number by)\n"
        "    {\n"
        "        n = n * 2 + by\n"
        "    }\n"
        "}\n";

    check(answers(doubler, "    doubler d(1)\n"
                           "    satellite.return(d.get())\n") == "7",
          "`doubler d(1)` RUNS THE CONSTRUCTOR WITH 1, after the field has its "
          "starting value -- 3 * 2 + 1");

    check(answers(doubler, "    doubler d(1)\n"
                           "    d.constructor(4)\n"
                           "    satellite.return(d.get())\n") == "18",
          "`d.constructor(4)` RUNS IT AGAIN, on the object as it now is -- "
          "7 * 2 + 4. The author's spelling, and public from outside the suit");

    check(answers(doubler, "    satellite.variable.number seed = 5\n"
                           "    doubler d(seed + 1)\n"
                           "    satellite.return(d.get())\n") == "12",
          "the arguments are ordinary expressions in the caller's scope");

    check(answers("satellite.spacesuit nine()\n"
                  "{\n"
                  "    satellite.protected { satellite.variable.number n = 0 }\n"
                  "    satellite.public\n"
                  "    {\n"
                  "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
                  "        {\n"
                  "            satellite.return(n)\n"
                  "        }\n"
                  "    }\n"
                  "    satellite.constructor()\n"
                  "    {\n"
                  "        n = 9\n"
                  "    }\n"
                  "}\n",
                  "    nine a\n"
                  "    nine b()\n"
                  "    satellite.return(a.get() + b.get())\n") == "18",
          "a constructor with no parameters runs for `nine a` and for `nine b()` "
          "alike -- M26's `counter tally` no longer skips it");

    // THE CHAIN RUNS FROM THE ROOT DOWN. Each constructor appends a digit, so
    // the answer spells the order: 17 is base then child, 71 would be the
    // reverse.
    const std::string lineage =
        "satellite.spacesuit base()\n"
        "{\n"
        "    satellite.protected { satellite.variable.number n = 0 }\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
        "        {\n"
        "            satellite.return(n)\n"
        "        }\n"
        "    }\n"
        "    satellite.constructor()\n"
        "    {\n"
        "        n = n * 10 + 1\n"
        "    }\n"
        "}\n"
        "satellite.spacesuit child(base)\n"
        "{\n"
        "    satellite.constructor(satellite.variable.number digit)\n"
        "    {\n"
        "        n = n * 10 + digit\n"
        "    }\n"
        "}\n";
    check(answers(lineage, "    child k(7)\n"
                           "    satellite.return(k.get())\n") == "17",
          "THE SUPERCLASS'S CONSTRUCTOR RUNS FIRST, with nothing, and only the "
          "most-derived one is handed the arguments -- v1 design/14's order");

    check(answers("satellite.spacesuit parent()\n"
                  "{\n"
                  "    satellite.protected { satellite.variable.number n = 0 }\n"
                  "    satellite.public\n"
                  "    {\n"
                  "        satellite.capsule get() satellite.returns(satellite.variable.number)\n"
                  "        {\n"
                  "            satellite.return(n)\n"
                  "        }\n"
                  "    }\n"
                  "    satellite.constructor(satellite.variable.number by)\n"
                  "    {\n"
                  "        n = n + by\n"
                  "    }\n"
                  "}\n"
                  "satellite.spacesuit heir(parent)\n"
                  "{\n"
                  "}\n",
                  "    heir h(6)\n"
                  "    satellite.return(h.get())\n") == "6",
          "a suit with no constructor of its own INHERITS its parent's, and it "
          "runs once -- 6, not a refusal for calling it a second time with none");

    {
        Run run;
        build(with_it(doubler, "    doubler d\n"
                               "    satellite.return(d.get())\n"),
              run);
        call(run, "it", {});
        check(ran_into(Code::EVAL_ARGUMENT_COUNT),
              "S0722: `doubler d` hands a one-parameter constructor nothing, and "
              "is refused by count rather than run on a missing argument");
    }

    check(refused_before_running(
              "satellite.spacesuit named()\n"
              "{\n"
              "    satellite.constructor() satellite.returns(satellite.variable.string)\n"
              "    {\n"
              "    }\n"
              "}\n",
              Code::RESOLVE_CONSTRUCTOR_RETURNS),
          "S0525: DESIGN §13's rule, on the constructor section");

    check(!refused_before_running("satellite.spacesuit named()\n"
                                  "{\n"
                                  "    satellite.public\n"
                                  "    {\n"
                                  "        satellite.capsule named() "
                                  "satellite.returns(satellite.variable.string)\n"
                                  "        {\n"
                                  "            satellite.return(\"x\")\n"
                                  "        }\n"
                                  "    }\n"
                                  "}\n",
                                  Code::RESOLVE_CONSTRUCTOR_RETURNS),
          "a capsule named after its suit is an ordinary method since the "
          "constructor became a section, and may declare a return type");

    check(refused_before_running(with_it(kCounter, "    satellite.variable.number n(5)\n"),
                                 Code::RESOLVE_ARGUMENTS_NOT_A_SPACESUIT),
          "S0526: arguments on a number have no constructor to go to");

    check(refused_before_running(with_it(kCounter, "    counter c(5)\n"),
                                 Code::RESOLVE_ARGUMENTS_NOT_A_SPACESUIT),
          "S0526: and neither do arguments on a suit with no constructor section");

    {
        Run run;
        build("satellite.spacesuit clash()\n"
              "{\n"
              "    satellite.public\n"
              "    {\n"
              "        satellite.capsule constructor()\n"
              "        {\n"
              "        }\n"
              "    }\n"
              "}\n",
              run);
        bool found = false;
        for (const errors::Diagnostic &at : run.parsed.errors)
            found = found || at.code == Code::PARSE_CAPSULE_NAMED_CONSTRUCTOR;
        check(found, "S0245: a capsule inside a suit may not take the name the "
                     "constructor section answers to");
    }
    {
        Run run;
        build("satellite.constructor()\n{\n}\n", run);
        bool found = false;
        for (const errors::Diagnostic &at : run.parsed.errors)
            found = found || at.code == Code::PARSE_CONSTRUCTOR_OUTSIDE_SPACESUIT;
        check(found, "S0244: a constructor at the top of a file has no suit");
    }

    // PUT IT BACK, which every section that installs does.
    eval::Handlers::table().clear();
    check(eval::Handlers::table().installed() == 0,
          "the table is left as it was found");
}

} // namespace eval_test
