// satellite.container.map<K, V> — the key contract, the read surface, and the
// two transforms that write.
//
// Part of src/evaluator/, and on the RUNTIME side of the seam eval_internal.hpp draws:
// this is method and module surface a bytecode VM calls unchanged, not tree
// walking a VM replaces.
//
// It is its own file rather than another arm in methods.cpp because methods.cpp
// is 341 lines against the ~400 limit, and the key canonicaliser alone is worth
// more explanation than it is code.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// ---------------------------------------------------------------------------
// What may be a key — §8.6
// ---------------------------------------------------------------------------

// A key's canonical bytes, or false when this value cannot be a key.
//
// NATIVE C++, and it can never be anything else. §7's invariant is that no
// satellite code runs inside Library::update(), because re-entering a held
// write lock deadlocks — so hashing and equality cannot be a user hook, and
// that alone decides which types may be keys.
//
// A NUMBER canonicalises through to_string(). That is exact rather than
// convenient: Number's operator== is compare() == 0, which is VALUE equality,
// so 1 and 1.0 are the same number held as `sig 1 exp 0` and `sig 10 exp -1`.
// to_string() renders only from normalized(), which strips trailing zeros and
// adjusts the exponent, so a.to_string() == b.to_string() exactly when a == b.
// 1 and 1.0 are therefore ONE key, which is the only answer consistent with
// `1 == 1.0` being true in the language.
//
// A STRING canonicalises through its raw SatChar codes and NEVER through
// decode(). This is the sharpest trap in the whole feature. §8.5's \home, \cwd,
// \user and their siblings are LIVE codes that expand at decode() time, so
// hashing decoded text would make a key's identity depend on the machine, on
// the user, and on the current working directory — and a key inserted before a
// satellite.directory.change() would stop being findable after it. Two strings
// that decode alike are different keys, and that is correct.
//
// The one-byte type tag is what stops the number 12 and the string "12" being
// the same key. Without it they canonicalise to different bytes only by luck.
bool map_key_of(const Value &v, std::string &out)
{
    if (const Number *n = std::get_if<Number>(&v)) {
        out = "n";
        out += n->to_string();
        return true;
    }
    if (const SatString *s = as_string(v)) {
        out = "s";
        // The codes themselves, two bytes each, exactly as they sit in memory.
        // Not decoded, not re-encoded, not narrowed.
        out.append(reinterpret_cast<const char *>(s->data()),
                   s->size() * sizeof(SatChar));
        return true;
    }
    return false;
}

// The message every "that cannot be a key" failure uses, so the rule is stated
// the same way wherever a program breaks it.
static std::string key_type_error(const std::string &method, const Value &got)
{
    return "satellite.container.map." + method +
           " wants a satellite.variable.string or satellite.variable.number "
           "key, got " + to_string(got);
}

// ---------------------------------------------------------------------------
// Reads
// ---------------------------------------------------------------------------

// The names the table below answers to, and the only list of them there is.
// Adding a row to call_map_method means adding a word here -- which is the same
// bargain is_mutator strikes two functions up, and it is struck for the same
// reason: a name list that lives in another file goes stale silently.
bool is_map_read_method(const std::string &name)
{
    return name == "length" || name == "to_string" || name == "keys" ||
           name == "values" || name == "get" || name == "has";
}

ValuePtr Evaluator::call_map_method(const ValuePtr &recv,
                                    const std::string &name,
                                    const std::vector<ValuePtr> &argv, Span span)
{
    const MapBody *self = as_map(*recv);
    if (!self)
        return nullptr;

    auto arity = [&](size_t want) {
        if (argv.size() == want)
            return true;
        fail(span, arity_message("satellite.container.map", name, want,
                                 argv.size()));
        return false;
    };

    if (name == "length")
        return arity(0) ? make_value(Number(self->entries.size())) : nullptr;

    if (name == "to_string")
        return arity(0) ? make_value(encode_raw(to_string(*recv))) : nullptr;

    // Insertion order, which is the whole reason MapBody keeps a vector beside
    // its index: .keys() is how a map is walked, since §5's only loop is the
    // C-shaped `for` and the language has no foreach.
    if (name == "keys" || name == "values") {
        if (!arity(0))
            return nullptr;
        List out;
        out.reserve(self->entries.size());
        for (const MapEntry &entry : self->entries)
            out.push_back(name == "keys" ? entry.key : entry.value);
        return make_value(std::move(out));
    }

    if (name == "get" || name == "has") {
        if (!arity(1))
            return nullptr;
        std::string key;
        if (!map_key_of(*argv[0], key)) {
            fail(span, key_type_error(name, *argv[0]));
            return nullptr;
        }
        auto found = self->index.find(key);
        if (name == "has")
            return make_value(found != self->index.end());
        // A missing key is an ERROR, not nil, and §7 already settled the
        // sibling case: an out-of-range index is an error. nil cannot mean
        // absent here because nil is a legitimate STORED value — default_of
        // hands one to any spacesuit-typed or file-typed slot — so returning it
        // would make "absent" and "present but nil" the same answer. .has()
        // is how a program asks without risking the error.
        if (found == self->index.end()) {
            fail(span, "no such key in the map: " + to_string(*argv[0]));
            return nullptr;
        }
        return self->entries[found->second].value;
    }

    fail(span, "satellite.container.map has no method " + name);
    return nullptr;
}

// ---------------------------------------------------------------------------
// Writes
//
// Both take the current body and produce the next one; neither touches a
// storage slot, a lock or the Library. That is deliberate — the read-modify-
// write protocol lives in exactly one place (Evaluator::update_through_slot),
// and these are pure functions it calls while holding whatever it holds.
//
// Copy-on-write, so a set is O(n). That is parity with the language's only
// other insertion primitive rather than a new cost: .append copies the whole
// backing vector too, measured at 0.04 / 0.18 / 0.77 s to build lists of
// 2000 / 4000 / 8000. §15's complaint about a list-backed symbol table is that
// LOOKUP is a linear scan, and the index answers exactly that.
// ---------------------------------------------------------------------------

bool map_with(const MapBody &current, const ValuePtr &key,
              const ValuePtr &value, MapBody &next, std::string &error)
{
    std::string canonical;
    if (!key || !map_key_of(*key, canonical)) {
        error = key_type_error("set", key ? *key : Value{std::monostate{}});
        return false;
    }

    next = current;
    auto found = next.index.find(canonical);
    if (found != next.index.end()) {
        // An existing key KEEPS ITS POSITION. Updating a value is not
        // re-inserting it, and a symbol table that reordered itself every time
        // a binding was refined would make .keys() useless for reporting.
        next.entries[found->second].value = value;
        return true;
    }
    next.index.emplace(canonical, next.entries.size());
    next.entries.push_back(MapEntry{key, value});
    return true;
}

bool map_without(const MapBody &current, const ValuePtr &key, MapBody &next,
                 std::string &error)
{
    std::string canonical;
    if (!key || !map_key_of(*key, canonical)) {
        error = key_type_error("remove", key ? *key : Value{std::monostate{}});
        return false;
    }

    auto found = current.index.find(canonical);
    if (found == current.index.end()) {
        // Symmetric with .get: removing what was never there is a bug in the
        // program, and .has() is how to ask first.
        error = "no such key in the map: " + to_string(*key);
        return false;
    }

    // Erasing from the middle of the vector shifts every later position, so the
    // index is rebuilt rather than patched. Rebuilding is O(n) and the copy
    // already was; patching would be O(n) too and is the kind of clever that
    // goes wrong silently.
    const size_t gone = found->second;
    next.entries.clear();
    next.entries.reserve(current.entries.size() - 1);
    for (size_t i = 0; i < current.entries.size(); i++)
        if (i != gone)
            next.entries.push_back(current.entries[i]);
    next.index.clear();
    next.index.reserve(next.entries.size());
    for (size_t i = 0; i < next.entries.size(); i++) {
        std::string k;
        map_key_of(*next.entries[i].key, k);   // already validated on insertion
        next.index.emplace(std::move(k), i);
    }
    return true;
}

} // namespace satellite
