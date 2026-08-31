// Scopes, frame slots, and the words a complaint is made of -- DESIGN §7.2 and
// §7.4. See name_resolver/resolve.hpp for what a slot is and why it exists.
//
// A SLOT IS AN INDEX AND THE FRAME IS A VECTOR, which is the whole of §7.2:
// "every capsule call gets its own frame; locals resolve to integer slot
// indices statically, before execution". Nothing here allocates anything at run
// time and nothing here is shared between threads, because a frame is reachable
// from one thread by construction -- which is the property §7.1's 1585 wrong
// results out of 1600 were the absence of.
//
// AND A SLOT IS NEVER REUSED, which is §7.4 and is the rule that looks like an
// optimisation left on the table until the reason is read. A redeclaration
// REBINDS the name to a fresh slot rather than writing over the old one,
// because a spacesuit is a reference type: reusing the slot would leave every
// handle already taken to the first instance pointing at the second, and a list
// built by that idiom would read back as n copies of its last element WITH NO
// ERROR ANYWHERE. The cost is one Value per redeclaration in a frame that lives
// for one call.

#include "name_resolver/resolve_internal.hpp"

#include "error_reporter/report.hpp"
#include "error_reporter/suggest.hpp"

#include <algorithm>
#include <cstddef>
#include <string_view>

namespace satellite::resolve {

Slot Resolver::declare(NodeIndex decl, uint32_t token, words::PathId type)
{
    const std::string_view spelling = ast_.token(token).text;

    // DESIGN §1's GENERATING RULE, ENFORCED WHERE THE PARSER CANNOT. `satellite`
    // is a legal `primary` -- §6 has it, and `satellite.return(satellite)` is
    // why -- so `expect_word()` accepts it in every position a name may take.
    // A local called `satellite` would shadow the root of the language for the
    // rest of its scope, which is DESIGN §2's reservation rule broken from the
    // inside.
    if (is_reserved_word(ast_.token(token))) {
        problem<errors::Code::RESOLVE_NAME_IS_RESERVED>(
            decl, frame_ != nullptr ? "local" : "global");
        return kSlotGlobal;
    }

    // A DECLARATION OUTSIDE A CAPSULE IS NOT A LOCAL. DESIGN §7.2 reserves
    // `satellite.library` for shared and global state, and this grammar has no
    // top-level statements at all (resolve.cpp's pass 3 says so), so the only
    // way here with no frame is a global's initialiser -- which cannot declare.
    if (frame_ == nullptr) {
        info(decl).slot = kSlotGlobal;
        info(decl).type = type;
        return kSlotGlobal;
    }

    const Slot slot = static_cast<Slot>(frame_->names.size());
    frame_->names.push_back(spelling);
    frame_->types.push_back(ast_[decl].a);

    info(decl).slot = slot;
    info(decl).type = type;

    // PUSHED AND NEVER OVERWRITTEN, which is §7.4 in one line: lookup() reads
    // backwards, so a second binding of one name shadows the first and the
    // first keeps its slot. The dump prints both, which is how somebody can see
    // that the two are not the same storage.
    bindings_.push_back({spelling, slot, type, decl, false});
    return slot;
}

const Resolver::Binding *Resolver::lookup(std::string_view spelling) const
{
    // BACKWARDS, AND OVER THE WHOLE STACK RATHER THAN SCOPE BY SCOPE. Every
    // binding still in this vector is live -- close_scope() truncates -- so the
    // innermost match is simply the last one, and an inner block shadowing an
    // outer name and a redeclaration shadowing itself are the same mechanism
    // rather than two.
    for (size_t i = bindings_.size(); i-- > 0;)
        if (bindings_[i].name == spelling)
            return &bindings_[i];
    return nullptr;
}

const Resolver::Capsule *Resolver::capsule_named(std::string_view spelling) const
{
    for (const Capsule &at : capsules_)
        if (at.name == spelling)
            return &at;
    return nullptr;
}

const Resolver::Capsule *Resolver::suit_named(std::string_view spelling) const
{
    for (const Capsule &at : suits_)
        if (at.name == spelling)
            return &at;
    return nullptr;
}

void Resolver::close_scope()
{
    bindings_.resize(scopes_.back());
    scopes_.pop_back();
}

std::string_view Resolver::nearest_in_scope(std::string_view spelling) const
{
    // DESIGN §4.6 OVER A LIST errors::suggest() CANNOT SEARCH. That function
    // walks one node's frozen children, which is exactly right for a misspelled
    // segment of a path and useless for a misspelled local: these names were
    // met four milestones after the trie was written and are not in it. The
    // POLICY is shared -- distance() and close_enough() are the reporter's, and
    // suggest.hpp argues why the threshold is a rule rather than a constant --
    // so what is written twice is the loop and not the judgement.
    std::string_view best;
    size_t fewest = errors::kTooFar;

    const auto consider = [&](std::string_view candidate) {
        const size_t edits = errors::distance(spelling, candidate);
        if (edits < fewest) {
            fewest = edits;
            best = candidate;
        }
    };

    // IN SCOPE FIRST AND THE CAPSULES AFTER, so a tie goes to the nearer thing.
    // Somebody who wrote `cout` inside a capsule that has a local `count` means
    // the local, even if a capsule three files down is spelled `couot`.
    for (size_t i = bindings_.size(); i-- > 0;)
        consider(bindings_[i].name);
    for (const Capsule &at : capsules_)
        consider(at.name);

    if (!errors::close_enough(fewest, std::max(spelling.size(), best.size())))
        return {};
    return best;
}

errors::Span Resolver::span_of(NodeIndex node) const
{
    // THE ANCHOR TOKEN AND NOT THE NODE'S EXTENT, which ast.hpp chose for M4
    // and said M5 or M7 would revisit if an extent turned out to be wanted on
    // every node. It is not: every complaint this pass makes is about ONE WORD
    // -- a name, a segment of a path, a type -- and the anchor token is that
    // word. ast.hpp's own example is the one that decides it: a caret under a
    // whole expression would point at `satellite.console.display("x")` where
    // what is wrong is `consle`.
    const Token &at = ast_.token_of(node);
    return errors::Span{at.start, at.end, at.line};
}

} // namespace satellite::resolve
