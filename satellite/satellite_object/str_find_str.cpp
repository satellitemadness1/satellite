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

    const std::size_t wanted = needle->size();
    if (wanted > haystack->size())
        return text_not_found;

    for (std::size_t at = 0; at + wanted <= haystack->size(); ++at) {
        bool matches = true;
        for (std::size_t k = 0; matches && k < wanted; ++k)
            matches = haystack->code_at_unchecked(at + k) == needle->code_at_unchecked(k);
        if (matches) {
            out = satelliteObject::of_number(satellite_number((unsigned long long int)at));
            return success;
        }
    }
    return text_not_found;
}

} // namespace satellite004
