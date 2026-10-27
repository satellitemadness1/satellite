#pragma once
// satellite/bytecode/scalar_calls.hpp -- WHAT THE OTHER SCALARS ANSWER, REACHED BY THEIR NUMBERS (NM-8, 2026-10-11).
//
// The author, 2026-10-11: "we still need to get the word satellite onto everything, before we do anything...
// .pointer, numbers floats, everything builtin needs the word satellite still". So a float's, a hex's, a
// binary's, a colour's, a fraction's, a percentage's, a bool's and a time's every word is the word
// satellite.variable.<type>.<it> in words/words.tsv, its code a file under satellite-numbers/words/ named by
// its numbers, and it is written `.satellite.`:
//
//     f.satellite.string   f.satellite.number   f.satellite.binary   f.satellite.hex          float      1 6 10 2-5
//     h.satellite.width    h.satellite.add(x)   h.satellite.reverse()   and the four          hex        1 6 11 n
//     b.satellite.width    b.satellite.clear    b.satellite.add(x)   b.satellite.reverse()    binary     1 6 5 n
//     c.satellite.transparency   c.satellite.transparency(50)   and the four                  color      1 6 19 1-5
//     q.satellite.numerator   q.satellite.denominator   q.satellite.string                    fraction   1 6 20 1-3
//     p.satellite.string   p.satellite.add(x)                                                  percentage 1 6 16 1-2
//     t.satellite.string                                                                       bool       1 6 6 1
//     d.satellite.string                                                                       time       1 6 3 1
//     d.satellite.time.convert(unit), one word of two parts (TIME-1, 2026-10-26)               time       1 6 3 2 1
//
// WHAT EACH ANSWERED THE DAY IT WAS NUMBERED, AND EVERY ROW 003 ALREADY HAD -- nothing more. Where 003 has two
// rows for one code, the row the code runs is the one strings run (1 6 1 19-22: `.string`, `.number`,
// `.binary`, `.hex`), and the others -- to_string, to_number, as_number, to_binary, to_hex -- are its second
// names, files that describe themselves and run nothing (1.6.1.13.cpp's way). A row with a code and no code
// of its own yet -- a binary's .width and .clear -- runs its number and is told it is not built; a float's
// .binary and .hex say why in their own files. .digits (1 6 5 5, 1 6 11 4) has no method code yet, so
// nothing reaches it: its rows wait for one.
//
// A WORD A TYPE DOES NOT HAVE STAYS AS IT WAS. `b.find("1")`, `t.number` on a bool, `q.reverse()` on a fraction
// were refused as they ran, and still are, written with `.`; written `.satellite.` they are told before
// anything runs that the type has no such word (method_periods.hpp's period_written_wrong).
//
// TWO ROUTES WERE THERE BEFORE, AND BOTH RUN NUMBERS NOW. A float, a hex, a colour and a fraction answered in
// their own <type>_values.cpp; each method's code moved from there, whole, into its number's file, and those
// files keep the checker's half and what a value of the type does when it is stored. A binary, a percentage,
// a bool and a time ran the walker's shared tail (expression.cpp: the conversions, .add and .reverse()), and
// their files hold a copy of it -- the tail still serves the kinds not numbered yet.

#include "expression.hpp"
#include "token_codes.hpp"

#include <string>
#include <vector>

namespace satellite004 {

struct satellite_float;

// WHETHER A KIND IS ONE OF THE EIGHT whose words this file numbers: float, hex, binary, color, fraction,
// percentage, bool and time.
bool an_other_scalar(satelliteObject::Kind kind);

// THE NUMBER EACH TYPE'S METHOD RUNS -- the code of satellite.variable.<type>.<it> -- or 0 for a method that
// type does not have. One switch a type, as string_number_of and list_number_of are, and asked only through
// method_periods.hpp's number_a_method_runs, the one place that knows every type has one.
token::Code float_number_of(token::Code method);
token::Code hexadecimal_number_of(token::Code method);
token::Code binary_number_of(token::Code method);
token::Code color_number_of(token::Code method);
token::Code fraction_number_of(token::Code method);
token::Code percentage_number_of(token::Code method);
token::Code bool_number_of(token::Code method);
token::Code time_number_of(token::Code method);

// THE WALKER'S HALF: the method's number, run -- the function its file put in the row at start-up, through
// the function table. `number` is never 0 here (the walker asked). `home` is the variable's own value while
// the chain is still on a name, and nullptr otherwise: only a colour's .transparency(n) writes through it.
// A count and missing brackets are refused by each word's own file, in the words it always said them in --
// they differ from type to type -- so this refuses only a number with nothing built behind it yet.
Value run_scalar_number(token::Code method, token::Code number, const Value &receiver, Value *home,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context);

// THE PIECES THE NUMBERED WORDS SHARE (scalar_calls.cpp), in a namespace of their own.
namespace scalar_method_parts {

// THE TEXT A CONVERSION ANSWERS, or refused as text that could not be written (`spelled` names the call).
Value text_value(const std::string &written, const std::string &spelled, ExpressionContext &context);

// ONE OF A FRACTION'S NUMBERS AS A PROGRAM GETS IT BACK -- .numerator and .denominator: the whole number when
// nothing follows the point (1/3's numerator is 1), the float when something does (1.5/2's is 1.5).
Value part_as_value(const satellite_float &part);

} // namespace scalar_method_parts

} // namespace satellite004
