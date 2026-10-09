#pragma once
// satellite/bytecode/number_methods.hpp -- a method written on a number
// variable: `my_number.power(3)`.
//
// (the author, 2026-09-16) "the to the power of can be ^ and an alias you have
// to take number_name.power() so .power() becomes something you can call on that
// object."
//
// SO `^` AND `.power()` ARE ONE OPERATION WITH TWO SPELLINGS, and they must stay
// one: both reach number_fast_path::power and neither has its own arithmetic.
// That is the whole design rule for this file -- a method here is a name bound
// to a fast path, never a second implementation of it. `n.modulus(3)` and
// `n % 3` are the same call.
//
// THIS IS NOT THE OBJECT MODEL, and it must not be mistaken for a start on one.
// The author, the same day: "building the satellite object model will be very
// very hard to do, given that names are simply strings... we will have
// satelliteObject and satelliteUserDefinedObject and they will optionally hold
// the class satellite_number and satellite_string, but we are not doing that at
// this moment." What this file does is the ONE HOP a declared name already
// allows: the variable's declared type is known (value.hpp keeps it), so the
// name after the dot can be resolved against that type and nothing else. A
// method on an expression, on a call's result, or on a user's own class is the
// object model's job and is refused here.
//
// WHY A NAME AND NOT A CODE. `my_number.power` is not a word of the language --
// `satellite.variable.number.power(a, b)` is (`1 6 4 10`), but the lexer cannot
// know that `my_number` is a number when it reads the line, so it writes
// name_token, method_token, name_token. Matching the text is therefore the only
// thing available before the object model exists, and it is why this is a
// string compare per call -- one of the costs the race against 003 measured.

#include "value.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// A method of satellite.variable.number, named by the text after the dot.
//
// Answers success and fills `out`; or a machine code with `why` set. A name that
// is not a method of this type answers satl_line_not_understood, so the caller
// can say which name was not understood rather than guessing.
//
// `arguments` are already evaluated, left to right.
signed long long int call_number_method(const std::string &name,
                                        const satellite_number &receiver,
                                        const std::vector<Value> &arguments,
                                        Value &out,
                                        std::string &why);

} // namespace satellite004
