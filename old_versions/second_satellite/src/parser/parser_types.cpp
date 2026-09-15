// DESIGN §6's `type` rule, and the two places a type is written that are not a
// declaration: a parameter list and a returns clause.
//
// SPLIT FROM parser_declarations.cpp BY SUBJECT. A type is the one production
// in §6 that both halves of the grammar reach -- `var_decl` in a block and
// `param` in a signature -- so it is neither a declaration's private business
// nor a statement's.
//
// THE THREE FORMS ARE ONE NODE AND TWO OF THEM LOOK ALIKE. §6 writes
//
//     type := "satellite" "." type_space "." IDENT [ "<" type { "," type } ">" ]
//           | "satellite"
//           | IDENT
//
// and the second and third both come out as a Type node with no type space --
// which is not a loss of information, because `is_reserved_word` on the node's
// own token tells them apart in one integer compare. Storing a third field to
// say which would be storing what the token already says.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite {

// A type, and every type written inside its `<...>`.
//
// THE `<...>` NESTING IS A VECTOR AND NOT THE C++ STACK -- M8.5, DESIGN §7.5.
// generic_arguments() used to call this function back, so `list<list<list<...>>>`
// was a depth the program chose. `open` below is one entry per argument list
// still waiting for its `>`, and the loop closes as many of them as the type it
// just read finished.
NodeIndex Parser::type()
{
    struct Open {
        uint32_t name = 0;
        words::SpellingId space = words::kNoSpelling;
        std::vector<NodeIndex> arguments;
    };
    std::vector<Open> open;

    for (;;) {
        NodeIndex node = kNoNode;

        if (is_reserved_word(peek()) && at_punct(".", 1) && at_word(2)) {
            const Segment1 space = segment1_of(peek(2).spelling);
            if (space != Segment1::Variable && space != Segment1::Container) {
                // A `satellite.` path whose segment 1 is not a type space is not
                // a type. Saying so here rather than letting the bare arm below
                // take the `satellite` and leave the rest is what makes the
                // error point at the word that was wrong.
                error<errors::Code::PARSE_NOT_A_TYPE>(here() + 2, peek(2).text);
                // OVER `satellite`'s CHILDREN AND NOT OVER {variable,
                // container}, which is a smaller candidate list and would be the
                // wrong one. Somebody who wrote `satellite.varable` wants
                // `variable`; somebody who wrote `satellite.console` in type
                // position wants to be told it is not a type, and offering them
                // `container` for it would be worse than offering nothing. The
                // trie level that failed is segment 1, so that is the level the
                // suggestion comes from.
                suggest(here() + 2,
                        static_cast<words::PathId>(words::NodeId::SATELLITE));
                return kNoNode;
            }

            const words::SpellingId space_id = peek(2).spelling;
            advance();  // satellite
            advance();  // .
            advance();  // variable | container
            if (!expect_punct(".", "after the type space"))
                return kNoNode;
            const uint32_t name = expect_word("the name of a type");
            if (panic_)
                return kNoNode;

            if (at_punct("<")) {
                advance();  // '<'
                open.push_back({name, space_id, {}});
                continue;   // the first argument is a type, read the same way
            }
            node = ast_.add(NodeKind::Type, name, space_id, kNoList);
        } else if (at_word()) {
            // `satellite` alone -- the singleton runtime type -- or a spacesuit
            // named bare (DESIGN §13). One node either way.
            //
            // OR `ship.box`, A SPACESUIT ANOTHER FILE DECLARES -- M25. The node
            // is anchored on `box` exactly as a bare `box` is, and ast.hpp's
            // qualifier_of() reads `ship` back two tokens behind it, so a
            // qualified type costs no payload word and no new kind.
            if (!is_reserved_word(peek()) && at_punct(".", 1) && at_word(2)) {
                advance();  // the spaceship
                advance();  // .
            }
            const uint32_t at = here();
            advance();
            node = ast_.add(NodeKind::Type, at, words::kNoSpelling, kNoList);
        } else {
            error<errors::Code::PARSE_EXPECTED_TYPE>(here(), describe(peek()));
            return kNoNode;
        }

        // What that type finished: nothing, one argument list, or several at
        // once -- `map<string, list<list<number>>>` closes two on its last `>`.
        for (;;) {
            if (open.empty())
                return node;
            open.back().arguments.push_back(node);
            if (take_punct(","))
                break;

            // `list<list<string>>` CLOSES AS TWO INDEPENDENT '>' TOKENS, which
            // is what DESIGN §5.5 buys by refusing `<<` and `>>` permanently:
            // there is no maximal munch to undo, so nested generics need no
            // special case and this loop needs no lookahead.
            //
            // THE ONE COLLISION LEFT IS `>=`, AND M4 CONFIRMS IT IS UNREACHABLE.
            // MILESTONES/M3.md §6 left `split_punct` uncalled and asked M4 to
            // say whether it stays that way. It does: a complete type is only
            // ever followed by IDENT, `)`, `,` or `>` in §6's grammar, and none
            // of those can begin with `=`. There is a second reason not to reach
            // for it even if that changes -- split_punct INSERTS into the token
            // vector, and this parser stores token INDICES in every node it has
            // already built, so a split partway through a parse renumbers the
            // anchors of the whole tree behind it. If the grammar ever makes
            // `>=` reachable here, the fix belongs at lex time or in a separate
            // record, not in a mid-parse mutation.
            if (at_punct(">=")) {
                error<errors::Code::PARSE_GENERIC_CLOSE_GE>(here());
                return kNoNode;
            }
            expect_punct(">", "to close the type's arguments");
            if (panic_)
                return kNoNode;

            const Open done = std::move(open.back());
            open.pop_back();
            node = ast_.add(NodeKind::Type, done.name, done.space,
                            ast_.add_list(done.arguments));
        }
    }
}

ListId Parser::param_list()
{
    if (!expect_punct("(", "to open the parameter list"))
        return kNoList;
    open_bracket();

    std::vector<NodeIndex> params;
    if (!at_punct(")")) {
        do {
            const NodeIndex declared = type();
            if (declared == kNoNode) {
                close_bracket();
                return kNoList;
            }
            const uint32_t name = expect_word("a name for the parameter");
            if (panic_) {
                close_bracket();
                return kNoList;
            }
            // A PARAMETER IS A VarDecl WITH NO INITIALISER, and reusing the
            // kind is DESIGN §7.1's sentence in the tree: "parameters are
            // locals too" is the reason its one-slot-per-local registry could
            // not be reframed as deliberate, because `arguments` would have
            // been a program-wide static. A separate Param kind would let a
            // later pass forget that.
            params.push_back(ast_.add(NodeKind::VarDecl, name, declared, kNoNode));
        } while (take_punct(","));
    }

    close_bracket();
    expect_punct(")", "to close the parameter list");
    return ast_.add_list(params);
}

NodeIndex Parser::returns_clause()
{
    advance();  // satellite
    advance();  // .
    advance();  // returns

    if (!expect_punct("(", "after satellite.returns"))
        return kNoNode;
    open_bracket();
    const NodeIndex declared = type();
    close_bracket();
    if (declared == kNoNode)
        return kNoNode;
    if (!expect_punct(")", "to close satellite.returns"))
        return kNoNode;
    return declared;
}

} // namespace satellite
