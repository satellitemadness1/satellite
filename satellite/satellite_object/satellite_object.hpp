#pragma once
// satellite/satellite_object/satellite_object.hpp -- THE SATELLITE OBJECT MODEL.
// The two classes the author asked for, 2026-09-16:
//
//   "we are going to have 2 main classes for the satellite object model -- class
//    satelliteObject and class satelliteUserDefinedObject... both are going to
//    hold std::variant<satellite_number, satellite_string, satellite_bytecode>
//    ... a satelliteObject will be something we hand design, and a
//    satelliteUserDefinedObject will be something that they insert the pieces
//    into... let's just bolt our methods onto these two classes."
//
// IT IS A GOD CLASS AND THAT IS THE DECISION, NOT AN ACCIDENT. The author, the
// same day: "it does become a god class, but let's just do it this way because
// this way it's both fast, easy and powerful." One type flows everywhere -- a
// value, a capsule, a program, a user's object are all satelliteObject -- so no
// code in the interpreter ever has to ask which of two families a thing belongs
// to. That is the "easy". The "fast" is below: the tag is an integer, the test is
// an integer compare, and every operation between two arms lives in its own
// header so it inlines into the caller.
//
// RESOURCE MANAGEMENT IS NOT A CONCERN HERE, and that is also the author's
// ruling: "We don't need to be careful and be picky about a 70 byte object, or a
// 20 byte object... I have 64 gigabytes." He is right, and the measurement backs
// it further than the argument does: this object is 80 bytes with the bytecode
// arm and 80 bytes WITHOUT it, because every arm is a handle and the contents
// live on the heap. Including bytecode costs nothing at all.
//
// ARMS ARE APPENDED, NEVER INSERTED, and this is the one rule that must not be
// broken. Kind below is the variant's INDEX, so inserting an arm in the middle
// silently renumbers every arm after it -- exactly the hazard the word codes have
// (a word's code is 4097 plus its ROW in words.tsv, so appending is safe and
// inserting shifts everything). satellite_float, satellite_binary_number and
// satellite_hexadecimal_number go on the END when they are built. The
// static_asserts under Kind are what make a mistake here a compile error rather
// than a wrong answer.
//
// THE TWO CLASSES ARE PARALLEL, NOT NESTED, and that is what makes this compile.
// The author: "2 main classes... both are going to hold std::variant<...>". So
// satelliteObject is defined first and satelliteUserDefinedObject after it,
// holding the same variant plus its fields and its capsules. Nothing points back.
//
// THE ONE THING THIS SHAPE LEAVES OPEN, and it is a real question for the author
// rather than a detail: a user's object cannot be stored in a variable, because a
// variable holds a satelliteObject and satelliteUserDefinedObject is not one of
// its arms. Making it an arm directly DOES NOT COMPILE -- tried, 2026-09-16:
// std::variant needs complete types, so an arm holding std::vector<satelliteObject>
// while satelliteObject is still being defined fails on is_destructible. If user
// objects must live in variables, the arm has to be a HANDLE
// (std::shared_ptr<satelliteUserDefinedObject>), which also gives them the
// reference behaviour M26 already ruled on -- a spacesuit IS a reference, and
// .pointer() copies nothing. See the note beside the reserved arms below.

#include "satellite_bytecode.hpp"
#include "satellite_capsule.hpp"
#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_string/satellite_string.hpp"
#include "../machine/machine_codes.hpp"

#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace satellite004 {

// ---------------------------------------------------------------------------
// satelliteObject -- "something we hand design". Every value in the language.
// ---------------------------------------------------------------------------
class satelliteObject {
public:
    // EVERY ARM OF THE VARIANT, in order. The order IS the Kind below.
    using Held = std::variant<std::monostate,     // 0  nothing
                              bool,               // 1  satellite.bool
                              satellite_number,   // 2  satellite.variable.number
                              satellite_string,   // 3  satellite.variable.string
                              satellite_bytecode, // 4  the numbered program
                              satelliteCapsule    // 5  a capsule as a value
                              // APPEND HERE, NEVER INSERT ABOVE:
                              // 6  satellite_float
                              // 7  satellite_binary_number
                              // 8  satellite_hexadecimal_number
                              // 9  std::shared_ptr<satelliteUserDefinedObject>
                              //    -- a HANDLE and not the object itself: the
                              //    direct arm does not compile (see the header
                              //    note), and a handle is what M26's ruling
                              //    wanted anyway.
                              >;

    enum Kind : std::size_t {
        nothing = 0,
        boolean = 1,
        number = 2,
        string = 3,
        bytecode = 4,
        capsule = 5,
        how_many_kinds = 6
    };

    // The tag and the arm cannot drift apart: these fail to compile if they do.
    static_assert(std::variant_size_v<Held> == how_many_kinds, "Kind must name every arm of Held");
    static_assert(std::is_same_v<std::variant_alternative_t<number, Held>, satellite_number>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<string, Held>, satellite_string>, "");
    static_assert(std::is_same_v<std::variant_alternative_t<bytecode, Held>, satellite_bytecode>, "");

    Held held;

    satelliteObject() = default;
    satelliteObject(bool from) : held(from) {}
    satelliteObject(satellite_number from) : held(std::move(from)) {}
    satelliteObject(satellite_string from) : held(std::move(from)) {}
    satelliteObject(satellite_bytecode from) : held(std::move(from)) {}
    satelliteObject(satelliteCapsule from) : held(std::move(from)) {}

    // Named makers, for the places where a bare literal would not say which arm.
    static satelliteObject of_nothing() { return satelliteObject(); }
    static satelliteObject of_bool(bool from) { return satelliteObject(from); }
    static satelliteObject of_number(satellite_number from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_string(satellite_string from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_bytecode(satellite_bytecode from) { return satelliteObject(std::move(from)); }
    static satelliteObject of_capsule(satelliteCapsule from) { return satelliteObject(std::move(from)); }
    // A machine code is a number, as it already was in value.hpp.
    static satelliteObject of_code(signed long long int code)
    {
        return satelliteObject(satellite_number::from_signed(code));
    }

    // WHICH ARM: one integer, and the test against it is one compare. Nothing on
    // a hot path should do more than this to find out what it is holding.
    Kind kind() const { return static_cast<Kind>(held.index()); }
    bool is_nothing() const { return held.index() == nothing; }
    bool is_bool() const { return held.index() == boolean; }
    bool is_number() const { return held.index() == number; }
    bool is_string() const { return held.index() == string; }
    bool is_bytecode() const { return held.index() == bytecode; }
    bool is_capsule() const { return held.index() == capsule; }

    // THE ARM, OR nullptr. std::get_if and not std::get: a wrong guess answers
    // nullptr rather than throwing, and satellite does not run on exceptions.
    const bool *as_bool() const { return std::get_if<bool>(&held); }
    const satellite_number *as_number() const { return std::get_if<satellite_number>(&held); }
    const satellite_string *as_string() const { return std::get_if<satellite_string>(&held); }
    const satellite_bytecode *as_bytecode() const { return std::get_if<satellite_bytecode>(&held); }
    const satelliteCapsule *as_capsule() const { return std::get_if<satelliteCapsule>(&held); }

    satellite_number *as_number() { return std::get_if<satellite_number>(&held); }
    satellite_string *as_string() { return std::get_if<satellite_string>(&held); }
    satellite_bytecode *as_bytecode() { return std::get_if<satellite_bytecode>(&held); }
    satelliteCapsule *as_capsule() { return std::get_if<satelliteCapsule>(&held); }

    // The name of the arm, for a refusal a person has to act on.
    const char *kind_name() const;

    // -----------------------------------------------------------------------
    // THE METHODS, BOLTED ON (the author). Each one answers a machine code and
    // writes through `out`, so the interpreter's table holds one kind of pointer
    // and a refusal always has somewhere to go.
    //
    // NONE OF THEM DO ARITHMETIC. Every one routes on the pair of tags to a
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

    // THE CONVERSIONS, which are explicit and never happen on their own
    // (DESIGN 1.1: nothing is converted). `to_string` is the one every arm
    // answers, because a refusal has to be able to name what it was given.
    signed long long int to_string(satellite_string &out, std::string &why) const;
    signed long long int to_number(satellite_number &out, std::string &why) const;
    signed long long int to_binary(satellite_string &out, std::string &why) const;
    signed long long int to_hexadecimal(satellite_string &out, std::string &why) const;

    friend bool operator==(const satelliteObject &l, const satelliteObject &r) { return l.held == r.held; }
    friend bool operator!=(const satelliteObject &l, const satelliteObject &r) { return !(l == r); }
};

// THE PAIR OF TAGS AS ONE INTEGER, which is what every binary method switches
// on. left * how_many_kinds + right is dense, fits easily in a switch, and a
// compiler turns it into a jump table. This is why the pair files can be named
// after their two types and found by a reader: the switch reads like the
// filenames do.
inline constexpr std::size_t pair_of(satelliteObject::Kind left, satelliteObject::Kind right)
{
    return (std::size_t)left * (std::size_t)satelliteObject::how_many_kinds + (std::size_t)right;
}

// ---------------------------------------------------------------------------
// satelliteUserDefinedObject -- "something that they insert the pieces into".
// ---------------------------------------------------------------------------
//
// The author: "we need satelliteUserDefinedObject to have spots for
// satelliteCapsule and stuff". Those spots are `capsules`. The pieces a user
// inserts are `fields`, and `value` is the variant both classes hold -- so a
// user's class that IS a number (one that wraps one) has somewhere to keep it
// without a field named by hand.
//
// IT IS DEFINED AFTER satelliteObject AND IS NOT AN ARM OF IT. That is what
// keeps this compiling; the header note at the top says what it costs and what
// the handle would buy.
//
// FIELDS ARE TWO PARALLEL VECTORS AND NOT A MAP, on purpose, and this is the one
// decision here I would defend hardest. A map from name to value PER INSTANCE is
// what makes an interpreter slow: 004 already loses 5x to CPython on `i = i + 1`
// for exactly that reason -- it rebuilds a std::string from 16-bit codes and
// hashes it on every evaluation, where CPython indexes an array slot. The names
// of a class's fields are the same for every instance of it, so the NAME is
// looked up once to get a SLOT and the slot is an integer from then on.
// `field_at` is the fast way in; `slot_of` is the slow way that finds it, and it
// should be called at check time and never in a loop.
struct satelliteUserDefinedObject {
    std::string class_name;
    satelliteObject value;                   // the variant, which both classes hold
    std::vector<std::string> field_names;    // the layout: name -> slot
    std::vector<satelliteObject> fields;     // one per name, same order
    std::vector<satelliteCapsule> capsules;  // the spots for methods (the author)

    satelliteUserDefinedObject() = default;
    explicit satelliteUserDefinedObject(std::string its_class) : class_name(std::move(its_class)) {}

    std::size_t how_many_fields() const { return field_names.size(); }
    std::size_t how_many_capsules() const { return capsules.size(); }

    // The slot a name sits in, or npos. THE STRING COMPARE HAPPENS HERE AND
    // NOWHERE ELSE.
    std::size_t slot_of(const std::string &name) const
    {
        for (std::size_t i = 0; i < field_names.size(); ++i)
            if (field_names[i] == name)
                return i;
        return (std::size_t)-1;
    }

    // The fast way in: an integer index, no compare, no hash.
    const satelliteObject *field_at(std::size_t slot) const
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }
    satelliteObject *field_at(std::size_t slot)
    {
        return slot < fields.size() ? &fields[slot] : nullptr;
    }

    const satelliteCapsule *capsule_of(const std::string &name) const
    {
        for (std::size_t i = 0; i < capsules.size(); ++i)
            if (capsules[i].name == name)
                return &capsules[i];
        return nullptr;
    }

    // Inserting a piece. Answers the slot it went into, so a caller that is
    // building a class keeps the integer and never looks the name up again.
    std::size_t insert_field(std::string name, satelliteObject held)
    {
        field_names.push_back(std::move(name));
        fields.push_back(std::move(held));
        return field_names.size() - 1;
    }
    void insert_capsule(satelliteCapsule its_capsule) { capsules.push_back(std::move(its_capsule)); }

    friend bool operator==(const satelliteUserDefinedObject &l, const satelliteUserDefinedObject &r)
    {
        return l.class_name == r.class_name && l.value == r.value &&
               l.field_names == r.field_names && l.fields == r.fields && l.capsules == r.capsules;
    }
    friend bool operator!=(const satelliteUserDefinedObject &l, const satelliteUserDefinedObject &r)
    {
        return !(l == r);
    }
};

} // namespace satellite004
