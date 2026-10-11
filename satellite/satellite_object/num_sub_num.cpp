// satellite/satellite_object/num_sub_num.cpp -- the same again. Negative answers
// are ordinary: the sign is a bool carried with the number, so 3 - 5 is -2 and
// nothing wraps.
//
// CANNOT REFUSE, for add's reason exactly.

#include "fast_paths.hpp"
#include "number_and_number_subtract.hpp"

namespace satellite004 {

signed long long int num_sub_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_number *l = left.as_number();
    const satellite_number *r = right.as_number();
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    satellite_number answer;
    const signed long long int code = number_and_number_subtract(*l, *r, answer);
    if (code != success)
        return code;
    out = satelliteObject::of_number(std::move(answer));
    return success;
}

} // namespace satellite004
