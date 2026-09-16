#pragma once
// satellite/satellite_object/number_and_string_add.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number + string. THE FILE THE AUTHOR NAMED when he gave the convention, and the
// one pair in the set that REFUSES on purpose.
//
// NOTHING IS CONVERTED (DESIGN 1.1). `"n = " + 4` is refused rather than quietly
// becoming "n = 4", and check.sh asserts that refusal by name -- 'a string and a
// number converts nothing -> 27'. A program that wants the join writes the
// conversion out loud, and number_to_string.hpp is the fast path it writes.
//
// THIS IS A SEAM, NOT A WALL. If the author rules that `+` may convert, the
// change is this one function -- call number_to_string then string_and_string_add
// -- and nothing else in the interpreter moves. That is the reason the pair has
// its own file even while it answers a refusal: the file is where the ruling
// would land.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_string_add(const satellite_number &,
                                                  const satellite_string &,
                                                  satellite_string &)
{
    return types_do_not_meet;
}

} // namespace satellite004
