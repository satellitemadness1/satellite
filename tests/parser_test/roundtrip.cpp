// The acceptance programs, parsed and printed back. See
// tests/parser_test/parser_test.hpp for what round-trip means.
//
// THIS IS THE MILESTONE'S DONE-WHEN. PLAN M4 states it in one clause --
// "`satl --unparse file.satl` round-trips, which is how we know the parser is
// right before anything can run" -- and LAYOUT.md says what the files in
// example/ are: "not samples. Each of these is what a milestone means by done."
//
// A TEST WHOSE SUBJECT IS A FILE MUST FAIL LOUDLY WHEN IT CANNOT READ THAT
// FILE, NEVER SKIP. That is words_test's rule for WORD_NUMBERS.md and
// lexer_test's for example/hello_world.satl, and it is the stale-binary lesson
// in a third place: a suite that quietly passes over its own inputs reports a
// green line about nothing.
//
// TWO OF THE SIX PROGRAMS IN example/ ARE NOT HERE, and that is a statement
// about them rather than about the parser -- MILESTONES/M4.md §6 names both,
// the constructs are `my_superclass():` and `800x600`, and neither is in
// DESIGN §6's grammar or DESIGN §8.5's literals. They are checked below as
// what they are: files that do not parse, with the reason pinned to the line.

#include "parser_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "abstract_syntax_tree/unparse.hpp"
#include "parser/parser.hpp"

#include <fstream>
#include <sstream>
#include <string>
#include <vector>

namespace parser_test {

namespace {

bool read_file(const std::string &path, std::string &into)
{
    std::ifstream file(path, std::ios::binary);
    if (!file)
        return false;
    std::ostringstream all;
    all << file.rdbuf();
    into = all.str();
    return true;
}

// Parse, print, parse the printout, print that. The two printouts must be
// identical -- see unparse.hpp for why this and not "the output equals the
// input".
void round_trips(const std::string &path)
{
    std::string source;
    if (!read_file(path, source)) {
        check(false, "cannot read " + path +
                         " -- the acceptance program is an INPUT to this test");
        return;
    }

    const Program first = run(source);
    if (!first.ok()) {
        check(false, path + " does not parse: " + first.first_error());
        return;
    }
    const std::string once = satellite::unparse(first.ast());

    const Program second = run(once);
    if (!second.ok()) {
        check(false, path + " does not parse after being printed back: " +
                         second.first_error());
        return;
    }
    const std::string twice = satellite::unparse(second.ast());

    check(once == twice, path + " is a fixpoint under parse-then-print");
    check(first.ast().size() == second.ast().size(),
          path + " comes back as the same number of nodes, which is what says "
                 "the printer dropped nothing");
    check(first.words.defined() == second.words.defined(),
          path + " defines the same names the second time round");
}

// DESIGN §3's fenced block, or the empty string.
//
// A TEXT PASS OVER THE DOCUMENT AND NOT A MARKDOWN PARSER. What it looks for
// is the heading DESIGN.md's own table of sections gives -- `## 3. Hello
// world` -- and then the first ```satellite fence under it, which is how every
// program in that file is written. Anything looser would find a different
// block on the day §3 grows a second one; anything stricter would fail on a
// heading somebody retitled, and a test that breaks when prose is edited is a
// test that gets deleted.
std::string design_section_three(const std::string &document)
{
    const std::string heading = "\n## 3. Hello world\n";
    const size_t at = document.find(heading);
    if (at == std::string::npos)
        return std::string();

    const std::string opener = "```satellite\n";
    const size_t opens = document.find(opener, at);
    if (opens == std::string::npos)
        return std::string();
    const size_t body = opens + opener.size();

    const size_t closes = document.find("\n```", body);
    if (closes == std::string::npos)
        return std::string();
    // The newline the closing fence sits on is the program's last, so it is
    // kept -- a .satl file ends with one and the comparison is byte for byte.
    return document.substr(body, closes + 1 - body);
}

} // namespace

void section_roundtrip()
{
    // -- The four acceptance programs ----------------------------------------

    round_trips(example_directory + "/hello_world.satl");
    round_trips(example_directory + "/advanced.satl");
    round_trips(example_directory + "/thread_test.satl");
    round_trips(example_directory + "/super_advanced.satl");

    // -- hello world, character for character --------------------------------

    {
        // NOT A FIXPOINT CHECK BUT AN EQUALITY ONE, and the difference matters:
        // a fixpoint says the printer and the parser agree with each other, and
        // this says they agree with DESIGN §3. What is allowed to differ from
        // the file is exactly what the lexer discards -- comments (§5.6) and
        // blank lines, which are not tokens.
        std::string source;
        if (!read_file(example_directory + "/hello_world.satl", source)) {
            check(false, "cannot read hello_world.satl");
            return;
        }
        const Program program = run(source);
        check(program.ok(), "hello world parses: " + program.first_error());
        check(satellite::unparse(program.ast()) ==
                  "satellite.include(satellite)\n"
                  "\n"
                  "satellite.capsule satellite.main(satellite.container.list<"
                  "satellite.variable.string> arguments)\n"
                  "{\n"
                  "    satellite.console.display(\"Hello, World!\")\n"
                  "    satellite.return(satellite)\n"
                  "}\n",
              "hello world prints back as DESIGN §3 writes it, less the two "
              "comments and the blank lines the lexer does not keep");
    }

    // -- and DESIGN §3 IS the file, byte for byte ----------------------------

    {
        // THE CHECK ABOVE CANNOT MAKE THIS CLAIM, WHICH IS WHY BOTH ARE HERE.
        // It compares the file against §3 TRANSCRIBED INTO A STRING LITERAL
        // twenty lines up -- so the parser and the printer are held to §3, and
        // §3 itself is free to drift from the program it says it is a copy of.
        // Three things would then disagree with each other in two directions,
        // and the tree's rule is that "prose may explain a number; it may never
        // be the only place the number lives."
        //
        // PLAN M17 IS WHY IT IS THIS FILE AND NOT ANOTHER. Its done-when is
        // "example/hello_world.satl, rather than a paragraph describing one",
        // and DESIGN §3 answers that by BEING the file: "the two must not be
        // able to drift; the way to guarantee that is for this section to be a
        // copy rather than a description." A copy is an intention until
        // something compares them, and until M17 nothing did.
        std::string document;
        std::string source;
        if (!read_file(design_document, document)) {
            check(false, "cannot read " + design_document +
                             " -- DESIGN §3 is an INPUT to this test");
        } else if (!read_file(example_directory + "/hello_world.satl", source)) {
            check(false, "cannot read hello_world.satl");
        } else {
            const std::string section = design_section_three(document);
            check(!section.empty(),
                  "DESIGN §3 has a ```satellite block under `## 3. Hello "
                  "world` -- if this fails the heading moved, and the check "
                  "below is not being made at all");
            check(section == source,
                  "DESIGN §3's program IS example/hello_world.satl, byte for "
                  "byte, comments included -- §3 says so about itself and PLAN "
                  "M17 rests its done-when on it");
        }
    }

    // -- A string literal survives its escapes -------------------------------

    {
        // lexer.hpp keeps BOTH halves of a string literal and says why: `str`
        // is the value and `text` is what the file says, and expansion is not
        // reversible. A printer that read the value would rewrite the source --
        // this is that claim, checked through the parser.
        const Program program = in_capsule("f(\"a\\nb\")");
        check(program.ok(), "a string with an escape parses: " + program.first_error());
        const satellite::NodeIndex node =
            first_of(program.ast(), satellite::NodeKind::String);
        check(program.ast().text_of(node) == "a\\nb",
              "the ESCAPE comes back, not the newline it expands to -- \"a\\nb\" "
              "and a body with a real newline in it are the same SatString and "
              "only one of them is legal to print back");
        check(program.ast().token_of(node).str.size() == 3,
              "while the value beside it is three characters, expanded");
    }

    // -- The Bits width is part of the value ---------------------------------

    {
        const Program program = in_capsule("f(x0009)");
        check(program.ok(), "a hex literal parses: " + program.first_error());
        check(program.ast().text_of(first_of(program.ast(),
                                             satellite::NodeKind::Bits)) == "x0009",
              "and DESIGN §8.5's width survives the round trip: x0009 is not x9");
    }

    // -- The two files that do not parse, and why ----------------------------

    {
        std::string source;
        if (!read_file(example_directory + "/class_test.satl", source)) {
            check(false, "cannot read class_test.satl");
        } else {
            const Program program = run(source);
            check(!program.ok(),
                  "example/class_test.satl does NOT parse, and the construct is "
                  "`satellite.spacesuit my_superclass():` -- DESIGN §6 writes the "
                  "superclass as `[ \"(\" IDENT \")\" ]`, so an empty pair is not "
                  "the form for 'no superclass' and the colon is in no rule at all");
            // THE LINE COMES OFF THE SPAN NOW rather than off a token index --
            // M5 replaced ParseError with errors::Diagnostic, and a span is
            // what a caret is drawn from, so the line is a field rather than a
            // lookup.
            check(program.parse.errors.front().at.line == 4,
                  "and the first error is on line 4, which is that line");
        }
    }

    {
        std::string source;
        if (!read_file(example_directory + "/gui_example.satl", source)) {
            check(false, "cannot read gui_example.satl");
        } else {
            const Program program = run(source);
            check(!program.ok(),
                  "example/gui_example.satl does NOT parse, and the construct is "
                  "`800x600` -- which the lexer reads as Number(800) then "
                  "Bits(x600), correctly, because DESIGN §8.5 gives `x` literals "
                  "to hex and no document in this tree has a dimensions literal");
        }
    }
}

} // namespace parser_test
