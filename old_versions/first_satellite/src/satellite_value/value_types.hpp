#pragma once

#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <atomic>
#include <cstdio>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
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
// a u16string and a vector stored inline it is not: copying would deep-copy the
// character buffer, so arithmetic would get faster and strings would get much
// slower.
//
// Behind a handle, copying a Value is a tag check and at most one refcount
// bump, never an allocation. The alternatives keep their POSITIONS in the
// variant — only their types change — because help_for() and module_of()
// switch on raw variant indices.
using Str     = std::shared_ptr<const SatString>;
using ListRef = std::shared_ptr<const List>;

// satellite.container.map<K, V>. One entry, in insertion order.
struct MapEntry {
    ValuePtr key;
    ValuePtr value;
};

// Insertion-ordered, with a side index for O(1) lookup by canonical key.
//
// ORDERED, and that is not a taste decision. §15's complaint is that a symbol
// table built on `list` is a linear scan; the index answers that. The ORDER
// answers a different requirement: `.keys()` is how a map is walked, since §5's
// only loop is the C-shaped `for`, and an unordered map would make every
// program that walks one non-deterministic. §15's stage 3 self-compiles to a
// byte-identical fixpoint, and a fixpoint test over unstable iteration order is
// unfalsifiable.
//
// BUILT, THEN FROZEN. A MapBody is fully populated before make_shared and is
// never written afterwards, which is what keeps the immutability contract above
// and the lock-free publish protocol intact — the same argument §8.3 makes for
// `file`. A mutable member, a const_cast, a lazily-built index or a memoised
// lookup cache would each be a data race, and NOTHING in the test suite would
// catch it: library_test drives the Library from C++ and never runs satellite
// code, so the evaluator's write paths have no ThreadSanitizer coverage.
//
// The key of `index` is the canonical byte string from map_key_of (src/evaluator/maps.cpp),
// never the decoded text of a key — see §8.6, and the reason is that §8.5's
// \home and \cwd expand at decode() time and would make a key's identity depend
// on the current directory.
struct MapBody {
    std::vector<MapEntry> entries;
    std::unordered_map<std::string, size_t> index;
};
using MapRef = std::shared_ptr<const MapBody>;

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

// satellite.variable.binary and satellite.variable.hex — §21.
//
// TWO TYPE NAMES, ONE ALTERNATIVE, and the radix is what tells them apart.
// matches() (src/evaluator/types.cpp) reads `radix` and answers `binary` for 2
// and `hex` for 16, so the two are exact-name-distinct at the surface exactly
// as the user asked, while the variant grows by one and not by two. §8.7's
// "a word means one thing" is not strained by this: `binary` and `hex` answer
// the SAME questions (.digits(), .bytes(), .to_number()) and differ only in the
// base their digits are written in, which is the same relation `fast`, `normal`
// and `ultra` have in §18 — three names, one Number.
//
// THE DIGITS ARE STORED AS WRITTEN, one char per digit, and that is the whole
// reason this is not a Number. §8.1 makes satellite.variable.number an exact
// decimal, and exact does not mean wide: Number::parse("0009") is 9, and 9 is
// what to_string gives back. The leading zeros are gone, because to a number
// they were never there. `x0009999CCC` has a WIDTH, and a value whose width is
// discarded on the way in cannot come back off a socket the way it went on —
// which is precisely what a hex constant is usually for.
//
// So: 22 digits in, 22 digits out. `.to_number()` is the lossy direction and
// the program has to ask for it by name.
//
// Hex digits are normalised to UPPER CASE at construction, so `x00ff` and
// `x00FF` are one value and `==` does not depend on how it was typed. That is a
// normalisation of SPELLING, not of width — the count of digits is untouched —
// and it is the same call §8.6 makes for map keys, one level down.
struct Bits {
    // 2 or 16. Nothing else is constructible: the lexer only produces these two
    // prefixes, so a third radix cannot arrive without a language change.
    unsigned radix = 16;

    // '0'..'9', 'A'..'F'. Never empty — a literal needs at least one digit, and
    // the lexer will not produce a prefix with nothing after it.
    std::string digits;

    bool operator==(const Bits &) const = default;
};

using BitsRef = std::shared_ptr<const Bits>;

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
//
// "NEVER SILENTLY REOPEN" IS A RULE ABOUT WHAT THE RUNTIME DOES BEHIND THE
// PROGRAM'S BACK, and .open() is what made that worth spelling out. §8.3's
// sentence is the answer to "what happens when a snapshot uses a closed
// handle": it gets an error, and the runtime does not quietly hand it a fresh
// descriptor. .read(), .write() and .clear() honour that literally — every one
// of them still refuses a closed fd rather than reopening it. What the sentence
// never governed is a reopen the program WRITES DOWN, which is the opposite of
// silent: `f.open()` is a line of source with a status the caller can read.
struct FileHandle {
    std::atomic<int> fd{-1};
    std::atomic<int> last_error{0};    // errno of the most recent failure

    // Written once, before the handle is published into a Value, and read-only
    // afterwards — the same contract `path` has always had, which is why none
    // of these needs to be an atomic while `fd` does. `fd` is the one thing
    // close() and open() move after publication.
    std::string path;

    // WHAT THE HANDLE MAY DO, decided by the mode word at open time.
    //
    // `writable` has been here since §8.3.1 and NOTHING read it: set in
    // satellite.file.open, consulted nowhere, so it recorded an intention
    // instead of enforcing one. It is live now, and `readable` joins it,
    // because "read_append" is what made the mistake they guard reachable.
    // While every mode was one-directional, asking a handle for the wrong
    // direction produced EBADF, and "Bad file descriptor" is the wrong sentence
    // about a descriptor that is perfectly good. The direction is checked
    // before the syscall now, and the message names the mode that would have
    // worked.
    bool readable = false;
    bool writable = false;

    // What .open() reopens with, which is NOT always what the handle was
    // created with. satellite.file.new adds O_EXCL so that creating a file that
    // already exists is a refusal rather than a silent clobber; replaying
    // O_EXCL on a reopen would then fail with EEXIST on the very file the
    // program just made. The flags describing the ACCESS are stored here; the
    // flags describing the CREATION are used once and dropped.
    int reopen_flags = 0;

    // RAII stays as the backstop, but it is NOT the interface: §8.3 requires an
    // explicit close() returning a status, because a destructor cannot report
    // that close failed with ENOSPC or EIO and buffered writes commit at close.
    ~FileHandle();
};

using FilePtr = std::shared_ptr<FileHandle>;

// The name of a spacesuit, out of line because SpacesuitInfo is defined in
// env.hpp and value.hpp sits below it in the include order. Defined in env.cpp.
const std::string &suit_name(const SpacesuitInfo *suit);

} // namespace satellite
