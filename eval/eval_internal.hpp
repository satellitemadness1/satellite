#pragma once

// Private to eval/. Nothing outside this directory may include it — the public
// surface is ../eval.hpp, and that has not changed.
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

#include "../eval.hpp"

#include "../library.hpp"
#include "../system.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

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

// satellite.bool.true / .false, and anything else resolved on a
// satellite-rooted path rather than called.
ValuePtr module_constant(const std::vector<std::string> &path);

// Methods that write back through the receiver's storage slot, so the call has
// to know where the receiver LIVES and not merely what it is.
bool is_mutator(const std::string &name);

int read_max_depth();
int read_division_digits();

// --- help.cpp --------------------------------------------------------------

std::string help_overview();
std::string help_for(const Value &value);

// --- methods.cpp, modules.cpp ----------------------------------------------

std::string arity_message(const char *module, const std::string &name,
                          size_t want, size_t got);

} // namespace satellite
