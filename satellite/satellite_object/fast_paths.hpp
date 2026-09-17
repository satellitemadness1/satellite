#pragma once
// satellite/satellite_object/fast_paths.hpp -- the one place every pair fast
// path is DECLARED. One function, one file, still: this header holds no bodies.
//
// (the author, 2026-09-16) "let's begin to define fast paths between all of
// these objects.. first we need str_add_str.cpp, which is the template for a
// function that adds two variants of satellite_string type together."
//
// THE NAME IS <left>_<operation>_<right> AND THE FILE IS THAT NAME, so a reader
// who wants to know what happens when a string meets a string under `+` opens
// str_add_str.cpp and finds one function in it.
//
// THEY TAKE VARIANTS, NOT RAW TYPES, and that is the difference between these
// and the *_and_*.hpp files beside them. The .hpp files are the arithmetic
// itself, on a bare satellite_number, inlined into the caller's loop. These take
// two satelliteObjects, check the arms, and write a satelliteObject back -- the
// layer that knows about the variant. One shape, so a table can hold them.

#include "satellite_object.hpp"

namespace satellite004 {

using ObjectPair = signed long long int (*)(const satelliteObject &, const satelliteObject &, satelliteObject &);

signed long long int str_add_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);
signed long long int str_minus_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);
signed long long int str_find_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);
signed long long int num_add_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);
signed long long int num_sub_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);
signed long long int num_div_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out);

// THE CONVERSIONS, through satellite_number as the hub -- see object_convert.cpp
// for why that is N and not N x N -- except a binary's `.bin` and `.string`,
// which keep its width and so never leave it for a number. One argument, because a
// conversion has no right-hand side.
using ObjectConversion = signed long long int (*)(const satelliteObject &, satelliteObject &);

signed long long int object_to_string(const satelliteObject &from, satelliteObject &out);
signed long long int object_to_number(const satelliteObject &from, satelliteObject &out);
signed long long int object_to_binary(const satelliteObject &from, satelliteObject &out);
signed long long int object_to_hexadecimal(const satelliteObject &from, satelliteObject &out);

} // namespace satellite004
