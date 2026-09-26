#pragma once

// The tables — part of format.hpp, which includes this file. The arity table,
// the selector list and the word-id list, all expanded from format.def.

#include <cstddef>
#include <cstdint>

#include "bytecode_format/format_ids.hpp"

namespace satellite::format {

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
#include "bytecode_format/format.def"
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
#include "bytecode_format/format.def"
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
#include "bytecode_format/format.def"
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

} // namespace satellite::format
