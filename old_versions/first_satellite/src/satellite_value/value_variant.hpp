#pragma once

#include "satellite_value/value_arguments.hpp"
#include "satellite_value/value_result.hpp"
#include "satellite_value/value_types.hpp"

namespace satellite {

// `double` is deliberately absent, per §8.1: satellite.variable.number is an
// exact decimal (bignum.hpp), so there is no binary float anywhere in the
// language's value space and no way for one to leak in. Number's deleted float
// constructors are what enforce that at the C++ level — `Value v = 3.14` is a
// compile error rather than the silent truncation to 3 that §8.1 measured.
//
// BitsRef is index 9 and was appended for exactly the reason below, not because
// binary and hex belong after a map. §21. ArgsRef is index 10 and was appended
// for the same reason -- it is not that the arguments object belongs after a
// hex literal, it is that there is nowhere else it may go. ResultRef is index
// 11 and is the third to make that argument: satellite.container.result is
// Satellite Orbit's answer, and appending is the only way to add one.
//
// APPEND ONLY. MapRef is index 8 because it was added last, not because a map
// belongs at the end: help_for() and module_of() switch on RAW variant indices,
// so inserting an alternative renumbers every alternative after it and silently
// changes what every one of those switches means.
//
// A shared_ptr is 16 bytes and the variant already holds five of them, so
// appending ArgsRef and then ResultRef leaves sizeof(Value) at 40 and the
// static_assert in value_layout.hpp keeps holding. An alternative that grew it
// would be refused on that ground alone: §8.1's Number migration and §10's
// Expr/Value split both rest on 40.
//
// TWO SWITCHES IN THE WHOLE TREE read a raw index -- module_of() in
// src/evaluator/types.cpp and help_for() in src/evaluator/help.cpp -- and both
// gained a `case 11:` with this alternative. A third place to check is
// reg_test, whose static_assert on variant_size is the tripwire that catches an
// alternative added and never converted; it fired for this one, which is what
// it is for.
using ValueBase = std::variant<std::monostate, bool, Number, Str, ListRef,
                               ObjectPtr, Time, FilePtr, MapRef, BitsRef,
                               ArgsRef, ResultRef>;

// One node that can hold anything the language has so far:
//   satellite.variable.bool / .number       -> the scalar alternatives
//   satellite.variable.string               -> SatString (32-bit satellite chars)
//   satellite.container.list                -> the List alternative
// This is the seed of the "satellite_expression" idea — when the parser
// exists, expression node kinds (call, assign, ...) join this same variant.
struct Value : ValueBase {
    using ValueBase::ValueBase;
};

} // namespace satellite
