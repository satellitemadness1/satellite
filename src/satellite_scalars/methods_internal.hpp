#pragma once

// What satellite_scalars' files share -- the install halves, and the checks
// every method makes before it answers. Not a door onto this module;
// satellite_scalars/handlers.hpp is. The same file the evaluator, the parser
// and the resolver each keep, for the same reason.
//
// A METHOD'S FIRST QUESTION IS WHAT IT WAS ASKED OF. The compiler proved the
// RECEIVER'S DECLARATION was the right type -- that is how the selector got
// its number -- but a declaration is not a value: the slot may hold nothing
// (declared and never assigned, S0714), and nothing stops a program assigning
// a number into a name declared as a string, because satellite checks VALUES
// and not annotations at this milestone. So every handler re-asks at run time,
// through the three receiver helpers below, and the sentence is written once.

#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_bits/bits.hpp"
#include "satellite_float/satellite_float.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_value/value.hpp"

#include <cstdint>

namespace satellite::scalars {

// The receiver or an argument as the string it must be, or a refusal in the
// machine and false. `who` is which of the handler's values to check -- 0 is
// the receiver.
bool string_at(eval::Machine &m, const Value *arguments, uint32_t who,
               const SatString **out);

// The same, for a number.
bool number_at(eval::Machine &m, const Value *arguments, uint32_t who,
               const Number **out);

// The receiver or an argument as the bit run it must be, or a refusal and
// false. M19.5, and the same shape as the two above for the same reason: the
// compiler proved the DECLARATION was a `satellite.variable.binary`, and a
// declaration is not a value.
bool bits_at(eval::Machine &m, const Value *arguments, uint32_t who,
             const bits::BitRun **out);

// The same, for a hex run. A SEPARATE HELPER AND NOT A RADIX ARGUMENT TO THE
// ONE ABOVE: value.hpp puts the two radices in different variant arms, so
// `as_binary` answers nullptr for a hex value and `as_hex` answers nullptr for
// a bit run, and neither of these two can ever be handed the other's receiver.
// That is what stops a hex method from silently reading a binary -- the check
// is the type system's here, not a field comparison somebody could omit.
bool hex_at(eval::Machine &m, const Value *arguments, uint32_t who,
            const bits::HexRun **out);

// The same, for a float. M21, and the type's first method table -- so this is
// the first helper in this file whose receiver arm did not exist before the
// milestone that needed it.
bool float_at(eval::Machine &m, const Value *arguments, uint32_t who,
              const Float **out);

// An argument as a position -- a whole number no less than zero that a machine
// word can hold -- or a refusal and false. Positions are 0-based: `at(0)` is
// the first character, which is the counting every reader of a C-family
// language already has, and the counting DESIGN §6.2's own example
// `my_list[0]` uses. (WORD_NUMBERS counts its PATHS from 1; that is a fact
// about the numbering, not about strings.)
bool position_at(eval::Machine &m, const Value *arguments, uint32_t who,
                 unsigned long long *out);

// The install halves handlers.cpp sums. Each row is a words.def path, so
// the table stays a property of the build -- dispatch.hpp's warning about
// user PathIds is why nothing here may ever take one.
void install_string_methods();
void install_number_methods();
void install_variant_methods();
void install_bits_methods();
void install_hex_methods();
void install_float_methods();

} // namespace satellite::scalars
