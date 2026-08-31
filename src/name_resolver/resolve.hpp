#pragma once

// Names to integer frame slots -- PLAN M7. DESIGN §7 is the specification and
// this header is the door onto it.
//
// WHAT THIS FIXES IS THE FIRST SATELLITE'S WORST VERIFIED DEFECT, and §7.1 is
// the receipt: with one global registry keyed by `<capsule>.<variable>`, a
// recursive `fact` returned 1 for EVERY input, and eight threads running a
// capsule with no recursion and no shared state produced 1585 wrong results out
// of 1600. Neither is a bug in the registry -- it is the right storage for
// globals -- and neither can be reframed as deliberate, because parameters are
// locals too and `arguments` would have been a program-wide static. §7.2's fix
// is per-call frames and integer slots decided BEFORE anything runs, and it is
// 7.1x faster besides: 160 ms against 1131 for 200k iterations of fact(15).
//
// THE RESOLVED DATA LIVES IN A SIDE TABLE INDEXED BY NODE, and that is PLAN
// §2.2's decision rather than this milestone's convenience. ast.hpp has held
// the space for it since M4 -- "`Name::slot`, which in the first satellite is a
// `mutable int` on a `shared_ptr<const Expr>` held off by a comment reading
// `resolve() must finish, on one thread, before any evaluation begins`, becomes
// a side table indexed by node index -- so the race is structurally impossible
// instead of documented. M7 builds that side table. M4 must not put a mutable
// field on a node." This is that table, and nothing here writes to the tree.
//
// FOUR PASSES, AND THE ORDER IS §7.3's. Every capsule name first, then every
// spacesuit name, then top-level statements, then each body -- which is what
// makes a forward reference work without a second pass and is why resolve is
// not folded into the parser: "a capsule may call one defined further down the
// file, and mutual recursion is unresolvable in single-pass recursive descent."
//
// PASS 2 IS A NAMED HOLE AND SPACESUITS ARE M26. It counts them, marks their
// declaration node, and resolves nothing inside -- and `satl --resolve` says so
// out loud, because a pass that silently resolved nothing would be
// indistinguishable from one that worked.
//
// THE BOUND BELOW IS WITHDRAWN AND HAS NOT BEEN REMOVED YET. The author's rule
// as of 2026-08-31 is that the language has NO depth limit -- DESIGN §7.5 was
// rewritten and PLAN §2.5 un-deferred the same day -- so `kMaxDepth`, `Depth`,
// `too_deep()` and errors.def's S0501 all go when this pass keeps its own stack
// on the heap instead of using the C++ one. `SCRATCH.md/NO_LIMITS.md` §5.1 is
// the plan. Everything below is what the file said when the bound was the
// decision, kept until the code catches up rather than edited into a claim the
// code does not keep.
//
// AND THE RECURSION BOUND HERE IS NOT DESIGN §7.5's. §7.5 sits inside §7 and
// reads as this milestone's; it is not. It bounds a program that is RUNNING,
// its ceiling is derived from RLIMIT_STACK, and `system_facts/facts.hpp`
// already says in its own words that M9's ceiling comes from there. What is
// bounded below is the RESOLVER's own C++ stack, which a deeply nested
// expression smashes while nothing is running at all. Two bounds, two
// milestones, one section of DESIGN.

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace satellite::resolve {

// A frame slot, or one of three things that is not one.
//
// THREE SENTINELS AND NOT SIX. The M6 draft in prototype/ defines six --
// GLOBAL, CAPSULE, METHOD, SUIT, FIELD and ARGUMENTS -- and three of them are
// spacesuits' (M26) while the fourth, ARGUMENTS, is never assigned by that
// draft at all: `arguments` is a parameter, so it takes a real slot like every
// other parameter, and the flag saying WHICH parameter it is rides beside the
// slot rather than replacing it. A sentinel with no producer is a case every
// later reader has to rule out; M5 carries `FrameRef` with no producer for a
// reason it argues at length, and this is not that reason.
using Slot = int32_t;

// Not in a frame: a top-level statement, or a name the registry owns.
inline constexpr Slot kSlotGlobal = -1;

// The name is a capsule this file declares. DESIGN §7.6: capsules live in their
// own table and cannot even form a legal registry key.
inline constexpr Slot kSlotCapsule = -2;

// The name is a spacesuit this file declares. What is INSIDE one is M26's.
inline constexpr Slot kSlotSpacesuit = -3;

constexpr bool in_a_frame(Slot slot) { return slot >= 0; }

// WHERE A NUMBER CAME FROM, which the dump prints and which is the only way to
// read MILESTONES/M4.5.md §5's clause honestly.
//
// FOUR ANSWERS AND NOT TWO, because "not from the cache" is not the same as
// "walked for" and the difference is what makes the count worth reading. A
// capsule's own number was allocated by the PARSER when it first met the name
// (WORD_NUMBERS §3, and parser.hpp says so), so it is in the tree before this
// pass starts; and a READ of a name that is already bound costs a scan of the
// scope stack and no trie walk at all. Counting either as a walk would inflate
// the number this milestone is answerable for, in the direction that flatters
// it -- MILESTONES/M4.5.md §5's clause is about walks the `.satc` could have
// saved, and neither of those is one.
enum class Origin : uint8_t {
    Parsed,    // the parser interned it -- a capsule or a spacesuit DECLARATION
    Bound,     // a READ of a name whose declaration already knew the number
    Walked,    // this pass read it out of the trie
    Cached,    // the `.satc` already had it and the walk was skipped
};

// What resolve decided about one node. Indexed by NodeIndex; see the header
// note for why this is a side table and not a field.
struct Info {
    Slot slot = kSlotGlobal;

    // The language path this node names, if it names one -- the trie walk
    // DESIGN §6.3 keeps out of the parser and hands to this pass.
    words::PathId path = words::kNoPath;

    // The declared type, as the node the type path ends at. On a VarDecl it is
    // what was written; on a Name it is copied from the slot's declaration, and
    // that ONE HOP is what makes WORD_NUMBERS §1.5's fold reachable -- see
    // names.cpp.
    words::PathId type = words::kNoPath;

    // How `path` was arrived at. MILESTONES/M4.5.md §5 is what asks for this.
    Origin origin = Origin::Parsed;

    // DESIGN §7.7's object: `satellite.main`'s parameter, under any of its six
    // spellings, and the members read off it.
    bool arguments = false;
};

// One capsule's frame -- §7.2's `std::vector<Value> slots`, decided statically.
struct Frame {
    words::PathId capsule = words::kNoPath;
    NodeIndex node = kNoNode;

    // Slots [0, parameters) are the parameter list, in order. Everything after
    // is a local, in the order the bodies declare them.
    uint32_t parameters = 0;

    std::vector<std::string_view> names;
    std::vector<NodeIndex> types;

    // Which slot §7.7's object is in, or -1. Only `satellite.main` has one.
    Slot arguments = -1;

    size_t size() const { return names.size(); }
};

// Everything the pass decided, and everything it could not.
struct Resolved {
    std::vector<Info> nodes;
    std::vector<Frame> frames;
    std::vector<errors::Diagnostic> problems;

    // MILESTONES/M4.5.md §5's clause, counted rather than timed. On a 273-byte
    // program the walk is far under what a shell loop can see -- M4.5's own
    // table is what says so -- and a count is exact where a millisecond is not.
    uint32_t walked = 0;
    uint32_t from_cache = 0;

    // Pass 2's named hole: how many spacesuits were seen and left to M26.
    uint32_t spacesuits = 0;

    // A DEFAULT AND NOT AN ASSERT for a node nothing decided anything about,
    // which is the same choice ast.hpp makes for node 0 and words.def for path
    // 0: a number literal is an ordinary answer, and a caller that had to check
    // would check at every one of them.
    const Info &at(NodeIndex node) const
    {
        static const Info nothing;
        return node < nodes.size() ? nodes[node] : nothing;
    }

    bool ok() const { return !errors::any_error(problems); }
};

// HOW DEEPLY A PROGRAM MAY BE WRITTEN, which is not how deeply it may recurse.
//
// 2000 is the M6 draft's `MAX_RESOLVE_DEPTH` and it is kept for the reason
// DESIGN §7.5 arrives at the same number for the other bound: one activation of
// this walk is a stack frame, and 2000 of them fit inside an ordinary 8 MiB
// stack with room to spare. Nothing in this tree needs the two to agree and
// nothing makes them: §7.5's is derived from RLIMIT_STACK at run time and is
// M9's, and this one is a constant because a program is written once.
inline constexpr int kMaxDepth = 2000;

// Resolve a parsed program. Never throws, and reports every problem it finds
// rather than the first -- the rule the lexer and the parser already keep.
//
// `words` IS THE RUN'S NUMBERING AND NOT A GLOBAL, for the reason
// words_runtime.hpp gives about M22: a user's PathId is valid inside one run
// only, so the object that allocated the names has to be the one asked about
// them.
//
// `marks` IS WHAT A `.satc` ALREADY KNEW. Empty for a program read from source,
// which is the ordinary case and costs one comparison.
Resolved resolve(const Ast &ast, words::Words &words,
                 const cache::Marks &marks = cache::Marks());

} // namespace satellite::resolve
