// The parser's codes, its notes and its suggestions -- the half of M5 that is
// about real output rather than about the machinery. See
// tests/reporter_test/reporter_test.hpp.
//
// A REPORTER TESTED ONLY AGAINST DIAGNOSTICS THE TEST WROTE IS A REPORTER
// TESTED AGAINST NOBODY. Everything here goes through parse(), so what is
// checked is the sentence a person actually gets -- which is also the only way
// to catch the failure a code registry makes possible: a site raising the wrong
// row. The sentence renders perfectly and describes a different problem.

#include "reporter_test.hpp"

#include "error_reporter/report.hpp"
#include "parser/parser.hpp"

#include <string>
#include <vector>

namespace reporter_test {

namespace {

using satellite::errors::Code;

// A capsule around a statement, because DESIGN §6 allows only declarations at
// the top level and a bare statement has nowhere to be.
std::string in_main(const std::string &statement)
{
    return "satellite.capsule fixture()\n{\n    " + statement + "\n}\n";
}

Code code_of_first(const std::string &source)
{
    const std::vector<satellite::errors::Diagnostic> found = problems_in(source);
    return found.empty() ? Code::NONE : found.front().code;
}

std::string suggestion_in(const std::string &source)
{
    const std::vector<satellite::errors::Diagnostic> found = problems_in(source);
    return found.empty() ? std::string() : found.front().suggestion;
}

} // namespace

void section_parsing()
{
    using namespace satellite::errors;

    // THE CODE EACH RULE RAISES, ONE PER ROW OF errors.def THAT THE PARSER OWNS.
    // A site raising a neighbouring code is the one defect a message registry
    // adds that bespoke strings did not have, and nothing but this catches it.
    check(code_of_first("satellite.console.display(\"x\")\n") ==
              Code::PARSE_EXPECTED_DECLARATION,
          "a statement at the top of a file names the four forms a file holds");
    check(code_of_first(in_main("satellite.statement 3\n")) ==
              Code::PARSE_STATEMENT_NEEDS_A_WORD,
          "satellite.statement with no word after it");
    check(code_of_first(in_main("satellite.statement.else { }\n")) ==
              Code::PARSE_ELSE_WITHOUT_IF,
          "a stray else is named rather than lumped in with `no such statement`");
    check(code_of_first(in_main("satellite.statement.wihle (x) { }\n")) ==
              Code::PARSE_NO_SUCH_STATEMENT,
          "a misspelled statement");
    check(code_of_first(in_main("satellite.capsule inner()\n{\n}\n")) ==
              Code::PARSE_DECLARATION_IN_BLOCK,
          "a declaration inside a block says which declaration it was");
    check(code_of_first(in_main("1 = 2\n")) == Code::PARSE_ASSIGN_TARGET,
          "assigning to something that is not a place");
    // IN A PARAMETER LIST AND NOT IN A STATEMENT, because DESIGN §6.1 dispatches
    // a statement on segment 1: `satellite.console.x y` inside a block never
    // reaches type() at all, it parses as an expression and then finds a second
    // statement on the line. A parameter is one of the four places the grammar
    // asks for a type outright, and it is where this rule is actually reachable.
    check(code_of_first("satellite.capsule f(satellite.console.x y)\n{\n}\n") ==
              Code::PARSE_NOT_A_TYPE,
          "a satellite. path that is not a type space, in type position");
    check(code_of_first(in_main("satellite.container.list<satellite.variable."
                                "string>= x\n")) == Code::PARSE_GENERIC_CLOSE_GE,
          "`>=` closing a generic is one token, and the fix is a space");
    check(code_of_first(in_main("x = y[]\n")) == Code::PARSE_EXPECTED_SUBSCRIPT,
          "empty brackets");
    check(code_of_first(in_main("x = *\n")) == Code::PARSE_EXPECTED_EXPRESSION,
          "an operator where a value goes");
    check(code_of_first("satellite.capsule satellite.notmain()\n{\n}\n") ==
              Code::PARSE_NOT_A_LANGUAGE_CAPSULE,
          "the reserved capsule name arm");
    check(code_of_first(in_main("x = 1 y = 2\n")) ==
              Code::PARSE_STATEMENT_ALREADY_ENDED,
          "two statements on one line");
    check(code_of_first("satellite.capsule f()\n{\n") ==
              Code::PARSE_EXPECTED_PUNCT,
          "a block that is never closed");
    check(code_of_first("satellite.spacesuit 3\n{\n}\n") ==
              Code::PARSE_EXPECTED_WORD,
          "a name that is not a word");
    check(code_of_first("satellite.spacesuit s\n{\n    3 = 4\n}\n") ==
              Code::PARSE_EXPECTED_FIELD_OR_CAPSULE,
          "a spacesuit body holding something that is neither");
    check(code_of_first("satellite.capsule f()\n{\n"
                        "    satellite.statement.if (x) { }\n"
                        "    satellite.statement.else 3\n}\n") ==
              Code::PARSE_ELSE_NEEDS_A_BLOCK,
          "an else followed by neither a block nor an if");
    check(code_of_first("satellite.capsule f(3 x)\n{\n}\n") ==
              Code::PARSE_EXPECTED_TYPE,
          "a parameter whose type is not one");

    // NINETEEN OF THE PARSER'S TWENTY-TWO ERROR ROWS ARE ABOVE, AND THE OTHER
    // THREE ARE NOT REACHABLE FROM A PROGRAM. Naming them is the point: a row
    // with no fixture reads as one somebody forgot.
    //
    //   PARSE_EXPECTED_STATEMENT and PARSE_EXPECTED_SUIT_ITEM are the
    //     "no rule may leave the cursor where it found it" guards in block()
    //     and suit_body(). Every rule that declines either consumes or reports,
    //     and a report sets `panic_`, which suppresses these -- so reaching one
    //     means a rule returned without doing either, which is a defect in the
    //     parser and not a shape a file can have. `@` inside a block answers
    //     S0231 and inside a suit answers S0207, checked above.
    //   PARSE_NAME_UNNUMBERABLE needs words::Words::define() to refuse after
    //     find() has already said the name is free, which it does only for an
    //     empty name -- and an empty name never reaches define_name(), because
    //     expect_word() has failed first.
    //
    // All three stay, because a guard that fires is worth a sentence rather
    // than a crash, and errors.def has the room.

    // DID YOU MEAN, AT THE FOUR PLACES THE PARSER LOOKS A WORD UP IN THE TRIE.
    // DESIGN §6.3 says the parser resolves nothing, so these four are the whole
    // of it: every other path in a program is a Member chain nobody has walked.
    check(suggestion_in(in_main("satellite.statement.wihle (x) { }\n")) == "while",
          "satellite.statement.wihle -- did you mean while?");
    check(suggestion_in("satellite.capsule f(satellite.varable.number x)\n"
                        "{\n}\n") == "variable",
          "satellite.varable in type position -- did you mean variable?");
    check(suggestion_in("satellite.capsul f()\n{\n}\n") == "capsule",
          "satellite.capsul at the top of a file -- did you mean capsule?");
    check(suggestion_in("satellite.capsule satellite.mian()\n{\n}\n") == "main",
          "satellite.mian as a reserved capsule name -- did you mean main?");

    check(suggestion_in(in_main("satellite.statement.qqqq (x) { }\n")).empty(),
          "and a word unlike any statement gets no suggestion at all");

    // THE UNCLOSED BRACKET NOTE, which is the note nine call sites share and
    // the reason `expect_punct` grew a parameter rather than nine sites growing
    // a composed sentence.
    {
        const std::string source = "satellite.include(satellite\n";
        const std::string out = rendered(source);
        check(holds(out, "error S0201:"), "the closer is what is missing");
        check(holds(out, "note S0290:"), "and a note points at the opener");
        check(holds(out, "this `(` is the one that is not closed"),
              "naming the character, because a file may have several kinds open");
        check(line(out, 5) == caret_row(18),
              "with its caret under the `(` and not under the error");
    }

    // A BLOCK'S BRACE IS THE SAME NOTE ON A DIFFERENT LINE, which is what makes
    // it worth having: the opener is nowhere near the error.
    {
        const std::string out = rendered("satellite.capsule f()\n{\n    x = 1\n");
        check(holds(out, "note S0290:"), "the unclosed brace gets a note");
        check(holds(out, source_row(2, "{")),
              "and the note quotes line 2, which is where the `{` is");
    }

    // A NAME DECLARED TWICE POINTS AT THE FIRST DECLARATION. This is the note
    // that needed a table -- parser_internal.hpp's `declared_at_` -- and it is
    // the one place in this parser where the useful second place is not a few
    // tokens back but a line somewhere else in the file.
    {
        const std::string source = "satellite.capsule helper()\n{\n}\n"
                                   "satellite.capsule helper()\n{\n}\n";
        const std::string out = rendered(source);
        check(holds(out, "error S0242:"), "declared twice");
        check(holds(out, "note S0291:"), "with a note at the first one");
        check(holds(out, "helper was declared here"), "naming it");
        check(holds(out, source_row(1, "satellite.capsule helper()")),
              "and quoting line 1, which is the line the person has to look at");
    }

    // A NAME THE LANGUAGE OWNS IS A DIFFERENT SENTENCE AND HAS NO SUCH NOTE,
    // because there is no line in this file to point at.
    {
        const std::string out = rendered("satellite.capsule main()\n{\n}\n");
        check(holds(out, "error S0241:"), "the language owns `main` under library");
        check(!holds(out, "note S0291:"),
              "and there is no first declaration in this file to point at");
    }

    // ONE ERROR PER SYNCHRONISATION, which is M4's rule and is unchanged --
    // and now the SECOND error is a separate diagnostic rather than a second
    // sentence, so a reader gets two blocks and not one run-on.
    {
        const std::vector<Diagnostic> found =
            problems_in("satellite.capsule f()\n{\n    x = *\n    y = *\n}\n");
        check(found.size() == 2, "two bad statements, two diagnostics");
    }

    // AND WHEN THERE ARE TOO MANY, THE READER IS TOLD. MILESTONES/M4.md §6
    // item 8 recorded the cap of 20 as a number in parser.cpp and in no
    // document; it is still that file's number, and what M5 adds is that
    // reaching it is said out loud instead of the output just stopping.
    {
        std::string many = "satellite.capsule f()\n{\n";
        for (int i = 0; i < 30; i++)
            many += "    x = *\n";
        many += "}\n";
        const std::vector<Diagnostic> found = problems_in(many);
        size_t raised = 0;
        for (const Diagnostic &problem : found)
            if (problem.is_error())
                raised++;
        check(raised >= 20 && raised < 30,
              "thirty bad statements stop at the cap rather than all being "
              "reported");
        check(found.back().code == Code::PARSE_TOO_MANY_ERRORS,
              "and the note that says so comes last");
        check(!found.back().is_error(),
              "as a note, so `ok()` still asks whether anything is an ERROR "
              "rather than counting");
    }

    // A CLEAN PROGRAM PRODUCES NOTHING AT ALL, which is what `satl --check`
    // prints on success and is the check that would fail if a diagnostic ever
    // got raised for something that is fine.
    check(problems_in("satellite.include(satellite)\n\n"
                      "satellite.capsule satellite.main()\n{\n"
                      "    satellite.console.display(\"Hello, World!\")\n"
                      "    satellite.return(satellite)\n}\n").empty(),
          "hello world says nothing");
}

} // namespace reporter_test
