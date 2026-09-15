#pragma once
// What one numbered library tells the interpreter about itself.
//
// Every library under satellite-numbers/ is compiled with no main and exports
// ONE function, `satellite_number_describe`, which fills in a LibraryRow: the
// path's name, its numbers from WORD_NUMBERS.md, and a pointer to each likely
// scenario it has. The interpreter calls it once, at start-up, and never
// looks anything up by name again while a program runs.
//
// A scenario is chosen by the kind of value the call is given. nullptr means
// the number has no scenario for that kind (yet).

#include "../strings/string_method.hpp"

#include <string>

namespace satellite004 {

inline constexpr unsigned int kMaxDepth = 16;

struct Scenarios {
    // The most likely scenario: display a string.
    signed long long int (*text)(const std::string &text, bool endline) = nullptr;
    signed long long int (*count)(unsigned long long int value, bool endline) = nullptr;
    signed long long int (*flag)(bool value, bool endline) = nullptr;
    signed long long int (*size)(long double value, const std::string &unit, bool endline) = nullptr;
    // A method of satellite.variable.string (1 6 1 n): see strings/string_method.hpp.
    StringMethod string_method = nullptr;
};

struct LibraryRow {
    const char *name = nullptr;                    // "satellite.console.display"
    unsigned long long int numbers[kMaxDepth] = {};
    unsigned int depth = 0;                         // 3 for `1 5 1`
    Scenarios scenarios;
};

// The one symbol each library exports. extern "C" so its name is not mangled
// and dlsym can find it by exactly this spelling.
inline constexpr const char *kDescribeSymbol = "satellite_number_describe";
using DescribeFunction = signed long long int (*)(LibraryRow *row);

} // namespace satellite004
