// Program invocation: argv -> satellite strings -> satellite.main's parameter.
//
// The load-bearing test in here is the encode_raw one. Everything else would
// still "work" with encode(), just silently wrong.

#include "console_output/console.hpp"
#include "interpreter/interp.hpp"
#include "satellite_library/library.hpp"
#include "satellite_string/satellite_string.hpp"
#include "system_facts/system.hpp"
#include "satellite_value/value.hpp"

#include <chrono>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unistd.h>
#include <vector>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

static void check_run(const std::string &source,
                      const std::vector<std::string> &args,
                      const std::string &want, const std::string &what)
{
    InterpResult result = run_program(source, args);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

static std::string decoded(const List &list, size_t i)
{
    return (i < list.size() && list[i]) ? to_string(*list[i]) : "<missing>";
}

int main()
{
    // --- the conversion ----------------------------------------------------
    // An argument is data the shell has already finished escaping. Running the
    // backslash-name expansion over it again rewrites it: encode() turns
    // C:\home into C:/home/madness with no error and no warning. This is the
    // same defect §3.3 fixed for source text, and argv is where it bites next.
    {
        const std::string raw = "--path=C:\\home";
        List args = args_to_list({raw});
        check(args.size() == 1, "one argument in, one value out");
        check(decoded(args, 0) == raw,
              "argv keeps its backslashes (encode_raw, not encode)");
        check(decode(encode(raw)) != raw,
              "encode() really would have corrupted it");
        check(decoded(args, 0).find(home_dir()) == std::string::npos,
              "argv does not expand \\home");
    }

    // Every argument becomes exactly one satellite.variable.string.
    {
        List args = args_to_list({"alpha", "beta", "gamma"});
        check(args.size() == 3, "three arguments");
        check(decoded(args, 0) == "alpha" && decoded(args, 2) == "gamma",
              "arguments keep their order");
        check(args_to_list({}).empty(), "no arguments is an empty list");
    }

    // Arguments that are not identifiers survive too: spaces, quotes, digits.
    {
        List args = args_to_list({"two words", "a\"b", "42", ""});
        check(decoded(args, 0) == "two words", "argument with a space");
        check(decoded(args, 1) == "a\"b", "argument with a quote");
        check(decoded(args, 2) == "42", "a numeric argument stays a string");
        check(decoded(args, 3).empty(), "an empty argument stays empty");
    }

    // --- satellite.main ----------------------------------------------------
    // The §2 hello world, byte for byte, finally invocable.
    check_run("satellite.include(satellite)\n"
              "\n"
              "satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(\"hello, world!\")\n"
              "\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"hello.sat"}, "hello, world!\n", "the DESIGN.md §2 hello world");

    // argz is bound, indexable, and has a length.
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(argz.length())\n"
              "    satellite.console.display(argz[0])\n"
              "    satellite.console.display(argz[2])\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog", "one", "two"}, "3\nprog\ntwo\n", "argz is bound");

    // A backslash argument survives all the way into the program.
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> argz)\n"
              "{\n"
              "    satellite.console.display(argz[1])\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog", "C:\\home"}, "C:\\home\n",
              "a backslash argument reaches the program intact");

    // The parameter is optional, and its name is the user's to choose.
    check_run("satellite.capsule satellite.main()\n"
              "{\n"
              "    satellite.console.display(\"no args\")\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "no args\n", "satellite.main with no parameter");
    check_run("satellite.capsule satellite.main("
              "satellite.container.list<satellite.variable.string> whatever)\n"
              "{\n"
              "    satellite.console.display(whatever.length())\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "1\n", "the parameter name is the user's");

    // Top-level statements run before main, and a program with no main is
    // still a legal program.
    check_run("satellite.console.display(\"top\")\n"
              "satellite.capsule satellite.main()\n"
              "{\n"
              "    satellite.console.display(\"main\")\n"
              "    satellite.return(satellite)\n"
              "}\n",
              {"prog"}, "top\nmain\n", "top-level runs before main");
    check_run("satellite.console.display(\"only top level\")\n", {"prog"},
              "only top level\n", "a program with no satellite.main is legal");

    // The signature is fixed by §2: a wrong parameter type is caught rather
    // than quietly accepted because today's argument list happens to be empty.
    {
        InterpResult r = run_program(
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.number> argz)\n"
            "{\n"
            "    satellite.return(satellite)\n"
            "}\n",
            {});
        check(!r.ok, "satellite.main with the wrong parameter type is rejected");
        check(r.output.find("must be") != std::string::npos,
              "and says what the parameter must be");
    }

    // --- exit status -------------------------------------------------------
    // satellite.return(satellite) is success (§2).
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(satellite)\n"
                                     "}\n",
                                     {"prog"});
        check(r.ok && r.status == 0, "return(satellite) is status 0");
    }
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(3)\n"
                                     "}\n",
                                     {"prog"});
        check(r.ok && r.status == 3, "return(3) is status 3");
    }
    {
        InterpResult r = run_program("nope\n", {"prog"});
        check(!r.ok && r.status == 1, "a failing program is status 1");
    }
    // Clamped, not masked: `n & 0xff` would turn 256 into 0 and report success
    // for a program that said it failed.
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(256)\n"
                                     "}\n",
                                     {"prog"});
        check(r.status == 255, "return(256) clamps to 255, not 0");
    }
    {
        InterpResult r = run_program("satellite.capsule satellite.main()\n"
                                     "{\n"
                                     "    satellite.return(0 - 5)\n"
                                     "}\n",
                                     {"prog"});
        check(r.status == 1, "a negative status is failure, not success");
    }

    // --- run_file ----------------------------------------------------------
    {
        std::string path = "/tmp/satellite_interp_test_" +
                           std::to_string(getpid()) + ".sat";
        {
            std::ofstream out(path);
            out << "satellite.capsule satellite.main("
                   "satellite.container.list<satellite.variable.string> argz)\n"
                   "{\n"
                   "    satellite.console.display(argz.length())\n"
                   "    satellite.console.display(argz[0])\n"
                   "    satellite.console.display(argz[1])\n"
                   "    satellite.return(satellite)\n"
                   "}\n";
        }

        // argz[0] is the script itself, mirroring argv[0].
        InterpResult r = run_file(path, {"extra"});
        check(r.ok, "run_file runs a program");
        check(r.output == "2\n" + path + "\nextra\n",
              "run_file puts the script path in argz[0]");

        std::remove(path.c_str());

        InterpResult missing = run_file("/nonexistent/nope.sat", {});
        check(!missing.ok && missing.status == 2,
              "a missing file is a clean error, not a crash");
        check(missing.output.find("cannot read") != std::string::npos,
              "and says it cannot read the file");
    }

    // --- an error names the file it happened in (§16) -----------------------
    //
    // The Span file id arriving before the loader does. run_file knows the
    // path, so a failure can say WHERE rather than just which line — which is
    // the whole reason the id exists, and it pays off with one file as readily
    // as with several.
    {
        std::string path = "/tmp/satellite_interp_named_" +
                           std::to_string(getpid()) + ".sat";
        {
            std::ofstream out(path);
            out << "satellite.capsule satellite.main("
                   "satellite.container.list<satellite.variable.string> argz)\n"
                   "{\n"
                   "    satellite.variable.number x = nope\n"
                   "    satellite.return(satellite)\n"
                   "}\n";
        }

        InterpResult r = run_file(path, {});
        check(!r.ok, "the bad program fails");
        check(r.output.find(path + ":3") != std::string::npos,
              "the error names the file and the line, not just the line");

        std::remove(path.c_str());

        // The same source with no path is the REPL's case, and must NOT invent
        // a file name. "line 3" is the truth there, not a degradation.
        InterpResult anonymous =
            run_program("satellite.capsule satellite.main("
                        "satellite.container.list<satellite.variable.string> argz)\n"
                        "{\n"
                        "    satellite.variable.number x = nope\n"
                        "    satellite.return(satellite)\n"
                        "}\n",
                        {"prog"});
        check(!anonymous.ok, "the same program still fails without a path");
        check(anonymous.output.find("line 3") != std::string::npos,
              "a source with no file renders a bare line number");
    }

    // --- the REPL's run command --------------------------------------------
    // The window has no shell behind it, so `run <file>` at the prompt is the
    // only way to interpret a file from inside it.
    {
        RunCommand c = parse_run_command("run hello.satl");
        check(c.matched && c.error.empty(), "run <file> is a run command");
        check(c.path == "hello.satl" && c.args.empty(), "and names the file");

        // Three spellings of one verb: the word, and the shell's own flag.
        check(parse_run_command("interpret hello.satl").path == "hello.satl",
              "interpret is an alias for run");
        check(parse_run_command("--run hello.satl").path == "hello.satl",
              "--run is an alias for run");

        // Leading whitespace is not a different command.
        check(parse_run_command("   run  hello.satl ").path == "hello.satl",
              "surrounding whitespace is ignored");
    }

    // Arguments after the file are the program's, in order.
    {
        RunCommand c = parse_run_command("run prog.satl one two");
        check(c.args.size() == 2 && c.args[0] == "one" && c.args[1] == "two",
              "arguments after the file reach the program");
    }

    // A path with a space needs quotes; a path with a backslash must NOT be
    // unescaped, or C:\home stops naming the file the user typed (§3.3).
    {
        check(parse_run_command("run \"my programs/a.satl\"").path ==
                  "my programs/a.satl",
              "quotes group a path containing a space");
        check(parse_run_command("run 'my programs/a.satl'").path ==
                  "my programs/a.satl",
              "single quotes group too");
        check(parse_run_command("run C:\\home\\a.satl").path ==
                  "C:\\home\\a.satl",
              "a backslash is a character, not an escape");
        RunCommand empty_arg = parse_run_command("run a.satl \"\"");
        check(empty_arg.args.size() == 1 && empty_arg.args[0].empty(),
              "an explicitly empty argument survives");
    }

    // The verb with nothing after it is a usage message, not an attempt to
    // evaluate `run` as satellite source.
    {
        RunCommand c = parse_run_command("run");
        check(c.matched && !c.error.empty(), "a bare run is a usage error");
        check(c.error.find("usage") != std::string::npos, "and says usage");
        check(parse_run_command("run   ").matched, "run plus spaces likewise");
        RunCommand bad = parse_run_command("run \"unclosed.satl");
        check(bad.matched && !bad.error.empty(),
              "an unterminated quote is refused, not half-run");
        check(bad.path.empty(), "and names no file");
    }

    // Ordinary source is left alone -- including a line that merely contains
    // the word, and the language's own runtime name.
    {
        check(!parse_run_command("satellite.console.display(\"run x\")").matched,
              "satellite source is not a run command");
        check(!parse_run_command("").matched, "an empty line is not a command");
        check(!parse_run_command("   ").matched, "nor a blank one");
        check(!parse_run_command("runner.satl").matched,
              "the verb must be a whole word");
        check(!parse_run_command("satellite.run(x)").matched,
              "a satellite.run call is source, not a command");
    }

    // End to end: the parsed command feeds run_file, argz[0] is the script.
    {
        std::string path = "/tmp/satellite_run_cmd_test_" +
                           std::to_string(getpid()) + ".satl";
        {
            std::ofstream out(path);
            out << "satellite.capsule satellite.main("
                   "satellite.container.list<satellite.variable.string> argz)\n"
                   "{\n"
                   "    satellite.console.display(argz[0])\n"
                   "    satellite.console.display(argz[1])\n"
                   "    satellite.return(satellite)\n"
                   "}\n";
        }

        RunCommand c = parse_run_command("run " + path + " here");
        InterpResult r = run_file(c.path, c.args);
        check(r.ok && r.output == path + "\nhere\n",
              "the parsed command runs the file with its arguments");

        std::remove(path.c_str());
    }

    // --- satellite.console.display(100ms), end to end -----------------------
    //
    // The whole path: source -> lexer -> a duration literal -> the evaluator ->
    // the Console this run was given. console_test proves the printer waits;
    // what is proved here is that a PROGRAM can make it wait, which no test
    // that reads output() back can see -- with a Console attached there is
    // nothing in output() to read.
    //
    // std::cout is captured because that is where a Console writes.
    {
        std::ostringstream sink;
        std::streambuf *saved = std::cout.rdbuf(sink.rdbuf());

        const auto start = std::chrono::steady_clock::now();
        {
            Console console;
            InterpResult r = run_program("satellite.console.display(40ms)\n"
                                         "satellite.console.display(\"one\")\n"
                                         "satellite.console.display(\"two\")\n"
                                         "satellite.console.display(\"three\")\n",
                                         {}, {}, &console);
            check(r.ok, "a paced program runs clean");
            // The displayed text went to the Console, so the result holds the
            // error report alone -- which is empty.
            check(r.output.empty(), "a Console run leaves nothing in output()");
        }
        const double ms = std::chrono::duration<double, std::milli>(
                              std::chrono::steady_clock::now() - start)
                              .count();

        std::cout.rdbuf(saved);
        check(sink.str() == "one\ntwo\nthree\n", "the paced lines all arrive");
        check(ms >= 75, "three lines at 40ms cost two pauses");
        check(ms < 130, "and only two");
    }

    if (failures)
        return 1;
    printf("PASS: interp (argv -> satellite strings via encode_raw, argz bound "
           "to satellite.main, §2 hello world runs, --run file + exit status, "
           "repl run/interpret/--run command, display(40ms) paces a run's "
           "output on the printer thread)\n");
    return 0;
}
