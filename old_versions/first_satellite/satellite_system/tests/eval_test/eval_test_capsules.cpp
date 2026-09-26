// The statement forms and what happens when a program is wrong: if, while and
// for; capsule calls and the frame each activation gets (M3, §6); the error
// cases; and eval_line, the REPL's entry point. Part of the eval_test binary;
// the harness these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include "interpreter/interp.hpp"

#include <string>

using namespace satellite;

void eval_test_control_flow()
{
    // --- control flow ------------------------------------------------------
    check_output("satellite.statement.if (1 < 2) {\n"
                 "    satellite.console.display(\"yes\")\n"
                 "}\n",
                 "yes\n", "if taken");
    check_output("satellite.statement.if (1 > 2) {\n"
                 "    satellite.console.display(\"yes\")\n"
                 "} satellite.statement.else {\n"
                 "    satellite.console.display(\"no\")\n"
                 "}\n",
                 "no\n", "else taken");

    check_output("satellite.variable.number i = 0\n"
                 "satellite.variable.number total = 0\n"
                 "satellite.statement.while (i < 4) {\n"
                 "    total = total + i\n"
                 "    i = i + 1\n"
                 "}\n"
                 "total\n",
                 "6\n", "while accumulates 0+1+2+3");

    check_output("satellite.variable.number total = 0\n"
                 "satellite.statement.for (satellite.variable.number i = 1; "
                 "i <= 4; i = i + 1) {\n"
                 "    total = total + i\n"
                 "}\n"
                 "total\n",
                 "10\n", "three-part for");
}

void eval_test_errors()
{
    // --- errors ------------------------------------------------------------
    check_error("nope\n", "no such variable: nope", "undeclared variable");
    check_error("satellite.variable.number n = \"hello\"\n",
                "cannot initialise", "declaration type check");
    check_error("satellite.variable.number n = 1\nn = \"hello\"\n",
                "cannot assign", "assignment type check");
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l.append(\"no\")\n",
                "cannot append", "list element type check at insertion");
    check_error("1 / 0\n", "division by zero", "division by zero");
    check_error("1 % 0\n", "modulo by zero", "modulo by zero");
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l[0]\n",
                "outside a list", "index out of range is an error");
    check_error("satellite.statement.if (1) {\n"
                "    satellite.console.display(\"x\")\n"
                "}\n",
                "must be a satellite.variable.bool", "no implicit truthiness");
    check_error("1.nonesuch()\n", "has no method nonesuch", "unknown method");
    check_error("1.plus(1, 2)\n", "takes 1 argument, got 2", "arity");
    check_error("1.plus(\"x\")\n", "wants a satellite.variable.number",
                "argument type");
    check_error("satellite.nowhere.nothing()\n", "no such module function",
                "unknown module function");
    check_error("\"abc\".missing\n", "no bare field access",
                "bare field access is not in the language");

    // A mutating method needs a receiver that names storage: there is nowhere
    // to write back the result of an append to a temporary (§7).
    check_error("\"abc\"[0:1].append(1)\n", "writes back through its receiver",
                "mutator needs a storage slot");
}

void eval_test_capsules()
{
    // --- capsule calls (M3, §6) --------------------------------------------
    // The measurement §6 is built on: the same source through the Library
    // returns 1 for every input, because one capsule has one slot per local for
    // the whole program and the base case's write is the last one standing. A
    // frame per activation is what makes this 3628800.
    check_output("satellite.capsule fact(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.statement.if (n <= 1) {\n"
                 "        satellite.return(1)\n"
                 "    }\n"
                 "    satellite.return(n * fact(n - 1))\n"
                 "}\n"
                 "fact(10)\n",
                 "3628800\n", "recursive fact(10) = 3628800");

    // Every input, not just the big one: the Library version returned 1 for all
    // five of these.
    check_output("satellite.capsule fact(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.statement.if (n <= 1) {\n"
                 "        satellite.return(1)\n"
                 "    }\n"
                 "    satellite.return(n * fact(n - 1))\n"
                 "}\n"
                 "fact(1)\n"
                 "fact(2)\n"
                 "fact(3)\n"
                 "fact(5)\n",
                 "1\n2\n6\n120\n", "fact is right for every input, not just one");

    // Mutual recursion, which is also the case single-pass parsing could not
    // resolve and the separate resolve() pass can.
    check_output("satellite.capsule is_even(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.statement.if (n == 0) {\n"
                 "        satellite.return(0 == 0)\n"
                 "    }\n"
                 "    satellite.return(is_odd(n - 1))\n"
                 "}\n"
                 "satellite.capsule is_odd(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.statement.if (n == 0) {\n"
                 "        satellite.return(0 != 0)\n"
                 "    }\n"
                 "    satellite.return(is_even(n - 1))\n"
                 "}\n"
                 "is_even(10)\n"
                 "is_even(7)\n"
                 "is_odd(7)\n",
                 "true\nfalse\ntrue\n",
                 "mutual recursion, defined in either order");

    // A local is per-activation too, not just a parameter: the inner call's
    // `doubled` must not be the outer call's.
    check_output("satellite.capsule twice(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.variable.number doubled = n + n\n"
                 "    satellite.statement.if (n > 1) {\n"
                 "        satellite.return(doubled + twice(n - 1))\n"
                 "    }\n"
                 "    satellite.return(doubled)\n"
                 "}\n"
                 "twice(3)\n",
                 "12\n", "a local belongs to the activation, not the capsule");

    // A capsule sees globals only through the four-segment path, and writes
    // through it land in the real satellite.library.
    {
        InterpResult r = run_source("satellite.variable.number seen = 0\n"
                                    "satellite.capsule bump()\n"
                                    "{\n"
                                    "    satellite.library.main.seen = "
                                    "satellite.library.main.seen + 1\n"
                                    "    satellite.return(satellite)\n"
                                    "}\n"
                                    "bump()\n"
                                    "bump()\n"
                                    "satellite.library.main.seen\n",
                                    "main", true);
        check(r.ok && r.output == "2\n",
              "a capsule reaches a global by its library path");
    }

    // A local list is built in the frame, so append writes back into the slot
    // and not through the Library's locked update — and the second call starts
    // from an empty list rather than the first call's.
    check_output("satellite.capsule build(satellite.variable.number n)\n"
                 "{\n"
                 "    satellite.container.list<satellite.variable.number> l\n"
                 "    l.append(n)\n"
                 "    l.append(n + 1)\n"
                 "    satellite.return(l)\n"
                 "}\n"
                 "build(1)\n"
                 "build(5)\n",
                 "1\n2\n"
                 "5\n6\n", "a local list is per-activation");

    // The declared type of a local is the capsule's, held once for every
    // activation — and still checked, at assignment and at insertion (§7).
    check_error("satellite.capsule bad()\n"
                "{\n"
                "    satellite.variable.number n = 1\n"
                "    n = \"x\"\n"
                "    satellite.return(n)\n"
                "}\n"
                "bad()\n",
                "cannot assign", "a local's declared type is checked");
    check_error("satellite.capsule bad()\n"
                "{\n"
                "    satellite.container.list<satellite.variable.number> l\n"
                "    l.append(\"x\")\n"
                "    satellite.return(l)\n"
                "}\n"
                "bad()\n",
                "cannot append", "a local list's element type is checked");

    // Falling off the end is a return of nil, not an error.
    check_output("satellite.capsule quiet()\n"
                 "{\n"
                 "    satellite.console.display(\"ran\")\n"
                 "}\n"
                 "quiet()\n",
                 "ran\n", "a capsule with no return yields nil");

    // Arity is static, so a wrong call count is caught before anything runs.
    check_error("satellite.capsule one(satellite.variable.number n)\n"
                "{\n"
                "    satellite.return(n)\n"
                "}\n"
                "one(1, 2)\n",
                "takes 1 argument, got 2", "capsule arity is checked statically");

    // An argument is checked against the parameter's declared type at the call.
    check_error("satellite.capsule one(satellite.variable.number n)\n"
                "{\n"
                "    satellite.return(n)\n"
                "}\n"
                "one(\"x\")\n",
                "cannot pass", "argument type is checked at the call");

    // A capsule is lexically closed: a bare global name inside one is a
    // resolve-time error, not an implicit read (§6).
    check_error("satellite.variable.number outside = 1\n"
                "satellite.capsule peek()\n"
                "{\n"
                "    satellite.return(outside)\n"
                "}\n"
                "peek()\n",
                "unknown variable in capsule", "capsules are lexically closed");

    // Runaway recursion is a satellite error, not a segfault: the guard's
    // limit is measured against what the C++ stack actually holds.
    check_error("satellite.capsule forever(satellite.variable.number n)\n"
                "{\n"
                "    satellite.return(forever(n + 1))\n"
                "}\n"
                "forever(0)\n",
                "max_depth", "runaway recursion raises a satellite error");

    // `satellite` is the one reserved word, enforced at the binding site (§1).
    {
        InterpResult r = run_source("satellite.variable.number satellite = 1\n",
                                    fresh_ns(), true);
        check(!r.ok, "satellite cannot be declared");
    }

    // A syntax error is reported and nothing runs.
    {
        InterpResult r = run_source("satellite.variable.number x = \n",
                                    fresh_ns(), true);
        check(!r.ok, "syntax error is reported");
    }
}

void eval_test_eval_line()
{
    // --- the REPL entry point ----------------------------------------------
    check(eval_line("1 + 1\n") == "2\n", "eval_line echoes a value");
}
