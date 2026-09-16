#pragma once
// satellite/bytecode/expression.hpp -- WHERE A TOKEN TRIGGERS A FAST PATH.
//
// (the author, 2026-09-16) "build these functions into the interpreter so that
// when it encounters a + sign, it adds, when it encounters a - sign, it
// subtracts, it multiplies, it divides, it... powers of, it modulus'."
//
// This is that file. `+` is plus_token is number_fast_path::add, and the three
// lines between them are a switch, a table lookup and a call the compiler
// inlines. Nothing is allocated to work out an expression, exactly as nothing is
// allocated to run a line (program_walk.hpp).
//
// PRECEDENCE IS 003 DESIGN §6.6's TABLE, WHICH ALREADY EXISTED and was decided
// at 003's M4 on 2026-08-30. It is not re-derived here:
//
//     5  tightest   ^          (power -- see the note below)
//     4             * / %
//     3             + -
//     2             < > <= >=
//     1  loosest    == !=
//
// and unary `-` and `!` bind tighter than all of them. Levels 1-4 are
// LEFT-associative, which §6.6 states and gives the reason for: the author's
// first instruction was "right to left every time", and scoped to all operators
// that would have made `10 - 3 - 2` answer 9.
//
// TWO THINGS HERE ARE NEW AND THE AUTHOR HAS NOT RULED ON THEM. Both are marked
// in expression.cpp where they bite, and both are listed in PROGRESS.md:
//
//   1. `^` IS POWER, AND THE REGISTRY CALLS IT bit_exclusive_or_token. There is
//      no power token: REGISTRY.satellite's arithmetic family holds exactly
//      + - * / %, and `^` sits in the logic family carrying a QUESTION -- "§6
//      has no bitwise row". The author asked for power as an operator, satellite
//      has no bitwise operations decided, so `^` is read as power. If the ruling
//      goes the other way the fix is one registry row and one case label.
//
//   2. `^` IS RIGHT-ASSOCIATIVE, the only operator that is. `2 ^ 3 ^ 2` is
//      2 ^ 9 = 512 and not (2 ^ 3) ^ 2 = 64, because that is what the notation
//      means in mathematics and because left-associative power is the one
//      spelling nobody writes on purpose. §6.6's left-associative rule is about
//      the four levels it lists, and power is not one of them.
//
// A REFUSAL CARRIES ITS REASON. The Outcome holds the first machine code the
// expression stopped on and a sentence saying which operator met which kinds, so
// `"a" - "b"` reports subtraction on two strings rather than a bare 27.

#include "bytecode_registry.hpp"
#include "function_table.hpp"
#include "value.hpp"

#include <string>

namespace satellite004 {

// Everything one expression may reach: the variables of the body it is in, the
// libraries, and the state to report through. The variables are the running
// body's own -- there are no globals.
struct ExpressionContext {
    ExpressionContext(VariableTable &their_variables, const FunctionTable &their_functions, MachineState &their_state)
        : variables(their_variables), functions(their_functions), state(their_state) {}

    VariableTable &variables;
    const FunctionTable &functions;
    MachineState &state;

    signed long long int code = success;  // the FIRST refusal; success while none
    std::string why;                      // and what to say about it

    void refuse(signed long long int stopped_on, std::string reason)
    {
        if (code != success)              // the first refusal is the true one
            return;
        code = stopped_on;
        why = std::move(reason);
    }
};

// One expression, starting at `at`, which is left on the first code that is not
// part of it -- the line_end_token, a `)`, or a `,`. Answers what it is worth;
// on a refusal the value is `nothing` and context.code says why.
//
// RUNS WHAT IT READS. A word's call inside an expression really calls it, inner
// before outer, which is what makes display(display("x")) work and what makes
// `n = satellite.variable.string.size("abc")` mean anything.
Value evaluate_expression(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context);

// One word's call at `at` -- its arguments evaluated, its scenario chosen by the
// KIND of the argument, its machine code answered as a number. Public because
// run_body calls a word as a STATEMENT as well as inside an expression.
Value call_word(const std::vector<std::bitset<16>> &row, std::size_t &at, ExpressionContext &context);

// A number given to a library: `count` when it fits an unsigned long long and is
// not negative, `text` otherwise -- which is the only exact path for a number
// larger than one limb. See number_conversions.hpp.
signed long long int display_a_number(const Scenarios &scenarios, const satellite_number &value);

} // namespace satellite004
