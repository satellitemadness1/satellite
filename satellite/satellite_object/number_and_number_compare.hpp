#pragma once
// satellite/satellite_object/number_and_number_compare.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number against number -> -1, 0 or 1. Every comparison the language has --
// < > <= >= == != -- comes off this one answer, so there is one place where two
// numbers are ordered and six spellings that read it.
//
// CANNOT REFUSE. Zero is never negative, so the sign test is exact.
//
// A HEADER, NOT A .cpp, AND THAT IS THE WHOLE POINT OF THE SHAPE.
// satellite_number's one-limb case is [[gnu::always_inline]] in its own header;
// reaching it through a .cpp would make the compiler see a CALL where the
// measurement wanted an ADD -- 2.4 ns an `i = i + 1` against 1.0 ns
// (number_arithmetic.hpp). A fast path that is not inlined is not a fast path.
//
// THE FUNCTION IS NAMED EXACTLY AS THE FILE IS, so a reader who wants to know
// what happens when these two types meet under this operation opens the file
// whose name says so, and finds one function in it.

#include "../satellite_variable_number/satellite_number.hpp"

namespace satellite004 {

inline int number_and_number_compare(const satellite_number &left, const satellite_number &right)
{
    return satellite_number::compare(left, right);
}

} // namespace satellite004
