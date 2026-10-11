#pragma once
// satellite/satellite_object/object_fraction.hpp -- EVERYTHING A FRACTION DOES AS A
// VALUE, and the one place satellite_object.cpp asks about it. `satellite.variable.fraction`.
//
// ONE FILE PER TYPE, SO FOUR TYPES COULD BE BUILT AT ONCE (2026-09-22). The author
// asked for the float, the hex, the colour and the fraction together; each of
// satelliteObject's methods asks this file about any pair that holds a fraction
// (satellite_object.cpp's answered_by_its_own_file), and nothing else in that file
// knows what a fraction is. A pair holding two of the four goes to the first of
// fraction, color, hexadecimal, float that it holds -- mixing them is the author's
// "grand finale", and until then the file asked refuses the pair.
//
// `sign` is the operator as written: + - * / % ^.

#include "satellite_object.hpp"

#include <string>

namespace satellite004 {

signed long long int fraction_operation(char sign, const satelliteObject &left, const satelliteObject &right,
                                     satelliteObject &out, std::string &why);
signed long long int fraction_compare(const satelliteObject &left, const satelliteObject &right, int &order,
                                   std::string &why);
bool fraction_same(const satellite_fraction &left, const satellite_fraction &right);
signed long long int fraction_to_string(const satellite_fraction &value, satellite_string &out, std::string &why);
signed long long int fraction_to_number(const satellite_fraction &value, satellite_number &out, std::string &why);
signed long long int fraction_to_binary(const satellite_fraction &value, satellite_string &out, std::string &why);
signed long long int fraction_to_hexadecimal(const satellite_fraction &value, satellite_string &out, std::string &why);

// ONE OF ITS TWO NUMBERS AS DISPLAY WRITES IT -- "display them as just the whole
// number" (the author, 2026-09-22): 3 for a float with nothing after its point,
// never 3.0, and 1.5 for one that has something there.
std::string fraction_part_written(const satellite_float &part);

// WHAT satellite.console.display PRINTS AND `.string` ANSWERS: the two numbers as
// they were written, round a touching slash -- 1/3, 2/4, -1/3, 1.5/2.
std::string fraction_written(const satellite_fraction &value);

// A PLAIN NUMBER AS A FRACTION: 3 is 3/1. How a number given to a fraction name is
// kept, and how a number is read when it is compared with a fraction.
satellite_fraction fraction_of_number(const satellite_number &value);

} // namespace satellite004
