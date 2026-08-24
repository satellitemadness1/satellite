// Running a program that came from a file: the script's own name in argz[0], a
// missing file as a clean error, and an error message that names the file it
// happened in. Part of the interp_test binary; the harness it calls is
// declared in interp_test.hpp.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <unistd.h>

using namespace satellite;

void interp_test_run_file()
{
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
}

void interp_test_error_names_file()
{
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
}
