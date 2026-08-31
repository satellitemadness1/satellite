// What a name is -- a slot, a capsule, a spacesuit, or nothing at all. DESIGN
// §7.2 is the storage and §7.7 is the one library global the language provides.
//
// THE ORDER OF THE LOOKUPS IS THE LANGUAGE'S SHADOWING RULE and is the only
// place it is written down. In scope first, then the capsules this file
// declares, then the spacesuits -- so a local called `fact` shadows a capsule
// called `fact` for the rest of its scope, which is what somebody who wrote
// both meant. It is the opposite order from words_runtime.hpp's find(), and
// deliberately: there the LANGUAGE's children are searched first, because a
// user's name may never answer in place of a word the language owns. Names the
// user chose shadow each other; nothing shadows the language.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "error_reporter/suggest.hpp"
#include "satellite_cache/paths.hpp"

#include <cstdint>
#include <string>
#include <string_view>

namespace satellite::resolve {

void Resolver::name(NodeIndex node)
{
    const std::string_view spelling = ast_.text_of(node);

    if (const Binding *bound = lookup(spelling)) {
        info(node).slot = bound->slot;
        info(node).type = bound->type;
        if (bound->arguments) {
            info(node).arguments = true;
            info(node).origin = Origin::Bound;
            info(node).path =
                static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS);
        }
        return;
    }

    // DESIGN §7.6: capsules are not in the registry and live in their own
    // table. A bare capsule name cannot even form a legal registry key -- v1
    // verified it -- so this is a lookup and never a fallback.
    if (const Capsule *found = capsule_named(spelling)) {
        info(node).slot = kSlotCapsule;
        info(node).path = found->path;
        info(node).origin = Origin::Bound;
        return;
    }

    if (const Capsule *found = suit_named(spelling)) {
        info(node).slot = kSlotSpacesuit;
        info(node).path = found->path;
        info(node).origin = Origin::Bound;
        return;
    }

    // NO "IT MIGHT BE A GLOBAL" ARM, AND THE M6 DRAFT HAS ONE. Its resolver
    // answers SLOT_GLOBAL for any unknown name met outside a capsule, which is
    // right for the grammar it was written against and wrong for this one:
    // DESIGN §6's `top_level` is include, capsule, spacesuit and global, so the
    // only expressions outside a body are a global's initialiser and an
    // include's argument -- and an unknown name in either of those is unknown.
    // The parser's S0204 says the same thing in the words a user reads.
    problem<errors::Code::RESOLVE_NO_SUCH_NAME>(node, spelling);
    if (const std::string_view near = nearest_in_scope(spelling); !near.empty())
        suggest(near);
}

void Resolver::member(NodeIndex node)
{
    const Node &n = ast_[node];

    // THE CHAIN FIRST, AND IT IS ASKED FROM THE OUTSIDE IN. That is the
    // direction satellite_cache/paths.cpp already walks and it is what makes
    // `satellite.time.now().some_function()` fall out with no rule of its own:
    // the outer chain does not match, so the receiver is resolved and the
    // member is a selector on whatever it came back as.
    const cache::PathMatch found = path_of(node, false);
    if (found.found()) {
        info(node).path = found.id;
        info(node).origin = took_ ? Origin::Cached : Origin::Walked;
        info(node).type = found.id;
        return;
    }

    // ROOTED AT `satellite` AND STOPPED PARTWAY, which is the error
    // satellite_cache/paths.cpp names and declines to raise: "a misspelled word
    // under `satellite` is a program M7 refuses with M5's did-you-mean over the
    // node the segment failed under; what a cache owes it is the program
    // UNCHANGED." This is that refusal, and it does NOT go on to resolve the
    // receiver -- every segment inside a broken path would fail the same way
    // and one bad word would print four carets.
    if (found.under != words::kNoPath) {
        no_such_word(found);
        return;
    }

    expression(n.a);
    const Info receiver = out_.at(n.a);
    const std::string_view word = ast_.text_of(node);

    if (receiver.arguments) {
        arguments_member(node, word);
        return;
    }

    // ONE HOP, AND THE BOUNDARY IS WORTH STATING. WORD_NUMBERS §1.5 says a
    // selector's number is reachable only THROUGH THE RECEIVER'S TYPE, and the
    // one receiver whose type this milestone knows is a name whose declaration
    // it just read. `my_list.size` is `1 4 2 2`; `f().size` is not, because
    // what `f()` returns is DESIGN §8's value model and that is M9's. A
    // milestone that inferred the second would be building M9's type rules to
    // finish M7's clause.
    if (receiver.type != words::kNoPath) {
        out_.walked++;
        const words::PathId selector =
            child_named(static_cast<words::NodeId>(receiver.type), word);
        if (selector != words::kNoPath) {
            info(node).path = selector;
            info(node).origin = Origin::Walked;
            info(node).type = selector;
        }
    }
}

void Resolver::call(NodeIndex node)
{
    const Node &n = ast_[node];

    // THREE QUESTIONS IN ONE ORDER, AND IT IS THE WRITER'S ORDER. `satl --satc`
    // asks the same three of the same tree one milestone earlier, so the number
    // in a `.satc` and the number resolve arrives at are the same number by
    // construction rather than by agreement.
    //
    // ONE: is the whole call a row of the numbering? `satellite.console.input()`
    // is 1 5 2 and the parentheses are part of what that number says.
    const cache::PathMatch whole = path_of(node, true);
    if (whole.found()) {
        info(node).path = whole.id;
        info(node).origin = took_ ? Origin::Cached : Origin::Walked;
        // AN ABSORBED ARGUMENT IS PART OF THE NUMBER AND NOT AN EXPRESSION.
        // SATC §5.1 step 3: `include(satellite)` is 1 1 1 and the reserved word
        // is what the number NAMES, so there is nothing left to resolve.
        if (!whole.absorbs_argument)
            for (uint32_t i = 0; i < ast_.list_size(n.b); i++)
                expression(ast_.list_at(n.b, i));
        return;
    }

    if (whole.under != words::kNoPath) {
        no_such_word(whole);
        return;
    }

    // TWO: is the TARGET a row, with the call still to make?
    // `satellite.console.display("x")` is 1 5 1 and the argument is the
    // program's own -- paths.hpp's own example of the distinction, and the
    // reason the two questions are asked separately rather than once.
    expression(n.a);
    if (out_.at(n.a).path != words::kNoPath) {
        for (uint32_t i = 0; i < ast_.list_size(n.b); i++)
            expression(ast_.list_at(n.b, i));
        return;
    }

    // THREE: a SELECTOR call, which is the one WORD_NUMBERS §1.5 says a `.satc`
    // can never carry -- "reaching its number needs the receiver's TYPE, which
    // is not known until resolve runs". It is known here for one receiver and
    // one only; names.cpp's member() states that boundary and why it is not
    // widened.
    if (n.a != kNoNode && ast_[n.a].kind == NodeKind::Member) {
        const words::PathId under = out_.at(ast_[n.a].a).type;
        if (under != words::kNoPath && !fold_option(node, n.a, under)) {
            out_.walked++;
            const cache::PathMatch shape = cache::shape_path(
                static_cast<words::NodeId>(under), ast_.text_of(n.a),
                static_cast<int>(ast_.list_size(n.b)), false);
            if (shape.found()) {
                info(n.a).path = shape.id;
                info(n.a).origin = Origin::Walked;
            }
        }
    }

    for (uint32_t i = 0; i < ast_.list_size(n.b); i++)
        expression(ast_.list_at(n.b, i));
}

// The refusal satellite_cache/paths.cpp names and declines to raise, written
// once because member() and call() both reach it: "a misspelled word under
// `satellite` is a program M7 refuses with M5's did-you-mean over the node the
// segment failed under; what a cache owes it is the program UNCHANGED."
//
// IT DOES NOT GO ON TO RESOLVE THE RECEIVER. Every segment inside a broken path
// would fail the same way, so one wrong word would print four carets.
void Resolver::no_such_word(const cache::PathMatch &stopped)
{
    problem<errors::Code::RESOLVE_NO_SUCH_WORD>(
        stopped.at, ast_.text_of(stopped.at),
        words::path_text(static_cast<words::NodeId>(stopped.under)));
    if (const std::string_view near =
            errors::suggest(stopped.under, ast_.text_of(stopped.at));
        !near.empty())
        suggest(near);
}

void Resolver::statement_form(NodeIndex node, words::NodeId under,
                              const char *word)
{
    // THE TWO FORMS THE PARSER DOES NOT BUILD AS A CHAIN, and there are exactly
    // two: `satellite.return(x)` is a Return node and
    // `satellite.include(satellite)` is an Include node. satellite_cache/
    // paths.hpp says the same thing about the same pair one milestone earlier
    // and for the same reason -- neither ever reaches the chain matcher, and
    // both still have to find their row.
    const Node &n = ast_[node];
    const bool has_argument = n.a != kNoNode;
    const int argc = has_argument ? 1 : 0;
    const bool reserved = has_argument && ast_[n.a].kind == NodeKind::Satellite;

    out_.walked++;
    const cache::PathMatch found = cache::shape_path(under, word, argc, reserved);
    if (found.found()) {
        info(node).path = found.id;
        info(node).origin = Origin::Walked;
    }

    if (!found.absorbs_argument)
        expression(n.a);
}

void Resolver::main_parameter(NodeIndex decl, std::string_view spelling,
                              Slot slot)
{
    // ONLY `satellite.main`, WHICH IS A DECISION AND NOT A NARROWING. DESIGN
    // §7.7 puts the object at `satellite.library.main.arguments` and describes
    // it as "the language handing the PROGRAM everything it knows about the
    // machine it woke up on" -- `satellite.main`'s parameter, and no other. A
    // capsule of the user's own with a parameter called `args` is holding
    // whatever its caller passed, and the M6 draft's version -- which
    // recognises the six spellings in every capsule -- would give that
    // parameter the machine's answers instead. That is DESIGN §1.1's "behind
    // their back" with the wrong value in the variable.
    if (frame_ == nullptr ||
        frame_->capsule != static_cast<words::PathId>(words::NodeId::MAIN))
        return;

    out_.walked++;
    if (child_named(words::NodeId::LIBRARY_MAIN, spelling) ==
        static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS)) {
        frame_->arguments = slot;
        if (!bindings_.empty())
            bindings_.back().arguments = true;
        info(decl).arguments = true;
        info(decl).origin = Origin::Walked;
        info(decl).path =
            static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN_ARGUMENTS);
        return;
    }

    // DESIGN §7.7's OPEN QUESTION, ANSWERED. "Declaring a parameter named
    // `argv` gets a plain list with no properties, silently. Under §9 that
    // silence is wrong -- the language should say so." It is said here and
    // ONLY where somebody plainly meant the object: the suggester has to come
    // back with one of the six first, so `argv` is refused and `input_lines` is
    // an ordinary list and is left alone. A rule and not a seventh spelling --
    // adding one is an edit to words.def, which is WORD_NUMBERS' authority and
    // the author's decision rather than this milestone's.
    const std::string_view near =
        errors::suggest(static_cast<words::PathId>(words::NodeId::LIBRARY_MAIN),
                        spelling);
    if (near.empty() || near == "()")
        return;

    problem<errors::Code::RESOLVE_ALMOST_ARGUMENTS>(
        decl, "arg, args, argz, argument, arguments or argumentz", spelling);
    suggest(near);
}

void Resolver::arguments_member(NodeIndex node, std::string_view word)
{
    // THE FIELDS COME OUT OF THE NUMBERING AND ARE NOT A LIST IN THIS FILE.
    // DESIGN §7.7 writes seven of them and words.def has all seven as nodes --
    // `arguments.machine.threads` is `1 14 1 1 1 3`, six numbers deep, which
    // WORD_NUMBERS §4 calls the clearest argument in the language for §1.3
    // refusing a segment limit. The M6 draft keeps a `kValidProps[]` array of
    // the same seven strings, which is a second place they live and the one
    // that goes stale when §7.7's open question about v1's other 33 is settled.
    const words::PathId parent = out_.at(ast_[node].a).path;
    if (parent == words::kNoPath)
        return;

    const words::PathId field =
        child_named(static_cast<words::NodeId>(parent), word);
    out_.walked++;
    if (field != words::kNoPath) {
        info(node).arguments = true;
        info(node).origin = Origin::Walked;
        info(node).path = field;
        return;
    }

    std::string has;
    for (words::PathId c = words::first_child(static_cast<words::NodeId>(parent));
         c != words::kNoPath; c = words::next_sibling(c)) {
        const std::string_view spelling =
            words::spelling_of(static_cast<words::NodeId>(c));
        if (spelling.empty())
            continue;
        if (!has.empty())
            has += ", ";
        has += spelling;
    }
    problem<errors::Code::RESOLVE_NO_SUCH_ARGUMENT_FIELD>(node, word, has);
    if (const std::string_view near = errors::suggest(parent, word); !near.empty())
        suggest(near);
}

} // namespace satellite::resolve
