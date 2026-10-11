// satellite/satellite_object/str_minus_str.cpp -- str_add_str.cpp's shape, with
// the operation changed. The author asked for it by name.
//
// WHAT `-` MEANS ON TWO STRINGS IS THE AUTHOR'S RULING (2026-09-17): "minus takes
// away the smallest string", the FIRST occurrence of the string after the minus.
// string_and_string_subtract.hpp holds the ruling, its examples and the walk; this
// file is the variant layer around it, as str_add_str.cpp is around joining.
//
// It replaces this file's own first guess, which took away EVERY occurrence and
// was never reachable: satelliteObject::subtract had no string arm until the
// ruling, so `"a" - "b"` was refused (27).

#include "fast_paths.hpp"
#include "string_and_string_subtract.hpp"

namespace satellite004 {

signed long long int str_minus_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_string *l = left.as_string();
    const satellite_string *r = right.as_string();
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    satellite_string answer;
    const signed long long int code = string_and_string_subtract(*l, *r, answer);
    if (code != success)
        return code;
    out = satelliteObject::of_string(std::move(answer));
    return success;
}

} // namespace satellite004
