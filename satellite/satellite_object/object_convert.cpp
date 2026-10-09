// satellite/satellite_object/object_convert.cpp -- EVERY CONVERSION, THROUGH ONE
// HUB, which is the answer to how many of them there have to be.
//
// (the author, 2026-09-16) "string_object.as_binary() string_object.binary()
// string_object.bin() and also take string_object.bin.something_else like...
// string_object.bin.find(\"1010111\") we auto convert in that case to find a
// binary within binary".
//
// THE COMBINATORIAL EXPLOSION IS NOT REAL, and this file is why. With N types
// there appear to be N x N conversions to write -- string to binary, string to
// hex, hex to binary, and so on -- and that is what makes the job look endless.
// It is not, because satellite_number is a HUB every type already converts
// through:
//
//     string --> number --> binary
//     hex ------^      \--> text
//
// So `.bin` on a string is string_to_number then number_to_binary: TWO hops
// through functions that already existed and are already checked, and no
// string_to_binary function is ever written. N types cost N conversions in and N
// out, not N x N.
//
// THE ONE EXCEPTION IS A satellite.variable.binary's OWN TEXT. Its `.bin` and
// `.string` do not go round the hub, because the hub is a number and a number
// has no width: b0011 would come back as "11". Its `.number` and `.hex` do go
// through, and lose the width exactly as 003 DESIGN 8.5 says a conversion must.
//
// EVERY ONE IS EXPLICIT AT THE SOURCE LEVEL. DESIGN 1.1 says nothing is converted
// on its own, and nothing here breaks that: a program reaches these by WRITING
// `.bin`, and an operator never calls them. The author's "we auto convert" is
// about the two hops inside one written conversion, not about a conversion the
// program did not ask for.

#include "fast_paths.hpp"
#include "bool_to_string.hpp"
#include "number_to_binary.hpp"
#include "number_to_hexadecimal.hpp"
#include "number_to_string.hpp"
#include "string_to_number.hpp"

namespace satellite004 {
namespace {

// The hub. Anything that can become a number becomes one here, and every other
// conversion is written as "reach the hub, then leave it".
signed long long int reach_the_number(const satelliteObject &from, satellite_number &out)
{
    if (const satellite_number *held = from.as_number()) { out = *held; return success; }
    // A binary is already a number underneath, so it reaches the hub in one step
    // and nothing is re-read -- only the width is left behind.
    if (const satellite_binary_number *held = from.as_binary()) { out = held->bits; return success; }
    if (const satellite_string *text = from.as_string())
        return string_to_number(*text, out);
    return types_do_not_meet;
}

} // namespace

signed long long int object_to_string(const satelliteObject &from, satelliteObject &out)
{
    satellite_string text;
    signed long long int code = success;
    if (const satellite_string *held = from.as_string())
        text = *held;
    else if (const satellite_number *held = from.as_number())
        code = number_to_string(*held, text);
    else if (const bool *held = from.as_bool())
        code = bool_to_string(*held, text);
    else if (const satellite_percentage *held = from.as_percentage()) {
        std::size_t bad_offset = 0;       // "50%": exactly what display prints
        code = satellite_string::from_utf8(held->written(), text, bad_offset);
    } else if (const satellite_binary_number *held = from.as_binary()) {
        std::size_t bad_offset = 0;       // "b00101010": exactly what display prints
        code = satellite_string::from_utf8(held->written(), text, bad_offset);
    } else if (from.is_infinity()) {
        std::size_t bad_offset = 0;       // "(infinity)": exactly what display prints
        code = satellite_string::from_utf8(satellite_infinity::display(from.as_infinity()), text, bad_offset);
    } else
        return types_do_not_meet;
    if (code != success)
        return code;
    out = satelliteObject::of_string(std::move(text));
    return success;
}

signed long long int object_to_number(const satelliteObject &from, satelliteObject &out)
{
    satellite_number value;
    const signed long long int code = reach_the_number(from, value);
    if (code != success)
        return code;
    out = satelliteObject::of_number(std::move(value));
    return success;
}

signed long long int object_to_binary(const satelliteObject &from, satelliteObject &out)
{
    // A BINARY IS NOT SENT ROUND THE HUB, because the hub has no width and the
    // trip would turn b0011 into "11". Its own digits are the answer, the b off
    // them as number_to_binary leaves it off.
    if (const satellite_binary_number *held = from.as_binary()) {
        satellite_string text;
        std::size_t bad_offset = 0;
        const signed long long int code = satellite_string::from_utf8(held->digits(), text, bad_offset);
        if (code != success)
            return code;
        out = satelliteObject::of_string(std::move(text));
        return success;
    }

    satellite_number value;                       // hop one: reach the hub
    const signed long long int code = reach_the_number(from, value);
    if (code != success)
        return code;
    satellite_string text;                        // hop two: leave it
    const signed long long int held = number_to_binary(value, text);
    if (held != success)
        return held;
    out = satelliteObject::of_string(std::move(text));
    return success;
}

signed long long int object_to_hexadecimal(const satelliteObject &from, satelliteObject &out)
{
    satellite_number value;
    const signed long long int code = reach_the_number(from, value);
    if (code != success)
        return code;
    satellite_string text;
    const signed long long int held = number_to_hexadecimal(value, text);
    if (held != success)
        return held;
    out = satelliteObject::of_string(std::move(text));
    return success;
}

} // namespace satellite004
