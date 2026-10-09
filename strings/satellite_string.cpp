// satellite-004/strings/satellite_string.cpp -- UTF-8, char32_t and .sati bits.
// See satellite_string.hpp for why 32 bits and why every conversion is strict.
//
// The decoder follows the Unicode standard's table of well-formed UTF-8
// (Table 3-7). The second byte's allowed range depends on the first byte, and
// that dependency is what rejects overlong forms, surrogates and values above
// U+10FFFF without any separate check:
//
//   first byte   second byte   then
//   00..7F       -             -
//   C2..DF       80..BF        -
//   E0           A0..BF        80..BF
//   E1..EC       80..BF        80..BF
//   ED           80..9F        80..BF          (no surrogates)
//   EE..EF       80..BF        80..BF
//   F0           90..BF        80..BF 80..BF
//   F1..F3       80..BF        80..BF 80..BF
//   F4           80..8F        80..BF 80..BF   (nothing above U+10FFFF)

#include "satellite_string.hpp"

#include "../satellite/machine/machine_codes.hpp"

namespace satellite004 {

namespace {

bool is_scalar_value(char32_t c)
{
    return c <= 0x10FFFF && (c < 0xD800 || c > 0xDFFF);
}

} // namespace

signed long long int utf8_to_char32(const std::string &utf8, std::u32string &out, size_t &bad_offset)
{
    out.clear();
    out.reserve(utf8.size());
    const size_t size = utf8.size();
    size_t i = 0;
    while (i < size) {
        const unsigned char b0 = static_cast<unsigned char>(utf8[i]);
        if (b0 < 0x80) {                                     // the most likely case: ASCII
            out.push_back(b0);
            i++;
            continue;
        }
        unsigned int length;
        unsigned char low = 0x80, high = 0xBF;              // allowed range of the SECOND byte
        char32_t value;
        if (b0 >= 0xC2 && b0 <= 0xDF) { length = 2; value = b0 & 0x1F; }
        else if (b0 == 0xE0) { length = 3; value = b0 & 0x0F; low = 0xA0; }
        else if (b0 >= 0xE1 && b0 <= 0xEC) { length = 3; value = b0 & 0x0F; }
        else if (b0 == 0xED) { length = 3; value = b0 & 0x0F; high = 0x9F; }
        else if (b0 >= 0xEE && b0 <= 0xEF) { length = 3; value = b0 & 0x0F; }
        else if (b0 == 0xF0) { length = 4; value = b0 & 0x07; low = 0x90; }
        else if (b0 >= 0xF1 && b0 <= 0xF3) { length = 4; value = b0 & 0x07; }
        else if (b0 == 0xF4) { length = 4; value = b0 & 0x07; high = 0x8F; }
        else { bad_offset = i; return string_error; }       // 80..C1, F5..FF can never start a character

        for (unsigned int k = 1; k < length; k++) {
            if (i + k >= size) { bad_offset = i; return string_error; }
            const unsigned char b = static_cast<unsigned char>(utf8[i + k]);
            const unsigned char from = k == 1 ? low : 0x80, to = k == 1 ? high : 0xBF;
            if (b < from || b > to) { bad_offset = i; return string_error; }
            value = (value << 6) | (b & 0x3F);
        }
        out.push_back(value);
        i += length;
    }
    return success;
}

signed long long int char32_to_utf8(const std::u32string &text, std::string &out, size_t &bad_offset)
{
    out.clear();
    out.reserve(text.size());
    for (size_t i = 0; i < text.size(); i++) {
        const char32_t c = text[i];
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
        } else if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            if (!is_scalar_value(c)) { bad_offset = i; return string_error; }
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            if (!is_scalar_value(c)) { bad_offset = i; return string_error; }
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return success;
}

void char32_to_bits(const std::u32string &text, std::string &bits)
{
    bits.assign(text.size() * 32, '0');
    for (size_t i = 0; i < text.size(); i++)
        for (unsigned int bit = 0; bit < 32; bit++)
            if ((text[i] >> (31 - bit)) & 1u)
                bits[i * 32 + bit] = '1';
}

signed long long int bits_to_char32(const std::string &bits, std::u32string &out, size_t &bad_offset)
{
    out.clear();
    if (bits.size() % 32 != 0) { bad_offset = bits.size() - bits.size() % 32; return string_error; }
    out.reserve(bits.size() / 32);
    for (size_t i = 0; i < bits.size(); i += 32) {
        char32_t value = 0;
        for (unsigned int bit = 0; bit < 32; bit++) {
            const char c = bits[i + bit];
            if (c != '0' && c != '1') { bad_offset = i + bit; return string_error; }
            value = (value << 1) | static_cast<char32_t>(c == '1');
        }
        if (!is_scalar_value(value)) { bad_offset = i; return string_error; }
        out.push_back(value);
    }
    return success;
}

signed long long int bits_to_cxx_str(const std::string &bits, std::string &out, size_t &bad_offset)
{
    std::u32string text;
    const signed long long int code = bits_to_char32(bits, text, bad_offset);
    if (code != success) {
        out.clear();
        return code;
    }
    return char32_to_utf8(text, out, bad_offset);           // cannot fail: bits_to_char32 checked every value
}

} // namespace satellite004
