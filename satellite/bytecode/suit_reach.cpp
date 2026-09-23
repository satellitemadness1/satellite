// satellite/bytecode/suit_reach.cpp -- WHAT A SPACESUIT'S NAME REACHES, AND WHAT AN
// OBJECT'S MEMBER DOES (2026-09-22). capsule_reach.cpp is the same for capsules; the
// rules here are 003's where 003 had one (resolve.cpp, names.cpp), quoted where they
// are its sentences, and marked where they are new.

#include "capsule_scan.hpp"

#include <algorithm>
#include <utility>

namespace satellite004 {
namespace {

std::string joined(const std::vector<std::string> &names)
{
    std::string written;
    for (const std::string &each : names) written += (written.empty() ? "" : ".") + each;
    return written;
}

// `scope` IS `outer` OR SOMEWHERE INSIDE IT.
bool within(const CapsuleTable &table, std::size_t scope, std::size_t outer)
{
    for (std::size_t at = scope; at != kNoScope; at = table.scopes[at].parent)
        if (at == outer)
            return true;
    return false;
}

std::string named(const CapsuleTable &table, std::size_t scope)
{
    const CapsuleScope &it = table.scopes[scope];
    if (it.parent == kNoScope)
        return "the file " + it.file;
    return (it.is_a_suit() ? "the spacesuit " : "the satellite.namespace ") + it.within;
}

} // namespace

std::size_t CapsuleTable::suit_around(std::size_t scope) const
{
    for (std::size_t at = scope; at != kNoScope; at = scopes[at].parent)
        if (scopes[at].is_a_suit())
            return at;
    return kNoScope;
}

const CapsuleSite &CapsuleTable::on_the_object(const CapsuleSite &site, const UserDefinedHandle &self) const
{
    if (site.suit == kNoScope || self == nullptr || self->layout == nullptr || self->layout->suit == site.suit)
        return site;
    for (const std::size_t each : self->layout->lineage) {
        if (each == site.suit)
            return site;
        const auto found = scopes[each].capsules.find(site.name);
        if (found != scopes[each].capsules.end())
            return sites[found->second];
    }
    return site;
}

std::size_t CapsuleTable::constructor_of(std::size_t suit) const
{
    const std::vector<std::size_t> own{suit};
    const std::vector<std::size_t> &lineage =
        scopes[suit].layout != nullptr && !scopes[suit].layout->lineage.empty() ? scopes[suit].layout->lineage : own;
    for (const std::size_t each : lineage)
        if (scopes[each].constructor != kNoSite)
            return scopes[each].constructor;
    return kNoSite;
}

std::size_t CapsuleTable::suit_named(std::size_t scope, const std::vector<std::string> &names, std::string &why) const
{
    const std::string written = joined(names);
    if (scope == kNoScope || names.empty()) {
        why = "no spacesuit named " + written;
        return kNoScope;
    }
    const std::size_t row = scopes[scope].row;

    // THE FIRST NAME, FROM HERE OUTWARD: a spacesuit, or -- when more names follow -- a
    // space; then a file this file includes. The nearest wins, as a bare capsule's does.
    std::vector<std::size_t> at;
    for (std::size_t outward = scope; outward != kNoScope && at.empty(); outward = scopes[outward].parent) {
        const auto suit = scopes[outward].suits.find(names.front());
        if (suit != scopes[outward].suits.end())
            at.push_back(suit->second);
        else if (names.size() > 1) {
            const auto space = scopes[outward].spaces.find(names.front());
            if (space != scopes[outward].spaces.end())
                at.push_back(space->second);
        }
    }
    if (at.empty() && names.size() > 1 && row < included.size()) {
        const auto file = included[row].find(names.front());
        if (file != included[row].end())
            at = file->second;
    }
    if (at.empty()) {
        // NOT DRAGGED INTO THE GLOBAL NAMESPACE, as a capsule is not -- but the spelling
        // that reaches it is said.
        if (names.size() == 1 && row < included.size())
            for (const auto &[stem, files] : included[row])
                for (const std::size_t file : files)
                    if (scopes[file].suits.count(names.front()) != 0) {
                        why = names.front() + " is a spacesuit of " + scopes[file].file + ", and a file's spacesuits "
                              "are reached through its name -- write " + stem + "." + names.front();
                        return kNoScope;
                    }
        why = "no spacesuit named " + written + " -- a type is a satellite.variable or satellite.container word, "
              "or the name of a satellite.spacesuit this line can reach";
        return kNoScope;
    }

    // EACH NAME AFTER IT: a spacesuit (or, before the last, a space) inside where the walk
    // is. A SPACESUIT DECLARED INSIDE ANOTHER is reached from outside that one only when
    // it was written in its satellite.public -- new in 004 (suit_scan.cpp).
    std::string walked = names.front();
    for (std::size_t n = 1; n < names.size(); ++n) {
        std::vector<std::size_t> deeper;
        for (const std::size_t each : at) {
            const auto suit = scopes[each].suits.find(names[n]);
            if (suit != scopes[each].suits.end()) {
                if (scopes[each].is_a_suit() && !scopes[suit->second].is_public && !within(*this, scope, each)) {
                    why = walked + "." + names[n] + " is declared inside the satellite.protected part of the spacesuit " +
                          scopes[each].within + ", so only " + scopes[each].within + " itself can name it -- move it "
                          "to satellite.public";
                    return kNoScope;
                }
                deeper.push_back(suit->second);
                continue;
            }
            if (n + 1 < names.size()) {
                const auto space = scopes[each].spaces.find(names[n]);
                if (space != scopes[each].spaces.end())
                    deeper.push_back(space->second);
            }
        }
        if (deeper.empty()) {
            why = named(*this, at.front()) + " has no spacesuit named " + names[n];
            return kNoScope;
        }
        at = std::move(deeper);
        walked += "." + names[n];
    }

    // THE LAST NAME IS A SPACESUIT, IN EXACTLY ONE PLACE. Two is two files of one name
    // that both declare it -- the one case the program could mean either.
    std::vector<std::size_t> suits;
    for (const std::size_t each : at)
        if (scopes[each].is_a_suit())
            suits.push_back(each);
    if (suits.empty()) {
        why = written + " is a satellite.namespace or a file, and not a spacesuit, so it cannot be a type";
        return kNoScope;
    }
    if (suits.size() > 1) {
        why = written + " could be a spacesuit of either of two files this file includes, both named " +
              names.front() + " -- rename one of the files";
        return kNoScope;
    }
    return suits.front();
}

const CapsuleSite *CapsuleTable::member(std::size_t suit, const std::string &name, std::size_t from,
                                        signed long long int &code, std::string &why) const
{
    const CapsuleScope &of = scopes[suit];
    const std::string shown = of.layout != nullptr ? of.layout->shown : of.within;
    // ITS OWN CAPSULES, THEN ITS SUPERTYPES', most derived first -- so a capsule a
    // spacesuit declares again replaces its supertype's for its objects (003's rule).
    const std::vector<std::size_t> own{suit};
    const std::vector<std::size_t> &lineage =
        of.layout != nullptr && !of.layout->lineage.empty() ? of.layout->lineage : own;
    // A METHOD WORD'S OTHER SPELLING: `b.to_number()` lexes to the method's code, which
    // reads back as its first spelling, `number` -- so a capsule declared `to_number` is
    // looked for by the code as well (the review, 2026-09-22).
    const token::Code as_a_method = token::method_code_of(name);
    for (const std::size_t each : lineage) {
        auto found = scopes[each].capsules.find(name);
        if (found == scopes[each].capsules.end() && as_a_method != 0)
            for (auto spelled = scopes[each].capsules.begin(); spelled != scopes[each].capsules.end(); ++spelled)
                if (token::method_code_of(spelled->first) == as_a_method) {
                    found = spelled;
                    break;
                }
        if (found == scopes[each].capsules.end())
            continue;
        const CapsuleSite &site = sites[found->second];
        // 003's S0516: a protected capsule, called with a dot, only from its spacesuit's
        // own capsules -- on this object or another of its kind -- and from a spacesuit
        // that extends it, or one declared inside it (both new in 004: a spacesuit's
        // supertype's capsules are its own, and one inside it is part of its body).
        const std::size_t caller = suit_around(from);
        const bool extends_it = caller != kNoScope && scopes[caller].layout != nullptr &&
                                scopes[caller].layout->is_a(each);
        if (!site.is_public && !within(*this, from, each) && !extends_it) {
            const std::string declaring = scopes[each].layout != nullptr ? scopes[each].layout->shown : shown;
            code = member_is_protected;
            why = name + " is inside the satellite.protected part of " + declaring + ", so only " + declaring +
                  "'s own capsules can reach it -- move it to satellite.public, or add a capsule there that answers it";
            return nullptr;
        }
        return &site;
    }
    // 003's S0517, WORD FOR WORD, and for a public field too: "a spacesuit field is
    // reachable from inside the spacesuit and nowhere else, which is what makes
    // satellite.protected a statement about the language rather than a comment"
    // (003 DESIGN §12). Inside, a field is its bare name.
    if (of.layout != nullptr && of.layout->slot_of(name) != kNoSlot) {
        code = member_is_protected;
        why = name + " is a field of " + shown + " and fields are reached from inside the spacesuit only -- write a "
              "capsule in " + shown + " that answers it and call that instead";
        return nullptr;
    }
    code = satl_line_not_understood;
    why = of.suits.count(name) != 0
              ? shown + "." + name + " is a spacesuit declared inside " + shown + ", and not something an object of it "
                "does -- it is a type: " + shown + "." + name + " a_name"
              : shown + " has no " + name + " -- a spacesuit answers only what its own capsules are called";
    return nullptr;
}

bool resolve_shape(const CapsuleTable &table, std::size_t scope, TypeShape &shape, std::string &why)
{
    if (shape.is_a_suit()) {
        shape.suit = table.suit_named(scope, shape.suit_names, why);
        return shape.suit != kNoScope;
    }
    for (TypeShape &inner : shape.parameters)
        if (!resolve_shape(table, scope, inner, why))
            return false;
    return true;
}

namespace scan {
namespace {

// A SPACESUIT'S SUPERTYPES' FIELDS BECOME ITS FIRST FIELDS, and its lineage is itself and
// then theirs. `s` is flattened after what it extends -- a chain is walked up to the
// first spacesuit already done, and flattened back down -- so no C++ recursion is spent
// on however long a chain a person writes.
void flatten(CapsuleTable &table, std::size_t s, std::vector<bool> &done)
{
    std::vector<std::size_t> chain;
    for (std::size_t at = s; at != kNoScope && !done[at]; at = table.scopes[at].super)
        chain.push_back(at);
    for (std::size_t n = chain.size(); n > 0; --n) {
        CapsuleScope &suit = table.scopes[chain[n - 1]];
        satelliteSuitLayout &layout = *suit.layout;
        done[chain[n - 1]] = true;
        layout.lineage.assign(1, chain[n - 1]);
        if (suit.super == kNoScope)
            continue;
        const CapsuleScope &parent = table.scopes[suit.super];
        const satelliteSuitLayout &inherited = *parent.layout;
        // A FIELD DECLARED AGAIN IS A SECOND SLOT OF ONE NAME, as in 003: the supertype's
        // capsules read the supertype's, and this spacesuit's its own (satelliteSuitLayout::
        // slot_of). A capsule of the same name REPLACES the supertype's for this spacesuit's
        // objects, as 003's most-derived capsule did. A FIELD AND A CAPSULE of one name
        // are refused: `name()` could then mean either.
        for (const SuitField &field : layout.fields) {
            bool a_capsule = false;
            for (const std::size_t each : inherited.lineage)
                a_capsule = a_capsule || table.scopes[each].capsules.count(field.name) != 0;
            if (a_capsule)
                refuse(table, field.row, field.at, name_declared_twice,
                       field.name + " is already a capsule of " + inherited.shown + ", which " + layout.shown +
                           " extends -- a field and a capsule cannot share a name");
        }
        for (const auto &[name, at] : suit.capsules)
            if (inherited.slot_of(name) != kNoSlot)
                refuse(table, table.sites[at].row, table.sites[at].declared_at, name_declared_twice,
                       name + " is already a field of " + inherited.shown + ", which " + layout.shown +
                           " extends -- a spacesuit has one name for each of its fields and capsules");
        std::vector<SuitField> fields = inherited.fields;
        layout.own_fields = fields.size();
        fields.insert(fields.end(), layout.fields.begin(), layout.fields.end());
        layout.fields = std::move(fields);
        layout.lineage.insert(layout.lineage.end(), inherited.lineage.begin(), inherited.lineage.end());
    }
}

// TWO SHAPES ARE THE SAME TYPE: the same word, the same spacesuit, the same between < and >.
bool same_shape(const TypeShape &a, const TypeShape &b)
{
    if (a.word != b.word || a.suit != b.suit || a.parameters.size() != b.parameters.size())
        return false;
    for (std::size_t n = 0; n < a.parameters.size(); ++n)
        if (!same_shape(a.parameters[n], b.parameters[n]))
            return false;
    return true;
}

// A CAPSULE A SUBTYPE DECLARES AGAIN REPLACES ITS SUPERTYPE'S FOR ITS OBJECTS -- and a
// name declared as the supertype may hold one of them, so the replacement must take and
// answer what the one it replaces does, and be as reachable: otherwise the check, which
// judges `any.call_kind()` by what `any` was declared, passes a line the object then
// refuses (the review of 2026-09-22 found both). 003 dispatched by the declared type
// and never met it.
void overrides_fit(CapsuleTable &table, std::size_t s)
{
    const CapsuleScope &suit = table.scopes[s];
    const satelliteSuitLayout &layout = *suit.layout;
    for (const auto &[name, own_at] : suit.capsules) {
        const CapsuleSite &own = table.sites[own_at];
        for (std::size_t n = 1; n < layout.lineage.size(); ++n) {
            const CapsuleScope &above = table.scopes[layout.lineage[n]];
            const auto replaced_at = above.capsules.find(name);
            if (replaced_at == above.capsules.end())
                continue;
            const CapsuleSite &replaced = table.sites[replaced_at->second];
            bool takes_the_same = own.parameters.size() == replaced.parameters.size();
            for (std::size_t p = 0; takes_the_same && p < own.parameters.size(); ++p)
                takes_the_same = same_shape(own.parameters[p].shape, replaced.parameters[p].shape);
            std::string differs;
            if (replaced.is_public && !own.is_public)
                differs = "it is in satellite.protected and the one it replaces is public";
            else if (!takes_the_same)
                differs = "it takes different arguments from the one it replaces";
            else if (!same_shape(own.returns, replaced.returns))
                differs = "it answers " + (own.answers() ? shape_written(own.returns) : std::string("nothing")) +
                          " and the one it replaces answers " +
                          (replaced.answers() ? shape_written(replaced.returns) : std::string("nothing"));
            if (!differs.empty())
                refuse(table, own.row, own.declared_at, satl_line_not_understood,
                       layout.shown + "'s " + name + " replaces " + above.layout->shown + "'s for its objects, and " +
                           differs + " -- a name declared " + above.layout->shown +
                           " may hold one of them, so the two must be called the same way");
            break;
        }
    }
}

// A FIELD WITH BRACKETS MAKES AN OBJECT WHENEVER ITS SPACESUIT'S OBJECT IS MADE -- so a
// spacesuit that, through such fields, comes back round to itself would never finish
// making one (the review, 2026-09-22: `node next(1)` inside node segfaulted). Refused at
// the first field of the ring; a field with nothing after its name starts empty.
void rings_of_making(CapsuleTable &table)
{
    for (std::size_t s = 0; s < table.scopes.size(); ++s) {
        if (!table.scopes[s].is_a_suit())
            continue;
        std::vector<bool> seen(table.scopes.size(), false);
        std::vector<std::size_t> waiting;
        for (const SuitField &field : table.scopes[s].layout->fields)
            if (field.made && field.shape.suit != kNoScope)
                waiting.push_back(field.shape.suit);
        bool comes_back = false;
        while (!waiting.empty() && !comes_back) {
            const std::size_t at = waiting.back();
            waiting.pop_back();
            if (at == s) { comes_back = true; break; }
            if (seen[at])
                continue;
            seen[at] = true;
            for (const SuitField &field : table.scopes[at].layout->fields)
                if (field.made && field.shape.suit != kNoScope)
                    waiting.push_back(field.shape.suit);
        }
        if (!comes_back)
            continue;
        const satelliteSuitLayout &layout = *table.scopes[s].layout;
        for (const SuitField &field : layout.fields)
            if (field.made) {
                refuse(table, field.row, field.at, satl_line_not_understood,
                       "making a " + layout.shown + " makes a " + shape_written(field.shape) + " for its field " +
                           field.name + ", and that comes back round to making a " + layout.shown +
                           " again, so an object would never finish being made -- declare " + field.name +
                           " with nothing after its name, and give it an object in satellite.constructor");
                break;
            }
    }
}

} // namespace

void resolve_types(CapsuleTable &table)
{
    for (CapsuleSite &site : table.sites) {
        for (CapsuleParameter &takes : site.parameters) {
            std::string why;
            if (!resolve_shape(table, site.scope, takes.shape, why))
                refuse(table, site.row, site.declared_at, name_not_declared,
                       site.shown + "'s " + takes.name + " is declared " + shape_written(takes.shape) + ", and " + why);
        }
        std::string why;
        if (site.answers() && !resolve_shape(table, site.scope, site.returns, why))
            refuse(table, site.row, site.declared_at, name_not_declared,
                   site.shown + " answers " + shape_written(site.returns) + ", and " + why);
    }
    for (std::size_t s = 0; s < table.scopes.size(); ++s) {
        CapsuleScope &suit = table.scopes[s];
        if (!suit.is_a_suit())
            continue;
        for (SuitField &field : suit.layout->fields) {
            std::string why;
            if (!resolve_shape(table, s, field.shape, why))
                refuse(table, field.row, field.at, name_not_declared,
                       field.name + " is declared " + shape_written(field.shape) + ", and " + why);
        }
        // WHAT IT EXTENDS is named from where its header stands, as any type there is.
        if (!suit.super_names.empty()) {
            std::string why;
            const std::size_t super = table.suit_named(suit.parent, suit.super_names, why);
            if (super == kNoScope)
                refuse(table, suit.row, suit.declared_at, name_not_declared,
                       "satellite.spacesuit " + suit.name + " extends " + joined(suit.super_names) +
                           ", and " + why);
            else
                suit.super = super;
        }
    }
    // A SPACESUIT THAT EXTENDS ITSELF, however far round: 003's S0520, word for word, and
    // the link is cut so the flattening below ends.
    for (std::size_t s = 0; s < table.scopes.size(); ++s) {
        if (!table.scopes[s].is_a_suit())
            continue;
        std::size_t at = table.scopes[s].super;
        for (std::size_t steps = 0; at != kNoScope && at != s && steps < table.scopes.size(); ++steps)
            at = table.scopes[at].super;
        if (at == s) {
            const std::string &name = table.scopes[s].name;
            refuse(table, table.scopes[s].row, table.scopes[s].declared_at, satl_line_not_understood,
                   name + " extends itself -- following the brackets from " + name + " leads back to " + name +
                       ", and a spacesuit cannot hold a copy of itself");
            table.scopes[s].super = kNoScope;
        }
    }
    std::vector<bool> done(table.scopes.size(), false);
    for (std::size_t s = 0; s < table.scopes.size(); ++s)
        if (table.scopes[s].is_a_suit())
            flatten(table, s, done);
    for (std::size_t s = 0; s < table.scopes.size(); ++s)
        if (table.scopes[s].is_a_suit())
            overrides_fit(table, s);
    rings_of_making(table);
}

} // namespace scan
} // namespace satellite004
