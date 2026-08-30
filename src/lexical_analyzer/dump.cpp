// What `satl --tokens` prints. See lexical_analyzer/dump.hpp.
//
// THE SPAN IS THE COLUMN THAT EARNS ITS WIDTH. A token's [start, end) is what
// every error message from M5 onwards will point a caret with, and it is the
// one field of a Token that cannot be checked by reading the program: an
// off-by-one in a span is invisible until a caret lands under the wrong
// character, months later, in a milestone that did not write it.

#include "lexical_analyzer/dump.hpp"

#include "lexical_analyzer/lexer.hpp"

#include <algorithm>
#include <string>
#include <vector>

namespace satellite {

namespace {

// Right-align a number in `width`, which is what makes the line and span
// columns readable straight down.
std::string right(unsigned long value, size_t width)
{
    std::string text = std::to_string(value);
    return text.size() >= width ? text : std::string(width - text.size(), ' ') + text;
}

} // namespace

std::string tokens_text(const std::string &source, bool &clean)
{
    const std::vector<Token> tokens = lex(source);

    clean = true;
    for (const Token &token : tokens)
        if (token.kind == TokenKind::Error)
            clean = false;

    // WIDTHS MEASURED FROM THIS STREAM RATHER THAN GUESSED. A column narrower
    // than its widest row stops being a column, and the widest row here depends
    // on the file -- a one-line program and a 40,000-byte one do not want the
    // same span column. satellite_words/dump.cpp measures its own for the same
    // reason and records what happens when the measurement and the comment
    // describe different things.
    size_t line_width = 1;
    size_t span_width = 1;
    for (const Token &token : tokens) {
        line_width = std::max(line_width, std::to_string(token.line).size());
        span_width = std::max(span_width, std::to_string(token.end).size());
    }

    std::string out;
    for (const Token &token : tokens) {
        out += "  " + right(token.line, line_width);
        out += "  " + right(token.start, span_width);
        out += ".." + right(token.end, span_width);
        out += "  " + describe(token) + "\n";
    }

    // WHAT IT DOES NOT DO, SAID IN THE OUTPUT. `satl --words` ends by saying
    // that almost nothing it lists runs yet, because a dump of 254 paths with
    // no such line reads as a feature list. A dump of tokens has the same
    // problem one milestone on: it looks like a program being understood, and
    // nothing here understands anything.
    //
    // THIS LINE SAID "the parser lands at M4" UNTIL M4 LANDED, 2026-08-30, and
    // is the kind of sentence that goes stale silently -- it was true when it
    // was written and became a lie the day the work it was waiting for arrived.
    // What replaces it names the command that DOES understand the file, because
    // that is the useful half for somebody who has just looked at a token dump.
    out += "\n";
    out += std::to_string(tokens.size()) + " tokens. This is what the lexer saw;";
    out += " `satl --unparse` is what the parser made of it.\n";
    if (!clean)
        out += "The stream holds an error, so lexing stopped early.\n";
    return out;
}

} // namespace satellite
