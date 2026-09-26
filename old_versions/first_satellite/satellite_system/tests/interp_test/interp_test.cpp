// Program invocation: argv -> satellite strings -> satellite.main's parameter.
//
// The load-bearing test in here is the encode_raw one. Everything else would
// still "work" with encode(), just silently wrong.
//
// This file holds what the whole binary shares -- the check helpers, the
// failure count, the per-case namespace counter -- and main(), which calls the
// sections in the order the checks were written in. The sections themselves
// live in the interp_test_<topic>.cpp files beside this one, every one of them
// compiled into this single interp_test binary.

#include "interp_test.hpp"

#include "interpreter/interp.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;

int failures = 0;

// A namespace per case, so one case's globals cannot be another's. The same
// helper eval_test and spacesuit_test use, for the same reason.
//
// The counter stays private to this file -- nothing outside it has any reason
// to know the numbering -- while fresh_ns() itself is shared, because the
// section that drives run_source is in another file now.
static int ns_counter = 0;

std::string fresh_ns()
{
    return "i" + std::to_string(++ns_counter);
}

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

void check_run(const std::string &source, const std::vector<std::string> &args,
               const std::string &want, const std::string &what)
{
    InterpResult result = run_program(source, args);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

int main()
{
    interp_test_argument_conversion();
    interp_test_main_capsule();
    interp_test_exit_status();
    interp_test_run_file();
    interp_test_error_names_file();
    interp_test_multiline_entry();
    interp_test_prompt_session();
    interp_test_run_command();
    interp_test_run_command_end_to_end();
    interp_test_console_pacing();

    if (failures)
        return 1;
    printf("PASS: interp (argv -> satellite strings via encode_raw, argz bound "
           "to satellite.main, §2 hello world runs, --run file + exit status, "
           "repl run/interpret/--run command, display(40ms) paces a run's "
           "output on the printer thread; multi-line entry opens a body for "
           "every block head and for none that closed itself, a brace inside a "
           "string is not a brace, and a prompt session's capsules survive the "
           "line while a non-session run's do not)\n");
    return 0;
}