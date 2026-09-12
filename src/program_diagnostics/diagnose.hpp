#pragma once

// WHAT A PERSON WOULD FIND BY READING THE PROGRAM, found by reading the program.
//
// THE FIFTH PASS, AND THE FIRST ONE THAT IS NOT ON THE ROAD TO RUNNING. Lex,
// parse, resolve and compile each exist because the next one cannot start
// without them; this one exists because a program that compiles can still be
// wrong in ways the four passes have no reason to look for. Nothing here is
// required to produce a running program and nothing here can stop one -- it is
// asked by `satl --check` and by nobody else.
//
// AN ANALYSER IS A CATALOGUE AND NOT AN INTELLIGENCE, which is the honest
// shape of the thing and is worth writing down before the second check is
// added. "Find the logical errors" is not a computable request; "does any suit
// reach itself through its fields" is. clang-tidy and Coverity are not clever,
// they are LONG -- a few hundred named patterns, each with a caret and a code.
// So this module grows one file per question, every question is answerable by
// walking something the earlier passes already built, and a question nobody can
// state precisely does not get added. satellite_words/words.def and
// error_reporter/errors.def are the same idea about two other lists.
//
// MOST OF WHAT IT FINDS IS A WARNING AND THAT IS THE POINT. A cycle among
// spacesuits is a leak, and a program that leaks is still a program the user
// asked to run -- DESIGN §1.1 is never doing anything behind their back, not
// refusing what they asked for. `stops_the_work()` in codes.hpp decides the
// exit status; SAT_WARNING rows are printed and change nothing.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "evaluator/evaluate.hpp"
#include "name_resolver/resolve.hpp"

#include <vector>

namespace satellite::diagnostics {

// Every check, over one compiled program. Never throws; answers what it found.
//
// ALL THREE PRODUCTS ARE TAKEN EVEN THOUGH TODAY'S ONE CHECK USES ONE. The
// questions this module is for divide by which pass can answer them -- a
// reference cycle is a fact about resolved TYPES, an unreached branch is a fact
// about compiled OPS, and a caret for either is a fact about the TREE -- so the
// signature is the set of things a check may ask, decided once.
std::vector<errors::Diagnostic> diagnose(const Ast &ast,
                                         const resolve::Resolved &resolved,
                                         const eval::Program &program);

} // namespace satellite::diagnostics
