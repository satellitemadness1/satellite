#pragma once
// satellite/satl/prompt_run.hpp -- `interpret <file>` and `run <file>` at the
// prompt: a whole program started from a typed line.
//
// (the author, 2026-09-22) "In 003, I would always type "interpret file.satl"
// into the prompt, how do you run a file with the prompt that is there
// currently??" -- and 004's prompt had no way at all. These are 003's two words
// (003 src/satellite_prompt/prompt.cpp, run_file_command), ported with 003's own
// rule: they are THE PROMPT'S words, like `exit`, and not the language's.

#include <string>

namespace satellite004 {

// When `typed` begins with `interpret ` or `run `, runs the file after it with the
// words after that as the program's own, and answers true with the program's exit
// status in `answer`. Answers false, touching nothing, for any other line.
bool run_a_file_from_the_prompt(const std::string &typed, signed long long int &answer);

} // namespace satellite004
