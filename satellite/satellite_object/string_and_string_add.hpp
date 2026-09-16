#pragma once
// satellite/satellite_object/string_and_string_add.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// string + string -> string. JOINING AND ADDING ARE THE SAME SHAPE, which is 003
// DESIGN 6.6 and the author at M19: `+` over two strings is one operator over
// two types, not a second meaning for the character.
//
// CANNOT REFUSE. append() widens to 32 bits only if the right side holds a
// character above U+FFFF, so joining two fast strings stays on the fast path.

#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int string_and_string_add(const satellite_string &left,
                                                  const satellite_string &right,
                                                  satellite_string &out)
{
    out = left;
    out.append(right);
    return success;
}

} // namespace satellite004
