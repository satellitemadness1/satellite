// The value types themselves: the M0 spine, arithmetic and how a number
// renders, strings, the two bools and the bare TRUE and FALSE that name them,
// and §21's binary and hexadecimal. Part of the eval_test binary; the harness
// these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "satellite_value/value.hpp"
#include "system_facts/system.hpp"

#include <string>

using namespace satellite;

void eval_test_spine()
{
    // --- the milestone itself ----------------------------------------------
    // satellite.variable.number x = 1 / x / x.plus(1), and the write must land
    // in the real satellite.library rather than a side table.
    {
        InterpResult r = run_source("satellite.variable.number x = 1\n"
                                    "x\n"
                                    "x.plus(1)\n",
                                    "main", true);
        check(r.ok, "spine runs clean");
        check(r.output == "1\n2\n", "spine echoes 1 then 2");

        ValuePtr x = Library::instance().get("main", "x");
        check(x != nullptr, "spine wrote satellite.library.main.x");
        check(x && to_string(*x) == "1", "satellite.library.main.x = 1");
    }

    // The full library path is the spelling program source must use; the bare
    // two-segment form stays a REPL-only debug convenience (§10). This one
    // pins the namespace to main, because the path in the source names it.
    {
        InterpResult r = run_source("satellite.variable.number q = 7\n"
                                    "satellite.library.main.q\n",
                                    "main", true);
        check(r.ok && r.output == "7\n",
              "satellite.library.<ns>.<name> reads a variable");
    }
}

void eval_test_arithmetic()
{
    // --- arithmetic and precedence -----------------------------------------
    check_output("1 + 2 * 3\n", "7\n", "* binds tighter than +");
    check_output("(1 + 2) * 3\n", "9\n", "parentheses");
    check_output("7 / 2\n", "3.5\n", "division is not integer division");
    check_output("7 % 2\n", "1\n", "modulo");
    check_output("-3 + 1\n", "-2\n", "unary minus");
    check_output("2 - -3\n", "5\n", "minus is never folded into a number");
}

void eval_test_number_rendering()
{
    // --- how numbers render ------------------------------------------------
    // The whole point: print the value, not six significant digits. Under the
    // old "%g" every one of the first four of these was silently wrong.
    check_output("1000000\n", "1000000\n", "a million prints whole");
    check_output("123456789\n", "123456789\n", "nine digits print whole");
    check_output("2000000 + 1\n", "2000001\n", "the + 1 is not rounded away");
    check_output("1000000 * 1000000\n", "1000000000000\n",
                 "a trillion prints whole");
    // ...without going the other way and printing float noise.
    check_output("3.14\n", "3.14\n", "3.14 is not 3.1400000000000001");
    check_output("0.1\n", "0.1\n", "0.1 is not 0.10000000000000001");
    // §8.1's migration is what changed this line. Under a double the answer
    // was 0.30000000000000004 and the test asserted it, because that was the
    // value that was actually there; the decimal makes it 0.3, which is the
    // value the program says.
    check_output("0.1 + 0.2\n", "0.3\n", "an exact decimal adds exactly");
    check_output("1 / 3\n", "0.3333333333333333333333333333333333\n",
                 "a third keeps division_digits digits");
    // Nothing rounds where it does not have to. A double loses the 1 entirely.
    check_output("100000000000000000000 + 1\n", "100000000000000000001\n",
                 "10^20 + 1 keeps its 1");
    check_output("-0\n", "0\n", "negative zero is just zero");
    check_output("2 - 2\n", "0\n", "zero is zero");
}

void eval_test_number_methods()
{
    // --- number methods ----------------------------------------------------
    check_output("3.5.floor()\n", "3\n", "floor");
    check_output("3.5.ceil()\n", "4\n", "ceil");
    check_output("(0 - 4).abs()\n", "4\n", "abs");
    check_output("10.divided_by(4)\n", "2.5\n", "divided_by");
    check_output("10.modulo(3)\n", "1\n", "modulo method");
}

void eval_test_bool_values()
{
    // --- the two satellite.variable.bool values -----------------------------
    // A MODULE CONSTANT, the shape §5 anticipated for satellite.math.pi, so it
    // costs no parser rule and no second reserved word. It cannot live under
    // satellite.variable.* — §4 reserves that as a type namespace where no path
    // is ever a value.
    check_output("satellite.bool.true\n", "true\n", "satellite.bool.true");
    check_output("satellite.bool.false\n", "false\n", "satellite.bool.false");
    check_output("satellite.variable.bool b = satellite.bool.true\nb\n",
                 "true\n", "a bool literal can be declared");
    check_output("satellite.bool.true.and(satellite.bool.false)\n", "false\n",
                 "and it has the bool methods");
    check_error("satellite.bool.maybe\n", "not a value",
                "there is no third bool");
}

void eval_test_strings()
{
    // --- strings -----------------------------------------------------------
    check_output("\"ab\".concat(\"cd\")\n", "abcd\n", "concat");
    check_output("\"abc\".length()\n", "3\n", "string length");
    check_output("\"abc\".contains(\"bc\")\n", "true\n", "contains");
    check_output("\"abc\".starts_with(\"ab\")\n", "true\n", "starts_with");
    check_output("\"abc\".ends_with(\"bc\")\n", "true\n", "ends_with");
    check_output("\"ab\" + \"cd\"\n", "abcd\n", "+ concatenates strings");

    // String indexing selects SATELLITE characters, not display characters.
    // "hi\home!" is four SatChars, and index 2 is the single home-directory
    // char that displays as a whole path (§7). This is unique to satellite and
    // is the reason it is pinned down by a test.
    check_output("\"hi\\home!\".length()\n", "4\n",
                 "\\home is one satellite character");
    check_output("\"hi\\home!\"[2]\n", home_dir() + "\n",
                 "string index selects satellite chars");
}

void eval_test_comparison()
{
    // --- booleans and comparison -------------------------------------------
    check_output("1 < 2\n", "true\n", "<");
    check_output("2 <= 2\n", "true\n", "<=");
    check_output("1 == 1\n", "true\n", "==");
    check_output("1 != 1\n", "false\n", "!=");
    check_output("1 == \"1\"\n", "false\n", "== across types is false");

    // Strings compare by CONTENT, not by identity. Every case here is one the
    // suite did not have when strings moved behind a shared_ptr handle, and the
    // whole suite stayed green while `"x" == "x"` answered false — the variant's
    // operator== was comparing pointers. `1 == 1` did not catch it because a
    // Number is stored inline, and the two string literals below are separate
    // allocations, which is the entire point of the test.
    check_output("\"x\" == \"x\"\n", "true\n", "two identical string literals");
    check_output("\"x\" == \"y\"\n", "false\n", "two different string literals");
    check_output("\"\" == \"\"\n", "true\n", "two empty strings");
    check_output("\"hello\"[0:1] == \"h\"\n", "true\n",
                 "a slice equals the literal it matches");
    check_output("\"ab\".concat(\"c\") == \"abc\"\n", "true\n",
                 "a built string equals the literal it matches");
    check_output("\"x\" != \"x\"\n", "false\n", "!= agrees with ==");
    // ... and inside a list, which recurses through the same comparison.
    check_output("satellite.container.list<satellite.variable.string> p\n"
                 "satellite.container.list<satellite.variable.string> q\n"
                 "p.append(\"a\")\n"
                 "q.append(\"a\")\n"
                 "p == q\n",
                 "true\n", "lists of strings compare by content");
    check_output("satellite.container.list<satellite.variable.string> r\n"
                 "satellite.container.list<satellite.variable.string> s\n"
                 "r.append(\"a\")\n"
                 "s.append(\"b\")\n"
                 "r == s\n",
                 "false\n", "and report a difference");
    check_output("!(1 < 2)\n", "false\n", "!");
    check_output("(1 < 2).and(2 < 3)\n", "true\n", "and");
    check_output("(1 < 2).or(3 < 2)\n", "true\n", "or");
}

void eval_test_true_and_false()
{
    // --- TRUE and FALSE ------------------------------------------------------
    //
    // Accepted as bare words, but only where nothing the user owns has the
    // name — which is what keeps §1 intact.
    check_output("satellite.variable.bool b = TRUE\n"
                 "b\n",
                 "true\n", "bare TRUE is the true bool");
    check_output("satellite.variable.bool b = FALSE\n"
                 "b\n",
                 "false\n", "bare FALSE is the false bool");

    // Inside a capsule too, which is where the old error was raised.
    check_output("satellite.capsule f()\n"
                 "{\n"
                 "    satellite.variable.bool b = TRUE\n"
                 "    satellite.return(b)\n"
                 "}\n"
                 "f()\n",
                 "true\n", "bare TRUE resolves inside a capsule");

    // §1 SURVIVES: a variable the user declared called TRUE is theirs, and the
    // literal does not take the name away from them.
    check_output("satellite.capsule f()\n"
                 "{\n"
                 "    satellite.variable.number TRUE = 5\n"
                 "    satellite.return(TRUE)\n"
                 "}\n"
                 "f()\n",
                 "5\n", "a user variable named TRUE still wins");
}

void eval_test_binary_and_hex()
{
    // --- §21: binary and hexadecimal ----------------------------------------
    //
    // The two things this type exists for are the two things checked hardest:
    // the WIDTH survives, and the value is exact at any length.
    check_output("x0009999CCCDDBBDFBDBDBD\n", "x0009999CCCDDBBDFBDBDBD\n",
                 "a hex literal keeps every digit it was written with");
    check_output("x0009999CCCDDBBDFBDBDBD.digits()\n", "22\n",
                 ".digits() is the width, leading zeros included");
    check_output("x0009999CCCDDBBDFBDBDBD.bytes()\n", "11\n",
                 ".bytes() packs two hex digits to a byte");
    check_output("x0009999CCCDDBBDFBDBDBD.to_number()\n",
                 "45334948838468516429245\n",
                 ".to_number() is exact past 64 bits");
    check_output("b10101011110101011\n", "b10101011110101011\n",
                 "a binary literal round trips");
    check_output("b10101011110101011.to_number()\n", "87979\n",
                 "a binary literal converts exactly");
    check_output("b10101011110101011.digits()\n", "17\n",
                 "a binary width is its bit count");
    check_output("b101.bytes()\n", "1\n",
                 "three bits are one byte — there is no seven-bit byte");

    // Value-preserving both ways, and exact in WIDTH only from hex to binary.
    check_output("x0F.to_binary()\n", "b00001111\n",
                 "one hex digit is exactly four binary digits");
    check_output("x0F.to_binary().to_hex()\n", "x0F\n",
                 "hex -> binary -> hex is a fixpoint");
    check_output("b101.to_hex().to_binary()\n", "b0101\n",
                 "binary -> hex rounds a width up to four, and says so");

    // The width is PART OF THE VALUE. This pair is the whole argument for the
    // type existing rather than x0009 being a number literal in another base.
    check_output("x0009 == x9\n", "false\n",
                 "a width is part of the value: x0009 is not x9");
    check_output("x0009.to_number() == x9.to_number()\n", "true\n",
                 "and .to_number() is how a program asks the other question");
    check_output("x00FF == x00ff\n", "true\n",
                 "case is normalised, so how it was typed does not matter");
    check_output("x0F.concat(xAB)\n", "x0FAB\n", ".concat() joins widths");

    check_output("satellite.variable.hexadecimal h = xFF\nh\n", "xFF\n",
                 "hexadecimal is hex — the language's one alias");

    // Exact-name matching is what the two types BUY and what they COST, and
    // both halves are pinned so neither can drift.
    check_error("satellite.variable.hex h = b1010\n", "cannot initialise",
                "a binary value does not satisfy hex");
    check_error("satellite.variable.number n = xFF\n", "cannot initialise",
                "a hex value does not satisfy number");
    check_error("x0F.concat(b1010)\n", "same radix",
                ".concat() refuses a mixed radix rather than guessing");

    // §21's cost, and §20.6.2's laxness, both named at the declaration.
    check_error("satellite.variable.number x1 = 5\n", "cannot name a variable",
                "a name may not be an x followed only by hex digits");
    check_error("satellite.variable.network n\n", "no such type",
                "a phantom variable type is refused rather than becoming nil");
}
