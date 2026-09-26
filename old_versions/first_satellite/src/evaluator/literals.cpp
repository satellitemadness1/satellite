// What a brace literal MEANS in a position that has a declared type.
//
// §8.6's containers could be built before ListLit existed -- declare empty,
// then .append() -- and could not be written down. ListLit gave them a
// spelling and left one question open, which ast_expr.hpp states exactly:
//
//     the ELEMENT TYPE is not here, because the type belongs to the variable
//     or parameter the literal is being handed to, not to the literal.
//
// This file is that sentence made true. `{a, b}` is SHAPE-NEUTRAL: handed to a
// list<list<string>> it is a list, handed to a list<map<string, number>> it is
// a map, and handed to nothing in particular it stays the list it always was.
// Nothing about the literal changes; what changes is that the position it lands
// in is now allowed to say what it wanted.
//
// RECURSIVE, which is what makes the crazy combinations work: shaping an
// element of a list<map<...>> asks this same function about the map, and
// shaping that map's values asks it again about whatever they are. There is no
// depth at which a new case has to be written.

#include "evaluator/eval_internal.hpp"

namespace satellite {
namespace {

// Every element is itself a two-element list -- the {{k, v}, {k, v}} spelling.
bool all_pairs(const List &items)
{
    for (const ValuePtr &item : items) {
        const List *pair = item ? as_list(*item) : nullptr;
        if (!pair || pair->size() != 2)
            return false;
    }
    return !items.empty();
}

} // namespace

bool is_brace_literal(const Expr *expr)
{
    return expr && std::holds_alternative<ListLit>(*expr);
}

// One entry, both halves shaped and both halves checked.
//
// The CHECK is not optional and not deferred to the caller's matches(): a key
// that cannot be a key has to fail HERE, because that failure is what tells the
// flat spelling apart from the nested one. {{a, 1}, {b, 2}} is even-length, so
// the flat reading is tried first and produces the key {a, 1} -- a list, which
// §8.6 refuses -- and the refusal is exactly the signal to try the other
// reading. Without it the flat form would silently win and build one nonsense
// entry out of two good ones.
static bool shape_entry(const Type &key_type, const Type &value_type,
                        const ValuePtr &key_in, const ValuePtr &value_in,
                        MapBody &body, std::string &error)
{
    std::string ignored;
    ValuePtr key = shape_literal(key_type, key_in, ignored);
    ValuePtr value = shape_literal(value_type, value_in, ignored);
    if (!key || !value)
        return false;

    std::string canonical;
    if (!map_key_of(*key, canonical))
        return false;
    if (!matches(key_type, *key) || !matches(value_type, *value))
        return false;

    MapBody next;
    if (!map_with(body, key, value, next, error))
        return false;
    body = std::move(next);
    return true;
}

ValuePtr shape_literal(const Type &type, const ValuePtr &value,
                       std::string &error)
{
    const List *items = value ? as_list(*value) : nullptr;
    if (!items || type.space != "container")
        return value;

    // A list wanting a list: shape the ELEMENTS and rebuild only if one moved.
    // This is the arm that carries the recursion, and it is why
    // list<list<map<...>>> needs nothing written for it.
    if (type.name == "list") {
        if (type.args.empty())
            return value;
        List shaped;
        shaped.reserve(items->size());
        bool moved = false;
        for (const ValuePtr &item : *items) {
            ValuePtr one = shape_literal(type.args[0], item, error);
            if (!one)
                return nullptr;
            moved = moved || (one != item);
            shaped.push_back(std::move(one));
        }
        return moved ? make_value(make_list(std::move(shaped))) : value;
    }

    if (type.name != "map" || type.args.size() != 2)
        return value;

    // `{}` is the one literal whose shape cannot be read off its contents, and
    // that is precisely why the type is allowed to say. An empty map, not an
    // empty list, when a map is what was declared.
    if (items->empty())
        return make_value(make_map(MapBody{}));

    // FLAT FIRST when the count is even -- {"str1", 99} and {"a", 1, "b", 2}.
    // This is the spelling that was asked for, so it is the one tried first.
    if (items->size() % 2 == 0) {
        MapBody body;
        bool ok = true;
        for (size_t i = 0; ok && i + 1 < items->size(); i += 2)
            ok = shape_entry(type.args[0], type.args[1], (*items)[i],
                             (*items)[i + 1], body, error);
        if (ok) {
            error.clear();
            return make_value(make_map(std::move(body)));
        }
    }

    // NESTED SECOND -- {{"a", 1}, {"b", 2}}. Reached either because the count
    // was odd or because the flat reading produced a key §8.6 refuses, which
    // is what a list-shaped key means here.
    if (all_pairs(*items)) {
        MapBody body;
        bool ok = true;
        for (const ValuePtr &item : *items) {
            const List &pair = *as_list(*item);
            ok = shape_entry(type.args[0], type.args[1], pair[0], pair[1], body,
                             error);
            if (!ok)
                break;
        }
        if (ok) {
            error.clear();
            return make_value(make_map(std::move(body)));
        }
    }

    // NEITHER READING WORKED, and the message says what both wanted rather
    // than what the last one tried. A program that writes an odd number of
    // elements has made a different mistake from one whose types are wrong,
    // and naming both spellings is what lets the reader tell which they made.
    error = "cannot read " + to_string(*value) + " as " + unparse(type) +
            ": a map literal is either key and value in turn -- "
            "{\"a\", 1, \"b\", 2} -- or a pair per entry -- "
            "{{\"a\", 1}, {\"b\", 2}}";
    return nullptr;
}

} // namespace satellite
