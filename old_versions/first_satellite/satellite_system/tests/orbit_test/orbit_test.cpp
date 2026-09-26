// Satellite Orbit — the five phases, and what each of them is allowed to claim.
//
// Four things this file exists to pin, because each is a rule that would
// otherwise be true only by accident:
//
//   1. THE PHASES ACCUMULATE. orbit.txt says guess builds "up on the direct
//      search, adding to the end result", and DECISION 2 records why the
//      obvious optimisation is wrong: a pipeline that exits at the first hit
//      hands frequency one unopposed candidate it can only rate 100%. If
//      anyone ever adds an early return, the accumulation tests below fail.
//   2. EVERY FINDING CARRIES ITS PHASE AND ITS REASON. DECISION 6, and it is
//      the condition this feature exists under: a phase-1 finding is a fact and
//      a phase-3 force finding is the engine's own proposal, and a user who
//      cannot tell them apart is being acted upon without being told.
//   3. MEMORY PROPOSES, IT NEVER INVENTS. A record whose value is not in the
//      corpus in front of us must produce nothing, or a result's `value` would
//      be something the searched structure does not contain.
//   4. orbit.txt's OWN EXAMPLE RESOLVES. {T,T,F,T,F,T,T} searched for {T,T},
//      with {T,F} reachable as a constructed alternative -- which is the shape
//      of answer the whole design was asked for.
//
// It runs the engine directly and never through a program, which is the
// contract search.hpp opens with and orbit.hpp repeats: the power is free of
// the Evaluator class, so the whole of it is testable without parsing anything.

#include "orbit_test.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace satellite;

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

ValuePtr str(const char *text) { return make_value(encode_raw(text)); }
ValuePtr yes() { return make_value(Value(true)); }
ValuePtr no() { return make_value(Value(false)); }

ValuePtr list_of(std::vector<ValuePtr> items)
{
    List out;
    for (ValuePtr &item : items)
        out.push_back(std::move(item));
    return make_value(std::move(out));
}

ValuePtr map_of(std::vector<std::pair<ValuePtr, ValuePtr>> entries)
{
    MapBody body, next;
    std::string error;
    for (auto &entry : entries)
        if (map_with(body, entry.first, entry.second, next, error))
            body = std::move(next);
    return make_value(std::move(body));
}

// Is there a finding whose value renders as `want`, from `phase`?
const Finding *find_from(const std::vector<Finding> &findings,
                                const char *want, OrbitPhase phase)
{
    for (const Finding &finding : findings)
        if (finding.phase == phase && finding.value &&
            to_string(*finding.value) == want)
            return &finding;
    return nullptr;
}

const Finding *find_any(const std::vector<Finding> &findings,
                               const char *want)
{
    for (const Finding &finding : findings)
        if (finding.value && to_string(*finding.value) == want)
            return &finding;
    return nullptr;
}

// The memory file every test that touches phase 4 points at, so no test ever
// reads or writes the real $HOME/.satl_orbit. Set before the first resolve,
// because orbit_resolve RECORDS -- every test in this file would otherwise
// leave a trail in the user's own memory.
const char *TEST_MEMORY = ".orbit_test_memory";

int main()
{
    orbit_test_runs();
    orbit_test_accumulates();
    orbit_test_provenance();
    orbit_test_the_example();
    orbit_test_link();
    orbit_test_memory();
    orbit_test_result_type();
    orbit_test_result_alternatives();
    orbit_test_result_searchable();
    orbit_test_settle_chases();
    orbit_test_settle_improves();
    orbit_test_settle_remembers_once();

    if (failures) {
        printf("orbit_test: %d failure(s)\n", failures);
        return 1;
    }
    printf("PASS: orbit (the run scan finds what score_containers cannot; the "
           "phases accumulate and guess reaches past the dial; every finding "
           "carries its phase and its reason; orbit.txt's {T,T} example "
           "resolves with {T,F} proposed and ranked below it; the one-hop "
           "link; memory round-trips, counts, and never invents; and the "
           "result type binds, carries all five phases, counts attention "
           "apart from what it weighs, holds its alternatives one level deep "
           "and is searchable by the power that made it; and a settle asks again "
           "with its own answer, stops at a repeated question, raises its own "
           "weight by asking better, and teaches memory exactly one thing)\n");
    return 0;
}
