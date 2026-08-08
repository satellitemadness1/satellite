// Runtime type matching, module naming, error rendering.
//
// Part of eval/, split from a 2208-line eval.cpp. See eval_internal.hpp
// for what these pieces share.

#include "eval_internal.hpp"

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
        // nil satisfies a file for the same reason it satisfies a spacesuit:
        // both are reference types, and a reference may name nothing.
        if (type.name == "file")
            return std::holds_alternative<FilePtr>(value) ||
                   std::holds_alternative<std::monostate>(value);
        return false;
    }

    if (type.space == "container" && type.name == "list") {
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
    default: return nullptr;
    }
}

std::string format_error(const EvalError &error, const std::string &source)
{
    size_t at = std::min(error.span.start, source.size());
    size_t begin = source.rfind('\n', at == 0 ? 0 : at - 1);
    begin = (begin == std::string::npos) ? 0 : begin + 1;
    size_t end = source.find('\n', at);
    if (end == std::string::npos)
        end = source.size();

    std::string out = "satellite: " + error.message + " (line " +
                      std::to_string(error.span.line) + ")\n";
    out += "    " + source.substr(begin, end - begin) + "\n";
    out += "    " + std::string(at - begin, ' ') + "^\n";
    return out;
}

// ---------------------------------------------------------------------------
// Evaluator
// ---------------------------------------------------------------------------

} // namespace satellite
