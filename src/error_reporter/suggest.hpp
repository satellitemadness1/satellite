#pragma once

// "Did you mean" over the trie level that failed -- DESIGN §4.6, part of
// report.hpp.
//
// §4.6 IS WHERE THIS IS SPECIFIED AND IT IS SPECIFIED AS A PAYOFF OF THE
// NUMBERING, not as a nicety: "a failed walk knows WHICH SEGMENT failed and
// WHICH NODE it failed under, so satellite.consle.display answers `no consle
// under satellite -- did you mean console?` by running edit distance over that
// one node's children. The first satellite's answer was `no such module
// function: satellite.consle.display`" -- a sentence that repeats the question.
//
// SO THE SEARCH IS ONE NODE'S CHILDREN AND NEVER THE LANGUAGE. That is the
// whole reason the answer is any good: 254 paths contain something within two
// edits of almost any word, and a suggester that searched all of them would
// answer `satellite.hex` for a misspelled statement. words_walk.hpp's `under`
// is what keeps the candidate list to the four words that could have gone there.
//
// IT LIVES HERE AND NOT IN satellite_words/ FOR ONE REASON WORTH KEEPING.
// LAYOUT.md and 040-sources.mk both record that everything under
// satellite_words/ except dump.cpp is constexpr data and pure functions over
// it, so a future `.satc` reader or disassembler can read the numbering without
// linking anything. Edit distance needs a scratch buffer and a policy about how
// close is close enough; the policy is a message's business, which is this
// module's, and putting it next door would cost the registry that property for
// nothing.

#include "satellite_words/words.hpp"

#include <cstddef>
#include <string_view>

namespace satellite::errors {

// The child of `under` most likely to be what `word` was meant to be, or empty.
//
// ALIASES ARE CANDIDATES TOO, and leaving them out would be a suggester that
// disagrees with the walk that just failed: `hexadecimal` is a real spelling of
// satellite.variable.hex (WORD_NUMBERS §2.3), so `hexadecimel` has an answer and
// it is not `hex`.
//
// The view points into the frozen tables, so it outlives every caller.
std::string_view suggest(words::PathId under, std::string_view word);

// The edit distance the suggester uses, exposed because the threshold is the
// half worth testing directly.
//
// OPTIMAL STRING ALIGNMENT AND NOT PLAIN LEVENSHTEIN, which is one extra arm
// and is the difference between this being useful and being decoration. A
// transposition is the commonest typing mistake there is, and plain Levenshtein
// charges two edits for it: `wihle` against `while` scores 2, which is over the
// threshold for a five-letter word, so the one suggestion a person most wants
// is the one they would not get. With the transposition arm it scores 1.
//
// The distance is capped: anything at or past `kTooFar` is not a suggestion at
// any length, and the loop stops paying for a word it has already ruled out.
size_t distance(std::string_view a, std::string_view b);

inline constexpr size_t kTooFar = 4;

// Whether a word that far from a candidate that long is worth offering.
//
// ONE EDIT, PLUS ONE FOR EVERY FOUR CHARACTERS. A rule rather than a constant,
// because a fixed cap is wrong at both ends: 2 offers `for` for `if` (three
// letters, two edits, and nothing about them is alike), and 1 refuses
// `argumentz` for `argumentts`. Measured against the language's own words --
// `while` for `wihle` is 1 of 2, `console` for `consle` is 1 of 2, `capsule`
// for `capsul` is 1 of 2, and `if` for `x` is 2 of 1, which is refused.
constexpr bool close_enough(size_t edits, size_t longest)
{
    return edits != 0 && edits <= 1 + longest / 4;
}

} // namespace satellite::errors
