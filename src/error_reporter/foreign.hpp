#pragma once

// "That is C++. Here is how satellite spells it." -- ERROR_HANDLING.md §9.
//
// WHAT THIS IS FOR. Everybody arrives at satellite from somewhere else, and the
// first thing they type is the other language's. `print("hi")` is a parse error
// and the parse error is correct, but "expected a declaration" is an answer to a
// question nobody asked: the reader knows what they wanted, they do not know how
// to say it HERE. This turns the most common wrong guesses into the right line.
//
// IT IS A TABLE AND NOT A PARSER, DELIBERATELY. Recognising C++ properly would
// be a C++ front end; what is wanted is much smaller -- a distinctive substring
// that only appears in one language, and the satellite sentence that replaces
// it. `std::cout` is unmistakable. `for` is not, and is therefore not in here.
//
// THE THIRD COLUMN IS THE INTERESTING ONE. Some foreign constructs have a
// satellite spelling and some do not, and the honest answer differs:
//
//   `println!("{}", x)`   has one -- satellite.console.display(x)
//   `x && y`              does NOT yet -- and that is a MILESTONE, named here
//   `asm { mov eax, 1 }`  does not and never will -- and that is said outright
//
// A row that names a milestone is a PROMISE, so every milestone named below is
// declared in ERROR_HANDLING.md §10 with what it covers. A row may not invent a
// number: if satellite cannot do the thing and no milestone covers it, the row
// says so plainly rather than implying a plan that does not exist.

#include <string>
#include <string_view>

namespace satellite::errors {

// What a foreign line means here.
struct Foreign {
    const char *language;    // "C++", "Python", "Java", "Rust", "assembly"
    const char *pattern;     // the distinctive substring
    const char *satellite;   // the satellite spelling, or nullptr
    const char *milestone;   // "M28" when satellite cannot say it yet, else nullptr
    const char *note;        // one clause of why, or nullptr
};

// The advice for a source line, or empty when nothing recognisable is in it.
//
// EMPTY IS THE COMMON ANSWER AND HAS TO BE. This runs on every rendered
// diagnostic; a matcher that fires on ordinary satellite would put noise under
// every error in the language. A line already containing `satellite.` is never
// matched, which is the cheapest possible guard and covers the case that
// matters -- correct code that happens to contain `new` or `let`.
std::string foreign_advice(std::string_view line);

// The whole table, for `satl --foreign` and for the test that checks every
// milestone named here is declared in ERROR_HANDLING.md.
const Foreign *foreign_table(std::size_t *count);

} // namespace satellite::errors
