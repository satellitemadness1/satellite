#pragma once
// time_test/satellite.library/cpp/bignum.hpp -- a whole number with no upper limit, for
// the C++ equivalents of race_program, big_numbers, long_add, long_multiply and everything.
//
// Hand-written because C++ has no such number, and standard library only (no GMP: the
// author's other machine may not have it). It is built the way satl builds its own, in
// satellite/satellite_variable_number/, so the race compares the same class of algorithm:
//   - 64-bit limbs, least significant first, no zero limb at the top (zero has no limbs);
//   - multiply: schoolbook over unsigned __int128, every limb of one number times every
//     limb of the other into a fresh run (satl's multiply_in_place);
//   - add and subtract: one pass with a carry or a borrow;
//   - divide by one limb: one pass, top limb first;
//   - divide by a many-limb number: Knuth's algorithm D (The Art of Computer Programming,
//     volume 2, section 4.3.1) in base 2^64, the divisor shifted so its top bit is set --
//     the same steps as divide_long in satl's satellite_number_divide.cpp;
//   - decimal text nineteen digits at a time, 10^19 being the largest power of ten in a limb.
// Clear rather than clever: satl divides by one limb with a precomputed reciprocal, this
// uses the machine's own 128-by-64 division. Only the display ever divides by one limb
// in these five programs, so the difference is not in anything the race times.
//
// UNSIGNED. Every value these five programs make is zero or more, so there is no sign;
// subtracting a larger number from a smaller one throws rather than answer wrongly.
//
// 128-PLACE FLOATS are at the bottom: a float is a bignum scaled by 10^128, the way satl
// works a float ("12.05 is 1205 at 2 places").

#include <bit>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

class bignum {
public:
    using limb = unsigned long long;
    using wide = unsigned __int128;

    bignum() = default;
    explicit bignum(limb value)
    {
        if (value != 0)
            limbs_.push_back(value);
    }

    // ---- decimal text -------------------------------------------------------------------

    // One or more ASCII digits and nothing else; nineteen digits a step,
    // number = number * 10^19 + chunk, as satl's from_text does.
    static bignum from_text(std::string_view digits)
    {
        if (digits.empty() || digits.find_first_not_of("0123456789") != std::string_view::npos)
            throw std::invalid_argument("bignum::from_text: not a whole number: " + std::string(digits));
        bignum number;
        std::size_t start = 0;
        std::size_t chunk_length = digits.size() % 19 == 0 ? 19 : digits.size() % 19;
        while (start < digits.size()) {
            limb chunk = 0;
            for (std::size_t index = start; index < start + chunk_length; index++)
                chunk = chunk * 10 + static_cast<limb>(digits[index] - '0');
            number = number * ten_to_the_19 + chunk;
            start += chunk_length;
            chunk_length = 19;
        }
        return number;
    }

    // Divides by 10^19 once per pass and keeps each remainder as nineteen digits.
    std::string to_text() const
    {
        if (is_zero())
            return "0";
        std::vector<limb> work = limbs_;
        std::vector<limb> chunks; // least significant first
        while (!work.empty()) {
            chunks.push_back(divide_run(work, ten_to_the_19));
            trim(work);
        }
        std::string text = std::to_string(chunks.back());
        for (std::size_t index = chunks.size() - 1; index-- > 0;) {
            const std::string chunk = std::to_string(chunks[index]);
            text += std::string(19 - chunk.size(), '0') + chunk;
        }
        return text;
    }

    // ---- compare ------------------------------------------------------------------------

    bool is_zero() const { return limbs_.empty(); }

    // -1, 0 or 1.
    static int compare(const bignum &left, const bignum &right)
    {
        if (left.limbs_.size() != right.limbs_.size())
            return left.limbs_.size() < right.limbs_.size() ? -1 : 1;
        for (std::size_t index = left.limbs_.size(); index-- > 0;)
            if (left.limbs_[index] != right.limbs_[index])
                return left.limbs_[index] < right.limbs_[index] ? -1 : 1;
        return 0;
    }

    friend bool operator==(const bignum &left, const bignum &right) { return compare(left, right) == 0; }
    friend bool operator<(const bignum &left, const bignum &right) { return compare(left, right) < 0; }

    // ---- add and subtract ---------------------------------------------------------------

    // Each limb of other is read before the same limb of this is written, so other may be *this.
    bignum &operator+=(const bignum &other)
    {
        const std::size_t other_count = other.limbs_.size();
        limbs_.resize((limbs_.size() > other_count ? limbs_.size() : other_count) + 1, 0);
        limb carry = 0;
        std::size_t index = 0;
        for (; index < other_count; index++)
            limbs_[index] = add_with_carry(limbs_[index], other.limbs_[index], carry);
        for (; carry != 0; index++) // the limb added at the top always stops the carry
            limbs_[index] = add_with_carry(limbs_[index], 0, carry);
        trim(limbs_);
        return *this;
    }

    bignum &operator+=(limb other)
    {
        limbs_.push_back(0);
        limb carry = 0;
        limbs_[0] = add_with_carry(limbs_[0], other, carry);
        for (std::size_t index = 1; carry != 0; index++)
            limbs_[index] = add_with_carry(limbs_[index], 0, carry);
        trim(limbs_);
        return *this;
    }

    bignum &operator-=(const bignum &other)
    {
        if (compare(*this, other) < 0)
            throw std::domain_error("bignum: subtracting a larger number (bignum has no sign)");
        limb borrow = 0;
        std::size_t index = 0;
        for (; index < other.limbs_.size(); index++)
            limbs_[index] = subtract_with_borrow(limbs_[index], other.limbs_[index], borrow);
        for (; borrow != 0; index++) // this >= other, so a higher limb always stops the borrow
            limbs_[index] = subtract_with_borrow(limbs_[index], 0, borrow);
        trim(limbs_);
        return *this;
    }

    friend bignum operator+(bignum left, const bignum &right) { return left += right; }
    friend bignum operator+(bignum left, limb right) { return left += right; }
    friend bignum operator-(bignum left, const bignum &right) { return left -= right; }

    // ---- multiply -----------------------------------------------------------------------

    // Schoolbook into a fresh run, as satl's multiply_in_place.
    friend bignum operator*(const bignum &left, const bignum &right)
    {
        const std::size_t count = left.limbs_.size(), other_count = right.limbs_.size();
        bignum product;
        if (count == 0 || other_count == 0)
            return product;
        product.limbs_.assign(count + other_count, 0);
        for (std::size_t i = 0; i < count; i++) {
            const limb multiplier = left.limbs_[i];
            if (multiplier == 0)
                continue;
            limb carry = 0;
            for (std::size_t j = 0; j < other_count; j++) {
                const wide step = static_cast<wide>(multiplier) * right.limbs_[j] + product.limbs_[i + j] + carry;
                product.limbs_[i + j] = static_cast<limb>(step);
                carry = static_cast<limb>(step >> 64);
            }
            product.limbs_[i + other_count] = carry;
        }
        trim(product.limbs_);
        return product;
    }

    // By one limb: the same schoolbook with a one-limb right side, one pass.
    friend bignum operator*(const bignum &left, limb right)
    {
        bignum product;
        if (left.is_zero() || right == 0)
            return product;
        product.limbs_.assign(left.limbs_.size() + 1, 0);
        limb carry = 0;
        for (std::size_t j = 0; j < left.limbs_.size(); j++) {
            const wide step = static_cast<wide>(left.limbs_[j]) * right + carry;
            product.limbs_[j] = static_cast<limb>(step);
            carry = static_cast<limb>(step >> 64);
        }
        product.limbs_.back() = carry;
        trim(product.limbs_);
        return product;
    }

    // ---- divide -------------------------------------------------------------------------

    // By one limb.
    friend bignum operator/(const bignum &left, limb right)
    {
        if (right == 0)
            throw std::domain_error("bignum: division by zero");
        bignum quotient = left;
        divide_run(quotient.limbs_, right);
        trim(quotient.limbs_);
        return quotient;
    }

    friend limb operator%(const bignum &left, limb right)
    {
        if (right == 0)
            throw std::domain_error("bignum: division by zero");
        wide remainder = 0;
        for (std::size_t index = left.limbs_.size(); index-- > 0;)
            remainder = ((remainder << 64) | left.limbs_[index]) % right;
        return static_cast<limb>(remainder);
    }

    // By any number: both answers worked out before either is written, so any of the four
    // may be the same object.
    static void divide(const bignum &dividend, const bignum &divisor, bignum &quotient, bignum &remainder)
    {
        if (divisor.is_zero())
            throw std::domain_error("bignum: division by zero");
        if (compare(dividend, divisor) < 0) {
            bignum kept = dividend;
            quotient = bignum();
            remainder = std::move(kept);
            return;
        }
        if (divisor.limbs_.size() == 1) {
            bignum whole = dividend / divisor.limbs_[0];
            bignum left_over(dividend % divisor.limbs_[0]);
            quotient = std::move(whole);
            remainder = std::move(left_over);
            return;
        }
        bignum whole, left_over;
        divide_long(dividend.limbs_, divisor.limbs_, whole.limbs_, left_over.limbs_);
        trim(whole.limbs_);
        trim(left_over.limbs_);
        quotient = std::move(whole);
        remainder = std::move(left_over);
    }

    friend bignum operator/(const bignum &left, const bignum &right)
    {
        bignum quotient, remainder;
        divide(left, right, quotient, remainder);
        return quotient;
    }

    friend bignum operator%(const bignum &left, const bignum &right)
    {
        bignum quotient, remainder;
        divide(left, right, quotient, remainder);
        return remainder;
    }

    // dividend / divisor rounded half away from zero (half up: there is no sign), the way
    // satl's divided_rounded rounds a float.
    static bignum divided_rounded(const bignum &dividend, const bignum &divisor)
    {
        bignum quotient, remainder;
        divide(dividend, divisor, quotient, remainder);
        if (compare(remainder + remainder, divisor) >= 0)
            quotient += 1ull;
        return quotient;
    }

private:
    static constexpr limb ten_to_the_19 = 10000000000000000000ull;

    std::vector<limb> limbs_;

    static void trim(std::vector<limb> &limbs)
    {
        while (!limbs.empty() && limbs.back() == 0)
            limbs.pop_back();
    }

    static limb add_with_carry(limb left, limb right, limb &carry)
    {
        const wide sum = static_cast<wide>(left) + right + carry;
        carry = static_cast<limb>(sum >> 64);
        return static_cast<limb>(sum);
    }

    static limb subtract_with_borrow(limb left, limb right, limb &borrow)
    {
        const limb difference = left - right - borrow;
        borrow = (left < right) || (left - right < borrow);
        return difference;
    }

    // Divides the run in place by one limb, top limb first; answers the remainder.
    static limb divide_run(std::vector<limb> &limbs, limb divisor)
    {
        wide remainder = 0;
        for (std::size_t index = limbs.size(); index-- > 0;) {
            const wide current = (remainder << 64) | limbs[index];
            limbs[index] = static_cast<limb>(current / divisor);
            remainder = current % divisor;
        }
        return static_cast<limb>(remainder);
    }

    // run shifted left by shift bits (0-63), into one more limb than it has.
    static std::vector<limb> shifted_left(const std::vector<limb> &run, int shift)
    {
        std::vector<limb> shifted(run.size() + 1, 0);
        for (std::size_t index = 0; index < run.size(); index++) {
            shifted[index] |= run[index] << shift;
            if (shift != 0)
                shifted[index + 1] = run[index] >> (64 - shift);
        }
        return shifted;
    }

    // KNUTH'S ALGORITHM D. The divisor has at least two limbs and the dividend is at least
    // as large. Fills quotient and remainder (either may keep zero limbs at the top).
    static void divide_long(const std::vector<limb> &dividend, const std::vector<limb> &divisor,
                            std::vector<limb> &quotient, std::vector<limb> &remainder)
    {
        const std::size_t n = divisor.size(), m = dividend.size() - divisor.size();
        // D1: shift both so the divisor's top bit is set; u has one limb more than the
        // dividend, and v's extra top limb is zero and never read.
        const int shift = std::countl_zero(divisor.back());
        std::vector<limb> u = shifted_left(dividend, shift);
        const std::vector<limb> v = shifted_left(divisor, shift);
        const limb top = v[n - 1], second = v[n - 2];
        quotient.assign(m + 1, 0);

        for (std::size_t j = m + 1; j-- > 0;) { // D2, D7: each quotient limb, top first
            // D3: estimate from the top two limbs of u and the top limb of v, then test
            // with the next limb of each; the estimate is at most two too large.
            const wide numerator = (static_cast<wide>(u[j + n]) << 64) | u[j + n - 1];
            wide estimate = numerator / top;
            wide rest = numerator % top;
            while ((estimate >> 64) != 0 || estimate * second > ((rest << 64) | u[j + n - 2])) {
                estimate -= 1;
                rest += top;
                if ((rest >> 64) != 0)
                    break;
            }
            limb guess = static_cast<limb>(estimate);

            // D4: u[j .. j+n] -= guess * v
            limb carry = 0, borrow = 0;
            for (std::size_t i = 0; i < n; i++) {
                const wide product = static_cast<wide>(guess) * v[i] + carry;
                carry = static_cast<limb>(product >> 64);
                u[i + j] = subtract_with_borrow(u[i + j], static_cast<limb>(product), borrow);
            }
            u[j + n] = subtract_with_borrow(u[j + n], carry, borrow);

            // D5, D6: the guess was one too large (rare): add v back once.
            if (borrow != 0) {
                guess -= 1;
                limb add_carry = 0;
                for (std::size_t i = 0; i < n; i++)
                    u[i + j] = add_with_carry(u[i + j], v[i], add_carry);
                u[j + n] += add_carry; // wraps back to the true value; the carry out is dropped
            }
            quotient[j] = guess;
        }

        // D8: the remainder is the low n limbs of u, shifted back.
        remainder.assign(n, 0);
        for (std::size_t i = 0; i < n; i++)
            remainder[i] = shift == 0 ? u[i] : (u[i] >> shift) | (u[i + 1] << (64 - shift));
    }
};

// ---- 128-PLACE FLOATS ---------------------------------------------------------------------
// A float is a bignum scaled by 10^128: 1/3 is 10^128 / 3. satl keeps 128 places, rounds a
// division that does not end half away from zero, and shows a float rounded the same way to
// 32 places with the fraction's trailing zeros removed.

namespace scaled_float {

constexpr unsigned places = 128;
constexpr unsigned shown_places = 32;

// 10^count, built once per count asked for (satl keeps its last four powers of ten too).
inline const bignum &ten_to_the(unsigned count)
{
    static std::vector<bignum> kept;
    if (kept.empty()) {
        bignum power(1ull);
        for (unsigned index = 0; index <= places; index++) {
            kept.push_back(power);
            power = power * 10ull;
        }
    }
    return kept.at(count);
}

// A whole number as a float.
inline bignum from_whole(bignum::limb whole) { return bignum(whole) * ten_to_the(places); }

// A float divided by a whole number, rounded to 128 places.
inline bignum divided(const bignum &value, bignum::limb whole) { return bignum::divided_rounded(value, bignum(whole)); }

// Two floats multiplied: the product has 256 places, rounded back to 128.
inline bignum times(const bignum &left, const bignum &right)
{
    return bignum::divided_rounded(left * right, ten_to_the(places));
}

// Rounded to 32 places, trailing zeros after the point removed (and the point, when none are left).
inline std::string text(const bignum &value)
{
    const bignum shown = bignum::divided_rounded(value, ten_to_the(places - shown_places));
    bignum whole, fraction;
    bignum::divide(shown, ten_to_the(shown_places), whole, fraction);
    std::string digits = fraction.to_text();
    digits = std::string(shown_places - digits.size(), '0') + digits;
    while (!digits.empty() && digits.back() == '0')
        digits.pop_back();
    return digits.empty() ? whole.to_text() : whole.to_text() + "." + digits;
}

} // namespace scaled_float
