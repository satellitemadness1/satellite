#pragma once
// satellite/bytecode/number_calls.hpp -- WHAT A NUMBER ANSWERS (MILESTONES.md M17), reached BY ITS NUMBER
// since 2026-10-11 (NEW_MILESTONES.md NM-8).
//
// The author, 2026-10-11: "we still need to get the word satellite onto everything, before we do anything...
// .pointer, numbers floats, everything builtin needs the word satellite still". So a number's methods are
// the words satellite.variable.number.<it>, 1 6 4 n in words/words.tsv -- 003's twenty, frozen, and three
// more from 1 6 4 21 -- each a file under satellite-numbers/words/ named by its number, and written
// `n.satellite.floor()`:
//
//     n.satellite.string  .number  .binary  .hex     its text, itself, its bits, its hex digits (1 6 4 17 to 20;
//                                                     1 6 4 6, to_string, is 17's second name)
//     n.satellite.add(x)                               n + x, the operator's own (1 6 4 21)
//     n.satellite.reverse()                            its digits back to front, the sign kept (1 6 4 22)
//     n.satellite.floor() .ceil() .round() .truncate() a whole number is its own, so each answers n
//     n.satellite.abs()                                n without its sign
//     n.satellite.max(x)  .min(x)                      the larger, the smaller, of n and x
//     n.satellite.clamp(low, high)                     n held between low and high
//     n.satellite.power(x)  .modulus(x)                n ^ x and n % x, the operators' own
//     n.satellite.shift_left(k)  .shift_right(k)       n times, and divided by, 2 ^ k
//     n.satellite.digits()  .bytes()                   how many decimal digits; how many bytes in memory
//     n.satellite.sqrt()                               numbered, and refused: it is not built (1 6 4 14)
//
// 003 WROTE SEVERAL AS CALLS THAT TAKE THE RECEIVER: satellite.variable.number.max(a, b), abs(a),
// clamp(a, low, high), power(a, b), modulus(a, b), truncate(a), sqrt(a). As a method each takes ONE
// ARGUMENT FEWER -- the receiver is a -- and a refusal names the method as the program wrote it,
// n.satellite.max(3), never the row's spelling.
//
// A NUMBER IS WHOLE, so floor, ceil, round and truncate answer it unchanged: 003 DESIGN 8.6 puts all four in
// its "exact and bounded" class, and they are here rather than refused so that a program written for a float
// can be moved to a number without its arithmetic changing meaning (number_methods.cpp's reading, whose code
// these files took when it was deleted, 2026-10-11).
//
// AN INFINITY'S WORDS ARE ITS OWN AND THE NUMBER'S (SATELLITE_INFINITY.md Q24 let a name declared
// satellite.variable.infinity hold a plain number, and the loss tests' `x.add(...)`, `.reverse()` and
// `.string` ran on one that did; the author's H of 2026-10-11 took that back -- an infinity name holds a
// minimum of infinity -- and whether an infinity keeps a number's words is his open "numbers A"). The checker judges by the declaration and the walker by the value, so
// they must agree on the period of every word -- and they do when an infinity's period is the union of its
// own words (1 6 17 n: .string, its display; .power_of, .resize and .nines, numbered and not built) and the
// number's (method_periods.hpp). A number's word on a value that IS an infinity runs the number's row,
// which refuses it in the words it was refused in before it had a number: an infinity has no digits, and
// its arithmetic is INF-3's.

#include "expression.hpp"
#include "token_codes.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// THE NUMBER A NUMBER'S METHOD RUNS -- the code of satellite.variable.number.<it>, 1 6 4 n -- or 0 for a
// method a number does not have.
token::Code number_number_of(token::Code method);

// THE NUMBER AN INFINITY'S OWN METHOD RUNS -- satellite.variable.infinity.<it>, 1 6 17 n -- or 0. Only its
// own: what the period of every word on an infinity is, its own and the number's, is method_periods.hpp's.
token::Code infinity_number_of(token::Code method);

// HOW MANY ARGUMENTS A NUMBER'S METHOD TAKES: max, min, power, modulus, shift_left and shift_right one, clamp
// two, the rest of the words above none -- and -1 for the conversions, add and reverse, which say their own
// counts in their own files, in the words they said before they had a number (as string_method_arity leaves
// a string's). NOT container_arity: a list's .max takes nothing, a number's takes what it is measured
// against. In the header so the checker and the walker read one list.
int number_method_arity(token::Code method);

// THE ELEVEN THAT WERE NAMES UNTIL A NUMBER'S WORDS HAD NUMBERS (2026-10-11): abs, floor, ceil, round, sqrt,
// digits, modulus, shift_left, shift_right, clamp and bytes had no method code, so after a period each was
// read as a name -- and a spacesuit's capsule called round was reached as one. They are method codes now,
// after any period on any receiver; an object's capsule of that name still runs (call_member takes any code),
// and the checker asks this where it lets a multiple's capsule names through as names.
bool named_until_numbered(token::Code method);

// A NUMBER'S WORD THAT IS NUMBERED AND NOT BUILT, said after the call -- "is numbered (1 6 4 14) and not
// built yet" -- or "" for every other method. Only .sqrt: an exact whole square root is a piece of work of
// its own (number_methods.cpp's reason). The checker says it before anything runs; the word's file, when
// one is run on a value the checker could not see.
std::string number_method_not_built(token::Code method);

// THE CHECKER'S HALF: `name.satellite.method(...)` on a name declared satellite.variable.number (or
// .infinity, which may hold one), or on a number literal, judged before anything runs -- how many it was
// given, in the walker's sentence, and .sqrt as not built. `bracketed` and `given` are what
// program_check.cpp's brackets_at found after the method. A method whose count is its own (the
// conversions, add, reverse) passes, as it did before it had a number.
signed long long int number_method_check(bool bracketed, std::size_t given, token::Code method,
                                         const std::string &spelling, std::string &why);

// THE WALKER'S HALF: the method's number, run. `number` is what number_a_method_runs answered for the
// receiver (method_periods.hpp), never 0 here: a 1 6 4 n for a number, and for an infinity its own 1 6 17 n
// or the number's. A count number_method_arity knows is refused here, before the number runs -- which is
// what lets each number's function read the arguments its word names without asking how many there are.
// A number's method never changes the number it is called on, so nothing is written back.
Value run_number_number(token::Code method, token::Code number, const Value &receiver,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context);

// THE PIECES THE NUMBERED METHODS SHARE (number_calls.cpp), in a namespace of their own so a name like
// the_number cannot meet another type's.
namespace number_method_parts {

// THE WHOLE NUMBER A METHOD WORKS ON -- the receiver's, or nullptr when the receiver is an infinity (a
// name declared satellite.variable.infinity holds one, or a multiple a number), refused as every type's word that
// is not built for it is: "x.satellite.floor is not built for an infinity yet -- so far it is a number's".
// `call` is the method as the refusal names it.
const satellite_number *the_number(const Value &receiver, const std::string &call, token::Code method,
                                   ExpressionContext &context);

// A WHOLE NUMBER GOING IN, as every one-argument method of number_methods.cpp took one: anything else
// is refused, naming what arrived. `what` says what it is for ("a number to compare with").
const satellite_number *number_argument(const Value &value, const std::string &call, const std::string &what,
                                        ExpressionContext &context);

} // namespace number_method_parts

} // namespace satellite004
