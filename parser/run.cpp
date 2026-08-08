// Driving the parser, and rendering a parse error.
//
// Part of parser/, split from a 1022-line parser.cpp.

#include "parser_internal.hpp"

namespace satellite {

ParseResult Parser::run()
{
    ParseResult result;

    while (!at_end()) {
        const size_t before = pos_;

        if (at_language_path("include")) {
            result.program.items.push_back(parse_include());
        } else if (at_language_path("capsule")) {
            result.program.items.push_back(parse_capsule());
        } else if (at_language_path("spacesuit")) {
            result.program.items.push_back(parse_spacesuit());
        } else {
            StmtPtr s = parse_statement();
            if (s)
                result.program.items.push_back(s);
        }

        // Error recovery must always consume something, or a malformed
        // token at top level would spin here forever.
        if (pos_ == before) {
            if (errors_.empty())
                error(peek(), "unexpected " + describe(peek()));
            advance();
        }
        if (panic_)
            synchronize();
        if (errors_.size() >= MAX_ERRORS)
            break;
    }

    result.errors = std::move(errors_);
    return result;
}

ParseResult parse(const std::vector<Token> &tokens)
{
    // A lexer error is a parse error too; there is nothing useful to parse
    // past it.
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            ParseResult result;
            result.errors.push_back(ParseError{t.text, span_of(t)});
            return result;
        }
    }
    return Parser(tokens).run();
}

ParseResult parse(const std::string &source)
{
    return parse(lex(source));
}

std::string format_error(const ParseError &error, const std::string &source)
{
    // Token offsets are byte offsets, so the line can be sliced straight out
    // of the source. Computing this from decoded text would drift: decode()
    // expands \cwd to whatever the working directory happens to be.
    size_t begin = 0;
    if (error.span.start > 0 && error.span.start <= source.size()) {
        size_t nl = source.rfind('\n', error.span.start - 1);
        begin = (nl == std::string::npos) ? 0 : nl + 1;
    }
    size_t end = source.find('\n', begin);
    if (end == std::string::npos)
        end = source.size();

    const std::string line = source.substr(begin, end - begin);
    const size_t column = error.span.start >= begin ? error.span.start - begin : 0;

    std::string number = std::to_string(error.span.line);
    std::string gutter(number.size(), ' ');

    std::string out = "satellite: line " + number + ": " + error.message + "\n";
    out += " " + number + " | " + line + "\n";
    out += " " + gutter + " | " + std::string(column, ' ') + "^";

    // Underline the whole token when it spans more than one character.
    if (error.span.end > error.span.start + 1)
        out += std::string(error.span.end - error.span.start - 1, '~');

    return out;
}

} // namespace satellite
