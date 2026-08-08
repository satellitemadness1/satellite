#pragma once

#include "bignum.hpp"
#include "satellite_string.hpp"

#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <variant>
#include <vector>

namespace satellite {

struct Value;

// env.hpp. Everything a spacesuit knows about itself — its field layout, its
// methods, its superclass — computed once by resolve() and shared by every
// instance, which is why an Object carries a pointer to it rather than a copy.
struct SpacesuitInfo;

// Snapshot handle used everywhere. The pointed-to Value is immutable, so
// a reader can keep using it while writers publish newer values, and a
// container can share children without copying them.
using ValuePtr = std::shared_ptr<const Value>;

// satellite.container.list<...>: containers hold pointers to other Values,
// so they can nest arbitrarily (lists of lists of ...).
using List = std::vector<ValuePtr>;

// What the variant actually stores for a string and a list: a refcounted
// handle, not the container itself.
//
// §17's value model, one step down. The measured cost of the tree walk is a
// heap allocation per intermediate value — 28 ns of the 112.5 ns an addition
// costs — and removing it means passing a Value BY VALUE rather than behind a
// shared_ptr. That is only an improvement if copying a Value is cheap, and with
// a u32string and a vector stored inline it is not: copying would deep-copy the
// character buffer, so arithmetic would get faster and strings would get much
// slower.
//
// Behind a handle, copying a Value is a tag check and at most one refcount
// bump, never an allocation. The alternatives keep their POSITIONS in the
// variant — only their types change — because help_for() and module_of()
// switch on raw variant indices.
using Str     = std::shared_ptr<const SatString>;
using ListRef = std::shared_ptr<const List>;

// satellite.variable.time — an absolute instant, nanoseconds since the Unix
// epoch, UTC.
//
// int64_t rather than double, and §8.2 verified why: epoch nanoseconds now is
// 1785988800000000000, which needs 61 bits. A double's 53-bit mantissa gives
// 198 ns of resolution at the current epoch, so satellite.time.now() called
// twice in quick succession returns the identical value — which makes it
// useless for the one thing a clock is for.
//
// An INSTANT only, per §8.2: no timezone, no duration, no instant/duration mode
// flag. a.minus(b) is a plain number of nanoseconds, and adding
// satellite.variable.duration later only adds overloads.
struct Time {
    long long ns = 0;
    bool operator==(const Time &) const = default;
};

// An instance of a satellite.spacesuit.
//
// Objects are a REFERENCE type, and §8.3 already decided that shape for `file`
// and `window`: the immutable-Value contract survives because it was never a
// claim about what a Value names. The ObjectPtr inside the Value never changes
// after construction, so the Value node's bytes stay immutable and the
// lock-free snapshot protocol is untouched. What is mutable is the object.
//
// The alternative — value semantics, copy on assignment — was rejected because
// it does not survive the first method. `my_object.my_func()` mutating a field
// would have to write back through the receiver's storage slot the way
// `.append()` does, and that works only while the receiver names a slot: an
// object inside a list, or passed as an argument, has nowhere to write back to.
// Reference semantics is what "class" means everywhere else, and it is the only
// reading under which the design's own example does what it looks like it does.
//
// The cost is stated honestly in §12: an object CAN close a shared_ptr cycle,
// which no previous value could, so cycles leak.
struct Object {
    // Never null once construction finishes. Owned by the ResolveResult, which
    // outlives every Evaluator run over the same Program.
    const SpacesuitInfo *suit = nullptr;

    // Fields are INDEXED, never searched, exactly as a Frame's slots are.
    // resolve() assigns the indices, laying a superclass's fields down first,
    // so an inherited method finds its own field at the same index in every
    // descendant's instance — which is what makes inheritance cost nothing at
    // the field access itself.
    //
    // The concurrency protocol is satellite.library's, one level down: readers
    // load a slot atomically and never lock; a writer takes write_lock and
    // publishes with one atomic store. Same protocol because it answers the
    // same question — an object reachable from a global is reachable from every
    // thread that can see the global, and a Frame's freedom from locks came
    // from being reachable from exactly one thread (§6), which an object is not.
    //
    // A fixed array rather than a vector because the count is known at
    // construction and never changes: a vector of std::atomic could not
    // reallocate anyway, so the array says so.
    std::mutex write_lock;
    std::unique_ptr<std::atomic<ValuePtr>[]> fields;
    size_t field_count = 0;

    explicit Object(size_t count)
        : fields(count ? new std::atomic<ValuePtr>[count] : nullptr),
          field_count(count)
    {
    }

    ValuePtr field(size_t i) const { return fields[i].load(); }
};

using ObjectPtr = std::shared_ptr<Object>;

// satellite.variable.file — an open file description, §8.3.
//
// A REFERENCE type, and §8.3 was explicit that this does not weaken the
// immutable-Value contract: the shared_ptr never changes after construction, so
// the Value node's bytes stay immutable and the lock-free snapshot protocol is
// untouched. What is mutable is the OS file description behind it.
//
//     satellite has value semantics for values, and reference semantics for the
//     external resources that file and window name.
//
// True value semantics is not achievable and §8.3 said why: dup() shares the
// file offset, so independent offsets require re-open() by path, which fails
// for pipes, sockets and unlinked files.
//
// The descriptor is an atomic because close() can race a read from another
// snapshot of the same value, and the loser has to get a clean language-level
// error rather than a descriptor that has been reused by the OS for something
// else. After a close, other snapshots see a closed handle; nothing ever
// silently reopens.
struct FileHandle {
    std::atomic<int> fd{-1};
    std::atomic<int> last_error{0};    // errno of the most recent failure
    std::string path;
    bool writable = false;

    // RAII stays as the backstop, but it is NOT the interface: §8.3 requires an
    // explicit close() returning a status, because a destructor cannot report
    // that close failed with ENOSPC or EIO and buffered writes commit at close.
    ~FileHandle();
};

using FilePtr = std::shared_ptr<FileHandle>;

// The name of a spacesuit, out of line because SpacesuitInfo is defined in
// env.hpp and value.hpp sits below it in the include order. Defined in env.cpp.
const std::string &suit_name(const SpacesuitInfo *suit);

// `double` is deliberately absent, per §8.1: satellite.variable.number is an
// exact decimal (bignum.hpp), so there is no binary float anywhere in the
// language's value space and no way for one to leak in. Number's deleted float
// constructors are what enforce that at the C++ level — `Value v = 3.14` is a
// compile error rather than the silent truncation to 3 that §8.1 measured.
using ValueBase = std::variant<std::monostate, bool, Number, Str, ListRef,
                               ObjectPtr, Time, FilePtr>;

// One node that can hold anything the language has so far:
//   satellite.variable.bool / .number       -> the scalar alternatives
//   satellite.variable.string               -> SatString (32-bit satellite chars)
//   satellite.container.list                -> the List alternative
// This is the seed of the "satellite_expression" idea — when the parser
// exists, expression node kinds (call, assign, ...) join this same variant.
struct Value : ValueBase {
    using ValueBase::ValueBase;
};

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

// 40 bytes is the figure §8.1 quotes when it records that removing `double`
// for a 32-byte Number left sizeof(Value) unchanged, and the one §10 quotes
// when it refuses to fold Expr into Value — that fold takes every Value to 96,
// and a million-element number list from 38 MB to 91 MB. Both claims were
// checked by reading a printed banner line, which is not a guard. This is.
// Guarded on 64-bit for the reason given at Span in ast.hpp: the Debian
// packages are Architecture: any and 32-bit layouts differ legitimately.
static_assert(sizeof(void *) != 8 || sizeof(Value) == 40,
              "Value must stay 40 bytes on 64-bit — §8.1's migration and §10's "
              "Expr/Value split both rest on this number");

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

} // namespace satellite
