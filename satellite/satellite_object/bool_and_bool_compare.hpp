#pragma once
// satellite/satellite_object/bool_and_bool_compare.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// bool against bool -> -1, 0 or 1. false orders before true, so == and != read
// off the same answer every other pair uses.
//
// ONLY == AND != MEAN ANYTHING on two bools, and the caller is what refuses an
// ordering. This answers the order; it does not decide which spellings may ask.

namespace satellite004 {

inline int bool_and_bool_compare(bool left, bool right)
{
    return left == right ? 0 : (left ? 1 : -1);
}

} // namespace satellite004
