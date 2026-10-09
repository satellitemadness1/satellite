#pragma once
// satellite/bytecode/capsule_key.hpp -- WHAT A BUTTON KEEPS TO NAME A CAPSULE, and the
// one place its format is written down.
//
// A KEY IS "row:names" -- `0:when_pressed`, `3:tools.go` -- and unique because a row
// is one file (capsule_scopes.hpp). A button's press happens long after the line that
// wired it, so it cannot keep the NAME: two files may each have a `when_pressed`.
//
// ITS OWN HEADER, AND IT INCLUDES NOTHING, because the window reads it too: a
// refusal about `.pressed(go)` should say `go`, and the window's files have no
// business including the bytecode's scope table to strip a prefix.

#include <string>

namespace satellite004 {

// THE NAME A KEY WAS WRITTEN AS -- `when_pressed` out of "0:when_pressed" -- for a
// sentence about it. Anything with no colon is handed back as it is.
inline std::string capsule_as_written(const std::string &key)
{
    const std::size_t colon = key.find(':');
    return colon == std::string::npos ? key : key.substr(colon + 1);
}

} // namespace satellite004
