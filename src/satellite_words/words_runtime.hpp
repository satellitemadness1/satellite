#pragma once

// The user's half of the numbering -- part of words.hpp.
//
// PLAN §8.1 IS THE WHOLE OF THIS FILE'S REASON TO EXIST, and its table is the
// thing to keep in view, because the two halves look alike and are not:
//
//                     the language's words        the user's names
//     numbered        ahead of time, in words.def at parse time, as met
//     frozen          forever, across programs    for one run
//     checked by      static_assert in a header   nothing a compiler can see
//     a PathId is     stable and quotable         valid inside one run only
//
// A capsule or a spacesuit the user writes gets a number too -- the next one
// free under the node that owns it, allocated when the name is FIRST MET
// (WORD_NUMBERS §3). satellite.library.main is 1 14 1 because the language put
// it there; a user's `x` is 1 14 3, because 1 and 2 are taken and nothing else
// under `library` has been met yet.
//
// M2 OWNS A REAL ALLOCATOR AND NOT A STUB. PLAN §8.1 asks for that decision to
// be made and recorded in the code, and this is the record. The counter was
// never optional -- PLAN M2 lists "a live child counter on every node" among
// the things the milestone builds -- and once the counter exists, refusing to
// hand out the number it is holding buys nothing and leaves a second thing to
// build later. THE CALLER ARRIVES AT M4: the parser is what meets a name for
// the first time, and PLAN M4 now says so. Until then the only consumer is
// tests/words_test, which is deliberate rather than an oversight.
//
// WHY THE FROZEN TABLE IS NOT COPIED. Seeding a mutable trie from the constexpr
// one at startup would cost a walk of 255 rows on every run, including the ones
// that never define a name, against PLAN §4.3's measured 0.01 ms budget for the
// whole of satl's own startup. What is copied is the counters -- one array of
// 256 uint32_t, about a kilobyte -- and user rows are appended in a vector that
// stays empty until something is defined.

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"
#include "satellite_words/words_spellings.hpp"

namespace satellite::words {

// The numbering as it stands during one run: the frozen table, plus whatever
// names this program has met.
//
// NOT A SINGLETON AND NOT A GLOBAL. A run's names end with the run, and an
// object that can simply be destroyed says that better than a reset() somebody
// has to remember to call -- which matters at M22, where the prompt runs many
// programs in one process and each needs its own numbering.
class Words {
public:
    Words()
    {
        for (PathId i = 0; i <= kNodeCount; i++)
            next_free_[i] = frozen_children(static_cast<NodeId>(i)) + 1;
    }

    // The number a name would take under this parent, without taking it.
    uint32_t next_free(NodeId parent) const
    {
        return next_free_[static_cast<PathId>(parent)];
    }

    // A name already met under this parent, or kNoPath.
    //
    // THE LANGUAGE'S CHILDREN ARE SEARCHED FIRST, so a user name can never be
    // answered in place of a word the language owns. What happens when a user
    // WRITES such a name is a different question, and M4 answered it: define()
    // below refuses, and parser_declarations.cpp turns that refusal into
    // S0241 with the node the name would have hung under.
    //
    // THIS SAID "DESIGN §2's reservation rule is decided at M7's resolve" UNTIL
    // 2026-08-31 and it was two milestones stale. M4 decided it, in the
    // function above. What M7 actually decides is the one case a bare name can
    // still shadow the language -- a LOCAL called `satellite`, which the parser
    // accepts because `satellite` is a legal `primary` (DESIGN §6) and only a
    // scope can refuse. errors.def's S0513 is that.
    PathId find(NodeId parent, std::string_view name) const
    {
        // AN EMPTY NAME IS NOT A NAME. The bare rows are spelled "" on purpose,
        // so without this an empty name matches the parent's bare shape and
        // comes back as a language word -- which define() below already refuses
        // and intern() would otherwise never reach. Found by review 2026-08-28.
        if (name.empty())
            return kNoPath;

        for (PathId c = first_child(parent); c != kNoPath; c = next_sibling(c))
            if (spelling_of(static_cast<NodeId>(c)) == name)
                return c;

        // THE ALIASES COUNT AS THE LANGUAGE OWNING THE SPELLING, and leaving
        // them out made the comment above this function false. Found by review
        // 2026-08-28: `intern(satellite.library.main, "args")` allocated a USER
        // number, 1 14 1 2, for one of DESIGN §7.7's six spellings of
        // `arguments` -- while walk() answered 1 14 1 1 for the same spelling
        // under the same parent. Two numbers for one word, and the half that
        // was wrong is the half a parser would have called.
        for (size_t i = 0; i < kAliasCount; i++)
            if (words::parent_of(kAliases[i].of) == parent &&
                std::string_view(kAliases[i].text) == name)
                return static_cast<PathId>(kAliases[i].of);

        for (size_t i = 0; i < user_.size(); i++)
            if (user_[i].parent == parent && user_[i].name == name)
                return kNodeCount + 1 + static_cast<PathId>(i);
        return kNoPath;
    }

    // Take the next free number under `parent` for `name`.
    //
    // Returns kNoPath when the language already owns that spelling under that
    // parent, or when the name has been defined in this run already. Both are
    // the caller's to report; M5 is the error reporter and M2 comes first, so
    // what this returns has to be enough to report from without changing later.
    PathId define(NodeId parent, std::string_view name)
    {
        if (name.empty() || find(parent, name) != kNoPath)
            return kNoPath;
        user_.push_back({parent, std::string(name),
                         next_free_[static_cast<PathId>(parent)]++});
        return kNodeCount + static_cast<PathId>(user_.size());
    }

    // Find, or define if this is the first time the name has been met. This is
    // what a parser wants; find() and define() are what a diagnostic wants.
    PathId intern(NodeId parent, std::string_view name)
    {
        const PathId found = find(parent, name);
        return found != kNoPath ? found : define(parent, name);
    }

    // The position under the parent, for either half of the numbering.
    uint32_t number_of(PathId id) const
    {
        if (is_language_word(id))
            return words::number_of(static_cast<NodeId>(id));
        return valid(id) ? user_[id - kNodeCount - 1].number : 0;
    }

    NodeId parent_of(PathId id) const
    {
        if (is_language_word(id))
            return words::parent_of(static_cast<NodeId>(id));
        return valid(id) ? user_[id - kNodeCount - 1].parent : NodeId::NONE;
    }

    // The user's own spelling. Empty for a language word, which has a text
    // rather than a name -- text_of() is that one, and the two are kept apart
    // because only one of them may ever be written into a `.satc` (SATC.md §3).
    std::string_view name_of(PathId id) const
    {
        return valid(id) ? std::string_view(user_[id - kNodeCount - 1].name)
                         : std::string_view();
    }

    size_t defined() const { return user_.size(); }

private:
    struct UserName {
        NodeId parent;
        std::string name;
        uint32_t number;
    };

    bool valid(PathId id) const
    {
        return id > kNodeCount && id - kNodeCount - 1 < user_.size();
    }

    uint32_t next_free_[kNodeCount + 1] = {};
    std::vector<UserName> user_;
};

} // namespace satellite::words
