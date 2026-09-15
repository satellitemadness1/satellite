// DESIGN §9's block, including the arms nothing raises yet. See
// tests/reporter_test/reporter_test.hpp.
//
// EVERY DIAGNOSTIC HERE IS BUILT BY HAND, and that is possible only because the
// reporter takes a Span -- three integers -- and the text those integers index.
// It is also necessary: a call stack has no producer until M9, a span past the
// end of its text is reachable only by a caller that rendered against the wrong
// file, and a missing argument cannot come out of make() at all. Those are
// three arms of the one function §9 says must be the only one, and a branch
// nothing exercises is a branch that does not work.

#include "reporter_test.hpp"

#include "error_reporter/report.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace reporter_test {

namespace {

using satellite::errors::Code;
using satellite::errors::Diagnostic;
using satellite::errors::Source;
using satellite::errors::Span;

} // namespace

void section_rendering()
{
    using namespace satellite::errors;

    // ONE LINE, ONE CARET, AND THE THREE THINGS A TOOL PARSES. The header's
    // shape is `path:line:column:` because that is what every editor's error
    // parser already reads -- report.cpp argues it -- so this is the check that
    // would fail if somebody made it prettier.
    {
        const std::string text = "satellite.console.display(\"hi\")\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_EXPRESSION>(
            Span{10, 17, 1}, "Punct(.)");
        const std::string out = render(problem, Source{"t.satl", text});

        check(line(out, 0).compare(0, 22, "satl: t.satl:1:11: err") == 0,
              "the header is satl: path:line:column: severity code: sentence");
        check(holds(line(out, 0), "error S0231:"),
              "the code is printed, because a code exists to be looked up");
        check(holds(line(out, 0), "Punct(.)"), "the hole is filled");
        check(line(out, 1) == source_row(1, "satellite.console.display(\"hi\")"),
              "the excerpt is the source line, in a numbered gutter");
        check(line(out, 2) == caret_row(11, 7),
              "the caret is under the span and is as wide as it");
    }

    // A ONE-BYTE SPAN STILL GETS A CARET. `end == start` is what a diagnostic
    // about a place rather than a token has, and a zero-width underline would
    // draw nothing at all.
    {
        const std::string text = "abc\n";
        Diagnostic problem = make<Code::PARSE_GENERIC_CLOSE_GE>(Span{1, 1, 1});
        check(line(render(problem, Source{"t.satl", text}), 2) == caret_row(2),
              "an empty span still draws one caret");
    }

    // THE LINE NUMBER IS THE SPAN'S AND THE COLUMN IS COUNTED, which is what
    // makes a caret on line 12 land under the right character rather than under
    // the twelfth byte of the file.
    {
        const std::string text = "one\ntwo\nthree four\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{14, 18, 3},
                                                             "Word(four)");
        const std::string out = render(problem, Source{"t.satl", text});
        check(holds(line(out, 0), "t.satl:3:7:"), "line 3, column 7");
        check(line(out, 1) == source_row(3, "three four"),
              "and the third line is quoted");
        check(line(out, 2) == caret_row(7, 4), "with the caret at column 7");
    }

    // A TAB IS ONE SPACE IN THE EXCERPT, so the caret's column, the character's
    // column and the number in the header are all the same number. report.cpp
    // argues why a tab stop cannot be guessed: it is a property of the terminal
    // and not of the file, so eight would be wrong on a terminal set to four
    // and both would be confidently wrong.
    {
        const std::string text = "\t\tx = 1\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{2, 3, 1}, "Word(x)");
        const std::string out = render(problem, Source{"t.satl", text});
        check(holds(line(out, 0), ":1:3:"), "a tab counts as one column");
        check(line(out, 1) == source_row(1, "  x = 1"), "and prints as one space");
        check(line(out, 2) == caret_row(3), "so the caret lands on the x");
    }

    // A MULTI-BYTE CHARACTER IS ONE COLUMN. The lexer's spans are BYTE offsets
    // -- lexer.hpp says why, and encode_raw maps one byte to one SatChar -- so
    // the conversion has to happen in the renderer or every caret after an
    // accent is one place right per extra byte.
    {
        const std::string text = "\"caf\xc3\xa9\" x\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{8, 9, 1}, "Word(x)");
        const std::string out = render(problem, Source{"t.satl", text});
        check(holds(line(out, 0), ":1:8:"),
              "a two-byte character is one column, not two");
        check(line(out, 2) == caret_row(8), "and the caret agrees with it");
    }

    // A `\r\n` FILE GETS A CARET AND NOT A PAINTED-OVER LINE. Left in the
    // excerpt, the carriage return sends the cursor back to column 0 and the
    // caret row overwrites the source it is pointing at.
    {
        const std::string text = "x = 1\r\ny = 2\r\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{7, 8, 2}, "Word(y)");
        check(line(render(problem, Source{"t.satl", text}), 1) ==
                  source_row(2, "y = 2"),
              "the carriage return is not part of the line");
    }

    // CLAMPED TO THE LINE, which is the first satellite's hardest-won line in
    // its own format_error and is inherited rather than rediscovered: an
    // unclamped column pads the caret row with as many spaces as the span is
    // past the end. Reachable by rendering a diagnostic against the wrong text,
    // which is a mistake somebody will make.
    {
        const std::string text = "ab\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{4000, 4002, 1},
                                                             "Word(x)");
        const std::string out = render(problem, Source{"t.satl", text});
        check(line(out, 2).size() < 20,
              "a span past the end of its text draws a bounded caret, not four "
              "thousand spaces");
    }

    // A NOTE WITH ITS OWN SPAN -- DESIGN §9's third clause, and the shape the
    // whole `Note` type exists for.
    {
        const std::string text = "satellite.include(\"x\"\n";
        Diagnostic problem = make<Code::PARSE_EXPECTED_PUNCT>(
            Span{21, 22, 1}, ")", "to close satellite.include", "Newline");
        problem.notes.push_back(note<Code::NOTE_OPENED_HERE>(Span{17, 18, 1}, "("));
        const std::string out = render(problem, Source{"t.satl", text});

        check(holds(line(out, 0), "error S0201:"), "the error first");
        check(holds(line(out, 3), "note S0290:"),
              "then the note, indented under it and carrying its own code");
        check(line(out, 5) == caret_row(18),
              "and the note's caret is at the note's span, not the error's");
    }

    // A NOTE WITH NO SPAN IS A SENTENCE AND NOTHING ELSE. The `.satc` reader's
    // four diagnostics are all of this shape -- errors.def argues that a caret
    // pointing into a generated file tells somebody to read a file the next
    // sentence tells them to delete.
    {
        Diagnostic problem = make<Code::SATC_NOT_A_SATC>(kNowhere);
        problem.notes.push_back(note<Code::NOTE_SATC_IGNORED>(kNowhere));
        const std::string out = render(problem, Source{"~/.satl/cache/x.satc", {}});

        check(line(out, 0).compare(0, 27, "satl: ~/.satl/cache/x.satc:") == 0,
              "with no span, the header names the file and stops");
        check(!holds(out, "|"), "and there is no excerpt to draw");
        check(holds(line(out, 1), "note S0390:"), "the note still prints");
        check(holds(out, "deleting the file is safe"),
              "and says what happens next, which is the whole point of saying "
              "anything");
    }

    // NEITHER A PATH NOR A SPAN. What a test renders, and what a diagnostic
    // about the run rather than about a file looks like.
    {
        const Diagnostic problem =
            make<Code::PARSE_TOO_MANY_ERRORS>(kNowhere, 20u);
        const std::string out = render(problem, Source{});
        check(line(out, 0) == "satl: note S0208: 20 problems were reported and "
                              "the rest of the file was not read -- fix these "
                              "and run it again",
              "with no path and no span the header is the severity and the "
              "sentence");
    }

    // A SPAN BUT NO PATH. The first satellite's span_location did exactly this
    // and its reason is the one to keep: WHERE an error happened must read the
    // same everywhere, so each part is dropped rather than filled with a
    // placeholder.
    {
        const std::string text = "x\ny\n";
        const Diagnostic problem =
            make<Code::PARSE_EXPECTED_TYPE>(Span{2, 3, 2}, "Word(y)");
        check(holds(line(render(problem, Source{{}, text}), 0), "line 2: error"),
              "with text and no path, the header says which line");
    }

    // THE CALL STACK -- DESIGN §9's fourth field. NOTHING PRODUCES ONE UNTIL
    // M9, which is exactly why it is rendered here: report.hpp argues that
    // building three of §9's four fields and adding the fourth later is the
    // retrofit this milestone exists to prevent, and a field that is carried
    // but never drawn is that retrofit with extra steps.
    {
        Diagnostic problem = make<Code::PARSE_EXPECTED_TYPE>(Span{0, 1, 1}, "Word(x)");
        problem.frames.push_back(
            FrameRef{static_cast<satellite::words::PathId>(
                         satellite::words::NodeId::LIBRARY_MAIN),
                     Span{0, 1, 12}});
        const std::string out = render(problem, Source{"t.satl", "x\n"});
        check(holds(out, "in satellite.library.main, called at line 12"),
              "a frame names the capsule and where the call was");
    }

    // A HOLE WITH NO ARGUMENT PRINTS THE HOLE. make() makes this unreachable --
    // the arity is a static_assert -- so what can produce it is a Diagnostic
    // built field by field, which is what a test does and what a reader of a
    // serialised diagnostic would do. Printing `{2}` is the answer that says a
    // hole is what is missing; dropping it silently is how a message loses half
    // of what it was about.
    {
        Diagnostic problem;
        problem.code = Code::PARSE_EXPECTED_PUNCT;
        problem.arguments = {")"};
        check(holds(sentence(problem), "{2}"),
              "an argument that was never supplied leaves its hole visible");
    }

    // THE SENTENCE ALONE, which is the one thing a caller that is QUOTING wants
    // -- satellite_cache/read.cpp says "this did not parse: <the parser's
    // sentence>" and has no caret to draw into a file it did not write.
    {
        const Diagnostic problem =
            make<Code::PARSE_EXPECTED_TYPE>(Span{0, 1, 1}, "Word(x)");
        check(!holds(sentence(problem), "satl:"),
              "a quoted sentence carries no prefix");
        check(!holds(sentence(problem), "S0221"), "and no code");
        check(holds(sentence(problem), "a type is satellite.variable"),
              "only the words");
    }

    // MANY DIAGNOSTICS RENDER IN THE ORDER THEY WERE FOUND, each ending in a
    // newline, because a caller concatenates them and a missing terminator puts
    // two errors on one line.
    {
        const std::vector<Diagnostic> both = {
            make<Code::PARSE_EXPECTED_TYPE>(Span{0, 1, 1}, "Word(a)"),
            make<Code::PARSE_EXPECTED_TYPE>(Span{2, 3, 2}, "Word(b)"),
        };
        const std::string out = render(both, Source{"t.satl", "a\nb\n"});
        check(holds(line(out, 0), ":1:1:") && holds(line(out, 3), ":2:1:"),
              "two diagnostics, in order, three lines each");
        check(out.back() == '\n', "and the block ends in a newline");
    }

    check(!any_error({make<Code::PARSE_TOO_MANY_ERRORS>(kNowhere, 20u)}),
          "a note on its own is not a failure");
    check(any_error({make<Code::PARSE_EXPECTED_TYPE>(kNowhere, "x")}),
          "an error is");
}

} // namespace reporter_test
