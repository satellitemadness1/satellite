#pragma once

#include "abstract_syntax_tree/ast_expr.hpp"
#include "abstract_syntax_tree/ast_span.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace satellite {

// --- statements ------------------------------------------------------------

struct Stmt;
using StmtPtr = std::shared_ptr<const Stmt>;

struct VarDecl {
    Type type;
    std::string name;           // bare: the user names it, the language types it
    ExprPtr init;               // null for a declaration with no initialiser

    // The slot this declaration writes. Same contract as Name::slot.
    mutable int slot = SLOT_GLOBAL;
};

struct Assign   { ExprPtr target; ExprPtr value; };
struct ExprStmt { ExprPtr expr; };
struct Return   { ExprPtr value; };                 // null for satellite.return()
struct Block    { std::vector<StmtPtr> statements; };
struct If       { ExprPtr condition; StmtPtr then_branch; StmtPtr else_branch; };
struct While    { ExprPtr condition; StmtPtr body; };

// satellite.statement.for(init; condition; step) { body }
//
// Three-part and C-shaped. It needs no syntax the language did not already
// have: ';' is an ordinary punctuation code, `init` is the same VarDecl a
// declaration statement produces, and `step` is the same Assign. Any of the
// three parts may be null, so for(;;) is the infinite loop.
struct For {
    StmtPtr init;           // usually a VarDecl; may be null
    ExprPtr condition;      // null means "always true"
    StmtPtr step;           // usually an Assign; may be null
    StmtPtr body;
};

using StmtBase = std::variant<VarDecl, Assign, ExprStmt, Return, Block,
                              If, While, For>;

struct Stmt : StmtBase {
    using StmtBase::StmtBase;
    Stmt(StmtBase base) : StmtBase(std::move(base)) {}
    Span span;
};

// See the note on Span. Stmt is the largest node in the tree and the one most
// likely to grow by accident, since every new statement kind widens the variant
// to its biggest alternative.
static_assert(sizeof(void *) != 8 || sizeof(Stmt) == 200,
              "Stmt must stay 200 bytes on 64-bit — see the note on Span");

StmtPtr make_stmt(StmtBase node, Span span);

// --- top level -------------------------------------------------------------

struct Param {
    Type type;
    std::string name;
    Span span;
};

struct Capsule {
    // satellite.main is prefixed because the runtime chooses and calls that
    // name; a capsule the user writes is bare, like every other name they pick.
    bool reserved = false;
    std::string name;
    std::vector<Param> params;
    std::optional<Type> returns;    // satellite.returns(TYPE)
    StmtPtr body;                   // always a Block
    Span span;
};

struct Include {
    ExprPtr what;
    Span span;
};

// --- spacesuits ------------------------------------------------------------

// satellite.spacesuit my_class_name(superclass) { ... } — a class.
//
// `spacesuit` earns the name: it is what a capsule's contents travel in, it is
// worn by one occupant at a time, and it is the thing that has an inside and an
// outside — which is the whole of `satellite.protected` and `satellite.public`.
//
// Every member sits inside an access block. Requiring that rather than
// defaulting an unannotated member keeps one canonical spelling, so unparse has
// one form to emit and a reader never has to remember which way the default
// goes.
enum class Access {
    Protected,      // reachable from this spacesuit and its descendants
    Public,         // reachable from anywhere
};

// A field is storage, so it is shaped like the VarDecl it reads as. It gets no
// `slot` of its own: field positions are assigned per SPACESUIT, not per
// declaration, because a subclass lays its parent's fields down first (env.hpp)
// and the same Field can therefore sit at one index in one instance layout.
struct Field {
    Access access = Access::Protected;
    Type type;
    std::string name;
    ExprPtr init;               // null for a field with no initialiser
    Span span;
};

// A method is an ordinary Capsule plus who may call it. Nothing else differs:
// it takes the same parameters, declares the same satellite.returns, and gets
// the same frame — the only addition at run time is a receiver.
//
// A CONSTRUCTOR is the same thing with `constructor` set. It is spelled with
// the spacesuit's own name and no `satellite.capsule` in front of it, because
// it is the one member that is never called by name: the construction site
// runs it, the way the runtime runs satellite.main. That also means it has no
// satellite.returns — what it produces is the object.
struct Method {
    Access access = Access::Protected;
    bool constructor = false;
    Capsule capsule;
};

// One vector rather than one per kind, for the same reason Program::items is
// one vector: unparse has to reproduce the order the members were written in.
using SuitItem = std::variant<Field, Method>;

struct Spacesuit {
    std::string name;
    std::string super;          // bare superclass name; empty when there is none
    Span super_span;
    std::vector<SuitItem> items;
    Span span;
};

// One vector rather than one per kind, so unparse can reproduce the order the
// items were written in.
using TopLevel = std::variant<Include, Capsule, Spacesuit, StmtPtr>;

struct Program {
    std::vector<TopLevel> items;
};

// --- operators -------------------------------------------------------------

// Binding power, higher binds tighter. 0 means "not a binary operator".
// Logical and/or are deliberately absent until they are specified.
int precedence(const std::string &op);
bool is_unary_op(const std::string &op);

// Control flow lives in its own language namespace, so the rule that anything
// the language provides is satellite-rooted holds with no exceptions. Together
// with `variable`, `container` and `library`, `statement` is one of the second
// segments the parser dispatches on: seeing it means a statement form follows,
// not a type and not a value.
inline constexpr const char *KW_IF    = "satellite.statement.if";
inline constexpr const char *KW_ELSE  = "satellite.statement.else";
inline constexpr const char *KW_WHILE = "satellite.statement.while";
inline constexpr const char *KW_FOR   = "satellite.statement.for";

// `spacesuit`, `protected` and `public` join `capsule`, `include` and `returns`
// as second segments the parser dispatches on. None of them is reserved — §1's
// one reserved word is still `satellite` — because each is special only in the
// second position of a satellite-rooted path, so a variable may still be called
// `public`.
inline constexpr const char *KW_SPACESUIT = "satellite.spacesuit";
inline constexpr const char *KW_PROTECTED = "satellite.protected";
inline constexpr const char *KW_PUBLIC    = "satellite.public";

inline const char *access_keyword(Access access)
{
    return access == Access::Public ? KW_PUBLIC : KW_PROTECTED;
}

// --- unparse ---------------------------------------------------------------

// Renders a tree back to source in canonical form: four-space indents, spaces
// around binary operators and '=', none around dots or inside brackets.
//
// unparse(parse(src)) == src holds when src is already canonical, which is what
// makes it a usable round-trip check for the parser. It is not an identity for
// arbitrary input, because comments, blank lines and redundant parentheses are
// not in the tree. What always holds is idempotence: unparsing a tree and
// reparsing it gives the same text back.
std::string unparse(const Program &program);
std::string unparse(const Expr &expr);
std::string unparse(const Stmt &stmt, int level = 0);
std::string unparse(const Type &type);
std::string unparse(const Capsule &capsule, int level = 0);
std::string unparse(const Include &include);
std::string unparse(const Field &field, int level = 0);
std::string unparse(const Spacesuit &suit);
std::string unparse(const TopLevel &item);

} // namespace satellite
