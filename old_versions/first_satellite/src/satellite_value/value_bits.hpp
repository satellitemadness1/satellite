#pragma once

#include "satellite_value/value_types.hpp"

namespace satellite {

// --- §21's operations on a binary or hex value, defined in bits.cpp ---------
//
// Free functions rather than members of Bits, for the reason every other
// operation in this header is a free function: a Bits inside a Value is behind
// a shared_ptr<const>, so nothing may mutate one, and a method that cannot
// mutate is a function that takes the value.

// Is `c` a digit in this radix? Hex accepts either case; `bits_normalise` is
// what makes the two one value.
bool bits_valid_digit(unsigned radix, char c);

// 0..15, or -1 for anything that is not a hex digit.
int bits_digit_value(char c);

// Upper-cases hex digits. The digit COUNT is never changed.
std::string bits_normalise(unsigned radix, std::string digits);

// 1 for binary, 4 for hex.
unsigned bits_per_digit(unsigned radix);

// What .bytes() answers: the packed size, rounding up to a whole byte.
size_t bits_byte_count(const Bits &bits);

// Exact base conversion, both directions, in decimal string arithmetic. See
// bits.cpp for why this is not done with Number::divide.
std::string bits_to_decimal(unsigned radix, const std::string &digits);
std::string bits_from_decimal(unsigned radix, const std::string &decimal,
                              size_t min_digits);

// Value-preserving. Exact in width for hex -> binary; hex is the direction that
// rounds a width up, and bits.cpp says so at the site.
Bits bits_convert(const Bits &from, unsigned to_radix);

// The packed bytes, big-endian and left-padded to a whole byte. Packing LOSES
// the digit count — `x0F` and `x000F` pack alike — so bits_unpack takes the
// count back, and §20.3 sends it beside the bytes.
std::string bits_pack(const Bits &bits);
Bits bits_unpack(unsigned radix, const std::string &bytes, size_t digit_count);

} // namespace satellite
