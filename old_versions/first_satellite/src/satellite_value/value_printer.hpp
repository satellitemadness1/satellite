#pragma once

#include "satellite_value/value_variant.hpp"

namespace satellite {

// One operator() per alternative and DELIBERATELY no generic `auto` fallback.
//
// The previous version tested four alternatives with get_if and then fell
// through to an unconditional `return decode(std::get<SatString>(v))`. Adding
// a fifth alternative compiled with zero warnings under -Wall -Wextra and then
// threw `std::get: wrong index for variant` on the first print — and the REPL
// prints through here, so the first `:get` of the new type killed the
// interpreter. A generic fallback arm silently reintroduces exactly that, so
// the exhaustiveness here is the point: adding an alternative to ValueBase now
// fails to compile until it is handled.
struct ValuePrinter {
    std::string operator()(std::monostate) const { return "nil"; }
    std::string operator()(bool b) const { return b ? "true" : "false"; }
    // §8.1.1's rule — print the value, never N significant digits — now lives
    // with the type it describes, in Number::to_string (bignum.cpp).
    std::string operator()(const Number &n) const { return n.to_string(); }
    std::string operator()(const Str &s) const { return s ? decode(*s) : ""; }
    std::string operator()(const ListRef &list) const;

    // Out of line in value.cpp: it needs the spacesuit's name, and printing an
    // object's fields instead would be wrong anyway — a field is reachable only
    // from inside the spacesuit, and a cyclic object graph would not terminate.
    std::string operator()(const ObjectPtr &object) const;

    std::string operator()(Time time) const;
    std::string operator()(const FilePtr &file) const;
    std::string operator()(const MapRef &map) const;

    // Prints WITH the prefix — `x00FF`, `b1010` — so that what a program
    // displays is what a program may type back in. §8.1.1's rule for a number
    // is "print the value, never N significant digits"; the equivalent for a
    // value whose width is load-bearing is to print every digit it has.
    std::string operator()(const BitsRef &bits) const;

    // One entry per line, `name` padded to the widest, then the value. NOT the
    // one-line `{k: v}` a map uses: there are about thirty entries and one of
    // them is a compiler version string, so a single line is unreadable in the
    // only place it is ever printed. `.lines()` on a list already established
    // that "one per line, aligned" is a form this language has.
    std::string operator()(const ArgsRef &args) const;

    // A result prints AS ITS MAP, one line, `{value: ..., weight: 50, ...}`.
    //
    // The opposite call to the arguments object above, and the reason is where
    // each of them is printed. An Arguments is displayed on its own, once, so a
    // column layout is readable; a result arrives in a LIST of results, and a
    // value that printed over sixteen lines would make `display(found)` a wall
    // no reader could find a result boundary in. The per-line rendering a person
    // wants is `.lines()`, which the list and the arguments object already spell
    // that way, and which a result answers with its own report.
    std::string operator()(const ResultRef &result) const;
};

inline std::string to_string(const Value &v)
{
    // Cast to the base: std::variant_size is not specialised for Value, which
    // merely inherits from it, so std::visit cannot deduce the alternatives.
    return std::visit(ValuePrinter{}, static_cast<const ValueBase &>(v));
}

inline std::string ValuePrinter::operator()(const ListRef &list) const
{
    if (!list)
        return "[]";
    std::string out = "[";
    for (size_t i = 0; i < list->size(); i++) {
        if (i)
            out += ", ";
        const ValuePtr &item = (*list)[i];
        out += item ? to_string(*item) : "nil";
    }
    return out + "]";
}

// `{key: value, key: value}`, in insertion order, and `{}` when empty. Braces
// rather than brackets so a map and a list are distinguishable at a glance in
// REPL output and in an error message.
//
// The order is what makes this testable: eval_test's check_output is exact
// string equality, so a map that rendered in hash order would make every test
// that prints one intermittently red.
inline std::string ValuePrinter::operator()(const MapRef &map) const
{
    if (!map || map->entries.empty())
        return "{}";
    std::string out = "{";
    for (size_t i = 0; i < map->entries.size(); i++) {
        if (i)
            out += ", ";
        const MapEntry &e = map->entries[i];
        out += e.key ? to_string(*e.key) : "nil";
        out += ": ";
        out += e.value ? to_string(*e.value) : "nil";
    }
    return out + "}";
}

// The map, and nothing added. A result IS its fields, so a printer that
// decorated them would make `to_string(r)` and `to_string(r.to_string())`
// two different renderings of one value.
inline std::string ValuePrinter::operator()(const ResultRef &result) const
{
    if (!result || !result->fields)
        return "{}";
    return to_string(*result->fields);
}

inline std::string ValuePrinter::operator()(const ArgsRef &args) const
{
    if (!args || args->entries.empty())
        return "";

    size_t width = 0;
    for (const ArgumentEntry &e : args->entries)
        if (e.name.size() > width)
            width = e.name.size();

    std::string out;
    for (size_t i = 0; i < args->entries.size(); i++) {
        const ArgumentEntry &e = args->entries[i];
        if (i)
            out += "\n";
        out += e.name;
        out.append(width - e.name.size() + 2, ' ');
        out += e.value ? to_string(*e.value) : "";
    }
    return out;
}

} // namespace satellite
