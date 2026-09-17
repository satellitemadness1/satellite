#pragma once
// satellite-004/strings/string_method.hpp -- the satellite_string32 type, and the
// one shape every string-method library is called with.
//
// RENAMED FROM `satellite_string` TO `satellite_string32` ON 2026-09-16, and the
// rename IS the point rather than a tidy-up. There are two string types in this
// tree and PROGRESS §5 has said so since 2026-09-15: this one -- 32 bits a
// character, one char32_t per code point -- and the language's own
// satellite/satellite_variable_string/satellite_string.hpp, which is 16 bits a
// character with the author's table and goes to 32 only for a character above
// U+FFFF. They had the SAME NAME in the SAME NAMESPACE, and nothing had ever
// included both, so nothing had noticed.
//
// THE OBJECT MODEL INCLUDED BOTH, and that is what made it a compile error
// instead of a trap: satelliteValue's string arm is the new type, and the
// interpreter also reaches number_row.hpp for a library's scenarios, which is
// where this header arrives. One of the two had to be named differently, and it
// is this one, because this one is the one that is going away.
//
// WHAT IS STILL OWED, unchanged by the rename (PROGRESS §5): the 23 string-method
// libraries run on THIS type, and they move onto the language's own string --
// checked against 003's satl, as they were the first time. The rename does not
// do that work; it makes the two tellable apart while it is done, and it makes
// every remaining use of the old one say `32` out loud.
//
// A SATELLITE STRING IS 32 BITS A CHARACTER (DESIGN §14, the author 2026-09-14):
// one char32_t per Unicode code point. `size` counts characters, `at(n)` is one
// index, and every language's characters are the same width.
//
// EACH METHOD IS ITS OWN LIBRARY (the author: "supply those methods for our
// satellite_string32 as libraries, each method will be a library"), in
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

struct satellite_string32 {
    std::u32string text;
};

struct StringArguments {
    std::vector<satellite_string32> strings;      // text arguments, in written order
    std::vector<signed long long int> positions; // position arguments, in written order
};

struct StringAnswer {
    enum class Kind { nothing, string, count, flag, strings } kind = Kind::nothing;
    satellite_string32 text;                      // Kind::string
    unsigned long long int count = 0;           // Kind::count
    bool flag = false;                          // Kind::flag
    std::vector<satellite_string32> list;         // Kind::strings (split)
};

using StringMethod = signed long long int (*)(satellite_string32 &self, const StringArguments &arguments,
                                              StringAnswer &answer);

} // namespace satellite004
