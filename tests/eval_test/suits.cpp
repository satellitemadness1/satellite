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

    const std::string constructor =
        "satellite.spacesuit named()\n"
        "{\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.capsule named()%s\n"
        "        {\n"
        "        }\n"
        "    }\n"
        "}\n";
    auto shaped = [&](const std::string &returns) {
        std::string text = constructor;
        text.replace(text.find("%s"), 2, returns);
        return text;
    };
    check(refused_before_running(shaped(" satellite.returns(satellite.variable.string)"),
                                 Code::RESOLVE_CONSTRUCTOR_RETURNS),
          "S0525: DESIGN §13's rule -- the capsule named after its own suit may "
          "not declare `satellite.returns`");
    check(!refused_before_running(shaped(""), Code::RESOLVE_CONSTRUCTOR_RETURNS),
          "and the same capsule without one is not refused");

    // PUT IT BACK, which every section that installs does.
    eval::Handlers::table().clear();
    check(eval::Handlers::table().installed() == 0,
          "the table is left as it was found");
}

} // namespace eval_test
