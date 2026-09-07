// What a typed line is. See satellite_prompt/block.hpp.

#include "satellite_prompt/block.hpp"

#include "lexical_analyzer/lexer.hpp"

#include <vector>

namespace satellite::prompt {

namespace {

// The four words S0204 names, and nothing else. Kept as text rather than as
// PathIds because this runs BEFORE any trie walk -- the line has not been
// parsed, let alone resolved, and `satellite.library.<name>` is a path whose
// last segment the user invented and no interner has seen.
//
// IF S0204's LIST EVER GAINS A FIFTH, THIS IS THE OTHER PLACE IT IS WRITTEN.
// Said here rather than assumed, because the two are a pair and only one of
// them errors when they disagree: the parser refuses the line, and this file
// merely puts it in the wrong half of the program, which reads as a language
// that has forgotten a word.
bool is_top_level_word(const std::string &word)
{
    return word == "include" || word == "capsule" || word == "spacesuit" ||
           word == "library";
}

// Segment 1 of a form whose body follows on the next line.
//
// `include` AND `library` ARE NOT HERE and that is the difference that matters:
// both are top-level forms, and neither takes a braced body, so a prompt that
// treated them as heads would sit waiting for a block after
// `satellite.include(satellite)`.
bool opens_a_body(const std::string &word)
{
    return word == "capsule" || word == "spacesuit" || word == "statement";
}

} // namespace

Scan scan(const std::string &line)
{
    Scan out;

    const std::vector<Token> tokens = lex(line);

    bool saw_something = false;
    bool at_start = true;
    bool after_satellite = false;
    bool head = false;
    bool saw_brace = false;
    bool after_library = false;

    for (const Token &token : tokens) {
        switch (token.kind) {
        case TokenKind::Error:
            out.lex_error = true;
            return out;

        // A NEWLINE IS A TOKEN IN THIS LANGUAGE (lexer.hpp says so, and calls it
        // a statement terminator rather than a skip), so it is not evidence that
        // the line held anything -- but it does end the "still at the start"
        // window, because a second physical line's first word is not the first
        // word of the entry.
        case TokenKind::Newline:
            at_start = false;
            after_satellite = false;
            continue;

        case TokenKind::End:
            continue;

        default:
            break;
        }

        saw_something = true;

        if (token.kind == TokenKind::Punct) {
            if (token.text == "{") {
                out.depth++;
                saw_brace = true;
            } else if (token.text == "}") {
                out.depth--;
                saw_brace = true;
            }
            // A `.` immediately after the root keeps the window open, so that
            // `satellite . capsule` -- legal, if nobody writes it -- is read the
            // same as `satellite.capsule`.
            else if (token.text != "." || !(after_satellite || after_library))
                at_start = false;
            continue;
        }

        if (token.kind == TokenKind::Word && at_start) {
            if (after_library) {
                out.library_name = token.text;
                after_library = false;
                at_start = false;
                continue;
            }
            if (!after_satellite) {
                // Segment 0. Anything but the root means a bare name, which is
                // the user's (DESIGN §1) and is never a top-level form.
                if (token.text == "satellite") {
                    after_satellite = true;
                    continue;
                }
                at_start = false;
                continue;
            }
            // Segment 1 -- the word the parser dispatches on.
            if (is_top_level_word(token.text))
                out.placement = Placement::TopLevel;
            if (opens_a_body(token.text))
                head = true;
            // `library` keeps the window open for ONE more word, which is the
            // global's name -- the only place this scanner reads segment 2.
            if (token.text == "library") {
                after_library = true;
                at_start = true;
                after_satellite = false;
                continue;
            }
            at_start = false;
            continue;
        }

        at_start = false;
    }

    // ONLY WHEN NO BRACE WAS WRITTEN. `satellite.statement.if (x) {` on one line
    // is already accounted for by the depth, and setting the flag as well would
    // make the prompt owe a body it has already been given.
    out.opens_body = head && !saw_brace;
    out.empty = !saw_something;
    return out;
}

} // namespace satellite::prompt
