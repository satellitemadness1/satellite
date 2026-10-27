#pragma once
// satellite/bytecode/time_calls.hpp -- satellite.time's words, run by the interpreter (2026-10-10).
//
//     satellite.time.second(1)           a time one second long        .s(1) is its second spelling
//     satellite.time.millisecond(250)    a quarter of a second         .ms(250)
//     satellite.time.nanosecond(30)      thirty nanoseconds            .ns(30)
//     satellite.time.now("s")            the computer's clock: the time since 1970, in the unit asked for
//     satellite.time.sleep(1s)           this thread waits that long: 1s, 250ms, 30ns, or a time's name
//
// (the author, 2026-10-26) "satellite.variable.time second = satellite.time.now("seconds") and you can type
// "nanoseconds" or "milliseconds" or "s" or "ns" or "ms" but if nothing is given just default to nanoseconds" --
// and a time's .satellite.time.convert(unit) takes the same six (TIME_PLAN.md TIME-1). Both read the unit here, in
// one place: unit_call_refused, unit_literal_refused and unit_given below.
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
//   before anything runs   a count of arguments other than one -- for now and convert, more than
//                          one; satellite.time.sleep(n, unit), 003's spelling, which is told to
//                          write the unit on the number: 90ms; sleep given a literal that is not a
//                          time -- a bare number names no unit, so sleep(5) is told to write 5s,
//                          5ms or 5ns; now and convert given a literal that is not one of the six
//                          units (S301), a minus in front or not, and written with no brackets;
//                          `.satellite.time` written alone on a time, a part of convert that runs
//                          nothing
//   when the line runs     second, millisecond and nanosecond given anything but a whole number
//                          or a float (1, 1.5); sleep given a worked-out value that is not a time;
//                          sleep given a time below zero (S440, time_below_zero, 003's "negative
//                          is not a length"); now and convert given a worked-out unit that is not
//                          one of the six, in the same sentence

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.time.now `1 9 1` (TIME-1), sleep(n) `1 9 3`, sleep(n, unit) `1 9 4`, second(n) `1 9 5`,
// millisecond(n) `1 9 6` and nanosecond(n) `1 9 7` -- and s, ms and ns, their second spellings, which
// the lexer turns into the same codes (words/aliases.tsv). now's row is spelled with no brackets, so
// now, now() and now("s") all lex to it, and the count says which was written. satellite.time.new is
// not among them: it is still numbered and unbuilt.
bool is_time_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS, and the walker asks again on its way in: `given`
// is how many arguments the call was written with. success, or the code and the sentence.
signed long long int time_word_refused(token::Code code, std::size_t given, std::string &why);

// A LITERAL sleep CAN NEVER TAKE, refused by the checker before anything runs (the review,
// 2026-10-10): sleep(5), sleep(-5), sleep(1.5), sleep("5") -- one literal that is not a time, a
// minus sign allowed in front, and the bracket straight after it -- and now given a literal unit
// that is not one of the six, now("minutes") or now(5). `at` is on the argument. A value worked out
// is still the walker's to judge, in the same sentence.
signed long long int time_literal_refused(token::Code code, const std::vector<std::bitset<16>> &row, std::size_t at,
                                          std::string &why);

// THE UNIT, READ IN ONE PLACE (TIME_PLAN.md Section 2), for satellite.time.now and a time's
// .satellite.time.convert: "seconds" or "s", "milliseconds" or "ms", "nanoseconds" or "ns", written
// exactly, and nothing between the brackets is the nanosecond. `receiver` and `word` are the call as
// its sentences name it -- "" and satellite.time.now, or waited and .satellite.time.convert -- joined
// only when a sentence is said, so a call that is right spells nothing (the fresh read, 2026-10-26).
//
// HOW IT IS CALLED, the checker's and the run's: with brackets, holding one unit or none -- S110 for
// a bare `satellite.time.now` (M4's row since TIME-1) and for a count of two or more.
signed long long int unit_call_refused(bool bracketed, std::size_t given, const std::string &receiver, const char *word,
                                       std::string &why);

// A LITERAL UNIT THAT IS NOT ONE OF THE SIX, before anything runs: a string spelled any other way,
// "minutes" or "S", and any literal that is not a string, 5 or 1s, a minus in front or not -- S301,
// as satellite.file.open's "append" is. `at` is on the argument; anything worked out waits for
// unit_given.
signed long long int unit_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t at,
                                          const std::string &receiver, const char *word, std::string &why);

// THE UNIT A CALL WAS GIVEN, when its line runs -- a string name's is judged here, in the checker's
// sentence. True with `unit` set, or false with the refusal in `context`.
bool unit_given(const std::vector<Value> &arguments, const std::string &receiver, const char *word,
                satellite_time::Unit &unit, ExpressionContext &context);

// `.satellite.time` WRITTEN ALONE ON A TIME (TIME-1): a part of .satellite.time.convert(unit), which
// the lexer reads whole after `.satellite.`, and it runs nothing, as a window's `window` runs nothing
// (window_shapes.cpp's a_word_in_parts). `dot` is the period before it. The part is read only after
// `.satellite.` -- after a plain period `time` is a name (bytecode_registry.hpp) -- so that is the
// only spelling this finds. Asked of a time only, by the walker and the checker, before the period's
// rule; told the word it begins, after `receiver`, the call as written up to the period. S110.
bool a_time_part(const std::vector<std::bitset<16>> &row, std::size_t dot);
std::string time_part_written_alone(const std::string &receiver);

// The call, its arguments already evaluated: a time -- now's from the clock -- or, for sleep,
// nothing once the wait is over, or a refusal in `context`.
Value call_time_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
