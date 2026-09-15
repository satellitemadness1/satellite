#pragma once

// `satl --compile` -- the evaluator's consumer, in the milestone that writes it.
//
// PLAN M2 ASKS FOR THIS BY NAME AND EVERY MILESTONE SINCE HAS ONE. `--words`
// was M2's, `--tokens` M3's, `--unparse` M4's, `--satc` M4.5's, `--check` M5's,
// `--limits` M6's, `--resolve` M7's, `--number` M8's, and this is M9's.
//
// IT IS THE SECOND-STRONGEST CASE OF THAT RULE, AFTER `--resolve`. A closure
// tree is invisible in the way a frame is: it produces no text, `--unparse`
// prints the same program with or without it, and the arena it lives in is
// thrown away when the process ends because PLAN §2.3 refuses to serialise one.
// Without this there is no way to see that a call became an index, that a
// literal became a constant, or that a `while` became one op -- which are the
// three things this milestone is for.
//
// AND IT PRINTS THE REFUSALS, WHICH IS THE PART A USER WILL ACTUALLY READ. Every
// op_refuse in the listing is a piece of DESIGN §6's grammar this evaluator does
// not run yet, with the milestone that will build it. `satl --compile` over a
// program is therefore a report on how much of it M9 can do, which is a question
// every milestone between here and M28 leaves somebody asking.
//
// THE ONLY PART OF THE MODULE THAT PRINTS, which is the seam
// satellite_words/dump.cpp, lexical_analyzer/dump.cpp, error_reporter/dump.cpp,
// machine_limits/dump.cpp and name_resolver/dump.cpp are all cut on.

#include "abstract_syntax_tree/ast.hpp"
#include "evaluator/evaluate.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::eval {

// Every op, every capsule and every refusal. `path` names the file it is about,
// because the first line says so and a caller should not have to.
std::string dump_text(const std::string &path, const Ast &ast,
                      const words::Words &words, const Program &program);

} // namespace satellite::eval
