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

    // `satellite.include()` IS A FORM AND NOT AN OMISSION, and it is the form
    // WORD_NUMBERS §1.3 teaches the whole numbering with: `1 1 0`, where "0
    // means nothing in that position -- a real number in the sequence and not
    // a piece of notation, which is why `include()` and `include(satellite)`
    // are two different sequences rather than one path called two ways." §1.3
    // then states the rule this parse answers: "a trailing 0 is written only
    // where a program can actually write the bare form."
    //
    // UNTIL M17 NOTHING HERE COULD, AND EVERY OTHER LAYER WAS ALREADY BUILT
    // FOR IT -- which is what makes this one line rather than a feature.
    // words.def carries INCLUDE_0 at `1 1 0`; shape_of() matches it at arity
    // 0; statement_form() already reads `n.a == kNoNode` as argc 0;
    // satellite_cache/write.cpp's form() already prints a zero-arity row with
    // no parentheses; and unnumber.cpp names THIS form as the reason a `0`
    // segment may not be skipped -- "so a reader that skipped zero, or treated
    // it as a terminator, would refuse a form the writer emits." The writer
    // could not emit one, because the parser answered S0231 and asked for an
    // expression. DESIGN §6's `include_decl` said `"(" expression ")"` and
    // WORD_NUMBERS is the authority over the numbering, so the grammar is what
    // moved: `"(" [ expression ] ")"`.
    NodeIndex what = kNoNode;
    const bool nothing_there = at_punct(")");
    if (!nothing_there)
        what = expression();
    close_bracket();
    if (!nothing_there && what == kNoNode)
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
// A spacesuit's body, and every section inside it -- ONE LOOP OVER `open`,
// because section() called this function back and a section may hold a section.
// M8.5, DESIGN §7.5: the fourth of this parser's four cycles, and the smallest.
//
// EVERY LEVEL USES THE SAME TWO SENTENCES -- "to open the spacesuit" and "to
// close the spacesuit" -- which is what the recursive version said for a
// section's braces too, because a section's body WAS a suit_body. The wording is
// left exactly as it was rather than improved here: a rewrite that changes what
// a program is told is two changes wearing one commit.
ListId Parser::suit_body(words::PathId owner)
{
    struct Open {
        uint32_t opener = 0;
        uint32_t access = 0;   // the section's word, unread at the outermost
        std::vector<NodeIndex> items;
    };
    std::vector<Open> open;

    // THE BRACE MAY BE ON ITS OWN LINE, exactly as it may for a block, and
    // example/class_test.satl writes it that way. block() has the argument for
    // why crossing this newline cannot swallow anything.
    skip_newlines();
    {
        const uint32_t opener = here();
        if (!expect_punct("{", "to open the spacesuit"))
            return kNoList;
        open.push_back({opener, 0, {}});
    }

    for (;;) {
        skip_newlines();

        if (!at_end() && !at_punct("}") && !stop()) {
            if (const Segment1 word = opening();
                word == Segment1::Protected || word == Segment1::Public) {
                advance();  // satellite
                advance();  // .
                const uint32_t access = here();
                advance();  // protected | public

                // THE SECTION'S OWN BRACE, opened here rather than by a second
                // call. A section that will not open is the same failure the
                // recursive version reported from inside suit_body().
                skip_newlines();
                const uint32_t opener = here();
                if (!expect_punct("{", "to open the spacesuit"))
                    return kNoList;
                open.push_back({opener, access, {}});
                continue;
            }

            // A SECTION NEEDS NO "did this rule consume anything" GUARD and a
            // member does, which is why the check is here rather than around
            // both: a section has already taken three tokens by the time it can
            // fail, so the case the guard exists for cannot arise for one.
            const size_t before = pos_;
            const NodeIndex item = suit_member(owner);
            if (item != kNoNode)
                open.back().items.push_back(item);
            if (pos_ == before) {
                error<errors::Code::PARSE_EXPECTED_SUIT_ITEM>(here(),
                                                              describe(peek()));
                advance();
            }
            if (panic_)
                synchronise();
            continue;
        }

        const bool closed =
            expect_punct("}", "to close the spacesuit", open.back().opener);
        const Open done = std::move(open.back());
        open.pop_back();

        if (!closed) {
            if (open.empty())
                return kNoList;
            // The section did not close, so it contributes no node -- which is
            // section() coming back kNoNode after `if (panic_)`, and the body
            // around it carrying on the way its loop always did.
            if (panic_)
                synchronise();
            continue;
        }

        const ListId items = ast_.add_list(done.items);
        if (open.empty())
            return items;

        // WHICH ACCESS IT IS, IS THE TOKEN'S SPELLING AND NOT A FIELD. Both
        // words are in words.def, so the token already carries an integer that
        // answers it, and a second copy in the node is a second thing to keep in
        // step.
        open.back().items.push_back(ast_.add(NodeKind::Section, done.access, items));
        if (panic_)
            synchronise();
    }
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
