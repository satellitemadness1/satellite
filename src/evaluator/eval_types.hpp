#pragma once

// Part of src/evaluator/eval.hpp — include THAT, never this. The umbrella
// kept its path and its meaning; these are only the pieces it is assembled from.
//
// What is here is everything the Evaluator declaration stands on: the Console it
// may write to, the activation record, the error record, and the status a
// satellite.return reports. The include block below is the original one, moved
// whole so the umbrella still hands its includers exactly what it always did.

#include <optional>

#include "abstract_syntax_tree/ast.hpp"
#include "environment/env.hpp"
#include "satellite_value/value.hpp"

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace satellite {

// Only ever held as a pointer here, so the declaration is enough and
// <thread> stays out of every translation unit that includes this one.
class Console;

// One capsule activation.
//
// No mutex and no atomic, deliberately: a frame is reachable from exactly one
// thread. That is the entire difference from satellite.library, and it is what
// turns §6's 1585/1600 into 0/1600.
//
// It sits here rather than beside CapsuleInfo in env.hpp, which is where it was
// written, because it is the one thing in the resolver's output that mentions a
// Value — and an activation is the tree walker's idea, not the resolver's. The
// compiler shares resolve() and has no frames at all; a local there is an
// `alloca`. See the note at the top of env.hpp.
struct Frame {
    std::vector<ValuePtr> slots;
};

struct EvalError {
    std::string message;
    Span span;
};

// satellite.return is a status, not a C++ exception. Measured in §10: 8.5 ns
// as an enum against 1537 ns thrown — 181× — which at one return per call is
// seconds of pure unwinding in any recursive program. Exceptions stay reserved
// for genuine errors, and errors here are not exceptional enough to throw:
// they are recorded and unwound by returning null.
enum class Flow {
    Normal,
    Return,
};

} // namespace satellite
