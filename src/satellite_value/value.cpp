#include "satellite_value/value.hpp"

// For the complete SpacesuitInfo, which value.hpp only forward-declares. This
// include is possible at all because env.hpp no longer includes value.hpp: the
// two headers used to be mutually dependent, and now the dependency runs one
// way — values know about resolver types, and the resolver knows nothing about
// values. That is what lets the compiler share src/environment/ without dragging the boxed
// value model in with it.
#include "environment/env.hpp"

#include <cstdio>
#include <ctime>
#include <unordered_set>

#include <unistd.h>

namespace satellite {

// §8.1's migration moved every question about how a number RENDERS into
// Number::to_string (bignum.cpp), because the answer is a property of the type
// rather than of the printer. The history is worth keeping in one sentence: the
// original snprintf("%g") printed six significant digits, so 123456789 came out
// as 1.23457e+08 and 2000000 + 1 as 2e+06 — wrong values, through the only
// output path the language has, since satellite.console.display, the REPL echo,
// .to_string() and every error message quoting a number all arrive here.

// Declared in value.hpp, which knows SpacesuitInfo only as a forward
// declaration. The fallback is not decoration: to_string() prints a Value from
// anywhere, including from an error message about an object whose construction
// did not finish.
//
// It lived in src/environment/run.cpp until env.hpp stopped including value.hpp. It is a
// function about how a Value RENDERS that happens to read a resolver type, so
// this is where it belongs, and src/environment/ is now free of the value model entirely.
const std::string &suit_name(const SpacesuitInfo *suit)
{
    static const std::string unknown = "spacesuit";
    return suit ? suit->name : unknown;
}

// How an instance renders: the spacesuit's name in angle brackets.
//
// Angle brackets because the result is deliberately not something a program
// could have written — there is no object literal, so anything that looked like
// one would mislead. What it is NOT is a dump of the fields: a field is
// reachable only from inside its spacesuit (§12 keeps bare field access out of
// the language), so printing them would hand out through the printer exactly
// what `satellite.protected` exists to withhold. A cyclic object graph would
// also not terminate, and objects are the first values that can build one.
//
// A spacesuit that wants a readable form writes `.to_string()`, which is an
// ordinary public method and takes precedence at the call site.
std::string ValuePrinter::operator()(const ObjectPtr &object) const
{
    return "<" + suit_name(object ? object->suit : nullptr) + ">";
}

// How a satellite.variable.time renders: ISO-8601, UTC, to the nanosecond.
//
// All nine fractional digits, always, rather than trimming trailing zeros: the
// value carries nanosecond resolution and a printed instant that silently
// changed width would be unsortable as text — which is most of what an ISO-8601
// timestamp is for. §8.2 fixes the type as an absolute instant in UTC, so the
// 'Z' is a fact about the type and never a guess about a timezone.
std::string ValuePrinter::operator()(Time time) const
{
    // Floor division, not truncation: C++ integer division rounds toward zero,
    // so a pre-epoch instant would otherwise land one second late with a
    // negative fraction. Instants before 1970 are rare and being quietly wrong
    // about them is not better than being right.
    long long seconds = time.ns / 1000000000;
    long long fraction = time.ns % 1000000000;
    if (fraction < 0) {
        fraction += 1000000000;
        seconds -= 1;
    }

    const std::time_t as_time = static_cast<std::time_t>(seconds);
    std::tm broken{};
    if (!gmtime_r(&as_time, &broken)) {
        // Outside what the C library can break down. The raw count is still the
        // value, so say it rather than nothing.
        char raw[40];
        snprintf(raw, sizeof raw, "%lldns", time.ns);
        return raw;
    }

    char buffer[64];
    snprintf(buffer, sizeof buffer, "%04d-%02d-%02dT%02d:%02d:%02d.%09lldZ",
             broken.tm_year + 1900, broken.tm_mon + 1, broken.tm_mday,
             broken.tm_hour, broken.tm_min, broken.tm_sec, fraction);
    return buffer;
}

// The backstop, and only the backstop. A program that cares whether its bytes
// reached the disk calls close() and reads the status; this is what catches the
// one that forgot. Nothing is reported from here because there is nobody left
// to report it to.
FileHandle::~FileHandle()
{
    const int open_fd = fd.exchange(-1);
    if (open_fd >= 0)
        ::close(open_fd);
}

// How a file renders. The path and whether it is still open, because those are
// the two things a person debugging one wants, and neither is the contents.
std::string ValuePrinter::operator()(const FilePtr &file) const
{
    if (!file)
        return "<file>";
    const bool open = file->fd.load() >= 0;
    return "<file " + file->path + (open ? "" : ", closed") + ">";
}


// `x00FF`, `b1010` — the prefix and every digit, including the leading zeros.
//
// The prefix is printed because it is what makes the output re-readable: a
// program that displays a hex value and a person who types the result back in
// must get the same value, and `00FF` alone would lex as a Number and then a
// Word. This is §8.1.1's rule ("print the value, never N significant digits")
// applied to a value whose width is part of it — every digit it has, no more
// and no fewer.
std::string ValuePrinter::operator()(const BitsRef &bits) const
{
    if (!bits)
        return "<bits>";
    return std::string(bits->radix == 2 ? "b" : "x") + bits->digits;
}


// ---------------------------------------------------------------------------
// .size() — §8.7's memory model, walked
// ---------------------------------------------------------------------------

namespace {

// One walk over one value, carrying the set of storage already counted.
//
// That set does two jobs with one mechanism, and both were requirements:
//
//   * SHARED STORAGE COUNTS ONCE. `list<...> b = a` shares one buffer, and a
//     list holding the same sublist twice holds one sublist. A model that
//     billed it twice would answer a question nobody asked — the bytes that
//     WOULD be freed if this value went away is the useful number, and it is
//     not the sum of the parts.
//
//   * A CYCLE TERMINATES. §12 records the cost of making objects a reference
//     type in one sentence: an object CAN close a shared_ptr cycle, which no
//     previous value could. to_string() sidesteps that by refusing to print an
//     object's fields at all; .size() has to walk them, so it needs the set.
//
// Depth is NOT capped, and that is a deliberate match to the precedent rather
// than an oversight: ValuePrinter recurses over nested lists with no cap
// either, so a structure deep enough to break this walk already breaks every
// print of the same value. The cycle — the one shape that would never
// terminate at any depth — is handled above.
struct SizeWalk {
    std::unordered_set<const void *> seen;

    // True the first time a pointer is offered, false on every later offer.
    // A null handle is never "new": it owns nothing, and admitting it would
    // make the FIRST nil in a structure special and every later one free.
    bool first_time(const void *p) { return p && seen.insert(p).second; }

    size_t of(const Value &v);
    size_t of_child(const ValuePtr &child)
    {
        return first_time(child.get()) ? of(*child) : 0;
    }
};

// One operator() per alternative and no generic `auto` fallback, for exactly
// the reason ValuePrinter states: a fallback arm compiles clean when a tenth
// alternative lands and then silently bills it as zero bytes. Exhaustiveness
// here means adding to ValueBase fails to compile until .size() has an answer.
struct SizeVisitor {
    SizeWalk &walk;

    // The node is billed by of(); these report only what the value OWNS.
    size_t operator()(std::monostate) const { return 0; }
    size_t operator()(bool) const { return 0; }

    // The small form owns nothing — a significand and an exponent live in the
    // node, which is what makes it the fast path. A boxed magnitude is billed
    // by its limbs, and deduped, because two numbers copied from one another
    // share one BigInt.
    size_t operator()(const Number &n) const
    {
        return walk.first_time(n.payload_id()) ? n.payload_bytes() : 0;
    }

    // §8.5: a character is 16 bits. So a string's bytes are exactly twice its
    // length() on every string that can exist, which is worth knowing before
    // reaching for .size() to count UTF-8 — satellite text is not UTF-8, and a
    // byte the code table has no entry for is one raw-area character, not part
    // of a multi-byte one.
    size_t operator()(const Str &s) const
    {
        return walk.first_time(s.get()) ? s->size() * VALUE_CHAR_BYTES : 0;
    }

    size_t operator()(const ListRef &list) const
    {
        if (!walk.first_time(list.get()))
            return 0;
        size_t total = list->size() * VALUE_HANDLE_BYTES;
        for (const ValuePtr &item : *list)
            total += walk.of_child(item);
        return total;
    }

    size_t operator()(const ObjectPtr &object) const
    {
        if (!walk.first_time(object.get()))
            return 0;
        // Through field(), which loads atomically. §14's protocol is that a
        // reader never locks, and a size walk is a reader like any other —
        // reaching into the array directly would be a data race against a
        // method mutating the same object from another thread.
        size_t total = object->field_count * VALUE_HANDLE_BYTES;
        for (size_t i = 0; i < object->field_count; i++)
            total += walk.of_child(object->field(i));
        return total;
    }

    // Eight bytes of nanoseconds, in the node.
    size_t operator()(Time) const { return 0; }

    // The path, and nothing else. §8.3 made `file` a REFERENCE type because
    // what it names is an OS file description; those bytes are the kernel's,
    // and billing a program for them would be reporting on the wrong thing.
    size_t operator()(const FilePtr &file) const
    {
        return walk.first_time(file.get()) ? file->path.size() : 0;
    }

    // One byte per digit, which is what the digits actually occupy — §8.7
    // bills "1 per byte of a path or a map key" and this is the same kind of
    // thing. It is deliberately NOT the number of bytes the value would occupy
    // packed: a hex digit is four bits of information stored in a whole char,
    // and .size() reports what a value COSTS, not what it could be compressed
    // to. `.bytes()` is the method that answers the other question.
    size_t operator()(const BitsRef &bits) const
    {
        return walk.first_time(bits.get()) ? bits->digits.size() : 0;
    }

    size_t operator()(const MapRef &map) const
    {
        if (!walk.first_time(map.get()))
            return 0;
        size_t total = map->entries.size() * 2 * VALUE_HANDLE_BYTES;
        for (const MapEntry &entry : map->entries) {
            total += walk.of_child(entry.key);
            total += walk.of_child(entry.value);
        }
        // The side index is content, not overhead, and §8.6 is why it is
        // MEASURED rather than inferred from the keys: it is keyed by the
        // canonical bytes, which are not the key's characters — \home and \cwd
        // expand at decode() time, so a key's text and a key's identity are
        // different lengths. Recomputing would get this wrong for exactly the
        // keys §8.6 was written about.
        for (const auto &slot : map->index)
            total += slot.first.size() + sizeof(size_t);
        return total;
    }
};

size_t SizeWalk::of(const Value &v)
{
    // Cast to the base for the reason to_string gives: std::variant_size is
    // not specialised for Value, which merely inherits from ValueBase.
    return VALUE_NODE_BYTES +
           std::visit(SizeVisitor{*this}, static_cast<const ValueBase &>(v));
}

} // namespace

size_t value_bytes(const ValuePtr &value)
{
    // A null handle is not a value that costs 40 bytes; it is the absence of
    // one. Whatever slot holds it has already been billed for the handle.
    if (!value)
        return 0;
    SizeWalk walk;
    walk.first_time(value.get());
    return walk.of(*value);
}

} // namespace satellite
