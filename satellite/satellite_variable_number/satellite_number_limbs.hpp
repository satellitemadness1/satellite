#pragma once
// satellite/satellite_variable_number/satellite_number_limbs.hpp -- arithmetic on
// raw runs of limbs, shared by satellite_number.cpp, satellite_number_divide.cpp
// and satellite_number_text.cpp. Not part of satellite_number's contract: nothing
// outside this folder includes it.
//
// A limb is one unsigned long long int; a run is least significant first. The
// only wider type used is unsigned __int128 (GCC and clang, the two compilers
// satellite builds with, including the Windows cross-build), for the product of
// two limbs and for one division of a two-limb number by a limb.
//
// Division by an invariant limb follows Moller and Granlund, "Improved division
// by invariant integers" (IEEE Transactions on Computers, 2011), algorithm 4: the
// divisor's reciprocal is found once, then each limb of the quotient costs two
// multiplications instead of a hardware division. to_text divides by
// 10,000,000,000,000,000,000 once per limb per pass, so this is its inner loop.

#include <cstddef>

namespace satellite004::number_limbs {

using limb_type = unsigned long long int;
using double_limb = unsigned __int128;

inline limb_type add_with_carry(limb_type left, limb_type right, limb_type &carry)
{
    double_limb sum = (double_limb)left + right + carry;
    carry = (limb_type)(sum >> 64);
    return (limb_type)sum;
}

inline limb_type subtract_with_borrow(limb_type left, limb_type right, limb_type &borrow)
{
    limb_type difference = left - right - borrow;
    borrow = (left < right) || (left - right < borrow);
    return difference;
}

// A divisor whose top bit is set, and its reciprocal floor((2^128 - 1) / divisor) - 2^64.
struct normalized_divisor {
    limb_type divisor;
    limb_type reciprocal;
};

inline normalized_divisor make_normalized_divisor(limb_type divisor) // divisor >= 2^63
{
    return {divisor, (limb_type)(~(double_limb)0 / divisor)};
}

// (high, low) / divisor, where high < divisor. Answers the quotient; remainder is set.
inline limb_type divide_two_by_one(limb_type high, limb_type low, const normalized_divisor &by, limb_type &remainder)
{
    double_limb estimate = (double_limb)by.reciprocal * high + (((double_limb)high << 64) | low);
    limb_type quotient = (limb_type)(estimate >> 64) + 1;
    limb_type estimate_low = (limb_type)estimate;
    limb_type left_over = low - quotient * by.divisor;
    if (left_over > estimate_low) {
        quotient -= 1;
        left_over += by.divisor;
    }
    if (left_over >= by.divisor) [[unlikely]] {
        quotient += 1;
        left_over -= by.divisor;
    }
    remainder = left_over;
    return quotient;
}

// Divides limbs[0..count) in place by a normalized divisor; answers the remainder.
inline limb_type divide_in_place(limb_type *limbs, std::size_t count, const normalized_divisor &by)
{
    limb_type remainder = 0;
    for (std::size_t index = count; index-- > 0;)
        limbs[index] = divide_two_by_one(remainder, limbs[index], by, remainder);
    return remainder;
}

// Compares two runs with no zero top limb beyond the first: -1, 0 or 1.
inline int compare_runs(const limb_type *left, std::size_t left_count, const limb_type *right, std::size_t right_count)
{
    if (left_count != right_count)
        return left_count < right_count ? -1 : 1;
    for (std::size_t index = left_count; index-- > 0;)
        if (left[index] != right[index])
            return left[index] < right[index] ? -1 : 1;
    return 0;
}

} // namespace satellite004::number_limbs
