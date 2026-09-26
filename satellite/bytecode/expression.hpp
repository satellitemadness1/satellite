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

    // THE WHOLE LINE IS THIS EXPRESSION AND ITS ANSWER IS LET GO (2026-09-22): a method
    // call standing as a statement, `log.call_append(text)`. The call that ends the line
    // is then not asked for an answer -- a capsule that hands back nothing is not wrong
    // there, only where its answer is used (capsule_calls.cpp).
    bool statement = false;

    // THE REFUSAL HAS ALREADY BEEN SHOWN (2026-09-22): a capsule this expression called
    // stopped, and printed its own report with its own line and caret. What called it
    // stops too, and must not print a second report about the same failure.
    bool reported = false;

    // WHERE IT WENT WRONG, for the caret. SATELLITE_ERROR E6.
    //
    // RECORDED ON THE REFUSAL AND NEVER PER TOKEN. A position kept as the
    // expression walks would be a store on the hottest path in the language, to
    // be read only on a path that has already failed -- so the refusal carries
    // its own position instead, and a refusal costs one more store than it did.
    //
    // `placed` false means the refusal did not say where, and the caller falls
    // back to the statement's own position: the right LINE, and a caret under
    // the start of the statement rather than under the operator. Better than
    // nothing, and it never claims a place it does not have.
    std::size_t refused_at = 0;
    bool placed = false;

    void refuse(signed long long int stopped_on, std::string reason)
    {
        if (code != success)              // the first refusal is the true one
            return;
        code = stopped_on;
        why = std::move(reason);
    }

    void refuse(signed long long int stopped_on, std::string reason, std::size_t at)
    {
        if (code != success)
            return;
        refused_at = at;
        placed = true;
        refuse(stopped_on, std::move(reason));
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

// `a[i] = v` and `a[i][j] = v`, the indices already worked out by the walker.
//
// `root` MUST BE THE VARIABLE'S OWN OBJECT, reached as a reference through the
// VariableTable. A copy here does not fail, it silently makes every write a full
// copy of the list -- satellite_object/satellite_list.hpp says why, and says how
// 003 lost months to exactly that.
signed long long int write_through_index(Value &root, const std::vector<Value> &indices, Value value,
                                         const std::string &name, std::size_t where, const TypeShape &shape,
                                         ExpressionContext &context);

// ONE LINK OF A METHOD CHAIN AS IT WAS WRITTEN, from its `.`: `.trim`, `.size()` or
// `.split(...)`, so a refusal can name the piece it refused -- `s.trim.append`, not
// `s.append` (the M16 review). What was inside the brackets is not spelled out, as
// `s[...]` never spells its index.
std::string link_spelled(const std::vector<std::bitset<16>> &row, std::size_t dot);

// `s.split(",")[2]` and `"abc"[1]` -- [ ] AFTER A METHOD'S ANSWER OR A LITERAL, which is
// not built: the sentence the walker refuses it with, and the checker too when it can
// see it before the run (the M16 review, 2026-09-26). `spelled` is what stands before
// the `[`, as `s.split(...)`.
std::string index_after_an_answer(const std::string &spelled);

} // namespace satellite004
