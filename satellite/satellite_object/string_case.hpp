#pragma once
// satellite/satellite_object/string_case.hpp -- ONE FUNCTION, ONE FILE: a string in capital
// letters, or in small ones.
//
// THE AUTHOR, 2026-09-25: "we need str.upper() and str.uppercase() and str.up() a
// str.lower() and str.lowercase()". The five spellings are two method codes
// (REGISTRY.satellite's upper_token and lower_token); this is what both answer.
//
// EVERY LANGUAGE'S LETTERS, NOT ONLY a-z. 003 changed a..z and nothing else (its
// string_methods.cpp, and satellite-numbers/satellite.variable.string.upper ported it as
// that), so "héllo" became "HéLLO". Here each character goes through the C library's own
// Unicode case table -- towupper_l / towlower_l on the C.UTF-8 locale glibc carries built
// in -- so é is É, ж is Ж and σ is Σ. A mapping that would change the LENGTH (ß to SS) is
// not one these functions make, and ß is left as it is. A machine with no C.UTF-8 locale
// gets 003's a-z, rather than nothing.
//
// A NEW STRING IS ANSWERED; the one it was asked of is unchanged, as every string method.

#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

#include <locale.h>
#include <string>
#include <wctype.h>

namespace satellite004 {

inline locale_t unicode_case_table()
{
    static const locale_t made = newlocale(LC_CTYPE_MASK, "C.UTF-8", static_cast<locale_t>(0));
    return made;
}

inline signed long long int string_case(const satellite_string &from, bool upper, satellite_string &out)
{
    const std::string text = from.to_utf8();
    std::string changed;
    changed.reserve(text.size());
    const locale_t table = unicode_case_table();
    for (std::size_t i = 0; i < text.size();) {
        const unsigned char first = static_cast<unsigned char>(text[i]);
        std::size_t length = first < 0x80 ? 1 : first < 0xE0 ? 2 : first < 0xF0 ? 3 : 4;
        if (i + length > text.size()) length = 1;
        unsigned long point = length == 1 ? first : (first & (0xFFu >> (length + 1)));
        for (std::size_t k = 1; k < length; ++k)
            point = (point << 6) | (static_cast<unsigned char>(text[i + k]) & 0x3Fu);
        unsigned long mapped = point;
        if (table != static_cast<locale_t>(0))
            mapped = static_cast<unsigned long>(upper ? towupper_l(static_cast<wint_t>(point), table)
                                                      : towlower_l(static_cast<wint_t>(point), table));
        else if (upper && point >= 'a' && point <= 'z')
            mapped = point - 'a' + 'A';
        else if (!upper && point >= 'A' && point <= 'Z')
            mapped = point - 'A' + 'a';
        if (mapped < 0x80) {
            changed += static_cast<char>(mapped);
        } else if (mapped < 0x800) {
            changed += static_cast<char>(0xC0 | (mapped >> 6));
            changed += static_cast<char>(0x80 | (mapped & 0x3F));
        } else if (mapped < 0x10000) {
            changed += static_cast<char>(0xE0 | (mapped >> 12));
            changed += static_cast<char>(0x80 | ((mapped >> 6) & 0x3F));
            changed += static_cast<char>(0x80 | (mapped & 0x3F));
        } else {
            changed += static_cast<char>(0xF0 | (mapped >> 18));
            changed += static_cast<char>(0x80 | ((mapped >> 12) & 0x3F));
            changed += static_cast<char>(0x80 | ((mapped >> 6) & 0x3F));
            changed += static_cast<char>(0x80 | (mapped & 0x3F));
        }
        i += length;
    }
    std::size_t bad_offset = 0;
    return satellite_string::from_utf8(changed, out, bad_offset);
}

} // namespace satellite004
