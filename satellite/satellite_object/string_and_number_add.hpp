#pragma once
// satellite/satellite_object/string_and_number_add.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// string + number. The mirror of number_and_string_add.hpp, and it refuses for
// the same reason and would be opened by the same ruling.
//
// BOTH ORDERS GET A FILE even though both answer the same thing today, because
// the pair table is indexed by (left, right) and a reader looking for 'what
// happens when a string meets a number' must find it under that name.

#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int string_and_number_add(const satellite_string &,
                                                  const satellite_number &,
                                                  satellite_string &)
{
    return types_do_not_meet;
}

} // namespace satellite004
