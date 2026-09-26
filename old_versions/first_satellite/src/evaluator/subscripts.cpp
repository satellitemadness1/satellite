// Indexing and slicing -- every form of [ ... ] the language has.
//
// Split out of operators.cpp on 2026-08-24, when the search subscript arrived
// and that file was at 324 lines against the 325 ceiling. The seam was already
// there: the file did four jobs and its own banner comment separated the two
// halves. eval_index and eval_slice moved VERBATIM apart from the search
// branch, which is the one thing this file has that the old one did not.

#include "evaluator/eval_internal.hpp"
#include "evaluator/search.hpp"

namespace satellite {


ValuePtr Evaluator::eval_index(const Index &node, Span span)
{
    if (!node.target || !node.subscript) {
        fail(span, "malformed index");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;
    ValuePtr sub = eval(*node.subscript);
    if (failed() || !sub)
        return nullptr;

    // A MAP is consulted before the subscript is coerced, and the order is the
    // whole change: this function used to demand a whole number of every
    // subscript up front, so `m["bolt"]` failed with "index must be a whole
    // satellite.variable.number" whatever the receiver turned out to be.
    //
    // A map key is not an index, so it takes the map's own rule (§8.6) rather
    // than as_index's.
    // A map key is not an index, so it takes the map's own rule (§8.6) rather
    // than as_index's.
    //
    // A MISS IS STILL AN ERROR, and that is §8.6 left exactly alone: nil is a
    // legitimate stored value, so "absent" and "present but nil" must not be
    // one answer, and .has() is how a program asks without risking it. Making
    // a miss search instead was tried and reverted -- eval_test pins the rule,
    // and a lookup that quietly became a search would change what m["k"]
    // MEANS for every program that has ever read a map.
    //
    // What DOES fall through is a subscript that could never have been a key
    // at all -- m[{"bolt", 7}]. §8.6 says what may BE a key; it says nothing
    // about what a non-key subscript should do, and that path was a dead end
    // with no rule attached to it. So the pair form reaches a map root, and no
    // existing spelling changes meaning.
    if (const MapBody *map = as_map(*target)) {
        std::string key;
        if (map_key_of(*sub, key)) {
            auto found = map->index.find(key);
            // Symmetric with an out-of-range index, and with .get.
            if (found == map->index.end()) {
                fail(node.subscript->span,
                     "no such key in the map: " + to_string(*sub));
                return nullptr;
            }
            const ValuePtr &value = map->entries[found->second].value;
            return value ? value : make_value(std::monostate{});
        }
        std::string error;
        ValuePtr found = search_collect(target, sub, false, max_depth_, error);
        if (!found)
            fail(node.subscript->span, error);
        return found;
    }

    // A RESULT IS SUBSCRIPTED BY FIELD NAME -- r["why"] -- because a result is a
    // map underneath and a map's subscript is by key. Consulted before the
    // whole-number demand below for the map's own reason: coercing the subscript
    // first would answer `r["why"]` with "index must be a whole
    // satellite.variable.number", which is a true sentence about the wrong
    // question.
    //
    // r[0] IS THE KEY 0 AND NOT ALTERNATIVE ZERO, and that is the search
    // power's DECISION 6a applied one level up: `l[0]` is positional in every
    // program ever written in this language, and a subscript whose meaning
    // depended on what the receiver turned out to be is the action at a distance
    // that decision spends its whole argument refusing. .alternatives() is a
    // list and takes [0] like any other.
    //
    // A MISSING FIELD IS AN ERROR, exactly as a missing map key is. The sixteen
    // fields are always all there, so the only way to reach this is a name that
    // was never a field -- which is a bug in the program, not an absence to
    // report as nil.
    if (const ResultBody *result = as_result(*target)) {
        ValuePtr fields = result->fields;
        if (fields) {
            const MapBody *map = as_map(*fields);
            std::string key;
            if (map && map_key_of(*sub, key)) {
                auto found = map->index.find(key);
                if (found == map->index.end()) {
                    fail(node.subscript->span,
                         "no such field in the result: " + to_string(*sub));
                    return nullptr;
                }
                const ValuePtr &value = map->entries[found->second].value;
                return value ? value : make_value(std::monostate{});
            }
            // Not a key at all -- a pair, a list -- so it falls through to the
            // search, which is what the map root does with the same subscript
            // and for the same reason: §8.6 says what may BE a key and says
            // nothing about what a non-key subscript should do.
            std::string error;
            ValuePtr found = search_collect(target, sub, false, max_depth_, error);
            if (!found)
                fail(node.subscript->span, error);
            return found;
        }
    }

    // THE ARGUMENTS OBJECT TAKES BOTH KINDS OF SUBSCRIPT, and which one it got
    // decides which half it reads. `args[1]` is the second command-line
    // argument, exactly as it was before this type existed; `args["username"]`
    // is a named entry. A string subscript is never a valid index, and a whole
    // number is never a valid entry name, so the two cannot be confused and
    // neither needs a flag to tell them apart.
    //
    // Consulted before the whole-number demand below, for the same reason the
    // map is: coercing the subscript first would make `args["username"]` fail
    // with "index must be a whole satellite.variable.number", which is a true
    // sentence about the wrong question.
    if (const Arguments *args = as_arguments(*target)) {
        if (const SatString *key = as_string(*sub))
            return argument_named(*args, decode(*key), node.subscript->span);

        // Falls through to the list rule below over the command-line half, so
        // the bounds, the negative index and the error text are the list's and
        // not a second copy of them.
        const List command_line = arguments_command_line(*args);
        long long index = 0;
        if (!as_index(*sub, index)) {
            fail(node.subscript->span,
                 "an arguments subscript is a whole satellite.variable.number "
                 "for a command-line argument, or a "
                 "satellite.variable.string for a named entry, got " +
                 to_string(*sub));
            return nullptr;
        }
        long long len = static_cast<long long>(command_line.size());
        if (index < 0)
            index += len;
        if (index < 0 || index >= len) {
            fail(node.subscript->span,
                 "index " + to_string(*sub) + " is outside the " +
                 std::to_string(command_line.size()) +
                 " command-line arguments");
            return nullptr;
        }
        const ValuePtr &value = command_line[static_cast<size_t>(index)];
        return value ? value : make_value(std::monostate{});
    }

    // Demanded per-receiver now rather than up front, so the message stays
    // exactly what it was for the receivers it applies to.
    long long i = 0;
    auto whole_number_index = [&]() {
        if (as_index(*sub, i))
            return true;
        fail(node.subscript->span,
             "index must be a whole satellite.variable.number, got " +
             to_string(*sub));
        return false;
    };

    // An out-of-range INDEX is an error; only a slice clamps (§7). A whole
    // number is positional, anything else a search pattern -- search_apply.cpp
    // has the reason l[0] may never move (DECISION 6a).
    if (const List *list = as_list(*target)) {
        if (!as_index(*sub, i)) {
            std::string error;
            ValuePtr found = search_collect(target, sub, false, max_depth_,
                                            error);
            if (!found)
                fail(node.subscript->span, error);
            return found;
        }
        long long len = static_cast<long long>(list->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a list of length " +
                                       std::to_string(len));
            return nullptr;
        }
        // O(1) and no copy at all: the child pointer is returned as-is.
        ValuePtr item = (*list)[static_cast<size_t>(i)];
        return item ? item : make_value(std::monostate{});
    }

    if (const SatString *s = as_string(*target)) {
        if (!whole_number_index())
            return nullptr;
        // Selects satellite characters, not display characters: encode("hi
        // \home!") is 4 SatChars that decode to 16 display bytes, so s[2] is
        // one unit that displays as a whole home directory. It is the only
        // O(1), stable rule, and it is unique to satellite (§7).
        long long len = static_cast<long long>(s->size());
        if (i < 0)
            i += len;
        if (i < 0 || i >= len) {
            fail(node.subscript->span, "index " + to_string(*sub) +
                                       " is outside a string of length " +
                                       std::to_string(len));
            return nullptr;
        }
        return make_value(SatString(1, (*s)[static_cast<size_t>(i)]));
    }

    fail(span, to_string(*target) + " cannot be indexed");
    return nullptr;
}

ValuePtr Evaluator::eval_slice(const Slice &node, Span span)
{
    if (!node.target) {
        fail(span, "malformed slice");
        return nullptr;
    }
    ValuePtr target = eval(*node.target);
    if (failed() || !target)
        return nullptr;

    long long len = 0;
    if (const List *list = as_list(*target))
        len = static_cast<long long>(list->size());
    else if (const SatString *s = as_string(*target))
        len = static_cast<long long>(s->size());
    else {
        fail(span, to_string(*target) + " cannot be sliced");
        return nullptr;
    }

    auto bound = [&](const ExprPtr &e, long long fallback, long long &out) {
        if (!e) {
            out = fallback;
            return true;
        }
        ValuePtr v = eval(*e);
        if (failed() || !v)
            return false;
        if (!as_index(*v, out)) {
            fail(e->span, "slice bound must be a whole "
                          "satellite.variable.number, got " + to_string(*v));
            return false;
        }
        return true;
    };

    long long lo = 0;
    long long hi = 0;
    if (!bound(node.lo, 0, lo) || !bound(node.hi, len, hi))
        return nullptr;
    clamp_range(lo, hi, len);

    if (const List *list = as_list(*target)) {
        // A full slice is the same list: no copy, and pointer identity is
        // preserved so `l[:]` is genuinely free.
        if (lo == 0 && hi == len)
            return target;
        // Slicing copies POINTERS only — the children stay shared.
        return make_value(List(list->begin() + lo, list->begin() + hi));
    }

    const SatString &s = *as_string(*target);
    if (lo == 0 && hi == len)
        return target;
    // Unlike a list slice, a string slice really does copy characters.
    return make_value(s.substr(static_cast<size_t>(lo),
                               static_cast<size_t>(hi - lo)));
}

} // namespace satellite
