#pragma once
// satellite/satellite_variable_string/plain_conversions.hpp -- the plain C++ that
// string_race.cpp holds satellite_string against. Not part of the interpreter.
//
// UTF-8 <-> char16_t and char32_t written the way a C++ programmer would, with
// NO character table: a unit is the Unicode number. As char16_t a character above
// U+FFFF is a surrogate pair (UTF-16) -- the race's (a). As char32_t it is one
// unit -- the race's (c), the same width satellite's slower path writes, so the
// cost of four bytes a character can be told apart from the cost of the table.
// Strict by the same rules as satellite_string (Table 3-7; a refusal is
// string_error at the first byte of the bad sequence): a decoder that skipped the
// checks would not be doing the same work.
//
// SEVERAL WAYS OF WRITING EACH, so the race's bar is the fastest plain C++ found,
// never a slow one: push_back; a pointer into a string sized with resize (zeros
// written first) or with string_overwrite.hpp (none); ASCII taken 8 or 16 at a
// time; and for encoding, sized for the most bytes or for ASCII first. Every hot
// loop works on local copies: a write through a char pointer may alias anything,
// so a loop reading captured variables re-reads them from memory each time
// (measured: 65 ms against 44 ms decoding 100 MB, the same loop).
// plain_like_satellite.hpp adds satellite's own loops with the table taken out.

#include "string_overwrite.hpp"

#include "../machine/machine_codes.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace satellite004 {

// One multi-byte sequence at bytes[at] (bytes[at] >= 0x80): its length, or 0.
inline unsigned int plain_sequence(const unsigned char *bytes, std::size_t size, std::size_t at, char32_t &value)
{
    const unsigned char b0 = bytes[at];
    if (b0 < 0xE0) {
        if (b0 < 0xC2 || size - at < 2 || (bytes[at + 1] & 0xC0) != 0x80)
            return 0;
        value = ((b0 & 0x1Fu) << 6) | (bytes[at + 1] & 0x3Fu);
        return 2;
    }
    if (b0 < 0xF0) {
        if (size - at < 3)
            return 0;
        const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2];
        if (b1 < (b0 == 0xE0 ? 0xA0 : 0x80) || b1 > (b0 == 0xED ? 0x9F : 0xBF) || (b2 & 0xC0) != 0x80)
            return 0;
        value = ((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu);
        return 3;
    }
    if (b0 > 0xF4 || size - at < 4)
        return 0;
    const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2], b3 = bytes[at + 3];
    if (b1 < (b0 == 0xF0 ? 0x90 : 0x80) || b1 > (b0 == 0xF4 ? 0x8F : 0xBF) || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80)
        return 0;
    value = ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu);
    return 4;
}

// Writes one character as Character units; answers the pointer after them.
template <typename Character>
Character *plain_put(Character *write, char32_t value)
{
    if (sizeof(Character) == sizeof(char16_t) && value > 0xFFFF) {
        *write++ = static_cast<Character>(0xD800 + ((value - 0x10000) >> 10));
        *write++ = static_cast<Character>(0xDC00 + ((value - 0x10000) & 0x3FF));
    } else {
        *write++ = static_cast<Character>(value);
    }
    return write;
}

template <typename Character>
signed long long int plain_decode_push_back(const std::string &utf8, std::basic_string<Character> &out, std::size_t &bad_offset)
{
    const std::size_t size = utf8.size();
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(utf8.data());
    out.clear();
    out.reserve(size);
    for (std::size_t at = 0; at < size;) {
        if (bytes[at] < 0x80) {
            out.push_back(bytes[at++]);
            continue;
        }
        char32_t value = 0;
        const unsigned int length = plain_sequence(bytes, size, at, value);
        if (length == 0) { bad_offset = at; return string_error; }
        Character units[2];
        out.append(units, static_cast<std::size_t>(plain_put(units, value) - units));
        at += length;
    }
    return success;
}

// A pointer into a string sized to the most units the bytes can make (a 4-byte
// sequence makes at most two), so the loop never checks capacity. `block` (1, 8
// or 16) takes runs of ASCII that many bytes at a time; `no_zeros` sizes the
// string with string_overwrite.hpp instead of resize.
template <typename Character, unsigned int block, bool no_zeros>
signed long long int plain_decode(const std::string &utf8, std::basic_string<Character> &out, std::size_t &bad_offset)
{
    const std::size_t size = utf8.size();
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(utf8.data());
    std::size_t at = 0;
    bool bad = false;
    const auto write_all = [bytes, size, &at, &bad](Character *const first, std::size_t) {
        Character *write = first;
        std::size_t i = at;
        bool refused = false;
        while (i < size) {
            if (bytes[i] < 0x80) {
                while (block > 1 && size - i >= block) {
                    std::uint64_t word = 0, second = 0;
                    std::memcpy(&word, bytes + i, 8);
                    if (block == 16)
                        std::memcpy(&second, bytes + i + 8, 8);
                    if ((word | second) & 0x8080808080808080ull)
                        break;
                    for (unsigned int k = 0; k < block; k++)
                        write[k] = bytes[i + k];
                    write += block;
                    i += block;
                }
                while (i < size && bytes[i] < 0x80)
                    *write++ = bytes[i++];
                continue;
            }
            char32_t value = 0;
            const unsigned int length = plain_sequence(bytes, size, i, value);
            if (length == 0) { refused = true; break; }
            write = plain_put(write, value);
            i += length;
        }
        at = i;
        bad = refused;
        return static_cast<std::size_t>(write - first);
    };
    if (no_zeros) {
        overwrite_string(out, size, write_all);
    } else {
        out.resize(size);
        out.resize(write_all(out.data(), size));
    }
    if (bad) { bad_offset = at; return string_error; }
    return success;
}

// The encoders take a string made by the decoders above, so every char16_t
// surrogate is half of a pair.
template <typename Character>
std::string plain_encode_push_back(const std::basic_string<Character> &text)
{
    std::string out;
    out.reserve(text.size());
    const std::size_t count = text.size();
    for (std::size_t k = 0; k < count; k++) {
        char32_t c = text[k];
        if (c < 0x80) {
            out.push_back(static_cast<char>(c));
            continue;
        }
        if (sizeof(Character) == sizeof(char16_t) && c >= 0xD800 && c <= 0xDFFF)
            c = 0x10000 + ((c - 0xD800) << 10) + (text[++k] - 0xDC00);
        if (c < 0x800) {
            out.push_back(static_cast<char>(0xC0 | (c >> 6)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else if (c < 0x10000) {
            out.push_back(static_cast<char>(0xE0 | (c >> 12)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        } else {
            out.push_back(static_cast<char>(0xF0 | (c >> 18)));
            out.push_back(static_cast<char>(0x80 | ((c >> 12) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | ((c >> 6) & 0x3F)));
            out.push_back(static_cast<char>(0x80 | (c & 0x3F)));
        }
    }
    return out;
}

// Writes units[k, count) from `write`; with ascii_only, stops at the first unit
// that is not ASCII. One call a pass, like satellite_string's encoder. `block` and
// `ascii_only` are template arguments: as run-time arguments of a call the
// compiler did not inline, the block loops ran 104 ms against 64 ms.
template <typename Character, unsigned int block, bool ascii_only>
void plain_write_utf8(const Character *const units, const std::size_t count, std::size_t &k_inout, char *&write_inout)
{
    std::size_t k = k_inout;
    char *write = write_inout;
    while (k < count) {
        char32_t c = units[k];
        if (c < 0x80) {
            while (block > 1 && count - k >= block) {
                Character every_bit = 0;
                for (unsigned int j = 0; j < block; j++)
                    every_bit |= units[k + j];
                if (every_bit >= 0x80)
                    break;
                for (unsigned int j = 0; j < block; j++)
                    write[j] = static_cast<char>(units[k + j]);
                write += block;
                k += block;
            }
            while (k < count && units[k] < 0x80)
                *write++ = static_cast<char>(units[k++]);
            continue;
        }
        if (ascii_only)
            break;
        if (sizeof(Character) == sizeof(char16_t) && c >= 0xD800 && c <= 0xDFFF)
            c = 0x10000 + ((c - 0xD800) << 10) + (units[++k] - 0xDC00);
        if (c < 0x800) {
            write[0] = static_cast<char>(0xC0 | (c >> 6));
            write[1] = static_cast<char>(0x80 | (c & 0x3F));
            write += 2;
        } else if (c < 0x10000) {
            write[0] = static_cast<char>(0xE0 | (c >> 12));
            write[1] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            write[2] = static_cast<char>(0x80 | (c & 0x3F));
            write += 3;
        } else {
            write[0] = static_cast<char>(0xF0 | (c >> 18));
            write[1] = static_cast<char>(0x80 | ((c >> 12) & 0x3F));
            write[2] = static_cast<char>(0x80 | ((c >> 6) & 0x3F));
            write[3] = static_cast<char>(0x80 | (c & 0x3F));
            write += 4;
        }
        k++;
    }
    k_inout = k;
    write_inout = write;
}

// `sizing`: 0 resize to the most bytes (zeros written), cut to what was written;
// 1 the same with string_overwrite.hpp; 2 satellite_string's way -- one byte a
// unit first, grown at the first unit that is not ASCII to what the rest can need.
template <typename Character, unsigned int block, int sizing>
std::string plain_encode(const std::basic_string<Character> &text)
{
    const std::size_t count = text.size();
    const Character *const units = text.data();
    const std::size_t most_bytes_a_unit = sizeof(Character) == sizeof(char16_t) ? 3 : 4; // a pair: 4 bytes for 2 units
    std::size_t k = 0;
    std::string out;
    const auto write_from = [units, count, &k](char *const first, std::size_t written, bool ascii_only) {
        char *write = first + written;
        if (ascii_only)
            plain_write_utf8<Character, block, true>(units, count, k, write);
        else
            plain_write_utf8<Character, block, false>(units, count, k, write);
        return static_cast<std::size_t>(write - first);
    };
    if (sizing == 0) {
        out.resize(count * most_bytes_a_unit);
        out.resize(write_from(out.data(), 0, false));
    } else if (sizing == 1) {
        overwrite_string(out, count * most_bytes_a_unit, [&](char *first, std::size_t) { return write_from(first, 0, false); });
    } else {
        overwrite_string(out, count, [&](char *first, std::size_t) { return write_from(first, 0, true); });
        if (k < count) {
            const std::size_t written = out.size();
            overwrite_string(out, written + (count - k) * most_bytes_a_unit,
                             [&](char *first, std::size_t) { return write_from(first, written, false); });
        }
    }
    return out;
}

} // namespace satellite004
