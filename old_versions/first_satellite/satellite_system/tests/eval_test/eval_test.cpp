// Evaluator tests: milestone M0 ("the spine") and the capsule calls of M3.
//
// Everything here drives the real pipeline through interp.hpp — lexer, parser,
// evaluator, satellite.library, to_string — so a break anywhere in it shows up
// as a failure here, and none of it links gtk4 or vte.
//
// This file holds what the whole binary shares -- the check helpers, the
// failure count, the per-case namespace counter -- and main(), which calls the
// sections in the order the checks were written in. The sections themselves
// live in the eval_test_<topic>.cpp files beside this one, every one of them
// compiled into this single eval_test binary.

#include "eval_test.hpp"

#include "interpreter/interp.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// Each case runs in its own satellite.library namespace, because the Library
// is a process-wide singleton and variables written by one case would
// otherwise be visible to the next.
//
// The counter stays private to this file -- nothing outside it has any reason
// to know the numbering -- while fresh_ns() itself is shared, because the
// sections that drive run_source are in other files now.
static int ns_counter = 0;

std::string fresh_ns()
{
    return "t" + std::to_string(++ns_counter);
}

void check_output(const std::string &source, const std::string &want,
                  const std::string &what)
{
    InterpResult result = run_source(source, fresh_ns(), true);
    if (result.output != want) {
        printf("FAIL: %s\n  want: %s\n  got:  %s\n", what.c_str(),
               want.c_str(), result.output.c_str());
        failures++;
    }
}

void check_error(const std::string &source, const std::string &fragment,
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
    eval_test_spine();
    eval_test_arithmetic();
    eval_test_number_rendering();
    eval_test_number_methods();
    eval_test_bool_values();
    eval_test_file_basics();
    eval_test_file_new();
    eval_test_file_reopen();
    eval_test_directory();
    eval_test_delete();
    eval_test_help();
    eval_test_strings();
    eval_test_comparison();
    eval_test_control_flow();
    eval_test_lists();
    eval_test_list_literals();
    eval_test_list_assignment();
    eval_test_true_and_false();
    eval_test_display_end();
    eval_test_maps();
    eval_test_brace_literals();
    eval_test_display();
    eval_test_pace();
    eval_test_errors();
    eval_test_capsules();
    eval_test_size_and_digits();
    eval_test_spacesuit_size();
    eval_test_binary_and_hex();
    eval_test_eval_line();
    eval_test_arguments();

    if (failures)
        return 1;
    printf("PASS: eval (M0 spine: x.plus(1) = 2 via satellite.library.main.x; "
           "arithmetic, strings, lists, half-open slicing, control flow; "
           "§21 binary and hex with the width part of the value; "
           "M3 frames: fact(10) = 3628800, mutual recursion, per-activation "
           "locals, depth guard; 20 error cases; the arguments object -- "
           "any parameter name, .length() still the command line, "
           "bare selectors, and no entry shadowing a method)\n");
    return 0;
}
