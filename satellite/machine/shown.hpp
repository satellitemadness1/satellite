#pragma once
// shown() -- a word from outside satl, made safe to put on a terminal inside a
// message satl writes (PLAN M0.5, DESIGN §9).
//
// A file name or a program word is the user's, and it arrives as BYTES: it can
// hold ESC ] 2 ; title BEL, which retitles the terminal the moment it is printed,
// or a CSI that moves the cursor, or U+202E, which turns the rest of the line
// around. So everything satl says ABOUT such a word goes through here, and the
// terminal only ever receives text:
//
//     a byte below 0x20, or 0x7F          \x1b   (ESC, BEL, NUL, tab, newline...)
//     a byte that is not UTF-8            \xff   (each byte of a broken sequence)
//     a C1 control, U+0080 to U+009F      \u{009b}   (0x9B is a CSI to some terminals)
//     a direction or isolate control      \u{202e}   (U+061C, U+200E-F, U+202A-E, U+2066-9)
//
// Every other character is left as it is, look-alikes included: `рrint` with a
// Cyrillic р is a real name and shows as one.
//
// WHERE IT IS USED: report_error() and display_machine_state() put every message
// through it (machine_state.cpp), because no message satl writes contains a
// control byte of its own. A PROGRAM's output is never put through it --
// satellite.console.display writes what the program asked for, byte for byte.

#include <string>
#include <string_view>

namespace satellite004 {

inline std::string shown(std::string_view text)
{
    static const char digits[] = "0123456789abcdef";
    std::string out;
    out.reserve(text.size());
    const auto byte_escape = [&](unsigned char c) {
        out += "\\x";
        out += digits[c >> 4];
        out += digits[c & 15];
    };

    for (std::size_t i = 0; i < text.size();) {
        const unsigned char c = static_cast<unsigned char>(text[i]);
        if (c < 0x20 || c == 0x7F) {
            byte_escape(c);
            ++i;
            continue;
        }
        if (c < 0x80) {
            out += static_cast<char>(c);
            ++i;
            continue;
        }

        // A multi-byte character: its length from the lead byte, then every
        // continuation byte checked, then overlong forms, surrogates and anything
        // past U+10FFFF refused -- the same strictness satellite_string decodes with.
        const std::size_t length = c >= 0xC2 && c <= 0xDF ? 2 : c >= 0xE0 && c <= 0xEF ? 3 : c >= 0xF0 && c <= 0xF4 ? 4 : 0;
        char32_t code = length == 2 ? c & 0x1F : length == 3 ? c & 0x0F : c & 0x07;
        bool valid = length != 0 && i + length <= text.size();
        for (std::size_t k = 1; valid && k < length; ++k) {
            const unsigned char next = static_cast<unsigned char>(text[i + k]);
            valid = (next & 0xC0) == 0x80;
            code = (code << 6) | (next & 0x3F);
        }
        if (valid)
            valid = !(length == 3 && code < 0x800) && !(length == 4 && code < 0x10000) &&
                    !(code >= 0xD800 && code <= 0xDFFF) && code <= 0x10FFFF;
        if (!valid) {
            byte_escape(c);
            ++i;
            continue;
        }

        const bool control = (code >= 0x80 && code <= 0x9F) || code == 0x061C || code == 0x200E || code == 0x200F ||
                             (code >= 0x202A && code <= 0x202E) || (code >= 0x2066 && code <= 0x2069);
        if (control) {
            out += "\\u{";
            for (int shift = 12; shift >= 0; shift -= 4)
                out += digits[(code >> shift) & 15];
            out += '}';
        } else {
            out.append(text.substr(i, length));
        }
        i += length;
    }
    return out;
}

} // namespace satellite004
