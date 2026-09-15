#pragma once

// The invariants -- part of words.hpp, and the reason the asserts live in a
// header rather than in tests/words_test: every future consumer inherits them
// by including this module, so they hold for anyone who reads the numbering and
// not only for whoever remembers to run the test. That is the single decision
// worth copying from the first satellite's registry.
//
// PLAN M2 NAMES FOUR PROPERTIES AND ONLY ONE OF THEM IS AN ASSERT HERE. That is
// not a gap; it is what choosing the encoding bought, and it is written down
// because a reader who counts four in the plan and one below will otherwise
// conclude three were forgotten:
//
//   1. no holes -- every parent's children dense from 1. BY CONSTRUCTION. A
//      number is a POSITION (words_numbers.hpp), computed as ++counter, so the
//      file has no way to express a hole. What a hole would really be is a row
//      MISSING against WORD_NUMBERS §2.2, which silently renumbers every later
//      sibling -- and no assert can see that, because both files would be
//      internally consistent. tests/words_test is what catches it, by walking
//      all 222 of the authority's paths.
//   2. no duplicates among non-aliases. BY CONSTRUCTION, same reason: two rows
//      cannot share a position. §2.2's three duplicate numbers are its three
//      `.range` aliases, and an alias is not a node here.
//   3. no orphans, and no node is its own ancestor. ASSERTED BELOW, and it is
//      the one of the four that the encoding does not already give.
//   4. no alias points at a number that does not exist. BY THE COMPILER. An
//      alias names an IDENTIFIER, so a bad one is an unknown enumerator and the
//      build stops with the row named.
//
// The rest below are properties this encoding needs and PLAN could not have
// known to ask for, because they are about the file's shape rather than about
// the numbering.

#include <array>
#include <cstdint>
#include <string_view>

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"
#include "satellite_words/words_spellings.hpp"

namespace satellite::words::detail {

// The two lists that must stay the same length. Both are expansions of the same
// rows, so they can only disagree through the hand-written parts of each -- the
// enum's NONE and COUNT_, and the array's row 0. This is what watches those.
constexpr bool enum_matches_table()
{
    return static_cast<PathId>(NodeId::COUNT_) == kNodeCount + 1;
}

// §1: a language-owned name is a dotted path ROOTED AT `satellite`. The closure
// property the whole registry rests on, so it is checked rather than assumed.
constexpr bool one_root()
{
    for (PathId i = 1; i <= kNodeCount; i++)
        if (kNodes[i].parent == NodeId::NONE && i != static_cast<PathId>(NodeId::SATELLITE))
            return false;
    return kNodes[static_cast<PathId>(NodeId::SATELLITE)].parent == NodeId::NONE;
}

// A parent is declared before its children.
//
// words_numbers.hpp NUMBERS THE WHOLE TABLE IN ONE FORWARD PASS, which is legal
// only because of this. A child declared above its parent would be counted
// before the parent's own counter existed and would take a number that belongs
// to somebody else -- silently, and with the file still looking ordered.
constexpr bool parents_come_first()
{
    for (PathId i = 1; i <= kNodeCount; i++)
        if (static_cast<PathId>(kNodes[i].parent) >= i)
            return false;
    return true;
}

// No node is its own ancestor. Implied by parents_come_first above, and checked
// separately because it is PLAN M2's property 3 as written and the two would
// stop implying one another the day the file gains a forward reference.
constexpr bool no_cycles()
{
    for (PathId i = 1; i <= kNodeCount; i++) {
        PathId steps = 0;
        for (NodeId at = static_cast<NodeId>(i); at != NodeId::NONE; at = parent_of(at))
            if (++steps > kNodeCount)
                return false;
    }
    return true;
}

// A bare shape is the parent called with nothing, so a parent has at most one.
constexpr bool one_bare_shape_each()
{
    for (PathId p = 0; p <= kNodeCount; p++) {
        bool seen = false;
        for (PathId c = kChildren.first[p]; c != kNoPath; c = kChildren.next[c]) {
            if (kNodes[c].kind != kBare)
                continue;
            if (seen)
                return false;
            seen = true;
        }
    }
    return true;
}

// SAT_BARE and "()" say the same thing, so they must never disagree.
//
// A SENTINEL THAT COLLIDES WITH A REAL VALUE IS NOT A SENTINEL -- the first
// satellite spent three commits with arity 0 meaning both "no operands" and
// "the table cannot say". Here the collision would be quieter: a row marked
// SAT_NUMBERED whose text is "()" would take a real position AND read as a bare
// call, and every sibling after it would be off by one.
constexpr bool bare_rows_are_spelled_bare()
{
    for (PathId i = 1; i <= kNodeCount; i++) {
        const bool marked = kNodes[i].kind == kBare;
        const bool written = std::string_view(kNodes[i].text) == "()";
        if (marked != written)
            return false;
    }
    return true;
}

constexpr bool kinds_are_distinct()
{
    return kNumbered != kBare;
}

// A row has a spelling exactly when its text does not begin with `(`.
//
// THE TWO ROWS WITHOUT ONE ARE THE TWO KINDS OF CALL SHAPE: "(satellite)", an
// argument that extends the path and is chosen by what the call hands over, and
// "()", a bare shape that is reached by calling its parent with nothing.
// Neither is a word, and words_walk.hpp REFUSES TO INTERN AN EMPTY SPELLING --
// so a word that lost its spelling would be unreachable by name and reachable
// by nothing else either, holding a number nothing can ever produce.
//
// Written first as "an argument row has no spelling" with "()" excluded from
// the argument case, which fired on all 38 bare rows: they begin with `(` too,
// and the honest predicate is about the character rather than about the kind.
constexpr bool spellings_match_texts()
{
    for (PathId i = 1; i <= kNodeCount; i++) {
        const std::string_view text = kNodes[i].text;
        if (text.empty())
            return false;
        if ((text.front() == '(') != spelling_of(static_cast<NodeId>(i)).empty())
            return false;
    }
    return true;
}

// Two children of one parent may share a SPELLING -- `input` is three rows
// under `console` and that is §1.3 working -- but never a whole TEXT, which
// includes the argument list. Two identical texts is one row the walk can never
// reach, holding a number that is therefore unusable.
constexpr bool sibling_texts_are_distinct()
{
    for (PathId p = 0; p <= kNodeCount; p++)
        for (PathId a = kChildren.first[p]; a != kNoPath; a = kChildren.next[a])
            for (PathId b = kChildren.next[a]; b != kNoPath; b = kChildren.next[b])
                if (std::string_view(kNodes[a].text) == std::string_view(kNodes[b].text))
                    return false;
    return true;
}

// An alias must not shadow a real row, or be written twice.
//
// The shadowing case is the one to fear: an alias is tried BEFORE the segment
// match, so an alias text equal to a sibling's text would silently answer for a
// node that has its own number, and the numbering would still be internally
// consistent while a path meant something else.
constexpr bool aliases_do_not_shadow()
{
    for (size_t i = 0; i < kAliasCount; i++) {
        if (kAliases[i].of == NodeId::NONE)
            return false;
        const NodeId under = parent_of(kAliases[i].of);
        const std::string_view text = kAliases[i].text;
        for (PathId c = kChildren.first[static_cast<PathId>(under)]; c != kNoPath;
             c = kChildren.next[c])
            if (std::string_view(kNodes[c].text) == text)
                return false;
        for (size_t j = 0; j < i; j++)
            if (parent_of(kAliases[j].of) == under &&
                std::string_view(kAliases[j].text) == text)
                return false;
    }
    return true;
}

} // namespace satellite::words::detail

namespace satellite::words {

static_assert(detail::enum_matches_table(),
              "words.def: NodeId and kNodes are different lengths -- check the "
              "hand-written NONE, COUNT_ and row 0, which are the only parts "
              "the X-macro does not write");
static_assert(detail::kinds_are_distinct(),
              "words.def: SAT_NUMBERED and SAT_BARE are the same value, so a "
              "bare shape cannot be told from a numbered child");
static_assert(detail::one_root(),
              "words.def: a node other than `satellite` has no parent -- DESIGN "
              "§1 says every language-owned name is rooted at `satellite`");
static_assert(detail::parents_come_first(),
              "words.def: a row names a parent declared below it -- the "
              "numbering is one forward pass and cannot count a parent it has "
              "not reached");
static_assert(detail::no_cycles(),
              "words.def: a node is its own ancestor (PLAN M2, property 3)");
static_assert(detail::one_bare_shape_each(),
              "words.def: a node has two SAT_BARE children -- there is one way "
              "to call something with no arguments, so there is one number 0");
static_assert(detail::bare_rows_are_spelled_bare(),
              "words.def: a row's kind and its text disagree -- SAT_BARE and "
              "the text \"()\" both mean position 0 and must always agree");
static_assert(detail::spellings_match_texts(),
              "words.def: a row's text is empty, or a row beginning with `(` "
              "was given a spelling, or a word was left without one");
static_assert(detail::sibling_texts_are_distinct(),
              "words.def: two children of one parent have the same text -- the "
              "second holds a number nothing can walk to");
static_assert(detail::aliases_do_not_shadow(),
              "words.def: a SAT_ALIAS repeats a sibling's text or another "
              "alias -- an alias is tried first, so it would answer for a node "
              "that has its own number");
static_assert(place_parameter_of(NodeId::CONSOLE_INPUT_PROMPT_TARGET) == 1 &&
                  place_parameter_of(NodeId::CONSOLE_INPUT_PROMPT) ==
                      kNoPlaceParameter,
              "words.def: the place list (M14) names `input(prompt, target)`'s "
              "second written argument and nothing else -- one row by policy, "
              "and the compiler's slot road depends on the index");

// WHAT THESE DO NOT COVER, said here rather than left to be assumed.
//
// EVERY ASSERT ABOVE IS ABOUT THE FROZEN HALF. A user's capsules and spacesuits
// are numbered at parse time, under the node that owns them, as they are met
// (WORD_NUMBERS §3, DESIGN §4.3, PLAN §8.1) -- and no compiler can see them.
// words_runtime.hpp is that half, and the guarantee it carries is weaker on
// purpose: a user name's number is stable inside one run and not between two,
// so anything that writes a PathId down has to record the name instead.
//
// Nor do they check the transcription. Every property above would hold of a
// words.def that had a row missing, because both files would still be
// internally consistent -- WORD_NUMBERS.md is the authority and the only thing
// that can catch it is a comparison against WORD_NUMBERS.md, which is
// tests/words_test.

} // namespace satellite::words
