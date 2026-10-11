#pragma once
// satellite/satellite_object/number_and_number_divide.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number / number -> number, TRUNCATED TOWARD ZERO. 7 / 2 is 3, -7 / 2 is -3,
// which is the rule satellite_number.hpp wrote down and check_numbers.py has
// checked against Python across 482,465 cases.
//
// REFUSES division_by_zero (22). The remainder is worked out and thrown away --
// satellite_number::divide answers both at once, so asking for the quotient
// alone costs nothing extra. A caller that wants both should call divide itself.
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

inline signed long long int number_and_number_divide(const satellite_number &left,
                                                     const satellite_number &right,
                                                     satellite_number &out)
{
    satellite_number remainder;
    return satellite_number::divide(left, right, out, remainder);
}

} // namespace satellite004
