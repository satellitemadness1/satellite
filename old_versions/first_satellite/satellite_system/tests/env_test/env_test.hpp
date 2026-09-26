#pragma once

// The shared harness for the env_test binary, and the list of its sections.
//
// env_test.cpp was 546 lines, nearly all of it one main(). It is split into
// one file per group of sections, along the `// --- title ---` dividers the
// test already had, because a resolver test grows a section every time the
// resolver grows a rule and the bottom of a 500-line main() is a bad place to
// keep putting them.
//
// Everything DECLARED here is DEFINED exactly once, in env_test.cpp: the
// failure counter, the check helpers that report through it, and the tree
// walk the slot assertions read their answers out of. Each section file
// defines only the env_test_* functions it owns. Nothing was reworded in the
// move — the section bodies are byte for byte what they were, so a comment
// recording a real bug still sits on the assertion that catches it.
//
// Include this as "env_test.hpp": it is a sibling of the files that include
// it, and -Isrc — which is what makes every OTHER include in this test
// root-relative — does not reach the tests tree.

#include "environment/env.hpp"
#include "syntax_parser/parser.hpp"

#include <string>
#include <utility>
#include <vector>

// Deliberate, and confined to this test. Every section writes Satellite source
// and reads a CapsuleInfo back out of it; qualifying each of those would have
// touched hundreds of lines that are otherwise an exact copy of what they were
// when this test was a single translation unit.
using namespace satellite;

// Counted, not fatal. A failing check prints its own line and the run carries
// on, so one broken rule reports alongside the other fifty instead of hiding
// them behind the first abort. main() turns a non-zero count into exit 1.
//
// It was `static` while this was one file. It is one definition in
// env_test.cpp and an extern declaration here now, which is the only thing
// about it the split changed.
extern int failures;

// No comment in this test writes one of these three names with an open paren
// after it. The number of assertion call sites is guarded by a grep for
// exactly that shape across the folder's .cpp files — it is how a check
// dropped or duplicated during a move gets caught — and a sentence written
// that way would be counted as an assertion that does not exist. Hence the
// prose everywhere here says "the check helpers" and means these three.
void check(bool ok, const std::string &what);

// --- walking the resolved tree ---------------------------------------------
//
// The resolver records slot numbers on the tree rather than returning them, so
// an assertion about slot allocation has to go and read them off the nodes.
// walk() flattens a subtree into (name, slot) pairs in source order, which is
// what makes "the return reads the second x" something a test can state.

using Slots = std::vector<std::pair<std::string, int>>;

void walk(const Expr &expr, Slots &out);
void walk(const Stmt &stmt, Slots &out);

Slots slots_of(const CapsuleInfo &info);

int slot_for(const Slots &slots, const std::string &name);

// --- error helpers ----------------------------------------------------------

bool has_error(const ResolveResult &result, const std::string &fragment);

void check_rejects(const std::string &source, const std::string &fragment,
                   const std::string &what);

void check_accepts(const std::string &source, const std::string &what);

// --- the sections ----------------------------------------------------------
//
// Listed in the order main() calls them, which is the order they ran in when
// they were consecutive blocks of one function. The order carries no meaning —
// no section leaves state behind for the next — but keeping it means a diff
// against the single-file version is a diff of nothing but indentation.

// env_test_slots.cpp
void env_test_slot_allocation();

// env_test_capsule_calls.cpp
void env_test_forward_calls();

// env_test_scoping.cpp
void env_test_lexical_closure();
void env_test_scoping();
void env_test_top_level();

// env_test_binding_errors.cpp
void env_test_reserved_word();
void env_test_duplicates();

// env_test_program.cpp
void env_test_entry_point();
void env_test_recursion_bound();
