#pragma once
// satellite/bytecode/time_calls.hpp -- satellite.time's words, run by the interpreter (2026-10-10).
//
//     satellite.time.second(1)           a time one second long        .s(1) is its second spelling
//     satellite.time.millisecond(250)    a quarter of a second         .ms(250)
//     satellite.time.nanosecond(30)      thirty nanoseconds            .ns(30)
//     satellite.time.sleep(1s)           this thread waits that long: 1s, 250ms, 30ns, or a time's name
//
// (the author, 2026-10-10) "we need to build satellite.time.sleep(1s or 1ms or 1ns) since we are
// taking in 3 types, we don't really need to figure out what the minimum time is, if the user
// enters 1ns and the function takes longer than that, then... oh well". So a sleep is AT LEAST its
// length and promises nothing about how much longer: the kernel wakes a thread when it can, and a
// nanosecond's sleep takes as long as asking the clock does.
//
// THE WORDS HAVE NO LIBRARY, for infinity_calls.hpp's reason: second(n) answers a TIME, and a
// library's scenarios answer machine codes (number_row.hpp); sleep is the interpreter's own thread
// waiting, which it must be able to stop. So the checker knows them by is_time_word().
//
// A SLEEP CAN ALWAYS BE STOPPED. It waits toward one deadline on the monotonic clock -- the clock
// NTP never steps, 003's choice -- and looks up every tenth of a second, as program join() does
// (program_calls.cpp): a thread asked to stop and satellite.return(satellite) on another thread end
// it there, as they end a join, and so does Ctrl-C at the prompt, as it ends a loop typed there. A
// sleep shorter than a tenth of a second is one wait.
// From a file, Ctrl-C ends satl itself, and closing satl's console hangs it up (SIGHUP), so a
// sleep never holds either open.
//
// WHAT IS REFUSED, AND WHERE:
//   before anything runs   a count of arguments other than one; satellite.time.sleep(n, unit),
//                          003's spelling, which is told to write the unit on the number: 90ms;
//                          sleep given a literal that is not a time -- a bare number names no
//                          unit, so sleep(5) is told to write 5s, 5ms or 5ns
//   when the line runs     second, millisecond and nanosecond given anything but a whole number
//                          or a float (1, 1.5); sleep given a worked-out value that is not a time;
//                          sleep given a time below zero (S440, time_below_zero, 003's "negative
//                          is not a length")

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.time.sleep(n) `1 9 3`, sleep(n, unit) `1 9 4`, second(n) `1 9 5`, millisecond(n)
// `1 9 6` and nanosecond(n) `1 9 7` -- and s, ms and ns, their second spellings, which the lexer
// turns into the same codes (words/aliases.tsv). satellite.time.now and .new are not among them:
// they are still numbered and unbuilt.
bool is_time_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS, and the walker asks again on its way in: `given`
// is how many arguments the call was written with. success, or the code and the sentence.
signed long long int time_word_refused(token::Code code, std::size_t given, std::string &why);

// A LITERAL sleep CAN NEVER TAKE, refused by the checker before anything runs (the review,
// 2026-10-10): sleep(5), sleep(-5), sleep(1.5), sleep("5") -- one literal that is not a time, a
// minus sign allowed in front, and the bracket straight after it. `at` is on the argument. A value
// worked out is still the walker's to judge, in the same sentence.
signed long long int time_literal_refused(token::Code code, const std::vector<std::bitset<16>> &row, std::size_t at,
                                          std::string &why);

// The call, its arguments already evaluated: a time, or -- for sleep -- nothing once the wait is
// over, or a refusal in `context`.
Value call_time_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
