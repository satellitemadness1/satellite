// satellite/satellite_object/str_add_str.cpp -- THE TEMPLATE. Every other pair
// file in this folder is this one with two words changed.
//
// (the author, 2026-09-16) "first we need str_add_str.cpp, which is the template
// for a function that adds two variants of satellite_string type together."
//
// THE SHAPE, AND IT IS THE WHOLE POINT OF THE FILE:
//
//   1. take two satelliteObjects -- VARIANTS, not bare strings
//   2. ask each for the arm this function is about, through as_string(), which
//      answers nullptr rather than throwing when the guess is wrong
//   3. refuse types_do_not_meet (27) if either is not that arm -- the caller
//      does not have to have checked
//   4. do the work through the *_and_*.hpp header that already does it
//   5. write a satelliteObject back through `out`, and answer a machine code
//
// EVERY PAIR FILE ANSWERS A MACHINE CODE even when it cannot fail, so one table
// can hold them all as one kind of pointer (ObjectPair, fast_paths.hpp).
//
// JOINING AND ADDING ARE THE SAME SHAPE -- 003 DESIGN 6.6, the author at M19:
// `+` over two strings is one operator over two types, not a second meaning for
// the character.

#include "fast_paths.hpp"
#include "string_and_string_add.hpp"

namespace satellite004 {

signed long long int str_add_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_string *l = left.as_string();
    const satellite_string *r = right.as_string();
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    satellite_string answer;
    const signed long long int code = string_and_string_add(*l, *r, answer);
    if (code != success)
        return code;
    out = satelliteObject::of_string(std::move(answer));
    return success;
}

} // namespace satellite004
