// The one lexical error, its span and its note. See
// tests/reporter_test/reporter_test.hpp.
//
// ONE ERROR IS THE WHOLE BLOCK AND THAT IS THE LEXER'S DESIGN. DESIGN §5.6
// gives a byte the code table has no entry for back as a Punct rather than as
// an error, "because the lexer's job is to describe the file, and deciding that
// a character is meaningless is the parser's" -- so the only thing that can
// stop a lex is a string literal with no end.
//
// WHAT M5 CHANGED HERE is the shape and not the behaviour: MILESTONES/M3.md §6
// item 3 left `TokenKind::Error` carrying a reason and no code, called that
// "the right shape for M3 and the wrong shape for M5", and this section is what
// holds the change in place.

#include "reporter_test.hpp"

#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"

#include <string>
#include <vector>

namespace reporter_test {

void section_lexing()
{
    using namespace satellite;

    const std::string source = "satellite.console.display(\"hello\n";
    const std::vector<Token> tokens = lex(source);

    // THE TOKEN CARRIES A CODE AND ITS TEXT IS THE BYTES. Before M5 the reason
    // lived in `text` as a string literal at the one site that produced it,
    // which made `text` mean something different for this kind than for every
    // other -- and lexer.hpp's own table said "the token as WRITTEN".
    const Token *stopped = nullptr;
    for (const Token &token : tokens)
        if (token.kind == TokenKind::Error)
            stopped = &token;
    check(stopped != nullptr, "an unterminated string stops the lex");
    if (stopped == nullptr)
        return;
    check(stopped->code == errors::Code::LEX_UNTERMINATED_STRING,
          "and the Error token says why as a code");
    check(holds(describe(*stopped), "S0101"),
          "`satl --tokens` prints the code, so a stopped stream can be looked "
          "up from the dump it appears in");

    check(tokens.back().kind == TokenKind::End,
          "the stream still ends with End -- DESIGN §5.6, and a consumer may "
          "inspect what it produced without checking a status first");

    // THE DIAGNOSTIC, AND BOTH OF ITS SPANS COME OUT OF THAT ONE TOKEN.
    const std::vector<errors::Diagnostic> found = diagnostics_of(tokens);
    check(found.size() == 1, "one error, one diagnostic");
    if (found.empty())
        return;

    const std::string out =
        errors::render(found, errors::Source{"t.satl", source});

    check(holds(line(out, 0), "error S0101:"), "S0101 is the unterminated string");
    check(holds(line(out, 0), "t.satl:1:27:"),
          "the caret goes under the OPENING QUOTE, which is the character the "
          "person has to look at -- not under the end of the line");
    check(line(out, 2) == caret_row(27),
          "and the caret is drawn there");

    // THE NOTE, WHICH IS THE HALF THAT NEEDED M5. A span for where the line ran
    // out was already in the token -- the lexer set `end` for it -- and until
    // there was somewhere to put a note there was nothing to do with it.
    check(holds(out, "note S0102:"), "a note says where the line ended");
    check(holds(out, "cannot carry on to the next one"),
          "and says why that is the end of the matter");

    // A CLEAN STREAM PRODUCES NOTHING, so a caller may run diagnostics_of
    // unconditionally rather than testing first.
    check(diagnostics_of(lex("satellite.console.display(\"hello\")\n")).empty(),
          "a stream with no Error token has no diagnostics");

    // AND THE PARSER STOPS AT IT RATHER THAN PARSING PAST IT, which is M4's
    // rule and is unchanged: what follows an Error token is the End the lexer
    // appended, so a tree built from it would claim the file stopped there.
    const std::vector<errors::Diagnostic> through = problems_in(source);
    check(through.size() == 1 &&
              through.front().code == errors::Code::LEX_UNTERMINATED_STRING,
          "a lexical error reaches a caller of parse() as itself, and the "
          "parser adds nothing of its own on top of it");
}

} // namespace reporter_test
