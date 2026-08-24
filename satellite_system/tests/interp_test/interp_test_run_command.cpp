// The REPL's `run <file>` command: what counts as the verb, how a path and its
// arguments are split, and the parsed command feeding run_file end to end.
// Part of the interp_test binary; the harness it calls is declared in
// interp_test.hpp.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace satellite;

void interp_test_run_command()
{
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
}

void interp_test_run_command_end_to_end()
{
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
}
