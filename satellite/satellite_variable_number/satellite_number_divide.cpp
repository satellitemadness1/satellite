// satellite/satellite_variable_number/satellite_number_divide.cpp -- satellite_number::divide.
//
// TRUNCATION. The quotient is rounded toward zero and the remainder takes the
// dividend's sign, as C++ does: 7 / -2 is -3 remainder 1, -7 / 2 is -3 remainder -1.
// dividend == quotient * divisor + remainder always, and |remainder| < |divisor|.
// A divisor of 0 answers division_by_zero (22) and leaves both answers untouched.
//
// ALIASING. Both answers are worked out in full before either is written, so any
// of the four arguments may be the same object. When quotient and remainder are
// the same object it is written quotient first, remainder last, so it holds the
// REMAINDER (the header says so too).
//
// THE PATHS. Both numbers one limb: the machine's own / and %. A dividend smaller
// than the divisor: quotient 0, remainder the dividend. A one-limb divisor: one
// pass of divide_two_by_one (a reciprocal, no hardware division). Otherwise
// Knuth's algorithm D (The Art of Computer Programming, volume 2, section 4.3.1)
// in base 2^64, the divisor shifted so its top bit is set.

#include "satellite_number.hpp"
#include "satellite_number_limbs.hpp"
#include "../machine/machine_codes.hpp"

#include <bit>
#include <utility>

namespace satellite004 {

using number_limbs::double_limb;
using number_limbs::limb_type;

namespace {

// run[0..count) shifted left by shift bits (0-63) into count + 1 limbs.
std::vector<limb_type> shifted_left(const limb_type *run, std::size_t count, int shift)
{
    std::vector<limb_type> shifted(count + 1, 0);
    for (std::size_t index = 0; index < count; index++) {
        shifted[index] |= run[index] << shift;
        if (shift != 0)
            shifted[index + 1] = run[index] >> (64 - shift);
    }
    return shifted;
}

// Knuth D: dividend has dividend_count limbs, divisor has divisor_count >= 2 and
// |dividend| >= |divisor|. Fills quotient and remainder (both unnormalized runs).
void divide_long(const limb_type *dividend, std::size_t dividend_count, const limb_type *divisor, std::size_t divisor_count,
                 std::vector<limb_type> &quotient, std::vector<limb_type> &remainder)
{
    const int shift = std::countl_zero(divisor[divisor_count - 1]);
    std::vector<limb_type> u = shifted_left(dividend, dividend_count, shift);   // dividend_count + 1 limbs
    std::vector<limb_type> v = shifted_left(divisor, divisor_count, shift);     // the top extra limb is 0
    const std::size_t n = divisor_count, m = dividend_count - divisor_count;
    const limb_type top = v[n - 1], second = v[n - 2];
    quotient.assign(m + 1, 0);

    for (std::size_t j = m + 1; j-- > 0;) {
        // Estimate from the top two limbs, then correct with the third (at most twice).
        const double_limb numerator = ((double_limb)u[j + n] << 64) | u[j + n - 1];
        double_limb estimate = numerator / top;
        double_limb rest = numerator - estimate * top;
        while ((estimate >> 64) != 0 || estimate * second > ((rest << 64) | u[j + n - 2])) {
            estimate -= 1;
            rest += top;
            if ((rest >> 64) != 0)
                break;
        }
        limb_type guess = (limb_type)estimate;

        // u[j .. j+n] -= guess * v
        limb_type carry = 0, borrow = 0;
        for (std::size_t i = 0; i < n; i++) {
            const double_limb product = (double_limb)guess * v[i] + carry;
            carry = (limb_type)(product >> 64);
            u[i + j] = number_limbs::subtract_with_borrow(u[i + j], (limb_type)product, borrow);
        }
        u[j + n] = number_limbs::subtract_with_borrow(u[j + n], carry, borrow);

        if (borrow != 0) { // the guess was one too large (rare): add v back
            guess -= 1;
            limb_type add_carry = 0;
            for (std::size_t i = 0; i < n; i++)
                u[i + j] = number_limbs::add_with_carry(u[i + j], v[i], add_carry);
            u[j + n] += add_carry; // wraps back to the true value; the carry out is dropped by design
        }
        quotient[j] = guess;
    }

    remainder.assign(n, 0);
    for (std::size_t i = 0; i < n; i++)
        remainder[i] = shift == 0 ? u[i] : (u[i] >> shift) | (u[i + 1] << (64 - shift));
}

} // namespace

signed long long int satellite_number::divide(const satellite_number &dividend, const satellite_number &divisor,
                                              satellite_number &quotient, satellite_number &remainder)
{
    if (divisor.is_zero())
        return division_by_zero;
    const bool quotient_negative = dividend.negative_ != divisor.negative_;
    const bool remainder_negative = dividend.negative_;

    if (dividend.fits_one_limb() && divisor.fits_one_limb()) {
        const unsigned long long int whole = dividend.small_ / divisor.small_;
        const unsigned long long int left_over = dividend.small_ % divisor.small_;
        quotient = satellite_number(whole, quotient_negative);
        remainder = satellite_number(left_over, remainder_negative);
        return success;
    }

    if (compare_magnitude(dividend, divisor) < 0) {
        satellite_number kept(dividend);
        quotient = satellite_number();
        remainder = std::move(kept);
        return success;
    }

    std::vector<limb_type> whole, left_over;
    const std::size_t dividend_count = dividend.limb_count(), divisor_count = divisor.limb_count();
    if (divisor_count == 1) {
        const int shift = std::countl_zero(divisor.small_);
        whole = shifted_left(dividend.limb_data(), dividend_count, shift);
        const number_limbs::normalized_divisor by = number_limbs::make_normalized_divisor(divisor.small_ << shift);
        const limb_type shifted_remainder = number_limbs::divide_in_place(whole.data(), whole.size(), by);
        left_over.assign(1, shifted_remainder >> shift);
    } else {
        divide_long(dividend.limb_data(), dividend_count, divisor.limb_data(), divisor_count, whole, left_over);
    }
    quotient.adopt(std::move(whole), quotient_negative);
    remainder.adopt(std::move(left_over), remainder_negative);
    return success;
}

} // namespace satellite004
