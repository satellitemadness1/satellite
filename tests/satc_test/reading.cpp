// §4's reading order, and the fixpoint that ties it to the writer. See tests/satc_test/satc_test.hpp.
//
// THE FIXPOINT IS THE MILESTONE'S DONE-WHEN AND EVERYTHING ELSE HERE SUPPORTS
// IT. M4 stated its own as "printing this output and parsing it again gives the
// same text", which unparse.hpp argues is the strongest statement available
// about a printer; the cache's version is one step longer, because there are two
// printers and a substitution in between. Write a program, read the file back,
// write THAT, and the two files must be identical -- which says that everything
// the writer put into numbers, the reader took back out as the same words.
//
// A WEAKER CHECK WOULD HAVE PASSED OVER THE DEFECT THE WRITER ALREADY HAD.
// `satellite.console.input(">>>", target)` written as `display` reads back
// perfectly and writes out identically, so the fixpoint alone proves nothing
// about MEANING -- section_examples() checking every number against the trie is
// what does that. The two are complementary and neither is enough: one says the
// numbers are the right ones, the other says nothing was lost carrying them.
//
// §5's WRITE IS NEXT DOOR, in tests/satc_test/writing.cpp, because it is the
// one part of this milestone whose subject is a disk rather than a format.

#include "satc_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satc_test {

namespace {

// A source's stamp, made up rather than stat'd, because §2's third line only
// has to be the SAME on both sides of a read for the header to match.
satellite::cache::Source fixture_source()
{
    satellite::cache::Source source;
    source.name = "fixture.satl";
    source.mtime = 1756304412;
    source.size = 142;
    return source;
}

std::string satc_of(const std::string &program, satellite::words::Words &words)
{
    const satellite::Parse parsed = satellite::parse(program, words);
    if (!parsed.ok()) {
        check(false, "fixture did not parse: " +
                         satellite::errors::sentence(parsed.errors.front()));
        return std::string();
    }
    return satellite::cache::satc_text(parsed.ast, words, fixture_source());
}

// THE MILESTONE'S DONE-WHEN, over one program.
void fixpoint(const std::string &what, const std::string &program)
{
    satellite::words::Words first_words;
    const std::string first = satc_of(program, first_words);
    if (first.empty())
        return;

    // A SECOND RUN AND NOT A SECOND CALL. `read_words` is fresh, so every user
    // name in the file is met and numbered again from nothing -- which is the
    // thing §5.2 exists to make survivable and the reason the writer may not
    // sort, group or hoist a declaration.
    satellite::words::Words read_words;
    const satellite::cache::Reading found =
        satellite::cache::read_text(first, fixture_source(), read_words);
    check(found.hit(), what + ": a `.satc` this build wrote is one it can read");
    if (!found.hit())
        return;

    const std::string again = satellite::cache::satc_text(
        found.program.ast, read_words, fixture_source());
    check(again == first, what + ": writing what was read gives the same file");
}

// §4's THREE MISSES AND ITS ONE REFUSAL, each made by bending one field.
//
// THE HEADER IS BENT RATHER THAN HAND-WRITTEN, so that a header line somebody
// adds later is still checked here: every fixture below starts from a file this
// build produced and changes exactly one thing about it.
void header_misses()
{
    satellite::words::Words words;
    const std::string good =
        satc_of("satellite.include(satellite)\n", words);
    if (good.empty())
        return;

    {
        satellite::words::Words fresh;
        std::string bent = good;
        bent.replace(bent.find("satc 1"), 6, "satc 2");
        const satellite::cache::Reading found =
            satellite::cache::read_text(bent, fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::REFUSED,
              "a format version this build does not have is refused");
        check(found.note.code == satellite::errors::Code::SATC_WRONG_VERSION,
              "and the refusal says which version it was written by");
    }

    {
        // A DIGEST THAT MOVED IS THE LOAD-BEARING MISS. §2: "a numbering that
        // has changed at all produces a different digest and every stale
        // `.satc` on the machine stops being read on the same instant."
        satellite::words::Words fresh;
        std::string bent = good;
        bent.replace(bent.find(satellite::words::digest_text()), 16,
                     "0000000000000000");
        const satellite::cache::Reading found =
            satellite::cache::read_text(bent, fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::STALE,
              "a file written against another numbering is a miss");

        // AND IT SAYS NOTHING, which is as much a requirement as the miss is.
        // This happens every time a word is appended to the language, on every
        // cached program on the machine at once; a note here would be a hundred
        // lines of output about a cache doing exactly its job.
        check(found.note.code == satellite::errors::Code::NONE,
              "and a stale file is silent");
    }

    {
        satellite::words::Words fresh;
        satellite::cache::Source other = fixture_source();
        other.size += 1;
        const satellite::cache::Reading found =
            satellite::cache::read_text(good, other, fresh);
        check(found.why == satellite::cache::Miss::STALE,
              "a source that has been edited is a miss");
        check(found.note.code == satellite::errors::Code::NONE,
              "and an edited source is silent too");
    }

    {
        satellite::words::Words fresh;
        const satellite::cache::Reading found = satellite::cache::read_text(
            "this is not a satc file at all\n", fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::MALFORMED,
              "a file with no `satc` line is malformed");
        check(found.note.code == satellite::errors::Code::SATC_NOT_A_SATC,
              "and malformed says so in plain words");
    }

    {
        // §2 IS THREE LINES AND A BLANK ONE, and the blank is what says the
        // header ended. Without it a fourth header line would be read as the
        // first line of the program.
        satellite::words::Words fresh;
        std::string bent = good;
        bent.erase(bent.find("\n\n") + 1, 1);
        const satellite::cache::Reading found =
            satellite::cache::read_text(bent, fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::MALFORMED,
              "a header with no blank line after it is malformed");
    }
}

// A `.satc` THAT CANNOT BE BELIEVED, which is §4's one thing to say out loud and
// the reason PLAN puts this milestone before M5.
void malformed_bodies()
{
    satellite::words::Words words;
    // WRAPPED IN A CAPSULE BECAUSE DESIGN §6 ALLOWS ONLY DECLARATIONS AT THE
    // TOP LEVEL, which is the same reason the harness's statements() exists.
    const std::string good =
        satc_of("satellite.capsule fixture()\n{\n"
                "satellite.console.display(\"x\")\n}\n",
                words);
    if (good.empty())
        return;

    {
        satellite::words::Words fresh;
        std::string bent = good;
        bent.replace(bent.find("#1.5.1"), 6, "#1.99.1");
        const satellite::cache::Reading found =
            satellite::cache::read_text(bent, fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::MALFORMED,
              "a number the numbering does not have is malformed");
        // THE CODE AND THE SENTENCE BOTH, because M5 made these two separable
        // and each can now be wrong on its own: a site can raise a neighbouring
        // row, and a row's holes can be filled in the wrong order.
        const std::string said = satellite::errors::sentence(found.note);
        check(found.note.code == satellite::errors::Code::SATC_UNKNOWN_PATH,
              "and it is the number, rather than the file, that is refused");
        check(said.find("#1.99.1") != std::string::npos,
              "and the sentence names the number: " + said);
        check(found.note.notes.size() == 1 &&
                  found.note.notes.front().code ==
                      satellite::errors::Code::NOTE_SATC_IGNORED,
              "and every malformed `.satc` carries the note that says deleting "
              "it is safe -- which was a suffix four sentences remembered to "
              "append until M5 made it a note they attach");
    }

    {
        satellite::words::Words fresh;
        std::string bent = good;
        bent.replace(bent.find("#1.5.1"), 6, "#");
        const satellite::cache::Reading found =
            satellite::cache::read_text(bent, fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::MALFORMED,
              "a mark with no number after it is malformed");
    }

    {
        // A TRUNCATED FILE IS THE ONE §5's ATOMIC WRITE EXISTS TO PREVENT, so
        // it is worth proving the reader survives one rather than trusting that
        // the write is never interrupted.
        satellite::words::Words fresh;
        const satellite::cache::Reading found = satellite::cache::read_text(
            good.substr(0, good.size() - 3), fixture_source(), fresh);
        check(found.why == satellite::cache::Miss::MALFORMED,
              "a body cut off part way through is malformed");
    }
}

// A MISS MUST COST EXACTLY ONE WALK AND NOTHING ELSE, which is §4's promise and
// is the one way this reader could break a program that has nothing wrong with
// it. Parsing a `.satc` allocates every user name in it; if a file that then
// turned out to be malformed left those names behind, the caller's fall back to
// the SOURCE would meet `helper` a second time and report the user's own
// program as declaring a name twice.
void a_miss_leaves_no_trace()
{
    satellite::words::Words words;
    const std::string good = satc_of(
        "satellite.capsule helper()\n{\nsatellite.return()\n}\n", words);
    if (good.empty())
        return;

    std::string bent = good;
    bent += "satellite.capsule\n";   // a declaration with no name: it will not parse

    satellite::words::Words caller;
    const satellite::cache::Reading found =
        satellite::cache::read_text(bent, fixture_source(), caller);
    check(found.why == satellite::cache::Miss::MALFORMED,
          "the fixture is a `.satc` that does not parse");
    check(caller.find(satellite::words::NodeId::LIBRARY, "helper") ==
              satellite::words::kNoPath,
          "a miss leaves the caller's numbering exactly as it found it");
}

// §3 AND §3.1 SURVIVE THE ROUND TRIP TOO, and a string is the case a textual
// substitution gets wrong. `display("#1.5.1")` prints seven characters; a reader
// that scanned inside literals would turn it into a program that prints a path.
void literals_are_not_scanned()
{
    satellite::words::Words words;
    std::string back;
    satellite::errors::Diagnostic why;
    check(satellite::cache::unnumber("#1.5.1(\"#1.5.1\")\n", back, why),
          "a mark inside a string is not a number: " +
              satellite::errors::sentence(why));
    check(back == "satellite.console.display(\"#1.5.1\")\n",
          "the string is copied and only the code is turned back: " + back);

    // §1.1's COMMENT COLUMN IS "NEVER TRUSTED -- the numbers are the file", so
    // a `#` a person typed into one cannot make a good file malformed.
    check(satellite::cache::unnumber("#1.5.2 // a person wrote #here\n", back,
                                     why),
          "a mark inside a comment is not a number: " +
              satellite::errors::sentence(why));
    check(back.find("satellite.console.input()") == 0,
          "and arity 0 keeps the parentheses the number already says: " + back);
}

} // namespace

void section_reading()
{
    fixpoint("include", "satellite.include(satellite)\n");
    fixpoint("a capsule",
             "satellite.capsule fact(satellite.variable.number n)\n"
             "{\nsatellite.return(n)\n}\n");

    // THE FOUR ACCEPTANCE PROGRAMS, because a fixture is a form and a program
    // is every form at once with the others around it. LAYOUT.md calls these
    // "what a milestone means by done" and this is the milestone that reads
    // them back.
    for (const char *const name : {"hello_world.satl", "advanced.satl",
                                   "thread_test.satl", "super_advanced.satl"}) {
        std::string source;
        if (!read_example(name, source)) {
            check(false, std::string("could not read ") + name);
            continue;
        }
        fixpoint(name, source);
    }

    header_misses();
    malformed_bodies();
    a_miss_leaves_no_trace();
    literals_are_not_scanned();
}

} // namespace satc_test
