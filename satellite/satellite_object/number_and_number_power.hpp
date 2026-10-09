#pragma once
// satellite/satellite_object/number_and_number_power.hpp -- ONE FUNCTION, ONE FILE.
//
// (the author, 2026-09-16) "put individual fast paths between the variant types
// in individual .hpp files, one function, one file, and name it like this:
// number_and_string_add.hpp".
//
// number ^ number -> number, by squaring over the exponent's limbs. The author
// ruled on 2026-09-16 that `^` is power and that .power() is its alias, so both
// spellings arrive here and neither has arithmetic of its own.
//
// REFUSES answer_is_not_whole (24) for a negative exponent -- 2 ^ -1 is exactly
// 1/2, which a WHOLE number cannot hold, and truncating it to 0 behind the
// program's back is the thing this language does not do. Also division_by_zero
// (22) for 0 ^ -n. NO CEILING ON THE EXPONENT, on purpose.
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

inline signed long long int number_and_number_power(const satellite_number &left,
                                                    const satellite_number &right,
                                                    satellite_number &out)
{
    return satellite_number::power(left, right, out);
}

} // namespace satellite004
