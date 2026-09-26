#pragma once

// The id spaces and their names — part of format.hpp, which includes this file.
// See format.hpp for why a consumer of format.def is the point.

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
#include "bytecode_format/format.def"
};

// The frozen registry: one id per distinct language-owned word, flat across all
// four segment positions.
enum class Word : uint64_t {
#define SAT_WORD(id, ident, text) ident = id,
#include "bytecode_format/format.def"
};

// Machine ops. A SEPARATE space that restarts at 1 and is read only against
// Kind::MACHINE_OP — never against Word.
enum class Op : uint64_t {
#define SAT_OP(id, ident, text, arity) ident = id,
#include "bytecode_format/format.def"
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
#include "bytecode_format/format.def"
    }
    return "<unassigned kind>";
}

constexpr const char *name(Word w)
{
    switch (w) {
#define SAT_WORD(id, ident, text) case Word::ident: return text;
#include "bytecode_format/format.def"
    }
    return "<unknown word>";
}

constexpr const char *name(Op o)
{
    switch (o) {
#define SAT_OP(id, ident, text, arity) case Op::ident: return text;
#include "bytecode_format/format.def"
    }
    return "<unknown op>";
}

// Operand units following the op, per §17's positional decoding.
constexpr unsigned arity(Op o)
{
    switch (o) {
#define SAT_OP(id, ident, text, arity) case Op::ident: return arity;
#include "bytecode_format/format.def"
    }
    return 0;
}

} // namespace satellite::format
