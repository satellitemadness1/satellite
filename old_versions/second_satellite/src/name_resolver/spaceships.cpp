// Names that live in another file -- PLAN M25, `satellite.include(spaceship)`,
// built 2026-09-13. See name_resolver/resolve.hpp for Spaceship and File.
//
// A SPACESHIP IS A NAMESPACE AND NOT A SCOPE, which is the author's sentence
// from the day it was built: "when we include a file, we name that
// spaceship.spacesuit_name, or spaceship.capsule_name then? It doesn't get
// dragged into the global namespace?" It does not. Everything `ship.satl`
// declares is numbered under `satellite.library.ship`, by the parser, with a
// counter of its own; nothing in this file is looked up bare; and every road in
// here starts from a name the including file WROTE -- `ship` -- and asks the
// numbering what is under it.
//
// WHAT THE NUMBERING DOES NOT SAY IS WHAT KIND OF THING A NAME IS, and that is
// what `Resolved::declared` is for. `ship.setup` is a PathId either way; a
// capsule is called, a spacesuit is declared, and a global has its own road
// through `satellite.library`, so each gets its own answer.

#include "name_resolver/resolve_internal.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "satellite_spaceship/shape.hpp"

#include <string_view>

namespace satellite::resolve {

// A FILE RESOLVED ON ITS OWN READS ITS OWN INCLUDES, and learns their names and
// nothing else. `satl --resolve` over a program with `satellite.include(ship)`
// in it should not call `ship` a name nobody declared; it also has not loaded
// `ship.satl`, so `node` stays kNoPath and every lookup through it is silent.
void Resolver::own_ships()
{
    for (NodeIndex i = 1; i < ast_.size(); i++) {
        if (ast_[i].kind != NodeKind::Include)
            continue;
        const spaceship::Shape shape = spaceship::shape_of(ast_, i);
        if (shape.named != spaceship::Named::Spaceship)
            continue;
        const std::string_view name = spaceship::name_of(ast_, shape);
        if (spaceship_named(name) == nullptr)
            ships_.push_back({name, words::kNoPath, 0});
    }
}

const Spaceship *Resolver::spaceship_named(std::string_view spelling) const
{
    for (const Spaceship &ship : ships_)
        if (ship.name == spelling)
            return &ship;
    return nullptr;
}

const Spaceship *Resolver::spaceship_at(words::PathId node) const
{
    if (node == words::kNoPath)
        return nullptr;
    for (const Spaceship &ship : ships_)
        if (ship.node == node)
            return &ship;
    return nullptr;
}

const Resolved *Resolver::resolved_of(uint32_t file) const
{
    if (run_ == nullptr || file >= run_->results->size())
        return nullptr;
    return &(*run_->results)[file];
}

bool Resolver::a_spaceship_node(words::PathId path) const
{
    if (path == words::kNoPath || run_ == nullptr)
        return false;
    for (size_t f = 1; f < run_->files->size(); f++)
        if ((*run_->files)[f].library == path)
            return true;
    return false;
}

bool Resolver::through_an_unloaded_spaceship(const cache::PathMatch &found) const
{
    if (found.under != static_cast<words::PathId>(words::NodeId::LIBRARY) ||
        found.at == kNoNode)
        return false;
    const Spaceship *ship = spaceship_named(ast_.text_of(found.at));
    return ship != nullptr && ship->node == words::kNoPath;
}

// THIS FILE'S SUITS FIRST, THEN EVERY OTHER FILE'S. A PathId is unique across
// the run -- one numbering, words_runtime.hpp -- so the first file holding it
// is the only one, and the order only decides how soon the search ends.
const Suit *Resolver::suit_anywhere(words::PathId path) const
{
    if (const Suit *mine = out_.suit_at(path))
        return mine;
    if (run_ == nullptr)
        return nullptr;
    for (const Resolved &other : *run_->results)
        if (&other != &out_)
            if (const Suit *theirs = other.suit_at(path))
                return theirs;
    return nullptr;
}

void Resolver::spaceship_member(NodeIndex node, const Info &receiver,
                                std::string_view word)
{
    const Spaceship *ship = spaceship_at(receiver.path);
    if (ship == nullptr)
        return; // not loaded here -- own_ships() says why that is silent

    const words::PathId found = words_.find(ship->node, word);
    const Resolved *theirs = resolved_of(ship->file);
    const Declared *what =
        found == words::kNoPath || theirs == nullptr ? nullptr
                                                     : theirs->declared_at(found);
    if (what == nullptr) {
        problem<errors::Code::RESOLVE_NO_SUCH_SPACESHIP_MEMBER>(node, ship->name,
                                                                word);
        return;
    }

    switch (what->kind) {
    case Declares::Capsule:
        info(node).slot = kSlotCapsule;
        info(node).path = found;
        info(node).origin = Origin::Bound;
        return;
    case Declares::Spacesuit:
        info(node).slot = kSlotSpacesuit;
        info(node).path = found;
        info(node).type = found;
        info(node).origin = Origin::Bound;
        return;
    case Declares::Global:
        problem<errors::Code::RESOLVE_SPACESHIP_GLOBAL>(node, ship->name, word);
        return;
    }
}

// `ship.box` IN TYPE POSITION. The qualifier must be a spaceship this file
// includes and the name must be one of its spacesuits -- a capsule is not a
// type, and saying "no capsule or spacesuit named" about one is still true.
words::PathId Resolver::qualified_type(NodeIndex node, uint32_t qualifier)
{
    const std::string_view ship_name = ast_.token(qualifier).text;
    const std::string_view bare = ast_.text_of(node);
    const Spaceship *ship = spaceship_named(ship_name);
    if (ship == nullptr) {
        // THE CARET UNDER THE QUALIFIER, which is the word nobody declared.
        problem_at_token<errors::Code::RESOLVE_NO_SUCH_NAME>(qualifier, ship_name);
        return words::kNoPath;
    }
    if (ship->node == words::kNoPath)
        return words::kNoPath;

    const words::PathId found = words_.find(ship->node, bare);
    const Resolved *theirs = resolved_of(ship->file);
    const Declared *what =
        found == words::kNoPath || theirs == nullptr ? nullptr
                                                     : theirs->declared_at(found);
    if (what != nullptr && what->kind != Declares::Spacesuit) {
        problem<errors::Code::RESOLVE_SPACESHIP_NOT_A_TYPE>(node, ship_name, bare);
        return words::kNoPath;
    }
    if (found == words::kNoPath || suit_anywhere(found) == nullptr) {
        problem<errors::Code::RESOLVE_NO_SUCH_SPACESHIP_MEMBER>(node, ship_name,
                                                                bare);
        return words::kNoPath;
    }
    info(node).path = found;
    info(node).type = found;
    info(node).origin = Origin::Bound;
    return found;
}

} // namespace satellite::resolve
