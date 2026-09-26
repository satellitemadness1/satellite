// The arguments object: satellite.main's one parameter, whatever the program
// calls it, holding the command line and the machine facts beside it. Part of
// the eval_test binary; the harness these call is declared in eval_test.hpp.

#include "eval_test.hpp"

#include "interpreter/interp.hpp"

#include <string>
#include <vector>

using namespace satellite;

void eval_test_arguments()
{
    // --- the arguments object -----------------------------------------------
    //
    // Driven through run_program, because the object exists only as
    // satellite.main's parameter -- run_source has no main and no argv, so
    // these cannot be written as top-level lines.
    {
        const std::vector<std::string> args = {"prog.satl", "one", "two"};

        // DECISION 1: the name is the user's and the language does not read
        // it. All six documented spellings, and one that is not documented at
        // all, must behave identically -- `banana` is in this list on purpose,
        // because it is what proves the six are documentation and not a gate.
        for (const char *name : {"arg", "args", "argz", "argument",
                                 "arguments", "argumentz", "banana"}) {
            std::string src =
                std::string("satellite.capsule satellite.main(") +
                "satellite.container.list<satellite.variable.string> " + name +
                ")\n{\n    satellite.console.display(" + name +
                ".length().to_string())\n    satellite.return(satellite)\n}\n";
            InterpResult r = run_program(src, args);
            check(r.ok && r.output == "3\n",
                  std::string("satellite.main's parameter may be called ") +
                  name);
        }

        auto program = [&](const std::string &body) {
            return "satellite.capsule satellite.main("
                   "satellite.container.list<satellite.variable.string> a)\n"
                   "{\n    " + body + "\n    satellite.return(satellite)\n}\n";
        };

        // DECISION 4, and the whole reason it is a decision: .length() and a
        // numeric index are the COMMAND LINE. If either ever starts counting
        // the machine entries, every program that loops over its arguments
        // silently starts reading the kernel release, and this is the test
        // that fails first.
        InterpResult r = run_program(
            program("satellite.console.display(a.length().to_string())"), args);
        check(r.ok && r.output == "3\n", ".length() is the command line only");

        r = run_program(program("satellite.console.display(a[2])"), args);
        check(r.ok && r.output == "two\n", "a[i] indexes the command line");

        r = run_program(
            program("satellite.console.display((a.count() > a.length())"
                    ".to_string())"), args);
        check(r.ok && r.output == "true\n",
              ".count() is larger than .length() -- the machine entries");

        // The bare selector, which is the headline spelling.
        r = run_program(program("satellite.console.display(a.operating_system)"),
                        args);
        check(r.ok && r.output == "Linux\n",
              "a.operating_system -- a bare selector on the arguments object");

        // The three spellings are one lookup, so they cannot disagree.
        r = run_program(
            program("satellite.console.display((a.username == a[\"username\"])"
                    ".and(a.username == a.get(\"username\")).to_string())"),
            args);
        check(r.ok && r.output == "true\n",
              "a.name, a[\"name\"] and a.get(\"name\") are one lookup");

        r = run_program(
            program("satellite.console.display(a.has(\"cxx_compiler\")"
                    ".to_string())"), args);
        check(r.ok && r.output == "true\n", ".has() finds a build fact");

        r = run_program(
            program("satellite.console.display(a.has(\"nonsense\").to_string())"),
            args);
        check(r.ok && r.output == "false\n", ".has() answers false, not an error");

        // DECISION 5c: absent is an error and the message lists what exists.
        r = run_program(program("satellite.console.display(a.nonsense)"), args);
        check(!r.ok && r.output.find("has no nonsense") != std::string::npos &&
                  r.output.find("cxx_compiler") != std::string::npos,
              "an unknown entry name fails, and names the ones that exist");

        // DECISION 5b: a method name wins over an entry name, and no entry is
        // named after a method. Checked by ASKING the object for its names
        // rather than by restating the list here, so adding an entry called
        // `length` fails this test rather than silently shadowing.
        r = run_program(
            program("satellite.container.list<satellite.variable.string> n = "
                    "a.names()\n"
                    "    satellite.variable.bool clash = satellite.bool.false\n"
                    "    satellite.statement.for (satellite.variable.number i = 0;"
                    " i < n.length(); i = i + 1) {\n"
                    "        satellite.statement.if (n[i] == \"length\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"count\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"names\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"get\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"has\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"first\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"last\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"contains\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"to_string\") { clash = satellite.bool.true }\n"
                    "        satellite.statement.if (n[i] == \"size\") { clash = satellite.bool.true }\n"
                    "    }\n"
                    "    satellite.console.display(clash.to_string())"), args);
        check(r.ok && r.output == "false\n",
              "no entry name collides with a method name");

        // Printing it names every entry. One line per entry, so the line count
        // is the entry count -- which also pins that display() does not print
        // the list form.
        r = run_program(program("satellite.console.display(a)"), args);
        check(r.ok && r.output.find("cxx_compiler") != std::string::npos &&
                  r.output.find("program") != std::string::npos,
              "displaying the object prints names beside data");

        // DECISION 5a: the refusal is narrowed by one case, not lifted.
        check_error("\"abc\".missing\n", "no bare field access",
                    "bare field access is still refused on everything else");
    }
}
