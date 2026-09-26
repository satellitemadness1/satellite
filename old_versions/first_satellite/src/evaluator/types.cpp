// Runtime type matching, module naming, error rendering.
//
// Part of src/evaluator/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

bool matches(const Type &type, const Value &value)
{
    if (type.is_singleton())
        return std::holds_alternative<std::monostate>(value);

    // A spacesuit type accepts an instance of itself or of any descendant —
    // is_a() is the whole subtype rule — and it accepts nil, because objects
    // are a reference type and a reference may name nothing. The second half is
    // what makes a field of spacesuit type legal before a method fills it.
    if (type.is_spacesuit()) {
        if (std::holds_alternative<std::monostate>(value))
            return true;
        const ObjectPtr *object = std::get_if<ObjectPtr>(&value);
        return object && *object && (*object)->suit &&
               (*object)->suit->is_a(type.name);
    }

    if (type.space == "variable") {
        if (type.name == "bool")
            return std::holds_alternative<bool>(value);
        if (type.name == "number")
            return std::holds_alternative<Number>(value);
        if (type.name == "string")
            return (as_string(value) != nullptr);
        if (type.name == "time")
            return std::holds_alternative<Time>(value);

        // §21. TWO NAMES, ONE ALTERNATIVE, told apart by the radix — so
        // `binary` and `hex` are exact-name-distinct exactly as every other
        // variable type is, and a capsule taking one will not accept the other.
        //
        // That is the requested behaviour and it has a cost worth naming here,
        // at the site that imposes it: matches() is exact-name equality and
        // §12 defers user-defined generics, so a capsule that wants "either"
        // cannot be written. The conversions are what a program uses instead —
        // .to_hex() and .to_binary() are total and value-preserving, so the
        // caller converts at the boundary rather than the callee accepting both.
        //
        // `hexadecimal` is the same type spelled out. It is the one alias in
        // the language, and it is here because the user asked for both
        // spellings; §1's one-spelling rule is bent knowingly rather than by
        // accident, and unparse always emits `hex`.
        if (type.name == "binary") {
            const Bits *bits = as_bits(value);
            return bits && bits->radix == 2;
        }
        if (type.name == "hex" || type.name == "hexadecimal") {
            const Bits *bits = as_bits(value);
            return bits && bits->radix == 16;
        }
        // nil satisfies a file for the same reason it satisfies a spacesuit:
        // both are reference types, and a reference may name nothing.
        if (type.name == "file")
            return std::holds_alternative<FilePtr>(value) ||
                   std::holds_alternative<std::monostate>(value);
        return false;
    }

    if (type.space == "container" && type.name == "list") {
        // The arguments object satisfies list<string>, which is what keeps §2's
        // signature — and therefore hello world, the man page, and every
        // program anybody has written — exactly as it was.
        //
        // It is a list of strings BY CONSTRUCTION: arguments_for() in
        // system.cpp is the only thing that builds one, and every entry it
        // writes carries a string Value. This used to trust that and skip the
        // element walk, saying so in a comment. It no longer does, for the
        // reason §20.3.1 gives about the crossing table: "by construction" is a
        // claim about today's only builder, and nothing in the compiler keeps
        // it true when a second builder appears. The walk is the check that a
        // comment was standing in for.
        //
        // The cost is nothing that matters. This runs when a declaration or an
        // insertion is type-checked against list<string>, and the arguments
        // object is bound to satellite.main's parameter ONCE per run — it is
        // not on any hot path, and the entry count is the command line plus
        // §8's table, not user data.
        if (const Arguments *args = as_arguments(value)) {
            if (type.args.empty())
                return true;
            if (type.args[0].space != "variable" ||
                type.args[0].name != "string")
                return false;
            for (const ArgumentEntry &entry : args->entries)
                if (!entry.value || !as_string(*entry.value))
                    return false;
            return true;
        }

        const List *list = as_list(value);
        if (!list)
            return false;
        if (type.args.empty())
            return true;
        for (const ValuePtr &item : *list)
            if (!item || !matches(type.args[0], *item))
                return false;
        return true;
    }

    // Both positions are checked, and args[1] is safe to read only because
    // Resolver::check_type guarantees a map's argument count is 0 or 2
    // (src/environment/scopes.cpp). A one-argument map never reaches here.
    // satellite.container.result -- Satellite Orbit's answer, and the one
    // container type that takes no type arguments.
    //
    // NIL SATISFIES IT, for the reason nil satisfies a file: a result is what a
    // search CONCLUDED, and there is no empty conclusion to default a
    // declaration to. `satellite.container.result r` is nil until a search
    // fills it, exactly as a file handle is nil until something opens it, and a
    // type that refused nil would make the declaration form unusable -- which
    // is the whole reason this type exists, since a result could not be bound
    // to any type at all before it.
    if (type.space == "container" && type.name == "result")
        return std::holds_alternative<ResultRef>(value) ||
               std::holds_alternative<std::monostate>(value);

    if (type.space == "container" && type.name == "map") {
        // A RESULT SATISFIES A BARE `map`, and satisfies an ARGUMENT-LESS one
        // only. It is a map underneath -- string keys, in insertion order -- so
        // `satellite.container.map m = found[0]` is true about it; but the
        // values are a list, a nil, a number, a bool and a string together, so
        // no `map<K, V>` is true about it and none is quietly allowed to be.
        // That heterogeneity is the measured reason this type was built, and a
        // matches() that papered over it would put the old lie back.
        if (const ResultBody *result = as_result(value))
            return type.args.empty() && result->fields != nullptr;

        const MapBody *map = as_map(value);
        if (!map)
            return false;
        if (type.args.empty())
            return true;    // bare `map` matches any map, as a bare list does
        for (const MapEntry &entry : map->entries)
            if (!entry.key || !matches(type.args[0], *entry.key) ||
                !entry.value || !matches(type.args[1], *entry.value))
                return false;
        return true;
    }

    return false;
}

const char *module_of(const Value &value)
{
    switch (value.index()) {
    case 0: return nullptr;                         // nil owns no methods
    case 1: return "satellite.variable.bool";
    case 2: return "satellite.variable.number";
    case 3: return "satellite.variable.string";
    case 4: return "satellite.container.list";
    case 5: {
        // The one entry that is not a literal: an instance's methods belong to
        // its spacesuit, so the "module path" is the suit's name. Borrowed from
        // the SpacesuitInfo, which the ResolveResult owns and which therefore
        // outlives any message this ends up in.
        const ObjectPtr &object = std::get<ObjectPtr>(value);
        return suit_name(object ? object->suit : nullptr).c_str();
    }
    case 6: return "satellite.variable.time";
    case 7: return "satellite.variable.file";
    case 8: return "satellite.container.map";
    // §21. The radix picks the name, which is what makes an error message about
    // a binary value say `binary` and not `hex`.
    case 9: {
        const Bits *bits = as_bits(value);
        return (bits && bits->radix == 2) ? "satellite.variable.binary"
                                          : "satellite.variable.hex";
    }
    // The arguments object. `satellite.container.arguments` rather than
    // `.list`, because an arity or type message that named it a list would send
    // the reader to the list's method table, which does not have .names() or
    // .count() in it. It IS a list<string> to matches(); it is its own thing to
    // anyone reading an error.
    case 10: return "satellite.container.arguments";
    // Satellite Orbit's answer. `satellite.container.result` and not `.map`, on
    // the same argument the arguments object makes one line up: a message that
    // called it a map would send the reader to the map's method table, which
    // has no .weight(), no .attention() and no .alternatives() in it.
    case 11: return "satellite.container.result";
    default: return nullptr;
    }
}

std::string format_error(const EvalError &error, const SourceMap &sources)
{
    const std::string &source = sources.text(error.span.file);

    size_t at = std::min(error.span.start, source.size());
    size_t begin = source.rfind('\n', at == 0 ? 0 : at - 1);
    begin = (begin == std::string::npos) ? 0 : begin + 1;
    size_t end = source.find('\n', at);
    if (end == std::string::npos)
        end = source.size();

    std::string out = "satellite: " + error.message + " (" +
                      span_location(error.span, sources) + ")\n";
    out += "    " + source.substr(begin, end - begin) + "\n";
    out += "    " + std::string(at - begin, ' ') + "^\n";
    return out;
}

// ---------------------------------------------------------------------------
// Evaluator
// ---------------------------------------------------------------------------

} // namespace satellite
