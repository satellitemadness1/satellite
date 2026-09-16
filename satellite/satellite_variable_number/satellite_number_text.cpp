// satellite/satellite_variable_number/satellite_number_text.cpp -- satellite_number
// as decimal text: from_text, to_text and digits().
//
// THE RULE (the header's): an optional '-' then one or more ASCII digits 0-9, and
// nothing else -- no '+', no spaces, no separators, no decimal point, no other
// script's digits. Leading zeros are allowed, and "-0" is 0, never negative. A
// text that breaks the rule answers int_error (3), bad_offset is the first byte
// that cannot be part of the number, and out is left untouched:
//     ""  -> 0      "-"  -> 1 (a digit was needed there)     "+1" -> 0
//     " 1" -> 0     "1 " -> 1     "1.5" -> 1     "--1" -> 1     "1\0" -> 1
//
// NINETEEN DIGITS A STEP. 10^19 is the largest power of ten below 2^64, so from_text
// takes the digits nineteen at a time (number = number * 10^19 + chunk) and
// to_text divides by 10^19 once per pass, keeping each remainder as nineteen
// digits. Both are quadratic in the length: correct at every size, and a million
// digits takes seconds (measured by check_numbers.py), not a limit.
//
// digits() of a one-limb number is counted directly; of a larger one, from its bit
// length, checked exactly against powers of ten (see digits()) -- a million digits
// in about a second instead of converting the whole number to text.

#include "satellite_number.hpp"
#include "satellite_number_limbs.hpp"
#include "../machine/machine_codes.hpp"

#include <bit>
#include <cmath>
#include <utility>

namespace satellite004 {

using number_limbs::double_limb;
using number_limbs::limb_type;

namespace {

const limb_type ten_to_the_19 = 10000000000000000000ull;

limb_type chunk_value(const char *digits, std::size_t count)
{
    limb_type value = 0;
    for (std::size_t index = 0; index < count; index++)
        value = value * 10 + (limb_type)(digits[index] - '0');
    return value;
}

// Writes value as exactly width digits (leading zeros) ending at end.
void write_digits(limb_type value, char *end, std::size_t width)
{
    for (std::size_t index = 0; index < width; index++) {
        *--end = (char)('0' + value % 10);
        value /= 10;
    }
}

std::size_t digits_of_limb(limb_type value)
{
    std::size_t count = 1;
    while (value >= 10) {
        value /= 10;
        count++;
    }
    return count;
}

} // namespace

signed long long int satellite_number::from_text(const std::string &text, satellite_number &out, std::size_t &bad_offset)
{
    const std::size_t size = text.size();
    std::size_t first = (size > 0 && text[0] == '-') ? 1 : 0;
    if (first == size) {
        bad_offset = first; // "" -> 0, "-" -> 1: a digit was needed there
        return int_error;
    }
    for (std::size_t index = first; index < size; index++) {
        if (text[index] < '0' || text[index] > '9') {
            bad_offset = index;
            return int_error;
        }
    }
    const bool negative = first == 1;
    while (first + 1 < size && text[first] == '0') // leading zeros change nothing
        first++;
    const std::size_t digit_count = size - first;
    const char *digits = text.data() + first;

    if (digit_count <= 19) {
        out = satellite_number(chunk_value(digits, digit_count), negative);
        return success;
    }
    std::vector<limb_type> limbs;
    limbs.reserve(digit_count / 19 + 1);
    std::size_t position = digit_count % 19 == 0 ? 19 : digit_count % 19;
    limbs.push_back(chunk_value(digits, position));
    for (; position < digit_count; position += 19) {
        limb_type carry = chunk_value(digits + position, 19);
        for (limb_type &each : limbs) {
            const double_limb step = (double_limb)each * ten_to_the_19 + carry;
            each = (limb_type)step;
            carry = (limb_type)(step >> 64);
        }
        if (carry != 0)
            limbs.push_back(carry);
    }
    out.adopt(std::move(limbs), negative);
    return success;
}

std::string satellite_number::to_text() const
{
    if (fits_one_limb()) {
        char buffer[21];
        std::size_t width = digits_of_limb(small_);
        write_digits(small_, buffer + 21, width);
        std::string text;
        text.reserve(width + 1);
        if (negative_)
            text.push_back('-');
        text.append(buffer + 21 - width, width);
        return text;
    }
    std::vector<limb_type> work(*large_);
    std::vector<limb_type> chunks; // least significant first, nineteen digits each
    chunks.reserve(work.size() * 20 / 19 + 1);
    const number_limbs::normalized_divisor by = number_limbs::make_normalized_divisor(ten_to_the_19);
    std::size_t count = work.size();
    while (count > 1) {
        chunks.push_back(number_limbs::divide_in_place(work.data(), count, by));
        while (count > 0 && work[count - 1] == 0)
            count--;
    }
    // A quotient of a number of two or more limbs is at least 1, so exactly one
    // non-zero limb is left, and below 2^64 it holds at most two chunks.
    limb_type top = work[0];
    if (top >= ten_to_the_19) {
        chunks.push_back(top % ten_to_the_19);
        top /= ten_to_the_19;
    }
    chunks.push_back(top);

    const std::size_t lead = digits_of_limb(chunks.back());
    std::string text((negative_ ? 1 : 0) + lead + (chunks.size() - 1) * 19, '0');
    char *end = text.data() + text.size();
    for (std::size_t index = 0; index + 1 < chunks.size(); index++, end -= 19)
        write_digits(chunks[index], end, 19);
    write_digits(chunks.back(), end, lead);
    if (negative_)
        text[0] = '-';
    return text;
}

// 10^exponent, by squaring.
static satellite_number power_of_ten(std::size_t exponent)
{
    satellite_number answer(1);
    const satellite_number ten(10);
    for (int bit = 63; bit >= 0; bit--) {
        answer *= answer;
        if ((exponent >> bit) & 1)
            answer *= ten;
    }
    return answer;
}

satellite_number satellite_number::digits() const
{
    if (fits_one_limb())
        return satellite_number((unsigned long long int)digits_of_limb(small_));
    // A number of b bits lies in [2^(b-1), 2^b), so it has the digits of 2^(b-1)
    // or one more. estimate is that count from a double, then exact comparisons
    // with 10^(estimate-1) and 10^estimate settle it: each step moves one digit,
    // so a double that is off at an astronomical size costs steps, never the answer.
    const std::size_t count = large_->size();
    const double bits = (double)(count - 1) * 64.0 + (double)(64 - std::countl_zero(large_->back()));
    std::size_t estimate = (std::size_t)std::floor((bits - 1.0) * std::log10(2.0)) + 1;
    const satellite_number magnitude = negative_ ? -*this : *this;
    satellite_number low = power_of_ten(estimate - 1), quotient, remainder;
    const satellite_number ten(10);
    while (magnitude < low) { // below 10^(estimate-1): fewer digits
        divide(low, ten, quotient, remainder);
        low = std::move(quotient);
        estimate--;
    }
    satellite_number high = low * ten;
    while (magnitude >= high) { // at or above 10^estimate: more digits
        low = high;
        high *= ten;
        estimate++;
    }
    return satellite_number((unsigned long long int)estimate);
}

} // namespace satellite004
