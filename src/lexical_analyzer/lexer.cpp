// The lexer. See lexical_analyzer/lexer.hpp for what a Token is and why the
// spelling it carries is not a PathId.
//
// ONE PASS, NO LOOKAHEAD BEYOND ONE CHARACTER, and DESIGN §5.5 is what buys
// that: satellite has no `<<` and no `>>`, ever, so the only greedy matches in
// the language are the four in kTwoCharOps and `list<list<string>>` closes as
// two independent '>' tokens. The C++98 maximal-munch bug cannot occur here,
// which is why this file has no state machine and no backtracking.

#include "lexical_analyzer/lexer.hpp"

#include "lexical_analyzer/lexer_chars.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace satellite {

namespace {

struct TwoCharOp {
    SatChar first;
    SatChar second;
    const char *text;
};

// THE COMPLETE SET, AND IT IS FOUR ROWS BECAUSE DESIGN §5.5 IS POLICY. `<<` and
// `>>` are not missing from this table, they are refused by the language: a
// shift is satellite.variable.number.shift_left(n). Adding either here would
// re-introduce the C++98 nested-generic bug the language was shaped to avoid,
// so this table is the one place that decision is enforceable and it is stated
// here rather than assumed.
constexpr TwoCharOp kTwoCharOps[] = {
    { SAT_EQUAL,   SAT_EQUAL, "==" },
    { SAT_LESS,    SAT_EQUAL, "<=" },
    { SAT_GREATER, SAT_EQUAL, ">=" },
    { SAT_BANG,    SAT_EQUAL, "!=" },
};

// The radix a word-shaped token would be a Bits literal in, or 0 if it is a
// Word after all (DESIGN §8.5).
//
// THE WHOLE BODY MUST BE DIGITS OF THAT RADIX, which is what keeps this from
// eating identifiers: `bad` starts with 'b' and 'a' is not a binary digit, so
// it is a Word; `x2_y` has an underscore, which is no hex digit, so it is a
// Word. What it does claim is `xa` and `b01` -- a name shaped exactly like a
// literal IS a literal, and that is §8.5's decision rather than this function's,
// because the width being part of the value is only meaningful if the spelling
// is reserved for it.
unsigned bits_radix(std::string_view spelling)
{
    if (spelling.size() < 2)
        return 0;
    if (spelling[0] != 'x' && spelling[0] != 'b')
        return 0;

    const unsigned radix = spelling[0] == 'b' ? 2u : 16u;
    for (size_t i = 1; i < spelling.size(); i++)
        if (!is_digit_in_radix(radix, spelling[i]))
            return 0;
    return radix;
}

} // namespace

words::SpellingId intern_word(std::string_view word)
{
    if (const words::SpellingId id = words::intern(word); id != words::kNoSpelling)
        return id;

    for (size_t i = 0; i < words::kAliasCount; i++) {
        // An alias is written relative to its node's PARENT and is free to
        // carry a dot; the dotted ones are two-segment path rewrites and no
        // amount of looking at one bare word can decide them. lexer.hpp says
        // which milestone owns those.
        const std::string_view spelling = words::spelling_of(words::kAliases[i].text);
        if (spelling.find('.') != std::string_view::npos)
            continue;
        if (spelling == word)
            return words::spelling_id(words::kAliases[i].of);
    }
    return words::kNoSpelling;
}

bool is_reserved_word(const Token &token)
{
    return token.kind == TokenKind::Word
        && token.spelling == words::kSatelliteSpelling;
}

std::vector<Token> lex(const std::string &source)
{
    // encode_raw AND NEVER encode -- DESIGN §5.3, and the reason is in
    // satellite_string.hpp. This one call is the whole of that rule's
    // enforcement, which is why lex() takes a std::string at all.
    return lex(encode_raw(source));
}

std::vector<Token> lex(const SatString &src)
{
    std::vector<Token> out;
    size_t i = 0;
    uint32_t line = 1;

    auto add = [&](TokenKind kind, size_t start, size_t end) -> Token & {
        Token token;
        token.kind = kind;
        token.start = static_cast<uint32_t>(start);
        token.end = static_cast<uint32_t>(end);
        token.line = line;
        out.push_back(std::move(token));
        return out.back();
    };

    while (i < src.size()) {
        const SatChar c = src[i];

        if (is_space(c)) {
            // A newline is a token; the other three are skipped. The counter
            // moves AFTER the token is added, so a Newline carries the line it
            // ends rather than the one it begins.
            if (is_newline(c)) {
                add(TokenKind::Newline, i, i + 1).text = "\n";
                line++;
            }
            i++;
            continue;
        }

        // `//` TO END OF LINE, DISCARDED HERE AND NEVER SEEN BY THE PARSER
        // (DESIGN §5.6). The newline is deliberately left in the stream: it is
        // a statement terminator and it is what keeps `line` correct. There is
        // no block comment, on purpose -- a form that can be left unclosed is a
        // form that can swallow a file, so `/*` is two ordinary Punct tokens.
        if (c == SAT_SLASH && i + 1 < src.size() && src[i + 1] == SAT_SLASH) {
            while (i < src.size() && !is_newline(src[i]))
                i++;
            continue;
        }

        const size_t start = i;

        // Words, and the Bits literals that are shaped like them.
        if (word_start(c)) {
            while (i < src.size() && word_cont(src[i]))
                i++;
            std::string spelling = decode(src.substr(start, i - start));

            if (const unsigned radix = bits_radix(spelling); radix != 0) {
                Token &token = add(TokenKind::Bits, start, i);
                token.radix = radix;
                token.text = std::move(spelling);
            } else {
                Token &token = add(TokenKind::Word, start, i);
                token.spelling = intern_word(spelling);
                token.text = std::move(spelling);
            }
            continue;
        }

        // Numbers. DIGIT-STARTED ONLY, WHICH IS WHAT MAKES `-1` TWO TOKENS
        // (DESIGN §5.6): a sign is never folded in, because folding would turn
        // `a-1` into Word(a) Number(-1) and break subtraction. Unary minus is an
        // expression rule and not a lexical one.
        if (is_digit(c)) {
            while (i < src.size() && is_digit(src[i]))
                i++;
            // A '.' joins the number only when a digit follows it, so `3.14` is
            // one Number and `main.x` is Word Punct Word.
            if (i + 1 < src.size() && src[i] == SAT_DOT && is_digit(src[i + 1])) {
                i++;
                while (i < src.size() && is_digit(src[i]))
                    i++;
            }
            add(TokenKind::Number, start, i).text = decode(src.substr(start, i - start));
            continue;
        }

        // String literals (DESIGN §5.3, §5.4).
        if (c == SAT_QUOTE) {
            i++;
            const size_t body = i;
            bool closed = false;
            while (i < src.size()) {
                if (is_newline(src[i]))
                    break;
                if (src[i] == SAT_QUOTE) {
                    closed = true;
                    break;
                }
                // AN ESCAPE MAY NOT SWALLOW THE NEWLINE. Skipping two
                // characters unconditionally lets a trailing backslash carry
                // the literal onto the next line, which is not a form this
                // language has -- and it does it silently AND leaves `line`
                // permanently one short for the rest of the file, so every
                // error span after it points at the wrong line.
                const bool escaped = src[i] == SAT_BACKSLASH && i + 1 < src.size()
                                  && !is_newline(src[i + 1]);
                i += escaped ? 2 : 1;
            }
            if (!closed) {
                // THE SPAN IS THE OPENING QUOTE TO WHERE THE LINE RAN OUT, and
                // both ends are load-bearing at M5. The caret goes under the
                // `"` that opened it -- which is the character the person has
                // to look at -- and `end` is where diagnostics_of() puts the
                // note, so the two halves of the message come out of one token.
                Token &stopped = add(TokenKind::Error, start, i);
                stopped.code = errors::Code::LEX_UNTERMINATED_STRING;
                stopped.text = decode(src.substr(start, i - start));
                break;
            }
            Token &token = add(TokenKind::String, start, i + 1);
            token.text = decode(src.substr(body, i - body));
            // encode() HERE AND ONLY HERE -- the body of a string literal is
            // the one place DESIGN §5.3 allows escape expansion.
            token.str = encode(token.text);
            i++;
            continue;
        }

        bool matched = false;
        if (i + 1 < src.size()) {
            for (const TwoCharOp &op : kTwoCharOps) {
                if (c == op.first && src[i + 1] == op.second) {
                    i += 2;
                    add(TokenKind::Punct, start, i).text = op.text;
                    matched = true;
                    break;
                }
            }
        }
        if (matched)
            continue;

        // Anything else is one Punct. A byte the code table has no entry for
        // reaches here through the raw area and becomes a Punct holding that
        // byte, rather than an Error -- the lexer's job is to describe the
        // file, and deciding that a character is meaningless is the parser's.
        i++;
        add(TokenKind::Punct, start, i).text = decode(src.substr(start, 1));
    }

    add(TokenKind::End, i, i);
    return out;
}

const char *kind_name(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Word:    return "Word";
    case TokenKind::Number:  return "Number";
    case TokenKind::Bits:    return "Bits";
    case TokenKind::String:  return "String";
    case TokenKind::Punct:   return "Punct";
    case TokenKind::Newline: return "Newline";
    case TokenKind::End:     return "End";
    case TokenKind::Error:   return "Error";
    }
    return "?";
}

std::string describe(const Token &token)
{
    switch (token.kind) {
    case TokenKind::End:
        return "End";
    case TokenKind::Newline:
        return "Newline";
    case TokenKind::Word:
        // The spelling id is shown only when the language HAS the word, so a
        // reader can tell at a glance which names in their program are the
        // language's and which are their own -- which is DESIGN §5.6's
        // distinction, printed.
        if (token.spelling != words::kNoSpelling)
            return "Word(" + token.text + " #" + std::to_string(token.spelling) + ")";
        return "Word(" + token.text + ")";
    case TokenKind::Error:
        // THE CODE AND NOT THE SENTENCE, because this is a token dump and the
        // sentence is four lines long. `satl --errors S0101` is where the
        // sentence is, and printing the code here is what makes that lookup
        // possible from a `--tokens` listing.
        return "Error(" + std::string(errors::code_text(token.code).view()) + ")";
    default:
        // AS WRITTEN, INCLUDING FOR A STRING, so one token stays one line. The
        // expanded body is in `str` and a real newline in it would break every
        // consumer of this that reads line by line, --tokens included.
        return std::string(kind_name(token.kind)) + "(" + token.text + ")";
    }
}

std::vector<errors::Diagnostic> diagnostics_of(const std::vector<Token> &tokens)
{
    std::vector<errors::Diagnostic> out;
    for (const Token &token : tokens) {
        if (token.kind != TokenKind::Error)
            continue;
        const errors::Span at{token.start, token.start + 1, token.line};
        switch (token.code) {
        case errors::Code::LEX_UNTERMINATED_STRING: {
            errors::Diagnostic problem =
                errors::make<errors::Code::LEX_UNTERMINATED_STRING>(at);
            // THE NOTE'S SPAN IS THE TOKEN'S `end`, WHICH IS WHERE THE LINE RAN
            // OUT. Both halves come out of one token because the string arm set
            // both ends for exactly this -- see the comment beside it. The
            // caret goes under the quote, and the note goes under the place the
            // person has to look at second.
            problem.notes.push_back(
                errors::note<errors::Code::LEX_LINE_ENDS_HERE>(
                    errors::Span{token.end, token.end + 1, token.line}));
            out.push_back(std::move(problem));
            break;
        }
        default:
            // A CODE THIS FUNCTION DOES NOT KNOW STILL BECOMES A DIAGNOSTIC.
            // There is one lexical error today and there may be two tomorrow;
            // the failure to avoid is a token that carries a code and reaches
            // the user as silence.
            errors::Diagnostic problem;
            problem.code = token.code;
            problem.at = at;
            out.push_back(std::move(problem));
            break;
        }
    }
    return out;
}

bool split_punct(std::vector<Token> &tokens, size_t index)
{
    if (index >= tokens.size())
        return false;

    Token &first = tokens[index];
    if (first.kind != TokenKind::Punct || first.text.size() != 2)
        return false;

    Token second = first;
    second.text = first.text.substr(1);
    second.start = first.start + 1;

    first.text = first.text.substr(0, 1);
    first.end = first.start + 1;

    tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                  std::move(second));
    return true;
}

} // namespace satellite
