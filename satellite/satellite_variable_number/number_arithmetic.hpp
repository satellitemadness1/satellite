#pragma once
// satellite/satellite_variable_number/number_arithmetic.hpp -- THE FAST PATHS THE
// TOKENS CALL. One function an arithmetic token, and one for the comparisons.
//
// (the author, 2026-09-16) "write a few functions that are fast paths that the
// tokens will call: add two numbers together, subtract two numbers, multiply two
// numbers, divide two numbers, number to the power of a number, modulus of a
// number... then wire them with the tokens so that the tokens trigger the fast
// paths."
//
// WHY THIS IS A HEADER AND EVERY FUNCTION IS `inline`. satellite_number's
// one-limb case is written inline in satellite_number.hpp and forced inline
// ([[gnu::always_inline]]) because measuring showed an out-of-line call made g++
// spill a loop's counter to memory -- 2.4 ns an `i = i + 1` against 1.0 ns. A
// fast path that the interpreter reaches through a .cpp would throw that away at
// the last step: the compiler would see a call, not an add. So these live in the
// header, the interpreter includes it, and `counter + 1` compiles down to the
// same handful of instructions the race measured.
//
// EVERY ONE ANSWERS A MACHINE CODE AND WRITES THROUGH `out`. That is not a style
// choice -- + - * cannot fail, but / and % refuse a zero divisor and ^ refuses a
// fraction, so a uniform shape lets the interpreter's operator table hold six
// pointers of one type instead of six special cases. `out` may be either
// argument: each function either delegates to satellite_number, whose aliasing
// rules are written down, or takes its copies first.
//
// WHAT IS NOT HERE, AND WHERE IT WENT. The token -> function choice is in
// satellite/bytecode/expression.cpp, not here, so that satellite.variable.number
// never has to know what a token is. This file is the arithmetic; that file is
// the wiring.

#include "satellite_number.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {
namespace number_fast_path {

// + -- and the one operator that is also defined on two strings (003 DESIGN
// §6.6, M19). Joining and adding are the same shape, so the interpreter sends
// two strings elsewhere and two numbers here.
inline signed long long int add(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    out = left + right;
    return success;
}

inline signed long long int subtract(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    out = left - right;
    return success;
}

inline signed long long int multiply(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    out = left * right;
    return success;
}

// / -- WHOLE-NUMBER DIVISION, TRUNCATED TOWARD ZERO, which is the rule
// satellite_number.hpp already wrote down and check_numbers.py already checks
// against Python across 477,253 cases. 7 / 2 is 3 and -7 / 2 is -3.
//
// THIS IS THE ONE TO REVISIT WHEN THE FRACTION LANDS. The author's plan is that
// `5/4` (no spaces) becomes a fraction while `5 / 4` (spaces) divides -- the
// lexer ALREADY tells those two apart, at bytecode_registry.cpp:251, because a
// touching slash is a path separator. So the spelling is free; what changes is
// what this function answers, and nothing else in the interpreter has to move.
inline signed long long int divide(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    satellite_number remainder;
    return satellite_number::divide(left, right, out, remainder);
}

// % -- the remainder takes the DIVIDEND's sign, as C++ does and as
// satellite_number.hpp decided: -7 % 2 is -1, not 1. 003 DESIGN §8.6 chose the
// same rule for floats ("truncated and not floored"), so the two types agree.
inline signed long long int modulus(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    satellite_number quotient;
    // Quotient FIRST when the two are the same object, which they are not here,
    // but the order matches what satellite_number::divide documents.
    return satellite_number::divide(left, right, quotient, out);
}

// ^ -- see satellite_number.hpp: a negative exponent answers answer_is_not_whole
// (24) rather than truncating 1/2 to 0 behind the program's back.
inline signed long long int power(const satellite_number &left, const satellite_number &right, satellite_number &out)
{
    return satellite_number::power(left, right, out);
}

// The shape all six share, so the interpreter's table is six pointers of one
// type. Named here rather than in the interpreter because it is this file's
// promise about them, not the caller's guess.
using Operation = signed long long int (*)(const satellite_number &, const satellite_number &, satellite_number &);

// < > <= >= == != all come off ONE comparison, which is satellite_number's own
// and already inline for the one-limb case. Kept beside the six because the
// tokens reach it the same way and for the same reason.
inline int compare(const satellite_number &left, const satellite_number &right)
{
    return satellite_number::compare(left, right);
}

} // namespace number_fast_path
} // namespace satellite004
