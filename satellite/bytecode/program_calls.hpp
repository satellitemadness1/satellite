#pragma once
// satellite/bytecode/program_calls.hpp -- the author's programs, in 004 (2026-10-01).
//
//     satellite.variable.program my_program = {"/dir/program", "arg1", "arg2"}
//     satellite.variable.program run_program = "/dir/some_program"
//     my_program.start()       starts it and answers at once; its output is shown in satl's own
//                              console, between satl's own lines, as it comes
//     my_program.start("hide") the same, with its output and its errors thrown away (STEP 2)
//     my_program.ok()          whether it could be started
//     my_program.error()       why not -- or, once it has ended, a code that was not 0 or the
//                              signal that ended it; "" when there is nothing to say
//     my_program.join()        waits for it to end, and answers its exit code
//     my_program.code()        the same as join()
//     my_program.return()      the same as join()
//     my_program.end()         stops it and everything under it, and answers its exit code (STEP 3);
//     my_program.exit()        .exit(), .quit() and .shutdown() are the same
//
// THE AUTHOR'S WORDS, 2026-10-01, in order: "satellite.variable.program my_program = {"/dir/program",
// "arg1", "arg2"} // or a list of str could optionally be put here, or just the name of the program
// in quotes"; "the output will be displayed unless my_program.start("hide") is called"; then
// "just make .join() .end() and .code() wait for the program to finish, all 3 do the same thing,
// and return the error code, and have .start() return to satl while the output from the program
// is displayed ... forcing the user to use both .start() and .join() together"; ".code() .return()
// (not .exit(), that sounds like .quit()) will be for the return"; and last ".shutdown() kill the
// process, and same with .end() ... .end() .exit() and .quit() and shutdown() all do the same
// thing". Asked which window: "satl's own console". With no display: "Use satl's own output".
//
// BUILT IN STEPS, as he asked ("one step at a time"): 1 start, ok, error, join, code, return;
// 2 start("hide"); 3 end, exit, quit, shutdown; 4 pass(text) -- "typed input while the program is
// running"; 5 satellite.variable.bash.
//
// MY CHOICES, HIS TO OVERRULE (none of these is his ruling):
//   - a run that could not start is not a refusal: ok() says false, error() says why, and join()
//     answers what a shell answers -- 127 for no such program, 126 for one that may not be run;
//   - a program ended by a signal answers 128 + the signal, as a shell does, and error() names it;
//   - every method needs its brackets, as a thread's do: p.ok is refused, p.ok() is right;
//   - ok(), error(), join() before any start() are refused, S741, as a thread's join() is;
//   - start() while it still runs is refused, S740; once it has ended, start() runs it again;
//   - "FORCING THE USER TO USE BOTH": a program the run started and no join() was ever reached for
//     is stopped at the end of the run -- it and every process under it, asked with SIGTERM and
//     killed five seconds later -- and the run fails, S742, at the start() that began it, unless
//     the run was already failing for its own reason. A join() that was reached counts, even one
//     that gave up waiting because satellite.return(satellite) ended the run;
//   - "CLOSE EVERYTHING" (the author's, for threads): at the end of the run every program still
//     running is stopped, joined or not -- programs first, so a thread waiting in a join() is let
//     go, then the threads, then any program a thread started while it closed;
//   - its output and its errors are one stream, in the order it wrote them, shown whole line by
//     whole line; a line with no end yet (a prompt) is shown once the program has been quiet for
//     50 ms, and one longer than 64 KiB is shown in pieces;
//   - what it leaves running (sh -c "x &") goes on writing to satl's console after the program
//     itself has ended and been joined, as on a terminal, until the run ends.

#include "expression.hpp"
#include "token_codes.hpp"
#include "../satellite_variable_program/satellite_program.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// `satellite.variable.program`, 1 6 23.
bool is_program_type(token::Code word);

// A STRING OR A LIST OF STRINGS STORED IN A PROGRAM NAME BECOMES A PROGRAM: what program_walk.cpp's
// on-store step asks of every type. Untouched for a name of any other type.
signed long long int program_on_store(token::Code holds, Value &value, std::string &why);

// The most arguments each method takes: start 1 (start() or start("hide")), every other 0. Any
// other method: -1. program_method_takes says it in words, for a refusal.
int program_method_arity(token::Code method);
std::string program_method_takes(token::Code method);

// For a refusal: the methods a program has.
std::string program_methods_are();

Value call_program_method(token::Code method, const ProgramHandle &which, const std::vector<Value> &arguments,
                          bool had_parentheses, const std::string &name, ExpressionContext &context);

// THE END OF THE RUN (structured-library.cpp): every program the run started and nobody joined is
// stopped, and reported -- when `run_ended_with` is success, the run's own ending. Answers
// program_never_joined when one was, or success.
signed long long int close_every_program(signed long long int run_ended_with);

} // namespace satellite004
