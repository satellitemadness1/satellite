// Small helpers shared across the evaluator: dotted paths and what one says
// when it is misused, the value a declaration starts at, whole-number indices
// and the ranges built out of them, the ValuePtr constructors, and the two
// lookups a call needs before it can dispatch.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share. This file was itself 779 lines and split again by
// what each helper is FOR: equality is in helpers_equality.cpp, the directory
// listing in helpers_listing.cpp, the satellite-rooted constants in
// helpers_module_constants.cpp, and the two tunable limits in helpers_limits.cpp.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

// Bounds the C++ recursion the tree walk costs, so a pathological tree raises
// a satellite error instead of segfaulting the C++ stack (§6).
// satellite.library.main.x -> {"satellite", "library", "main", "x"}.
// False for anything that is not a pure SatelliteLit/Member chain, which is
// what separates a language path from an expression with members on it.
bool flatten_path(const Expr &expr, std::vector<std::string> &out)
{
    if (std::holds_alternative<SatelliteLit>(expr)) {
        out.push_back("satellite");
        return true;
    }
    if (const Member *m = std::get_if<Member>(&expr)) {
        if (!m->target || !flatten_path(*m->target, out))
            return false;
        out.push_back(m->name);
        return true;
    }
    return false;
}

std::string duration_misuse(const std::string &text)
{
    // Names the working form rather than only refusing: a duration is legal in
    // exactly one position, so "not here" without "there" leaves the reader
    // nothing to do. §8.2 is cited because the absence of a duration TYPE is a
    // decision with a reason, not an omission.
    return "a duration is not a value: " + text +
           " is a length of time, and satellite.console.display(" + text +
           ") — which sets the pause the printer takes between two displayed "
           "lines — is the only place the language asks for one (§8.2: there "
           "is no satellite.variable.duration)";
}

std::string join_path(const std::vector<std::string> &path)
{
    std::string out;
    for (size_t i = 0; i < path.size(); i++) {
        if (i)
            out += ".";
        out += path[i];
    }
    return out;
}

// A declaration with no initialiser still gets a value of its declared type,
// so `satellite.container.list<...> l` is an empty list you can append to
// rather than a nil you cannot. The bare `satellite` type has no such value
// and stays nil.
//
// A SPACESUIT type is nil here, and the declaration statement builds the
// instance instead (exec, below). The split is not arbitrary: a spacesuit's
// default is an ALLOCATION, so this function would have to be able to fail and
// to run satellite code, and a field of a suit's own type would default-
// construct forever. A variable is not part of any object's layout, so it has
// no such regress and gets a real instance; a field is, so it starts as nil and
// a method fills it with my_class_name().
Value default_of(const Type &type)
{
    if (type.is_singleton() || type.is_spacesuit())
        return std::monostate{};
    if (type.space == "variable") {
        if (type.name == "bool")
            return false;
        if (type.name == "number")
            return Number();
        if (type.name == "string")
            return make_string(SatString{});
        // The epoch. A time has no "empty" the way a string does, and the epoch
        // is the one instant that is a fact rather than a choice.
        if (type.name == "time")
            return Time{};
        // A file is nil until it is opened, and nil is a real answer rather
        // than a placeholder: satellite.variable.file is a reference type
        // (§8.3), and a declaration cannot open anything because it has no path
        // to open and nowhere to report that opening failed.
    }
    if (type.space == "container" && type.name == "list")
        return make_list(List{});
    // An empty map, not nil, for the same reason a list starts empty: a
    // declaration you cannot immediately .set() into would be useless, and nil
    // would make every map variable need an initialiser the language has no
    // syntax for.
    if (type.space == "container" && type.name == "map")
        return make_map(MapBody{});
    return std::monostate{};
}


// An index must be a whole number: 1.5 is a bug in the program, not a
// silently truncated 1.
bool as_index(const Value &v, long long &out)
{
    // Number::to_integer answers both halves at once — is it whole, and does it
    // fit — so there is no isfinite/floor dance any more, and no way for a
    // value that merely rounds to an integer to pass as one.
    const Number *n = std::get_if<Number>(&v);
    return n && n->to_integer(out);
}

// Half-open, negative bounds Python-style, out-of-range clamps, hi < lo is
// empty (§7).
void clamp_range(long long &lo, long long &hi, long long len)
{
    if (lo < 0)
        lo += len;
    if (hi < 0)
        hi += len;
    lo = std::max(0LL, std::min(lo, len));
    hi = std::max(0LL, std::min(hi, len));
    if (hi < lo)
        hi = lo;
}

ValuePtr make_value(Value v)
{
    return std::make_shared<const Value>(std::move(v));
}

// A string and a list are stored behind a handle (value.hpp), so neither
// converts to a Value implicitly any more. These two overloads keep that a
// detail of the value model rather than something every call site restates.
ValuePtr make_value(SatString s)
{
    return make_value(make_string(std::move(s)));
}

ValuePtr make_value(List items)
{
    return make_value(make_list(std::move(items)));
}

ValuePtr make_value(MapBody body)
{
    return make_value(make_map(std::move(body)));
}

// Methods that write back through their receiver. They are the reason a
// receiver has to name a storage slot.
bool is_mutator(const std::string &name)
{
    return name == "append" || name == "set" || name == "remove";
}

// One named entry of the arguments object, or a failure that lists what does
// exist.
//
// ABSENT IS AN ERROR, never the empty string, and the reason is the same one
// §8.6 gives for a missing map key: a typo that quietly returns "" is a bug
// that surfaces three capsules away from the line that caused it. .has() is
// how a program asks without failing.
//
// The message names every entry, because the whole point of the object is that
// the program does not have to know the list in advance -- so the moment it
// guesses wrong is exactly the moment to show it.
ValuePtr Evaluator::argument_named(const Arguments &args,
                                   const std::string &name, Span span)
{
    auto found = args.index.find(name);
    if (found != args.index.end()) {
        const ValuePtr &value = args.entries[found->second].value;
        return value ? value : make_value(std::monostate{});
    }

    std::string known;
    for (const ArgumentEntry &e : args.entries) {
        if (!known.empty())
            known += ", ";
        known += e.name;
    }
    fail(span, "the arguments object has no " + name + ". It has: " + known);
    return nullptr;
}

} // namespace satellite
