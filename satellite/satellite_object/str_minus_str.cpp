// satellite/satellite_object/str_minus_str.cpp -- str_add_str.cpp's shape, with
// the operation changed. The author asked for it by name.
//
// WHAT `-` MEANS ON TWO STRINGS IS A DECISION AND IT IS MINE UNTIL THE AUTHOR
// TAKES IT. Nothing in DESIGN gives the minus sign a meaning over strings, so
// this file chose the one that makes `-` the inverse of `+`: **every occurrence
// of the right string is removed from the left**, so
//
//     ("a" + "b") - "b"   is "a"
//     "banana" - "an"     is "ba"      (both occurrences go)
//
// The alternative worth naming is "remove the FIRST occurrence only", which
// makes `-` the exact inverse of one `+` and leaves `"banana" - "an"` as "bana".
// Changing the ruling is this one loop and nothing else in the interpreter.
//
// A RIGHT STRING THAT IS EMPTY REMOVES NOTHING rather than looping forever,
// which is the same refusal 003 makes for an empty needle (empty_search_text,
// 18) -- but here it is an answer and not a refusal, because subtracting nothing
// from something is that something.

#include "fast_paths.hpp"

namespace satellite004 {

signed long long int str_minus_str(const satelliteObject &left, const satelliteObject &right, satelliteObject &out)
{
    const satellite_string *l = left.as_string();
    const satellite_string *r = right.as_string();
    if (l == nullptr || r == nullptr)
        return types_do_not_meet;

    const std::size_t taken = r->size();
    satellite_string answer;
    if (taken == 0) {
        out = satelliteObject::of_string(*l);
        return success;
    }

    for (std::size_t at = 0; at < l->size(); ) {
        bool matches = at + taken <= l->size();
        for (std::size_t k = 0; matches && k < taken; ++k)
            matches = l->code_at_unchecked(at + k) == r->code_at_unchecked(k);
        if (matches) { at += taken; continue; }      // skip it: this is the removal
        const signed long long int held = answer.append_code(l->code_at_unchecked(at));
        if (held != success)
            return held;
        ++at;
    }
    out = satelliteObject::of_string(std::move(answer));
    return success;
}

} // namespace satellite004
