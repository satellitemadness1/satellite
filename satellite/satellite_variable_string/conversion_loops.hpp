#pragma once
// satellite/satellite_variable_string/conversion_loops.hpp -- the loops that turn
// UTF-8 bytes into codes and codes into UTF-8 bytes. Used by satellite_string.cpp
// only; written once for char16_t and char32_t alike, as templates.
//
// SPEED (string_race.cpp measures it against plain C++ with no table):
//   - a string is sized once and written through a pointer, without zeros first
//     (string_overwrite.hpp): decoding to the most characters the bytes can make,
//     encoding to one byte a code and grown once at the first code that is not ASCII;
//   - runs of ASCII: decoding takes 8 bytes at a time (one test for a top bit, then
//     eight table loads; 44 ms against 48 ms for 16), encoding 16 codes at a time
//     (48 ms against 60 ms for 8, the same as a copy with no table);
//   - each loop is one call a pass and works on local copies (a char write could
//     alias anything it reads through a reference).
//
// STRICT: decode follows the Unicode standard's Table 3-7 exactly as
// strings/satellite_string.cpp does -- the second byte's range depends on the
// first, which rejects overlong forms, surrogates and anything above U+10FFFF:
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

#include "character_table.hpp"
#include "string_overwrite.hpp"

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>

namespace satellite004 {
namespace conversion_loops {

using character_table::ascii_of_code;
using character_table::code_of_ascii;

enum class stopped { at_the_end, at_a_bad_sequence, at_a_wide_character };

// Decodes bytes[at, size) into `write`. With stop_before_wide (the char16_t
// pass) it stops at a well-formed 4-byte sequence and leaves `at` on its first
// byte; otherwise it decodes it. On a bad sequence `at` is that sequence's first
// byte. `write` is left one past the last code written.
template <typename Code, bool stop_before_wide>
stopped decode(const unsigned char *const bytes, const std::size_t size, std::size_t &at_inout, Code *&write_inout)
{
    std::size_t at = at_inout;
    Code *write = write_inout;
    stopped why = stopped::at_the_end;
    while (at < size) {
        unsigned char b0 = bytes[at];
        if (b0 < 0x80) {
            // The most likely case: ASCII. Eight at a time while all eight are
            // (eight, not sixteen: 44.3 ms against 48.2 ms decoding 100 MB).
            while (size - at >= 8) {
                std::uint64_t eight;
                std::memcpy(&eight, bytes + at, 8);
                if (eight & 0x8080808080808080ull)
                    break;
                // Written out, not a loop: g++ kept an 8-step loop (a counter and a
                // branch a byte, 80.8 ms for 100 MB) where clang made eight
                // independent loads; written out, both make them (52.6 and 51.3 ms).
                write[0] = code_of_ascii[bytes[at]];
                write[1] = code_of_ascii[bytes[at + 1]];
                write[2] = code_of_ascii[bytes[at + 2]];
                write[3] = code_of_ascii[bytes[at + 3]];
                write[4] = code_of_ascii[bytes[at + 4]];
                write[5] = code_of_ascii[bytes[at + 5]];
                write[6] = code_of_ascii[bytes[at + 6]];
                write[7] = code_of_ascii[bytes[at + 7]];
                write += 8;
                at += 8;
            }
            while (at < size && (b0 = bytes[at]) < 0x80) {
                *write++ = code_of_ascii[b0];
                ++at;
            }
            continue;
        }
        if (b0 < 0xE0) {                                    // C2..DF 80..BF
            if (b0 < 0xC2 || size - at < 2) { why = stopped::at_a_bad_sequence; break; }
            const unsigned char b1 = bytes[at + 1];
            if ((b1 & 0xC0) != 0x80) { why = stopped::at_a_bad_sequence; break; }
            *write++ = static_cast<Code>(((b0 & 0x1Fu) << 6) | (b1 & 0x3Fu));
            at += 2;
        } else if (b0 < 0xF0) {                             // E0 A0..BF, E1..EC 80..BF, ED 80..9F, EE..EF 80..BF; then 80..BF
            if (size - at < 3) { why = stopped::at_a_bad_sequence; break; }
            const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2];
            const unsigned char low = b0 == 0xE0 ? 0xA0 : 0x80, high = b0 == 0xED ? 0x9F : 0xBF;
            if (b1 < low || b1 > high || (b2 & 0xC0) != 0x80) { why = stopped::at_a_bad_sequence; break; }
            *write++ = static_cast<Code>(((b0 & 0x0Fu) << 12) | ((b1 & 0x3Fu) << 6) | (b2 & 0x3Fu));
            at += 3;
        } else {                                            // F0 90..BF, F1..F3 80..BF, F4 80..8F; then 80..BF 80..BF
            if (b0 > 0xF4 || size - at < 4) { why = stopped::at_a_bad_sequence; break; }
            const unsigned char b1 = bytes[at + 1], b2 = bytes[at + 2], b3 = bytes[at + 3];
            const unsigned char low = b0 == 0xF0 ? 0x90 : 0x80, high = b0 == 0xF4 ? 0x8F : 0xBF;
            if (b1 < low || b1 > high || (b2 & 0xC0) != 0x80 || (b3 & 0xC0) != 0x80) {
                why = stopped::at_a_bad_sequence;
                break;
            }
            if (stop_before_wide) { why = stopped::at_a_wide_character; break; }
            *write++ = static_cast<Code>(((b0 & 0x07u) << 18) | ((b1 & 0x3Fu) << 12) | ((b2 & 0x3Fu) << 6) | (b3 & 0x3Fu));
            at += 4;
        }
    }
    at_inout = at;
    write_inout = write;
    return why;
}

// Writes codes[k, count) as UTF-8 from `write`; with ascii_only it stops at the
// first code that is not ASCII. Leaves `k` and `write` after the last code written.
// ASCII goes sixteen codes at a time while all sixteen are -- 48.3 ms against
// 60.0 ms for eight, and the same as a copy with no table at all (47.7 ms).
// One call a pass, never one a run: a call per run of ASCII measured 9% slower.
template <typename Code, bool ascii_only>
void write_utf8(const Code *const codes, const std::size_t count, std::size_t &k_inout, char *&write_inout)
{
    std::size_t k = k_inout;
    char *write = write_inout;
    while (k < count) {
        const char32_t c = codes[k];
        if (c < 0x80) {                                     // a code below 128 is an ASCII character, in the author's order
            while (count - k >= 16) {
                const Code *const sixteen = codes + k;
                const Code every_bit = sixteen[0] | sixteen[1] | sixteen[2] | sixteen[3] | sixteen[4] | sixteen[5] | sixteen[6] | sixteen[7] | sixteen[8] | sixteen[9] |
                                       sixteen[10] | sixteen[11] | sixteen[12] | sixteen[13] | sixteen[14] | sixteen[15];
                if (every_bit >= 0x80)
                    break;
                // Written out, not a loop, for the reason decode gives.
                write[0] = static_cast<char>(ascii_of_code[sixteen[0]]);
                write[1] = static_cast<char>(ascii_of_code[sixteen[1]]);
                write[2] = static_cast<char>(ascii_of_code[sixteen[2]]);
                write[3] = static_cast<char>(ascii_of_code[sixteen[3]]);
                write[4] = static_cast<char>(ascii_of_code[sixteen[4]]);
                write[5] = static_cast<char>(ascii_of_code[sixteen[5]]);
                write[6] = static_cast<char>(ascii_of_code[sixteen[6]]);
                write[7] = static_cast<char>(ascii_of_code[sixteen[7]]);
                write[8] = static_cast<char>(ascii_of_code[sixteen[8]]);
                write[9] = static_cast<char>(ascii_of_code[sixteen[9]]);
                write[10] = static_cast<char>(ascii_of_code[sixteen[10]]);
                write[11] = static_cast<char>(ascii_of_code[sixteen[11]]);
                write[12] = static_cast<char>(ascii_of_code[sixteen[12]]);
                write[13] = static_cast<char>(ascii_of_code[sixteen[13]]);
                write[14] = static_cast<char>(ascii_of_code[sixteen[14]]);
                write[15] = static_cast<char>(ascii_of_code[sixteen[15]]);
                write += 16;
                k += 16;
            }
            while (k < count && codes[k] < 0x80)
                *write++ = static_cast<char>(ascii_of_code[codes[k++]]);
            continue;
        }
        if (ascii_only)
            break;
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

// Codes -> UTF-8 in one pass. Sized first for ASCII alone, one byte a code, so
// the most likely text is allocated at its exact size. The first code that is not
// ASCII grows the string once, to the most the rest can need, and writing carries on.
template <typename Code>
std::string encode(const Code *const codes, const std::size_t count)
{
    std::string out;
    std::size_t k = 0;
    overwrite_string(out, count, [&](char *const first, std::size_t) {
        char *write = first;
        write_utf8<Code, true>(codes, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
    if (k == count)
        return out;
    const std::size_t written = out.size();
    const std::size_t most_bytes_a_code = sizeof(Code) == sizeof(char16_t) ? 3 : 4;
    overwrite_string(out, written + (count - k) * most_bytes_a_code, [&](char *const first, std::size_t) {
        char *write = first + written;
        write_utf8<Code, false>(codes, count, k, write);
        return static_cast<std::size_t>(write - first);
    });
    return out;
}

} // namespace conversion_loops
} // namespace satellite004
