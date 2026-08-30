// §5.1's transformation: the alias collapsed, the argument absorbed, the shape
// slotted by arity. See tests/satc_test/satc_test.hpp.
//
// THESE THREE STEPS ARE WHERE A `.satc` GOES WRONG WITHOUT BREAKING, and both
// defects this suite has found were here. Step 4 written without step 4's guard
// wrote `satellite.console.display` for every call under `satellite.console`;
// step 1 written to look for a call after an alias's FIRST word never matched a
// dot-carrying alias at all. Neither produced an error, an empty line or a
// crash -- both produced a file that parses.

#include "satc_test.hpp"

#include <string>

namespace satc_test {

namespace {

void arity()
{
    // A ROW WITH NO ARGUMENT LIST KEEPS THE PROGRAM'S CALL. `display` is 1 5 1
    // and the whole of it; the argument is the program's and is written out.
    check(one_line("satellite.console.display(\"x\")") == "#1.5.1(\"x\")",
          "display keeps its argument: " + one_line("satellite.console.display(\"x\")"));

    // THE FOUR ROWS OF `input`, WHICH IS THE WORD THE NUMBERING SLOTS BY ARITY
    // AND THE ONE THE FIRST DEFECT GOT WRONG. 1 5 2, 1 5 3 and 1 5 4 differ by
    // nothing a reader can see except the count of what follows.
    check(one_line("satellite.console.input()") == "#1.5.2",
          "input() is #1.5.2 and eats its parentheses: " +
              one_line("satellite.console.input()"));
    check(one_line("satellite.console.input(\"p\")") == "#1.5.3(\"p\")",
          "input(prompt) is #1.5.3: " + one_line("satellite.console.input(\"p\")"));
    check(one_line("satellite.console.input(\"p\", t)") == "#1.5.4(\"p\", t)",
          "input(prompt, target) is #1.5.4: " +
              one_line("satellite.console.input(\"p\", t)"));

    // AND THE MUTATION THAT PROVES THOSE THREE. Every one of them is a call
    // under `satellite.console`, and the defect found on 2026-08-30 wrote all
    // of them as 1.5.1 -- so a check that any of the four is 1.5.1 has to be
    // one that SHOULD be.
    check(one_line("satellite.console.input(\"p\")").find("#1.5.1") == std::string::npos,
          "input is not display");

    // A word with no call at all, which must not acquire one.
    check(one_line("satellite.console.width") == "#1.5.6",
          "a word with no call stays one: " + one_line("satellite.console.width"));
}

void absorbing()
{
    // §5.1 STEP 3, IN BOTH THE PLACES THE NUMBERING HAS IT. The number names
    // the reserved word, so nothing follows it.
    check(one_line("satellite.return(satellite)") == "#1.15.1",
          "return(satellite) is #1.15.1 alone: " + one_line("satellite.return(satellite)"));
    check(one_line("satellite.return()") == "#1.15.0",
          "return() is the bare shape: " + one_line("satellite.return()"));

    // AND THE ROW OF EQUAL ARITY THAT MUST NOT BE ABSORBED. `return(value)` is
    // 1 15 2 and one argument, exactly as `return(satellite)` is 1 15 1 and one
    // argument. Taking the absorber here would DROP the value.
    check(one_line("satellite.return(n)") == "#1.15.2(n)",
          "return(value) keeps its value: " + one_line("satellite.return(n)"));

    const std::string included = body("satellite.include(satellite)\n");
    check(code_of(line_with(included, "#1.1")) == "#1.1.1",
          "include(satellite) is #1.1.1 alone: " + code_of(line_with(included, "#1.1")));

    // A SPACESHIP'S NAME IS A USER NAME AND SURVIVES, at the row of the same
    // arity next door. This is the pair that makes the absorber a decision
    // rather than a rule: 1 1 1 and 1 1 2 are both `include` with one argument.
    const std::string ship = body("satellite.include(my_ship)\n");
    check(code_of(line_with(ship, "#1.1")) == "#1.1.2(my_ship)",
          "include(spaceship) keeps the name: " + code_of(line_with(ship, "#1.1")));
}

void aliases()
{
    // §5.1 STEP 1, AND THE THREE ALIASES THAT SPAN A DOT. words.def calls them
    // "a FOUR segment spelling of a node that is three numbers", which is why
    // they could not be nodes -- and in the tree they are two Member nodes, so
    // matching one against a single word can never succeed.
    check(one_line("satellite.random.fast(1, 10)") == "#1.7.5(1, 10)",
          "fast(min, max) is #1.7.5: " + one_line("satellite.random.fast(1, 10)"));
    check(one_line("satellite.random.fast.range(1, 10)") == "#1.7.5(1, 10)",
          "fast.range collapses onto the same node: " +
              one_line("satellite.random.fast.range(1, 10)"));

    // THE ARITY STILL DECIDES AFTERWARDS. `fast()` is 1 7 1 and `fast(min,
    // max)` is 1 7 5, and an alias that matched without checking the count
    // would make the two the same.
    check(one_line("satellite.random.fast()") == "#1.7.1",
          "fast() is its own row: " + one_line("satellite.random.fast()"));

    // THE SIX SPELLINGS OF `arguments`, of which five are aliases. All six are
    // one node, so all six are one number.
    check(one_line("satellite.console.display(satellite.library.main.arguments)") ==
              "#1.5.1(#1.14.1.1)",
          "arguments is #1.14.1.1");
    check(one_line("satellite.console.display(satellite.library.main.args)") ==
              "#1.5.1(#1.14.1.1)",
          "args is the same node as arguments: " +
              one_line("satellite.console.display(satellite.library.main.args)"));

    // AND THE TYPE ALIAS, which is the one the LEXER knows about rather than
    // the parser. Both spellings are 1 6 11, and SATC.md §6's last bullet is
    // the consequence: the file cannot say which one the source wrote, so a
    // `.satc` is never a substitute for its source.
    check(one_line("satellite.variable.hexadecimal h = 1") == "#1.6.11 h = 1",
          "hexadecimal is #1.6.11: " + one_line("satellite.variable.hexadecimal h = 1"));
    check(one_line("satellite.variable.hex h = 1") == "#1.6.11 h = 1",
          "hex is the same number: " + one_line("satellite.variable.hex h = 1"));
}

void statement_forms()
{
    // The four under `satellite.statement`, which the parser gives nodes of
    // their own rather than Member chains -- so none of them reaches the chain
    // matcher and each has to find its number another way.
    const std::string branch = statements(
        "satellite.statement.if (x > 1)\n{\nsatellite.console.display(\"a\")\n}\n"
        "satellite.statement.else\n{\nsatellite.console.display(\"b\")\n}\n");
    check(code_of(line_with(branch, "#1.13.1")) == "#1.13.1 (x > 1)",
          "if is #1.13.1: " + code_of(line_with(branch, "#1.13.1")));
    check(code_of(line_with(branch, "#1.13.4")) == "#1.13.4",
          "else is #1.13.4: " + code_of(line_with(branch, "#1.13.4")));

    const std::string loop =
        statements("satellite.statement.while (x > 1)\n{\nx = x - 1\n}\n");
    check(code_of(line_with(loop, "#1.13.3")) == "#1.13.3 (x > 1)",
          "while is #1.13.3: " + code_of(line_with(loop, "#1.13.3")));

    // `satellite.returns`, which DESIGN §13 decided and M4 built. 1 21, and it
    // is NOT 1 15 -- `return` and `returns` are two words one letter apart and
    // the numbering puts them six rows apart.
    const std::string returns = body(
        "satellite.capsule helper(satellite.variable.number n) "
        "satellite.returns(satellite.variable.number)\n{\nsatellite.return(n)\n}\n");
    check(code_of(line_with(returns, "#1.21")) == "#1.2 helper(#1.6.4 n) #1.21(#1.6.4)",
          "returns is #1.21: " + code_of(line_with(returns, "#1.21")));
}

} // namespace

void section_shapes()
{
    arity();
    absorbing();
    aliases();
    statement_forms();
}

} // namespace satc_test
