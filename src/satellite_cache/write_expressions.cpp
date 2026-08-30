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

std::string Writer::expression(NodeIndex node)
{
    const Node &n = ast_[node];
    switch (n.kind) {
    case NodeKind::Number:
    case NodeKind::Bits:
    case NodeKind::Name:
        return text(node);
    case NodeKind::String:
        // SATC.md §3: literals stay literal. Numbering one "would buy
        // nothing and cost the readability §1.1 exists for", and the body
        // is printed from the token's `text` for the reason unparse.cpp
        // gives at length -- expansion is not reversible.
        return "\"" + text(node) + "\"";
    case NodeKind::Satellite:
        return "satellite";
    case NodeKind::Member:
    case NodeKind::Call:
    case NodeKind::Index:
    case NodeKind::Slice:
        return postfix(node);
    case NodeKind::Unary:
        return text(node) + bracketed(n.a, kBindsTighterThanAny, false);
    case NodeKind::Binary: {
        const int level = precedence_of(text(node));
        const std::string left = bracketed(n.a, level, false);
        const std::string right = bracketed(n.b, level, true);
        return left + " " + text(node) + " " + right;
    }
    case NodeKind::Type:
        return type(node);
    default:
        return "<" + std::string(kind_name(n.kind)) + " is not an expression>";
    }
}

// The postfix chain -- SATC.md §3.1, and the one place the format's whole
// rule is applied. A chain rooted at the reserved word that the numbering
// accounts for becomes a number; anything else is printed as written, which
// covers a user's capsule call, a selector after a receiver, and a path in
// a shape the language does not have.
std::string Writer::postfix(NodeIndex node)
{
    const Node &n = ast_[node];
    if (const PathMatch match = language_path(ast_, node); match.found())
        return numbered_chain(match, n.kind == NodeKind::Call, n.b);

    switch (n.kind) {
    case NodeKind::Member: {
        const std::string receiver = expression(n.a);
        return receiver + "." + text(node);
    }
    case NodeKind::Call: {
        const std::string target = expression(n.a);
        return target + "(" + arguments(n.b) + ")";
    }
    case NodeKind::Index: {
        const std::string target = expression(n.a);
        const std::string subscript = expression(n.b);
        return target + "[" + subscript + "]";
    }
    default: {
        const std::string target = expression(n.a);
        const std::string low = n.b != kNoNode ? expression(n.b) : std::string();
        const std::string high = n.c != kNoNode ? expression(n.c) : std::string();
        return target + "[" + low + ":" + high + "]";
    }
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
std::string Writer::type(NodeIndex node)
{
    if (node == kNoNode)
        return std::string();
    const Node &n = ast_[node];
    if (n.a == words::kNoSpelling)
        return text(node);

    std::string out = fixed("satellite." +
                            std::string(words::spelling_of(
                                static_cast<words::NodeId>(n.a))) +
                            "." + text(node));
    if (n.b == kNoList)
        return out;
    out += "<";
    for (uint32_t i = 0; i < ast_.list_size(n.b); i++) {
        if (i > 0)
            out += ", ";
        out += type(ast_.list_at(n.b, i));
    }
    return out + ">";
}

std::string Writer::arguments(ListId list)
{
    std::string out;
    for (uint32_t i = 0; i < ast_.list_size(list); i++) {
        if (i > 0)
            out += ", ";
        out += expression(ast_.list_at(list, i));
    }
    return out;
}

std::string Writer::bracketed(NodeIndex node, int level, bool on_the_right)
{
    const Node &n = ast_[node];
    const int child =
        n.kind == NodeKind::Binary ? precedence_of(text(node)) : 0;
    const bool needs = n.kind == NodeKind::Binary &&
                       (child < level || (on_the_right && child == level));
    const std::string inner = expression(node);
    return needs ? "(" + inner + ")" : inner;
}

} // namespace satellite::cache
