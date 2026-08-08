#include "lexer.hpp"

#include <cstdlib>

namespace satellite {

// --- character classes -----------------------------------------------------
//
// These are written against the code table, not against C's ctype functions.
// isspace() on a SatChar would be meaningless: a space has no entry in the
// table at all, so it arrives as SAT_RAW_BASE + ' '.

static bool is_letter(SatChar c) { return c >= SAT_A && c < SAT_DIGIT_0; }
static bool is_digit(SatChar c) { return c >= SAT_DIGIT_0 && c < SAT_PUNCT_BASE; }

static bool is_newline(SatChar c) { return c == SAT_RAW_BASE + '\n'; }

static bool is_space(SatChar c)
{
    return c == SAT_RAW_BASE + ' '  || c == SAT_RAW_BASE + '\t'
        || c == SAT_RAW_BASE + '\r' || is_newline(c);
}

// Underscore counts as an identifier character even though the code table
// files it under punctuation, so my_time is one Word rather than three tokens.
static bool word_start(SatChar c) { return is_letter(c) || c == SAT_UNDERSCORE; }
static bool word_cont(SatChar c)
{
    return is_letter(c) || is_digit(c) || c == SAT_UNDERSCORE;
}

// The complete set of multi-character operators. '<' and '>' are absent by
// design and must stay that way: it is what lets list<list<string>> close with
// two independent '>' tokens, so nested generics need no special handling and
// the C++98 maximal-munch bug cannot occur here. satellite has no '<<' or '>>'
// operator; shifts, if they are ever wanted, are named methods.
static const struct {
    SatChar first, second;
    const char *text;
} TWO_CHAR[] = {
    { SAT_EQUAL,   SAT_EQUAL, "==" },
    { SAT_LESS,    SAT_EQUAL, "<=" },
    { SAT_GREATER, SAT_EQUAL, ">=" },
    { SAT_BANG,    SAT_EQUAL, "!=" },
};

// ---------------------------------------------------------------------------

std::vector<Token> lex(const std::string &source)
{
    return lex(encode_raw(source));
}

std::vector<Token> lex(const SatString &src)
{
    std::vector<Token> out;
    size_t i = 0;
    size_t line = 1;

    auto add = [&](TokenKind kind, size_t start, size_t end) -> Token & {
        Token t;
        t.kind = kind;
        t.start = start;
        t.end = end;
        t.line = line;
        out.push_back(std::move(t));
        return out.back();
    };

    while (i < src.size()) {
        SatChar c = src[i];

        if (is_space(c)) {
            if (is_newline(c))
                line++;
            i++;
            continue;
        }

        // // comment, to the end of the line. The newline itself is left for
        // the loop above so the line counter stays correct.
        if (c == SAT_SLASH && i + 1 < src.size() && src[i + 1] == SAT_SLASH) {
            while (i < src.size() && !is_newline(src[i]))
                i++;
            continue;
        }

        const size_t start = i;

        if (word_start(c)) {
            while (i < src.size() && word_cont(src[i]))
                i++;
            add(TokenKind::Word, start, i).text =
                decode(src.substr(start, i - start));
            continue;
        }

        if (is_digit(c)) {
            while (i < src.size() && is_digit(src[i]))
                i++;
            // A '.' continues the number only when a digit follows it, so
            // 3.14 is one Number while main.x is a path and 3. is 3 then '.'.
            if (i + 1 < src.size() && src[i] == SAT_DOT && is_digit(src[i + 1])) {
                i++;
                while (i < src.size() && is_digit(src[i]))
                    i++;
            }
            // No sign is ever folded in: -1 is Punct(-) then Number(1), or
            // a-1 would lex as two adjacent operands and subtraction would
            // stop working. Unary minus belongs to the expression grammar.
            std::string digits = decode(src.substr(start, i - start));
            Token &t = add(TokenKind::Number, start, i);
            Number::parse(digits, t.number);
            t.text = std::move(digits);
            continue;
        }

        if (c == SAT_QUOTE) {
            i++; // opening quote
            const size_t body = i;
            bool closed = false;
            while (i < src.size()) {
                if (is_newline(src[i]))
                    break; // a string literal may not span lines
                if (src[i] == SAT_QUOTE) {
                    closed = true;
                    break;
                }
                // Skip an escaped character so \" does not end the literal.
                i += (src[i] == SAT_BACKSLASH && i + 1 < src.size()) ? 2 : 1;
            }
            if (!closed) {
                add(TokenKind::Error, start, i).text =
                    "unterminated string literal";
                break;
            }
            Token &t = add(TokenKind::String, start, i + 1);
            // The body is still raw source text, so escapes are expanded now —
            // and only now. decode() inverts encode_raw() exactly, which is
            // what makes this round trip safe. Both forms are kept: `str` is
            // the value, `text` is how it was written, and unparse needs the
            // latter or "a\nb" would come back as a real newline in the source.
            t.text = decode(src.substr(body, i - body));
            t.str = encode(t.text);
            i++; // closing quote
            continue;
        }

        bool matched = false;
        if (i + 1 < src.size()) {
            for (const auto &op : TWO_CHAR) {
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

        // Every byte maps to something, because anything the code table has
        // not assigned lands in the raw area. So there is no such thing as an
        // unrecognised character and this branch cannot fail.
        i++;
        add(TokenKind::Punct, start, i).text = decode(src.substr(start, 1));
    }

    add(TokenKind::End, i, i);
    return out;
}

const char *kind_name(TokenKind kind)
{
    switch (kind) {
    case TokenKind::Word:   return "Word";
    case TokenKind::Number: return "Number";
    case TokenKind::String: return "String";
    case TokenKind::Punct:  return "Punct";
    case TokenKind::End:    return "End";
    case TokenKind::Error:  return "Error";
    }
    return "?";
}

std::string describe(const Token &token)
{
    switch (token.kind) {
    case TokenKind::End:
        return "End";
    case TokenKind::String:
        return "String(" + decode(token.str) + ")";
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

    // `first` is invalidated by the insert, so it must not be touched after.
    tokens.insert(tokens.begin() + static_cast<std::ptrdiff_t>(index) + 1,
                  std::move(second));
    return true;
}

} // namespace satellite
