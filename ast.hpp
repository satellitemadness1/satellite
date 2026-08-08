#pragma once

#include "lexer.hpp"
#include "satellite_string.hpp"

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

// The satellite syntax tree.
//
// Expr is deliberately a std::variant of node structs held by
// shared_ptr<const>, mirroring the idiom value.hpp uses for runtime values —
// but it is a SEPARATE variant, and that separation is on purpose.
//
// The original plan was to fold expression kinds into Value itself, so that
// one node type held both code and data. It reads well and it is why lists in
// value.hpp already hold pointers to other nodes. The reason not to do it is
// measurable rather than stylistic: adding the expression kinds grows every
// runtime Value from 40 to 96 bytes, so a list of a million numbers goes from
// 38 MB to 91 MB — and every runtime type check would need arms meaning "this
// can't happen", because there is no satellite.variable.<something> for a call
// node.
//
// None of the idea is lost. Code becomes data again with ONE alternative added
// to Value later — a Quoted{ProgramPtr, const Expr *}, spelled
// satellite.variable.expression — rather than fifteen. Data becomes code with
// one alternative added here. The bridge stays a bridge instead of collapsing
// the two sides into each other.
//
// Nothing here resolves anything. `satellite.time.now()` parses to a plain
// Member/Member/Call chain, exactly like `my_time.some_function()`, and the
// evaluator works out what each segment means at run time. Teaching the parser
// the shape of the standard library would mean a new grammar rule per
// namespace.

namespace satellite {

// Half-open source range, in bytes. Byte offsets rather than decoded columns
// because decode() is neither injective nor stable — \cwd changes width after
// a chdir — so a caret computed from decoded text drifts.
struct Span {
    size_t start = 0;
    size_t end = 0;
    size_t line = 1;
};

Span span_of(const Token &token);
Span span_join(Span first, Span last);

// --- types -----------------------------------------------------------------

// A type is never a value. `satellite.variable.*` and `satellite.container.*`
// are reserved as type-only namespaces, which is what keeps '<' unambiguous:
// without that rule `satellite.container.list<satellite.variable.string> argz`
// has the same token stream as the chained comparison
// ((satellite.container.list < satellite.variable.string) > argz), which is the
// C++98 a<b>c problem and is not fixable by lookahead.
//
// A SPACESUIT type is the one type spelled with a bare name, because §1 says a
// user-owned thing is bare and a spacesuit is named by the user. That does not
// reopen §4: §4's ambiguity is created by '<', and a spacesuit takes no generic
// arguments — `my_class<T>` is not grammar, so the token stream that made
// `list<string> argz` ambiguous cannot be produced. What makes a bare type
// parse at all is §5's own observation that TWO ADJACENT WORDS are a shape no
// expression can produce; the parser requires them on one line, exactly as it
// already does for `satellite.variable.number x`.
struct Type {
    std::string space;          // "variable" or "container"; empty for both the
                                // bare `satellite` singleton and a spacesuit
    std::string name;           // "time", "string", "list", or a spacesuit name
    std::vector<Type> args;     // generic arguments: list<string> has one
    Span span;

    // Empty space AND empty name: the bare `satellite` singleton type.
    bool is_singleton() const { return space.empty() && name.empty(); }

    // Empty space with a name: `my_class_name`, a user-declared spacesuit.
    bool is_spacesuit() const { return space.empty() && !name.empty(); }
};

// --- expressions -----------------------------------------------------------

struct Expr;
using ExprPtr = std::shared_ptr<const Expr>;

struct NumberLit {
    Number value;
    std::string text;           // as written, so 3.10 does not unparse as 3.1
};

struct StringLit {
    SatString value;            // escapes already expanded
    std::string source;         // as written, without the quotes
};

// The bare word `satellite`: the runtime singleton, as a value. It is what
// satellite.include(satellite) includes and what satellite.return(satellite)
// returns, and treating it as an object is also what makes satellite.time.now()
// ordinary member access rather than a special path form.
struct SatelliteLit {};

// Where a resolved name lives. Filled in by resolve() (env.hpp); see the
// contract on Name::slot below.
//
// The non-negative half of the int is a frame slot index, so everything else
// has to live in the negative half. Three of the four are single sentinels; a
// FIELD is not, because a spacesuit has one field slot per field and the index
// has to travel with the name — hence a base to count down from rather than a
// second int on every Name.
inline constexpr int SLOT_GLOBAL  = -1;   // satellite.library.<namespace>.<name>
inline constexpr int SLOT_CAPSULE = -2;   // names a capsule, not a variable
inline constexpr int SLOT_METHOD  = -3;   // names a method of the enclosing suit
inline constexpr int SLOT_SUIT    = -4;   // names a spacesuit, not a variable
inline constexpr int SLOT_FIELD   = -5;   // field 0; field i is SLOT_FIELD - i

inline bool is_field_slot(int slot) { return slot <= SLOT_FIELD; }
inline int  field_slot(int index)   { return SLOT_FIELD - index; }
inline int  field_index(int slot)   { return SLOT_FIELD - slot; }

struct Name {                                               // my_time, argz, fact
    std::string text;

    // A frame slot index when >= 0, otherwise one of the sentinels above.
    //
    // `mutable` because the tree is shared as shared_ptr<const Expr>, and that
    // makes the contract load-bearing rather than incidental: resolve() must
    // finish, on one thread, before any evaluation begins. Resolving lazily
    // during eval would be a data race the moment two threads walk one shared
    // Program — which is exactly what §6's 8-thread test does.
    mutable int slot = SLOT_GLOBAL;
};

struct Member { ExprPtr target; std::string name; };        // <expr> . name
struct Call   { ExprPtr target; std::vector<ExprPtr> args; };
struct Index  { ExprPtr target; ExprPtr subscript; };       // <expr> [ i ]

// <expr> [ lo : hi ] — either bound may be null, so l[:], l[2:], l[:5] and
// l[2:5] are all this node. Bounds are half-open: len(l[a:b]) == b - a, which
// is a subtraction with no +1 and maps straight onto List(begin+a, begin+b).
struct Slice { ExprPtr target; ExprPtr lo; ExprPtr hi; };

struct Unary  { std::string op; ExprPtr operand; };
struct Binary { std::string op; ExprPtr left; ExprPtr right; };

using ExprBase = std::variant<NumberLit, StringLit, SatelliteLit, Name,
                              Member, Call, Index, Slice, Unary, Binary>;

struct Expr : ExprBase {
    using ExprBase::ExprBase;
    // Inherited constructors deliberately skip the base's own copy and move,
    // so building an Expr out of an already-formed ExprBase needs this.
    Expr(ExprBase base) : ExprBase(std::move(base)) {}
    Span span;
};

ExprPtr make_expr(ExprBase node, Span span);

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
