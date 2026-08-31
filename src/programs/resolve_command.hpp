#pragma once

// `satl --resolve <file>` -- PLAN M7's consumer. See DESIGN §7 for what resolve
// is and src/name_resolver/dump.hpp for why the pass needs a consumer at all.
//
// WHY IT IS NOT THREE LINES IN main.cpp, WHICH IS THE SAME QUESTION
// cache_command.hpp ANSWERS AND THE SAME ANSWER ONE MILESTONE ON. This arm is
// SATC.md §4's reading order and then a pass over what it produced, and it has
// one thing to decide that no other arm has: what to do when the tree came out
// of a `.satc` and the pass finds something wrong in it. The tree's spans index
// into the `.satc`'s words and not into the file the user typed, so a caret
// drawn from them lands on the wrong line of the right file -- which is worse
// than no caret. See the .cpp.

#include <string>

namespace satellite {

// Resolve `path` and print every frame, every path and the two counts.
//
// THE EXIT STATUS IS ABOUT THE PROGRAM, which is the rule --satc and --check
// already keep: a cache that missed costs a walk, and what decides the status
// is whether the user's program parsed and resolved.
int resolve_command(const std::string &path);

} // namespace satellite
