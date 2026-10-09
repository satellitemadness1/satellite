#pragma once
// satellite/satellite_object/object_float.hpp -- EVERYTHING A FLOAT DOES AS A
// VALUE, and the one place satellite_object.cpp asks about it. `satellite.variable.float`.
//
// ONE FILE PER TYPE, SO FOUR TYPES COULD BE BUILT AT ONCE (2026-09-22). The author
// asked for the float, the hex, the colour and the fraction together; each of
// satelliteObject's methods asks this file about any pair that holds a float
// (satellite_object.cpp's answered_by_its_own_file), and nothing else in that file
// knows what a float is. A pair holding two of the four goes to the first of
// fraction, color, hexadecimal, float that it holds -- mixing them is the author's
// "grand finale", and until then the file asked refuses the pair.
//
// `sign` is the operator as written: + - * / % ^.

#include "satellite_object.hpp"

#include <string>

namespace satellite004 {

signed long long int float_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                     satelliteObject &out, std::string &why);
signed long long int float_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                   std::string &why);
bool float_same(const satellite_float &left, const satellite_float &right);
signed long long int float_to_string(const satellite_float &value, satellite_string &out, std::string &why);
signed long long int float_to_number(const satellite_float &value, satellite_number &out, std::string &why);
signed long long int float_to_binary(const satellite_float &value, satellite_string &out, std::string &why);
signed long long int float_to_hexadecimal(const satellite_float &value, satellite_string &out, std::string &why);

} // namespace satellite004
