#pragma once
// satellite-004/strings/string_method.hpp -- the satellite_string type, and the
// one shape every string-method library is called with.
//
// A SATELLITE STRING IS 32 BITS A CHARACTER (DESIGN §14, the author 2026-09-14):
// one char32_t per Unicode code point. `size` counts characters, `at(n)` is one
// index, and every language's characters are the same width.
//
// EACH METHOD IS ITS OWN LIBRARY (the author: "supply those methods for our
// satellite_string as libraries, each method will be a library"), in
// satellite-numbers/<word>/<word>.satellite.cpp, built as <its numbers>.so by
// satellite-numbers/build_libraries.py from words/words.tsv.
//
// BEHAVIOUR IS 003 06's (src/satellite_scalars/string_methods.cpp), with two
// differences that come from the character width, not from a new decision:
//   - lower / upper change a..z and A..Z only, as 003 did. Case for every
//     alphabet is a separate decision (it needs Unicode's case tables).
//   - 003's six live escapes (\threads, \home ...) do not exist in 004 yet, so
//     `resolved` answers the string unchanged.
//
// POSITIONS ARE `signed long long int` UNTIL satellite_number EXISTS (PLAN M4).
// A negative one is refused (not_a_position); a string longer than that could
// not fit in memory, so no real position is lost.
//
// THE CALL: a method gets the receiver (non-const, because append and clear
// change it), its arguments, and an answer to fill. It returns a machine code:
// success, or the refusal 003 would have made.

#include <string>
#include <vector>

namespace satellite004 {

struct satellite_string {
    std::u32string text;
};

struct StringArguments {
    std::vector<satellite_string> strings;      // text arguments, in written order
    std::vector<signed long long int> positions; // position arguments, in written order
};

struct StringAnswer {
    enum class Kind { nothing, string, count, flag, strings } kind = Kind::nothing;
    satellite_string text;                      // Kind::string
    unsigned long long int count = 0;           // Kind::count
    bool flag = false;                          // Kind::flag
    std::vector<satellite_string> list;         // Kind::strings (split)
};

using StringMethod = signed long long int (*)(satellite_string &self, const StringArguments &arguments,
                                              StringAnswer &answer);

} // namespace satellite004
