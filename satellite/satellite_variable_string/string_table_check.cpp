// satellite/satellite_variable_string/string_table_check.cpp -- proves the author's
// character table in satellite_string.cpp. Not part of the interpreter.
//
//     build/string_table_check                  the checks below
//     build/string_table_check every-char32     and check 3 over all 4,294,967,296 char32_t values
//
// 1. Codes 0-127 are every ASCII character exactly once, in exactly the order
//    satellite_string.hpp writes. The order is spelled out here a second time,
//    from the header's own words, independent of the arrays in satellite_string.cpp.
// 2. code_of and unicode_of are exact inverses over every Unicode scalar value
//    (0..0x10FFFF except the surrogates D800-DFFF, 1,112,064 values): each maps
//    that set into itself, and each undoes the other, so both are one-to-one onto it.
// 3. Above 127 both are the identity -- also for the surrogates and above
//    0x10FFFF, which is why append_code can refuse a non-character by its code.
// 4. A code is above 0xFFFF exactly when its character is, so "fits 16 bits" is
//    the same question asked of either.
//
// check_strings16.py runs this, and also reads the table from the header's text.

#include "satellite_string.hpp"

#include <cstdio>
#include <string>

using namespace satellite004;

namespace {

int failures = 0;

void report(bool passed, const char *what)
{
    std::printf("%s %s\n", passed ? "ok  " : "FAIL", what);
    failures += passed ? 0 : 1;
}

bool is_scalar_value(char32_t value)
{
    return value <= 0x10FFFF && (value < 0xD800 || value > 0xDFFF);
}

// The header's table, code by code: 0 NUL; 1-26 a-z; 27-52 A-Z; 53-62 0-9;
// 63-72 ! @ # $ % ^ & * ( ); 73-94 - + = _ [ ] { } \ | ; : ' " < > ? , . / ` ~;
// 95 96 97 space, tab, newline; 98-127 the other 30 controls in ASCII order.
std::u32string header_order()
{
    std::u32string order;
    order += U'\0';
    for (char32_t c = U'a'; c <= U'z'; c++)
        order += c;
    for (char32_t c = U'A'; c <= U'Z'; c++)
        order += c;
    for (char32_t c = U'0'; c <= U'9'; c++)
        order += c;
    order += U"!@#$%^&*()";
    order += U"-+=_[]{}\\|;:'\"<>?,./`~";
    order += U" \t\n";
    for (char32_t c = 0x01; c <= 0x7F; c++)
        if ((c < 0x20 || c == 0x7F) && c != U'\t' && c != U'\n')
            order += c;
    return order;
}

} // namespace

int main(int argc, char **argv)
{
    const std::u32string order = header_order();

    // 1. The permutation, and the header's range boundaries.
    bool each_once = order.size() == 128;
    int seen[128] = {};
    for (char32_t c : order)
        each_once = each_once && c < 128 && ++seen[c] == 1;
    report(each_once, "the header's order names 128 codes, every ASCII character exactly once");
    report(order[1] == U'a' && order[26] == U'z' && order[27] == U'A' && order[52] == U'Z' && order[53] == U'0' &&
               order[62] == U'9' && order[63] == U'!' && order[72] == U')' && order[73] == U'-' && order[94] == U'~' &&
               order[95] == U' ' && order[96] == U'\t' && order[97] == U'\n' && order[98] == 0x01 &&
               order[105] == 0x08 && order[106] == 0x0B && order[126] == 0x1F && order[127] == 0x7F,
           "its boundaries are the header's: 1 a, 26 z, 27 A, 52 Z, 53 0, 62 9, 63 !, 72 ), 73 -, 94 ~, 95-97 space tab newline, 98 0x01 .. 127 0x7F");
    bool table_is_header = true;
    for (char32_t code = 0; code < 128; code++)
        table_is_header = table_is_header && satellite_string::unicode_of(code) == order[code] &&
                          satellite_string::code_of(order[code]) == code;
    report(table_is_header, "unicode_of(code) and code_of(character) give exactly that order for codes 0-127");

    // 2 and 4. Every Unicode scalar value.
    unsigned long long int scalar_values = 0, into_itself = 0, inverse = 0, same_width = 0;
    for (char32_t value = 0; value <= 0x10FFFF; value++) {
        if (!is_scalar_value(value))
            continue;
        scalar_values++;
        const char32_t code = satellite_string::code_of(value), unicode = satellite_string::unicode_of(value);
        into_itself += is_scalar_value(code) && is_scalar_value(unicode);
        inverse += satellite_string::unicode_of(code) == value && satellite_string::code_of(unicode) == value;
        same_width += (code > 0xFFFF) == (value > 0xFFFF);
    }
    std::printf("      %llu Unicode scalar values\n", scalar_values);
    report(scalar_values == 1112064 && into_itself == scalar_values,
           "code_of and unicode_of map the 1,112,064 scalar values into themselves");
    report(inverse == scalar_values, "unicode_of(code_of(u)) == u and code_of(unicode_of(u)) == u for every one");
    report(same_width == scalar_values, "a code is above 0xFFFF exactly when its character is");

    // 3. The identity above 127, over the surrogates and past 0x10FFFF.
    bool identity = true;
    for (char32_t value = 128; value <= 0x10FFFF + 0x10000; value++)
        identity = identity && satellite_string::code_of(value) == value && satellite_string::unicode_of(value) == value;
    for (char32_t value : {char32_t(0x7FFFFFFF), char32_t(0x80000000), char32_t(0xFFFFFFFE), char32_t(0xFFFFFFFF)})
        identity = identity && satellite_string::code_of(value) == value && satellite_string::unicode_of(value) == value;
    report(identity, "above 127 both are the identity, surrogates and values past 0x10FFFF included");

    if (argc == 2 && std::string(argv[1]) == "every-char32") {
        bool every = true;
        char32_t value = 0;
        do {
            every = every && satellite_string::unicode_of(satellite_string::code_of(value)) == value &&
                    satellite_string::code_of(satellite_string::unicode_of(value)) == value;
        } while (++value != 0);
        report(every, "inverses over all 4,294,967,296 char32_t values");
    }
    std::printf("%s\n", failures == 0 ? "the table is exact" : "the table is WRONG");
    return failures == 0 ? 0 : 1;
}
