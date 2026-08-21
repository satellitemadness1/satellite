#pragma once

// The bytecode format, as C++ — §17. format.def is the data; this header is what
// reads it, and until this file existed nothing did.
//
// WHY A CONSUMER IS THE POINT. format.def opens by promising that the registry,
// the arity table, the id->name strings and the disassembler "all come from ONE
// list and cannot drift apart, which is the whole reason the file exists." A list
// with no consumer cannot deliver that. It is a claim about what a reader will
// notice, and readers did not notice: two sections of DESIGN.md assigned kind 4
// to different things and neither noticed the other; id 56 (`empty`) was frozen
// for a selector no receiver implements; §17.5 pinned the builtin selectors to
// the literal range 38..72 one commit before the map added 73..79.
//
// None of those is a hard problem. All four are the same problem, which is that
// a number lived only in prose. This header expands each list several ways so
// that the C++ compiler is the reader instead:
//
//   - an enum per id space, so a name is spelled once
//   - a switch per id space, where a duplicate id is `error: duplicate case
//     value` and not a silent aliasing of two meanings
//   - constexpr arrays plus static_asserts at the bottom, which is where the
//     structural invariants (dense, ascending, no duplicates, every path segment
//     a defined word) are actually enforced
//
// The static_asserts are in the HEADER rather than in format_test on purpose.
// Every future consumer -- the compiler, the loader, the disassembler -- gets
// them by including this file, so the invariants hold for anyone who reads the
// format, not only for whoever remembers to run the test.
//
// WHAT IS NOT HERE. No Unit type, no encoder, no decoder, no constant pool. This
// header is the registry and nothing else; §17's four-word unit is the next piece
// and it reads its kind tags from here.

#include <cstddef>
#include <cstdint>

namespace satellite::format {

// ---------------------------------------------------------------------------
// The id spaces
// ---------------------------------------------------------------------------

// Scoped enums, so that `EMPTY`, `FILE`, `ERROR`, `TRUE_` and the rest are
// Word::EMPTY and cannot collide with a macro, a libc typedef, or satellite's
// own Number/Value names. The underlying type is the unit's word width.

// Word 0 bits[63:60]. Which space the rest of the unit is read against.
enum class Kind : uint64_t {
#define SAT_KIND(id, ident, text) ident = id,
#include "format.def"
};

// The frozen registry: one id per distinct language-owned word, flat across all
// four segment positions.
enum class Word : uint64_t {
#define SAT_WORD(id, ident, text) ident = id,
#include "format.def"
};

// Machine ops. A SEPARATE space that restarts at 1 and is read only against
// Kind::MACHINE_OP — never against Word.
enum class Op : uint64_t {
#define SAT_OP(id, ident, text, arity) ident = id,
#include "format.def"
};

// ---------------------------------------------------------------------------
// Names, for errors and for the disassembler
// ---------------------------------------------------------------------------
//
// Each of these is a switch over its own list, and that is not merely a
// convenient way to write a lookup. A switch is the cheapest duplicate detector
// C++ has: two rows sharing an id is `error: duplicate case value`, at compile
// time, with both identifiers named. An enum alone accepts the duplicate
// silently — verified, which is why these are switches and not arrays.

constexpr const char *name(Kind k)
{
    switch (k) {
#define SAT_KIND(id, ident, text) case Kind::ident: return text;
#include "format.def"
    }
    return "<unassigned kind>";
}

constexpr const char *name(Word w)
{
    switch (w) {
#define SAT_WORD(id, ident, text) case Word::ident: return text;
#include "format.def"
    }
    return "<unknown word>";
}

constexpr const char *name(Op o)
{
    switch (o) {
#define SAT_OP(id, ident, text, arity) case Op::ident: return text;
#include "format.def"
    }
    return "<unknown op>";
}

// Operand units following the op, per §17's positional decoding.
constexpr unsigned arity(Op o)
{
    switch (o) {
#define SAT_OP(id, ident, text, arity) case Op::ident: return arity;
#include "format.def"
    }
    return 0;
}

// ---------------------------------------------------------------------------
// The tables
// ---------------------------------------------------------------------------

// A complete language path and how many operand units follow it. The four ids
// are exactly the four 64-bit words of the name code, zero for absent segments.
struct Path {
    uint64_t segment[4];
    unsigned arity;
    const char *ident;      // metadata; a stream never carries it
};

inline constexpr Path kPaths[] = {
#define SAT_PATH(ident, s1, s2, s3, s4, arity) {{s1, s2, s3, s4}, arity, #ident},
#include "format.def"
};
inline constexpr size_t kPathCount = sizeof(kPaths) / sizeof(kPaths[0]);

// The arity column's one non-count value, read back from format.def so the
// number lives there and only here reads it. A path carrying this is followed by
// a kind-1 immediate holding the operand count, then by that many operand units.
#ifndef SAT_VARIADIC
#error "format.def did not define SAT_VARIADIC"
#endif
inline constexpr unsigned kVariadic = SAT_VARIADIC;

// Whether the count comes from the stream rather than from the table. A decoder
// must ask this BEFORE trusting `arity`, because on a variadic row `arity` is
// not a count and consuming 255 operand units would run off the end.
constexpr bool is_variadic(const Path &p)
{
    return p.arity == kVariadic;
}

// The words that name a METHOD, and therefore compile to CALL_METHOD rather than
// to a framed call. §17.5 wrote this as the range 38..72; it is a list because
// the set is not contiguous and never was — see format.def.
inline constexpr Word kSelectors[] = {
#define SAT_SELECTOR(ident) Word::ident,
#include "format.def"
};
inline constexpr size_t kSelectorCount = sizeof(kSelectors) / sizeof(kSelectors[0]);

// §17.5: a builtin selector builds NO frame. It is a table jump to native code
// taking register indices, so `+` stays one dispatch and one write, and §17.4's
// frame machinery is for user capsules, which are the only things with frames.
//
// Derived from the list rather than compared against a range, so that adding a
// selector cannot leave this answering wrong — which is exactly what the range
// did when the map landed.
constexpr bool is_selector(Word w)
{
    for (size_t i = 0; i < kSelectorCount; i++)
        if (kSelectors[i] == w)
            return true;
    return false;
}

// Every id the registry defines. Used by the asserts below and by a decoder that
// has to reject a word id no version of the language ever assigned.
inline constexpr uint64_t kWordIds[] = {
#define SAT_WORD(id, ident, text) id,
#include "format.def"
};
inline constexpr size_t kWordCount = sizeof(kWordIds) / sizeof(kWordIds[0]);

constexpr bool is_defined_word(uint64_t id)
{
    for (size_t i = 0; i < kWordCount; i++)
        if (kWordIds[i] == id)
            return true;
    return false;
}

// Positional decoding needs the arity, and the arity is found by the whole
// quadruple: the space is flat, so no single segment identifies a path.
constexpr const Path *find_path(const uint64_t segment[4])
{
    for (size_t i = 0; i < kPathCount; i++) {
        const Path &p = kPaths[i];
        if (p.segment[0] == segment[0] && p.segment[1] == segment[1] &&
            p.segment[2] == segment[2] && p.segment[3] == segment[3])
            return &p;
    }
    return nullptr;
}

// ---------------------------------------------------------------------------
// The invariants — enforced here, so every consumer inherits them
// ---------------------------------------------------------------------------
//
// ast.hpp:50-54 states the principle these exist to serve: "A printed number
// that nobody compares is not a budget." Everything below is a number this
// format has been running on trust.

namespace detail {

template <size_t N>
constexpr bool all_distinct(const uint64_t (&ids)[N])
{
    for (size_t i = 0; i < N; i++)
        for (size_t j = i + 1; j < N; j++)
            if (ids[i] == ids[j])
                return false;
    return true;
}

template <size_t N>
constexpr bool ascending(const uint64_t (&ids)[N])
{
    for (size_t i = 1; i < N; i++)
        if (ids[i] <= ids[i - 1])
            return false;
    return true;
}

// A ceiling on the VALUES, which is not the same thing as a ceiling on how many
// there are — and the difference is not academic. This predicate replaces an
// assert that read `sizeof(kKindIds) / sizeof(uint64_t) <= 16` under the message
// "a kind is four bits". That expression counts ROWS. Seven rows numbered
// {0,1,2,3,4,5,99} satisfy it, satisfy `ascending`, satisfy `all_distinct`, and
// encode a kind that does not fit the four bits word 0 gives it — checked, and
// the build was silent.
//
// It is left in as a named predicate rather than folded into the assert because
// a bound stated as arithmetic on a sizeof is exactly what hid the bug.
template <size_t N>
constexpr bool all_within(const uint64_t (&ids)[N], uint64_t ceiling)
{
    for (size_t i = 0; i < N; i++)
        if (ids[i] > ceiling)
            return false;
    return true;
}

// Dense from `first`, allowing exactly the gaps listed. §17 holds 18..20 for
// break and continue, and that reservation is the only hole the registry may
// have — any other is a typo that would silently freeze a wrong id.
template <size_t N>
constexpr bool dense_except_reserved(const uint64_t (&ids)[N], uint64_t first,
                                     uint64_t gap_lo, uint64_t gap_hi)
{
    uint64_t expected = first;
    for (size_t i = 0; i < N; i++) {
        if (expected == gap_lo)
            expected = gap_hi + 1;
        if (ids[i] != expected)
            return false;
        expected++;
    }
    return true;
}

constexpr uint64_t kKindIds[] = {
#define SAT_KIND(id, ident, text) id,
#include "format.def"
};
constexpr uint64_t kOpIds[] = {
#define SAT_OP(id, ident, text, arity) id,
#include "format.def"
};

constexpr bool paths_reference_defined_words()
{
    for (size_t i = 0; i < kPathCount; i++)
        for (int s = 0; s < 4; s++)
            if (kPaths[i].segment[s] != 0 && !is_defined_word(kPaths[i].segment[s]))
                return false;
    return true;
}

// A name code is four words with absent segments zeroed at the END. {1,0,7,0}
// would decode as a three-segment path with a hole and is not a path at all.
constexpr bool paths_are_left_packed()
{
    for (size_t i = 0; i < kPathCount; i++) {
        bool ended = false;
        for (int s = 0; s < 4; s++) {
            if (kPaths[i].segment[s] == 0)
                ended = true;
            else if (ended)
                return false;
        }
    }
    return true;
}

// §1: a language-owned name is a dotted path ROOTED AT `satellite`. That is the
// closure property the whole registry rests on, so it is checked rather than
// assumed — the previous form of this test only rejected a leading zero, which
// let {38,0,0,0} through: a bare selector's own name code, the one shape §17.5
// says can never have an arity row.
constexpr bool paths_are_rooted()
{
    for (size_t i = 0; i < kPathCount; i++)
        if (kPaths[i].segment[0] != static_cast<uint64_t>(Word::SATELLITE))
            return false;
    return true;
}

// The quadruple is the KEY, and find_path returns the first match. Two rows
// sharing all four segments would silently shadow one another: the second loses
// its arity and the first answers for both, which is a decoder reading the wrong
// number of operand units and then resuming at a boundary that is not one.
constexpr bool paths_distinct()
{
    for (size_t i = 0; i < kPathCount; i++)
        for (size_t j = i + 1; j < kPathCount; j++) {
            bool same = true;
            for (int s = 0; s < 4; s++)
                if (kPaths[i].segment[s] != kPaths[j].segment[s])
                    same = false;
            if (same)
                return false;
        }
    return true;
}

// The arity column holds a count, or the one sentinel that means "the count is
// in the stream". Anything above the sentinel is a row that read_path would hand
// a decoder as a number of operand units to consume, so the boundary is stated
// rather than trusted.
constexpr bool arities_are_in_band()
{
    for (size_t i = 0; i < kPathCount; i++)
        if (kPaths[i].arity > kVariadic)
            return false;
    return true;
}

constexpr bool selectors_are_words()
{
    for (size_t i = 0; i < kSelectorCount; i++)
        if (!is_defined_word(static_cast<uint64_t>(kSelectors[i])))
            return false;
    return true;
}

constexpr bool selectors_distinct()
{
    for (size_t i = 0; i < kSelectorCount; i++)
        for (size_t j = i + 1; j < kSelectorCount; j++)
            if (kSelectors[i] == kSelectors[j])
                return false;
    return true;
}

} // namespace detail

// The kind space. This is the assert that would have caught the collision: two
// sections of DESIGN.md each extended a four-row markdown table and both chose
// 4, and nothing objected because markdown does not compile.
static_assert(detail::all_distinct(detail::kKindIds),
              "format.def: two kinds share an id — see SAT_KIND");
static_assert(detail::ascending(detail::kKindIds),
              "format.def: SAT_KIND rows must ascend");
static_assert(detail::kKindIds[0] == 0,
              "format.def: kind 0 is the name code and must stay 0");
static_assert(detail::all_within(detail::kKindIds, 15),
              "format.def: a kind is four bits — no kind id may exceed 15");

// The registry. Never renumbered, never reused, always appended, with 18..20
// held open for break and continue.
static_assert(detail::all_distinct(kWordIds),
              "format.def: two words share an id — ids are frozen, never reused");
static_assert(detail::ascending(kWordIds),
              "format.def: SAT_WORD rows must ascend — append, never insert");
static_assert(detail::dense_except_reserved(kWordIds, 1, 18, 20),
              "format.def: the registry has a hole other than the reserved 18..20");

// Machine ops: their own space, dense from 1.
static_assert(detail::all_distinct(detail::kOpIds),
              "format.def: two machine ops share an id — see SAT_OP");
static_assert(detail::ascending(detail::kOpIds),
              "format.def: SAT_OP rows must ascend");
static_assert(detail::kOpIds[0] == 1,
              "format.def: the machine-op space restarts at 1, not at 0");
// "Dense from 1" was a claim in a comment until this line. gap_lo == gap_hi == 0
// degenerates to plain density, because `expected` starts at 1 and never reaches
// 0 — the machine ops have no reserved block the way the registry does.
static_assert(detail::dense_except_reserved(detail::kOpIds, 1, 0, 0),
              "format.def: the machine-op space has a hole — ops are dense from 1");

// The arity table is the grammar of the format (§17), so a malformed row is a
// stream that cannot be decoded.
static_assert(detail::paths_reference_defined_words(),
              "format.def: a SAT_PATH segment names an id no SAT_WORD defines");
static_assert(detail::paths_are_left_packed(),
              "format.def: a SAT_PATH has a zero segment before a nonzero one");
static_assert(detail::paths_are_rooted(),
              "format.def: a SAT_PATH is not rooted at `satellite` (§1)");
static_assert(detail::paths_distinct(),
              "format.def: two SAT_PATH rows share a quadruple — the second is unreachable");

// The variadic marker. `kVariadic != 0` is the whole point of choosing 255: 0 is
// a REAL arity, and P_HELP carried it for three commits meaning "no operands"
// when the truth was "the table cannot say." A sentinel that is also a valid
// value cannot be distinguished from one, which is how that went unnoticed.
static_assert(kVariadic != 0,
              "format.def: SAT_VARIADIC collides with a real arity — 0 means "
              "a path that genuinely takes no operands");
static_assert(detail::arities_are_in_band(),
              "format.def: a SAT_PATH arity exceeds SAT_VARIADIC");
// The count unit's kind is part of the marker's contract, so it is pinned here
// rather than described. A decoder that reads the count against any other space
// reads a number that is not the count.
static_assert(static_cast<uint64_t>(Kind::IMMEDIATE) == 1,
              "format.def: a variadic path's count unit is a kind-1 immediate");

// Selectors are cross-referenced by identifier, so the compiler already rejects
// a name that is not a word; these catch a duplicated row and keep the
// is_defined_word path honest.
static_assert(detail::selectors_are_words(),
              "format.def: a SAT_SELECTOR names something that is not a word");
static_assert(detail::selectors_distinct(),
              "format.def: a word appears twice in the selector list");

} // namespace satellite::format
