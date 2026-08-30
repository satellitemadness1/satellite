#pragma once

// The lexer -- PLAN M3. Source text in, a flat vector of tokens out.
//
// DESIGN §5 IS THE SPECIFICATION and its five rules are inherited rather than
// rediscovered: each one fixed a verified defect in the first satellite. They
// land here as §5.1 underscore (lexer_chars.hpp), §5.2 whitespace in the raw
// area (lexer_chars.hpp), §5.3 lex raw and expand escapes only inside a string
// body (lex takes encode_raw'd text), §5.4 real escapes, and §5.5 no `<<` and
// no `>>`, ever -- which is why kTwoCharOps has exactly four rows.
//
// THE LEXER NEVER THROWS (DESIGN §5.6). It emits an Error token carrying a
// position and always terminates the stream with End, so a consumer can inspect
// what it produced without a try block and without checking a status first.
//
// AND WHAT IT CARRIES IS A CODE, NOT A REASON, AS OF M5. MILESTONES/M3.md §6
// item 3 left that open in as many words -- "TokenKind::Error carries a reason
// and no code ... that is the right shape for M3 and the wrong shape for M5,
// and M5 is where it changes" -- and this is the change. `text` goes back to
// meaning what it means for every other kind, the bytes as written, and
// diagnostics_of() below is where a stopped lex becomes something a person
// reads.

#include "error_reporter/report.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

enum class TokenKind : uint8_t {
    Word,     // satellite, my_time, _leading, x2_y
    Number,   // 3, 3.14
    Bits,     // x00FF, b1010 -- DESIGN §8.5
    String,   // "hello, world!"
    Punct,    // . = ( ) [ ] { } < > , : + - * / % == <= >= != ; &
    Newline,  // a statement terminator, which is why it is a token and not a skip
    End,      // always the last token
    Error,    // lexing stopped; `code` says why
};

struct Token {
    TokenKind kind = TokenKind::End;

    // The token as WRITTEN, which is what unparse round-trips (PLAN M4).
    //
    // Word:   the identifier -- "satellite", "my_time"
    // Punct:  the operator   -- "." or ">="
    // Number: the digits     -- "3.14"
    // Bits:   the literal    -- "x00FF", case preserved
    // String: the body without its quotes and with escapes STILL IN IT
    // Error:  the bytes the lexer stopped in the middle of; `code` is why
    std::string text;

    // String literals only: the body with escapes expanded (DESIGN §5.3).
    //
    // BOTH HALVES ARE KEPT AND NEITHER IS DERIVABLE FROM THE OTHER. `str` is
    // the value the program means; `text` is what the file says. Expansion is
    // not reversible -- "a\nb" and a body with a real newline in it expand to
    // the same SatString, and only one of them is legal to print back -- so an
    // unparser that had only `str` would silently rewrite a program's source.
    SatString str;

    // Bits only: 2 for a `b` literal, 16 for an `x` literal, 0 otherwise.
    unsigned radix = 0;

    // Half-open [start, end) BYTE offsets into the original source.
    //
    // Byte offsets and not SatChar indices, and encode_raw() is what makes the
    // two the same number: it maps one input byte to exactly one SatChar, so an
    // index into the encoded text is an index into the file. decode() cannot be
    // used to recover this -- it is not injective -- which is the reason
    // satellite_string.hpp gives for having a raw encoder at all.
    uint32_t start = 0;
    uint32_t end = 0;

    // 1-based line. LOAD-BEARING, not a diagnostic nicety: DESIGN §6.2's
    // postfix loop continues only on the SAME LINE.
    //
    // AND M4 DOES NOT READ IT TO ENFORCE THAT, which this comment claimed it
    // would ("the parser reads this field to decide where an expression ends",
    // written 2026-08-29 and corrected 2026-08-30 when the parser landed). The
    // rule holds for a better reason: DESIGN §5.6 leaves the NEWLINE in the
    // stream as a token, so an opener on the next line is separated from its
    // receiver by a token the postfix loop never crosses. The terminator being
    // a token is the mechanism; this field is the proof, and tests/lexer_test
    // and tests/parser_test check it from both sides. It is still load-bearing
    // -- M5's reporter is built on it, and a parser that ever needs to compare
    // two lines has them.
    uint32_t line = 1;

    // WHICH SPELLING OF THE LANGUAGE THIS WORD IS, or kNoSpelling for a name
    // the language does not use. Words only.
    //
    // A SPELLING ID AND NOT A PathId, and the difference is the one thing in
    // this header most likely to be got wrong -- an earlier draft of this lexer
    // did get it wrong, and it compiled, because `using SpellingId = PathId`
    // makes them the same 32 bits. They are not the same fact:
    //
    //   a SpellingId  answers "is this bare word one the language uses, and
    //                 which one" -- DESIGN §4.4, where the interner is
    //                 explicitly "deduplication, not identity", because `list`
    //                 under `container` and `list` under `directory` are two
    //                 nodes sharing one string.
    //   a PathId      answers "which node is this", and DESIGN §4.5 says it is
    //                 produced BY A WALK, once, when a whole path is read.
    //
    // The lexer sees `display`. It cannot know whether that is
    // satellite.console.display or a user's variable, because the answer is a
    // property of the path being built around it, and the parser is what builds
    // one. So what comes out of here is the spelling, and M4's walk turns a run
    // of them into a PathId. Storing intern()'s answer in a field called a
    // PathId would have handed the parser a number that means "the first node
    // in words.def spelled this" and let it be dispatched on.
    //
    // DESIGN §5.6's "known words carry their node identity out of the lexer" is
    // this field, read against §4.4: the identity a bare word HAS at lex time
    // is its spelling.
    words::SpellingId spelling = words::kNoSpelling;

    // Error tokens only: WHY the lex stopped, as one of errors.def's S01xx
    // rows. kNone for every other kind.
    //
    // A CODE AND NOT A SENTENCE, WHICH IS THE WHOLE OF WHAT M5 CHANGED HERE.
    // The reason used to live in `text` as a string literal at the one site
    // that produced it -- which is the shape DESIGN §9 counts 199 of in the
    // first satellite, at the smallest possible scale. One row in errors.def
    // instead, and diagnostic_of() is what turns it into the block a person
    // sees, caret, note and all.
    errors::Code code = errors::Code::NONE;
};

// Lex satellite source. Never throws.
//
// The result always ends with End. If lexing stopped early, an Error token
// carrying the code for why sits immediately before it.
std::vector<Token> lex(const std::string &source);

// Same, for source already through encode_raw(). The std::string overload is
// this one after an encode_raw, and it is spelled out so that no caller can
// reach for encode() by mistake -- DESIGN §5.3 is the rule it would break, and
// the symptom is "C:\home" in a comment becoming somebody's home directory.
std::vector<Token> lex(const SatString &source);

// Everything wrong with a token stream, as diagnostics the reporter renders.
//
// IT IS THE LEXER'S AND NOT THE PARSER'S, even though the parser is what calls
// it, and the reason is DESIGN §9's "rendering in exactly one place" applied
// one level up: what an unterminated string literal MEANS -- that the caret
// goes under the opening quote and the note goes where the line ran out -- is a
// fact about lexing, and a converter living in the parser would be a second
// module that has to know it. The parser has the same function next door and
// neither knows the other's codes.
//
// EMPTY FOR A CLEAN STREAM, so a caller may run it unconditionally.
std::vector<errors::Diagnostic> diagnostics_of(const std::vector<Token> &tokens);

// A bare word's spelling id, aliases included -- THE LEXER'S HALF OF THE
// SPELLING TABLE, which is what PLAN M3 owes M2's nine aliases.
//
// words::intern() knows the nodes. It does not know that `hexadecimal` is a
// second spelling of `hex`, or that `arg` `args` `argz` `argument` and
// `argumentz` are five more spellings of `arguments` -- DESIGN §7.7's one node
// with six spellings, WORD_NUMBERS §2.3. Those are alias rows in words.def, and
// resolving them to the SAME id as the word they alias is what makes "one node,
// six spellings" true of the token stream rather than only of the registry.
//
// THE THREE DOTTED ALIASES ARE NOT LEXICAL AND ARE SKIPPED HERE.
// `fast.range(min, max)` is a rewrite of a two-segment PATH, so it cannot be
// decided by looking at one bare word; PLAN M13 owns it. The filter is exactly
// "an alias whose spelling has no dot in it", which is why this function can be
// read as a spelling table and not as a path table.
words::SpellingId intern_word(std::string_view word);

// Whether a token is the reserved word -- ONE INTEGER COMPARE, which is what
// DESIGN §4.4 promises and the lexer's half of DESIGN §2's reservation rule.
//
// §2 is settled in three tokens -- `satellite . variable` against
// `satellite . library` separates a type path from a value path -- and the
// parser makes that test by index. What it must not have to do is compare
// strings to make it, on every path, in a language where every path starts with
// this word.
bool is_reserved_word(const Token &token);

const char *kind_name(TokenKind kind);

// "Word(satellite)", "Punct(>=)", "Number(3.14)" -- for tests and for --tokens.
std::string describe(const Token &token);

// Split a two-character Punct in place, inserting the second half after it.
// Returns false if that token is not a two-character Punct.
//
// THIS EXISTS FOR ONE TOKEN AND DESIGN §5.5 IS WHY IT IS SO SMALL. A generic
// close is a single '>', and `>=` is matched greedily without knowing whether a
// type is being read; a parser that wants '>' and finds '>=' calls this and
// takes the '>'. §5.5 notes no such collision is actually reachable in §6's
// grammar, because a complete type is only ever followed by IDENT, ')', ',' or
// '>' -- so this is the guard v1 verified it needed, kept against the grammar
// changing rather than against a failure this grammar has.
//
// M4 LANDED AND STILL DOES NOT CALL IT, 2026-08-30, which MILESTONES/M3.md §6
// item 4 asked this milestone to confirm or deny. Confirmed: parser_types.cpp
// reads every generic argument list in the language and closes them all with a
// plain '>', because §5.5's argument holds -- nothing that follows a complete
// type can begin with '='.
//
// AND M4 FOUND A SECOND REASON NOT TO REACH FOR IT, which is worth more than
// the first because it survives the grammar changing. THIS FUNCTION INSERTS
// INTO THE VECTOR, and the parser stores TOKEN INDICES in every node it has
// already built -- so a split partway through a parse renumbers the anchor of
// every node behind it, silently. If a future grammar does make `>=` reachable
// where a '>' is wanted, the fix belongs at lex time or in a record beside the
// stream, not in a mid-parse mutation of it. Kept rather than deleted, and
// tested, because the fix at lex time is this function with a different caller.
bool split_punct(std::vector<Token> &tokens, size_t index);

} // namespace satellite
