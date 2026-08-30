#pragma once

// The arena AST -- PLAN M4. Tokens in, a contiguous vector of small PODs out.
//
// PLAN §2.2 IS THE SPECIFICATION AND IT IS A MEASUREMENT, not a preference. The
// first satellite's tree is `shared_ptr<const Expr>` with a static_assert
// pinning `sizeof(Expr)` at 96, and PLAN §2.2 names the three costs: a cache
// miss per child, 96 bytes paid by every node to fit the widest alternative,
// and a refcount that buys nothing because the tree is immutable, lives as long
// as the program and never frees a node early. The fix is one sentence -- an
// arena of PODs indexed by uint32_t -- and everything below is that sentence
// with the corners filled in.
//
// WHAT THE ARENA BUYS THAT A TIDIER TREE WOULD NOT. Walking becomes ATOMIC-FREE
// rather than merely safe, which is what DESIGN §10.5's threads need; and
// `Name::slot`, which in the first satellite is a `mutable int` on a
// `shared_ptr<const Expr>` held off by a comment reading "resolve() must finish,
// on one thread, before any evaluation begins", becomes a side table indexed by
// node index -- so the race is structurally impossible instead of documented.
// M7 builds that side table. M4 must not put a mutable field on a node.
//
// NO STRING LIVES IN THIS TREE. Every node that has text -- a number, a string,
// a name, an operator -- carries the INDEX OF ITS TOKEN instead, and the token
// vector comes along with the arena. That is not a saving of bytes so much as a
// saving of truth: lexer.hpp says a Token's `text` is "the token as WRITTEN,
// which is what unparse round-trips", so a tree that copied the text would
// have two spellings of one fact and a `--unparse` that could drift from the
// file it read. It also makes every node's span free.

#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstddef>
#include <cstdint>
#include <string_view>
#include <type_traits>
#include <vector>

namespace satellite {

// An index into the arena. 0 is not a node, exactly as PathId 0 is not a path
// and for the same reason: an absent child is a legal answer everywhere in this
// grammar -- `satellite.return()` has no value, a var_decl has no initialiser,
// an `if` has no else -- and a sentinel that is also a valid index is how those
// three become the same bug.
using NodeIndex = uint32_t;

inline constexpr NodeIndex kNoNode = 0;

// A handle to a list of children, or 0 for the empty list.
//
// THE COUNT IS STORED WITH THE LIST rather than in the node, and that is what
// keeps a node at four payload words when `satellite.capsule` needs a path id,
// a parameter list, a return type and a body. Handle 0 is the empty list with
// no special case anywhere: the arena's list vector starts as a single 0, so
// reading the count at handle 0 reads that 0.
using ListId = uint32_t;

inline constexpr ListId kNoList = 0;

// WHAT A NODE CAN BE, and every one of them comes from DESIGN §6's grammar
// rather than from what a parser might find convenient. The grammar has no list
// literal, no named argument and no duration, so neither does this enum: a kind
// that nothing in §6 can produce is a kind that the unparser can print and no
// program can write, which is a way for the tree and the language to disagree.
enum class NodeKind : uint8_t {
    None = 0,

    Program,    // the whole file

    // Primaries -- DESIGN §6's `primary`.
    Number,
    String,
    Bits,
    Satellite,  // the reserved word used as a value, `satellite.return(satellite)`
    Name,       // a bare IDENT

    // The postfix chain -- §6.2. Member and Call are PEERS, which is what makes
    // `satellite.time.now().some_function()` fall out with no extra rule.
    Member,
    Call,
    Index,
    Slice,

    Unary,
    Binary,

    Type,       // §6's `type`, in all three of its forms

    // Statements -- §6's `statement`.
    VarDecl,
    Assign,
    ExprStmt,
    Return,
    Block,
    If,
    While,
    For,

    // Declarations -- §6's `top_level`, plus the two forms inside a suit block.
    Include,
    Capsule,
    Spacesuit,
    Section,    // satellite.protected { ... } / satellite.public { ... }
    Global,     // satellite.library.IDENT [= expression]
};

// ONE NODE. Five words, and the four payload words mean different things in
// each kind -- which is the price of a POD arena and is paid here, in one
// table, rather than by a reader who has to guess.
//
//   kind        token             a                b               c            d
//   ---------------------------------------------------------------------------
//   Program     0                 items list       -               -            -
//   Number      the number        -                -               -            -
//   String      the string        -                -               -            -
//   Bits        the literal       -                -               -            -
//   Satellite   `satellite`       -                -               -            -
//   Name        the identifier    -                -               -            -
//   Member      the member name   receiver         -               -            -
//   Call        the `(`           target           args list       -            -
//   Index       the `[`           target           subscript       -            -
//   Slice       the `[`           target           low or none     high or none -
//   Unary       the operator      operand          -               -            -
//   Binary      the operator      left             right           -            -
//   Type        the type's name   space SPELLING   generic list    -            -
//   VarDecl     the declared name type             init or none    -            -
//   Assign      the `=`           target           value           -            -
//   ExprStmt    the expression's  expression       -               -            -
//   Return      `satellite`       value or none    -               -            -
//   Block       the `{`           statements list  -               -            -
//   If          `satellite`       condition        then block      else or none -
//   While       `satellite`       condition        body            -            -
//   For         `satellite`       init or none     condition       step or none body
//   Include     `satellite`       argument         -               -            -
//   Capsule     the capsule name  PATH ID          params list     returns type body
//   Spacesuit   the suit name     PATH ID          items list      super or none -
//   Section     `protected`/`public`  items list   -               -            -
//   Global      the global's name PATH ID          init or none    -            -
//
// THREE OF THOSE COLUMNS DO NOT HOLD A NODE INDEX and the table is the only
// place that can be seen: `Type::a` is a words::SpellingId, and the path id on
// Capsule, Spacesuit and Global is a words::PathId. They are all uint32_t, so
// nothing here can catch a confusion between them -- which is the same hazard
// lexer.hpp spends a page on for SpellingId against PathId, one milestone on
// and one type further from the compiler's help.
//
// THE ANCHOR TOKEN IS THE TOKEN THAT NAMES THE NODE, not the node's first
// token, and the difference shows on `Binary` -- the operator rather than the
// left operand. That is the choice a diagnostic wants: M5 puts a caret under
// this token, and `a + ` should point at the `+`.
//
// WHAT IS DELIBERATELY NOT HERE IS A NODE'S EXTENT. One token gives a span, and
// a span from the first token to the last would give the underline M5 draws
// under a whole expression. It is not stored because the reporter that would
// use it does not exist yet and its shape would be guessed; the extent is
// recoverable by walking a node's children, and M5 is where the walk becomes a
// field if it turns out to be wanted on every node. M3 made the same call about
// the Error token's missing code, and MILESTONES/M3.md §6 records it.
struct Node {
    NodeKind kind = NodeKind::None;
    uint32_t token = 0;
    uint32_t a = 0;
    uint32_t b = 0;
    uint32_t c = 0;
    uint32_t d = 0;
};

// THE TWO PROPERTIES PLAN §2.2 ACTUALLY ASKS FOR, asserted rather than
// described, because both are invisible at a call site and neither would break
// a build if it stopped being true.
//
// 24 bytes against the first satellite's 96. The number is asserted so that a
// field added without thinking is a compile error naming this line, instead of
// a tree that quietly grew by a third -- which is exactly how the 96 happened,
// one alternative at a time.
static_assert(sizeof(Node) == 24,
              "ast.hpp: a Node is five words. PLAN §2.2 is the argument and the "
              "first satellite's 96 bytes is what it is measured against");

// Trivially copyable is the POD half of "an arena of PODs", and it is what
// M4.5's `.satc` writer will want: a vector of these is bytes on a disk and
// back with no visitor and no per-node allocation. A std::variant of node
// structs -- the obvious C++ answer, and what the M4 prototype did -- makes
// every node as wide as the widest alternative and this assert impossible.
static_assert(std::is_trivially_copyable_v<Node>,
              "ast.hpp: a Node holds no owning member -- text lives in the "
              "token stream and children live in indices");

// The arena: the nodes, the child lists, and the tokens they point into.
//
// THE TOKENS ARE PART OF THE TREE. Handing back an arena whose nodes index a
// token vector somebody else owns is a dangling reference waiting for its first
// caller, and the caller it would get is `--unparse`, which reads a token's
// text for every literal in the file.
class Ast {
public:
    Ast() = default;

    explicit Ast(std::vector<Token> tokens) : tokens_(std::move(tokens)) {}

    // A new node of `kind`, anchored at `token`. The four payload words are the
    // table above and this is the one function that writes them.
    NodeIndex add(NodeKind kind, uint32_t token, uint32_t a = 0, uint32_t b = 0,
                  uint32_t c = 0, uint32_t d = 0);

    // A list of children, copied into the arena's own storage.
    ListId add_list(const std::vector<NodeIndex> &items);

    const Node &operator[](NodeIndex index) const { return nodes_[index]; }

    // The children behind a handle. Returns an empty view for kNoList without
    // a branch, because lists_[0] is a stored 0.
    std::vector<NodeIndex>::const_iterator list_begin(ListId list) const
    {
        return lists_.begin() + list + 1;
    }

    uint32_t list_size(ListId list) const { return lists_[list]; }

    NodeIndex list_at(ListId list, uint32_t i) const { return lists_[list + 1 + i]; }

    const Token &token_of(NodeIndex index) const { return tokens_[nodes_[index].token]; }

    const Token &token(uint32_t index) const { return tokens_[index]; }

    // The text a node's anchor token was written with -- lexer.hpp's `text`,
    // which is the source's spelling and not an expansion of it.
    std::string_view text_of(NodeIndex index) const;

    NodeIndex root() const { return root_; }
    void set_root(NodeIndex root) { root_ = root; }

    size_t size() const { return nodes_.size(); }
    size_t list_words() const { return lists_.size(); }
    const std::vector<Token> &tokens() const { return tokens_; }

private:
    // Index 0 is a None node so that kNoNode is safe to dereference, and
    // lists_[0] is the empty list's count. Both are the same trick words.def
    // plays with node 0, and for the same reason: a sentinel that cannot be
    // read is a branch at every call site.
    std::vector<Node> nodes_{Node{}};
    std::vector<NodeIndex> lists_{0};
    std::vector<Token> tokens_;
    NodeIndex root_ = kNoNode;
};

const char *kind_name(NodeKind kind);

// How tightly a binary operator binds, or 0 if it is not one.
//
// THE OPERATOR SET LIVES HERE AND NOT IN THE PARSER, because it has two
// readers and FORMAT/CXX.md §1 has one rule about that. The parser reads it to
// climb precedence; the unparser reads it to decide whether a child needs
// brackets printed back, since no node records that a parenthesis was written.
// A second copy in the printer is a copy that can disagree, and the way it
// would show up is a program that changes meaning when it is round-tripped.
//
// DESIGN §6.6 IS THE SPECIFICATION AND THIS IS THE ENFORCEMENT. §6 wrote the
// whole expression rule as `... precedence climbing ...` and named no operators,
// so M4 had to choose the levels to write a parser at all; §6.6 was added on
// 2026-08-30 and now states them, marked as decided there. Two places, and that
// is the arrangement DESIGN §5.5 already has for the two-character operators:
// the document says what the language is and the table is what a build can
// check. tests/parser_test/expressions.cpp is what keeps them equal.
int precedence_of(std::string_view op);

} // namespace satellite
