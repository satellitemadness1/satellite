// DESIGN §4.6, and the transposition that earns it. See
// tests/reporter_test/reporter_test.hpp.
//
// §4.6 IS THE SPECIFICATION AND ITS WORKED EXAMPLE IS THE FIRST CHECK BELOW.
// "satellite.consle.display answers `no consle under satellite -- did you mean
// console?` by running edit distance over that one node's children." Two halves
// have to hold for that sentence to be true: the distance has to be small
// enough to offer, and the CANDIDATE LIST has to be one node's children --
// because 254 paths contain something within two edits of nearly any word, and
// a suggester that searched them all would answer confidently and wrongly.

#include "reporter_test.hpp"

#include "error_reporter/suggest.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace reporter_test {

namespace {

using satellite::words::NodeId;
using satellite::words::PathId;

PathId under(NodeId id) { return static_cast<PathId>(id); }

std::string meant(NodeId parent, const std::string &word)
{
    return std::string(satellite::errors::suggest(under(parent), word));
}

} // namespace

void section_suggesting()
{
    using satellite::errors::close_enough;
    using satellite::errors::distance;
    using satellite::errors::kTooFar;

    // DESIGN §4.6's OWN EXAMPLE, by name.
    check(meant(NodeId::SATELLITE, "consle") == "console",
          "satellite.consle -- did you mean console? (DESIGN §4.6)");

    // THE TRANSPOSITION, AND IT IS WHY THE DISTANCE IS OPTIMAL STRING ALIGNMENT
    // AND NOT PLAIN LEVENSHTEIN. `wihle` against `while` is two edits under
    // Levenshtein -- a delete and an insert -- which is over the threshold for
    // a five-letter word, so the single suggestion a person most often wants
    // would be the one they never got. With the transposition arm it is one.
    check(distance("wihle", "while") == 1,
          "a transposition is ONE edit -- plain Levenshtein charges two and "
          "loses the commonest typing mistake there is");
    check(meant(NodeId::STATEMENT, "wihle") == "while",
          "satellite.statement.wihle -- did you mean while?");

    check(meant(NodeId::STATEMENT, "els") == "else", "els -- else");
    check(meant(NodeId::SATELLITE, "capsul") == "capsule", "capsul -- capsule");
    check(meant(NodeId::SATELLITE, "conosle") == "console", "conosle -- console");

    // AN ALIAS IS A CANDIDATE, because it is a real spelling of a real node
    // (WORD_NUMBERS §2.3) and a suggester that left them out would disagree
    // with the walk that just failed.
    check(meant(NodeId::VARIABLE, "hexadecimel") == "hexadecimal",
          "an alias is something a person can have meant");

    // AN ALIAS THAT CARRIES A DOT IS NOT. `fast.range(min, max)` spans two
    // segments and a call shape, and offering it for one misspelled word is
    // advice nobody can act on -- the same `find('.')` filter the lexer's
    // spelling table uses, and for the same reason.
    check(meant(NodeId::RANDOM, "fast.rang").empty(),
          "a dotted alias is not offered for one wrong segment");

    // THE CANDIDATE LIST IS ONE LEVEL, WHICH IS THE WHOLE OF §4.6's CLAIM.
    // `display` is `satellite.console.display` and is nowhere under
    // `satellite` -- a suggester that walked the language would find it and
    // answer a question nobody asked.
    check(meant(NodeId::SATELLITE, "displya").empty(),
          "a word from another level is not offered -- the search is the failed "
          "node's children and nothing else");
    check(meant(NodeId::CONSOLE, "displya") == "display",
          "and under the right node it is found");

    // A BARE ROW IS NOT A CANDIDATE, AND THE CHECK EXISTS BECAUSE A MUTATION
    // FOUND NOTHING WITHOUT IT. words.def's 39 bare shapes and 6 argument rows
    // have an EMPTY spelling by design -- "such a row has no spelling of its
    // own and is never walked to by name" -- and `satellite.container`'s empty
    // child is declared BEFORE `list`. Drop the guard in suggest() and `ls`
    // matches the empty spelling at the same distance, wins on being first, and
    // then fails the threshold as a zero-length candidate: the suggestion is
    // lost rather than made wrong, which is the kind of regression a test that
    // only asks for good answers cannot see.
    check(meant(NodeId::CONTAINER, "ls") == "list",
          "a bare shape's empty spelling does not displace a real child");

    // NOTHING CLOSE IS NOTHING SAID. A suggestion that is not one is worse than
    // silence: it is a sentence the reader has to evaluate and reject.
    check(meant(NodeId::STATEMENT, "x").empty(), "one letter suggests nothing");
    check(meant(NodeId::SATELLITE, "qqqqqqqq").empty(),
          "a word unlike everything suggests nothing");
    check(meant(NodeId::STATEMENT, "").empty(), "an empty word suggests nothing");

    // A USER'S OWN NAME IS NOT A NODE OF THE LANGUAGE, so it has no children to
    // search. words_runtime.hpp's numbering starts above the frozen table and
    // indexing the frozen child lists with one reads past the end of both --
    // which is the defect M4 found by being M2's first caller.
    check(satellite::errors::suggest(9999, "anything").empty(),
          "a PathId above the frozen table has no children here");

    // THE THRESHOLD, AS A RULE RATHER THAN A CONSTANT. A fixed cap is wrong at
    // both ends: 2 offers `for` for `if`, and 1 refuses a long word one letter
    // out. suggest.hpp measures the rule against the language's own words.
    check(close_enough(1, 5), "one edit in five is close");
    check(close_enough(2, 8), "two edits in eight is close");
    check(!close_enough(2, 3), "two edits in three is a different word");
    check(!close_enough(0, 5),
          "a distance of zero is a match and not a suggestion");

    // The distance itself, in the corners.
    check(distance("", "abc") == 3, "an empty word is its length away");
    check(distance("abc", "abc") == 0, "a word is zero from itself");
    check(distance("abc", "abd") == 1, "one substitution");
    check(distance("abc", "abcd") == 1, "one insertion");
    check(distance("abcdefgh", "hgfedcba") >= kTooFar,
          "a word nothing like another is capped rather than counted to the end");
    check(distance(std::string(80, 'a'), "a") == kTooFar,
          "a word longer than the buffer is refused rather than truncated");
}

} // namespace reporter_test
