// satellite.container.list: what a declared list starts as, half-open slicing,
// the list literal, and assignment through an index. Part of the eval_test
// binary; the harness these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include <string>

void eval_test_lists()
{
    // --- lists -------------------------------------------------------------
    // An uninitialised declaration is a value of its declared type, so a list
    // is empty and appendable rather than nil.
    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.length()\n",
                 "0\n", "declared list starts empty");

    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l.append(2)\n"
                 "l.append(3)\n"
                 "l\n"
                 "l.length()\n"
                 "l[0]\n"
                 "l[-1]\n",
                 // The echo of a list is .lines(): one element per line, not
                 // the "[1, 2, 3]" of .to_string(). The three scalars after it
                 // are .length(), l[0] and l[-1], which are numbers and echo as
                 // themselves.
                 "1\n2\n3\n"
                 "3\n1\n3\n", "append, length, index, negative index");

    // Half-open: len(l[a:b]) == b - a, with no +1 anywhere.
    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l.append(2)\n"
                 "l.append(3)\n"
                 "l[1:3]\n"
                 "l[:2]\n"
                 "l[2:]\n"
                 "l[:]\n"
                 "l[3:3]\n",
                 // Five slices, each echoed one element per line, so the line
                 // count is the sum of their lengths: 2 + 2 + 1 + 3 + 0. The
                 // EMPTY slice is the exception and still prints "[]" -- zero
                 // elements would otherwise be zero lines, and an echo that
                 // prints nothing at all cannot be told from one that failed.
                 "2\n3\n"
                 "1\n2\n"
                 "3\n"
                 "1\n2\n3\n"
                 "[]\n", "half-open slicing");

    // An out-of-range slice clamps; an out-of-range index is an error.
    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l[0:99]\n"
                 "l[5:9]\n",
                 "1\n[]\n", "slice clamps");
    check_output("\"abcde\"[1:3]\n", "bc\n", "string slice is half-open");
}

void eval_test_list_literals()
{
    // --- list literals -------------------------------------------------------
    //
    // Written down rather than built by .append(). Asserted through .length()
    // and element reads rather than by echoing the list, so these stay true
    // whatever the REPL's echo format is.
    check_output("satellite.container.list<satellite.variable.number> l = {1, 2, 3}\n"
                 "l.length()\n"
                 "l[0]\n"
                 "l[2]\n",
                 "3\n1\n3\n", "a list literal as an initializer");

    check_output("satellite.container.list<satellite.variable.string> l = {}\n"
                 "l.length()\n",
                 "0\n", "the empty list literal");

    // The element type is NOT part of the literal, so a literal handed to a
    // string list and one handed to a number list are the same syntax.
    check_output("satellite.container.list<satellite.variable.string> l = "
                 "{\"a\", \"b\"}\n"
                 "l[1]\n",
                 "b\n", "a literal of strings");

    // Elements are ordinary expressions in the enclosing scope: a name, a
    // call, arithmetic, and another literal all work.
    check_output("satellite.variable.number x = 7\n"
                 "satellite.container.list<satellite.variable.number> l = "
                 "{x, x + 1, 2 * x}\n"
                 "l[0]\n"
                 "l[1]\n"
                 "l[2]\n",
                 "7\n8\n14\n", "literal elements are expressions");

    check_output("satellite.container.list<satellite.variable.number> l = {1, 2}\n"
                 "l.length()\n",
                 "2\n", "a literal survives being declared and read back");

    // Straight into a call, which is where view_forge writes 1406 of them.
    check_output("satellite.capsule take("
                 "satellite.container.list<satellite.variable.number> l)\n"
                 "{\n"
                 "    satellite.console.display(\"got \" + l.length())\n"
                 "    satellite.return(satellite)\n"
                 "}\n"
                 "take({4, 5, 6})\n",
                 "got 3\n", "a list literal as a call argument");

    check_error("satellite.container.list<satellite.variable.number> l = {1, 2\n",
                "to close the list", "an unclosed list literal is an error");
}

void eval_test_list_assignment()
{
    // --- l[i] = x ------------------------------------------------------------
    check_output("satellite.container.list<satellite.variable.number> l = {1, 2, 3}\n"
                 "l[1] = 99\n"
                 "l[0]\n"
                 "l[1]\n"
                 "l[2]\n",
                 "1\n99\n3\n", "a list element can be assigned");

    // The same index rules the read side has, deliberately.
    check_output("satellite.container.list<satellite.variable.number> l = {1, 2, 3}\n"
                 "l[-1] = 9\n"
                 "l[2]\n",
                 "9\n", "a negative index assigns from the end");

    check_error("satellite.container.list<satellite.variable.number> l = {1}\n"
                "l[5] = 1\n",
                "outside a list of length 1",
                "an out-of-range index assignment is an error, not a grow");

    // A map keeps .set(), and keeps the words the old message used.
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[\"a\"] = 1\n",
                "cannot assign to this expression",
                "a map is still written with .set()");
}
