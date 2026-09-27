#pragma once
// satellite/satellite_object/satellite_map_entry.hpp -- ONE ENTRY OF A satellite.container.map:
// a key, its value, and the width of each.
//
// The author, 2026-09-26: *"a satellite.container.map is just two multiples and a width for each
// entry, so a satellite.container.map is just two multiples and a width, there's nothing more to it
// than that, you just need a class that has two registers -- one for the key and one for the value,
// and both key and value are a satellite.container.multiple"*; *"just look at the
// satellite.variable.float, it's just two satellite.variable.number's, this is no different, there's
// just a satelliteMapWidth added on this time"*; and before that, *"satellite.container.map holds the
// width of both the key and the value at all times"*.
//
// A satellite.container.multiple IS a satelliteObject (bytecode/type_shape.hpp: *"multiple is a
// std::variant"*, and satelliteObject is that variant), so an entry is two satelliteObjects -- as a
// float is two satellite_numbers -- and a width for each.
//
// THE WIDTHS ARE KEPT AS THE ENTRY CHANGES, NOT WORKED OUT WHEN ASKED: a key never changes after it
// is filed, so its width is measured once; a value's is measured every time a value is put in
// (put_value, below -- the one way a value goes in). A write INSIDE a value -- `m["a"][2] = 5` on a
// list -- leaves it the kind it was, and a container's width is 0, so nothing there goes stale.
// Nothing reads a width yet: .sort("key") and .sort("value") will.

#include "satellite_object.hpp"

#include <cstddef>
#include <utility>

namespace satellite004 {

// A WIDTH, AS A satellite.variable.number. The author: *"I don't necessarily want our numbers to
// have to adjust a width for every single calculation, so we will ... take a copy of it and have
// this copy only used for satellite.container.map to hold a width, we will call it a
// satelliteMapWidth, and it's superclass will just be satellite.variable.number"*. So every number
// stays as it was; a satelliteMapWidth IS one -- it compares, adds and prints as one -- and only a
// map entry holds them. It adds no field, so it is exactly a satellite_number's size.
class satelliteMapWidth : public satellite_number {
public:
    satelliteMapWidth() = default;   // 0: nothing measured, or nothing with a width
    explicit satelliteMapWidth(satellite_number width) : satellite_number(std::move(width)) {}

    // HOW WIDE A KEY OR A VALUE IS, in characters -- *"10 is always bigger than 9, it's 2 digits"*:
    //   a number     its digits (the minus sign is not a digit, as a float's sign is its own bool)
    //   a string     its characters
    //   anything else written as one value -- a float, a fraction, a bool, a binary, a hexadecimal,
    //                a percentage, a colour, infinity -- the characters display writes for it,
    //                a leading minus sign left out
    //   a container, a spacesuit object, a file, a window, a thread, a capsule, nothing: 0, being
    //                no one thing written out -- and a list's text would be measured again on
    //                every write
    static satelliteMapWidth of(const satelliteObject &held)
    {
        if (const satellite_number *number = held.as_number())
            return satelliteMapWidth(number->digits());
        if (const satellite_string *text = held.as_string())
            return satelliteMapWidth(satellite_number(static_cast<unsigned long long int>(text->size())));
        switch (held.kind()) {
        case satelliteObject::boolean:
        case satelliteObject::binary:
        case satelliteObject::percentage:
        case satelliteObject::infinity:
        case satelliteObject::floating:
        case satelliteObject::hexadecimal:
        case satelliteObject::color:
        case satelliteObject::fraction:
            break;
        default:
            return satelliteMapWidth();
        }
        satellite_string written;
        std::string why;
        if (held.to_string(written, why) != success)
            return satelliteMapWidth();
        // satellite's own character codes, not Unicode's: the minus sign is asked for by its Unicode.
        std::size_t characters = written.size();
        if (characters > 0 && satellite_string::unicode_of(written.code_at_unchecked(0)) == U'-')
            --characters;
        return satelliteMapWidth(satellite_number(static_cast<unsigned long long int>(characters)));
    }
};

// ONE ENTRY: *"two registers -- one for the key and one for the value"*, and the width of each.
struct satelliteMapEntry {
    satelliteObject key;              // a satellite.container.multiple
    satelliteObject value;            // a satellite.container.multiple
    satelliteMapWidth key_width;      // measured once: a key never changes
    satelliteMapWidth value_width;    // measured every time a value is put in

    satelliteMapEntry() = default;
    explicit satelliteMapEntry(const satelliteObject &filed)
        : key(filed), key_width(satelliteMapWidth::of(filed)), value_width(satelliteMapWidth::of(value)) {}
};

// A VALUE IN, AND ITS WIDTH WITH IT -- the one way a value goes into an entry.
inline void put_value(satelliteMapEntry &entry, satelliteObject &&value)
{
    entry.value = std::move(value);
    entry.value_width = satelliteMapWidth::of(entry.value);
}

} // namespace satellite004
