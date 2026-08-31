#pragma once

// `satl --resolve` -- the resolver's consumer, in the milestone that writes it.
//
// PLAN M2 ASKS FOR THIS BY NAME AND EVERY MILESTONE SINCE HAS ONE. The first
// satellite shipped three commits where its word registry had no reader at all
// and four defects accumulated behind a guarantee nothing was checking, so the
// rule is that the thing built gets a consumer in the same commit. `satl
// --words` was M2's, `--tokens` M3's, `--unparse` M4's, `--satc` M4.5's,
// `--check` M5's, `--limits` M6's, and this is M7's.
//
// IT IS THE STRONGEST CASE OF THAT RULE SO FAR, because a slot is invisible in
// a way a token and a tree are not. `--tokens` prints a list somebody can read
// against the file and `--unparse` prints a program satl can read back; a frame
// is a decision about storage that produces no text at all until M9 puts values
// in it. DESIGN §7.1's failure is the argument: the first satellite's registry
// gave a recursive capsule ONE slot for the whole program and returned 1 for
// every input, and nothing about the source, the tokens or the tree says so.
// The frame is where it would have been visible.
//
// THE ONLY PART OF THE MODULE THAT PRINTS, which is the seam
// satellite_words/dump.cpp, lexical_analyzer/dump.cpp, error_reporter/dump.cpp
// and machine_limits/dump.cpp are all cut on.

#include "abstract_syntax_tree/ast.hpp"
#include "name_resolver/resolve.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite::resolve {

// Every frame, every path, and the two counts. `path` names the file it is
// about, because the first line says so and a caller should not have to.
std::string dump_text(const std::string &path, const Ast &ast,
                      const words::Words &words, const Resolved &resolved);

} // namespace satellite::resolve
