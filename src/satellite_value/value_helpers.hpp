#pragma once

#include "satellite_value/value_variant.hpp"

namespace satellite {

// Build a string or list Value without every call site spelling out the
// make_shared. These exist so that the handle is an implementation detail of
// value.hpp rather than a thing 22 sites in eval.cpp have to know about.
inline Value make_string(SatString s)
{
    return Value(std::make_shared<const SatString>(std::move(s)));
}
inline Value make_list(List items)
{
    return Value(std::make_shared<const List>(std::move(items)));
}
// Takes the body BY VALUE and freezes it: after this returns, the MapBody is
// const and shared, and the only way to "change" a map is to build a new body
// and publish it through the storage slot. See MapBody above for why.
inline Value make_map(MapBody body)
{
    return Value(std::make_shared<const MapBody>(std::move(body)));
}

// Build a binary or hex value. Takes the digits already validated and, for hex,
// already upper-cased — normalisation belongs to whoever parsed the digits, so
// that this stays the one cheap way to make one.
inline Value make_bits(unsigned radix, std::string digits)
{
    return Value(std::make_shared<const Bits>(Bits{radix, std::move(digits)}));
}

// Takes the body BY VALUE and freezes it, the same contract make_map has. The
// caller (arguments_for, in system.cpp) is the only place that builds one, and
// it fills `entries`, `index` and `command_line_count` before calling this.
inline Value make_arguments(Arguments body)
{
    return Value(std::make_shared<const Arguments>(std::move(body)));
}

// Read a string or list out of a Value, or null if it is not one.
//
// These replace `std::get_if<SatString>(&v)` at the call sites, and the
// indirection they hide is exactly the point: get_if on a handle alternative
// returns a pointer TO THE HANDLE, so every site would otherwise have to
// remember the second dereference. Forgetting it is a compile error today and
// would be a silent wrong-type read the moment anything is cached.
inline const SatString *as_string(const Value &v)
{
    const Str *p = std::get_if<Str>(&v);
    return p ? p->get() : nullptr;
}
inline const List *as_list(const Value &v)
{
    const ListRef *p = std::get_if<ListRef>(&v);
    return p ? p->get() : nullptr;
}
inline const MapBody *as_map(const Value &v)
{
    const MapRef *p = std::get_if<MapRef>(&v);
    return p ? p->get() : nullptr;
}
inline const Bits *as_bits(const Value &v)
{
    const BitsRef *p = std::get_if<BitsRef>(&v);
    return p ? p->get() : nullptr;
}
inline const Arguments *as_arguments(const Value &v)
{
    const ArgsRef *p = std::get_if<ArgsRef>(&v);
    return p ? p->get() : nullptr;
}

// The command-line half of an Arguments, as a plain List, so that every place
// which already knows what to do with a list<string> can be handed one without
// learning a second shape. Copies the handles, never the strings.
//
// This is what makes DECISION 4 cheap: .length(), [i], slicing, .first(),
// .last() and .contains() all run over THIS, so they keep answering exactly
// what they answered before the arguments object existed.
inline List arguments_command_line(const Arguments &args)
{
    List out;
    out.reserve(args.command_line_count);
    for (size_t i = 0; i < args.command_line_count && i < args.entries.size(); i++)
        out.push_back(args.entries[i].value);
    return out;
}

} // namespace satellite
