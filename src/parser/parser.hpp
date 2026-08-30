#pragma once

// The parser -- PLAN M4. A token stream in, an arena AST out.
//
// DESIGN §6 IS THE SPECIFICATION and three of its rules are inherited rather
// than rediscovered, each having been a verified defect in the first satellite:
//
//   §6.1  DISPATCH ON SEGMENT 1, NOT ON SHAPE. `satellite.control.return x` and
//         `satellite.variable.time x` are the same four tokens, so a structural
//         rule -- "a dotted path followed by a bare word is a declaration" --
//         silently declares a variable of type satellite.control.return. No
//         amount of lookahead fixes it. What decides is the word at segment 1,
//         and under DESIGN §4 that is not a string compare: the lexer has
//         already interned it, so it is one integer against a table of eleven.
//   §6.2  ONE POSTFIX LOOP OVER ONE NODE VARIABLE. `parse_primary` followed by
//         `while (peek == '.')` cannot parse `foo().bar()`. Member, Call and
//         Index are peers and each REPLACES the node it wraps.
//   §6.3  THE PARSER RESOLVES NOTHING. It emits a flat Member/Call/Index chain
//         and does not know that satellite.library.<fn>.<var> is four segments.
//         The trie walk that turns a path into a PathId happens once, after
//         parsing, in M6's resolve pass -- teaching it to the grammar is a new
//         parse rule per namespace.
//
// THE PARSER NEVER THROWS, which is the rule the lexer already keeps (DESIGN
// §5.6, §9.1) and for the stronger reason: every error here has a token, and a
// caller that has to unwind cannot show a user the second one. Errors come back
// in a vector with the token they are about, and the tree that was built up to
// that point comes back with them.
//
// WHAT IT DOES RESOLVE IS NAMES IT DEFINES, and that is not the same thing. A
// capsule or a spacesuit takes the next number free under the node that owns it
// at the moment it is first met (WORD_NUMBERS §3), so this is the first caller
// of words::Words::intern -- built at M2 and called by nothing until here.

#include "abstract_syntax_tree/ast.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

// One thing wrong, and the token it is wrong at.
//
// A REASON AND NO CODE, WHICH IS THE SHAPE M3 CHOSE FOR TokenKind::Error and
// the same milestone changes both. M5 builds the reporter -- codes, a source
// excerpt with a caret, notes with their own spans, and "did you mean" over the
// trie level that failed -- and every one of those needs a decision this
// milestone would have to guess at. What M5 must not have to do is go looking
// for where the error was: `token` indexes the stream the Ast carries, so the
// span, the line and the text are all one lookup away.
struct ParseError {
    std::string reason;
    uint32_t token = 0;
};

struct Parse {
    Ast ast;
    std::vector<ParseError> errors;

    bool ok() const { return errors.empty(); }
};

// Parse source text. Never throws.
//
// `words` IS THE CALLER'S AND NOT A GLOBAL, which M2 decided for a reason this
// milestone is the first to feel: the names a program defines end with that
// program, and M11.B runs many programs in one process. A parse that reached
// for a singleton would carry one program's capsules into the next one's
// numbering.
Parse parse(const std::string &source, words::Words &words);

// Same, over a stream that has already been lexed. The tokens are MOVED into
// the tree -- ast.hpp says why the tree owns them -- so a caller that wants to
// keep its own copy has to say so by copying.
Parse parse(std::vector<Token> tokens, words::Words &words);

} // namespace satellite
