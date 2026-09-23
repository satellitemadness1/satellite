#pragma once
// satellite/bytecode/suit_run.hpp -- MAKING AN OBJECT (2026-09-22).
//
// THE DECLARATION IS THE CONSTRUCTION -- 003's rule and the author's shape:
//
//     tagged_report.run_log log(log_path)     a new object, its constructor given log_path
//     tagged_report.run_signal signal         a new object, its constructor given nothing
//     run_log also = log                      NO new object: `also` and `log` are one
//
// A SPACESUIT IS A REFERENCE TYPE (DESIGN 7.4), so `=` shares and never copies. And a
// name declared AGAIN makes a new object and leaves the old one to whatever still holds
// it -- the author's declaration files do exactly that, 14,704 times: a `view_object
// local_view_object` line, its fields set, `list.append(local_view_object)`, and the
// same line again for the next one.
//
// A NEW OBJECT IS ITS FIELDS' INITIALISERS, IN THE ORDER THEY ARE WRITTEN, AND THEN ITS
// CONSTRUCTOR -- 003's order (compile_statements.cpp). An initialiser runs before the
// object has a value to give it and cannot name another field (003's S0511), so the
// fields are made in a frame of their own and moved into the object. A FIELD of a
// spacesuit's type with nothing after its name starts EMPTY, as in 003: four of the
// author's are their own spacesuit's type, and making one would make another forever.

#include "capsule_scopes.hpp"
#include "function_table.hpp"
#include "value.hpp"

#include <bitset>
#include <cstddef>
#include <vector>

namespace satellite004 {

// A NAME, ANY RUN OF `.name`, AND THEN A NAME: `run_log log`, `tagged_report.run_log log`.
// How a statement that declares an object is told from every other statement a name
// starts -- read without making a string, because the walker asks it of every one.
bool an_object_declaration_at(const std::vector<std::bitset<16>> &row, std::size_t at);

// THE STATEMENT, `at` on its first name and left past it. `as_a_field` is true while an
// object's own fields are being made, where `Type name` alone is an empty slot.
signed long long int run_object_declaration(const BytecodeRegistry &registry, const CapsuleTable &capsules,
                                            const FunctionTable &functions, std::size_t which_row, std::size_t &at,
                                            VariableTable &variables, MachineState &state, bool as_a_field);

// A NEW OBJECT OF THE SPACESUIT `suit`: every field's initialiser, then its constructor
// with `arguments`. Answers success, or the code the program stopped on.
signed long long int make_an_object(const BytecodeRegistry &registry, const CapsuleTable &capsules,
                                    const FunctionTable &functions, std::size_t suit, std::vector<Value> arguments,
                                    MachineState &state, UserDefinedHandle &made);

// DOES THIS SHAPE NAME A SPACESUIT ANYWHERE -- itself, or between its < and >?
bool names_a_suit(const TypeShape &shape);

} // namespace satellite004
