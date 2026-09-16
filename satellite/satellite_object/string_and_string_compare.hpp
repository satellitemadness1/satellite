#pragma once
// satellite/satellite_object/string_and_string_compare.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// string against string -> -1, 0 or 1, code by code, a shorter string that is a
// prefix coming first.
//
// THE ORDER IS THE AUTHOR'S CHARACTER TABLE, not ASCII and not Unicode: codes
// 0-127 are every ASCII character once in HIS order, so comparing codes sorts
// the way 003 sorted. That is why this reads satellite_string::compare and never
// compares the UTF-8 bytes.

#include "../satellite_variable_string/satellite_string.hpp"

namespace satellite004 {

inline int string_and_string_compare(const satellite_string &left, const satellite_string &right)
{
    return satellite_string::compare(left, right);
}

} // namespace satellite004
