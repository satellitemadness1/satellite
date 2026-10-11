// satellite/satellite_object/str_find_str.cpp -- `string_object.find("str")`.
//
// (the author, 2026-09-16) "str_find_str is for string_object.find(\"str\"), then
// we need to wire this one in to collect when we hit .find(\"str\")".
//
// SO THIS ONE IS REACHED BY A METHOD AND NOT BY AN OPERATOR, which is the first
// of its kind in this folder and is why it is worth saying: the left side is the
// RECEIVER -- the string the method was written on -- and the right side is its
// one argument. Everything else about the shape is str_add_str.cpp's exactly.
//
// THE ANSWER IS A POSITION, so it comes back as a satellite_number and not a
// string. Counted in CHARACTERS and not bytes, because a satellite_string holds
// codes: `"日本語".find("語")` is 2, which is the answer a person means.
//
// NOT FOUND IS text_not_found (15) AND NOT -1. The machine code already exists
// and is 003's own refusal for this (its S0716), and a language whose numbers
// have no ceiling should not borrow a sentinel from one whose numbers do. `out`
// is left untouched.
//
// AN EMPTY NEEDLE IS FOUND AT 0, which is what every library answers and what
// the loop below produces on its own.

#include "fast_paths.hpp"

namespace satellite004 {

signed long long int str_find_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_string *haystack = left.as_string();
    const satellite_string *needle = right.as_string();
    if (haystack == nullptr || needle == nullptr)
        return types_do_not_meet;

    if (needle->size() > haystack->size())
        return text_not_found;

    // CHARACTER BY CHARACTER, comparing UNITS from the start of each one. A needle is
    // whole characters, so its units match the haystack's from a character's start
    // exactly when its characters do -- a wide character's 40000 and both halves
    // included. Starting anywhere else could meet a low half that is itself 40000
    // (satellite_string.hpp), which is why the walk never does. The answer is the
    // CHARACTER position, as it always was.
    const std::size_t wanted = needle->units();
    const std::size_t units = haystack->units();
    for (std::size_t unit = 0, character = 0; unit + wanted <= units; ++character) {
        if (std::char_traits<char16_t>::compare(haystack->unit_data() + unit, needle->unit_data(), wanted) == 0) {
            out = satelliteObject::of_number(satellite_number((unsigned long long int)character));
            return success;
        }
        std::size_t width = 0;
        haystack->code_at_unit(unit, width);
        unit += width;
    }
    return text_not_found;
}

} // namespace satellite004
