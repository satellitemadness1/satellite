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

// --- §21's operations on a binary or hex value, defined in bits.cpp ---------
//
// Free functions rather than members of Bits, for the reason every other
// operation in this header is a free function: a Bits inside a Value is behind
// a shared_ptr<const>, so nothing may mutate one, and a method that cannot
// mutate is a function that takes the value.

// Is `c` a digit in this radix? Hex accepts either case; `bits_normalise` is
// what makes the two one value.
bool bits_valid_digit(unsigned radix, char c);

// 0..15, or -1 for anything that is not a hex digit.
int bits_digit_value(char c);

// Upper-cases hex digits. The digit COUNT is never changed.
std::string bits_normalise(unsigned radix, std::string digits);

// 1 for binary, 4 for hex.
unsigned bits_per_digit(unsigned radix);

// What .bytes() answers: the packed size, rounding up to a whole byte.
size_t bits_byte_count(const Bits &bits);

// Exact base conversion, both directions, in decimal string arithmetic. See
// bits.cpp for why this is not done with Number::divide.
std::string bits_to_decimal(unsigned radix, const std::string &digits);
std::string bits_from_decimal(unsigned radix, const std::string &decimal,
                              size_t min_digits);

// Value-preserving. Exact in width for hex -> binary; hex is the direction that
// rounds a width up, and bits.cpp says so at the site.
Bits bits_convert(const Bits &from, unsigned to_radix);

// The packed bytes, big-endian and left-padded to a whole byte. Packing LOSES
// the digit count — `x0F` and `x000F` pack alike — so bits_unpack takes the
// count back, and §20.3 sends it beside the bytes.
std::string bits_pack(const Bits &bits);
Bits bits_unpack(unsigned radix, const std::string &bytes, size_t digit_count);

// The name of a spacesuit, out of line because SpacesuitInfo is defined in
// env.hpp and value.hpp sits below it in the include order. Defined in env.cpp.
const std::string &suit_name(const SpacesuitInfo *suit);

// The value satellite.main is handed: the command line, and then everything
// the runtime can truthfully say about the machine it is running on.
//
// A LIST OF STRINGS THAT KNOWS WHAT EACH STRING IS. matches() accepts one
// wherever satellite.container.list<satellite.variable.string> is declared, so
// §2's signature does not move and hello world keeps its five lines; what it
// adds is that every element has a NAME, and that names past the command line
// carry facts (argv, cwd, the user, the OS, the compiler that built satl)
// which a program would otherwise have no way to ask for at all.
//
// .length() AND NUMERIC [i] COVER THE COMMAND LINE AND NOTHING ELSE. That is
// the whole reason `command_line_count` is stored rather than derived. Every
// program that takes arguments writes `for (i = 1; i < args.length(); i++)`,
// and if .length() counted the environment entries that loop would walk off
// the user's arguments and start reading the kernel release as though it had
// been typed. The extra entries are reached by NAME -- args.cxx_compiler,
// args["cxx_compiler"], args.get("cxx_compiler") -- and .count() is the total
// for anyone who wants it.
//
// BUILT, THEN FROZEN, exactly as MapBody is and for the same reason: an
// Arguments is fully populated before make_shared and never written afterwards,
// which is what keeps the immutability contract and the lock-free publish
// protocol intact. The index is built eagerly for the same reason MapBody's is
// -- a lazily built one is a data race nothing in the test suite would catch.
struct ArgumentEntry {
    // ASCII, lower case, underscores. Chosen to be disjoint from every method
    // name a list or an Arguments answers to, because a method wins over an
    // entry and a colliding name would be unreachable rather than ambiguous.
    std::string name;

    // Always a string Value. An Arguments is a list<string> to the type system
    // and this is what makes that true rather than nearly true.
    ValuePtr value;
};

struct Arguments {
    // In display order: the command line first, in argv order, then the
    // environment entries in the order §8's table lists them.
    std::vector<ArgumentEntry> entries;

    // name -> position in `entries`. Eager, see above.
    std::unordered_map<std::string, size_t> index;

    // How many leading entries came from (argc, argv), argv[0] included. This
    // is what .length() answers and what [i] is bounded by.
    size_t command_line_count = 0;
};
using ArgsRef = std::shared_ptr<const Arguments>;

// `double` is deliberately absent, per §8.1: satellite.variable.number is an
// exact decimal (bignum.hpp), so there is no binary float anywhere in the
// language's value space and no way for one to leak in. Number's deleted float
// constructors are what enforce that at the C++ level — `Value v = 3.14` is a
// compile error rather than the silent truncation to 3 that §8.1 measured.
//
// BitsRef is index 9 and was appended for exactly the reason below, not because
// binary and hex belong after a map. §21. ArgsRef is index 10 and was appended
// for the same reason -- it is not that the arguments object belongs after a
// hex literal, it is that there is nowhere else it may go.
//
// APPEND ONLY. MapRef is index 8 because it was added last, not because a map
// belongs at the end: help_for() and module_of() switch on RAW variant indices,
// so inserting an alternative renumbers every alternative after it and silently
// changes what every one of those switches means.
//
// A shared_ptr is 16 bytes and the variant already holds five of them, so
// appending ArgsRef leaves sizeof(Value) at 40 and the static_assert below
// keeps holding. An alternative that grew it would be refused on that ground
// alone: §8.1's Number migration and §10's Expr/Value split both rest on 40.
using ValueBase = std::variant<std::monostate, bool, Number, Str, ListRef,
                               ObjectPtr, Time, FilePtr, MapRef, BitsRef,
                               ArgsRef>;

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
// Takes the body BY VALUE and freezes it: after this returns, the MapBody is
// const and shared, and the only way to "change" a map is to build a new body
// and publish it through the storage slot. See MapBody above for why.
inline Value make_map(MapBody body)
{
    return Value(std::make_shared<const MapBody>(std::move(body)));
}

// Build a binary or hex value. Takes the digits already validated and, for hex,
// already upper-cased — normalisation belongs to whoever parsed the digits, so
// that this stays the one cheap way to make one.
inline Value make_bits(unsigned radix, std::string digits)
{
    return Value(std::make_shared<const Bits>(Bits{radix, std::move(digits)}));
}

// Takes the body BY VALUE and freezes it, the same contract make_map has. The
// caller (arguments_for, in system.cpp) is the only place that builds one, and
// it fills `entries`, `index` and `command_line_count` before calling this.
inline Value make_arguments(Arguments body)
{
    return Value(std::make_shared<const Arguments>(std::move(body)));
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
inline const MapBody *as_map(const Value &v)
{
    const MapRef *p = std::get_if<MapRef>(&v);
    return p ? p->get() : nullptr;
}
inline const Bits *as_bits(const Value &v)
{
    const BitsRef *p = std::get_if<BitsRef>(&v);
    return p ? p->get() : nullptr;
}
inline const Arguments *as_arguments(const Value &v)
{
    const ArgsRef *p = std::get_if<ArgsRef>(&v);
    return p ? p->get() : nullptr;
}

// The command-line half of an Arguments, as a plain List, so that every place
// which already knows what to do with a list<string> can be handed one without
// learning a second shape. Copies the handles, never the strings.
//
// This is what makes DECISION 4 cheap: .length(), [i], slicing, .first(),
// .last() and .contains() all run over THIS, so they keep answering exactly
// what they answered before the arguments object existed.
inline List arguments_command_line(const Arguments &args)
{
    List out;
    out.reserve(args.command_line_count);
    for (size_t i = 0; i < args.command_line_count && i < args.entries.size(); i++)
        out.push_back(args.entries[i].value);
    return out;
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

// ---------------------------------------------------------------------------
// §8.7's memory model — the three numbers `.size()` is made of
// ---------------------------------------------------------------------------
//
// These are the 64-bit layout FROZEN AS THE DEFINITION, which is the same move
// the static_assert above already makes for sizeof(Value). `.size()` has to
// answer identically on every platform the language is built for: the Debian
// packages are Architecture: any, and §15's stage 3 self-compiles to a
// byte-identical fixpoint — a size that changed with the pointer width would
// make any program that prints one unportable, and any test that asserts one
// unfalsifiable on half the build matrix.
//
// So the model bills three things and nothing else:
//
//   a NODE     40 bytes, once per Value reached
//   a HANDLE   16 bytes, once per slot that points at a Value
//   CONTENT    2 per character, 4 per limb, 1 per byte of a path or a map key
//
// What it does NOT bill is the ALLOCATOR: make_shared control blocks, vector
// capacity beyond its size, hash-table bucket arrays, malloc's rounding. Those
// are real bytes, and leaving them out is the deliberate half of the design —
// they are properties of the C++ runtime a build happened to use, not of the
// program's data, and a `.size()` that moved when libstdc++ changed its
// growth factor would be reporting on the wrong thing.
inline constexpr size_t VALUE_NODE_BYTES = 40;
inline constexpr size_t VALUE_HANDLE_BYTES = 16;
inline constexpr size_t VALUE_CHAR_BYTES = 2;

// The model was DERIVED from the layout, so on the platform it was derived on
// it must still match it. Guarded on 64-bit for the reason the assert above is:
// a 32-bit layout differs legitimately, and there the constants are the
// definition doing its job rather than a layout claim gone stale.
static_assert(sizeof(void *) != 8 ||
                  (VALUE_NODE_BYTES == sizeof(Value) &&
                   VALUE_HANDLE_BYTES == sizeof(ValuePtr) &&
                   VALUE_CHAR_BYTES == sizeof(SatChar)),
              "§8.7's constants have drifted from the layout they model");

// Bytes a value occupies, per §8.7: a node plus what it owns, followed all the
// way down into a list's elements, a map's entries and an object's fields.
//
// Takes the HANDLE rather than the Value, and that is not a convenience. The
// walk dedupes on pointer identity — shared storage is counted once, and §12's
// cyclic object graph terminates — so the TOP node has to enter the set like
// every other one. Passing a bare Value would leave it out, and a value that
// reaches itself would then be counted twice before the cycle closed.
size_t value_bytes(const ValuePtr &value);

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
    std::string operator()(const MapRef &map) const;

    // Prints WITH the prefix — `x00FF`, `b1010` — so that what a program
    // displays is what a program may type back in. §8.1.1's rule for a number
    // is "print the value, never N significant digits"; the equivalent for a
    // value whose width is load-bearing is to print every digit it has.
    std::string operator()(const BitsRef &bits) const;

    // One entry per line, `name` padded to the widest, then the value. NOT the
    // one-line `{k: v}` a map uses: there are about thirty entries and one of
    // them is a compiler version string, so a single line is unreadable in the
    // only place it is ever printed. `.lines()` on a list already established
    // that "one per line, aligned" is a form this language has.
    std::string operator()(const ArgsRef &args) const;
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

// `{key: value, key: value}`, in insertion order, and `{}` when empty. Braces
// rather than brackets so a map and a list are distinguishable at a glance in
// REPL output and in an error message.
//
// The order is what makes this testable: eval_test's check_output is exact
// string equality, so a map that rendered in hash order would make every test
// that prints one intermittently red.
inline std::string ValuePrinter::operator()(const MapRef &map) const
{
    if (!map || map->entries.empty())
        return "{}";
    std::string out = "{";
    for (size_t i = 0; i < map->entries.size(); i++) {
        if (i)
            out += ", ";
        const MapEntry &e = map->entries[i];
        out += e.key ? to_string(*e.key) : "nil";
        out += ": ";
        out += e.value ? to_string(*e.value) : "nil";
    }
    return out + "}";
}

inline std::string ValuePrinter::operator()(const ArgsRef &args) const
{
    if (!args || args->entries.empty())
        return "";

    size_t width = 0;
    for (const ArgumentEntry &e : args->entries)
        if (e.name.size() > width)
            width = e.name.size();

    std::string out;
    for (size_t i = 0; i < args->entries.size(); i++) {
        const ArgumentEntry &e = args->entries[i];
        if (i)
            out += "\n";
        out += e.name;
        out.append(width - e.name.size() + 2, ' ');
        out += e.value ? to_string(*e.value) : "";
    }
    return out;
}

} // namespace satellite
