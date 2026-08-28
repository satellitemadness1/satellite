// The live child counter, and names numbered as they are met. See
// words_test.hpp.
//
// PLAN §8.1 SPLITS M2 IN TWO AND THIS IS THE HALF NO static_assert COVERS. The
// language's words are frozen in words.def and asserted at compile time; a
// user's capsules and spacesuits are numbered at parse time, under the node
// that owns them, and last exactly as long as the program does. This section is
// the only check those get, which is why it is here rather than waiting for the
// parser that will call it at M4.

#include "words_test.hpp"

#include "satellite_words/words.hpp"

#include <string>

using namespace satellite::words;

namespace words_test {

void section_runtime()
{
    const NodeId library = static_cast<NodeId>(walk("satellite.library").id);
    check(library != NodeId::NONE, "satellite.library must exist to number under");

    // WORD_NUMBERS §3's own worked example. `library` has `main` at 1 14 1 and
    // `system` at 1 14 2 frozen, so the first name a program defines under it
    // takes 3.
    check(frozen_children(library) == 2,
          "words.def gives satellite.library two children");

    const NodeId console = static_cast<NodeId>(walk("satellite.console").id);

    Words words;
    check(words.next_free(library) == 3,
          "§3: the next number free under `library` is 3");
    check(words.defined() == 0, "a fresh numbering has met no names");

    const PathId x = words.define(library, "x");
    check(x != kNoPath, "§3: a user capsule takes the next free number");
    check(words.number_of(x) == 3, "and that number is 3 -- satellite.library.x");
    check(words.next_free(library) == 4, "which the counter then moves past");
    check(words.parent_of(x) == library, "and it is numbered under `library`");
    check(words.name_of(x) == "x", "a user's name is kept as a NAME");

    // THE HALF THE FROZEN GUARANTEE DOES NOT REACH, said in a check rather than
    // only in a comment. SATC.md §3 and WORD_NUMBERS §3 both require anything
    // that writes a PathId down to record the name instead when it is the
    // user's, and this is the predicate that tells the two apart.
    check(!is_language_word(x),
          "§8.1: a user's PathId is not a language word and must never be "
          "written into a .satc, a wire format, or anything else that outlives "
          "the run that allocated it");
    check(is_language_word(static_cast<PathId>(library)),
          "and the language's own words are");

    const PathId y = words.define(library, "y");
    check(words.number_of(y) == 4 && y != x, "a second name takes 4");

    // Met again is not defined again. This is the property a parser leans on:
    // it meets a name once per mention and must get one number.
    check(words.intern(library, "x") == x, "a name met twice is one number");
    check(words.defined() == 2, "and does not allocate a second time");
    check(words.define(library, "x") == kNoPath,
          "defining a name twice is refused rather than silently renumbered");

    // A NAME THE LANGUAGE ALREADY OWNS IS REFUSED, IN PLAIN WORDS. Whether a
    // user MAY shadow satellite.library.main is DESIGN §2's reservation rule
    // and is settled at M6's resolve; what M2 must not do is decide it by
    // accident by handing out a number that makes the language's own word
    // unreachable.
    check(words.define(library, "main") == kNoPath,
          "a user name may not take a spelling the language owns here");
    check(words.find(library, "main") == walk("satellite.library.main").id,
          "and looking it up answers with the language's node");
    check(words.next_free(library) == 5, "a refusal takes no number");

    // --- three defects found by review on 2026-08-28 -----------------------
    //
    // THE ALIASES COUNT AS THE LANGUAGE OWNING THE SPELLING. find() scanned
    // only the numbered children, so `args` -- one of DESIGN §7.7's six
    // spellings of `arguments` -- was not recognised as the language's, and
    // intern() allocated a USER number for it. walk() answered 1 14 1 1 for the
    // same spelling under the same parent at the same moment. Two numbers for
    // one word, and a parser would have called the half that was wrong.
    const NodeId main_node = static_cast<NodeId>(walk("satellite.library.main").id);
    const PathId arguments = walk("satellite.library.main.arguments").id;
    for (const char *spelling : {"arg", "args", "argz", "argument", "argumentz"}) {
        check(words.find(main_node, spelling) == arguments,
              std::string("§7.7: `") + spelling +
                  "` is the language's spelling of `arguments`, not a free name");
        check(words.define(main_node, spelling) == kNoPath,
              std::string("§7.7: `") + spelling + "` may not be taken as a user name");
    }
    check(words.find(static_cast<NodeId>(walk("satellite.variable").id),
                     "hexadecimal") == walk("satellite.variable.hex").id,
          "§2.3: `hexadecimal` is the language's second spelling of `hex`");

    // AN EMPTY NAME IS NOT A NAME. The 38 bare rows are spelled "" by design,
    // so an unguarded compare answered one of them -- `find(console, "")`
    // returned satellite.console(), a language word, which define() had always
    // refused and intern() reached find() before ever getting to that refusal.
    check(words.find(console, "") == kNoPath, "an empty name finds nothing");
    check(words.intern(console, "") == kNoPath, "and interns to nothing");
    check(words.define(console, "") == kNoPath, "and defines nothing");

    // The counters are per node, exactly as the numbering is per parent.
    check(words.next_free(console) == frozen_children(console) + 1,
          "§4.2: allocating under `library` cannot move `console`'s counter");

    // A run's names end with the run.
    Words second;
    check(second.next_free(library) == 3 && second.defined() == 0,
          "§8.1: a second numbering starts from the frozen table again, which "
          "is why a user's number is not stable between runs");
}

} // namespace words_test
