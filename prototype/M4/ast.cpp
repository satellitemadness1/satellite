// Arena AST implementation -- Milestone 4 Prototype.

#include "ast.hpp"

#include <utility>

namespace satellite {

AstArena::AstArena()
{
    // Index 0 is kNullNode
    nodes_.push_back(AstNode{NodeKind::None, Span{}, std::monostate{}});
}

void AstArena::clear()
{
    nodes_.clear();
    nodes_.push_back(AstNode{NodeKind::None, Span{}, std::monostate{}});
}

NodeIndex AstArena::alloc_node(NodeKind kind, Span span, NodeData data)
{
    NodeIndex idx = static_cast<NodeIndex>(nodes_.size());
    nodes_.push_back(AstNode{kind, span, std::move(data)});
    return idx;
}

const AstNode &AstArena::get(NodeIndex idx) const
{
    if (idx < nodes_.size())
        return nodes_[idx];
    return nodes_[0];
}

AstNode &AstArena::get(NodeIndex idx)
{
    if (idx < nodes_.size())
        return nodes_[idx];
    return nodes_[0];
}

NodeIndex AstArena::make_number(std::string text, Span span)
{
    return alloc_node(NodeKind::NumberLit, span, NumberLit{std::move(text)});
}

NodeIndex AstArena::make_duration(std::string text, uint64_t ns, Span span)
{
    return alloc_node(NodeKind::DurationLit, span, DurationLit{std::move(text), ns});
}

NodeIndex AstArena::make_bits(std::string text, unsigned radix, Span span)
{
    return alloc_node(NodeKind::BitsLit, span, BitsLit{std::move(text), radix});
}

NodeIndex AstArena::make_string(std::string text, SatString str, Span span)
{
    return alloc_node(NodeKind::StringLit, span, StringLit{std::move(text), std::move(str)});
}

NodeIndex AstArena::make_satellite(Span span)
{
    return alloc_node(NodeKind::SatelliteLit, span, SatelliteLit{});
}

NodeIndex AstArena::make_name(std::string text, Span span)
{
    return alloc_node(NodeKind::Name, span, NameExpr{std::move(text)});
}

NodeIndex AstArena::make_member(NodeIndex target, std::string name, Span span)
{
    return alloc_node(NodeKind::Member, span, MemberExpr{target, std::move(name)});
}

NodeIndex AstArena::make_call(NodeIndex target, std::vector<NodeIndex> args, Span span)
{
    return alloc_node(NodeKind::Call, span, CallExpr{target, std::move(args)});
}

NodeIndex AstArena::make_index(NodeIndex target, NodeIndex subscript, Span span)
{
    return alloc_node(NodeKind::Index, span, IndexExpr{target, subscript});
}

NodeIndex AstArena::make_slice(NodeIndex target, NodeIndex lo, NodeIndex hi, Span span)
{
    return alloc_node(NodeKind::Slice, span, SliceExpr{target, lo, hi});
}

NodeIndex AstArena::make_unary(std::string op, NodeIndex operand, Span span)
{
    return alloc_node(NodeKind::Unary, span, UnaryExpr{std::move(op), operand});
}

NodeIndex AstArena::make_binary(std::string op, NodeIndex left, NodeIndex right, Span span)
{
    return alloc_node(NodeKind::Binary, span, BinaryExpr{std::move(op), left, right});
}

NodeIndex AstArena::make_list(std::vector<NodeIndex> elements, Span span)
{
    return alloc_node(NodeKind::ListLit, span, ListLit{std::move(elements)});
}

NodeIndex AstArena::make_named_arg(std::string name, NodeIndex value, Span span)
{
    return alloc_node(NodeKind::NamedArg, span, NamedArg{std::move(name), value});
}

NodeIndex AstArena::make_var_decl(Type type, std::string name, NodeIndex init, Span span)
{
    return alloc_node(NodeKind::VarDecl, span, VarDeclStmt{std::move(type), std::move(name), init});
}

NodeIndex AstArena::make_assign(NodeIndex target, NodeIndex value, Span span)
{
    return alloc_node(NodeKind::Assign, span, AssignStmt{target, value});
}

NodeIndex AstArena::make_expr_stmt(NodeIndex expr, Span span)
{
    return alloc_node(NodeKind::ExprStmt, span, ExprStmt{expr});
}

NodeIndex AstArena::make_return(NodeIndex value, Span span)
{
    return alloc_node(NodeKind::Return, span, ReturnStmt{value});
}

NodeIndex AstArena::make_block(std::vector<NodeIndex> stmts, Span span)
{
    return alloc_node(NodeKind::Block, span, BlockStmt{std::move(stmts)});
}

NodeIndex AstArena::make_if(NodeIndex cond, NodeIndex then_b, NodeIndex else_b, Span span)
{
    return alloc_node(NodeKind::If, span, IfStmt{cond, then_b, else_b});
}

NodeIndex AstArena::make_while(NodeIndex cond, NodeIndex body, Span span)
{
    return alloc_node(NodeKind::While, span, WhileStmt{cond, body});
}

NodeIndex AstArena::make_for(NodeIndex init, NodeIndex cond, NodeIndex step, NodeIndex body, Span span)
{
    return alloc_node(NodeKind::For, span, ForStmt{init, cond, step, body});
}

NodeIndex AstArena::make_include(NodeIndex what, Span span)
{
    return alloc_node(NodeKind::Include, span, IncludeDecl{what});
}

NodeIndex AstArena::make_capsule(bool reserved, std::string name, std::vector<ParamDecl> params,
                                 std::optional<Type> returns, NodeIndex body, words::PathId path_id, Span span)
{
    return alloc_node(NodeKind::Capsule, span, CapsuleDecl{reserved, std::move(name), std::move(params), std::move(returns), body, path_id, span});
}

NodeIndex AstArena::make_spacesuit(std::string name, std::string super, Span super_span,
                                   std::vector<SuitItem> items, words::PathId path_id, Span span)
{
    return alloc_node(NodeKind::Spacesuit, span, SpacesuitDecl{std::move(name), std::move(super), super_span, std::move(items), path_id});
}

NodeIndex AstArena::make_global_decl(std::string name, NodeIndex init, words::PathId path_id, Span span)
{
    return alloc_node(NodeKind::GlobalDecl, span, GlobalDecl{std::move(name), init, path_id});
}

// ---------------------------------------------------------------------------
// SourceMap & Helpers
// ---------------------------------------------------------------------------

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

Span span_join(Span first, Span last)
{
    return Span{first.start, last.end, first.line, first.file};
}

int precedence(const std::string &op)
{
    if (op == "==" || op == "!=" || op == "<" || op == ">" ||
        op == "<=" || op == ">=")
        return 1;
    if (op == "+" || op == "-")
        return 2;
    if (op == "*" || op == "/" || op == "%")
        return 3;
    if (op == "&" || op == "|" || op == "^")
        return 1;
    return 0;
}

bool is_unary_op(const std::string &op)
{
    return op == "-" || op == "!" || op == "~";
}

} // namespace satellite

