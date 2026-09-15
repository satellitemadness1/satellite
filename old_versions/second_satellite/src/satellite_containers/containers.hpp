#pragma once

// The two body transforms every writer of a map shares -- PLAN M16. The types
// themselves live in satellite_value/value.hpp, because a List holds Values by
// value and the variant's arms are declared there; what lives HERE is the part
// of the containers that is behaviour rather than shape.
//
// MACHINE-FREE ON PURPOSE, like search.hpp beside it: a body transform takes
// the current body and produces the next one, and it neither touches a storage
// slot nor raises a diagnostic -- the caller owns both. That is v1's contract,
// kept for v1's reason: the read-modify-write protocol lives in exactly one
// place (here, the machine's mutating write-back, dispatch.hpp's `mutates`
// column), and these are pure functions it calls. It is also what lets
// search_walk.cpp build a hit map without linking the evaluator.

#include "satellite_value/value.hpp"

#include <string>

namespace satellite::containers {

// The message every "that cannot be a key" failure quotes, so the rule is
// stated the same way wherever a program breaks it. The caller wraps it in
// S0727; the text of WHICH types may be keys lives in errors.def's row.
//
// `map_key_of` itself is satellite_value's (value.hpp), beside the body whose
// index it feeds.

// The body with one entry set. An existing key KEEPS ITS POSITION -- updating
// a value is not re-inserting it, and a map that reordered itself every time a
// binding was refined would make `.keys()` useless for reporting. False when
// `key` cannot be a key at all; nothing else can fail.
bool map_with(const MapBody &current, const Value &key, const Value &value,
              MapBody &next);

// The body without one entry, or false when the key is not a key or is not
// there -- the caller tells those apart with map_key_of, exactly as the read
// side does. Erasing from the middle shifts every later position, so the index
// is rebuilt rather than patched: rebuilding is O(n) and the copy already was,
// and patching is the kind of clever that goes wrong silently.
bool map_without(const MapBody &current, const Value &key, MapBody &next);

} // namespace satellite::containers
