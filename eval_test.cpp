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

    // --- satellite.container.map (§8.6) -------------------------------------
    // A declaration is an empty map, not nil, so it can be .set() into
    // immediately — the same reason a list starts empty.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m\n"
                 "m.length()\n",
                 "{}\n0\n", "a declared map is empty, not nil");

    // INSERTION ORDER, and it is not cosmetic: .keys() is how a map is walked,
    // and check_output is exact string equality, so a map that rendered in hash
    // order would make this file intermittently red.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"bolt\", 40)\n"
                 "m.set(\"washer\", 100)\n"
                 "m.set(\"nut\", 7)\n"
                 "m\n"
                 "m.keys()\n"
                 "m.values()\n"
                 "m.length()\n",
                 "{bolt: 40, washer: 100, nut: 7}\n"
                 "[bolt, washer, nut]\n[40, 100, 7]\n3\n",
                 "map keeps insertion order");

    // Updating an existing key KEEPS ITS POSITION. A symbol table that
    // reordered itself whenever a binding was refined would make .keys()
    // useless for reporting.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 1)\n"
                 "m.set(\"b\", 2)\n"
                 "m.set(\"a\", 9)\n"
                 "m\n"
                 "m.length()\n",
                 "{a: 9, b: 2}\n2\n", "set on an existing key keeps position");

    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 1)\n"
                 "m.set(\"b\", 2)\n"
                 "m.get(\"b\")\n"
                 "m[\"a\"]\n"
                 "m.has(\"a\")\n"
                 "m.has(\"zz\")\n"
                 "m.remove(\"a\")\n"
                 "m\n",
                 "2\n1\ntrue\nfalse\n{b: 2}\n", "get, subscript, has, remove");

    // A NUMBER key canonicalises through to_string(), which is exact: 1 and 1.0
    // are one number (Number::operator== is compare()==0) and must therefore be
    // one key. The second set updates rather than inserts.
    check_output("satellite.container.map<satellite.variable.number, "
                 "satellite.variable.string> m\n"
                 "m.set(1, \"one\")\n"
                 "m.set(2, \"two\")\n"
                 "m.set(1.0, \"ONE\")\n"
                 "m\n"
                 "m.length()\n"
                 "m[1]\n",
                 "{1: ONE, 2: two}\n2\nONE\n", "1 and 1.0 are one key");

    // The type tag in the canonical form is what stops these colliding.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> s\n"
                 "s.set(\"12\", 1)\n"
                 "s.has(\"12\")\n",
                 "true\n", "a string key and a number key do not collide");

    // EQUALITY IS BY CONTENT, and this is the arm that matters most. A MapRef
    // is a handle to something with value semantics, exactly like the string
    // handle whose missing arm shipped, passed all eleven test binaries, and
    // was caught only by the bootstrap lexer. Two maps built independently,
    // never sharing a pointer.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "a.set(\"y\", 2)\n"
                 "b.set(\"x\", 1)\n"
                 "b.set(\"y\", 2)\n"
                 "a == b\n",
                 "true\n", "two independently built maps compare equal");

    // Order-INSENSITIVE. Order is how a map prints and is walked; it is not
    // part of what a map IS. Two symbol tables that disagree only about which
    // name was seen first hold the same symbols.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "a.set(\"y\", 2)\n"
                 "b.set(\"y\", 2)\n"
                 "b.set(\"x\", 1)\n"
                 "a == b\n"
                 "a\n"
                 "b\n",
                 "true\n{x: 1, y: 2}\n{y: 2, x: 1}\n",
                 "map equality ignores order, printing does not");

    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> a\n"
                 "satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> b\n"
                 "a.set(\"x\", 1)\n"
                 "b.set(\"x\", 2)\n"
                 "a == b\n",
                 "false\n", "maps differing in a value are not equal");

    // Walking a map with the only loop the language has (§5).
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.variable.number> m\n"
                 "m.set(\"a\", 10)\n"
                 "m.set(\"b\", 20)\n"
                 "satellite.variable.number total = 0\n"
                 "satellite.container.list<satellite.variable.string> k = m.keys()\n"
                 "satellite.statement.for (satellite.variable.number i = 0; "
                 "i < k.length(); i = i + 1) { total = total + m[k[i]] }\n"
                 "total\n",
                 "30\n", "a map is walked through .keys()");

    // Nesting, which is what makes it usable as a symbol table.
    check_output("satellite.container.map<satellite.variable.string, "
                 "satellite.container.list<satellite.variable.number>> m\n"
                 "satellite.container.list<satellite.variable.number> l\n"
                 "l.append(1)\n"
                 "l.append(2)\n"
                 "m.set(\"nums\", l)\n"
                 "m\n"
                 "m[\"nums\"][1]\n",
                 "{nums: [1, 2]}\n2\n", "a map of lists nests");

    // A missing key is an ERROR, not nil — §7 already settled the sibling case
    // for an out-of-range index. nil cannot mean absent because nil is a
    // legitimate stored value.
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.get(\"nope\")\n",
                "no such key in the map", "get on a missing key is an error");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[\"nope\"]\n",
                "no such key in the map", "subscript on a missing key errors");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.remove(\"nope\")\n",
                "no such key in the map", "remove of a missing key errors");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.set(satellite.bool.true, 1)\n",
                "as a key", "a bool cannot be a key");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.set(\"a\", \"not a number\")\n",
                "cannot store", "the value type is checked at insertion");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[0:1]\n",
                "cannot be sliced", "a map cannot be sliced");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm[\"a\"] = 1\n",
                "cannot assign to this expression",
                "m[k] = v is not in the language, exactly as l[i] = v is not");

    // The resolver rejects a malformed container type before anything runs.
    // This is what makes args[1] safe to read at every runtime site.
    check_error("satellite.container.map<satellite.variable.string> m\n",
                "takes two type arguments", "a map needs two type arguments");
    check_error("satellite.container.list<satellite.variable.string, "
                "satellite.variable.number> l\n",
                "takes one type argument", "a list takes exactly one");
    check_error("satellite.container.zzz<satellite.variable.string> z\n",
                "no such container type", "an unknown container is rejected");
    check_error("satellite.container.map<satellite.variable.bool, "
                "satellite.variable.number> m\n",
                "a map key must be", "a bool key is rejected at resolve time");

    // A selector belongs to ONE container, and which container the receiver is
    // is a question only the receiver can answer.
    //
    // Deriving it from the method name instead was a heap-buffer-overflow read,
    // found by AddressSanitizer and not by this suite: `.set` took the map path
    // whatever the receiver was, so a list's one-element generic argument
    // vector was indexed at [1]. At -O2 there was no crash — just a diagnostic
    // about storing a value in a list, produced by matching against a Type
    // whose std::strings were built from unallocated heap bytes.
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l.append(1)\n"
                "l.set(1, 2)\n",
                "satellite.container.list has no method set",
                "set on a list is refused, and does not read past its args");
    check_error("satellite.container.list<satellite.variable.string> l\n"
                "l.set(\"a\", \"b\")\n",
                "satellite.container.list has no method set",
                "set on a list of strings is refused too");
    check_error("satellite.container.list<satellite.variable.number> l\n"
                "l.remove(1)\n",
                "satellite.container.list has no method remove",
                "remove on a list is refused, with the list's own wording");
    check_error("satellite.container.map<satellite.variable.string, "
                "satellite.variable.number> m\nm.append(1)\n",
                "satellite.container.map has no method append",
                "append on a map is refused");
    // The bare, un-parameterised forms must survive the same routing.
    check_error("satellite.container.list l\nl.set(1, 2)\n",
                "has no method set", "set on a bare list is refused");

    // Only a container is generic. Pre-existing laxness — the parser validates
    // a type's SPACE and not its name, so this resolved clean before the
    // container arity check existed.
    check_error("satellite.variable.number<satellite.variable.string> x\n",
                "is not generic", "a variable type takes no type arguments");
    check_error("satellite.variable.string<satellite.variable.number, "
                "satellite.variable.bool> s\n",
                "is not generic", "and not two of them either");

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
