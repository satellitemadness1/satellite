// satellite.variable.binary and satellite.variable.hex — the operations. §21.
//
// Everything here works on the DIGITS AS WRITTEN, because the width is part of
// the value (see Bits in value.hpp). The two directions that leave this type —
// .to_number() and the packed bytes the wire format sends — are the two places
// a width can be lost, and each of them says so at the site.
//
// The decimal bridge is done in DECIMAL STRING ARITHMETIC and not with
// Number::divide, deliberately. divide() takes a digit count, which is a
// precision, and a base conversion has no business choosing one: an integer in
// base 16 is an exact integer in base 10 and the conversion is exact or it is
// wrong. Long multiplication and long division over a digit string are exact by
// construction, and §8.1's whole argument is that exactness is what this
// language trades performance for.

#include "satellite_value/value.hpp"

#include <algorithm>
#include <cctype>

namespace satellite {

bool bits_valid_digit(unsigned radix, char c)
{
    if (radix == 2)
        return c == '0' || c == '1';
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
           (c >= 'A' && c <= 'F');
}

int bits_digit_value(char c)
{
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

// Upper case for hex, so `x00ff` and `x00FF` are one value. A normalisation of
// SPELLING only: the count of digits is untouched, which is the invariant this
// whole type exists to keep.
std::string bits_normalise(unsigned radix, std::string digits)
{
    if (radix == 16)
        for (char &c : digits)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return digits;
}

unsigned bits_per_digit(unsigned radix) { return radix == 2 ? 1u : 4u; }

// Bytes the value occupies once packed, rounding UP: seven bits are one byte,
// because there is no such thing as a seven-bit byte on a socket. This is what
// .bytes() answers and what the wire format allocates.
size_t bits_byte_count(const Bits &bits)
{
    const size_t total = bits.digits.size() * bits_per_digit(bits.radix);
    return (total + 7) / 8;
}

// --- the decimal bridge ----------------------------------------------------

// digits (base `radix`) -> a decimal digit string, exact. Horner by long
// multiplication: result = result * radix + digit, with result held as decimal
// digits most-significant first.
std::string bits_to_decimal(unsigned radix, const std::string &digits)
{
    std::vector<int> acc;    // decimal digits, LEAST significant first
    for (char c : digits) {
        int carry = bits_digit_value(c);
        for (size_t i = 0; i < acc.size(); i++) {
            const int v = acc[i] * static_cast<int>(radix) + carry;
            acc[i] = v % 10;
            carry = v / 10;
        }
        while (carry) {
            acc.push_back(carry % 10);
            carry /= 10;
        }
    }
    if (acc.empty())
        return "0";
    std::string out;
    out.reserve(acc.size());
    for (size_t i = acc.size(); i-- > 0;)
        out += static_cast<char>('0' + acc[i]);
    return out;
}

// A decimal digit string -> digits in `radix`, by repeated long division.
// Left-padded to `min_digits`, which is how a caller asks for a WIDTH: a
// conversion that knows the answer must be four digits wide says so here rather
// than patching the result afterwards.
std::string bits_from_decimal(unsigned radix, const std::string &decimal,
                              size_t min_digits)
{
    std::string work;
    for (char c : decimal)
        if (c >= '0' && c <= '9')
            work += c;

    std::string out;                     // digits, LEAST significant first
    static const char *kDigits = "0123456789ABCDEF";
    while (!work.empty()) {
        std::string next;
        int carry = 0;
        for (char c : work) {
            const int v = carry * 10 + (c - '0');
            const int q = v / static_cast<int>(radix);
            carry = v % static_cast<int>(radix);
            if (!next.empty() || q)
                next += static_cast<char>('0' + q);
        }
        out += kDigits[carry];
        work = next;
    }
    while (out.size() < min_digits)
        out += '0';
    if (out.empty())
        out += '0';
    std::reverse(out.begin(), out.end());
    return out;
}

// --- conversion between the two radices ------------------------------------

// Value-preserving. hex -> binary is exact in BOTH directions because one hex
// digit is exactly four binary digits, so a width of n becomes 4n and comes
// back as n.
//
// binary -> hex is exact in value and NOT in width: 3 binary digits are one hex
// digit, and that hex digit converts back to 4. A program that round-trips
// `b101` through hex gets `b0101`. That is stated rather than hidden, and it is
// the reason .to_hex() is a method a program asks for rather than something
// that happens on its own.
Bits bits_convert(const Bits &from, unsigned to_radix)
{
    if (from.radix == to_radix)
        return from;

    if (from.radix == 16 && to_radix == 2) {
        std::string out;
        out.reserve(from.digits.size() * 4);
        for (char c : from.digits) {
            const int v = bits_digit_value(c);
            for (int bit = 3; bit >= 0; bit--)
                out += ((v >> bit) & 1) ? '1' : '0';
        }
        return Bits{2, out};
    }

    // binary -> hex: four binary digits per hex digit, so the count rounds up.
    const size_t hex_digits = (from.digits.size() + 3) / 4;
    std::string padded(hex_digits * 4 - from.digits.size(), '0');
    padded += from.digits;
    std::string out;
    out.reserve(hex_digits);
    static const char *kDigits = "0123456789ABCDEF";
    for (size_t i = 0; i < padded.size(); i += 4) {
        int v = 0;
        for (size_t j = 0; j < 4; j++)
            v = v * 2 + (padded[i + j] - '0');
        out += kDigits[v];
    }
    return Bits{16, out};
}

// --- the wire, and any other packed byte form -------------------------------

// Big-endian, left-padded to a whole byte. Big-endian because §20.4 puts an
// endianness marker in the frame for the machine's OWN words and this is not
// one of those: a hex constant is written most-significant digit first, and the
// bytes it names are that order. Reversing them on one architecture would make
// `x0102` arrive as `x0201`, which is not an endianness question at all.
std::string bits_pack(const Bits &bits)
{
    const Bits as_binary = bits_convert(bits, 2);
    const size_t bytes = (as_binary.digits.size() + 7) / 8;
    std::string padded(bytes * 8 - as_binary.digits.size(), '0');
    padded += as_binary.digits;

    std::string out;
    out.reserve(bytes);
    for (size_t i = 0; i < padded.size(); i += 8) {
        unsigned v = 0;
        for (size_t j = 0; j < 8; j++)
            v = v * 2 + static_cast<unsigned>(padded[i + j] - '0');
        out += static_cast<char>(v);
    }
    return out;
}

// The inverse, given the digit count the value had — WHICH MUST BE CARRIED
// SEPARATELY. Packing loses it: `x0F` and `x000F` pack to the same one byte,
// so a wire format that sent bytes alone would hand back a value of a different
// width than the one that was sent, and the width is the whole point of the
// type. §20.3's frame therefore sends the digit count beside the bytes.
Bits bits_unpack(unsigned radix, const std::string &bytes, size_t digit_count)
{
    std::string binary;
    binary.reserve(bytes.size() * 8);
    for (unsigned char b : bytes)
        for (int bit = 7; bit >= 0; bit--)
            binary += ((b >> bit) & 1) ? '1' : '0';

    const size_t want = digit_count * bits_per_digit(radix);
    if (binary.size() > want)
        binary.erase(0, binary.size() - want);
    while (binary.size() < want)
        binary.insert(binary.begin(), '0');

    return bits_convert(Bits{2, binary}, radix);
}

} // namespace satellite
