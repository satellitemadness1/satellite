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
//
// THE FILE IS AN UMBRELLA. It was one header until it outgrew the line budget;
// it is now three parts included in an order that compiles, and this path and
// its meaning are unchanged. Include this file and you get everything, exactly
// as before. format.def is NOT split -- it is the one permanent exception, and
// each part re-expands the lists it needs.
//
//   format_ids.hpp         the id spaces, their names, and op arity
//   format_tables.hpp      Path, kPaths, kSelectors, kWordIds, and the lookups
//   format_invariants.hpp  namespace detail and the static_asserts

#include <cstddef>
#include <cstdint>

#include "bytecode_format/format_ids.hpp"
#include "bytecode_format/format_tables.hpp"
#include "bytecode_format/format_invariants.hpp"
