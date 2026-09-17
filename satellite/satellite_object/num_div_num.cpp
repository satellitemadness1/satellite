// satellite/satellite_object/num_div_num.cpp -- THE SAME PAIR, THROUGH THE
// TEMPLATE. The author: "then build with template programming division".
//
// Put this file beside num_add_num.cpp and the difference is the whole argument
// for object_pair.hpp: the five steps written out there are one line here, and
// they compile to the same instructions because `Arm` and `Operation` are known
// at compile time.
//
// DIVISION IS THE RIGHT ONE TO DO FIRST, because it is the first pair that can
// REFUSE -- division_by_zero (22) -- so it shows the template carrying a machine
// code back out rather than only carrying a value.
//
// WHOLE-NUMBER DIVISION, TRUNCATED TOWARD ZERO: 7 / 2 is 3, -7 / 2 is -3, which
// check_numbers.py has checked against Python across 482,465 cases.

#include "fast_paths.hpp"
#include "object_pair.hpp"
#include "number_and_number_divide.hpp"

namespace satellite004 {

signed long long int num_div_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    return object_pair<satellite_number, satellite_number>(
        left, right, out, number_and_number_divide,
        [](satellite_number answer) { return satelliteObject::of_number(std::move(answer)); });
}

} // namespace satellite004
