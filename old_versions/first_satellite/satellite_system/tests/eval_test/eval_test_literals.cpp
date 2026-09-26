// eval_test_literals.cpp -- what a brace literal MEANS in a typed position.
// Part of the eval_test binary; the harness and main() are in eval_test.cpp.
//
// The rule under test is one sentence from ast_expr.hpp, finally made true:
// the element type belongs to the variable or parameter the literal is handed
// to, not to the literal. So the SAME four characters are a map in one line
// and a list in the next, and which one they are is never a property of how
// they were written.
//
// Half of these cases exist to pin the NON-change. `{"a", "b"}` into a
// list<list<string>> was a list before this feature and is a list after it,
// and a reshape that reached one line too far would take that away silently.

#include "eval_test.hpp"

void eval_test_brace_literals()
{
    // THE HEADLINE. A pair into a list of maps is one map of one entry.
    check_output("satellite.container.list<satellite.container.map<"
                 "satellite.variable.string, satellite.variable.number>> lm\n"
                 "lm.append({\"str1\", 99})\n"
                 "lm.to_string()\n",
                 "[{str1: 99}]\n", "a pair appended to a list of maps");

    // Key and value in turn, for more than one entry.
    check_output("satellite.container.list<satellite.container.map<"
                 "satellite.variable.string, satellite.variable.number>> lm\n"
                 "lm.append({\"a\", 1, \"b\", 2})\n"
                 "lm.to_string()\n",
                 "[{a: 1, b: 2}]\n", "four elements are two entries");

    // A pair per entry, which is the other spelling. Both readings are tried:
    // this one is even-length too, so the flat reading runs FIRST, produces a
    // list-shaped key that §8.6 refuses, and the refusal is what selects this.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m = {{\"c\", 3}, {\"d\", 4}}\n"
                 "m.to_string()\n",
                 "{c: 3, d: 4}\n", "a pair per entry");

    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m = {\"a\", 1, \"b\", 2}\n"
                 "m.to_string()\n",
                 "{a: 1, b: 2}\n", "an initialiser, flat");

    // `{}` is the one literal whose shape cannot be read off its contents,
    // which is exactly why the declaration is allowed to say.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m = {}\n"
                 "m.length()\n",
                 "0\n", "an empty brace is an empty map when a map is declared");

    // Assignment, not only declaration.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m = {\"z\", 26}\n"
                 "m.to_string()\n",
                 "{z: 26}\n", "assignment shapes it too");

    // RECURSION, which is what makes the crazy combinations work: the map's
    // VALUE position is a list<map<...>>, so shaping the outer literal asks
    // the same question again about the inner one. Nothing was written for
    // this depth, or for any other.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.container.list<satellite.container.map<"
                 "satellite.variable.string, satellite.variable.number>>> d\n"
                 "d.set(\"wave\", {{\"Tenacity\", 85845}})\n"
                 "d.to_string()\n",
                 "{wave: [{Tenacity: 85845}]}\n",
                 "a map of lists of maps, from one literal");

    // THE NON-CHANGE, and it is the half that matters most. The identical
    // literal handed to a list of lists is still a list of lists.
    check_output("satellite.container.list<satellite.container.list<"
                 "satellite.variable.string>> ll\n"
                 "ll.append({\"a\", \"b\"})\n"
                 "ll.to_string()\n",
                 "[[a, b]]\n", "a pair into a list of lists is still a list");

    // A literal in no DECLARED position is a list, as it always was. Written
    // as an argument rather than as a bare statement, because a '{' where a
    // statement is expected opens a block -- which is grammar this feature
    // does not touch and was never trying to.
    check_output("satellite.console.display({1, 2}.to_string())\n",
                 "[1, 2]\n", "an undeclared literal is a list");

    // A LIST THAT ARRIVED THROUGH A VARIABLE IS LEFT ALONE. Only what was
    // WRITTEN as braces is reshaped, because a value that changed type on the
    // way into a call the program can read and see it did not is the magic §1
    // spends the whole language avoiding.
    check_error("satellite.container.list<satellite.variable.number> pair\n"
                "pair.append(1)\n"
                "pair.append(2)\n"
                "satellite.container.list<satellite.container.map<"
                "satellite.variable.string, satellite.variable.number>> lm\n"
                "lm.append(pair)\n",
                "cannot append", "a variable holding a list is not reshaped");

    // Both readings refused, and the message names both so the reader can tell
    // which mistake they made.
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m = {\"a\", 1, \"b\"}\n",
                "key and value in turn", "an odd count names both spellings");

    // The declared types still have the last word. The shape is offered; it is
    // not a licence to store the wrong thing.
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m = {\"a\", \"not a number\"}\n",
                "key and value in turn", "a wrong value type is still refused");
}
