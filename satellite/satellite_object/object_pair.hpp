#pragma once
// satellite/satellite_object/object_pair.hpp -- THE PAIR FILES, GENERALISED.
//
// (the author, 2026-09-16) "then build with template programming division", and
// "it's just more templates built out of our fast paths of functions that we
// defined then".
//
// str_add_str.cpp, num_add_num.cpp and num_sub_num.cpp each write the same five
// steps out by hand -- ask both variants for one arm, refuse if either is not
// it, call the *_and_*.hpp function that does the work, wrap the answer back up.
// Written once, that is this template. num_div_num.cpp is a single line through
// it, and every pair file after it can be.
//
// WHY BOTH FORMS ARE KEPT. The hand-written ones are the TEMPLATE in the author's
// sense -- the shape a reader copies -- and this is the template in C++'s sense.
// A pair that does something unusual (str_minus_str's removal loop,
// str_find_str's position) writes itself out; a pair that is only "unwrap, call,
// wrap" is one line here. Neither costs anything at run time: `Arm` and
// `Operation` are known at compile time, so this inlines to what the hand-written
// version compiles to.
//
// `Answer` IS A SEPARATE PARAMETER because a pair does not always answer its own
// kind: two numbers divided answer a number, but two strings compared answer a
// bool and a string searched answers a number.

#include "satellite_object.hpp"

namespace satellite004 {

// Both sides must hold `Arm`; `operation` does the work on two of them and fills
// an `Answer`; `wrap` turns that Answer into a satelliteObject.
template <typename Arm, typename Answer, typename Operation, typename Wrap>
inline signed long long int object_pair(const satelliteObject &left, const satelliteObject &right,
                                        satelliteObject &out, Operation operation, Wrap wrap)
{
    const Arm *l = arm_of<Arm>(left);
    const Arm *r = arm_of<Arm>(right);
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    Answer answer;
    const signed long long int code = operation(*l, *r, answer);
    if (code != success)
        return code;
    out = wrap(std::move(answer));
    return success;
}

} // namespace satellite004
