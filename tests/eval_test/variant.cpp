// M12's rows: the variant's four questions, and what "nothing" is. DESIGN
// §8.7 is the answer this section proves -- nothing is a STATE EVERY TYPE HAS,
// and `satellite.variable.variant` is the type whose vocabulary can name it --
// and PLAN §8's M12 entry is the done-when each fixture below quotes.
//
// THE TABLE IS PROCESS-WIDE, SO THIS SECTION PUTS IT BACK EMPTY -- the same
// contract scalars.cpp and dispatch.cpp keep, for the same reason.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_scalars/handlers.hpp"

#include <string>

namespace eval_test {

namespace {

std::string capsule(const std::string &body)
{
    return "satellite.capsule it()\n{\n" + body + "}\n";
}

std::string answers(const std::string &body)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

bool refused_with(const std::string &body, satellite::errors::Code code)
{
    Run run;
    build(capsule(body), run);
    if (!run.built)
        return false;
    call(run, "it", {});
    return ran_into(code);
}

} // namespace

void section_variant()
{
    using namespace satellite;

    scalars::install_handlers();
    check(eval::Handlers::table().installed() == 50,
          "M11's 33 rows plus M12's 4 -- `holding`, `holds(x)`, `held`, "
          "`clear` -- plus M19.5's 11 bit-run rows, 6 binary and 5 hex, "
          "M20's `resolved` and M21's `float.to_string`, and the count is "
          "here so a row dropped from an install loop cannot vanish quietly");

    // --- holding: one word per arm, and the nothing state IS an answer ------
    check(answers("    satellite.variable.variant box = 5\n"
                  "    satellite.return(box.holding())\n") == "number",
          "`holding` on a number");
    check(answers("    satellite.variable.variant box = \"hi\"\n"
                  "    satellite.return(box.holding())\n") == "string",
          "`holding` on a string");
    check(answers("    satellite.variable.variant box = satellite.bool.true\n"
                  "    satellite.return(box.holding())\n") == "bool",
          "`holding` on a bool");
    check(answers("    satellite.variable.variant box\n"
                  "    satellite.return(box.holding())\n") == "nothing",
          "a declared variant holds nothing, and `holding` ANSWERS it -- the "
          "one method family for which the nothing state is not a refusal, "
          "which is M12's whole reason to exist");

    {
        // The runtime arm, and the capsule-with-no-return producer -- the two
        // arms a literal cannot write. `sky` answers the runtime; `quiet`
        // falls off its end and answers nothing, which is value.hpp's
        // documented producer for the state.
        Run run;
        build("satellite.capsule sky()\n{\n"
              "    satellite.return(satellite)\n}\n"
              "satellite.capsule it()\n{\n"
              "    satellite.variable.variant box = sky()\n"
              "    satellite.return(box.holding())\n}\n",
              run);
        check(run.built && answer_of(run, "it", {}) == "satellite",
              "`holding` on the runtime singleton");
    }
    {
        Run run;
        build("satellite.capsule quiet()\n{\n"
              "    satellite.variable.number unused = 0\n}\n"
              "satellite.capsule it()\n{\n"
              "    satellite.variable.variant box = 5\n"
              "    box = quiet()\n"
              "    satellite.return(box.holding())\n}\n",
              run);
        check(run.built && answer_of(run, "it", {}) == "nothing",
              "a capsule with no `satellite.return` hands over nothing, and "
              "the variant says so -- the gap between M9's value model and "
              "the language, closed");
    }

    // --- holds: the same question as yes or no ------------------------------
    check(answers("    satellite.variable.variant box = \"hi\"\n"
                  "    satellite.return(box.holds(\"string\"))\n") == "true" &&
              answers("    satellite.variable.variant box = \"hi\"\n"
                      "    satellite.return(box.holds(\"number\"))\n") ==
                  "false",
          "`holds` answers yes and no");
    check(answers("    satellite.variable.variant box\n"
                  "    satellite.return(box.holds(\"nothing\"))\n") == "true",
          "`holds(\"nothing\")` is the ask M14's `typed()` and M19's "
          "`read_line` inherit -- an empty line is a line; nobody typing is "
          "nothing");
    check(refused_with("    satellite.variable.variant box = 5\n"
                       "    satellite.return(box.holds(\"strng\"))\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: a word off the list is refused, never answered false -- a "
          "typo answered false forever is a condition no program can satisfy "
          "wearing a working test's clothes");
    check(refused_with("    satellite.variable.variant box = 5\n"
                       "    satellite.return(box.holds(7))\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: a number where the word belongs");

    // --- held: the checked extraction ---------------------------------------
    check(answers("    satellite.variable.variant box = 5\n"
                  "    satellite.variable.number n = box.held()\n"
                  "    satellite.return(n.abs())\n") == "5",
          "`held` hands the value over and the declared type's methods take "
          "it from there");
    check(refused_with("    satellite.variable.variant box\n"
                       "    satellite.return(box.held())\n",
                       errors::Code::EVAL_HOLDING_NOTHING),
          "S0714: `held` of a variant holding nothing refuses -- PLAN M12's "
          "done-when, second clause");
    {
        // AND THE REFUSAL IS BY NAME, which is the clause's own word. Until
        // M12, text_of() answered a call's anchor token and this sentence
        // read "`(` was asked of a variable that holds nothing" -- machine.cpp
        // carries the fix and this is its regression test.
        bool named = false;
        for (const errors::Diagnostic &problem : last_problems)
            if (problem.code == errors::Code::EVAL_HOLDING_NOTHING)
                named = !problem.arguments.empty() &&
                        problem.arguments[0] == "held";
        check(named, "and the sentence names `held`, not the `(` the call "
                     "node anchors on");
    }

    // --- clear: the one mutating row ----------------------------------------
    check(answers("    satellite.variable.variant box = 5\n"
                  "    box.clear()\n"
                  "    satellite.return(box.holding())\n") == "nothing",
          "`clear` writes nothing back to the slot it was called on");
    check(answers("    satellite.variable.variant box = 5\n"
                  "    satellite.variable.variant t = box.clear()\n"
                  "    satellite.return(t.holding())\n") == "nothing",
          "and a mutating method's answer is its receiver's new value -- one "
          "value, two destinations, operations_dispatch.cpp's contract");
    check(answers("    satellite.variable.variant box\n"
                  "    box.clear()\n"
                  "    satellite.return(box.holding())\n") == "nothing",
          "clearing an empty variant answers nothing rather than refusing -- "
          "the method promises a state, not a transition");

    // --- reading one, run through the seams ---------------------------------
    check(answers("    satellite.variable.variant box = 5\n"
                  "    satellite.variable.number n = box\n"
                  "    satellite.return(n.abs())\n") == "5",
          "plain assignment copies a variant's value out with no ceremony -- "
          "satellite checks values, not annotations, DESIGN §8.7");
    check(answers("    satellite.variable.string s\n"
                  "    satellite.variable.variant box = s\n"
                  "    satellite.return(box.holding())\n") == "nothing",
          "and nothing FLOWS -- a state every type has can be handed to a "
          "variant, which is reading one against reading two exactly");
    check(refused_with("    satellite.return("
                       "satellite.variable.variant.holding)\n",
                       errors::Code::EVAL_NEEDS_RECEIVER),
          "S0718: the written-out spelling is not surface syntax, from the "
          "one place that knows");
    check(refused_with("    satellite.variable.variant box = \"hi\"\n"
                       "    satellite.return(box.upper())\n",
                       errors::Code::EVAL_NO_SUCH_QUESTION),
          "S0723: `upper` is not a question a variant answers, and the "
          "sentence says what a variant DOES answer -- not S0720's 'name the "
          "receiver first', which cannot help a receiver that is a name");
    check(refused_with("    satellite.variable.string s = \"hi\"\n"
                       "    satellite.return(s.holding())\n",
                       errors::Code::EVAL_NO_SUCH_QUESTION),
          "and it is the general sentence, not a variant special case -- "
          "`holding` is not a question a string answers either");

    // PUT IT BACK. See the file note.
    eval::Handlers::table().clear();
    check(eval::Handlers::table().installed() == 0,
          "the table is left as it was found");
}

} // namespace eval_test
