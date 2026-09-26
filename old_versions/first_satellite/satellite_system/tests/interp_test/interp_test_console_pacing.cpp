// satellite.console.display(100ms) end to end: a program setting the pace its
// own output arrives at, on the Console's printer thread. Part of the
// interp_test binary; the harness it calls is declared in interp_test.hpp.

#include "interp_test.hpp"

#include "console_output/console.hpp"
#include "interpreter/interp.hpp"

#include <chrono>
#include <iostream>
#include <sstream>
#include <string>

using namespace satellite;

void interp_test_console_pacing()
{
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
}
