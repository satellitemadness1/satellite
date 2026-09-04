// The two numbers PLAN M2 names, the interner, and what a failed walk says.
// See words_test.hpp.
//
// PLAN M2's done-when is "a test proving satellite.console.display walks to
// 1 5 1, satellite.random.normal to 1 7 2, and that both intern to stable
// PathIds." Those two are checked here BY NAME as well as by authority.cpp's
// sweep of all 227, because a milestone's own acceptance condition should be
// findable by grepping for it rather than by trusting that a loop covered it.

#include "words_test.hpp"

#include "satellite_words/words.hpp"

#include <string>

using namespace satellite::words;

namespace {

PathId walked(const char *path)
{
    const Walk found = walk(path);
    return found.error == WalkError::NONE ? found.id : kNoPath;
}

} // namespace

namespace words_test {

void section_walking()
{
    // --- PLAN M2's two worked examples --------------------------------------
    const Walk display = walk("satellite.console.display");
    check(display.error == WalkError::NONE &&
              number_text(static_cast<NodeId>(display.id)) == "1 5 1",
          "PLAN M2: satellite.console.display must walk to 1 5 1");

    const Walk normal = walk("satellite.random.normal");
    check(normal.error == WalkError::NONE &&
              number_text(static_cast<NodeId>(normal.id)) == "1 7 2",
          "PLAN M2: satellite.random.normal must walk to 1 7 2");

    // "Stable" means two things and both are checked: the same path gives the
    // same id every time, and the id is a language word -- which is what makes
    // it quotable at all. A user's name is neither (WORD_NUMBERS §3).
    check(display.id == walked("satellite.console.display") &&
              normal.id == walked("satellite.random.normal"),
          "a path must intern to the same PathId every time it is walked");
    check(is_language_word(display.id) && is_language_word(normal.id),
          "both must be language words, so their PathIds may be written down");
    check(display.id != normal.id, "two different paths must be two different ids");

    // --- the interner, in both directions -----------------------------------
    //
    // DESIGN §4.4 describes MANY NODES, ONE STRING and WORD_NUMBERS §2.3
    // describes ONE NODE, MANY STRINGS, and the two are easy to conflate. These
    // are the document's own examples of each.
    const PathId container_list = walked("satellite.container.list");
    const PathId directory_list = walked("satellite.directory.list()");
    check(container_list != kNoPath && directory_list != kNoPath &&
              container_list != directory_list,
          "§4.4: `list` under `container` and under `directory` are two nodes");
    check(spelling_id(static_cast<NodeId>(container_list)) ==
              spelling_id(static_cast<NodeId>(directory_list)),
          "§4.4: ... that share one interned spelling");

    const PathId console = walked("satellite.console");
    const PathId window_console = walked("satellite.window.console");
    check(console != window_console &&
              spelling_id(static_cast<NodeId>(console)) ==
                  spelling_id(static_cast<NodeId>(window_console)),
          "§2.4: `console` is spelled twice and is 1 5 and 1 24 2");

    check(intern("console") == spelling_id(static_cast<NodeId>(console)),
          "a bare word must intern to the spelling its nodes share");
    check(intern("satellite") == kSatelliteSpelling,
          "§1's reserved word must be one interner comparison");
    check(intern("consle") == kNoSpelling,
          "a word the language does not have must intern to nothing");
    check(intern("") == kNoSpelling, "the empty spelling is not a word");

    // An argument row has no spelling and must never answer a lookup for one.
    check(intern("(satellite)") == kNoSpelling && intern("()") == kNoSpelling,
          "an argument or a bare shape must not be reachable as a word");

    // --- what a failure says ------------------------------------------------
    //
    // M5 is the error reporter and does not exist. What M2 owes it is enough to
    // be written against without this signature changing, and DESIGN §4.6 is
    // specific about what that is: "no `consle` under `satellite` -- did you
    // mean `console`?" needs the node the segment failed UNDER, so that edit
    // distance runs over that one node's children.
    const Walk consle = walk("satellite.consle.display");
    check(consle.error == WalkError::NO_SUCH_WORD, "a misspelling is NO_SUCH_WORD");
    check(consle.id == kNoPath, "a failed walk returns no id");
    check(consle.under == static_cast<PathId>(NodeId::SATELLITE),
          "§4.6: the failure must name `satellite` as the node it looked under");
    check(consle.offset == 10, "and where in the path the bad segment began");

    // A real word called a way it does not have is a different sentence, and
    // must not be reported as a misspelling.
    const Walk shape = walk("satellite.console.display(a, b, c)");
    check(shape.error == WalkError::NO_SUCH_SHAPE,
          "a real word with no such call shape is NO_SUCH_SHAPE, not a misspelling");
    check(shape.under == static_cast<PathId>(walked("satellite.console")),
          "and it failed under `console`, where `display` lives");

    // AN EMPTY SEGMENT MUST MATCH NOTHING. Found by review 2026-08-28 and it
    // was reachable from the command line: the 39 bare rows and the 6 argument
    // rows are spelled "" by design, so an empty `word` compared equal to every
    // one of them and the walk handed back the bare shape.
    // `satl --words satellite.console.` answered `1 5 0` and exited 0.
    check(walk("satellite.console.").error == WalkError::NO_SUCH_WORD,
          "a trailing dot is not a path to the bare call shape");
    check(walk("satellite.include.(satellite)").error == WalkError::NO_SUCH_WORD,
          "a dot before an argument list is not a path -- the argument joins "
          "with no dot, which is what makes it an argument and not a word");
    check(walk("satellite.container.list.").error == WalkError::NO_SUCH_WORD,
          "and it is not reachable at any depth");
    check(walk("satellite..console").error == WalkError::NO_SUCH_WORD,
          "nor is an empty segment in the middle");

    check(walk("console.display").error == WalkError::NOT_ROOTED,
          "§1: a language-owned name is a path rooted at `satellite`");
    check(walk("satellites.console").error == WalkError::NOT_ROOTED,
          "and `satellites` is not `satellite` with something after it");
    check(walk("satellite.console.display.more").error == WalkError::NO_SUCH_WORD,
          "a leaf has no children to walk into");

    // AN ALIAS ANSWERS BARE, THE WAY THE WORD IT ALIASES DOES. `normal` walks
    // to 1 7 2, its lowest-numbered shape; `normal.range` names exactly one
    // shape and must walk to it rather than failing. Found 2026-08-28 while
    // checking words.def against SCRATCH.md/WORD_SURFACE.md, whose survey
    // writes all three `.range` rows without their arguments.
    for (const char *tier : {"fast", "normal", "ultra"}) {
        const std::string bare = std::string("satellite.random.") + tier + ".range";
        const PathId with = walked((bare + "(min, max)").c_str());
        check(with != kNoPath, bare + "(min, max) must walk");
        check(walked(bare.c_str()) == with,
              bare + " must reach the same node as " + bare + "(min, max)");
    }

    // --- the shapes of one word ---------------------------------------------
    //
    // WORD_NUMBERS §1.3's own example, and the reason a number identifies a
    // CALL SHAPE rather than a path: three things a program can ask `console`
    // for, so three numbers.
    check(number_text(static_cast<NodeId>(walked("satellite.console.input()"))) ==
              "1 5 2", "input() is 1 5 2");
    check(number_text(static_cast<NodeId>(walked("satellite.console.input(prompt)"))) ==
              "1 5 3", "input(prompt) is 1 5 3");
    check(number_text(static_cast<NodeId>(
              walked("satellite.console.input(prompt, target)"))) == "1 5 4",
          "input(prompt, target) is 1 5 4");

    // The other depth, which is the same rule: `include` has a number of its
    // own, so its shapes are its children rather than its siblings.
    check(number_text(static_cast<NodeId>(walked("satellite.include"))) == "1 1",
          "include is 1 1");
    check(number_text(static_cast<NodeId>(walked("satellite.include()"))) == "1 1 0",
          "include() is 1 1 0 -- `0` is a real number meaning nothing there");
    check(number_text(static_cast<NodeId>(walked("satellite.include(satellite)"))) ==
              "1 1 1", "include(satellite) is 1 1 1, because `satellite` is word 1");

    // --- depth is not a special case ----------------------------------------
    const PathId cores = walked("satellite.library.main.arguments.machine.cores");
    check(number_text(static_cast<NodeId>(cores)) == "1 14 1 1 1 1" &&
              depth_of(static_cast<NodeId>(cores)) == 6,
          "§1.4: the deepest path is six segments and interns to one uint32_t");
    check(sizeof(PathId) == 4, "§4.5: a PathId is four bytes");

    // --- the digest ---------------------------------------------------------
    check(kDigest != 0, "the numbering's digest must not be zero");
    check(digest_text().size() == 16 && digest_text() == digest_text(),
          "SATC.md §2: the digest is sixteen fixed-width hex digits");
}

} // namespace words_test
