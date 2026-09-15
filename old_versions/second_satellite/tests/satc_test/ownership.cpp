// §3 and §3.1: what may NOT become a number. See tests/satc_test/satc_test.hpp.
//
// TWO LINES RUN THROUGH THIS FILE AND THEY ARE NOT THE SAME LINE. §3 is about
// OWNERSHIP -- a capsule the user wrote, a global they declared, the name of a
// parameter -- and the reason a user name stays a name is that its number is
// "valid inside one run only" (WORD_NUMBERS §3), so writing it down would be
// writing down something that is not true tomorrow. §3.1 is about REACHABILITY
// -- `sort` in `my_list.sort()` is language-owned and does have a number, and
// it still may not be written, because that number is reachable only through
// the receiver's type and nothing has decided the receiver's type yet.
//
// GETTING THE SECOND ONE WRONG IS THE FAILURE SATC.md §7 IS THE SENTENCE
// FORBIDDING: a `.satc` that wrote `satellite.container.list.sort(my_list)`
// would name a handler, and the file would stop being a source with the
// dictionary applied and start being bytecode.

#include "satc_test.hpp"

#include <string>

namespace satc_test {

namespace {

void user_names()
{
    // A CAPSULE THE USER WROTE. SATC.md §3's own worked example, checked:
    // `satellite.capsule` is 1.2 because the language owns it, `fact` stays
    // `fact`, and `n` stays `n`.
    const std::string written = body(
        "satellite.capsule fact(satellite.variable.number n)\n{\n"
        "satellite.return(n)\n}\n");
    check(code_of(line_with(written, "#1.2")) == "#1.2 fact(#1.6.4 n)",
          "a user's capsule and parameter stay names: " +
              code_of(line_with(written, "#1.2")));

    // AND THE ONE CAPSULE NAME THE LANGUAGE OWNS. `satellite.main` is 1 3, so
    // the same line that leaves `fact` alone must number this one -- which is
    // what makes the rule ownership and not "the second word is never
    // numbered".
    const std::string main_capsule = body(
        "satellite.capsule satellite.main(satellite.container.list"
        "<satellite.variable.string> arguments)\n{\nsatellite.return(satellite)\n}\n");
    check(code_of(line_with(main_capsule, "#1.2")) ==
              "#1.2 #1.3(#1.4.2<#1.6.1> arguments)",
          "satellite.main is numbered: " + code_of(line_with(main_capsule, "#1.2")));

    // A GLOBAL: the language-owned prefix numbered, the user's segment left as
    // a name, joined with a dot. Checked at its DECLARATION and at its USE,
    // because those are two different code paths and only one of them has the
    // PathId the parser interned.
    const std::string global = body(
        "satellite.library.counter = 0\n\n"
        "satellite.capsule fixture()\n{\n"
        "satellite.console.display(satellite.library.counter)\n"
        "satellite.return(satellite)\n}\n");
    check(code_of(line_with(global, "#1.14.counter =")) == "#1.14.counter = 0",
          "a global is declared as #1.14.<name>: " +
              code_of(line_with(global, "#1.14.counter =")));
    check(code_of(line_with(global, "#1.5.1")) == "#1.5.1(#1.14.counter)",
          "and read back the same way: " + code_of(line_with(global, "#1.5.1")));

    // A SPACESUIT AND ITS SECTIONS. 1.10, 1.11 and 1.12 are the language's; the
    // suit's name and its superclass are the user's.
    const std::string suit = body(
        "satellite.spacesuit my_suit(my_super)\n{\n"
        "satellite.public\n{\n"
        "satellite.capsule show()\n{\nsatellite.console.display(\"x\")\n}\n"
        "}\n}\n");
    check(code_of(line_with(suit, "#1.10")) == "#1.10 my_suit(my_super)",
          "a spacesuit's name and superclass stay names: " +
              code_of(line_with(suit, "#1.10")));
    check(code_of(line_with(suit, "#1.12")) == "#1.12",
          "satellite.public is #1.12: " + code_of(line_with(suit, "#1.12")));
}

void selectors()
{
    // §3.1's WORKED EXAMPLE, BOTH HALVES, ON TWO LINES OF ONE FIXTURE.
    // `satellite.container.list.sort` IS numbered -- it is 1 4 2 3 and
    // words_test checks that against the authority -- and it still may not be
    // written here, because at the instant the writer runs nothing has decided
    // that `my_list` is a list.
    const std::string written = statements(
        "my_list.sort()\n"
        "satellite.console.display(\"x\")\n");
    check(code_of(line_with(written, "sort")) == "my_list.sort()",
          "a selector stays a selector: " + code_of(line_with(written, "sort")));
    check(code_of(line_with(written, "#1.5.1")) == "#1.5.1(\"x\")",
          "and a path on the next line is still numbered");

    // AND THE SUGAR IS NEVER FLIPPED INTO ITS DISPATCH FORM, which is the one
    // thing SATC.md §7 forbids outright. A test for the absence of a string is
    // a weak check in general and the right one here, because the defect it
    // guards against is a writer that produces something PLAUSIBLE.
    check(written.find("#1.4.2.3") == std::string::npos,
          "sort's number never appears in the file");
    check(written.find("(my_list") == std::string::npos,
          "the receiver is never written out as a first argument");

    // A LITERAL OPTION IS NOT FOLDED. WORD_NUMBERS §1.5 lets `sort("down")`
    // intern to a different PathId than `sort("up")` at RESOLVE time; §5.1 says
    // the file keeps the string. "This is the one place the numbering
    // deliberately says more than the file does."
    const std::string down = statements("my_list.sort(\"down\")\n");
    check(code_of(line_with(down, "sort")) == "my_list.sort(\"down\")",
          "a literal option stays a literal: " + code_of(line_with(down, "sort")));

    // A USER'S CAPSULE CALL, which is neither a path nor a selector and is left
    // alone by the same rule as both.
    const std::string called = statements("helper(1, \"two\")\n");
    check(code_of(line_with(called, "helper")) == "helper(1, \"two\")",
          "a user's call is untouched: " + code_of(line_with(called, "helper")));
}

void literals()
{
    // §3: LITERALS STAY LITERAL, and each of these is a different lexer path.
    // The string is printed from its token's `text` -- the body as WRITTEN,
    // escapes unexpanded -- for the reason unparse.cpp gives: expansion is not
    // reversible, so a writer that read the expanded value would rewrite the
    // program it was handed.
    check(one_line("satellite.variable.string s = \"a\\nb\"") ==
              "#1.6.1 s = \"a\\nb\"",
          "a string keeps its escapes unexpanded: " +
              one_line("satellite.variable.string s = \"a\\nb\""));
    check(one_line("satellite.variable.number n = 3.14") == "#1.6.4 n = 3.14",
          "a float is a float: " + one_line("satellite.variable.number n = 3.14"));
    check(one_line("satellite.variable.binary b = x0009") == "#1.6.5 b = x0009",
          "a bits literal keeps its width -- DESIGN §8.5: " +
              one_line("satellite.variable.binary b = x0009"));
    // THE COLLISION THE PATH MARK EXISTS FOR, AND THE ONLY CHECK THAT CAN SEE
    // IT. `satellite.console` is 1 5, which closes up to `1.5`, which is also
    // the float one-and-a-half -- so before paths.hpp's kPathMark these two
    // tokens were spelled identically and a reader had nothing to decide with.
    // Marked, the path is `#1.5` and the literal is `1.5`, and this check fails
    // the moment a writer marks a number that was a value.
    check(one_line("satellite.variable.number half = 1.5") == "#1.6.4 half = 1.5",
          "a float that spells a path is still a float: " +
              one_line("satellite.variable.number half = 1.5"));
    check(one_line("satellite.variable.number half = 1.5").find("#1.5") ==
              std::string::npos,
          "and is never marked as one");
}

} // namespace

void section_ownership()
{
    user_names();
    selectors();
    literals();
}

} // namespace satc_test
