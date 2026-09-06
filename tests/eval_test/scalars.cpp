// M11's rows: the module constants, the string's sixteen methods, the
// number's fifteen, and the mutation contract. tests/eval_test/dispatch.cpp
// proves the TABLE with handlers of its own; this file installs the real rows
// -- satellite_scalars/ -- and proves what they answer, which is PLAN M2's
// rule that a registry gets a reader in the milestone that writes it.
//
// THE TABLE IS PROCESS-WIDE, SO THIS SECTION PUTS IT BACK EMPTY -- the same
// contract dispatch.cpp keeps, for the same reason: a suite that passes in one
// order and fails in another is worse than one that fails.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "evaluator/dispatch.hpp"
#include "satellite_scalars/handlers.hpp"

#include <string>

namespace eval_test {

namespace {

// One capsule around a body, no parameters -- the scalar fixtures make their
// own values, so control.cpp's `n` would be one more thing to explain.
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

void section_scalars()
{
    using namespace satellite;

    scalars::install_handlers();
    check(eval::Handlers::table().installed() == 37,
          "M11's 33 rows -- 2 module constants, 16 string methods, 15 number "
          "methods -- plus M12's 4 variant methods, one summed install, and "
          "the count is here so a row dropped from an install loop cannot "
          "vanish quietly");

    // --- the module constants -- a path that evaluates without a call --------
    check(answers("    satellite.return(satellite.bool.true)\n") == "true" &&
              answers("    satellite.return(satellite.bool.false)\n") == "false",
          "`satellite.bool.true` `1 17 2` and `.false` `1 17 1` answer -- "
          "DESIGN §6.1's module-constant case, M11's whole reason the Member "
          "arm dispatches instead of refusing");

    check(answers("    satellite.statement.if (satellite.bool.true)\n    {\n"
                  "        satellite.return(1)\n    }\n"
                  "    satellite.return(2)\n") == "1",
          "and a module constant is a condition -- the first way a program can "
          "WRITE a bool where a test wants one");

    // --- the string methods, by what they answer -----------------------------
    check(answers("    satellite.variable.string s = \"Hello\"\n"
                  "    satellite.return(s.size())\n") == "5",
          "`size` counts codes");
    check(answers("    satellite.variable.string s = \"\"\n"
                  "    satellite.return(s.empty())\n") == "true",
          "`empty` on an empty string");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.find(\"ell\"))\n") == "3",
          "`find` answers a 0-based position");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.contains(\"moon\"))\n") == "false",
          "`contains` answers false rather than stopping, which is why it "
          "exists beside `find` -- and asked about `tell` it answers true, "
          "because sa-TELL-ite");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.substring(0, 3))\n") == "sat",
          "`substring` is start-inclusive, end-exclusive");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.substring(9, 9))\n") == "",
          "and both ends may sit at the size -- the empty tail is an answer");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.starts_with(\"sat\"))\n") == "true" &&
              answers("    satellite.variable.string s = \"satellite\"\n"
                      "    satellite.return(s.ends_with(\"lite\"))\n") == "true",
          "`starts_with` and `ends_with`");
    check(answers("    satellite.variable.string s = \"Mixed UP 42\"\n"
                  "    satellite.return(s.lower())\n") == "mixed up 42" &&
              answers("    satellite.variable.string s = \"Mixed UP 42\"\n"
                      "    satellite.return(s.upper())\n") == "MIXED UP 42",
          "case maps the two letter runs and leaves everything else alone");
    check(answers("    satellite.variable.string s = \"  spaced out  \"\n"
                  "    satellite.return(s.trim())\n") == "spaced out",
          "`trim` strips the raw-area whitespace from both ends");
    check(answers("    satellite.variable.string s = \"a-b-c\"\n"
                  "    satellite.return(s.replace(\"-\", \"..\"))\n") == "a..b..c",
          "`replace` is every occurrence, left to right");
    check(answers("    satellite.variable.string s = \"aaa\"\n"
                  "    satellite.return(s.replace(\"a\", \"aa\"))\n") == "aaaaaa",
          "and never rescans what it wrote in, so growth terminates");
    check(answers("    satellite.variable.string s = \"6.25\"\n"
                  "    satellite.return(s.to_number() * 4)\n") == "25",
          "`to_number` answers the number the same text would parse as");
    check(answers("    satellite.variable.string s = \"satellite\"\n"
                  "    satellite.return(s.at(0))\n") == "s",
          "`at` answers a one-character string -- the language has no "
          "character type to answer with");

    // --- mutation: the receiver's slot changes and the answer is its new value
    check(answers("    satellite.variable.string s = \"count\"\n"
                  "    s.append(\"down\")\n"
                  "    satellite.return(s)\n") == "countdown",
          "`append` writes back to the slot it was called on -- DESIGN §6.4's "
          "storage-slot rule, run forwards");
    check(answers("    satellite.variable.string s = \"x\"\n"
                  "    satellite.variable.number i = 0\n"
                  "    satellite.statement.while (i < 3)\n    {\n"
                  "        s.append(\"!\")\n"
                  "        i = i + 1\n    }\n"
                  "    satellite.return(s)\n") == "x!!!",
          "and accumulates across a loop, which is QUAD's sstream shape");
    check(answers("    satellite.variable.string s = \"full\"\n"
                  "    satellite.variable.string t = s.clear()\n"
                  "    satellite.return(t.empty())\n") == "true" &&
              answers("    satellite.variable.string s = \"full\"\n"
                      "    s.clear()\n"
                      "    satellite.return(s.empty())\n") == "true",
          "a mutating method's answer IS the receiver's new value -- one "
          "value, two destinations, operations_dispatch.cpp's contract");

    // --- the refusals, each by its row ---------------------------------------
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    satellite.return(s.find(\"zz\"))\n",
                       errors::Code::EVAL_NOT_FOUND),
          "S0716: `find` on a missing needle refuses rather than answering a "
          "sentinel -- a refusal can loosen at M12, a -1 is forever");
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    satellite.return(s.at(3))\n",
                       errors::Code::EVAL_PAST_THE_END),
          "S0715: `at` one past the last character");
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    satellite.return(s.substring(2, 1))\n",
                       errors::Code::EVAL_BACKWARDS),
          "S0719: a backwards substring");
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    satellite.return(s.find(5))\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: a number where a string argument belongs");
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    satellite.return(s.at(\"x\"))\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: a string where a position belongs");
    check(refused_with("    satellite.variable.string s\n"
                       "    satellite.return(s.size())\n",
                       errors::Code::EVAL_HOLDING_NOTHING),
          "S0714: DESIGN §6.4 q3's declared-and-holding-nothing, at its one "
          "runtime site");
    check(refused_with("    satellite.variable.string s = \"abc\"\n"
                       "    s = 5\n"
                       "    satellite.return(s.upper())\n",
                       errors::Code::EVAL_WRONG_TYPE),
          "S0713: the declaration folded the selector and the VALUE is what "
          "the handler checks -- satellite checks values, not annotations");
    check(refused_with("    satellite.variable.string s = \"nope\"\n"
                       "    satellite.return(s.to_number())\n",
                       errors::Code::NUMBER_NOT_A_NUMBER),
          "S0610: `to_number` refuses with the literal's own sentence, one "
          "rule in one row");
    check(refused_with("    satellite.return("
                       "satellite.variable.string.upper)\n",
                       errors::Code::EVAL_NEEDS_RECEIVER),
          "S0718: the written-out spelling is not surface syntax -- DESIGN "
          "§6.4 -- and the row itself is what refuses it");

    // --- the row that knew its milestone, and now answers ---------------------
    //
    // `split` REFUSED WITH S0720 NAMING M16 FROM M11 UNTIL 2026-09-06, on the
    // grounds that "it answers a `satellite.container.list`, and PLAN.md §8
    // builds the containers at M16". The containers landed, so the refusal
    // became the implementation and this fixture flipped with it -- which is
    // the shape a row that knows its milestone is supposed to have.
    check(answers("    satellite.variable.string s = \"a b\"\n"
                  "    satellite.return(s.split(\" \"))\n") == "[a, b]",
          "`split` `1 6 1 10` answers a list, since M16");

    // --- the three that waited on M15, answering -----------------------------
    // The digits are float_test's to prove; what this suite owns is the
    // DISPATCH -- the fold through the declared type, the float coming back
    // as a value, and the S06xx refusals arriving with the right rows.
    check(answers("    satellite.variable.number n = 2\n"
                  "    satellite.return(n.power(10))\n") == "1024.0",
          "`power` answers, and answers a FLOAT even at an exact row -- "
          "DESIGN §8.6's result-type argument");
    check(answers("    satellite.variable.number n = 6.25\n"
                  "    satellite.return(n.sqrt())\n") == "2.5",
          "`sqrt` answers, exactly where the root is exact");
    check(answers("    satellite.variable.number n = 0 - 2.7\n"
                  "    satellite.return(n.truncate())\n") == "-2",
          "`truncate` is toward zero -- a float's left half, kept a NUMBER");
    check(refused_with("    satellite.variable.number n = 0 - 4\n"
                       "    satellite.return(n.sqrt())\n",
                       errors::Code::NUMBER_NO_REAL_ROOT),
          "S0602: sqrt of a negative");
    check(refused_with("    satellite.variable.number n = 0 - 4\n"
                       "    satellite.return(n.power(0.5))\n",
                       errors::Code::NUMBER_NEGATIVE_FRACTIONAL_POWER),
          "S0603: a negative base under a fractional exponent");
    check(refused_with("    satellite.variable.number n = 0\n"
                       "    satellite.return(n.power(0 - 1))\n",
                       errors::Code::NUMBER_DIVIDE_BY_ZERO),
          "S0601: zero to a negative power is one over zero");

    // --- the number methods --------------------------------------------------
    check(answers("    satellite.variable.number n = 0 - 7\n"
                  "    satellite.return(n.abs())\n") == "7",
          "`abs`, through M8's sign");
    check(answers("    satellite.variable.number n = 3\n"
                  "    satellite.return(n.max(9) + n.min(1))\n") == "10",
          "`max(a, b)` and `min(a, b)` -- the receiver is the `a` the row "
          "names, WORD_NUMBERS' written-out count");
    check(answers("    satellite.variable.number n = 15\n"
                  "    satellite.return(n.clamp(0, 10))\n") == "10",
          "`clamp(a, low, high)`");
    check(answers("    satellite.variable.number n = 2.5\n"
                  "    satellite.return(n.floor() + n.ceil())\n") == "5",
          "`floor` and `ceil`");
    check(answers("    satellite.variable.number n = 2.5\n"
                  "    satellite.return(n.round())\n") == "3",
          "`round` is half away from zero -- bignum.hpp's documented rule, "
          "not M15's open one");
    check(answers("    satellite.variable.number n = 12345\n"
                  "    satellite.return(n.digits())\n") == "5",
          "`digits`");
    check(answers("    satellite.variable.number n = 3\n"
                  "    satellite.variable.string s = n.to_string()\n"
                  "    satellite.return(s.append(\"!\"))\n") == "3!",
          "`to_string` answers a real string a string method will take");
    check(answers("    satellite.variable.number n = 3\n"
                  "    satellite.return(n.shift_left(10))\n") == "3072",
          "`shift_left` is × 2ⁿ -- DESIGN §5.5's answer, exact");
    check(answers("    satellite.variable.number n = 3072\n"
                  "    satellite.return(n.shift_right(10))\n") == "3" &&
              answers("    satellite.variable.number n = 1\n"
                      "    satellite.return(n.shift_right(2))\n") == "0.25",
          "`shift_right` is ÷ 2ⁿ and terminates exactly, because 2 divides 10");
    check(answers("    satellite.variable.number n = 17\n"
                  "    satellite.return(n.modulus(5))\n") == "2",
          "`modulus` -- exact and bounded, which is why it is not at M15");
    check(refused_with("    satellite.variable.number n = 17\n"
                       "    satellite.return(n.modulus(0))\n",
                       errors::Code::NUMBER_DIVIDE_BY_ZERO),
          "S0601: modulus by zero is the `%` sentence, one rule in one row");

    // PUT IT BACK. See the file note.
    eval::Handlers::table().clear();
    check(eval::Handlers::table().installed() == 0,
          "the table is left as it was found");
}

} // namespace eval_test
