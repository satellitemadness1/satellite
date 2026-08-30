// Spans, lines, and the acceptance program.
//
// A SPAN IS THE ONE FIELD OF A TOKEN THAT CANNOT BE CHECKED BY READING THE
// PROGRAM. Every error message from M5 onwards points a caret with [start,
// end), and an off-by-one in one is invisible until a caret lands under the
// wrong character in a milestone that did not write it. So it is checked here
// the only way it can be: by slicing the source back out and comparing.
//
// AND THE LINE IS LOAD-BEARING, not a diagnostic nicety. DESIGN §6.2's postfix
// loop continues only on the SAME LINE, so a lexer that miscounts lines changes
// where the parser thinks an expression ends.

#include "lexer_test.hpp"

#include "lexical_analyzer/lexer.hpp"

#include <cstdio>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>

namespace lexer_test {

namespace {

using satellite::Token;
using satellite::TokenKind;

// EVERY SPAN SLICES ITS OWN TEXT BACK OUT, for the kinds whose text IS the
// source. A String's text has its quotes stripped and its escapes unexpanded,
// and a Newline's is written "\n" whatever the file holds, so those two are
// checked by their WIDTH instead of by their content.
void spans_are_exact(const std::string &source, const char *what)
{
    for (const Token &token : satellite::lex(source)) {
        const bool ordered = token.start <= token.end && token.end <= source.size();
        check(ordered, std::string(what) + ": the span is inside the source");
        if (!ordered)
            return;

        const std::string slice = source.substr(token.start, token.end - token.start);
        switch (token.kind) {
        case TokenKind::Word:
        case TokenKind::Number:
        case TokenKind::Bits:
        case TokenKind::Punct:
            check(slice == token.text,
                  std::string(what) + ": " + satellite::describe(token)
                      + " slices back to itself");
            break;
        case TokenKind::String:
            // The two quotes are in the span and not in the text.
            check(slice.size() == token.text.size() + 2,
                  std::string(what) + ": a String's span covers its quotes");
            break;
        case TokenKind::Newline:
            check(slice == "\n", std::string(what) + ": a Newline spans one byte");
            break;
        case TokenKind::End:
            check(token.start == source.size() && token.end == source.size(),
                  std::string(what) + ": End sits at the end and is empty");
            break;
        case TokenKind::Error:
            break;
        }
    }
}

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

} // namespace

void section_spans()
{
    // -- The stream's shape ------------------------------------------------

    {
        const std::vector<Token> tokens = satellite::lex("");
        check(tokens.size() == 1 && tokens[0].kind == TokenKind::End,
              "an empty source is exactly one End token");
    }
    {
        const std::vector<Token> tokens = satellite::lex("a b c");
        check(tokens.back().kind == TokenKind::End, "the stream always ends with End");
        int ends = 0;
        for (const Token &token : tokens)
            if (token.kind == TokenKind::End)
                ends++;
        check(ends == 1, "and there is exactly one of them");
    }

    // -- Spans ---------------------------------------------------------------

    spans_are_exact("satellite.console.display(\"hi\")", "a call");
    spans_are_exact("a >= b\nmy_time = 3.14 + x00FF", "operators and literals");
    spans_are_exact("// comment\nword\n", "a comment and a word");

    // -- Lines ---------------------------------------------------------------

    {
        const std::vector<Token> tokens = satellite::lex("a\nb\nc");
        check(tokens[0].line == 1 && tokens[2].line == 2 && tokens[4].line == 3,
              "the line advances once per newline");
        check(tokens[1].line == 1,
              "a Newline carries the line it ENDS, not the one it begins");
    }
    {
        // A comment must not disturb the count -- it stops AT the newline and
        // leaves it in the stream, which is the whole reason it is discarded
        // that way rather than consumed to the next line.
        const std::vector<Token> tokens = satellite::lex("a // c\nb");
        check(tokens[0].line == 1, "before a comment: line 1");
        check(tokens[2].line == 2, "after a comment's newline: line 2");
    }
    {
        // THE ESCAPED-NEWLINE TRAP, and it is a LINE-COUNTING bug wearing a
        // string literal's clothes. A lexer that skips two characters for every
        // backslash carries this literal to the quote on the NEXT line, hands
        // back a String, and never sees the newline in between -- so `line`
        // stays one short for the rest of the file and every error span after
        // it points one line too high. Refusing the literal is what keeps the
        // count honest, because a string may not span lines.
        const std::vector<Token> tokens = satellite::lex("\"a\\\nb\"\nx");
        check(tokens[0].kind == TokenKind::Error,
              "a trailing backslash does not carry a literal onto the next line");
        for (const Token &token : tokens)
            check(token.kind != TokenKind::String,
                  "and no String is produced by spanning the newline");
    }

    // -- The acceptance program ---------------------------------------------
    //
    // A TEST WHOSE SUBJECT IS A FILE MUST FAIL LOUDLY WHEN IT CANNOT READ THAT
    // FILE, never skip -- tests/words_test records the same rule for
    // WORD_NUMBERS.md. A suite that reports PASS over a file it never opened is
    // a green line, not a result.
    std::string source;
    if (!read_file(example_path, source)) {
        check(false, "cannot read " + example_path + " -- the acceptance program is an INPUT");
        return;
    }

    const std::vector<Token> tokens = satellite::lex(source);
    for (const Token &token : tokens)
        check(token.kind != TokenKind::Error,
              "hello world lexes with no Error: " + token.text);

    spans_are_exact(source, "hello_world.satl");

    // DESIGN §3 is a byte-for-byte copy of this file, so what it opens with is
    // a fact about the language and not about this test's fixture.
    check(!tokens.empty() && satellite::is_reserved_word(tokens[0])
              == (tokens[0].kind == TokenKind::Word),
          "the first Word in hello world is the reserved word");

    // The program's own words are the language's; `arguments` is the only name
    // in it that a user could have chosen, and §7.7 says the language owns that
    // one too.
    size_t user_owned = 0;
    for (const Token &token : tokens)
        if (token.kind == TokenKind::Word
            && token.spelling == satellite::words::kNoSpelling)
            user_owned++;
    check(user_owned == 0,
          "every Word in hello world is one the language owns -- §7.7 includes `arguments`");
}

} // namespace lexer_test
