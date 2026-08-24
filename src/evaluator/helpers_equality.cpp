// What == means for a satellite value.
//
// Moved verbatim out of helpers.cpp, which was 779 lines. value_equals is one
// rule with one arm per alternative that is a handle to something with value
// semantics, and the comment on each arm records the bug that put it there --
// which is why the whole of it stays in one place.
//
// Part of src/evaluator/. See eval_internal.hpp for what these pieces share.

#include "evaluator/eval_internal.hpp"

namespace satellite {

// Structural, not pointer, equality: two lists holding equal values are equal
// even when they share no children. The variant's own operator== would compare
// the shared_ptrs inside a List, so the List arm has to come first.
//
// An OBJECT is the exception, and it needs no arm: the variant's own operator==
// compares the ObjectPtrs, which is identity, and identity is what equality
// means for a reference type. Two instances with equal fields are two
// instances — a spacesuit that wants them equal says so with a method, because
// only it knows which of its fields are part of what it means to be equal.
bool value_equals(const Value &a, const Value &b)
{
    if (a.index() != b.index())
        return false;
    if (const List *la = as_list(a)) {
        const List &lb = *as_list(b);
        if (la->size() != lb.size())
            return false;
        for (size_t i = 0; i < la->size(); i++) {
            const ValuePtr &x = (*la)[i];
            const ValuePtr &y = lb[i];
            if (!x || !y) {
                if (x != y)
                    return false;
                continue;
            }
            if (!value_equals(*x, *y))
                return false;
        }
        return true;
    }
    // Strings compare by CONTENT, and this arm is not optional.
    //
    // The variant's own operator== compares alternatives, and the string
    // alternative is a shared_ptr since strings moved behind a handle. That
    // compares POINTERS, so "x" == "x" was false whenever the two sides were
    // built separately — which is every interesting case. It shipped, and all
    // eleven test binaries passed, because nothing in the suite compared two
    // independently constructed strings. The satellite lexer written in
    // satellite is what caught it: its whitespace test stopped matching and
    // spaces started coming out as punctuation tokens.
    //
    // Any future alternative that is a handle to something with value semantics
    // needs an arm here too. The fallback below is correct only for
    // alternatives whose own operator== already means what the language means.
    if (const SatString *sa = as_string(a))
        return *sa == *as_string(b);

    // §21, and exactly the case the paragraph above warns about: a Bits is
    // behind a shared_ptr, so the variant's own operator== would compare
    // pointers and `x00FF == x00FF` would be false whenever the two sides were
    // written separately.
    //
    // Equal means SAME RADIX AND SAME DIGITS, so `x0009` and `x9` are not
    // equal. That follows from the width being part of the value rather than
    // being a separate decision: if they compared equal, one of them would have
    // to be a valid substitute for the other, and it is not — they pack to a
    // different number of bytes and arrive off a socket as different values.
    // A program that wants the numeric comparison asks for it: a.to_number()
    // .equals(b.to_number()) says which question is being asked.
    if (const Bits *ba = as_bits(a)) {
        const Bits *bb = as_bits(b);
        return bb && ba->radix == bb->radix && ba->digits == bb->digits;
    }

    // A map is exactly the shape the comment above warns about: a handle to
    // something with value semantics, whose fallback would compare MapRefs and
    // therefore pointers. Two independently built maps holding the same entries
    // must be equal.
    //
    // Order-INSENSITIVE, which is deliberate and is the one place a map's
    // insertion order does not count. Order is how a map is PRINTED and WALKED,
    // because those need to be deterministic; it is not part of what a map IS.
    // Two symbol tables that disagree only about which name was seen first hold
    // the same symbols.
    //
    // Looked up through b's index, so this is O(n) rather than O(n^2).
    if (const MapBody *ma = as_map(a)) {
        const MapBody *mb = as_map(b);
        if (ma->entries.size() != mb->entries.size())
            return false;
        for (const MapEntry &entry : ma->entries) {
            std::string key;
            if (!entry.key || !map_key_of(*entry.key, key))
                return false;
            auto found = mb->index.find(key);
            if (found == mb->index.end())
                return false;
            const ValuePtr &other = mb->entries[found->second].value;
            if (!entry.value || !other) {
                if (static_cast<bool>(entry.value) != static_cast<bool>(other))
                    return false;
                continue;
            }
            if (!value_equals(*entry.value, *other))
                return false;
        }
        return true;
    }

    return static_cast<const ValueBase &>(a) == static_cast<const ValueBase &>(b);
}

} // namespace satellite
