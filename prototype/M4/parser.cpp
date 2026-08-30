// Parser driver and cursor helpers -- Milestone 4 Prototype.

#include "parser_internal.hpp"

namespace satellite {

namespace {
constexpr size_t kMaxErrors = 50;
}

Parser::Parser(const std::vector<Token> &toks, AstArena &arena, words::Words &words, uint32_t file)
    : toks_(toks), arena_(arena), words_(words), file_(file)
{
}

bool Parser::at_end() const
{
    return pos_ >= toks_.size() || toks_[pos_].kind == TokenKind::End;
}

const Token &Parser::peek(size_t offset) const
{
    static const Token end_tok{TokenKind::End, "", {}, 0, 0, 0, 0, words::kNoPath};
    if (pos_ + offset < toks_.size())
        return toks_[pos_ + offset];
    return end_tok;
}

const Token &Parser::previous() const
{
    static const Token start_tok{TokenKind::End, "", {}, 0, 0, 0, 0, words::kNoPath};
    if (pos_ > 0 && pos_ - 1 < toks_.size())
        return toks_[pos_ - 1];
    return start_tok;
}

const Token &Parser::advance()
{
    if (!at_end())
        pos_++;
    return previous();
}

bool Parser::is_punct(size_t offset, const std::string &text) const
{
    const Token &t = peek(offset);
    return t.kind == TokenKind::Punct && t.text == text;
}

bool Parser::is_word(size_t offset, const std::string &text) const
{
    const Token &t = peek(offset);
    return t.kind == TokenKind::Word && t.text == text;
}

bool Parser::match_punct(const std::string &text)
{
    if (is_punct(0, text)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::match_word(const std::string &text)
{
    if (is_word(0, text)) {
        advance();
        return true;
    }
    return false;
}

bool Parser::at_language_path(const std::string &sub) const
{
    return is_word(0, "satellite") && is_punct(1, ".") && is_word(2, sub);
}

bool Parser::at_statement_keyword(const std::string &kw) const
{
    return is_word(0, "satellite") && is_punct(1, ".") &&
           is_word(2, "statement") && is_punct(3, ".") && is_word(4, kw);
}

void Parser::take_statement_keyword()
{
    advance(); // satellite
    advance(); // .
    advance(); // statement
    advance(); // .
    advance(); // kw
}

bool Parser::at_type() const
{
    if (is_word(0, "satellite") && is_punct(1, ".") &&
        (is_word(2, "variable") || is_word(2, "container")))
        return true;

    // Bare spacesuit type: two adjacent words on the same line
    if (peek(0).kind == TokenKind::Word &&
        (peek(1).kind == TokenKind::Word || peek(1).kind == TokenKind::Bits) &&
        peek(0).line == peek(1).line)
        return true;

    return false;
}

bool Parser::expect_punct(const std::string &text, const std::string &msg)
{
    if (match_punct(text))
        return true;
    error(peek(), "expected '" + text + "'" + (msg.empty() ? "" : " " + msg) + ", found " + describe(peek()));
    return false;
}

std::string Parser::expect_word(const std::string &msg)
{
    if (peek().kind == TokenKind::Word) {
        return advance().text;
    }
    error(peek(), "expected " + msg + ", found " + describe(peek()));
    return "";
}

void Parser::expect_statement_end()
{
    while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
        advance();
}

Span Parser::span_from(size_t first) const
{
    const Token &start_tok = (first < toks_.size()) ? toks_[first] : previous();
    const Token &end_tok = previous();
    return Span{start_tok.start, end_tok.end, start_tok.line, file_};
}

Span Parser::span_of(const Token &tok) const
{
    return Span{tok.start, tok.end, tok.line, file_};
}

void Parser::error(const Token &tok, std::string msg)
{
    panic_ = true;
    errors_.push_back(ParseError{std::move(msg), span_of(tok)});
}

void Parser::synchronize()
{
    panic_ = false;
    while (!at_end()) {
        if (peek().kind == TokenKind::Newline || is_punct(0, ";")) {
            advance();
            return;
        }
        if (at_language_path("capsule") || at_language_path("spacesuit") ||
            at_language_path("include") || at_statement_keyword("if") ||
            at_statement_keyword("while") || at_statement_keyword("for"))
            return;
        advance();
    }
}

ParseResult Parser::run()
{
    ParseResult result;
    while (!at_end()) {
        // Skip leading newlines and semicolons
        while (!at_end() && (peek().kind == TokenKind::Newline || is_punct(0, ";")))
            advance();
        if (at_end())
            break;

        const size_t before = pos_;
        NodeIndex item = kNullNode;

        if (at_language_path("include")) {
            item = parse_include();
        } else if (at_language_path("capsule")) {
            item = parse_capsule();
        } else if (at_language_path("spacesuit")) {
            item = parse_spacesuit();
        } else if (is_word(0, "satellite") && is_punct(1, ".") &&
                   is_word(2, "library") && is_punct(3, ".")) {
            item = parse_global_decl();
        } else {
            item = parse_statement();
        }

        if (item != kNullNode)
            result.program.items.push_back(item);

        if (pos_ == before) {
            if (errors_.empty())
                error(peek(), "unexpected " + describe(peek()));
            advance();
        }
        if (panic_)
            synchronize();
        if (errors_.size() >= kMaxErrors)
            break;
    }

    result.errors = std::move(errors_);
    return result;
}

// ---------------------------------------------------------------------------
// Standalone Entry Points
// ---------------------------------------------------------------------------

ParseResult parse(const std::vector<Token> &tokens, AstArena &arena, words::Words &words, uint32_t file)
{
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            ParseResult res;
            res.errors.push_back(ParseError{t.text, Span{t.start, t.end, t.line, file}});
            return res;
        }
    }
    return Parser(tokens, arena, words, file).run();
}

ParseResult parse(const std::string &source, AstArena &arena, words::Words &words, uint32_t file)
{
    return parse(lex(source), arena, words, file);
}

std::string format_error(const ParseError &error, const SourceMap &sources)
{
    const std::string &source = sources.text(error.span.file);
    size_t begin = 0;
    if (error.span.start > 0 && error.span.start <= source.size()) {
        size_t nl = source.rfind('\n', error.span.start - 1);
        begin = (nl == std::string::npos) ? 0 : nl + 1;
    }
    size_t end = source.find('\n', begin);
    if (end == std::string::npos)
        end = source.size();

    const std::string line = source.substr(begin, end - begin);
    size_t column = error.span.start >= begin ? error.span.start - begin : 0;
    if (column > line.size())
        column = line.size();

    std::string number = std::to_string(error.span.line);
    std::string gutter(number.size(), ' ');

    return span_location(error.span, sources) + ": error: " + error.message + "\n" +
           number + " | " + line + "\n" +
           gutter + " | " + std::string(column, ' ') + "^\n";
}

} // namespace satellite
