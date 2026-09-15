#pragma once

// The entries, as a table indexed by PathId -- PLAN M18.
//
// A ROW OF help.def IS A ROW OF words.def, AND THE COMPILER IS WHAT SAYS SO.
// help.def is generated in words.def order, one entry per node, so entry i
// belongs to PathId i and the lookup is an array index with no search in it --
// the same arrangement words_nodes.hpp has for the nodes themselves. That is a
// claim about a generated file, which is exactly the kind of claim that goes
// quietly wrong, so it is not left as a comment: kOrderIsRight below compares
// the two lists element by element at compile time and a mismatch is a build
// error naming this header.
//
// WHICH IS THE WHOLE OF DESIGN §4.6, MECHANISED. §4.6 says help "cannot drift
// from what exists"; a word appended to the registry with no entry written for
// it is precisely that drift, and here it is a compile error the day the word
// lands rather than a blank page somebody finds in six months.
//
// THE TEXT IS COMPILED IN AND NOT READ FROM A FILE, and that is the same
// decision as errors.def's sentences one layer up. A help that opened a
// document at run time could be asked on a machine where the document is
// missing, is a different version, or has been edited -- and "the language's
// own account of itself" that can disagree with the language is v1's defect
// with an extra failure mode bolted on. 90KB of string literals in a 3.8MB
// binary is what that costs, measured, and MILESTONES/M18.md carries the number.

#include "satellite_words/words.hpp"

#include <cstddef>

namespace satellite::help {

using words::NodeId;
using words::PathId;

// One node's help.
//
// `head` IS THE GROUP AND IT IS THE ONLY FIELD THAT IS NOT PROSE. Asking about
// a path answers for that node and the shapes written beside it together --
// `satellite.help(satellite.console.input)` brings up `input()`,
// `input(prompt)` and `input(prompt, target)`, which are three sibling nodes
// and not a parent and its children. What they share is the `>` line the entries
// were written under, and that line, walked to the node it names, is this field.
struct Entry {
    NodeId head;
    const char *prose;
    const char *example;
};

// Row 0 is not a node, so kEntries[id] needs no arithmetic -- words_nodes.hpp's
// kNodes trick, and it has to be the same trick or the two tables would need
// different indices for the same word.
inline constexpr Entry kEntries[] = {
    {NodeId::NONE, "", ""},
#define SAT_HELP(ident, head, prose, example) {NodeId::head, prose, example},
#include "satellite_help/help.def"
};

// The same list again, as the node each row CLAIMS to be about. Only
// kOrderIsRight reads it, and it exists so that the claim can be checked
// instead of trusted.
inline constexpr NodeId kAbout[] = {
    NodeId::NONE,
#define SAT_HELP(ident, head, prose, example) NodeId::ident,
#include "satellite_help/help.def"
};

inline constexpr size_t kEntryCount = sizeof(kEntries) / sizeof(kEntries[0]) - 1;

// Every row is the node at its own index, and every head is its own head.
//
// THE SECOND HALF IS WHAT MAKES THE GROUPS A PARTITION. A group is "every node
// whose head is this one", so a head that pointed at a node with a different
// head again would put an entry in a group nothing can ask for -- reachable in
// the document and unreachable in the language. One compare rules it out for
// all 269 at once.
constexpr bool order_is_right()
{
    if (kEntryCount != words::kNodeCount)
        return false;
    for (PathId i = 1; i <= kEntryCount; i++) {
        if (static_cast<PathId>(kAbout[i]) != i)
            return false;
        const PathId head = static_cast<PathId>(kEntries[i].head);
        if (head == 0 || head > kEntryCount)
            return false;
        if (kEntries[head].head != kEntries[i].head)
            return false;
    }
    return true;
}

static_assert(order_is_right(),
              "help_text.hpp: help.def must carry exactly one row per node of "
              "words.def, in words.def order, and every head must be its own "
              "head. Regenerate it with `python3 help_lines/gen.py`");

constexpr const Entry &entry_of(PathId id)
{
    return kEntries[id <= kEntryCount ? id : 0];
}

// Whether two nodes are asked about together.
constexpr bool same_group(PathId a, PathId b)
{
    return entry_of(a).head == entry_of(b).head;
}

constexpr PathId head_of(PathId id)
{
    return static_cast<PathId>(entry_of(id).head);
}

} // namespace satellite::help
