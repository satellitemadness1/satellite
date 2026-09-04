#pragma once

// The namespace trie and the path interner -- PLAN M2. words.def is the data;
// this header is what reads it.
//
// WHY A CONSUMER IS THE POINT, and it is the one lesson from the first
// satellite this module exists to not repeat. Its registry opened by promising
// that the id-to-name strings, the arity table and the disassembler "all come
// from ONE list and cannot drift apart" -- and for three commits NOTHING
// INCLUDED IT. A list with no consumer cannot deliver that, because it is a
// claim about what a reader will notice, and four defects accumulated behind it
// in exactly that window. PLAN M2 therefore requires the registry to gain a
// consumer in the milestone that writes it: `satl --words` is that consumer,
// and tests/words_test is the proof.
//
// DESIGN §4 is why the numbering has this shape and WORD_NUMBERS.md is the
// authority over every number in it. This module reads neither at run time
// except in the test, which opens WORD_NUMBERS.md §2.2 and walks all 227 of its
// paths -- because the transcription is the one thing no static_assert can see.
//
// WHAT THIS IS FOR. The numbering is satellite's bytecode (DESIGN §4): the job a
// bytecode does -- give every operation a small integer so dispatch is an array
// index rather than a comparison -- is the job the numbering does, assigned to
// the NAMESPACE once and permanently instead of to an instruction stream. So
// the walk below happens once, when a source is read, and everything after it
// holds one uint32_t and dispatches through handlers[path_id]. The first
// satellite had the table and still joined paths into heap strings and ran a
// chain of string compares. The table was the bytecode and nothing read it.
//
// NOTHING HERE EXECUTES ANYTHING. There are no handlers at M2 and the word
// `handlers` appears in this module only in comments. This is the spine.
//
// THE FILE IS AN UMBRELLA, which PLAN §3 names as a legitimate answer to the
// 300-line ceiling: one door, whose parts are included in an order that
// compiles, so every consumer's include line stays the same. words.def is NOT
// split -- it is the permanent exception, and each part re-expands the lists it
// needs.
//
//   words_nodes.hpp        NodeId, PathId, Node, kNodes, kAliases, reading a text
//   words_numbers.hpp      the numbers, derived from file order at compile time
//   words_spellings.hpp    the spelling interner, and every node's children
//   words_walk.hpp         a path, read against the trie
//   words_invariants.hpp   the static_asserts, so every consumer inherits them
//   words_digest.hpp       the numbering's identity, for a `.satc` header
//   words_runtime.hpp      the user's half: names numbered at parse time
//
// The parts are included in dependency order and each includes what it needs on
// its own, so any one of them can be included alone. This file is the door
// because a consumer should not have to know which part holds what.

#include "satellite_words/words_nodes.hpp"
#include "satellite_words/words_numbers.hpp"
#include "satellite_words/words_spellings.hpp"
#include "satellite_words/words_walk.hpp"
#include "satellite_words/words_invariants.hpp"
#include "satellite_words/words_digest.hpp"
#include "satellite_words/words_runtime.hpp"
