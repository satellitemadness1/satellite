#include "abstract_syntax_tree/ast.hpp"

#include <utility>

namespace satellite {

Span span_of(const Token &token, uint32_t file)
{
    return Span{token.start, token.end, static_cast<uint32_t>(token.line), file};
}

// Takes the FIRST span's file, not the last's. The two are always the same
// today — a construct cannot begin in one spaceship and end in another, since
// each is parsed whole before the next is loaded — and taking the first is the
// answer that stays right if that ever stops being true, because the first is
// where the reader's eye goes.
Span span_join(Span first, Span last)
{
    return Span{first.start, last.end, first.line, first.file};
}

SourceMap::SourceMap(std::string text, std::string path)
{
    add(std::move(text), std::move(path));
}

uint32_t SourceMap::add(std::string text, std::string path)
{
    files_.push_back(Entry{std::move(text), std::move(path)});
    return static_cast<uint32_t>(files_.size() - 1);
}

const std::string &SourceMap::text(uint32_t file) const
{
    static const std::string none;
    return file < files_.size() ? files_[file].text : none;
}

const std::string &SourceMap::path(uint32_t file) const
{
    static const std::string none;
    return file < files_.size() ? files_[file].path : none;
}

std::string span_location(const Span &span, const SourceMap &sources)
{
    const std::string &path = sources.path(span.file);
    if (path.empty())
        return "line " + std::to_string(span.line);
    return path + ":" + std::to_string(span.line);
}

ExprPtr make_expr(ExprBase node, Span span)
{
    Expr e(std::move(node));
    e.span = span;
    return std::make_shared<const Expr>(std::move(e));
}

StmtPtr make_stmt(StmtBase node, Span span)
{
    Stmt s(std::move(node));
    s.span = span;
    return std::make_shared<const Stmt>(std::move(s));
}

// ---------------------------------------------------------------------------
// Operators
// ---------------------------------------------------------------------------

int precedence(const std::string &op)
{
    if (op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=")
        return 1;
    if (op == "+" || op == "-")
        return 2;
    if (op == "*" || op == "/" || op == "%")
        return 3;
    return 0;
}

bool is_unary_op(const std::string &op)
{
    return op == "-" || op == "!";
}

} // namespace satellite
