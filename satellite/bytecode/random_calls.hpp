#pragma once
// satellite/bytecode/random_calls.hpp -- satellite.random's words, run by the interpreter.
//
//     satellite.random.fast(20)            twenty digits: uniform over 0 .. 10^20 - 1
//     satellite.random.normal(1, 6)        a die: 1 to 6, both ends in
//     satellite.random.ultra(0, 100, 5)    one of 0, 5, 10 ... 100
//     satellite.random.seed(42)            this thread's generator starts again from 42, and
//                                          the three grades draw from it from then on
//
// THE WORDS AND THEIR NUMBERS ARE 003'S, whole: `1 7 1` to `1 7 16` in words/words.tsv,
// the author's table of 2026-09-04 (003's words.def) -- three grades by four shapes, and
// four `seeded` rows a 003 build minted and nobody built; those refuse by name. 004 added
// `satellite.random.seed(seed)`, `1 7 17` (the author, 2026-10-02: "make .fast(), normal()
// and ultra() throw away random numbers from the most recent seed").
//
// ONE GENERATOR, THREE WINDOWS. Every grade draws from pcg512_k16384 -- the generator the
// author asked for on 2026-10-02, pcg32_k16384 at 512 bits (satellite_random/pcg_512.hpp),
// one a thread, seeded whole from the kernel -- and what makes a grade is how long it
// throws draws away before it answers (random_spin.hpp): fast 500 to 1000 ms, normal 1000
// to 2000, ultra 2000 to 3000, his windows, each call's span drawn inside them.
//
// THE WORDS HAVE NO LIBRARY, for infinity_calls.hpp's reason turned round: a library's
// scenarios consume a value and answer a machine code (number_row.hpp), and these answer a
// NUMBER -- drawn by a generator that is the interpreter's own. So the checker knows them
// by is_random_word(), as it knows satellite.infinity() and satellite.info's words.
//
// WHAT IS REFUSED, AND WHERE (machine_codes.hpp 71-75, s_codes.hpp S430-S434) -- every one
// of them BEFORE the spin, so a call that will be refused never spends two seconds first:
//   before anything runs    fast(), normal(), ultra() -- the bare shape draws nothing
//                           (003's ruling, 2026-09-04: "not a question with an answer");
//                           satellite.random(...) itself; a count of arguments no shape
//                           has; seeded, which is not built
//   when the line runs      an argument that is not a whole number, a count of digits
//                           below 0, min above max, a step below 1, a step that never
//                           lands on max from min (003's rule, the same day)
//
// Written 2026-10-02.

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.random `1 7`, satellite.random() `1 7 0`, every word under it, `1 7 1` to
// `1 7 16` -- one run of codes -- and `1 7 17`, seed, appended later.
bool is_random_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS, and the walker asks again on its way in:
// `given` is how many arguments the call was written with. Answers success for a call
// that draws, or the machine code and the sentence for one that cannot.
signed long long int random_word_refused(token::Code code, std::size_t given, std::string &why);

// The call, its arguments already evaluated: a number, after its grade's spin, or a
// refusal in `context`.
Value call_random_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
