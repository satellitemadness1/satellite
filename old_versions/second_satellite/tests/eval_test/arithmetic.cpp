// DESIGN §8.1's arithmetic through the evaluator, and DESIGN §6.6's table.
//
// M8 ALREADY PROVED THE NUMBERS AND THIS IS NOT THAT. tests/number_test has 208
// check sites over `Number` itself; what is checked here is that the EVALUATOR
// reaches them -- that `a - b - c` is `(a - b) - c` and not `a - (b - c)`, that
// `/` is handed the division_digits it was given, and that an operand of the
// wrong type is a sentence rather than a wrong answer.

#include "eval_test.hpp"

#include "error_reporter/codes.hpp"
#include "satellite_value/render.hpp"

#include <string>

namespace eval_test {

namespace {

// One capsule whose whole body is `satellite.return(<expression>)`.
std::string answering(const std::string &expression)
{
    return "satellite.capsule it()\n{\n    satellite.return(" + expression +
           ")\n}\n";
}

std::string value_of(const std::string &expression)
{
    Run run;
    build(answering(expression), run);
    if (!run.built)
        return "<did not compile>";
    return answer_of(run, "it", {});
}

} // namespace

void section_arithmetic()
{
    using namespace satellite;

    check(value_of("1 + 1") == "2", "1 + 1");
    check(value_of("7 % 2") == "1", "DESIGN §8.6's modulus, which M8 built");
    check(value_of("2 * 3 + 4") == "10", "§6.6: `*` binds tighter than `+`");
    check(value_of("2 + 3 * 4") == "14", "and from the other side");

    // LEFT-ASSOCIATIVE, WHICH IS §6.6 IN ONE LINE: "a - b - c is (a - b) - c
    // and a - (b - c) is a different number that keeps its brackets."
    check(value_of("10 - 3 - 2") == "5", "§6.6: `-` is left-associative");
    check(value_of("10 - (3 - 2)") == "9", "and the brackets change it");

    // DESIGN §8.1's WHOLE ARGUMENT, reachable from a program for the first
    // time. "not integer-only (1/3 -> 0 is unacceptable), and not double."
    check(value_of("0.1 + 0.2") == "0.3",
          "§8.1: exact decimal, so 0.1 + 0.2 is 0.3 and not 0.30000000000000004");
    check(holds(value_of("1 / 3"), "0.3333333333333333333333333333333333"),
          "1 / 3 keeps division_digits significant digits");

    // §5.6 REFUSES TO FOLD A SIGN INTO A NUMBER so that `a-1` stays a
    // subtraction, which makes unary minus an expression rule by construction.
    check(value_of("-5") == "-5", "unary minus");
    check(value_of("- - 5") == "5", "and it composes");
    check(value_of("3 - -2") == "5", "a binary minus and a unary one, adjacent");

    check(value_of("1 < 2") == "true", "comparison answers a bool");
    check(value_of("2 <= 2") == "true", "and `<=` is not `<`");
    check(value_of("1 == 1") == "true", "equality");
    check(value_of("1 != 1") == "false", "and its negation");
    check(value_of("!(1 == 2)") == "true", "unary `!` over a comparison");

    // EQUALITY WORKS ACROSS TYPES AND ORDERING DOES NOT, which is DESIGN §8's
    // table read honestly -- there is no conversion in this language, so `1` and
    // `"1"` are two values and the answer is false rather than an error.
    check(value_of("1 == \"1\"") == "false",
          "a number and a string are not equal, and asking is not an error");

    {
        Run run;
        build(answering("1 < \"a\""), run);
        call(run, "it", {});
        check(ran_into(errors::Code::EVAL_NOT_COMPARABLE),
              "S0712: ordering a string against a number has no answer");
    }
    {
        Run run;
        build(answering("1 + satellite.bool.true"), run);
        call(run, "it", {});
        check(!last_problems.empty(),
              "adding something that is not a number is refused rather than "
              "guessed at");
    }
    {
        Run run;
        build(answering("1 / 0"), run);
        call(run, "it", {});
        check(ran_into(errors::Code::NUMBER_DIVIDE_BY_ZERO),
              "S0601 and not a row of its own -- `satl --number 1 / 0` already "
              "says this sentence, and FORMAT/CXX.md §1 has one place per fact");
    }

    // A STRING IS A VALUE NOW, WHICH IS THE OTHER HALF OF THIS MILESTONE'S
    // satellite_value/. DESIGN §5's live codes are answered when the string is
    // USED -- satellite_value/render.cpp -- and never when it is lexed.
    check(value_of("\"hello\"") == "hello", "a string literal is a value");
    check(value_of("\"a\\nb\"") == "a\nb", "§5.3: escapes are already expanded");
    check(value_of("\"\\threads\"") != "<threads>",
          "and a LIVE code is answered from the machine rather than left as its "
          "placeholder -- PLAN §6.1's six lines, finished at M9");
    check(!value_of("\"\\threads\"").empty(), "the machine answered something");
}

} // namespace eval_test
