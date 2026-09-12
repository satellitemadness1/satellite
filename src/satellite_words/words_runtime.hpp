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
// one at startup would cost a walk of 260 rows on every run, including the ones
// that never define a name, against PLAN §4.3's measured 0.01 ms budget for the
// whole of satl's own startup. What is copied is the counters -- one array of
// 261 uint32_t, about a kilobyte -- and user rows are appended in a vector that
// stays empty until something is defined.

// A USER'S NAME CAN BE A PARENT SINCE M26, AND M4 IS WHERE THAT WAS OWED.
// MILESTONES/M4.md §6 found it and named both the cost and the milestone: this
// file seeded one counter per node of the FROZEN table and walked the frozen
// child lists, so a `PathId` above kNodeCount handed to find() or define()
// indexed past the end of both -- and **a capsule declared inside a spacesuit
// has exactly such a parent**. A method therefore got no number at M4, its node
// carried kNoPath, and tests/parser_test/declarations.cpp asserted that rather
// than leaving it to be discovered.
//
// "Fixing it is a growable counter vector and a user-side child list -- small,
// and M26's, because M26 is where a spacesuit's members have to resolve." That
// is what this is, and it was small: `parent` widens from NodeId to PathId, the
// counters get a second home that grows with `user_`, and the three lookups
// each gain one arm for a parent above kNodeCount.
//
// THE FROZEN HALF IS UNTOUCHED AND THE ASYMMETRY IS THE POINT. A language
// parent still searches its frozen children, then the aliases, then the user
// rows -- in that order, so a user name can never answer in place of a word the
// language owns. A USER parent has no frozen children and no aliases by
// construction: nothing in words.def hangs under a name that did not exist when
// words.def was written. So its arm is the user rows alone, and the reservation
// rule is not weakened by a spacesuit having members.

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
    uint32_t next_free(PathId parent) const
    {
        return is_language_word(parent) ? next_free_[parent]
               : valid(parent)          ? user_[parent - kNodeCount - 1].next_free
                                        : 1;
    }

    uint32_t next_free(NodeId parent) const
    {
        return next_free(static_cast<PathId>(parent));
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
        return find(static_cast<PathId>(parent), name);
    }

    PathId find(PathId parent, std::string_view name) const
    {
        // A USER PARENT HAS NEITHER FROZEN CHILDREN NOR ALIASES, by
        // construction -- nothing in words.def hangs under a name that did not
        // exist when words.def was written -- so its arm is the user rows
        // alone. See the header note: this is where the two halves stop being
        // symmetrical, and it is why the reservation rule survives a spacesuit
        // having members.
        if (!is_language_word(parent)) {
            if (name.empty() || !valid(parent))
                return kNoPath;
            for (size_t i = 0; i < user_.size(); i++)
                if (user_[i].parent == parent && user_[i].name == name)
                    return kNodeCount + 1 + static_cast<PathId>(i);
            return kNoPath;
        }
        return find_under_language(static_cast<NodeId>(parent), name);
    }

private:
    PathId find_under_language(NodeId parent, std::string_view name) const
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
        // number, 1 14 1 2, for one of DESIGN §7.7's seven spellings of
        // `arguments` -- while walk() answered 1 14 1 1 for the same spelling
        // under the same parent. Two numbers for one word, and the half that
        // was wrong is the half a parser would have called.
        for (size_t i = 0; i < kAliasCount; i++)
            if (words::parent_of(kAliases[i].of) == parent &&
                std::string_view(kAliases[i].text) == name)
                return static_cast<PathId>(kAliases[i].of);

        for (size_t i = 0; i < user_.size(); i++)
            if (user_[i].parent == static_cast<PathId>(parent) &&
                user_[i].name == name)
                return kNodeCount + 1 + static_cast<PathId>(i);
        return kNoPath;
    }

public:
    // Take the next free number under `parent` for `name`.
    //
    // Returns kNoPath when the language already owns that spelling under that
    // parent, or when the name has been defined in this run already. Both are
    // the caller's to report; M5 is the error reporter and M2 comes first, so
    // what this returns has to be enough to report from without changing later.
    PathId define(PathId parent, std::string_view name)
    {
        if (name.empty() || find(parent, name) != kNoPath)
            return kNoPath;

        // A USER PARENT MUST EXIST BEFORE IT CAN OWN ANYTHING, which cannot be
        // arranged by ordering alone: the parser defines a spacesuit's name
        // before it parses the body, so the order is right by construction, and
        // this is the guard that says so rather than the guard that makes it so.
        if (!is_language_word(parent) && !valid(parent))
            return kNoPath;

        const uint32_t taken = is_language_word(parent)
                                   ? next_free_[parent]++
                                   : user_[parent - kNodeCount - 1].next_free++;
        user_.push_back({parent, std::string(name), taken, 1});
        return kNodeCount + static_cast<PathId>(user_.size());
    }

    PathId define(NodeId parent, std::string_view name)
    {
        return define(static_cast<PathId>(parent), name);
    }

    // Find, or define if this is the first time the name has been met. This is
    // what a parser wants; find() and define() are what a diagnostic wants.
    PathId intern(PathId parent, std::string_view name)
    {
        const PathId found = find(parent, name);
        return found != kNoPath ? found : define(parent, name);
    }

    PathId intern(NodeId parent, std::string_view name)
    {
        return intern(static_cast<PathId>(parent), name);
    }

    // The position under the parent, for either half of the numbering.
    uint32_t number_of(PathId id) const
    {
        if (is_language_word(id))
            return words::number_of(static_cast<NodeId>(id));
        return valid(id) ? user_[id - kNodeCount - 1].number : 0;
    }

    // WHAT THIS HANGS UNDER, AND IT ANSWERS A PathId SINCE M26 BECAUSE IT HAS
    // TO. A spacesuit's method hangs under the spacesuit, which is a user name,
    // and a NodeId cannot say so -- it could only answer NONE, which is what
    // "no parent" means. Every caller that genuinely wants a language node asks
    // `is_language_word` first, and the two that print a path now recurse.
    PathId parent_of(PathId id) const
    {
        if (is_language_word(id))
            return static_cast<PathId>(words::parent_of(static_cast<NodeId>(id)));
        return valid(id) ? user_[id - kNodeCount - 1].parent
                         : static_cast<PathId>(NodeId::NONE);
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
        PathId parent;
        std::string name;
        uint32_t number;

        // THE COUNTER THIS NAME HANDS OUT TO ITS OWN CHILDREN -- M26's half of
        // the growable counter vector M4 asked for. It rides on the row rather
        // than in a second vector so that a name and the numbers it owns cannot
        // get out of step with each other, which is the same argument
        // `next_free_` makes for the frozen half by being indexed by PathId.
        //
        // IT STARTS AT 1 AND NOT AT 0, because 0 is the bare shape -- WORD_NUMBERS
        // §1.3, "a trailing 0 is written only where a program can actually
        // write the bare form", and a spacesuit's members start at one for the
        // same reason every language node's children do.
        uint32_t next_free;
    };

    bool valid(PathId id) const
    {
        return id > kNodeCount && id - kNodeCount - 1 < user_.size();
    }

    uint32_t next_free_[kNodeCount + 1] = {};
    std::vector<UserName> user_;
};

} // namespace satellite::words
