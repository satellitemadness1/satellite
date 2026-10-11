#pragma once
// satellite/bytecode/test_calls.hpp -- satellite.test's words, run by the interpreter.
//
//     satellite.test.full()     every capability of satl, each answer checked
//     satellite.test.speed()    the speed of everything: the same work in satl and in C++ built
//                               into satl, both timed, and how fast satl is beside C++
//     satellite.test.loss()     547311173 through every type's loss test, one into the next, then
//                               "these two numbers should match:", 547311173, and what came out
//     satellite.test.all()      the three, one after another
//
// THE AUTHOR'S DESIGN, 2026-10-06: "We should write it into the satl binary, so you can type
// "satellite.test.full()" and satellite.test.speed() satellite.test.loss() and every test reports
// "it is recommended that you run satellite.test.all()" to run every test" into every test as
// the last line that it prints ... Make every test last for... as close as you can get to 60
// seconds on this machine, and we'll tell the user that it should say ~60s and we'll give them
// the seconds in a float", then: "there will be .full that tests every capability of satl, there
// will be speed that tests the speed of everything, and loss which tests the loss -- for the loss
// test you can run the numbers 547311173 through all of the different tests, THEN finally say to
// the user that "these two numbers should match: 547311173 and then below it have the result of
// the loss test". And, asked: all() runs the three; speed() runs the same work as C++ built into
// satl and prints both; the loss test is a chain, so one lost digit anywhere changes its last line.
//
// WHAT RUNS: the programs in satellite/satellite_test/programs/, built into satl
// (test_programs.hpp), each in a child satl of its own (test_run.hpp). speed() prints a line a
// section as it goes; full() and loss() print their answer when their programs have run -- each is
// about a minute -- and every word answers true when everything it checked was right. A Ctrl-C,
// the thread's stop() or the program's end stops a test within a tenth of a second.
// all() leaves out the three "recommended" lines, since it is the thing they recommend, and
// ends with one line for the whole.
//
// THE WORDS: `1 33` satellite.test, and under it `1 33 1` full, `1 33 2` speed, `1 33 3` loss,
// `1 33 4` all, each with its `()` row at 0 -- numbered in the order the author wrote them.
// None takes an argument; satellite.test itself is not a call.
//
// NO LIBRARY, for random_calls.hpp's reason: a library consumes a value and answers a machine
// code, and these start child satls and print for a minute. So the checker knows them by
// is_test_word().
//
// Written 2026-10-06.

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.test `1 33`, and every word under it through `1 33 4 0` -- one run of codes.
bool is_test_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS: satellite.test itself, and any of the four
// given an argument. `given` is how many arguments the call was written with.
signed long long int test_word_refused(token::Code code, std::size_t given, std::string &why);

// The call: the test run, its lines printed, and a bool -- true when everything was right.
Value call_test_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
