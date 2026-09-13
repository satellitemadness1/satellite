// DESIGN §6's expression, and SATC.md §3.1's one real decision. See
// satellite_cache/write_internal.hpp.
//
// §3.1 IS THE WHOLE OF postfix() AND IT IS THE SENTENCE THE FORMAT RESTS ON. A
// PATH is rooted at the reserved word and resolves with no context, so
// substituting its number is sound anywhere; a SELECTOR is a bare word after a
// receiver and its number is reachable only through the receiver's TYPE, which
// nothing has decided at M4.5. So the chain is offered to the matcher, and what
// the matcher declines is printed exactly as the program wrote it.
//
// THE BRACKETS COME BACK FROM PRECEDENCE AND NOT FROM MEMORY, which is
// unparse.cpp's rule and is inherited rather than rediscovered: no node records
// that a parenthesis was written, a child needs brackets when it binds looser
// than its parent, and the right-hand child needs them when it binds equally
// too. `a - (b - c)` is the one case that silently changes an answer, and a
// `.satc` that got it wrong would be a cache that changes arithmetic.

#include "satellite_cache/write_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_cache/paths.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>

namespace satellite::cache {

void Writer::expand_expression(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Number:
    case NodeKind::Bits:
    case NodeKind::Name:
        say(text(node));
        return;
    case NodeKind::String:
        // SATC.md §3: literals stay literal. Numbering one "would buy
        // nothing and cost the readability §1.1 exists for", and the body
        // is printed from the token's `text` for the reason unparse.cpp
        // gives at length -- expansion is not reversible.
        say("\"" + text(node) + "\"");
        return;
    case NodeKind::Satellite:
        say("satellite");
        return;
    case NodeKind::Member:
    case NodeKind::Call:
    case NodeKind::Index:
    case NodeKind::Slice:
        postfix(node);
        return;
    case NodeKind::Unary:
        say(text(node));
        bracketed(n.a, kBindsTighterThanAny, false);
        return;
    case NodeKind::Binary: {
        const int level = precedence_of(text(node));
        bracketed(n.a, level, false);
        say(" " + text(node) + " ");
        bracketed(n.b, level, true);
        return;
    }
    case NodeKind::Type:
        type_of(node);
        return;
    default:
        say("<" + std::string(kind_name(n.kind)) + " is not an expression>");
        return;
    }
}

// The postfix chain -- SATC.md §3.1, and the one place the format's whole
// rule is applied. A chain rooted at the reserved word that the numbering
// accounts for becomes a number; anything else is printed as written, which
// covers a user's capsule call, a selector after a receiver, and a path in
// a shape the language does not have.
void Writer::postfix(NodeIndex node)
{
    const Node &n = ast_[node];
    if (const PathMatch match = language_path(ast_, node); match.found()) {
        chain(match, n.kind == NodeKind::Call, n.b,
              n.kind == NodeKind::Call ? n.c : kNoList);
        return;
    }

    switch (n.kind) {
    case NodeKind::Member:
        expr(n.a);
        say("." + text(node));
        return;
    case NodeKind::Call:
        // §3.1 STILL DECLINES THE SELECTOR AND M19.6 CHANGED ONLY ITS ARGUMENT.
        // `sort` is printed as the program wrote it -- the receiver's type is
        // known now, but a selector's number is a property of the RUN for a
        // user's method and of the BUILD for the language's, and the file has
        // no way to say which it is holding. The option is not that: `down` is
        // a word the program wrote, and `0#down` records which one.
        expr(n.a);
        say("(");
        arguments(n.b, folds_.at(n.a));
        named_arguments(n.c, ast_.list_size(n.b) > 0);
        say(")");
        return;
    case NodeKind::Index:
        expr(n.a);
        say("[");
        expr(n.b);
        say("]");
        return;
    default:
        expr(n.a);
        say("[");
        if (n.b != kNoNode)
            expr(n.b);
        say(":");
        if (n.c != kNoNode)
            expr(n.c);
        say("]");
        return;
    }
}

// A type -- §6's `type` in all three forms. `satellite.container.list<...>`
// is 1.4.2<1.6.1>, a bare spacesuit name stays a name, and the generics are
// types again.
//
// words::walk() RATHER THAN paths.hpp's ARITY MATCHER, because a type has
// no call shape to slot into: the Type node carries the space's SPELLING
// and the name, so the path is already text and the trie's own walk is the
// function that reads text. It is also what resolves `hexadecimal` onto
// `hex`'s node, which is §5.1 step 1 arriving here for free.
void Writer::expand_type(NodeIndex node)
{
    if (node == kNoNode)
        return;
    const Node &n = ast_[node];
    if (n.a == words::kNoSpelling) {
        say(text(node));
        return;
    }

    fixed("satellite." +
          std::string(words::spelling_of(static_cast<words::NodeId>(n.a))) +
          "." + text(node));
    if (n.b == kNoList)
        return;
    say("<");
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            say(", ");
        type_of(ast_.list_at(n.b, i));
    }
    say(">");
}

// THE OPTION IS ALWAYS THE FIRST ARGUMENT AND NEVER ANY OTHER -- M19.6.
// `fold_option()` reads `ast_.list_at(n.b, 0)` and gives up unless that node is
// a String, so position 0 is the only one a fold can have come from and the
// flag says nothing about the rest. `my_list.sort("up", key)` folds on `up` and
// leaves `key` alone, which is why this takes a bool rather than rewriting
// every string in the list.
void Writer::arguments(ListId list, bool first_is_an_option)
{
    for (uint32_t i = 0; i < ast_.list_size(list); i++) {
        if (i > 0)
            say(", ");
        const NodeIndex argument = ast_.list_at(list, i);

        // PRINTED HERE RATHER THAN IN expand_expression(), because the node is
        // an ordinary String and only its POSITION under a folded selector
        // makes it an option. A String that knew it was one would be the tree
        // carrying a resolve-time fact, which ast.hpp forbids and PLAN §2.2
        // says why.
        if (i == 0 && first_is_an_option &&
            ast_[argument].kind == NodeKind::String) {
            say(std::string(kOptionMark) + text(argument));
            continue;
        }
        expr(argument);
    }
}

// `name=value`, after the positional arguments -- M30. The name is a word the
// program wrote and not a path, so it is printed as written; the value is an
// ordinary expression and numbers like any other.
void Writer::named_arguments(ListId list, bool after_positional)
{
    for (uint32_t i = 0; list != kNoList && i < ast_.list_size(list); i++) {
        if (i > 0 || after_positional)
            say(", ");
        const NodeIndex named = ast_.list_at(list, i);
        say(text(named) + "=");
        expr(ast_[named].a);
    }
}

void Writer::bracketed(NodeIndex node, int level, bool on_the_right)
{
    const Node &n = ast_[node];
    const int child =
        n.kind == NodeKind::Binary ? precedence_of(text(node)) : 0;
    const bool needs = n.kind == NodeKind::Binary &&
                       (child < level || (on_the_right && child == level));
    if (!needs) {
        expr(node);
        return;
    }
    say("(");
    expr(node);
    say(")");
}

} // namespace satellite::cache
