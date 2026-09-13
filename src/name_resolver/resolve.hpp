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
// NOTHING BOUNDS THE DEPTH OF THIS WALK, WHICH IS THE RULE AND NOT AN OVERSIGHT.
// DESIGN §7.5: the language has no depth limit. M7 shipped a fixed
// `kMaxDepth = 2000` with errors.def's S0501 behind it; the author's answer was
// that an interpreter which stops at a depth is broken rather than bounded, and
// both were deleted on 2026-08-31. `SCRATCH.md/NO_LIMITS.md` is the record.
//
// WHAT HOLDS IT UP TODAY IS THE STACK satl ASKS FOR. machine_limits/limits.hpp
// raises RLIMIT_STACK at startup to a share of what the machine has -- 32 KiB
// for every MiB, which is 1.9 GiB here -- and M6's watchdog refuses in words
// before that is reached whenever MEMORY_MAX is set below it, because touched
// stack pages are resident memory. That is a very large number and not the
// absence of one -- DESIGN §7.5.1 says so, and a walker keeping its own stack
// on the heap is what finally makes the rule true.

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

// The name is a FIELD of the spacesuit whose method we are inside -- M26.
// `Info::member` is which one, by index into the suit's field list.
//
// A FOURTH SENTINEL AND NOT A FRAME SLOT, and the difference is the milestone.
// A frame slot is this activation's storage: private, fresh per call, gone when
// the call returns (DESIGN §7.2). A field is the OBJECT's storage: shared by
// every call on that object, and outliving all of them. Reading `n` inside a
// method is therefore not a local read at all -- it is a read through the
// receiver, which lives at slot 0 of every method's frame.
inline constexpr Slot kSlotField = -4;

// The name is a SPACESHIP this file includes -- M25, 2026-09-13. `ship` in
// `ship.setup()` and `ship.box b`: not storage and not a capsule, but the file
// whose names the next segment is looked up among. `Info::path` is its node,
// `satellite.library.<ship>`.
inline constexpr Slot kSlotSpaceship = -5;

constexpr bool in_a_frame(Slot slot) { return slot >= 0; }

// ONE SPACESHIP, AS A FILE THAT INCLUDES IT SEES IT -- M25. The name the
// include wrote, the node its names are numbered under, and which file of the
// run it is. `node` is kNoPath when nothing loaded the file, which is what a
// single-file arm like `satl --resolve` sees: the name is still a spaceship's,
// and what is inside it is simply not known there.
struct Spaceship {
    std::string_view name;
    words::PathId node = words::kNoPath;
    uint32_t file = 0;
};

// ONE FILE OF A RUN -- M25. File 0 is the one satl was given; every other is a
// spaceship, whose names are numbered under its own node rather than under
// `satellite.library` itself.
struct File {
    const Ast *ast = nullptr;
    words::PathId library = static_cast<words::PathId>(words::NodeId::LIBRARY);
    std::vector<Spaceship> ships;
};

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

    // WHICH FIELD, when `slot` is kSlotField -- M26. Meaningless otherwise, and
    // it is a separate word rather than a reuse of `path` because a PathId and
    // an index are both uint32_t and this tree has a page in lexer.hpp about
    // what that costs when two of them share a field.
    uint32_t member = 0;

    // How `path` was arrived at. MILESTONES/M4.5.md §5 is what asks for this.
    Origin origin = Origin::Parsed;

    // DESIGN §7.7's object: `satellite.main`'s parameter, under any of its six
    // spellings, and the members read off it.
    bool arguments = false;

    // WORD_NUMBERS §1.5's LITERAL OPTION, FOLDED INTO THE NUMBER -- set on
    // the selector of `my_list.sort("down")`, which is `sort_down()`
    // `1 4 2 5` and takes no written argument at all.
    //
    // IT IS THE ABSORBED ARGUMENT ONE FORM ALONG, and it needs its own flag
    // for the reason the fold's own rule creates: `satellite.include`'s
    // absorber is recognisable from the numbering alone (the row's argument
    // list names a reserved word), while a fold is recognisable only from
    // the fact that the fold RAN -- `sort("up")` lands on `sort()`, whose
    // spelling is the word the source already wrote, so nothing about the
    // resolved path can tell the compiler that a string was consumed.
    //
    // BUILT AT M16, WHICH IS THE FOLD'S FIRST CONSUMER. M7 built the fold
    // and nothing could reach it: the only word in the numbering with
    // options is `sort`, and there were no lists until this milestone. So a
    // folded call had never once been compiled, and the argument it absorbs
    // had never once had to be dropped.
    bool folded_option = false;
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

    // THE SPACESUIT THIS IS A METHOD OF, or kNoPath for an ordinary capsule --
    // M26. When it is set, SLOT 0 IS THE RECEIVER and the written parameters
    // start at 1, which is DESIGN §6.4's "methods are sugar" made structural:
    // the section writes the receiver out as the first argument, so a method's
    // frame is a capsule's frame with that argument really there.
    //
    // THE RECEIVER'S NAME IS UNSPELLABLE ON PURPOSE. It is stored so the dump
    // has something to print, and it is not a word the lexer can produce, so
    // no program can reach slot 0 by writing its name -- DESIGN §12 defers bare
    // field access and this is the one place that could have quietly granted it.
    words::PathId suit = words::kNoPath;

    size_t size() const { return names.size(); }
};

// ONE SPACESUIT, RESOLVED -- M26, and this is pass 2's output where there used
// to be a counter.
//
// A FIELD IS A SLOT AND A METHOD IS A PATH, which is the one sentence this
// whole structure is. DESIGN §7.2's argument for frames applies to a
// spacesuit's storage word for word -- reached by INDEX, decided before
// anything runs, never looked up by name at run time -- and DESIGN §7.6's
// argument for capsules applies to its methods: they live in their own table
// and are called by number. So the two halves of a suit are the two halves the
// language already had, and neither needed a new mechanism.
//
// THE FIELD ORDER IS THE DECLARATION ORDER ACROSS EVERY SECTION, flattened. A
// suit with a protected block, then a public one, then another protected one
// numbers its fields 0, 1, 2 in the order they are written and not in the order
// of the sections -- because a section is about ACCESS and not about layout,
// which is what PLAN §8's M26 entry means by "blocks inside the suit, not
// modifiers on a member".
struct Field {
    std::string_view name;
    words::PathId type = words::kNoPath;
    NodeIndex at = kNoNode;
    NodeIndex init = kNoNode;   // the declared initialiser, run at construction
    bool is_public = false;
};

struct Method {
    words::PathId path = words::kNoPath;
    std::string_view name;
    NodeIndex node = kNoNode;
    bool is_public = false;
};

struct Suit {
    words::PathId path = words::kNoPath;
    std::string_view name;
    NodeIndex node = kNoNode;

    // THE SUIT THIS ONE EXTENDS, AND HOW MUCH OF `fields` AND `methods` CAME
    // FROM IT -- M26. A child's layout is its parent's layout followed by its
    // own, which is the whole of what inheritance costs here: because the
    // parent's fields keep the INDICES THEY HAD, every op the parent's methods
    // already compiled reads the right slot of a child object without knowing
    // one exists. That is the author's sentence -- "a spacesuit can hold a copy
    // of its superclass" -- as a memory layout rather than as a lookup.
    //
    // AND THE TWO COUNTS ARE WHAT TELL A SUIT'S OWN MEMBERS FROM ITS BORROWED
    // ONES, which two passes need and neither can recompute. resolve's pass 4
    // and the compiler's pass 4 both walk every suit's method list, and an
    // inherited method must be resolved and compiled ONCE -- against the suit
    // that declared it, whose layout its field indices belong to. Walking from
    // `inherited_methods` is how each pass skips what it has already done.
    words::PathId parent = words::kNoPath;
    uint32_t inherited = 0;
    uint32_t inherited_methods = 0;

    // The inherited members first, then this suit's own. See above.
    std::vector<Field> fields;
    std::vector<Method> methods;

    // THE SEARCH RUNS BACKWARDS, AND THAT IS THE SHADOWING RULE -- M26.
    // The inherited members sit at the front, so the LAST match is the
    // most-derived one: a child that declares a field its parent also declares
    // gets its own, and the parent's methods go on reading the parent's copy at
    // the index they always used. All eleven spacesuits in the author's own
    // infinity_data_main.satl declare `spacesuit_name`, two of them in a
    // parent-and-child pair, so a forwards search would have silently handed
    // the child its parent's name -- which is the one outcome worse than a
    // refusal, because the program keeps running and prints the wrong thing.
    const Field *field_named(std::string_view spelling) const
    {
        for (size_t i = fields.size(); i > 0; i--)
            if (fields[i - 1].name == spelling)
                return &fields[i - 1];
        return nullptr;
    }

    const Method *method_named(std::string_view spelling) const
    {
        for (size_t i = methods.size(); i > 0; i--)
            if (methods[i - 1].name == spelling)
                return &methods[i - 1];
        return nullptr;
    }

    // WHAT THIS SUIT DECLARED ITSELF, which is the question the two
    // "declared twice" checks ask and the only one they may ask. Inheriting a
    // name is not declaring it twice -- it is the shadowing the search above
    // exists to resolve -- so a collision is looked for among a suit's OWN
    // members and never among the ones it was handed.
    const Field *own_field_named(std::string_view spelling) const
    {
        for (size_t i = fields.size(); i > inherited; i--)
            if (fields[i - 1].name == spelling)
                return &fields[i - 1];
        return nullptr;
    }

    const Method *own_method_named(std::string_view spelling) const
    {
        for (size_t i = methods.size(); i > inherited_methods; i--)
            if (methods[i - 1].name == spelling)
                return &methods[i - 1];
        return nullptr;
    }
};

// A VARIABLE DECLARED IN A CAPSULE BODY, REACHABLE FROM OUTSIDE IT -- M26, and
// the author's sentence is the specification: "satellite.library is supposed to
// have access to every capsule, and every variable inside of every capsule like
// this: satellite.library.capsule_name.variable_name."
//
// WHAT IT ANSWERS IS THE DECLARED INITIALISER, AND THERE IS NO OTHER COHERENT
// ANSWER. A capsule's local is a FRAME slot -- it exists per call, DESIGN §7.1
// is emphatic that parameters and locals are not capsule-static, and M23's
// threads mean several calls can be live at once. So there is no single "the"
// value of `memory_vars.target_gb` while the program runs; what there is, is
// the value the declaration WRITES DOWN. That makes a capsule usable as a
// namespace of constants, which is exactly what infinity_data_main.satl line
// 1307 asks of it, and it is evaluated once at startup like any other global.
//
// SO A LATER ASSIGNMENT INSIDE THE CAPSULE DOES NOT MOVE IT. Said here rather
// than discovered: the outside view is the declaration, not the slot.
struct CapsuleConstant {
    words::PathId path = words::kNoPath;   // satellite.library.<capsule>.<name>
    words::PathId type = words::kNoPath;
    NodeIndex declaration = kNoNode;
    NodeIndex initialiser = kNoNode;
};

// WHAT A FILE DECLARES AT ITS TOP, BY NUMBER -- M25. Another file reaching
// `ship.setup` has a PathId from the numbering and needs to know what kind of
// thing it is, which the numbering does not record and should not: a capsule
// is called, a spacesuit is declared, and a global is read through
// `satellite.library`.
enum class Declares : uint8_t { Capsule, Spacesuit, Global };

struct Declared {
    words::PathId path = words::kNoPath;
    Declares kind = Declares::Capsule;
    NodeIndex node = kNoNode;
};

// Everything the pass decided, and everything it could not.
struct Resolved {
    std::vector<Info> nodes;
    std::vector<Frame> frames;
    std::vector<CapsuleConstant> capsule_constants;
    std::vector<errors::Diagnostic> problems;

    // MILESTONES/M4.5.md §5's clause, counted rather than timed. On a 273-byte
    // program the walk is far under what a shell loop can see -- M4.5's own
    // table is what says so -- and a count is exact where a millisecond is not.
    uint32_t walked = 0;
    uint32_t from_cache = 0;

    // WHAT PASS 2 RESOLVED -- M26. This was `uint32_t spacesuits`, a count, from
    // M7 until 2026-09-12: the pass existed, kept its place in DESIGN §7.3's
    // order, resolved nothing, and SAID SO, because "a pass that silently
    // resolved nothing is indistinguishable from one that worked". The count is
    // now `suits.size()` and the honesty is kept by there being something to
    // count.
    std::vector<Suit> suits;

    // Every capsule, spacesuit and global the file declares -- M25.
    std::vector<Declared> declared;

    const Declared *declared_at(words::PathId path) const
    {
        for (const Declared &at : declared)
            if (at.path == path)
                return &at;
        return nullptr;
    }

    const Suit *suit_at(words::PathId path) const
    {
        for (const Suit &at : suits)
            if (at.path == path)
                return &at;
        return nullptr;
    }

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
// `folded` IS THE OTHER HALF OF IT AT M19.6 -- which selectors the file said
// had an option folded into them, so `fold_option()` looks the row up instead
// of working out that there is one to look up.
Resolved resolve(const Ast &ast, words::Words &words,
                 const cache::Marks &marks = cache::Marks(),
                 const cache::Folded &folded = cache::Folded());

// EVERY FILE OF A RUN, RESOLVED TOGETHER -- M25. One answer per file, in the
// order given, and the passes run ACROSS the files rather than one file at a
// time: every file's names, then every file's members, then every file's
// bodies. DESIGN §7.3's forward-reference argument is the reason, one level up
// -- "a capsule may call one defined further down the file", and now in another
// file, which may itself include this one.
std::vector<Resolved> resolve_run(const std::vector<File> &files,
                                  words::Words &words);

// WHAT THE `.satc` WRITER NEEDS OUT OF ALL THIS -- M19.6, and it lives on THIS
// side of the seam because this is the side that may name both types. cache.hpp
// cannot: `resolve()` above takes `cache::Marks`, so a writer that named
// `Resolved` would close the cycle. One conversion in one place, so the two
// callers that need it -- programs/cache_command.cpp and the test that checks
// the round trip -- are asking the same function rather than each keeping three
// lines that could drift.
cache::Folds folds_of(const Resolved &resolved);

} // namespace satellite::resolve
