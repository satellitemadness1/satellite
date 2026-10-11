#pragma once
// satellite/satellite_object/number_and_number_add.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number + number -> number. The pair that carries every loop in the language.
//
// CANNOT REFUSE. Addition of two whole numbers is always a whole number, and
// satellite_number has no ceiling to overflow past, so this answers success
// unconditionally. The machine code is still returned, because every pair file
// shares one shape and the caller's table holds one kind of pointer.
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
#include "../machine/machine_codes.hpp"

namespace satellite004 {

inline signed long long int number_and_number_add(const satellite_number &left,
                                                  const satellite_number &right,
                                                  satellite_number &out)
{
    out = left + right;
    return success;
}

} // namespace satellite004
