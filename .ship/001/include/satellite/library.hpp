// satellite.library -- where every variable in a running satellite program
// lives.
//
// THE ONE IDEA: satellite.library.<capsule>.<var> reaches any variable
// anywhere.  There is no scope chain to walk, no enclosing environment to
// search.  A name is a path, a path is a key, and a key is one hash lookup.
//
//     satellite.library.main.counter
//     `--------------' `--' `-----'
//        the prefix    where  what
//
// So the storage is FLAT.  A tree of nested scopes would be the obvious design
// for most languages, but satellite does not need one: the language already
// says every variable is addressable by its full path from anywhere, and that
// is exactly a flat map.
//
// ---------------------------------------------------------------------------
// TWO WAYS TO ASK, AND A THIRD WAY THAT IS THE FAST ONE
// ---------------------------------------------------------------------------
//
//   1. Two strings -- where it lives, and what it is called:
//
//          lib.define("satellite.main", "counter", Container::integer(0));
//          lib.get("satellite.main", "counter", &v);
//
//   2. One string -- the path exactly as it appears in satellite source:
//
//          lib.get_path("satellite.library.main.counter", &v);
//
//   3. A Slot -- an integer, resolved ONCE:
//
//          Slot s = lib.slot_of("satellite.main", "counter");   // once
//          lib.read(s, &v);                                     // forever after
//
// Forms 1 and 2 hash a string every single time.  That is fine while parsing
// and fine from C++, but a running program must never do it: a loop that
// touches one variable a million times would hash that name a million times,
// and string hashing would become the slowest thing in the language.
//
// So the compiler resolves each name to a Slot at compile time and the virtual
// machine only ever uses the integer.  Strings are how SOURCE spells a
// variable; integers are how the MACHINE reaches it.
//
// ---------------------------------------------------------------------------
// WHY A DEQUE, AND WHY THAT MATTERS FOR 200 THREADS
// ---------------------------------------------------------------------------
//
// Entries live in a std::deque, which never moves an element once placed.  So
// `address(slot)` hands back a Container* that stays valid for the life of the
// Library even as thousands more variables are defined behind it.
//
// That is what makes the threading model cheap.  The name->Slot map needs a
// lock, because defining a variable mutates it -- but that happens at compile
// time.  Once every worker holds resolved pointers, reading and writing a
// variable touches no shared map and takes no lock at all.  The global
// contention point that "any variable reachable from anywhere" would normally
// create gets paid once per name, not once per access.
//
// (Two threads writing the SAME variable still race.  That is a per-variable
// question for when the VM exists, not a property of the store.)
#pragma once

#include <cstdint>
#include <deque>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

#include "satellite/container.hpp"
#include "satellite/lexer.hpp"

namespace satellite {

// A resolved handle to one library entry.  1-based; 0 means "no such name".
using Slot = uint32_t;
constexpr Slot kNoSlot = 0;

// Variables not written inside any capsule live at this location.
inline const char* const kGlobal = "";

// What a name holds.  The Container is the same for all of them -- this only
// records what the name MEANS, so that `satellite.main` can be found as a
// capsule rather than as a variable that happens to be called main.
enum class Kind : uint8_t {
    Variable = 0,
    Capsule,      // satellite.capsule some_name(...)
    Spacesuit,    // satellite.spacesuit some_class
    Native        // provided by the interpreter, not by satellite source
};
const char* kind_name(Kind k);

// ---------------------------------------------------------------------------
struct Entry {
    std::string location;              // canonical: "main", not "satellite.main"
    std::string name;                  // "counter"
    Container   value;
    Kind        kind = Kind::Variable;
    Slot        slot = kNoSlot;

    std::string path() const;          // "main.counter"
    std::string display() const;       // "satellite.library.main.counter"
};

// ---------------------------------------------------------------------------
class Library {
public:
    Library() = default;
    Library(const Library&)            = delete;
    Library& operator=(const Library&) = delete;

    // -----------------------------------------------------------------------
    // Two strings: location, then name.
    // -----------------------------------------------------------------------

    // Creates it, or overwrites it if the name already exists.  Returns the
    // slot either way, so defining twice is safe and gives the same handle.
    Slot define(const std::string& location, const std::string& name,
                Container value, Kind kind = Kind::Variable);

    // Assign to a name that must ALREADY exist.  false if it does not --
    // this is what catches a typo'd variable rather than silently creating one.
    bool set(const std::string& location, const std::string& name, Container value);

    bool get(const std::string& location, const std::string& name, Container* out) const;
    bool has(const std::string& location, const std::string& name) const;
    Slot slot_of(const std::string& location, const std::string& name) const;

    // -----------------------------------------------------------------------
    // One string: the path as satellite source spells it.
    // -----------------------------------------------------------------------
    Slot define_path(const std::string& dotted, Container value, Kind kind = Kind::Variable);
    bool set_path(const std::string& dotted, Container value);
    bool get_path(const std::string& dotted, Container* out) const;
    bool has_path(const std::string& dotted) const;
    Slot slot_of_path(const std::string& dotted) const;

    // -----------------------------------------------------------------------
    // By slot -- no string, no hash.  What the VM uses.
    // -----------------------------------------------------------------------
    bool read(Slot s, Container* out) const;
    bool write(Slot s, Container value);
    Kind kind_of(Slot s) const;

    // Stable for the life of the Library (deque never relocates).  Resolve
    // once at compile time; the running program then needs no lock.
    Container*       address(Slot s);
    const Container* address(Slot s) const;

    // Copy of the bookkeeping for one slot, for diagnostics.
    bool describe(Slot s, Entry* out) const;

    // -----------------------------------------------------------------------
    // Whole-store questions
    // -----------------------------------------------------------------------
    bool   erase(const std::string& location, const std::string& name);
    void   clear();
    size_t size() const;

    std::vector<std::string> locations() const;                       // sorted
    std::vector<std::string> names_in(const std::string& location) const;  // sorted
    std::vector<Slot>        slots() const;                           // define order
    std::string              dump() const;                            // human readable

    // -----------------------------------------------------------------------
    // Path arithmetic.  Static and lock-free, so it is testable on its own.
    //
    // All of these spellings mean the same variable:
    //
    //     satellite.library.main.counter      as written in satellite source
    //     satellite.library.satellite.main.counter   main spelled in full
    //     satellite.main.counter              location + name, from C++
    //     main.counter                        canonical
    //
    // and a bare `counter` with no dot means the global location.
    //
    // The escape hatch still works: satellite.library.helper.satellite splits
    // into location "helper", name "satellite", because only the LAST dot
    // separates the name.
    // -----------------------------------------------------------------------

    // "satellite.main" -> "main".  A capsule genuinely named `satellite` (via
    // the escape hatch) has no dot, so it survives untouched.
    static std::string normalize_location(const std::string& location);

    static bool        split_path(const std::string& dotted,
                                  std::string* location, std::string* name);
    static std::string join_path(const std::string& location, const std::string& name);
    static std::string display_path(const std::string& location, const std::string& name);

private:
    Slot find_locked(const std::string& location, const std::string& name) const;

    mutable std::shared_mutex             mu_;
    std::deque<Entry>                     entries_;   // stable addresses
    std::unordered_map<std::string, Slot> by_key_;    // "main.counter" -> slot
};

// ---------------------------------------------------------------------------
// The entry point.
//
// Every app starts at `satellite.capsule satellite.main`, and it must be in
// the FIRST file given to the interpreter -- not in an include.  An included
// file that declares main is a library pretending to be a program, and the
// error should say so.
//
// This scans tokens because version 001 has no parser.  When the parser lands
// it will register capsules into the Library as Kind::Capsule, and finding
// main becomes lib.slot_of("satellite", "main") with no scanning at all.
// ---------------------------------------------------------------------------
struct MainLocation {
    bool   found       = false;
    int    line        = 0;
    int    col         = 0;
    size_t token_index = 0;
    bool   has_parens  = false;   // `satellite.main(` -- a real capsule header
};

MainLocation find_main(const std::vector<Token>& tokens);

}  // namespace satellite
