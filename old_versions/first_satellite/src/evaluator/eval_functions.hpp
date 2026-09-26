#pragma once

// Part of src/evaluator/eval.hpp — include THAT, never this.
//
// The three free functions of the evaluator's public surface. They are not
// members: matches() and module_of() are asked about a Value by code that has no
// Evaluator, and format_error() renders an EvalError long after the run.

#include "evaluator/eval_types.hpp"

#include <string>

namespace satellite {

// Runtime type check, applied at declaration and at insertion only — not on
// every assignment, and not to slices, which preserve element type by
// construction (§7).
bool matches(const Type &type, const Value &value);

// The module path that owns a value's methods, or nullptr for nil, which has
// no module. Deliberately a table keyed on the variant index rather than
// derivation from the type name: satellite.container.list has two segments and
// satellite.time has one, so the path is not recoverable from the name (§7).
const char *module_of(const Value &value);

// Renders an error with the offending source line and a caret under it. Spans
// are byte offsets, never decoded positions — decode() is neither injective
// nor stable, so a caret computed from decoded text drifts (§10).
//
// Takes the SourceMap rather than one text because this is the call §16 named
// as the one that would otherwise lie: a runtime error inside an included
// spaceship rendered against the includer's text prints line N of the wrong
// file, confidently and with a caret.
std::string format_error(const EvalError &error, const SourceMap &sources);

} // namespace satellite
