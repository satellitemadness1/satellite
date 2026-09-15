// M15's rows through the machine: the float declaration and its conversion,
// mixed arithmetic, the two numeric arms agreeing about order and equality,
// and THE RETUNE -- dispatch.hpp's Assigners, proved here with a row this
// suite installs itself.
//
// THE DIGITS ARE float_test's TO PROVE. What this suite owns is the machine:
// that a value crosses into a float-declared name through op_to_float, that
// op_binary promotes exactly, that policy().float_digits is what a float
// division reads, and that an assignment to a numbered language path
// dispatches through the write table -- PLAN M2's rule that a registry needs
// a consumer in the milestone that builds it, satisfied the way
// section_dispatch satisfied it for the read table: with the suite's own row,
// because this binary deliberately links neither satellite_system nor
// machine_limits (the header's load-bearing omission).

#include "eval_test.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_scalars/handlers.hpp"

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

// The section's own write row: stores what arrives through the machine's own
// retune methods, so the fixture below proves the WHOLE road -- compile to
// op_retune, dispatch through Assigners, a store into the running Policy,
// and the next division observing it.
bool test_retune(satellite::eval::Machine &m, const satellite::Value *a,
                 uint32_t, satellite::Value *)
{
    const satellite::Number *count = std::get_if<satellite::Number>(&a[0]);
    long long wide = 0;
    if (count == nullptr || !count->to_integer(wide))
        return false;
    m.retune_division_digits(static_cast<unsigned>(wide));
    return true;
}

} // namespace

void section_floats()
{
    using namespace satellite;

    scalars::install_handlers();

    // --- the declaration converts, and only the declaration ----------------
    check(answers("    satellite.variable.float x = 3.14\n"
                  "    satellite.return(x)\n") == "3.14",
          "a float declaration converts its initialiser");
    check(answers("    satellite.variable.float x = 4\n"
                  "    satellite.return(x)\n") == "4.0",
          "and a whole one prints its point -- the type is visible");
    check(answers("    satellite.variable.number n = 4\n"
                  "    satellite.return(n)\n") == "4",
          "a number declaration stores checkless, exactly as before");
    check(answers("    satellite.variable.float x = 1\n"
                  "    x = 2\n"
                  "    satellite.return(x)\n") == "2.0",
          "assignment into a float-declared name converts too -- the three "
          "stores share into_declared and cannot drift");
    check(refused_with("    satellite.variable.float x = \"abc\"\n"
                       "    satellite.return(x)\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: a string cannot cross into a float declaration");

    // --- mixed arithmetic promotes exactly; number-only stays exact --------
    check(answers("    satellite.variable.float x = 2.5\n"
                  "    satellite.return(x * 2)\n") == "5.0",
          "float times number is a float");
    check(answers("    satellite.variable.float x = 0.1\n"
                  "    satellite.return(x + 0.2)\n") == "0.3",
          "float addition is exact -- the property `double` does not have, "
          "kept through the machine");
    check(answers("    satellite.variable.float x = 7.5\n"
                  "    satellite.return(x % 2.1)\n") == "1.2",
          "float modulus is exact and takes the receiver's sign road");
    check(answers("    satellite.variable.float x = 1\n"
                  "    satellite.return(x / 3)\n") ==
              "0.3333333333333333333333333333333333",
          "a float division reads policy().float_digits -- 34 places by the "
          "same default the number's division has significant digits");
    check(answers("    satellite.variable.float x = 2.5\n"
                  "    satellite.return(0 - x)\n") == "-2.5",
          "negation reaches the float arm");
    check(refused_with("    satellite.variable.float x = 1\n"
                       "    satellite.return(x / 0.0)\n",
                       errors::Code::NUMBER_DIVIDE_BY_ZERO),
          "S0601: a float divided by zero is the same fact as a number");

    // --- the two numeric arms agree about order and equality ----------------
    check(answers("    satellite.variable.float x = 3\n"
                  "    satellite.return(x == 3)\n") == "true",
          "a float equals the number it holds -- value.cpp's M15 decision");
    check(answers("    satellite.variable.float x = 3\n"
                  "    satellite.return(x <= 3)\n") == "true",
          "and the ordering answers the same way, or QUAD's comparators "
          "stop being a strict weak order");
    check(answers("    satellite.variable.float x = 2.5\n"
                  "    satellite.variable.float y = 2.4\n"
                  "    satellite.return(x > y)\n") == "true",
          "float against float orders");
    check(answers("    satellite.variable.float x = 3\n"
                  "    satellite.return(x == \"3\")\n") == "false",
          "every other cross-arm pair is still never equal");
    check(refused_with("    satellite.variable.float x = 1\n"
                       "    satellite.statement.if (x)\n"
                       "    {\n"
                       "        satellite.return(satellite)\n"
                       "    }\n",
                       errors::Code::EVAL_NOT_A_CONDITION),
          "S0710: a float is not a condition -- no truthiness arrived with "
          "the arm");

    // --- the variant's vocabulary widened, as its file note promised --------
    check(answers("    satellite.variable.float f = 1.5\n"
                  "    satellite.variable.variant box = f / 0.5\n"
                  "    satellite.return(box.holding())\n") == "float",
          "`holding` answers the seventh word");
    check(answers("    satellite.variable.float f = 2.5\n"
                  "    satellite.variable.variant box = f * 1\n"
                  "    satellite.return(box.holds(\"float\"))\n") == "true",
          "`holds(\"float\")` loosened from a refusal into an answer");

    // --- the retune: one mechanism, proved end to end -----------------------
    eval::Assigners::table().install(
        static_cast<words::PathId>(words::NodeId::LIBRARY_SYSTEM_DIVISION_DIGITS),
        {test_retune, "eval_test"});

    check(answers("    satellite.library.system.division_digits = 5\n"
                  "    satellite.return(1 / 3)\n") == "0.33333",
          "an assignment to a numbered language path dispatches through the "
          "write table, stores into the running Policy, and the NEXT "
          "division observes it -- the retune, end to end");
    check(refused_with("    satellite.console.width = 5\n"
                       "    satellite.return(satellite)\n",
                       errors::Code::EVAL_NOT_RETUNABLE),
          "S0724: a language path with no write row refuses by name");
    {
        // A RETUNE IN A BRANCH THAT NEVER RUNS IS A PROGRAM THAT RUNS --
        // op_refuse's argument, held for the write table too.
        Run run;
        build(capsule("    satellite.variable.bool never = satellite.bool.false\n"
                      "    satellite.statement.if (never)\n"
                      "    {\n"
                      "        satellite.console.width = 5\n"
                      "    }\n"
                      "    satellite.return(7)\n"),
              run);
        check(run.built && answer_of(run, "it", {}) == "7",
              "an unreachable retune of an unwritable path still runs");
    }

    eval::Assigners::table().clear();
    eval::Handlers::table().clear();
}

} // namespace eval_test
