#pragma once
// satellite/satellite_variable_string/plain_like_satellite.hpp -- satellite_string's
// own conversion loops (conversion_loops.hpp) with the author's table taken out,
// for string_race.cpp. Not part of the interpreter.
//
// WHY: the race's bar must never be weaker than the very code satellite runs, less
// the one thing plain C++ does not do. Line for line the loops are the same --
// blocks of ASCII written out, one branch a sequence length, one call a pass --
// except that an ASCII byte is its own unit, and as char16_t a character above
// U+FFFF becomes a surrogate pair. They matter most under g++, which compiled
// plain_conversions.hpp's loops less well (91.8 ms against clang's 55.9 ms on the
// race's mixed text).

#include "plain_conversions.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace satellite004 {

template <typename Character>
signed long long int plain_decode_like_satellite(const std::string &utf8, std::basic_string<Character> &out, std::size_t &bad_offset)
{
    const std::size_t size = utf8.size();
    const unsigned char *const bytes = reinterpret_cast<const unsigned char *>(utf8.data());
    std::size_t stopped_at = 0;
    bool refused = false;
    overwrite_string(out, size, [bytes, size, &stopped_at, &refused](Character *const first, std::size_t) {
        Character *write = first;
        std::size_t at = 0;
        bool bad = false;
        while (at < size) {
            unsigned char b0 = bytes[at];
            if (b0 < 0x80) {
                while (size - at >= 8) {
                    std::uint64_t eight;
                    std::memcpy(&eight, bytes + at, 8);
                    if (eight & 0x8080808080808080ull)
                        break;
                    write[0] = bytes[at + 0];
                    write[1] = bytes[at + 1];
                    write[2] = bytes[at + 2];
                    write[3] = bytes[at + 3];
                    write[4] = bytes[at + 4];
                    write[5] = bytes[at + 5];
                    write[6] = bytes[at + 6];
                    write[7] = bytes[at + 7];
                    write += 8;
                    at += 8;
                }
                while (at < size && (b0 = bytes[at]) < 0x80) {
                    *write++ = b0;
                    ++at;
                }
                continue;
            }
            if (b0 < 0xE0) {
                if (b0 < 0xC2 || size - at < 2) { bad = true; break; }
                const unsigned char b1 = bytes[at + 1];
                if ((b1 & 0xC0) != 0x80) { bad = true; break; }
                *write++ = static_cast<Character>(((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu));
                at += 2;
            } else if (b0 < 0xF0) {
                if (size - at < 3) { bad = true; break; }
                const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2];
                const unsigned char low = b0 == 0xE0 ? 0xA0 : 0x80, high = b0 == 0xED ? 0x9F : 0xBF;
                if (b1 < low || b1 > high || (b2 & 0xC0) != 0x80) { bad = true; break; }
                *write++ = static_cast<Character>(((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu));
                at += 3;
            } else {
                if (b0 > 0xF4 || size - at < 4) { bad = true; break; }
                const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2], b3 = bytes[at + 3];
                const unsigned char low = b0 == 0xF0 ? 0x90 : 0x80, high = b0 == 0xF4 ? 0x8F : 0xBF;
                if (b1 < low || b1 > high || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) { bad = true; break; }
                write = plain_put(write, ((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu));
                at += 4;
            }
        }
        stopped_at = at;
        refused = bad;
        return static_cast<std::size_t>(write - first);
    });
    if (refused) { bad_offset = stopped_at; return string_error; }
    return success;
}

template <typename Character, bool ascii_only>
void plain_write_utf8_like_satellite(const Character *const units, const std::size_t count, std::size_t &k_inout, char *&write_inout)
{
    std::size_t k = k_inout;
    char *write = write_inout;
    while (k < count) {
        char32_t c = units[k];
        if (c < 0x80) {
            while (count - k >= 16) {
                const Character every_bit = units[k + 0] | units[k + 1] | units[k + 2] | units[k + 3] | units[k + 4] | units[k + 5] | units[k + 6] | units[k + 7] | units[k + 8] | units[k + 9] | units[k + 10] | units[k + 11] | units[k + 12] | units[k + 13] | units[k + 14] | units[k + 15];
                if (every_bit >= 0x80)
                    break;
                write[0] = static_cast<char>(units[k + 0]);
                write[1] = static_cast<char>(units[k + 1]);
                write[2] = static_cast<char>(units[k + 2]);
                write[3] = static_cast<char>(units[k + 3]);
                write[4] = static_cast<char>(units[k + 4]);
                write[5] = static_cast<char>(units[k + 5]);
                write[6] = static_cast<char>(units[k + 6]);
                write[7] = static_cast<char>(units[k + 7]);
                write[8] = static_cast<char>(units[k + 8]);
                write[9] = static_cast<char>(units[k + 9]);
                write[10] = static_cast<char>(units[k + 10]);
                write[11] = static_cast<char>(units[k + 11]);
                write[12] = static_cast<char>(units[k + 12]);
                write[13] = static_cast<char>(units[k + 13]);
                write[14] = static_cast<char>(units[k + 14]);
                write[15] = static_cast<char>(units[k + 15]);
                write += 16;
                k += 16;
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

template <typename Character>
std::string plain_encode_like_satellite(const std::basic_string<Character> &text)
{
    const std::size_t count = text.size();
    const Character *const units = text.data();
    std::string out;
    std::size_t k = 0;
    overwrite_string(out, count, [&](char *const first, std::size_t) {
        char *write = first;
        plain_write_utf8_like_satellite<Character, true>(units, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
    if (k == count)
        return out;
    const std::size_t written = out.size();
    const std::size_t most_bytes_a_unit = sizeof(Character) == sizeof(char16_t) ? 3 : 4;
    overwrite_string(out, written + (count - k) * most_bytes_a_unit, [&](char *const first, std::size_t) {
        char *write = first + written;
        plain_write_utf8_like_satellite<Character, false>(units, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
    return out;
}

} // namespace satellite004
