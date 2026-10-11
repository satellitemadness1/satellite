#pragma once
// satellite/arguments/argument_value.hpp -- ONE VARIANT, ANYTHING SATELLITE CAN
// MAKE. SATELLITE_ARGUMENTS A7-A10.
//
// The author's brief, 2026-09-17: "the variant object in the class can be of
// these types: satellite string, satellite number, satellite.variable.bool,
// satellite.variable.binary, satellite.variable.hex, unsigned long long int,
// signed long long int, std::string, satelliteObject (you may use a shared ptr
// here), satelliteUserDefinedObject or a satelliteUserDefinedSpacesuit, so the
// argument can be absolutely anything that we can make in satellite, some of
// these will not make sense just write every class_name in to the std::variant,
// so that we can just hold anything".
//
// TWO OF THOSE NAMES DO NOT EXIST, and SATELLITE_ARGUMENTS Part 2 says so:
//   satellite.variable.hex   is a CONVERSION (number_to_hexadecimal.hpp), not a
//                            class. satelliteObject reserves arm 10 for
//                            satellite_hexadecimal_number and it is unbuilt, so
//                            it is left out rather than invented here. Red
//                            note 2 is the author's to close.
//   satelliteUserDefinedSpacesuit  is spelled satelliteSpacesuit (one of ours or
//                            one the user defined) and satelliteUserDefinedObject
//                            (held as UserDefinedHandle, a shared_ptr, because
//                            DESIGN 7.4 makes a spacesuit a reference type).
//                            Both arms are here; a third name is not invented.
//
// RULING R5: NOTHING IS ADDED TO satelliteObject::Held. This is its own variant
// in its own file. satelliteObject's Kind IS its variant index and a
// static_assert enforces it, so an arm appended there renumbers nothing but buys
// nothing here either -- an argument is not an expression value.
//
// ARMS ARE APPENDED, NEVER INSERTED, for the same reason satelliteObject says
// it: ArgumentKindOf below is the index, and an arm put in the middle silently
// renumbers every arm after it. The static_asserts make that a compile error.

#include "../satellite_object/satellite_object.hpp"
#include "../satellite_object/satellite_spacesuit.hpp"
#include "../satellite_object/satellite_capsule.hpp"
#include "../satellite_variable_binary/satellite_binary_number.hpp"
#include "../satellite_variable_number/satellite_number.hpp"
#include "../satellite_variable_percentage/satellite_percentage.hpp"
#include "../satellite_variable_string/satellite_string.hpp"

#include <cstddef>
#include <string>
#include <type_traits>
#include <variant>

namespace satellite004 {

// EVERY ARM, IN ORDER. The order IS ArgumentKindOf.
using ArgumentValue = std::variant<std::monostate,           //  0  nothing yet
                                   // A8 -- satellite's own types
                                   satellite_string,         //  1  satellite.variable.string
                                   satellite_number,         //  2  satellite.variable.number
                                   bool,                     //  3  satellite.variable.bool
                                   satellite_binary_number,  //  4  satellite.variable.binary
                                   satellite_percentage,     //  5  satellite.variable.percentage
                                   // A9 -- the plain ones, for a fact read
                                   // straight out of the machine that has no
                                   // reason to become a satellite_number first
                                   unsigned long long int,   //  6  a count
                                   signed long long int,     //  7  a count that can go under
                                   std::string,              //  8  text
                                   // A10 -- the object arms
                                   satelliteObject,          //  9  one expression's worth
                                   UserDefinedHandle,        // 10  one object of a spacesuit
                                   satelliteSpacesuit,       // 11  the class itself
                                   satelliteCapsule          // 12  a capsule as a value
                                   // APPEND HERE, NEVER INSERT ABOVE.
                                   >;

// The index of each arm, by name, so nothing reads a bare number.
enum class ArgumentKindOf : std::size_t {
    nothing = 0,
    string = 1,
    number = 2,
    boolean = 3,
    binary = 4,
    percentage = 5,
    count = 6,
    signed_count = 7,
    text = 8,
    object = 9,
    user_defined = 10,
    spacesuit = 11,
    capsule = 12
};

// THE ARM AT EACH INDEX IS THE ARM ArgumentKindOf NAMES. Asked of the TYPE and
// not of a constructed value, because most of these arms are not literal types
// and cannot be built in a constant expression -- satellite_number holds a
// pointer for a big number, so `ArgumentValue(satellite_number()).index()` is
// not constant and does not compile. variant_alternative_t asks the same
// question of the type alone, and asks it more strictly.
template <ArgumentKindOf kind, typename T>
inline constexpr bool argument_arm_is =
    std::is_same_v<std::variant_alternative_t<std::size_t(kind), ArgumentValue>, T>;

static_assert(argument_arm_is<ArgumentKindOf::nothing, std::monostate>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::string, satellite_string>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::number, satellite_number>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::boolean, bool>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::binary, satellite_binary_number>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::percentage, satellite_percentage>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::count, unsigned long long int>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::signed_count, signed long long int>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::text, std::string>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::object, satelliteObject>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::user_defined, UserDefinedHandle>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::spacesuit, satelliteSpacesuit>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(argument_arm_is<ArgumentKindOf::capsule, satelliteCapsule>,
              "ArgumentKindOf must be the variant's index -- an arm was inserted, not appended");
static_assert(std::variant_size_v<ArgumentValue> == 13,
              "an arm was added: append it to ArgumentKindOf, name it in argument_kind_name(), and assert it above");

inline ArgumentKindOf kind_of(const ArgumentValue &value)
{
    return ArgumentKindOf(value.index());
}

// The arm's name, for a report and for types_do_not_meet. One entry per arm, in
// the same order, so the table and the variant are read together.
inline const char *argument_kind_name(ArgumentKindOf kind)
{
    switch (kind) {
    case ArgumentKindOf::nothing:      return "nothing";
    case ArgumentKindOf::string:       return "satellite.variable.string";
    case ArgumentKindOf::number:       return "satellite.variable.number";
    case ArgumentKindOf::boolean:      return "satellite.variable.bool";
    case ArgumentKindOf::binary:       return "satellite.variable.binary";
    case ArgumentKindOf::percentage:   return "satellite.variable.percentage";
    case ArgumentKindOf::count:        return "count";
    case ArgumentKindOf::signed_count: return "signed count";
    case ArgumentKindOf::text:         return "text";
    case ArgumentKindOf::object:       return "satellite.object";
    case ArgumentKindOf::user_defined: return "satellite.spacesuit object";
    case ArgumentKindOf::spacesuit:    return "satellite.spacesuit";
    case ArgumentKindOf::capsule:      return "satellite.capsule";
    }
    return "nothing";
}

inline const char *argument_kind_name(const ArgumentValue &value)
{
    return argument_kind_name(kind_of(value));
}

} // namespace satellite004
