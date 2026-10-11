#pragma once
// satellite/satellite_object/number_and_number_modulus.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number % number -> number. The remainder takes the DIVIDEND's sign, as C++
// does: -7 % 2 is -1, not 1. 003 DESIGN 8.6 chose the same rule for floats, so
// the two types will agree when satellite_float lands.
//
// REFUSES division_by_zero (22).
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

inline signed long long int number_and_number_modulus(const satellite_number &left,
                                                      const satellite_number &right,
                                                      satellite_number &out)
{
    // Quotient first, which is the order satellite_number::divide documents for
    // the case where two of its arguments are the same object.
    satellite_number quotient;
    return satellite_number::divide(left, right, quotient, out);
}

} // namespace satellite004
