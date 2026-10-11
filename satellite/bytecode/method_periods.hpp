#pragma once
// satellite/bytecode/method_periods.hpp -- WHICH PERIOD A METHOD IS WRITTEN WITH: `.` or `.satellite.`
// (NEW_MILESTONES.md NM-8).
//
// The author, 2026-10-10: "we need to place everything that isn't under the name "satellite", like
// everything built in, under the name "satellite"" -- `.find("str")` becomes `.satellite.find()`. Then,
// on when and how: each type takes it as soon as its methods have numbers ("E. yes we do this before we
// do anything further"); the old spelling is refused, naming the new one ("D. yes"); every call of a
// chain takes it ("B yes": `s.satellite.upper().satellite.find("X")`); and his own spacesuit's capsules
// stay as he writes them ("C yes": only the language's own words take satellite).
//
// SO A METHOD IS WRITTEN `.satellite.` EXACTLY WHEN IT RUNS A NUMBER -- today a string's 24
// (string_number_of) and a list's 24 (list_number_of). Every other method keeps its `.` until its type
// is numbered: a map's, a file's, a number's, a window's. An object's capsules keep it for good.
//
// ONE RULE, ASKED IN TWO PLACES: by the walker (expression.cpp's call_method), which knows what the
// receiver holds, and by the checker (program_check.cpp's method_on_a_name), which knows what a name was
// declared as and says it before anything runs. The lexer keeps which period was written
// (satellite_method_token, bytecode_registry.cpp) and judges nothing: it does not know the receiver.

#include "container_calls.hpp"
#include "string_calls.hpp"
#include "token_codes.hpp"
#include "file_calls.hpp"
#include "../satellite_object/satellite_object.hpp"

#include <string>

namespace satellite004 {

// A METHOD'S PERIOD: `.`, or `.satellite.` before a word under satellite. The lexer makes the second only
// straight before a method's own code, so a reader that asks this and then is_method_code(next) has found
// a method call written either way; which way its receiver takes is the rule below.
inline bool a_method_period(token::Code code)
{
    return code == token::method_token || code == token::satellite_method_token;
}

// The number a method runs on this kind of receiver, or 0 when it runs none -- and so is written `.`.
inline token::Code number_a_method_runs(satelliteObject::Kind kind, token::Code method)
{
    if (kind == satelliteObject::string) return string_number_of(method);
    if (kind == satelliteObject::list) return list_number_of(method);
    return 0;
}

// WHAT A METHOD ANSWERS, by what its receiver is -- for the checker, which follows a chain past its first
// link only while this knows: `s.satellite.upper().find("X")` is told before anything runs that its second
// link takes `.satellite.` too (the author's B of 2026-10-10: every call of a chain). how_many_kinds is
// "not known here" -- an item of a list, a capsule's answer -- and the walker judges the rest when it runs.
inline satelliteObject::Kind kind_a_method_answers(satelliteObject::Kind receiver, token::Code method)
{
    using O = satelliteObject;
    switch (receiver) {
    case O::string:
        switch (method) {
        case token::size_token: case token::find_token: case token::to_number_token: return O::number;
        case token::empty_token: case token::contains_token: case token::starts_with_token:
        case token::ends_with_token: return O::boolean;
        case token::split_token: return O::list;
        default: return string_number_of(method) != 0 ? O::string : O::how_many_kinds;
        }
    case O::list:
        switch (method) {
        case token::size_token: case token::index_of_token: case token::search_token: return O::number;
        case token::contains_token: case token::empty_token: return O::boolean;
        case token::join_token: return O::string;
        case token::sort_token: case token::by_name_token: case token::by_value_token: case token::reverse_token:
        case token::append_token: case token::clear_token: case token::insert_token: case token::truncate_token:
        case token::reserve_token: case token::remove_token: case token::remove_at_token:
        case token::remove_first_token: case token::remove_last_token: return O::list;   // each answers the list
        default: return O::how_many_kinds;     // an item -- .first, .last, .max -- is what the run put there
        }
    case O::index:
        switch (method) {
        case token::keys_token: case token::values_token: return O::list;
        case token::size_token: return O::number;
        case token::contains_token: case token::empty_token: return O::boolean;
        case token::sort_token: case token::reverse_token: return O::index;   // a map in order is still a map
        default: return O::how_many_kinds;
        }
    case O::number:
        if (method == token::to_number_token) return O::number;
        [[fallthrough]];
    case O::floating: case O::hexadecimal: case O::binary: case O::fraction: case O::color: case O::percentage:
        return method == token::to_string_token || method == token::to_binary_token ||
                       method == token::to_hexadecimal_token
                   ? O::string
                   : O::how_many_kinds;
    case O::time: case O::boolean: case O::infinity:
        return method == token::to_string_token ? O::string : O::how_many_kinds;   // their text, as shown
    default:
        return O::how_many_kinds;
    }
}

// THE SENTENCE FOR A METHOD WRITTEN WITH THE OTHER PERIOD. `receiver` is the call as written up to its
// period -- `s`, `s.satellite.upper()` -- `word` the method's name, and `kind` what the receiver is, "a
// string" (kind_name). `under` is whether `.satellite.` was written: asked only when that is not what
// number_a_method_runs says, so under means the method runs no number on this kind, and not under means
// it does. The checker passes the name as the program wrote it (`s.str`), the walker the registry's.
inline std::string period_written_wrong(const std::string &receiver, const std::string &word, const char *kind,
                                        bool under)
{
    const std::string kinds = std::string(kind) + "'s";
    if (!under)
        return receiver + "." + word + " is written " + receiver + ".satellite." + word + " now -- " + kinds +
               " own words are under satellite";
    const std::string bare = receiver + ".satellite." + word + " is written " + receiver + "." + word + " -- ";
    if (std::string(kind) == "an object" || std::string(kind) == "a pointer")
        return bare + "an object's capsules are its spacesuit's own, and only the language's own words are "
                      "under satellite";
    // A STRING'S AND A LIST'S EVERY WORD IS UNDER SATELLITE, so one that is not among them is not theirs at all:
    // `s.satellite.first` is told a string has no first, never sent to write s.first and be refused again.
    if (std::string(kind) == "a string" || std::string(kind) == "a list")
        return receiver + ".satellite." + word + " -- " + kind + " has no " + word + "; its own words are written "
               "under satellite, and " + word + " is not one of them";
    return bare + kinds + " words are not under satellite yet -- a string's and a list's are";
}

} // namespace satellite004
