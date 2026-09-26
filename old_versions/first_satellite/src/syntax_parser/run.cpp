// Driving the parser, and rendering a parse error.
//
// Part of src/syntax_parser/, split from a 1022-line parser.cpp.

#include "syntax_parser/parser_internal.hpp"

#include <algorithm>

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

ParseResult parse(const std::vector<Token> &tokens, uint32_t file)
{
    // A lexer error is a parse error too; there is nothing useful to parse
    // past it.
    for (const Token &t : tokens) {
        if (t.kind == TokenKind::Error) {
            ParseResult result;
            result.errors.push_back(ParseError{t.text, span_of(t, file)});
            return result;
        }
    }
    return Parser(tokens, file).run();
}

ParseResult parse(const std::string &source, uint32_t file)
{
    return parse(lex(source), file);
}

std::string format_error(const ParseError &error, const SourceMap &sources)
{
    const std::string &source = sources.text(error.span.file);

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

    // Clamped to the line, which it was not before §16. While format_error was
    // handed the one and only source, a span was always an offset into the text
    // it was rendered against and could not point past it. A span now names its
    // own source, so a caller with a stale or unknown file id gets a short text
    // and an offset from a longer one — and an unclamped column would pad the
    // caret line with thousands of spaces. Clamping draws the caret at the end
    // of the line instead, which is wrong but bounded and visibly wrong.
    size_t column = error.span.start >= begin ? error.span.start - begin : 0;
    if (column > line.size())
        column = line.size();

    std::string number = std::to_string(error.span.line);
    std::string gutter(number.size(), ' ');

    // The gutter stays the bare line number even when the header names a
    // spaceship: it labels the source line printed below it, and widening it to
    // a whole path would push the caret block off the screen for no gain.
    std::string out =
        "satellite: " + span_location(error.span, sources) + ": " +
        error.message + "\n";
    out += " " + number + " | " + line + "\n";
    out += " " + gutter + " | " + std::string(column, ' ') + "^";

    // Underline the whole token when it spans more than one character, never
    // past the end of the line printed above it — same reason as the column.
    if (error.span.end > error.span.start + 1) {
        size_t width = error.span.end - error.span.start - 1;
        width = std::min(width, line.size() - column);
        out += std::string(width, '~');
    }

    // Terminated, like the resolver's and the evaluator's. This one was not,
    // and it did not show while a program was one file: a lone parse error is
    // the last thing printed, so the missing newline only ever cost the shell
    // prompt its own line. §16 makes it show — the loader keeps parsing after
    // a bad spaceship, so two files with a syntax error each used to render
    // the second one's caret welded onto the first one's underline.
    return out + "\n";
}

} // namespace satellite
