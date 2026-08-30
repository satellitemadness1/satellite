#pragma once

// The satellite Arena AST -- Milestone 4 Prototype.
//
// Every node is indexed by uint32_t (NodeIndex) into a contiguous AstArena.
// No std::shared_ptr is used anywhere in the tree (PLAN §2.2, §8).
//
// Multi-threaded walking is atomic-free and safe.
// Resolved frame and word metadata lives in separate side-tables rather than
// mutable fields on AST nodes.

#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace satellite {

using NodeIndex = uint32_t;
inline constexpr NodeIndex kNullNode = 0;

// Half-open byte range in source text.
struct Span {
    uint32_t start = 0;
    uint32_t end = 0;
    uint32_t line = 1;
    uint32_t file = 0;
};

// ---------------------------------------------------------------------------
// Types
// ---------------------------------------------------------------------------

struct Type {
    std::string space;          // "variable", "container", or empty
    std::string name;           // "string", "number", "list", spacesuit name, etc.
    std::vector<Type> args;     // Generic type arguments, e.g. list<string>
    Span span;

    bool is_singleton() const { return space.empty() && name.empty(); }
    bool is_spacesuit() const { return space.empty() && !name.empty(); }
};

enum class Access : uint8_t {
    Protected,
    Public,
};

inline const char *access_keyword(Access a)
{
    return (a == Access::Public) ? "satellite.public" : "satellite.protected";
}

// ---------------------------------------------------------------------------
// AST Node Payloads
// ---------------------------------------------------------------------------

enum class NodeKind : uint8_t {
    None = 0,

    // Expressions
    NumberLit,
    DurationLit,
    BitsLit,
    StringLit,
    SatelliteLit,
    Name,
    Member,
    Call,
    Index,
    Slice,
    Unary,
    Binary,
    ListLit,
    NamedArg,

    // Statements
    VarDecl,
    Assign,
    ExprStmt,
    Return,
    Block,
    If,
    While,
    For,

    // Declarations & Top-level
    Include,
    Capsule,
    Spacesuit,
    GlobalDecl,
};

struct NumberLit {
    std::string text;
};

inline constexpr uint64_t kNsPerMs = 1'000'000ULL;
inline constexpr uint64_t kNsPerUs = 1'000ULL;
inline constexpr uint64_t kNsPerS  = 1'000'000'000ULL;

struct DurationLit {
    std::string text;
    uint64_t nanoseconds = 0;
};

struct BitsLit {
    std::string text;
    unsigned radix = 0; // 2 or 16
};

struct StringLit {
    std::string text;   // raw source body without quotes
    SatString str;      // escape sequences expanded
};

struct SatelliteLit {};

struct NameExpr {
    std::string text;
};

struct MemberExpr {
    NodeIndex target = kNullNode;
    std::string name;
};

struct CallExpr {
    NodeIndex target = kNullNode;
    std::vector<NodeIndex> args;
};

struct IndexExpr {
    NodeIndex target = kNullNode;
    NodeIndex subscript = kNullNode;
};

struct SliceExpr {
    NodeIndex target = kNullNode;
    NodeIndex lo = kNullNode;
    NodeIndex hi = kNullNode;
};

struct UnaryExpr {
    std::string op;
    NodeIndex operand = kNullNode;
};

struct BinaryExpr {
    std::string op;
    NodeIndex left = kNullNode;
    NodeIndex right = kNullNode;
};

struct ListLit {
    std::vector<NodeIndex> elements;
};

struct NamedArg {
    std::string name;
    NodeIndex value = kNullNode;
};

struct VarDeclStmt {
    Type type;
    std::string name;
    NodeIndex init = kNullNode;
};

struct AssignStmt {
    NodeIndex target = kNullNode;
    NodeIndex value = kNullNode;
};

struct ExprStmt {
    NodeIndex expr = kNullNode;
};

struct ReturnStmt {
    NodeIndex value = kNullNode; // kNullNode for bare satellite.return()
};

struct BlockStmt {
    std::vector<NodeIndex> statements;
};

struct IfStmt {
    NodeIndex condition = kNullNode;
    NodeIndex then_branch = kNullNode;
    NodeIndex else_branch = kNullNode;
};

struct WhileStmt {
    NodeIndex condition = kNullNode;
    NodeIndex body = kNullNode;
};

struct ForStmt {
    NodeIndex init = kNullNode;
    NodeIndex condition = kNullNode;
    NodeIndex step = kNullNode;
    NodeIndex body = kNullNode;
};

struct IncludeDecl {
    NodeIndex what = kNullNode;
};

struct ParamDecl {
    Type type;
    std::string name;
    Span span;
};

struct CapsuleDecl {
    bool reserved = false;
    std::string name;
    std::vector<ParamDecl> params;
    std::optional<Type> returns;
    NodeIndex body = kNullNode;
    words::PathId path_id = words::kNoPath;
    Span span;
};

struct FieldDecl {
    Access access = Access::Protected;
    Type type;
    std::string name;
    NodeIndex init = kNullNode;
    Span span;
};

struct MethodDecl {
    Access access = Access::Protected;
    bool constructor = false;
    CapsuleDecl capsule;
    Span span;
};

using SuitItem = std::variant<FieldDecl, MethodDecl>;

struct SpacesuitDecl {
    std::string name;
    std::string super;
    Span super_span;
    std::vector<SuitItem> items;
    words::PathId path_id = words::kNoPath;
};

struct GlobalDecl {
    std::string name;
    NodeIndex init = kNullNode;
    words::PathId path_id = words::kNoPath;
};

using NodeData = std::variant<
    std::monostate,
    NumberLit, DurationLit, BitsLit, StringLit, SatelliteLit,
    NameExpr, MemberExpr, CallExpr, IndexExpr, SliceExpr,
    UnaryExpr, BinaryExpr, ListLit, NamedArg,
    VarDeclStmt, AssignStmt, ExprStmt, ReturnStmt, BlockStmt,
    IfStmt, WhileStmt, ForStmt,
    IncludeDecl, CapsuleDecl, SpacesuitDecl, GlobalDecl
>;

struct AstNode {
    NodeKind kind = NodeKind::None;
    Span span;
    NodeData data;
};

struct Program {
    std::vector<NodeIndex> items;
};

// ---------------------------------------------------------------------------
// AstArena
// ---------------------------------------------------------------------------

class AstArena {
public:
    AstArena();

    NodeIndex make_number(std::string text, Span span);
    NodeIndex make_duration(std::string text, uint64_t ns, Span span);
    NodeIndex make_bits(std::string text, unsigned radix, Span span);
    NodeIndex make_string(std::string text, SatString str, Span span);
    NodeIndex make_satellite(Span span);
    NodeIndex make_name(std::string text, Span span);
    NodeIndex make_member(NodeIndex target, std::string name, Span span);
    NodeIndex make_call(NodeIndex target, std::vector<NodeIndex> args, Span span);
    NodeIndex make_index(NodeIndex target, NodeIndex subscript, Span span);
    NodeIndex make_slice(NodeIndex target, NodeIndex lo, NodeIndex hi, Span span);
    NodeIndex make_unary(std::string op, NodeIndex operand, Span span);
    NodeIndex make_binary(std::string op, NodeIndex left, NodeIndex right, Span span);
    NodeIndex make_list(std::vector<NodeIndex> elements, Span span);
    NodeIndex make_named_arg(std::string name, NodeIndex value, Span span);

    NodeIndex make_var_decl(Type type, std::string name, NodeIndex init, Span span);
    NodeIndex make_assign(NodeIndex target, NodeIndex value, Span span);
    NodeIndex make_expr_stmt(NodeIndex expr, Span span);
    NodeIndex make_return(NodeIndex value, Span span);
    NodeIndex make_block(std::vector<NodeIndex> stmts, Span span);
    NodeIndex make_if(NodeIndex cond, NodeIndex then_b, NodeIndex else_b, Span span);
    NodeIndex make_while(NodeIndex cond, NodeIndex body, Span span);
    NodeIndex make_for(NodeIndex init, NodeIndex cond, NodeIndex step, NodeIndex body, Span span);

    NodeIndex make_include(NodeIndex what, Span span);
    NodeIndex make_capsule(bool reserved, std::string name, std::vector<ParamDecl> params,
                           std::optional<Type> returns, NodeIndex body, words::PathId path_id, Span span);
    NodeIndex make_spacesuit(std::string name, std::string super, Span super_span,
                             std::vector<SuitItem> items, words::PathId path_id, Span span);
    NodeIndex make_global_decl(std::string name, NodeIndex init, words::PathId path_id, Span span);

    const AstNode &get(NodeIndex idx) const;
    AstNode &get(NodeIndex idx);
    size_t size() const { return nodes_.size(); }
    void clear();

private:
    NodeIndex alloc_node(NodeKind kind, Span span, NodeData data);
    std::vector<AstNode> nodes_;
};

// ---------------------------------------------------------------------------
// SourceMap & Span helpers
// ---------------------------------------------------------------------------

class SourceMap {
public:
    SourceMap() = default;
    explicit SourceMap(std::string text, std::string path = {});

    uint32_t add(std::string text, std::string path = {});
    const std::string &text(uint32_t file) const;
    const std::string &path(uint32_t file) const;
    size_t size() const { return files_.size(); }

private:
    struct Entry {
        std::string text;
        std::string path;
    };
    std::vector<Entry> files_;
};

std::string span_location(const Span &span, const SourceMap &sources);
Span span_join(Span first, Span last);

int precedence(const std::string &op);
bool is_unary_op(const std::string &op);

} // namespace satellite

