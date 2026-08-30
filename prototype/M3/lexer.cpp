// Lexer implementation prototype -- Milestone 3.
// See lexer.hpp for interface and design guarantees.

#include "lexer.hpp"
#include "lexer_chars.hpp"

#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace satellite {

namespace {

// The complete set of multi-character operators (DESIGN §5.5).
// Shifts (<<, >>) do not exist in the language and must never be added here.
struct TwoCharOp {
    SatChar first;
    SatChar second;
    const char *text;
};

constexpr TwoCharOp kTwoCharOps[] = {
    { SAT_EQUAL,   SAT_EQUAL, "==" },
    { SAT_LESS,    SAT_EQUAL, "<=" },
    { SAT_GREATER, SAT_EQUAL, ">=" },
    { SAT_BANG,    SAT_EQUAL, "!=" },
};

words::PathId lookup_word(std::string_view spelling)
{
    words::PathId id = words::intern(spelling);
    if (id != words::kNoSpelling)
        return id;

    // Check aliases in words.def (e.g. hexadecimal, arg, args, etc.)
    for (size_t i = 0; i < words::kAliasCount; ++i) {
        if (words::spelling_of(words::kAliases[i].text) == spelling)
            return static_cast<words::PathId>(words::kAliases[i].of);
    }
    return words::kNoPath;
}

} // namespace

std::vector<Token> lex(const std::string &source)
{
    return lex(encode_raw(source));
}

std::vector<Token> lex(const SatString &src)
{
    std::vector<Token> out;
    size_t i = 0;
    uint32_t line = 1;

    auto add = [&](TokenKind kind, size_t start, size_t end) -> Token & {
        Token t;
        t.kind = kind;
        t.start = static_cast<uint32_t>(start);
        t.end = static_cast<uint32_t>(end);
        t.line = line;
        out.push_back(std::move(t));
        return out.back();
    };

    while (i < src.size()) {
        SatChar c = src[i];

        // Whitespace handling
        if (is_space(c)) {
            if (is_newline(c)) {
                add(TokenKind::Newline, i, i + 1).text = "\n";
                line++;
            }
            i++;
            continue;
        }

        // Line comment: // to end of line (DESIGN §5.6).
        // The newline is left in the stream so the line counter stays correct.
        if (c == SAT_SLASH && i + 1 < src.size() && src[i + 1] == SAT_SLASH) {
            while (i < src.size() && !is_newline(src[i]))
                i++;
            continue;
        }

        const size_t start = i;

        // Words and Bits literals
        if (word_start(c)) {
            while (i < src.size() && word_cont(src[i]))
                i++;
            std::string spelling = decode(src.substr(start, i - start));

            // Binary (b...) and Hex (x...) literals (DESIGN §8.5).
            // Lowercase prefix, followed by at least 1 digit, ALL of which
            // are valid digits in that radix.
            unsigned radix = 0;
            if (spelling.size() > 1 && (spelling[0] == 'x' || spelling[0] == 'b')) {
                const unsigned candidate = (spelling[0] == 'b') ? 2u : 16u;
                bool all_digits = true;
                for (size_t d = 1; d < spelling.size(); ++d) {
                    if (!bits_valid_digit(candidate, spelling[d])) {
                        all_digits = false;
                        break;
                    }
                }
                if (all_digits)
                    radix = candidate;
            }

            if (radix != 0) {
                Token &t = add(TokenKind::Bits, start, i);
                t.radix = radix;
                t.text = std::move(spelling);
            } else {
                Token &t = add(TokenKind::Word, start, i);
                t.word_id = lookup_word(spelling);
                t.text = std::move(spelling);
            }
            continue;
        }

        // Numbers: digit-started (DESIGN §5.6).
        if (is_digit(c)) {
            while (i < src.size() && is_digit(src[i]))
                i++;
            // '.' joins the number only if followed immediately by another digit.
            if (i + 1 < src.size() && src[i] == SAT_DOT && is_digit(src[i + 1])) {
                i++;
                while (i < src.size() && is_digit(src[i]))
                    i++;
            }
            std::string digits = decode(src.substr(start, i - start));
            Token &t = add(TokenKind::Number, start, i);
            t.text = std::move(digits);
            continue;
        }

        // String literals (DESIGN §5.3, §5.4).
        if (c == SAT_QUOTE) {
            i++; // consume opening quote
            const size_t body = i;
            bool closed = false;
            while (i < src.size()) {
                if (is_newline(src[i]))
                    break; // string literal may not span lines
                if (src[i] == SAT_QUOTE) {
                    closed = true;
                    break;
                }
                // Skip escaped character
                i += (src[i] == SAT_BACKSLASH && i + 1 < src.size()) ? 2 : 1;
            }
            if (!closed) {
                add(TokenKind::Error, start, i).text = "unterminated string literal";
                break;
            }
            Token &t = add(TokenKind::String, start, i + 1);
            t.text = decode(src.substr(body, i - body));
            t.str = encode(t.text);
            i++; // consume closing quote
            continue;
        }

        // Two-character operators
        bool matched = false;
        if (i + 1 < src.size()) {
            for (const auto &op : kTwoCharOps) {
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

        // Single-character punctuation / raw byte
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
    case TokenKind::String:
        return "String(" + decode(token.str) + ")";
    case TokenKind::Word:
        if (token.word_id != words::kNoPath)
            return "Word(" + token.text + " #" + std::to_string(token.word_id) + ")";
        return "Word(" + token.text + ")";
    default:
        return std::string(kind_name(token.kind)) + "(" + token.text + ")";
    }
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

