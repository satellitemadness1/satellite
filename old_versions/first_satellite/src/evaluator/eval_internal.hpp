#pragma once

// Private to src/evaluator/. Nothing outside this directory may include it — the public
// surface is src/evaluator/eval.hpp, and that has not changed.
//
// eval.cpp was 2208 lines. Split at the seams the code already had, into twelve
// files none of which reaches 400. The boundaries are not arbitrary: the first
// four below are the RUNTIME — the method and module surface that a bytecode VM
// calls unchanged — and the rest are the tree walker, which a VM replaces. §17's
// migration wants exactly that seam, so drawing it here makes the later
// extraction a matter of moving files rather than untangling one.
//
//   runtime, survives a VM     methods.cpp mutators.cpp modules.cpp help.cpp
//                              helpers.cpp types.cpp
//   the walker, a VM replaces  session.cpp stmt.cpp slots.cpp expr.cpp
//                              calls.cpp operators.cpp
//
// The helpers below lived in an anonymous namespace when there was one
// translation unit. They are declared here and defined once now, because
// internal linkage cannot cross a file boundary. That is the only semantic
// change the split makes; every function body moved verbatim.

#include "evaluator/eval.hpp"

#include "satellite_library/library.hpp"
#include "system_facts/system.hpp"
#include "system_facts/interrupt.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite {

// Recursion accounting. Defined here rather than in one .cpp because three of
// the split files construct one; it was an anonymous-namespace type when they
// were all the same translation unit.
struct DepthGuard {
    explicit DepthGuard(int &depth) : depth_(depth) { depth_++; }
    ~DepthGuard() { depth_--; }

    DepthGuard(const DepthGuard &) = delete;
    DepthGuard &operator=(const DepthGuard &) = delete;

private:
    int &depth_;
};

// --- helpers.cpp -----------------------------------------------------------

// A dotted path as written, flattened to its segments. False when the
// expression is not a plain path (a call or an index in the middle of one).
bool flatten_path(const Expr &expr, std::vector<std::string> &out);

// What a duration literal says when it appears anywhere except the argument of
// satellite.console.display. One function because two callers report it — the
// walker, for `x = 100ms`, and the call form, for `foo(100ms)` — and a rule
// with two spellings of its own error is a rule users learn twice.
std::string duration_misuse(const std::string &text);

// The same, for a named argument in a position that does not take one.
std::string named_arg_misuse(const std::string &name);
std::string join_path(const std::vector<std::string> &path);

// The value a declaration starts at, per its type.
Value default_of(const Type &type);

bool value_equals(const Value &a, const Value &b);

// An exact non-negative integer index, or false. Never truncates silently.
bool as_index(const Value &v, long long &out);
void clamp_range(long long &lo, long long &hi, long long len);

ValuePtr make_value(Value v);
ValuePtr make_value(SatString s);
ValuePtr make_value(List items);
ValuePtr make_value(MapBody body);

// --- maps.cpp --------------------------------------------------------------

// A map key's canonical bytes, or false when the value cannot be a key.
//
// Declared here rather than in maps.cpp because value_equals needs it: two maps
// are equal when they hold the same canonical keys, and helpers.cpp would
// otherwise have to reimplement the canonical form. One definition, or the two
// drift and equality stops agreeing with lookup. §8.6 is the contract.
bool map_key_of(const Value &v, std::string &out);

// The two writes, as pure functions of the current body. They take no lock and
// touch no slot: the read-modify-write protocol lives once, in
// Evaluator::update_through_slot, and calls these while holding whatever that
// storage kind requires.
bool map_with(const MapBody &current, const ValuePtr &key, const ValuePtr &value,
              MapBody &next, std::string &error);
bool map_without(const MapBody &current, const ValuePtr &key, MapBody &next,
                 std::string &error);

// The names call_map_method answers to. Beside the table it describes, for the
// reason is_mutator is: a row added there is a row here, and a list that lives
// in another file is a list that goes stale.
//
// A RESULT BORROWS THAT TABLE over its own fields, which is what keeps .keys(),
// .values(), .get() and .length() one implementation rather than two that have
// to agree. This predicate is how methods_containers.cpp knows which names to
// hand over -- and, just as usefully, which to refuse in the RESULT's name
// rather than in the map's.
bool is_map_read_method(const std::string &name);

// satellite.bool.true / .false, and anything else resolved on a
// satellite-rooted path rather than called.
ValuePtr module_constant(const std::vector<std::string> &path);

// Methods that write back through the receiver's storage slot, so the call has
// to know where the receiver LIVES and not merely what it is.
bool is_mutator(const std::string &name);

// --- literals.cpp ----------------------------------------------------------

// Was this argument WRITTEN as `{ ... }`? Only a literal is reshaped by the
// position it lands in: a list that arrived through a variable keeps being the
// list it was, because reshaping it would mean a value changed type on the way
// into a call the program can read and see it did not.
bool is_brace_literal(const Expr *expr);

// What a brace literal means where `type` is declared -- a map when a map was
// asked for, a list otherwise, recursively. Answers `value` unchanged when no
// reshaping applies, and null with `error` set when the shape cannot be made.
ValuePtr shape_literal(const Type &type, const ValuePtr &value,
                       std::string &error);

int read_max_depth();
int read_division_digits();

// --- methods_result.cpp ----------------------------------------------------

// satellite.container.result's own methods: .alternatives(), .lines(), and
// every field by name. std::nullopt means "not one of mine" -- the caller then
// hands the name to the map's read surface over the result's fields.
//
// A free function, on the contract search.hpp opens with: the power and its
// result are free of the Evaluator class, and eval_evaluator.hpp is at the
// 325-line ceiling besides.
std::optional<ValuePtr> result_method(const ValuePtr &recv,
                                      const std::string &name,
                                      const std::vector<ValuePtr> &argv,
                                      std::string &error);

// --- help.cpp --------------------------------------------------------------

std::string help_overview();
std::string help_for(const Value &value);
// What one module answers to, for `satellite.directory` and its siblings.
// Empty for a name that is not a module, which is how the caller tells.
std::string help_for_module(const std::string &module);

// A topic: a thing the language DOES, as against a value's methods or a
// module's functions. Empty when the name is not a topic, which is how both
// callers tell -- src/evaluator/modules.cpp for the quoted spelling and
// src/evaluator/expr.cpp for the bare one.
std::string help_for_topic(const std::string &topic);

// One element per line, with what each element IS when it names something on
// disk. The rendering satellite.container.list.lines() hands back, and the one
// the REPL echoes a list with.
std::string list_lines(const List &items);

// What is in a spaceship, counted from its tokens. Answers the report; sets
// `error` and answers empty when the file cannot be read.
std::string analyze_spaceship(const std::string &path, std::string &error);

// --- methods.cpp, modules.cpp ----------------------------------------------

std::string arity_message(const char *module, const std::string &name,
                          size_t want, size_t got);

} // namespace satellite
