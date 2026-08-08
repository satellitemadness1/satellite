// Evaluator tests: milestone M0 ("the spine") and the capsule calls of M3.
//
// Everything here drives the real pipeline through interp.hpp — lexer, parser,
// evaluator, satellite.library, to_string — so a break anywhere in it shows up
// as a failure here, and none of it links gtk4 or vte.

#include "interp.hpp"
#include "library.hpp"
#include "satellite_string.hpp"
#include "system.hpp"
#include "value.hpp"

#include <cstdio>
#include <string>

#include <unistd.h>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// Each case runs in its own satellite.library namespace, because the Library
// is a process-wide singleton and variables written by one case would
// otherwise be visible to the next.
static int ns_counter = 0;

static std::string fresh_ns()
{
    return "t" + std::to_string(++ns_counter);
}

// Runs `source` and compares everything it printed against `want`.
static void check_output(const std::string &source, const std::string &want,
                         const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

// Runs `source` expecting it to fail, with `fragment` somewhere in the report.
static void check_error(const std::string &source, const std::string &fragment,
                        const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.ok || result.output.find(fragment) == std::string::npos) {
        printf("FAIL: %s\n  want error containing: %s\n  got: %s\n",
               what.c_str(), fragment.c_str(), result.output.c_str());
        failures++;
    }
}

int main()
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

    // --- arithmetic and precedence -----------------------------------------
    check_output("1 + 2 * 3\n", "7\n", "* binds tighter than +");
    check_output("(1 + 2) * 3\n", "9\n", "parentheses");
    check_output("7 / 2\n", "3.5\n", "division is not integer division");
    check_output("7 % 2\n", "1\n", "modulo");
    check_output("-3 + 1\n", "-2\n", "unary minus");
    check_output("2 - -3\n", "5\n", "minus is never folded into a number");

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

    // --- number methods ----------------------------------------------------
    check_output("3.5.floor()\n", "3\n", "floor");
    check_output("3.5.ceil()\n", "4\n", "ceil");
    check_output("(0 - 4).abs()\n", "4\n", "abs");
    check_output("10.divided_by(4)\n", "2.5\n", "divided_by");
    check_output("10.modulo(3)\n", "1\n", "modulo method");

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

    // --- satellite.variable.file, §8.3 --------------------------------------
    // A reference type with an EXPLICIT close that returns a status, because a
    // destructor cannot report that close failed with ENOSPC or EIO and
    // buffered writes commit at close.
    {
        const std::string path = "/tmp/satellite_eval_test_file.txt";
        remove(path.c_str());

        check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                         "\", \"write\")\n"
                     "f.ok()\n"
                     "f.write(\"one\\ntwo\\n\")\n"
                     "f.close()\n",
                     "true\ntrue\ntrue\n", "open, write, close");

        check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                         "\", \"read\")\n"
                     "satellite.variable.string t = f.read()\n"
                     "f.close()\n"
                     "t.length()\n"
                     "t.starts_with(\"one\")\n",
                     "true\n8\ntrue\n", "open, read, close");

        // encode_raw, never encode. §3.3's defect in its third home: a source
        // file containing \home would otherwise be rewritten to the reader's
        // home directory before the lexer ever saw it.
        check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                         "\", \"write\")\n"
                     "f.write(\"C:\\\\home\")\n"
                     "f.close()\n"
                     "satellite.variable.file g = satellite.file.open(\"" + path +
                         "\", \"read\")\n"
                     "satellite.variable.string t = g.read()\n"
                     "g.close()\n"
                     "t.length()\n",
                     "true\ntrue\ntrue\n7\n",
                     "a backslash read from a file is not expanded");

        // Append does not truncate.
        check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                         "\", \"append\")\n"
                     "f.write(\"!\")\n"
                     "f.close()\n"
                     "satellite.variable.file g = satellite.file.open(\"" + path +
                         "\", \"read\")\n"
                     "satellite.variable.string t = g.read()\n"
                     "g.close()\n"
                     "t.length()\n",
                     "true\ntrue\ntrue\n8\n", "append does not truncate");

        // A failed open is a VALUE, not an error: "does this file exist" has to
        // be answerable without killing the program that asked.
        check_output("satellite.variable.file f = "
                     "satellite.file.open(\"/tmp/satellite_no_such_file_xyz\", \"read\")\n"
                     "f.ok()\n"
                     "f.error().length() > 0\n",
                     "false\ntrue\n", "a failed open is a value, not an error");

        // Closing twice is not a failure, and nothing ever silently reopens.
        check_output("satellite.variable.file f = satellite.file.open(\"" + path +
                         "\", \"read\")\n"
                     "f.close()\n"
                     "f.close()\n"
                     "f.ok()\n",
                     "true\ntrue\nfalse\n", "close is idempotent, reopen never");

        check_error("satellite.variable.file f = satellite.file.open(\"" + path +
                        "\", \"read\")\n"
                    "f.close()\n"
                    "f.read()\n",
                    "closed file", "reading a closed file is an error");

        check_error("satellite.file.open(\"" + path + "\", \"sideways\")\n",
                    "must be", "an unknown mode is rejected");

        check_error("satellite.file.open(\"" + path + "\")\n",
                    "takes 2 arguments", "open checks its arity");

        // A declaration cannot open anything, so a file starts nil — the same
        // rule a spacesuit-typed field follows, for the same reason.
        check_output("satellite.variable.file f\nf == satellite\n", "true\n",
                     "an unopened file is nil");

        remove(path.c_str());
    }

    // --- satellite.directory ------------------------------------------------
    // The working directory is PROCESS-wide, not per-evaluator, so these cases
    // put it back where they found it. A test that leaves the process somewhere
    // else breaks every later test that opens a relative path, and it would do
    // it at a distance, in a different case, for no visible reason.
    {
        char saved[4096];
        const char *back = getcwd(saved, sizeof saved);

        check_output("satellite.directory.exists(\"/tmp\")\n", "true\n",
                     "an existing directory");
        check_output("satellite.directory.exists(\"/tmp/no_such_dir_xyz\")\n",
                     "false\n", "a missing directory");
        // A file is not a directory. stat() alone would say yes; S_ISDIR is
        // what makes the answer mean what the name says.
        check_output("satellite.directory.exists(\"/etc/hostname\")\n", "false\n",
                     "a file is not a directory");

        check_output("satellite.directory.change(\"/tmp\")\n"
                     "satellite.directory.current()\n",
                     "true\n/tmp\n", "change then read back");
        // Like a failed satellite.file.open, a refused change is a VALUE: the
        // program that asked has to survive being told no.
        check_output("satellite.directory.change(\"/tmp/no_such_dir_xyz\")\n",
                     "false\n", "changing to nowhere is false, not an error");
        check_output("satellite.directory.change(\"/etc/hostname\")\n", "false\n",
                     "changing to a file is false");

        check_error("satellite.directory.change()\n", "takes 1 argument",
                    "change checks its arity");
        check_error("satellite.directory.current(\"x\")\n", "takes 0 arguments",
                    "current takes none");
        check_error("satellite.directory.change(7)\n", "wants a satellite.variable.string",
                    "change checks its argument type");

        if (back)
            (void)!chdir(saved);
    }

    // --- satellite.help -----------------------------------------------------
    // Reachable WITHOUT parentheses, on purpose: someone who needs help may not
    // remember the calling syntax, and that is the one place a language can
    // least afford to insist on it.
    {
        InterpResult bare = run_source("satellite.help\n", fresh_ns(), true);
        check(bare.ok && bare.output.find("the whole language") != std::string::npos,
              "satellite.help works with no parentheses");
        InterpResult called = run_source("satellite.help()\n", fresh_ns(), true);
        check(called.ok && called.output == bare.output,
              "satellite.help() gives exactly the same text");

        // Given a value, it answers with that value's methods -- and the table
        // it reads from is the one call_method dispatches on, so a listed
        // method is a method that exists.
        check_output("satellite.help(1)\n",
                     std::string("satellite.variable.number\n") +
                     "  .plus(n) .minus(n) .times(n) .divided_by(n) .modulo(n)\n" +
                     "  .abs() .floor() .ceil() .round() .to_string()\n\n",
                     "help on a number lists number methods");
        InterpResult str = run_source("satellite.help(\"x\")\n", fresh_ns(), true);
        check(str.output.find(".starts_with(s)") != std::string::npos,
              "help on a string lists string methods");
        InterpResult nil = run_source("satellite.help(satellite)\n", fresh_ns(), true);
        check(nil.output.find("no methods") != std::string::npos,
              "help on nil says so");

        check_error("satellite.help(1, 2)\n", "takes 1 argument",
                    "help takes at most one value");
    }

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

    // --- booleans and comparison -------------------------------------------
    check_output("1 < 2\n", "true\n", "<");
    check_output("2 <= 2\n", "true\n", "<=");
    check_output("1 == 1\n", "true\n", "==");
    check_output("1 != 1\n", "false\n", "!=");
    check_output("1 == \"1\"\n", "false\n", "== across types is false");
    check_output("!(1 < 2)\n", "false\n", "!");
    check_output("(1 < 2).and(2 < 3)\n", "true\n", "and");
    check_output("(1 < 2).or(3 < 2)\n", "true\n", "or");

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
                 "[1, 2, 3]\n3\n1\n3\n", "append, length, index, negative index");

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
                 "[2, 3]\n[1, 2]\n[3]\n[1, 2, 3]\n[]\n", "half-open slicing");

    // An out-of-range slice clamps; an out-of-range index is an error.
    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l[0:99]\n"
                 "l[5:9]\n",
                 "[1]\n[]\n", "slice clamps");
    check_output("\"abcde\"[1:3]\n", "bc\n", "string slice is half-open");

    check_output("satellite.container.list<satellite.variable.number> l\n"
                 "l.append(2)\n"
                 "l.contains(2)\n"
                 "l.first()\n"
                 "l.last()\n",
                 "true\n2\n2\n", "contains, first, last");

    // --- display and return ------------------------------------------------
    check_output("satellite.console.display(\"hello, world!\")\n",
                 "hello, world!\n", "console.display");
    check_output("satellite.console.display(1 + 1)\n", "2\n",
                 "display renders any value");

    // satellite.return stops the program. It is a status enum, not a thrown
    // exception (§10) — what matters here is only that it stops.
    {
        std::string ns = fresh_ns();
        InterpResult r = run_source("satellite.variable.number a = 1\n"
                                    "satellite.return(2)\n"
                                    "satellite.variable.number b = 99\n",
                                    ns, true);
        check(r.ok, "return runs clean");
        check(Library::instance().get(ns, "a") != nullptr, "return: a was set");
        check(Library::instance().get(ns, "b") == nullptr,
              "return stopped before b");
    }

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
                 "[1, 2]\n[5, 6]\n", "a local list is per-activation");

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

    // --- the REPL entry point ----------------------------------------------
    check(eval_line("1 + 1\n") == "2\n", "eval_line echoes a value");

    if (failures)
        return 1;
    printf("PASS: eval (M0 spine: x.plus(1) = 2 via satellite.library.main.x; "
           "arithmetic, strings, lists, half-open slicing, control flow; "
           "M3 frames: fact(10) = 3628800, mutual recursion, per-activation "
           "locals, depth guard; 20 error cases)\n");
    return 0;
}
