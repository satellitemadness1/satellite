#pragma once

// The orbit_test harness: what every section needs, declared once.
//
// The sections live in orbit_test_phases.cpp and the harness in orbit_test.cpp,
// which is the split every other test in this tree already has -- and the
// reason the per-test source list in 120-tests.mk is a wildcard over the
// folder rather than a spelled-out list.

#include "satellite_orbit_search/orbit.hpp"
#include "evaluator/eval_internal.hpp"

#include <string>
#include <vector>

extern int failures;

void check(bool ok, const std::string &what);

// Building the values a section searches, without a parser. The engine is free
// of the Evaluator class, so a test of it needs no program -- which is the
// contract search.hpp opens with and orbit.hpp repeats.
satellite::ValuePtr str(const char *text);
satellite::ValuePtr yes();
satellite::ValuePtr no();
satellite::ValuePtr list_of(std::vector<satellite::ValuePtr> items);
satellite::ValuePtr map_of(
    std::vector<std::pair<satellite::ValuePtr, satellite::ValuePtr>> entries);

// Is there a finding whose value renders as `want`, and did `phase` make it?
// find_from is the one that proves WHICH phase concluded something, which is
// most of what this test is for: a finding that could have come from any phase
// pins nothing.
const satellite::Finding *find_from(
    const std::vector<satellite::Finding> &findings, const char *want,
    satellite::OrbitPhase phase);
const satellite::Finding *find_any(
    const std::vector<satellite::Finding> &findings, const char *want);

// The memory file every section that touches phase 4 points at, so no test ever
// reads or writes the real $HOME/.satl_orbit. Set before the first resolve,
// because orbit_resolve RECORDS -- every section would otherwise leave a trail
// in the user's own memory.
extern const char *TEST_MEMORY;

void orbit_test_runs();
void orbit_test_accumulates();
void orbit_test_provenance();
void orbit_test_the_example();
void orbit_test_link();
void orbit_test_memory();

// satellite.container.result: the type, the two numbers, and the alternatives.
void orbit_test_result_type();
void orbit_test_result_alternatives();
void orbit_test_result_searchable();

// Layer five: the answer becoming the question, and where that stops.
void orbit_test_settle_chases();
void orbit_test_settle_improves();
void orbit_test_settle_remembers_once();
