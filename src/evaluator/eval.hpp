#pragma once

// The satellite evaluator: a tree walker over the AST the parser produces.
// No bytecode, no lowering pass — the tree stays the tree.
//
// Storage comes in exactly two kinds, and which one a name uses is decided
// before the walk starts, by resolve() (env.hpp):
//
//   * a TOP-LEVEL variable lives in satellite.library under one namespace,
//     which is what makes
//
//         satellite.variable.number x = 1
//
//     observable as satellite.library.main.x — the check that the evaluator
//     wrote into the real Library rather than a side table (§10);
//
//   * a CAPSULE LOCAL lives in a frame slot, reached by integer index, with
//     one frame per activation (§6).
//
// The second kind is not an optimisation. Routing locals through the Library
// instead is measured in §6 to make a recursive fact() return 1 for every
// input, and to make eight threads running a capsule with no recursion and no
// shared state produce 1585 wrong results out of 1600: the Library gives
// atomicity, and locals need isolation.
//
// An Evaluator therefore requires the ResolveResult for the Program it is
// about to run, and resolve() must already have finished — see the contract on
// Name::slot in ast.hpp.

// This file kept its path and its meaning and is now assembled from its parts,
// which are included in the order that compiles: the types the class stands on,
// then the class, then the free functions that speak in terms of both. Every
// declaration below arrived verbatim; nothing outside this directory changed.

#include "evaluator/eval_types.hpp"
#include "evaluator/eval_evaluator.hpp"
#include "evaluator/eval_functions.hpp"
