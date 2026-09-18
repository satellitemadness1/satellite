#pragma once
// satellite/satellite_object/satellite_object.hpp -- WHAT ONE EXPRESSION IS
// WORTH. The variant every value in the language is an arm of.
//
// (the author, 2026-09-16) "2 main classes for the satellite object model...
// both are going to hold std::variant<satellite_number, satellite_string,
// satellite_bytecode>... let's just bolt our methods onto these two classes...
// it does become a god class, but let's just do it this way because this way
// it's both fast, easy and powerful."
//
// THE THREE WORDS, AND THEY ARE THREE DIFFERENT THINGS. This was got wrong once
// and the author caught it -- "this is our spacesuit... this is satelliteSpacesuit
// not satelliteObject!" -- so it is written down here in the language's own
// vocabulary, which 004's words.tsv and 003 both already settled:
//
//   spacesuit   THE CLASS. `satellite.spacesuit` is word `1 10`, a real word of
//               this language, aliased `satellite.class` by the author on
//               2026-09-09. DESIGN 13: "Classes are satellite.spacesuit...
//               Reference semantics."  -> satelliteSpacesuit
//   object      ONE INSTANCE of a spacesuit. NOT a word of the language -- it is
//               in none of words.tsv or REGISTRY.satellite -- but it is the
//               author's own prose for an instance: DESIGN 13's "what a
//               constructor produces is the object", M26's `object_name.pointer()`.
//                                                       -> satelliteObject
//   spaceship   A FILE that is included, `satellite.include(spaceship)`, `1 1 2`.
//               A different thing again, and nothing here.
//
// satelliteObject IS THE ONE WE HAND DESIGN (the author): "satelliteObject
// which is satelliteCapsule, satellite_number satellite_string
// satellite_bytecode, satellite_bool, satellite_time, satellite_file, and any
// other variable we have". Every built-in type of the language is an arm of it.
// 003 called this same type `Value` (src/satellite_value/value.hpp, sixteen arms
// since M26); satellite_time and satellite_file are two of the arms it already
// had and 004 has not built yet.
//
// RESOURCE MANAGEMENT IS NOT A CONCERN (the author): "We don't need to be
// careful and be picky about a 70 byte object, or a 20 byte object... I have 64
// gigabytes." He is right, and it is better than the argument: every heavy arm
// is a handle, so the variant is the same size whether bytecode is in it or not.
//
// ARMS ARE APPENDED, NEVER INSERTED. Kind IS the variant's index, so an arm put
// in the middle silently renumbers every arm after it. 003's value.hpp says the
// same thing in capitals and traces it to v1, and the word codes have the same
// hazard for the same reason (a word's code is 4097 plus its ROW). The
// static_asserts under Kind make a mistake here a compile error.

#include "satellite_bytecode.hpp"
#include "satellite_capsule.hpp"
#include "../satellite_variable_binary/satellite_binary_number.hpp"
#include "../satellite_variable_file/satellite_file.hpp"
#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace satellite004 {

// A SPACESUIT THE USER DEFINED, declared here and defined in
// satellite_spacesuit.hpp -- because its fields are satelliteObjects and this
// file is what defines those. A std::shared_ptr of an incomplete type is
// complete, so the variant below is satisfied by the handle alone.
struct satelliteUserDefinedObject;

// THE HANDLE, AND IT IS THE AUTHOR'S OWN RULING RATHER THAN A C++ WORKAROUND.
// DESIGN 7.4: "a spacesuit is a reference type", and M26 built it that way --
// a capsule handed a spacesuit mutates it and the CALLER SEES THE MUTATION. So
// two names holding one of them hold ONE of them. Storing it inline would make
// `b = a` copy it, which is the opposite behaviour, decided by accident. M26
// also chose the counting: refcount now, `.pointer()` as the weak reference, a
// cycle collector as its own milestone.
using UserDefinedHandle = std::shared_ptr<satelliteUserDefinedObject>;

// A FILE IS A REFERENCE TOO (003's rule, and M15's warning that it is "the first
// arm to come off const, because a file is not a value"): two names for one open
// file are two names for ONE file, so the arm is a handle and a copy shares it.
using FileHandle = std::shared_ptr<satellite_file>;

// A LIST HOLDS satelliteObjectS, AND A satelliteObject MAY BE A LIST -- so the
// arm cannot be the vector itself, for exactly the reason the spacesuit's cannot
// be the object: the type would contain itself. Defined in satellite_list.hpp,
// which this file cannot include because that file needs this one complete.
//
// The handle does NOT mean a list is a reference type. Nothing in 004 can change
// a list after it is made, so sharing is unobservable today; satellite_list.hpp
// carries the red note and 003 §12's copy-on-write ruling for when it is not.
struct satelliteList;
using ListHandle = std::shared_ptr<satelliteList>;

class satelliteObject {
public:
    // EVERY ARM, IN ORDER. The order IS the Kind below.
    using Held = std::variant<std::monostate,    // 0  nothing
                              bool,              // 1  satellite.bool
                              satellite_number,  // 2  satellite.variable.number
                              satellite_string,  // 3  satellite.variable.string
                              satellite_bytecode,// 4  the numbered program
                              satelliteCapsule,  // 5  a capsule as a value
                              UserDefinedHandle,   // 6  one object of a spacesuit
                              satellite_binary_number, // 7  satellite.variable.binary
                              satellite_percentage,    // 8  satellite.variable.percentage
                              FileHandle,              // 9  satellite.variable.file (2026-09-18)
                              ListHandle               // 10 {a, b} (2026-09-18)
                              // APPEND HERE, NEVER INSERT ABOVE. 003's own list
                              // is the map of what comes: Flo, Lst, Map, Fil,
                              // Bin, Hex, Arg, Thr. Arms take their numbers in
                              // the order they are BUILT, not the order they
                              // were named in: binary was asked for first
                              // (2026-09-16) and is 7, percentage next
                              // (2026-09-17) and is 8, the file was built next
                              // (2026-09-18, for the storyline generator) and is
                              // 9, and the braced list is 10 -- asked for the
                              // same day, to give satellite.feedback({"a","b"})
                              // a shape to arrive in.
                              // 11  satellite_float
                              // 12  satellite_hexadecimal_number
                              >;

    enum Kind : std::size_t {
        nothing = 0,
        boolean = 1,
        number = 2,
        string = 3,
        bytecode = 4,
        capsule = 5,
        user_defined = 6,
        binary = 7,
        percentage = 8,
        file = 9,
        list = 10,
        how_many_kinds = 11
    };

    static_assert(std::variant_size_v<Held> == how_many_kinds, "Kind must name every arm of Held");
    static_assert(std::is_same_v<std::variant_alternative_t<number, Held>, satellite_number>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<string, Held>, satellite_string>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<bytecode, Held>, satellite_bytecode>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<user_defined, Held>, UserDefinedHandle>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<binary, Held>, satellite_binary_number>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<percentage, Held>, satellite_percentage>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<file, Held>, FileHandle>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<list, Held>, ListHandle>, "");

    Held held;

    satelliteObject() = default;
    satelliteObject(bool from) : held(from) {}
    satelliteObject(satellite_number from) : held(std::move(from)) {}
    satelliteObject(satellite_string from) : held(std::move(from)) {}
    satelliteObject(satellite_bytecode from) : held(std::move(from)) {}
    satelliteObject(satelliteCapsule from) : held(std::move(from)) {}
    satelliteObject(UserDefinedHandle from) : held(std::move(from)) {}
    satelliteObject(satellite_binary_number from) : held(std::move(from)) {}
    satelliteObject(satellite_percentage from) : held(std::move(from)) {}
    satelliteObject(FileHandle from) : held(std::move(from)) {}
    satelliteObject(ListHandle from) : held(std::move(from)) {}

    static satelliteObject of_nothing() { return satelliteObject(); }
    static satelliteObject of_bool(bool from) { return satelliteObject(from); }
    static satelliteObject of_number(satellite_number from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_string(satellite_string from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_bytecode(satellite_bytecode from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_capsule(satelliteCapsule from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_user_defined(UserDefinedHandle from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_binary(satellite_binary_number from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_percentage(satellite_percentage from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_file(FileHandle from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_list(ListHandle from) { return satelliteObject(std::move(from)); }
    // A machine code is a number, as it already was in bytecode/value.hpp.
    static satelliteObject of_code(signed long long int code)
    {
        return satelliteObject(satellite_number::from_signed(code));
    }
    // UTF-8 straight in, for the one place a literal arrives as bytes: the
    // lexer's counted payload. Answers string_error (4) with the bad offset.
    static signed long long int of_utf8(const std::string &utf8, satelliteObject &out, std::size_t &bad_offset);

    // WHICH ARM: one integer, and the test is one compare.
    Kind kind() const { return static_cast<Kind>(held.index()); }
    bool is_nothing() const { return held.index() == nothing; }
    bool is_bool() const { return held.index() == boolean; }
    bool is_number() const { return held.index() == number; }
    bool is_string() const { return held.index() == string; }
    bool is_bytecode() const { return held.index() == bytecode; }
    bool is_capsule() const { return held.index() == capsule; }
    bool is_user_defined() const { return held.index() == user_defined; }
    bool is_binary() const { return held.index() == binary; }
    bool is_percentage() const { return held.index() == percentage; }
    bool is_file() const { return held.index() == file; }
    bool is_list() const { return held.index() == list; }

    // THE ARM, OR nullptr. std::get_if and never std::get: a wrong guess answers
    // nullptr rather than throwing, and satellite does not run on exceptions.
    const bool *as_bool() const { return std::get_if<bool>(&held); }
    const satellite_number *as_number() const { return std::get_if<satellite_number>(&held); }
    const satellite_string *as_string() const { return std::get_if<satellite_string>(&held); }
    const satellite_bytecode *as_bytecode() const { return std::get_if<satellite_bytecode>(&held); }
    const satelliteCapsule *as_capsule() const { return std::get_if<satelliteCapsule>(&held); }
    const UserDefinedHandle *as_user_defined() const { return std::get_if<UserDefinedHandle>(&held); }
    const satellite_binary_number *as_binary() const { return std::get_if<satellite_binary_number>(&held); }
    const satellite_percentage *as_percentage() const { return std::get_if<satellite_percentage>(&held); }
    // THE FILE ITSELF, or nullptr: through the handle, so a const value still
    // reaches a file that can change -- which is what a reference type is.
    satellite_file *as_file() const
    {
        const FileHandle *handle = std::get_if<FileHandle>(&held);
        return handle != nullptr ? handle->get() : nullptr;
    }

    // THE HANDLE ITSELF, because a list is passed on rather than reached into:
    // a library scenario is handed the whole list. satellite_list.hpp defines
    // what is behind it, so only a file that includes THAT can read the items --
    // which keeps this header free of the type that contains it.
    const ListHandle *as_list() const { return std::get_if<ListHandle>(&held); }

    // THE WRITABLE HANDLE, and the reason it must be reached through a REFERENCE
    // to the object and never a copy is in satellite_list.hpp: copy-on-write asks
    // `use_count() == 1`, and a copy taken on the way here makes that answer no
    // every time. 003 built exactly that and it was dead code for months.
    ListHandle *as_list() { return std::get_if<ListHandle>(&held); }

    satellite_number *as_number() { return std::get_if<satellite_number>(&held); }
    satellite_string *as_string() { return std::get_if<satellite_string>(&held); }
    satellite_bytecode *as_bytecode() { return std::get_if<satellite_bytecode>(&held); }
    satelliteCapsule *as_capsule() { return std::get_if<satelliteCapsule>(&held); }
    UserDefinedHandle *as_user_defined() { return std::get_if<UserDefinedHandle>(&held); }
    satellite_binary_number *as_binary() { return std::get_if<satellite_binary_number>(&held); }
    satellite_percentage *as_percentage() { return std::get_if<satellite_percentage>(&held); }

    // The name of the arm, for a refusal a person has to act on.
    const char *kind_name() const;

    // The UTF-8 bytes of a string arm, for the one place a value leaves the
    // interpreter as bytes: a numbered library's `text` scenario, which takes a
    // std::string (number_row.hpp). Empty for every other arm.
    std::string text_utf8() const;

    // -----------------------------------------------------------------------
    // THE METHODS, BOLTED ON (the author). Each answers a machine code and
    // writes through `out`.
    //
    // NONE OF THEM DO ARITHMETIC. Every one routes on the PAIR of tags to a
    // one-function-one-file header -- number_and_number_add.hpp,
    // number_and_string_add.hpp -- which is where the work is and where it
    // inlines. This class chooses; those files act.
    // -----------------------------------------------------------------------
    signed long long int add(const satelliteObject &other, satelliteObject &out, std::string &why) const;
    signed long long int subtract(const satelliteObject &other, satelliteObject &out, std::string &why) const;
    signed long long int multiply(const satelliteObject &other, satelliteObject &out, std::string &why) const;
    signed long long int divide(const satelliteObject &other, satelliteObject &out, std::string &why) const;
    signed long long int modulus(const satelliteObject &other, satelliteObject &out, std::string &why) const;
    signed long long int power(const satelliteObject &other, satelliteObject &out, std::string &why) const;

    // -1, 0 or 1 through `order`. Refuses a pair with no ordering.
    signed long long int compare(const satelliteObject &other, int &order, std::string &why) const;

    // THE CONVERSIONS, explicit and never automatic (DESIGN 1.1).
    signed long long int to_string(satellite_string &out, std::string &why) const;
    signed long long int to_number(satellite_number &out, std::string &why) const;
    signed long long int to_binary(satellite_string &out, std::string &why) const;
    signed long long int to_hexadecimal(satellite_string &out, std::string &why) const;

    // SAMENESS IS WRITTEN OUT ARM BY ARM AND NOT LEFT TO std::variant, for two
    // reasons and the second is the one that matters.
    //
    // The first is that it does not compile: variant's own operator== is
    // constexpr and reaches every arm through a visit, and satellite_string's
    // comparison is an out-of-line call (satellite_string.cpp), so the visit
    // cannot be formed. g++ 14's diagnostic for that is forty lines naming a
    // lambda inside <variant>, which is worth never seeing again.
    //
    // The second is that ONE ARM MUST NOT BE COMPARED THE WAY THE REST ARE. Two
    // objects are the same object when they are AT THE SAME ADDRESS -- a
    // spacesuit is a reference type (DESIGN 7.4), so sameness is identity and
    // never a field-by-field walk. Writing that by hand is how it stays true;
    // the default would quietly have become whatever std::shared_ptr's == does,
    // which happens to be right today and would not be if a deep compare were
    // ever bolted on.
    friend bool operator==(const satelliteObject &l, const satelliteObject &r);
    friend bool operator!=(const satelliteObject &l, const satelliteObject &r) { return !(l == r); }
};

// THE PAIR OF TAGS AS ONE INTEGER, which is what every binary method switches
// on. Dense, fits a switch, and the case labels read like the filenames:
// `case pair_of(number, string):` sits above the call to number_and_string_add.
// THE ARM BY ITS C++ TYPE, which is what lets object_pair.hpp be a template at
// all: `as_number()` names the arm in the function's name, `arm_of<satellite_number>`
// names it in a type parameter, and a template can only do the second.
template <typename Arm>
inline const Arm *arm_of(const satelliteObject &value)
{
    return std::get_if<Arm>(&value.held);
}

inline constexpr std::size_t pair_of(satelliteObject::Kind left, satelliteObject::Kind right)
{
    return (std::size_t)left * (std::size_t)satelliteObject::how_many_kinds + (std::size_t)right;
}

} // namespace satellite004
