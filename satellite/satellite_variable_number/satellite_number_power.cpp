// satellite/satellite_variable_number/satellite_number_power.cpp -- power, and
// the number in base 2 and base 16. The header says what each refuses and why.
//
// POWER IS BY SQUARING, MOST SIGNIFICANT BIT FIRST, over the exponent's own
// limbs. The obvious loop -- multiply by base, exponent times -- is linear in the
// VALUE of the exponent, so 2 ^ 1000 would be 1000 multiplications; this is
// linear in its BIT LENGTH, so the same answer costs 10 squarings and 6
// multiplies. That difference is not a micro-optimisation: the exponent is a
// satellite_number with no upper bound, so a loop counting up to it is a loop
// that does not finish, while a loop over its bits always does.
//
// THE ANSWER IS BUILT IN A LOCAL AND MOVED OUT AT THE END, which is what makes
// power(x, y, x) -- out aliasing either argument -- correct: `value` and `times`
// are copies taken before anything is written.
//
// BASE 2 AND BASE 16 ARE READ STRAIGHT OFF THE LIMBS, not by repeated division,
// because both are powers of two: a digit is 1 bit or 4 bits and never straddles
// a limb (64 is a multiple of both). So the conversion is a shift and a mask per
// digit, exact by construction, and there is no division at all -- unlike
// to_text(), which must divide because 10 is not a power of two.

#include "satellite_number.hpp"
#include "satellite_number_limbs.hpp"
#include "../machine/machine_codes.hpp"

#include <bit>
#include <utility>

namespace satellite004 {

using number_limbs::limb_type;

signed long long int satellite_number::power(const satellite_number &base, const satellite_number &exponent,
                                             satellite_number &out)
{
    // Copies first: out may be base, or exponent, or both.
    const satellite_number value = base;
    const satellite_number times = exponent;

    if (times.negative()) {
        // 0 ^ -n is 1/0. Said as division_by_zero because that is what it is,
        // and a program that meant it gets the sharper of the two words.
        if (value.is_zero())
            return division_by_zero;
        // |base| == 1 is the whole family whose reciprocal is still whole.
        if (value.fits_one_limb() && value.limb(0) == 1) {
            const bool odd = (times.limb(0) & 1ull) != 0;
            out = satellite_number(1ull, value.negative() && odd);
            return success;
        }
        return answer_is_not_whole;
    }

    if (times.is_zero()) { // x ^ 0 is 1, and 0 ^ 0 is 1
        out = satellite_number(1ull);
        return success;
    }

    // The top limb of a settled number is never zero, and times is not zero, so
    // countl_zero is asked about a limb that has a bit set.
    const std::size_t limbs = times.limb_count();
    int bit = 63 - std::countl_zero(times.limb(limbs - 1));

    satellite_number answer(1ull);
    for (std::size_t index = limbs; index-- > 0; ) {
        const limb_type digit = times.limb(index);
        for (; bit >= 0; --bit) {
            answer *= answer;            // a *= a is correct: the header says so
            if ((digit >> bit) & 1ull)
                answer *= value;
        }
        bit = 63;                        // every limb below the top is full width
    }
    out = std::move(answer);
    return success;
}

namespace {

// 1 for base 2, 4 for base 16. 0 says the radix is not one this handles.
unsigned int bits_per_digit(unsigned int radix)
{
    if (radix == 2) return 1;
    if (radix == 16) return 4;
    return 0;
}

char digit_character(unsigned int value)
{
    return value < 10 ? (char)('0' + value) : (char)('A' + (value - 10));
}

// The digit a character stands for, or 16 for one that is not a digit of this
// radix. Upper and lower case both read, because a program may write either.
unsigned int character_digit(char c, unsigned int radix)
{
    unsigned int value = 16;
    if (c >= '0' && c <= '9') value = (unsigned int)(c - '0');
    else if (c >= 'a' && c <= 'f') value = (unsigned int)(c - 'a') + 10;
    else if (c >= 'A' && c <= 'F') value = (unsigned int)(c - 'A') + 10;
    return value < radix ? value : 16;
}

} // namespace

std::string satellite_number::to_radix_text(unsigned int radix) const
{
    const unsigned int width = bits_per_digit(radix);
    if (width == 0)
        return std::string();
    if (is_zero())
        return std::string("0");

    // The bit length: whole limbs below the top one, plus the bits used in it.
    const std::size_t count = limb_count();
    const limb_type top = limb(count - 1);
    const std::size_t bits = (count - 1) * 64 + (std::size_t)(64 - std::countl_zero(top));
    const std::size_t digits = (bits + width - 1) / width;

    std::string text;
    text.reserve(digits + (negative_ ? 1 : 0));
    if (negative_)
        text.push_back('-');
    // Highest digit first. A digit never straddles a limb: 64 is a multiple of
    // both 1 and 4, so one shift and one mask reads it.
    for (std::size_t index = digits; index-- > 0; ) {
        const std::size_t at = index * width;
        const limb_type held = limb(at / 64);
        text.push_back(digit_character((unsigned int)((held >> (at % 64)) & ((1ull << width) - 1))));
    }
    return text;
}

signed long long int satellite_number::from_radix_text(const std::string &text, unsigned int radix,
                                                       satellite_number &out, std::size_t &bad_offset)
{
    const unsigned int width = bits_per_digit(radix);
    if (width == 0) {
        bad_offset = 0;
        return int_error;
    }
    const std::size_t size = text.size();
    std::size_t first = (size > 0 && text[0] == '-') ? 1 : 0;
    if (first == size) {          // "" -> 0, "-" -> 1: a digit was needed there
        bad_offset = first;
        return int_error;
    }
    for (std::size_t index = first; index < size; index++) {
        if (character_digit(text[index], radix) == 16) {
            bad_offset = index;
            return int_error;
        }
    }
    const bool negative = first == 1;
    while (first + 1 < size && text[first] == '0')   // leading zeros change nothing
        first++;

    // Lowest digit last, so the limbs fill from the END of the text backwards.
    const std::size_t digits = size - first;
    std::vector<limb_type> limbs((digits * width + 63) / 64, 0);
    for (std::size_t index = 0; index < digits; index++) {
        const std::size_t at = index * width;         // index 0 is the LAST character
        const limb_type held = (limb_type)character_digit(text[size - 1 - index], radix);
        limbs[at / 64] |= held << (at % 64);
    }
    out.adopt(std::move(limbs), negative);
    return success;
}

} // namespace satellite004
