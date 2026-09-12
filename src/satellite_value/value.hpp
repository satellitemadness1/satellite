#pragma once

// What a satellite expression evaluates to -- PLAN M9, and M10's fifth arm.
// DESIGN §8 is the table of types and §8.2 is the one sentence this file is
// built around.
//
// FORTY BYTES, AND THE NUMBER IS A BUDGET RATHER THAN A RESULT. DESIGN §8.2
// asks that "small exact integers, `bool` and nil must never allocate", and
// PLAN §6.1 calls `sizeof(Number)` "the one number that could make this port
// not fit" -- so M8 measured it before writing a line and chose the layout that
// keeps it at 32. A std::variant is its widest arm plus a discriminator rounded
// to the alignment: 32 + 8 = 40, with nothing to spare. The static_assert at
// the bottom is what makes an arm added without thinking a compile error naming
// this line, instead of a Value that quietly grew by a word.
//
// THE VARIANT IS APPEND-ONLY AND THAT IS THE FIRST SATELLITE'S NOTE, KEPT.
// v1's value header says "APPEND ONLY" and "silently renumber every alternative
// after it" in capitals, because `std::variant`'s index IS the order the arms
// are written and v1 switched on raw indices. PLAN §7 throws that away -- there
// is no switch on an index in this tree and there must not be one -- but the
// append-only rule survives it, because `.satc` files, dumps and the inline
// caches in evaluator/dispatch.hpp all key on the type tag.
//
// FIVE ARMS AT M10, AND THE EMPTINESS IS DELIBERATE. DESIGN §8's table has
// thirteen rows and this held the five a program could PRODUCE at M10: nothing,
// a bool, a number, a string and the runtime singleton. THIRTEEN since M20. An arm with no producer
// is a case every later reader has to rule out -- name_resolver/resolve.hpp
// refuses six sentinels for three on exactly that argument -- so the rest
// arrive with the milestone that can build one. PLAN §8's M9 entry names three
// of the known appends: the file handle (M19), the arguments object (M20) and
// whatever Orbit's result becomes (M28). Each re-runs the assert.
//
// `Runtime` IS THE FIRST APPEND AND IT COST NO BYTES, which is the append-only
// rule working rather than being obeyed. M10 gave `satellite` a producer --
// DESIGN §3's "the singleton runtime object, not a zero sentinel" -- so the arm
// arrived with the milestone that can build one, at the END of the list, and
// the assert below did not move: an empty struct in a variant whose widest arm
// is 32 bytes is free.
//
// AND `satellite.variable.float` CANNOT BE ONE OF THEM, which was known four
// milestones before M15 tried. DESIGN §8.6 makes a float a bool and TWO
// `satellite_number`s -- 72 bytes laid flat, against a 40-byte budget the
// number alone fills to the brim. So a float arrives behind a handle the way a
// list and a map will, and that is a consequence of §8.1's exactness rather
// than a decision M15 got to take. M15 landed it exactly there -- `Flo` below,
// the THIRD append, sixteen shared_ptr bytes against the 32-byte widest arm --
// and the assert did not move, which is this paragraph doing the job it was
// written for.

#include "satellite_bits/bits.hpp"
#include "satellite_file/file_handle.hpp"
#include "satellite_float/satellite_float.hpp"
#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace satellite {

// A string value. DESIGN §8.3: `SatString` over the 16-bit code table, which
// IS the language's alphabet.
//
// SHARED AND CONST, so assignment and argument passing copy sixteen bytes and a
// refcount rather than the characters. That is the one arm at M9 which
// allocates at all, and DESIGN §8.2's promise is about the other three: a loop
// counter, an index and every small literal live entirely in a `long long`.
using Str = std::shared_ptr<const SatString>;

// Nothing. DESIGN §6.4 qualification 3 names it -- "a declared variable holding
// nothing" -- and DESIGN §8.7, M12's, says what it IS: a state every type has,
// askable through the variant's rows. It is the state a capsule with no
// `satellite.return` answers with, and the state a declared-but-uninitialised
// slot holds.
struct Nothing {
    bool operator==(const Nothing &) const = default;
};

// THE RUNTIME SINGLETON. DESIGN §8's table calls it "the runtime singleton" and
// §3 says what it is for: "`satellite` as a value is the singleton runtime
// object, NOT A ZERO SENTINEL. That is why `satellite.include(satellite)` and
// `satellite.return(satellite)` use the same word to mean sensible things:
// include the runtime, return the runtime (that is, success)."
//
// IT CARRIES NOTHING AND THAT IS THE WHOLE TYPE. There is one runtime and a
// program cannot have a second, so a value of this arm says which arm it is and
// has nothing left to say. `Nothing` is the same shape one row up and they are
// two arms rather than one, because a capsule that answered `satellite` and a
// capsule that answered nothing gave two different answers -- which is the
// distinction §6.4 q3 draws between an undeclared variable and a declared one
// holding nothing, one layer along.
struct Runtime {
    bool operator==(const Runtime &) const = default;
};

// AN INSTANT: int64 nanoseconds on the Unix epoch, `system_clock`'s reading.
// DESIGN §13's Time entry, settled 2026-09-04 -- the VALUE is the wall clock,
// because a value has to mean something outside the process that read it; the
// timer under `satellite.time.sleep` is `steady_clock` and never appears here.
// 61 bits of epoch against a 53-bit mantissa is why this is an integer and not
// a `double`: 198 ns of resolution at the current epoch would make two
// back-to-back readings identical, which is useless for the one thing a clock
// is for. M13 gives an instant exactly one ability -- being displayed -- and
// M29 is where its methods arrive; the arm carries everything they will need.
struct Time {
    long long ns = 0;
    bool operator==(const Time &) const = default;
};

// A FLOAT VALUE, BEHIND ITS HANDLE. DESIGN §8.6's type is 72 bytes laid
// flat, so what the arm holds is what `Str` holds one row up: sixteen bytes of
// shared const handle, copied by refcount. SHARED AND CONST for Str's reason
// exactly -- a float assigned or passed is a pointer copy, and nothing can
// mutate a value two slots see.
using Flo = std::shared_ptr<const Float>;

// THE TWO CONTAINERS, BEHIND THE SAME KIND OF HANDLE -- M16. DESIGN §8's
// table: a list is "vector of values, children shared" and a map is "body
// behind a handle; insertion-ordered". Both bodies are BUILT, THEN FROZEN --
// v1's rule, kept for v1's reason: every mutation is a copy published whole
// through the receiver's storage slot (DESIGN §6.4), so a value two slots see
// can never change under either of them. The types are defined below Value
// because a List holds Values by value; a shared_ptr to an incomplete type is
// all the variant needs here.
struct List;
struct MapBody;
using Lst = std::shared_ptr<const List>;
using Map = std::shared_ptr<const MapBody>;

// AN OPEN FILE -- M19, the SIXTH append, and the first arm in this variant
// that is NOT const behind its handle. Every row above shares an immutable
// body: a value two slots see can never change under either of them, because
// every mutation is a copy published whole through the receiver's storage slot
// (DESIGN §6.4). DESIGN §8's table calls a file a "reference type" and means
// the opposite -- two names for one open file ARE one open file, and `close`
// through either closes it for both, because there is one descriptor and the
// kernel has never heard of our slots.
//
// SO THE `const` COMES OFF, AND IT COMES OFF EXACTLY HERE AND NOWHERE ELSE.
// The rule the other arms keep is a rule about VALUES; a file is not one, and
// writing it as `shared_ptr<const FileHandle>` with mutable atomics inside
// would be the same reference semantics with a `const` that lied about them.
// satellite_file/file_handle.hpp carries which fields move and why two of them
// are atomic.
//
// SIXTEEN BYTES AGAINST A 32-BYTE WIDEST ARM, so the assert at the bottom does
// not move -- `Str`'s accounting, for the fifth time. PLAN §8's M9 entry named
// this append four milestones before it arrived.
using Fil = std::shared_ptr<file::FileHandle>;

// A RUN OF BITS -- `satellite.variable.binary` `1 6 5`, M19.5, and the
// SEVENTH append. DESIGN §8.5's type: the width is part of the value, so the
// body is a `std::vector<bool>` and never an integer with a length beside it.
// satellite_bits/bits.hpp is the type and carries the measurement behind
// "one bit per bit".
//
// SHARED AND CONST, WHICH PUTS IT BACK ON THE RULE `Fil` CAME OFF. A bit run
// is a VALUE -- two names for one are two values, and there is no descriptor
// underneath for the kernel to have an opinion about -- so it takes `Str`'s
// arrangement and not the file's: sixteen bytes of handle, copied by refcount,
// and nothing can mutate a body two slots see.
//
// BEHIND A HANDLE FOR THE FLOAT'S REASON RATHER THAN THE FLOAT'S SHAPE. A
// float is 72 bytes because DESIGN §8.6 makes it two `Number`s and a sign; a
// bit run is unbounded because §8.5 makes the width part of the value and puts
// no ceiling on it. Different types, same consequence: neither fits in the
// 40-byte budget inline, so both arrive as a pointer.
using Bin = std::shared_ptr<const bits::BitRun>;

// A RUN OF HEX DIGITS -- `satellite.variable.hex` `1 6 11`, M19.5's second
// half, and the EIGHTH append. Sixteen bytes again, for `Str`'s reason for the
// sixth time, so the assert at the bottom does not move.
//
// A SEPARATE ARM AND NOT A FLAG ON `Bin`, which is DESIGN §8.5's "`b1111 ==
// xF` is false" made structural. The two hold the SAME BITS -- a hex run is a
// `BitRun` in a wrapper -- so a shared arm with a radix field would have made
// that comparison true unless every reader remembered to check the field, and
// `same()` in value.cpp would have been the one place forgetting was silent.
// Two arms make different-arms-never-equal do the work, which §8's model
// already promised and which costs nothing here.
//
// THE WRAPPER IS WHY THIS IS NOT `Bin` SPELLED TWICE. bits.hpp's `HexRun`
// holds a `BitRun` rather than deriving from one precisely so that these two
// `shared_ptr`s have no implicit conversion between them -- write
// `Value::binary` where you meant `Value::hex` and it is a compile error
// rather than a value that changed type on the way into a slot.
using Hex = std::shared_ptr<const bits::HexRun>;

// THE ARGUMENTS OBJECT -- `satellite.container.arguments` `1 4 3`, M20, and
// the NINTH append. DESIGN §7.7's "the language handing the PROGRAM everything
// it knows about the machine it woke up on": the command line it was started
// with, reached by number, and the machine's own answers reached by name.
//
// FORWARD-DECLARED HERE AND DEFINED IN value_arguments.hpp, which is `List`
// and `MapBody`'s arrangement six rows up and is forced by the same thing: the
// body holds `Value`s, so it cannot be defined until this type is complete,
// while the variant below cannot be spelled until the handle has a name.
//
// SHARED AND CONST, so the assert at the bottom does not move -- sixteen bytes
// against a 32-byte widest arm, `Str`'s accounting for the SEVENTH time. PLAN
// §8's M9 entry named this append four milestones before it arrived and asked
// that the milestone re-run the assert; M20 did, and it holds at 40.
//
// A VALUE AND NOT A REFERENCE TYPE, which is the line `Fil` is on the other
// side of. Two names for one open file are one open file because there is one
// descriptor and the kernel has never heard of our slots; two names for one
// arguments object are two values of a body nothing can write, because the
// body is built before `make_shared` and frozen -- `MapBody`'s rule, and this
// arm keeps it.
struct Arguments;
using Arg = std::shared_ptr<const Arguments>;

// A DEFERRED CALL -- `satellite.variable.capsule` `1 6 16`, M23, and the TENTH
// append. DESIGN §13's settled form, in one sentence of its own: "the handler
// evaluates the arguments and stores (capsule number, argument values) for the
// thread to run later."
//
//     satellite.variable.thread t = satellite.thread.new(capsule_test(word))
//
// SO `word` IS IN HERE AND `capsule_test` IS NOT RUN. The arguments were
// evaluated on the calling thread at the moment `new` ran, which is what makes
// this a value rather than a closure: there is no environment to capture and
// nothing here can see a slot. DESIGN §12's deferral of "a bare name can be a
// value" survives it for exactly that reason -- what was written is a CALL, and
// the only new semantics is that one handler packaged its arguments instead of
// performing the call.
//
// SHARED AND CONST, which puts it on `Str`'s side of the line and not `Fil`'s.
// A deferred call is a VALUE: two names for one are two values, nothing can
// write it, and handing the same one to two threads is two runs of one capsule
// rather than one run seen twice. Sixteen bytes against a 32-byte widest arm,
// so the assert at the bottom does not move -- `Str`'s accounting for the
// EIGHTH time.
//
// FORWARD-DECLARED, `Arguments`' arrangement one row up and forced by the same
// thing: the body holds `Value`s, so it cannot be defined until this type is
// complete. satellite_thread/thread_handle.hpp is where it is.
namespace thread {
struct Deferred;
struct ThreadHandle;
} // namespace thread

using Cap = std::shared_ptr<const thread::Deferred>;

// A THREAD -- `satellite.variable.thread` `1 6 13`, M23, and the ELEVENTH
// append. DESIGN §10.5's type, and §7's frames are what make a capsule call
// safe to run on one.
//
// NOT const BEHIND ITS HANDLE, WHICH IS `Fil`'s SIDE OF THE LINE AND THE SECOND
// ROW EVER ON IT. DESIGN §8's table calls a file a reference type and means
// that two names for one open file ARE one open file; a thread is the same
// sentence about a different resource. `start()` through either handle starts
// the one thread, `join()` through either waits for the same one, and there is
// one `pthread_t` underneath that the kernel has never heard of our slots
// about. Writing it `shared_ptr<const ThreadHandle>` with mutable atomics
// inside would be the same reference semantics with a `const` that lied.
//
// AND IT IS THE FIRST ARM WHOSE BODY IS TOUCHED BY TWO THREADS AT ONCE, which
// is the sentence `Fil`'s note has been waiting for: satellite_file/
// file_handle.hpp says "M23 is when it first gets EXERCISED, not when it gets
// written". satellite_thread/thread_handle.hpp carries which of its fields are
// atomic and why.
//
// SIXTEEN BYTES AGAIN, `Str`'s accounting for the NINTH time, so the assert at
// the bottom does not move.
using Thr = std::shared_ptr<thread::ThreadHandle>;

// A SPACESUIT INSTANCE -- `satellite.spacesuit` `1 10`, M26, and the TWELFTH
// append. DESIGN §7 calls its subject "the most important decision in the
// language", and this is the storage that is not a call's: an object's fields,
// shared by every method called on it and outliving all of them.
//
// NOT const, WHICH IS THE THIRD ARM ON THAT SIDE OF THE LINE AFTER `Fil` AND
// `Thr` -- and the first of the three where it is the LANGUAGE's semantics
// rather than a resource's. A file is a reference type because there is one
// descriptor; a thread because there is one `pthread_t`; a spacesuit because
// DESIGN §13 says so under Decided and §7.4 carries the receipt for what the
// other answer costs: "a spacesuit is a reference type, so reusing the old slot
// would leave every handle already taken to the first instance pointing at the
// second -- and a list built by that idiom would read back as n copies of its
// last element WITH NO ERROR ANYWHERE."
//
// SIXTEEN BYTES, `Str`'s accounting for the TENTH time, so the assert at the
// bottom does not move. satellite_spacesuit/suit_object.hpp is the body and
// carries the cycle-leak decision.
namespace suit {
struct SuitObject;
} // namespace suit

using Sui = std::shared_ptr<suit::SuitObject>;

// APPEND ONLY. A new arm goes at the END of this list, never in the middle.
// `Time` IS THE SECOND APPEND AND IT COST NO BYTES -- eight against a 32-byte
// widest arm, the same accounting `Runtime`'s note above runs. `Flo` is the
// third, M15's, sixteen bytes by the same account; `Lst` and `Map` are the
// fourth and fifth, M16's, sixteen each by the same account again; `Fil` is
// the sixth, M19's, sixteen more; `Bin` is the SEVENTH, M19.5's, sixteen more
// again; `Hex` is the EIGHTH, M19.5's other half, sixteen more still.
//
// PLAN §8's M19.5 ENTRY CALLS THESE "the sixth and seventh appends" AND THAT
// IS OFF BY ONE, which is worth correcting here rather than anywhere else
// because this list is the fact it is a claim about. The entry was written on
// 2026-09-08 while M19 was in flight, and M19's `Fil` took sixth. Binary is
// the seventh and hex is the eighth, landed 2026-09-09. The assert did not
// move either way, which is the half of the sentence that was the point.
//
// `Cap` AND `Thr` ARE THE TENTH AND ELEVENTH, M23's, sixteen each by the same
// account again -- and `Thr` is the SECOND arm ever to come off the `const`,
// after `Fil`. The line it is on the far side of is "is this a value", and a
// thread is not one for a file's reason exactly.
//
// `Arg` IS THE NINTH, M20's, sixteen more again -- and it is the one append
// this file predicted by name. The paragraph five screens up has said "the
// arguments object (M20)" since M9 wrote it, beside the instruction that each
// named append re-runs the assert. It was re-run and it holds at 40.
using ValueBase = std::variant<Nothing, bool, Number, Str, Runtime, Time, Flo,
                              Lst, Map, Fil, Bin, Hex, Arg, Cap, Thr, Sui>;

// One value. DESIGN §8's table, SIXTEEN arms of it since M26.
//
// A STRUCT OVER THE VARIANT AND NOT AN ALIAS, so that the helpers below have
// somewhere to live and so that `Value` is a name the compiler prints in an
// error rather than sixty characters of angle brackets.
struct Value : ValueBase {
    using ValueBase::ValueBase;

    Value() : ValueBase(Nothing{}) {}

    static Value nothing() { return Value(Nothing{}); }
    static Value boolean(bool b) { return Value(b); }
    static Value number(Number n) { return Value(std::move(n)); }
    static Value string(SatString s)
    {
        return Value(std::make_shared<const SatString>(std::move(s)));
    }
    static Value runtime() { return Value(Runtime{}); }
    static Value instant(long long ns) { return Value(Time{ns}); }
    static Value floating(Float f)
    {
        return Value(std::make_shared<const Float>(std::move(f)));
    }
    // Defined below List and MapBody, which need Value complete first.
    static Value list(List items);
    static Value map(MapBody body);

    // A RUN OF BITS -- M19.5. This one IS a plain factory the way `string` and
    // `instant` are, and the note below about `file` says exactly why the file
    // is not: a bit run is built from a value already in hand, there is one way
    // to make one, and nothing about it can fail.
    static Value binary(bits::BitRun run)
    {
        return Value(std::make_shared<const bits::BitRun>(std::move(run)));
    }

    // A RUN OF HEX DIGITS -- M19.5's second half. `binary`'s argument exactly:
    // one way to make one, built from a value already in hand, nothing to fail.
    static Value hex(bits::HexRun run)
    {
        return Value(std::make_shared<const bits::HexRun>(std::move(run)));
    }

    // NO `Value::file(...)` FACTORY, and the absence is deliberate. Every
    // factory above BUILDS its body from a plain C++ value, because there is
    // exactly one way to make a string or an instant and no state to get wrong.
    // A FileHandle is opened, which is a syscall that can fail, and the failure
    // is itself a handle -- DESIGN §9's failed open is a value. Putting that
    // behind a one-line factory here would put the O_EXCL argument, the mode
    // table and the errno contract in the value model, where nothing else knows
    // what a file is. satellite_file/file_handle.cpp's `opened()` is the one
    // constructor, both module rows go through it, and this arm is written
    // `Value(handle)` at the two call sites that have one.

    bool is_nothing() const { return std::holds_alternative<Nothing>(*this); }
    bool is_bool() const { return std::holds_alternative<bool>(*this); }
    bool is_number() const { return std::holds_alternative<Number>(*this); }
    bool is_string() const { return std::holds_alternative<Str>(*this); }
    bool is_runtime() const { return std::holds_alternative<Runtime>(*this); }
    bool is_time() const { return std::holds_alternative<Time>(*this); }
    bool is_float() const { return std::holds_alternative<Flo>(*this); }
    bool is_list() const { return std::holds_alternative<Lst>(*this); }
    bool is_map() const { return std::holds_alternative<Map>(*this); }
    bool is_file() const { return std::holds_alternative<Fil>(*this); }
    bool is_binary() const { return std::holds_alternative<Bin>(*this); }
    bool is_hex() const { return std::holds_alternative<Hex>(*this); }
    bool is_arguments() const { return std::holds_alternative<Arg>(*this); }
    bool is_capsule() const { return std::holds_alternative<Cap>(*this); }
    bool is_thread() const { return std::holds_alternative<Thr>(*this); }
    bool is_suit() const { return std::holds_alternative<Sui>(*this); }
};

// `satellite.container.list<T>` -- a vector of values with a name a forward
// declaration can carry, which an alias cannot. NOT polymorphic and never
// deleted through the base; the inheritance is spelling, not design.
struct List : std::vector<Value> {
    using std::vector<Value>::vector;
};

// One `satellite.container.map` entry, in insertion order. The stored KEY
// VALUE rides beside the value so `.keys` can answer what the program wrote;
// the canonical form lives only in the body's index.
struct MapEntry {
    Value key;
    Value value;
};

// `satellite.container.map<K, V>`. INSERTION-ORDERED (DESIGN §8.4), with a
// side index for O(1) lookup by canonical key -- `.keys()` is how a map is
// walked, since §6's only loop is the C-shaped `for`, and an unordered map
// would make every program that walks one non-deterministic. The key of
// `index` is map_key_of's canonical byte string, never decoded text --
// map_key_of's note in value.cpp is the argument.
struct MapBody {
    std::vector<MapEntry> entries;
    std::unordered_map<std::string, size_t> index;
};

inline Value Value::list(List items)
{
    return Value(std::make_shared<const List>(std::move(items)));
}

inline Value Value::map(MapBody body)
{
    return Value(std::make_shared<const MapBody>(std::move(body)));
}

// The body behind a container arm, or nullptr when the value is not that
// container. A NULL HANDLE ANSWERS AN EMPTY BODY, defensively -- the factories
// above never build one, and a reader that crashed on it would be a crash
// waiting on a producer this module cannot see (string_at's rule, one module
// over).
inline const List *as_list(const Value &value)
{
    static const List empty;
    if (const Lst *handle = std::get_if<Lst>(&value))
        return *handle ? handle->get() : &empty;
    return nullptr;
}

inline const MapBody *as_map(const Value &value)
{
    static const MapBody empty;
    if (const Map *handle = std::get_if<Map>(&value))
        return *handle ? handle->get() : &empty;
    return nullptr;
}

// The run behind a binary arm, or nullptr when the value is not one. A NULL
// HANDLE ANSWERS AN EMPTY RUN, which is as_list's rule one row up and is held
// to for as_list's reason: Value::binary never builds one, and a reader that
// crashed on it would be a crash waiting on a producer this module cannot see.
inline const bits::BitRun *as_binary(const Value &value)
{
    static const bits::BitRun empty;
    if (const Bin *handle = std::get_if<Bin>(&value))
        return *handle ? handle->get() : &empty;
    return nullptr;
}

// The run behind a hex arm, or nullptr when the value is not one. as_binary's
// rule and as_binary's reason, one arm over.
//
// IT ANSWERS nullptr FOR A BINARY AND as_binary ANSWERS nullptr FOR A HEX,
// which is the pair of facts DESIGN §8.5's "`b1111 == xF` is false" rests on
// down here. Neither reader can be handed the other's value, so no method has
// to check the radix and none does.
inline const bits::HexRun *as_hex(const Value &value)
{
    static const bits::HexRun empty;
    if (const Hex *handle = std::get_if<Hex>(&value))
        return *handle ? handle->get() : &empty;
    return nullptr;
}

// A key's canonical bytes, or false when this value cannot be a key. DESIGN
// §8.4 restricts key types because a map's hash and equality run inside a
// mutation and must be native (§6.5); WHICH types is decided here, beside the
// body whose index it feeds: a number or a string, exactly v1's answer.
// value.cpp carries the two traps the canonical form steps around.
bool map_key_of(const Value &value, std::string &out);

// 40 BYTES. See the header note -- this is DESIGN §8.2's budget, and the arm
// that fills it is `Number` at 32.
static_assert(sizeof(void *) != 8 || sizeof(Value) == 40,
              "value.hpp: a Value is 40 bytes on 64-bit. DESIGN §8.2 is the "
              "budget and satellite_number/bignum_number.hpp's 32 is what "
              "fills it -- an arm wider than a Number does not go inline");

// WHAT A VALUE IS CALLED, which is the word a diagnostic uses and not a C++
// type name. DESIGN §6.4 q3 wants "an undeclared variable" and "a declared
// variable holding nothing" to read differently, and every S07xx sentence that
// names a type gets the word from here so there is one spelling of each.
const char *type_name(const Value &value);

// Whether a value counts as true. DESIGN §6's conditions.
//
// ONLY A BOOL IS, AND THAT IS THE LANGUAGE RATHER THAN A CONVENIENCE. There is
// no truthiness ladder in satellite -- no empty string that is false, no zero
// that is false -- because DESIGN §1.1's rule is never to do anything behind
// the user's back and a number silently standing in for a condition is exactly
// that. Anything else is a refusal at the site, which is why this answers into
// an out parameter rather than returning a bool nobody can tell apart from
// `false`.
bool truth_of(const Value &value, bool *out);

// Two values are equal. Different arms are never equal -- there is no
// conversion in this language, so `1 == satellite.bool.true` is false rather
// than an error.
bool same(const Value &left, const Value &right);

} // namespace satellite
