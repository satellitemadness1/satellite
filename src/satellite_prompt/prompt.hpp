#pragma once

// The prompt -- PLAN M22. What `satl --repl` runs.
//
// WHAT THIS MILESTONE IS, in PLAN §8's own words: "the prompt, the prompt's
// Ctrl-C -- the byte 0x03, because raw mode turns ISIG off and the signal never
// arrives -- and the exit words", plus the window that stops closing. The four
// files under it do the work; this one is the loop.
//
// THE BANNER IS opening_text() AND NOT A LITERAL, which programs/opening.hpp
// asked for three milestones before there was anything to ask: "RETURNS A STRING
// RATHER THAN PRINTING, because it has a second caller coming. At M22 this same
// text becomes the REPL's banner, and the first satellite's mistake was to write
// that banner as a separate literal -- which then drifted, and said 0.1 for
// months while --version said 002." This is that second caller.

#include <string>

namespace satellite::prompt {

// Runs until an exit word, Ctrl-D, or the end of input. Answers the process's
// exit status.
int run_prompt();

// Whether a line is one of the exit words -- `exit`, `quit`, `exit()`,
// `quit()`. Exposed for tests/prompt_test.
//
// FOUR SPELLINGS AND THEY ARE v1's, which is the reason they are these four
// rather than a shorter set: `old_versions/first_satellite/src/programs/
// main_repl.cpp` accepts exactly these, so a person who used the first
// satellite finds the word they already know. PLAN M22 calls them "the exit
// words" and does not enumerate them; this is the enumeration, written where it
// can be checked.
//
// AND `help` IS DELIBERATELY NOT ONE OF THEM. A bare word belongs to the user
// (DESIGN §1), so the prompt recognising one is a cost paid only where it buys
// something: leaving a session is a thing a person must be able to do without
// knowing any of the language yet. Asking for help is not -- there is a path
// for it, and pointing at the path is better than adding a second spelling.
bool is_exit_word(const std::string &line);

} // namespace satellite::prompt
