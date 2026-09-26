#pragma once

#include "satellite_value/value_variant.hpp"

namespace satellite {

// 40 bytes is the figure §8.1 quotes when it records that removing `double`
// for a 32-byte Number left sizeof(Value) unchanged, and the one §10 quotes
// when it refuses to fold Expr into Value — that fold takes every Value to 96,
// and a million-element number list from 38 MB to 91 MB. Both claims were
// checked by reading a printed banner line, which is not a guard. This is.
// Guarded on 64-bit for the reason given at Span in ast.hpp: the Debian
// packages are Architecture: any and 32-bit layouts differ legitimately.
static_assert(sizeof(void *) != 8 || sizeof(Value) == 40,
              "Value must stay 40 bytes on 64-bit — §8.1's migration and §10's "
              "Expr/Value split both rest on this number");

// ---------------------------------------------------------------------------
// §8.7's memory model — the three numbers `.size()` is made of
// ---------------------------------------------------------------------------
//
// These are the 64-bit layout FROZEN AS THE DEFINITION, which is the same move
// the static_assert above already makes for sizeof(Value). `.size()` has to
// answer identically on every platform the language is built for: the Debian
// packages are Architecture: any, and §15's stage 3 self-compiles to a
// byte-identical fixpoint — a size that changed with the pointer width would
// make any program that prints one unportable, and any test that asserts one
// unfalsifiable on half the build matrix.
//
// So the model bills three things and nothing else:
//
//   a NODE     40 bytes, once per Value reached
//   a HANDLE   16 bytes, once per slot that points at a Value
//   CONTENT    2 per character, 4 per limb, 1 per byte of a path or a map key
//
// What it does NOT bill is the ALLOCATOR: make_shared control blocks, vector
// capacity beyond its size, hash-table bucket arrays, malloc's rounding. Those
// are real bytes, and leaving them out is the deliberate half of the design —
// they are properties of the C++ runtime a build happened to use, not of the
// program's data, and a `.size()` that moved when libstdc++ changed its
// growth factor would be reporting on the wrong thing.
inline constexpr size_t VALUE_NODE_BYTES = 40;
inline constexpr size_t VALUE_HANDLE_BYTES = 16;
inline constexpr size_t VALUE_CHAR_BYTES = 2;

// The model was DERIVED from the layout, so on the platform it was derived on
// it must still match it. Guarded on 64-bit for the reason the assert above is:
// a 32-bit layout differs legitimately, and there the constants are the
// definition doing its job rather than a layout claim gone stale.
static_assert(sizeof(void *) != 8 ||
                  (VALUE_NODE_BYTES == sizeof(Value) &&
                   VALUE_HANDLE_BYTES == sizeof(ValuePtr) &&
                   VALUE_CHAR_BYTES == sizeof(SatChar)),
              "§8.7's constants have drifted from the layout they model");

// Bytes a value occupies, per §8.7: a node plus what it owns, followed all the
// way down into a list's elements, a map's entries and an object's fields.
//
// Takes the HANDLE rather than the Value, and that is not a convenience. The
// walk dedupes on pointer identity — shared storage is counted once, and §12's
// cyclic object graph terminates — so the TOP node has to enter the set like
// every other one. Passing a bare Value would leave it out, and a value that
// reaches itself would then be counted twice before the cycle closed.
size_t value_bytes(const ValuePtr &value);

} // namespace satellite
