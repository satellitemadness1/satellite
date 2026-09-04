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
// thirteen rows and this holds the five a program can PRODUCE today: nothing, a
// bool, a number, a string and the runtime singleton. An arm with no producer
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
// AND `satellite.variable.float` CANNOT BE ONE OF THEM, which is worth knowing
// four milestones before M15 tries. DESIGN §8.6 makes a float a bool and TWO
// `satellite_number`s -- 72 bytes laid flat, against a 40-byte budget the
// number alone fills to the brim. So a float arrives behind a handle the way a
// list and a map do, and that is a consequence of §8.1's exactness rather than
// a decision M15 gets to take. Recorded here because the place it would be
// discovered is a failing static_assert with no explanation attached.

#include "satellite_number/bignum.hpp"
#include "satellite_string/satellite_string.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <variant>

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

// APPEND ONLY. A new arm goes at the END of this list, never in the middle.
using ValueBase = std::variant<Nothing, bool, Number, Str, Runtime>;

// One value. DESIGN §8's table, five rows of it.
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

    bool is_nothing() const { return std::holds_alternative<Nothing>(*this); }
    bool is_bool() const { return std::holds_alternative<bool>(*this); }
    bool is_number() const { return std::holds_alternative<Number>(*this); }
    bool is_string() const { return std::holds_alternative<Str>(*this); }
    bool is_runtime() const { return std::holds_alternative<Runtime>(*this); }
};

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
