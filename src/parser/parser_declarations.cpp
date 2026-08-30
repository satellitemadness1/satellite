// The four top-level forms, the two that go inside a suit block, and the one
// call in this milestone that gives a name a number.
//
// THIS IS THE FIRST CALLER OF words::Words::intern, WHICH M2 BUILT AND NOTHING
// CALLED. PLAN §8.1 is the specification: every node keeps a live count of its
// children, so a user's capsules and spacesuits take the next number free under
// the node that owns them, allocated WHEN THE NAME IS FIRST MET -- and the
// parser is what meets a name for the first time. WORD_NUMBERS §3 gives the
// worked example: `satellite.library.main` is 1 14 1 because the language put
// it there, so a user's first name under `library` is 1 14 3, since main and
// system are taken.
//
// AND PLAN M4 SETS THE POLICY FOR THE COLLISION: a name the language already
// owns under that parent is REFUSED rather than renumbered. DESIGN §2's
// reservation rule decides at M7 whether refusing is the right answer; what
// this milestone must not do is quietly hand out a second number for a word
// that already has one, which is what `intern()` alone would do.

#include "parser/parser_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "lexical_analyzer/lexer.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace satellite {

NodeIndex Parser::top_level()
{
    switch (opening()) {
    case Segment1::Include:   return include_decl();
    case Segment1::Capsule:   return capsule_decl(static_cast<words::PathId>(
                                  words::NodeId::LIBRARY));
    case Segment1::Spacesuit: return spacesuit_decl();
    case Segment1::Library:   return global_decl();
    default: break;
    }

    // DESIGN §6: `top_level := include_decl | capsule_decl | spacesuit_decl |
    // global_decl`, and nothing else. A statement at the top of a file is not
    // an unfinished feature, it is a program with no capsule to run -- so the
    // sentence names the four forms rather than describing the token that was
    // found.
    error<errors::Code::PARSE_EXPECTED_DECLARATION>(here(), describe(peek()));
    // `satellite.capsul` IS THE CASE THIS EXISTS FOR, and the guard is what
    // keeps it from being noise: a suggestion is only offered when what was
    // written IS a `satellite.` path, because DESIGN §4.6's edit distance is
    // over one node's children and a bare word is not a segment under
    // `satellite` at all.
    if (is_reserved_word(peek()) && at_punct(".", 1) && at_word(2))
        suggest(here() + 2, static_cast<words::PathId>(words::NodeId::SATELLITE));
    return kNoNode;
}

NodeIndex Parser::include_decl()
{
    const uint32_t at = here();
    advance();  // satellite
    advance();  // .
    advance();  // include

    const uint32_t opener = here();
    if (!expect_punct("(", "after satellite.include"))
        return kNoNode;
    open_bracket();
    const NodeIndex what = expression();
    close_bracket();
    if (what == kNoNode)
        return kNoNode;
    if (!expect_punct(")", "to close satellite.include", opener))
        return kNoNode;
    return ast_.add(NodeKind::Include, at, what);
}

NodeIndex Parser::capsule_decl(words::PathId owner)
{
    advance();  // satellite
    advance();  // .
    advance();  // capsule

    // `capsule_name := IDENT | "satellite" "." IDENT`, and the second arm is
    // RESERVED: it names a capsule the language already has a number for, which
    // today is `satellite.main` and nothing else. So the two arms do opposite
    // things with the same table -- one looks a name up and must find it, the
    // other defines a name and must not.
    uint32_t name = 0;
    words::PathId path = words::kNoPath;
    if (is_reserved_word(peek()) && at_punct(".", 1) && at_word(2)) {
        advance();
        advance();
        name = here();
        path = words_.find(words::NodeId::SATELLITE, peek().text);
        if (path == words::kNoPath || !words::is_language_word(path)) {
            error<errors::Code::PARSE_NOT_A_LANGUAGE_CAPSULE>(name, peek().text);
            suggest(name, static_cast<words::PathId>(words::NodeId::SATELLITE));
            return kNoNode;
        }
        advance();
    } else {
        name = expect_word("a name for the capsule");
        if (panic_)
            return kNoNode;
        path = define_name(owner, name, "capsule");
        if (panic_)
            return kNoNode;
    }

    const ListId params = param_list();
    if (panic_)
        return kNoNode;

    NodeIndex returns = kNoNode;
    if (opening() == Segment1::Returns) {
        returns = returns_clause();
        if (returns == kNoNode)
            return kNoNode;
    }

    const NodeIndex body = block();
    if (body == kNoNode)
        return kNoNode;
    return ast_.add(NodeKind::Capsule, name, path, params, returns, body);
}

NodeIndex Parser::spacesuit_decl()
{
    advance();  // satellite
    advance();  // .
    advance();  // spacesuit

    const uint32_t name = expect_word("a name for the spacesuit");
    if (panic_)
        return kNoNode;
    const words::PathId path =
        define_name(static_cast<words::PathId>(words::NodeId::LIBRARY), name,
                    "spacesuit");
    if (panic_)
        return kNoNode;

    // `[ "(" IDENT ")" ]` -- the superclass, and the parentheses are all or
    // nothing. An empty pair is not the grammar's form for "no superclass";
    // leaving them out is.
    NodeIndex super = kNoNode;
    if (at_punct("(")) {
        const uint32_t opener = here();
        advance();
        const uint32_t super_name = expect_word("the name of the spacesuit this "
                                                "one extends");
        if (panic_)
            return kNoNode;
        super = ast_.add(NodeKind::Name, super_name);
        if (!expect_punct(")", "to close the superclass", opener))
            return kNoNode;
    }

    const ListId items = suit_body(path);
    if (panic_)
        return kNoNode;
    return ast_.add(NodeKind::Spacesuit, name, path, items, super);
}

// The body of a spacesuit, and the body of a section inside one -- ONE LOOP,
// BECAUSE DESIGN §6's SPLIT OF THE TWO CANNOT PARSE ITS OWN EXAMPLE. The
// grammar writes
//
//     spacesuit_decl := ... suit_block
//     suit_block     := "{" { suit_section | member } "}"
//     suit_section   := "satellite" "." ( "protected" | "public" ) block
//     block          := "{" { statement } "}"
//
// so a section's body was a block, a block holds statements, and a capsule
// declaration is not a statement -- while every section in
// example/class_test.satl holds capsule declarations and nothing else. Read as
// written, the rule rejected the program it was written for.
//
// DESIGN §6 WAS CORRECTED ON 2026-08-30 and `suit_section` now cites
// `suit_block`, with the old line kept visible beside it. What a section holds
// is what a suit block holds, which is why this is one function.
ListId Parser::suit_body(words::PathId owner)
{
    // THE BRACE MAY BE ON ITS OWN LINE, exactly as it may for a block, and
    // example/class_test.satl writes it that way. block() has the argument for
    // why crossing this newline cannot swallow anything.
    skip_newlines();
    const uint32_t opener = here();
    if (!expect_punct("{", "to open the spacesuit"))
        return kNoList;

    std::vector<NodeIndex> items;
    skip_newlines();
    while (!at_end() && !at_punct("}") && !stop()) {
        const size_t before = pos_;
        const Segment1 word = opening();
        const NodeIndex item =
            (word == Segment1::Protected || word == Segment1::Public)
                ? section(owner)
                : suit_member(owner);
        if (item != kNoNode)
            items.push_back(item);
        if (pos_ == before) {
            error<errors::Code::PARSE_EXPECTED_SUIT_ITEM>(here(), describe(peek()));
            advance();
        }
        if (panic_)
            synchronise();
        skip_newlines();
    }

    if (!expect_punct("}", "to close the spacesuit", opener))
        return kNoList;
    return ast_.add_list(items);
}

NodeIndex Parser::section(words::PathId owner)
{
    advance();  // satellite
    advance();  // .
    const uint32_t access = here();
    advance();  // protected | public

    const ListId body = suit_body(owner);
    if (panic_)
        return kNoNode;

    // WHICH ACCESS IT IS, IS THE TOKEN'S SPELLING AND NOT A FIELD. Both words
    // are in words.def, so the token already carries an integer that answers
    // it, and a second copy in the node is a second thing to keep in step.
    return ast_.add(NodeKind::Section, access, body);
}

NodeIndex Parser::suit_member(words::PathId owner)
{
    const Segment1 word = opening();
    if (word == Segment1::Capsule)
        return capsule_decl(owner);

    if (word == Segment1::Variable || word == Segment1::Container ||
        at_declaration()) {
        const NodeIndex declared = type();
        if (declared == kNoNode)
            return kNoNode;
        return var_decl(declared);
    }

    error<errors::Code::PARSE_EXPECTED_FIELD_OR_CAPSULE>(here(), describe(peek()));
    return kNoNode;
}

NodeIndex Parser::global_decl()
{
    advance();  // satellite
    advance();  // .
    advance();  // library

    if (!expect_punct(".", "after satellite.library"))
        return kNoNode;
    const uint32_t name = expect_word("a name for the global");
    if (panic_)
        return kNoNode;

    const words::PathId path = define_name(
        static_cast<words::PathId>(words::NodeId::LIBRARY), name, "global");
    if (panic_)
        return kNoNode;

    NodeIndex init = kNoNode;
    if (take_punct("=")) {
        init = expression();
        if (init == kNoNode)
            return kNoNode;
    }
    return ast_.add(NodeKind::Global, name, path, init);
}

words::PathId Parser::define_name(words::PathId owner, uint32_t token,
                                  const char *what)
{
    const std::string_view name = toks()[token].text;

    // A USER'S SPACESUIT CANNOT YET BE A PARENT, AND M4 IS WHERE THAT WAS
    // FOUND -- by being M2's first caller, which is the whole reason PLAN asks
    // for a consumer in the milestone that writes a thing. words::Words seeds
    // one counter per node of the FROZEN table and walks the frozen child
    // lists, both sized kNodeCount + 1; a user's PathId starts above that, so
    // handing one to find() or define() indexes past the end of both arrays.
    // So a capsule declared inside a spacesuit gets no number here, its node
    // carries kNoPath, and tests/parser_test/declarations.cpp asserts that
    // rather than leaving it to be discovered. MILESTONES/M4.md §6 carries what
    // fixing it costs.
    if (!words::is_language_word(owner))
        return words::kNoPath;
    const words::NodeId parent = static_cast<words::NodeId>(owner);

    if (const words::PathId taken = words_.find(parent, name);
        taken != words::kNoPath) {
        if (words::is_language_word(taken)) {
            error<errors::Code::PARSE_NAME_IS_LANGUAGE_OWNED>(
                token, name, words::path_text(parent), what);
        } else {
            error<errors::Code::PARSE_NAME_ALREADY_DEFINED>(token, name);
            // THE NOTE THAT NEEDED A TABLE, and it is the reason `declared_at_`
            // exists. "declared twice" is the one diagnostic in this parser
            // where the useful second place is not a bracket a few tokens back
            // but a line somewhere else in the file, which is exactly what
            // DESIGN §9 means by "notes carrying their own spans".
            for (const auto &[id, at] : declared_at_)
                if (id == taken) {
                    attach(errors::note<errors::Code::NOTE_FIRST_DECLARED_HERE>(
                        span_of(at), name));
                    break;
                }
        }
        return words::kNoPath;
    }

    const words::PathId defined = words_.define(parent, name);
    if (defined == words::kNoPath) {
        error<errors::Code::PARSE_NAME_UNNUMBERABLE>(token, name);
        return words::kNoPath;
    }
    declared_at_.emplace_back(defined, token);
    return defined;
}

} // namespace satellite
