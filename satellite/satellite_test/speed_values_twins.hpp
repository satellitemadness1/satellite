#pragma once
// satellite/satellite_test/speed_values_twins.hpp -- the C++ twins of satellite.test.speed()'s values,
// arithmetic and statements sections. Each twin does the same work as speed_<name>.satl, statement for
// statement, and answers exactly the line that program displays (without its newline). passes is the number
// written on the first line of that program's satellite.main, `satellite.variable.number passes = <N>`.

#include <string>

namespace satellite004 {

std::string speed_while(long long passes);        // speed_while.satl
std::string speed_for(long long passes);          // speed_for.satl
std::string speed_if_else(long long passes);      // speed_if_else.satl
std::string speed_compare(long long passes);      // speed_compare.satl
std::string speed_arithmetic(long long passes);   // speed_arithmetic.satl
std::string speed_big_numbers(long long passes);  // speed_big_numbers.satl
std::string speed_huge_numbers(long long passes); // speed_huge_numbers.satl
std::string speed_trillion(long long passes);     // speed_trillion.satl
std::string speed_float(long long passes);        // speed_float.satl
std::string speed_percent(long long passes);      // speed_percent.satl
std::string speed_hex_binary(long long passes);   // speed_hex_binary.satl
std::string speed_bool(long long passes);         // speed_bool.satl
std::string speed_conversions(long long passes);  // speed_conversions.satl
std::string speed_fraction(long long passes);     // speed_fraction.satl

} // namespace satellite004
