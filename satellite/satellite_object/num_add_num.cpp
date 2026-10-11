// satellite/satellite_object/num_add_num.cpp -- str_add_str.cpp's shape with the
// arm and the operation changed. The author asked for it next, and the point of
// asking for it next is that nothing about the shape moved.
//
// CANNOT REFUSE. Two whole numbers added are a whole number, and satellite_number
// has no ceiling to overflow past -- but the machine code is answered anyway, so
// every pair file is one kind of pointer (ObjectPair).

#include "fast_paths.hpp"
#include "number_and_number_add.hpp"

namespace satellite004 {

signed long long int num_add_num(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_number *l = left.as_number();
    const satellite_number *r = right.as_number();
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    satellite_number answer;
    const signed long long int code = number_and_number_add(*l, *r, answer);
    if (code != success)
        return code;
    out = satelliteObject::of_number(std::move(answer));
    return success;
}

} // namespace satellite004
