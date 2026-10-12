#pragma once
// satellite/bytecode/convert_calls.hpp -- satellite.convert.number(x) and satellite.convert.float(x),
// run by the interpreter (the author, 2026-10-12: "what about always a fraction? 4 / 2 is the
// fraction", and his 9c, "build satellite.convert.float and .number now, the rest of convert later").
//
// EVERY a / b OF TWO NUMBERS IS A FRACTION NOW, KEPT AS WRITTEN (satellite_object.cpp's divide), and
// these two words are how a program says which worth of one it wants:
//
//     satellite.convert.number(7 / 2)      4        the whole number nearest it: .5 and above is one
//     satellite.convert.number(5 / 4)      1        more, below .5 is not (the author, 2026-10-12:
//     satellite.convert.number(-7 / 2)     -4       "7 / 2 = 4 if a number", ".500000000 is plus one
//                                                   .499999999 is not plus one"), half away from zero
//     satellite.convert.number(f)          4        f a fraction 7/2: the same
//     satellite.convert.number(2.75)       3        a float the same way: 2.4999 is 2, -2.5 is -3
//     satellite.convert.number(4 / 2)      2        even, so nothing is said
//     satellite.convert.number(9)          9
//
// AND WITH A WARNING, ON THE SCREEN, WHEN SOMETHING AFTER THE POINT WAS DROPPED -- S021, once a line,
// and the run goes on (the author: "With a warning", "But don't stop the interpreter").
//     satellite.convert.float(5 / 4)       1.25     the fraction's worth, as 5.0 / 4 is
//     satellite.convert.float(1 / 3)       0.33...  at arguments.float.decimal places (128), the last
//                                                   rounded half away from zero, as every float is
//     satellite.convert.float(9)           9.0
//     satellite.convert.whole(7 / 2)       3        CUT toward zero, with nothing said -- what `/` of two
//     satellite.convert.whole(-7 / 2)      -3       numbers was before 2026-10-12, so the programs written
//     satellite.convert.whole(2.75)        2        for it keep their answers (the author: "change the
//                                                   programming so they still work if needed")
//
// Anything else is refused by name: the rest of satellite.convert is built later (his 9c). A division
// at the root of .whole is one whole-number division too (evaluate_whole_expression), as a for's step is.
//
// satellite.convert.number(a / b) IS ONE WHOLE DIVISION, AT TODAY'S SPEED (his 9e: "... at today's
// speed"; rounded since his "7 / 2 = 4 if a number"). Its value is worked out by
// evaluate_rounded_expression (expression.hpp), so a division of two numbers at its ROOT is one
// whole-number division and a look at what is left over -- no fraction is made to be rounded. A
// division anywhere else in it -- (a / b) in brackets, a / b + 1 -- is a fraction, as everywhere.
//
// THE WORDS HAVE NO LIBRARY, for infinity_calls.hpp's reason: a library's scenarios consume values and
// answer machine codes (number_row.hpp), and these answer a value of their own. call_word takes them
// BEFORE it works out its arguments, as it takes satellite.thread.new and satellite.access, because
// the value is worked out here, by the evaluator that rounds the root's division.

#include "expression.hpp"

#include <bitset>
#include <cstddef>
#include <vector>

namespace satellite004 {

// satellite.convert.number(x) `1 35 1`, satellite.convert.float(x) `1 35 2` and satellite.convert.whole(x)
// `1 35 3`. Not the bare
// satellite.convert `1 35`, which is no call: the checker refuses it as a word with no library.
bool is_convert_word(token::Code code);

// The call, from its `(` at `at`, which is left past its `)`.
Value call_convert_word(token::Code code, const std::vector<std::bitset<16>> &row, std::size_t &at,
                        ExpressionContext &context);

} // namespace satellite004
