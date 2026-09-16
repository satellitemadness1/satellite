// Comprehensive Test Suite for Milestone 10 (M10): Containers & the Search Power.
// Part of prototype/M10.

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "runtime.hpp"
#include "dispatch.hpp"
#include "search.hpp"
#include "console.hpp"
#include "satellite_string/satellite_string.hpp"

namespace {

int g_failures = 0;

void check(bool condition, const char *msg)
{
    if (!condition) {
        std::printf("  [FAIL] %s\n", msg);
        g_failures++;
    } else {
        std::printf("  [PASS] %s\n", msg);
    }
}

// ---------------------------------------------------------------------------
// Test 1: Map Operations (all 9 methods + index assignment)
// ---------------------------------------------------------------------------

void test_map_operations()
{
    std::printf("\n--- Test 1: Map Operations (9 Methods & Subscripts) ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.container.map m = satellite.container.map\n"
        "    satellite.variable.number fails = 0\n"
        "\n"
        "    // 1. empty\n"
        "    satellite.statement.if (!m.empty) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.size != 0) { fails = fails + 1 }\n"
        "\n"
        "    // 2. set & get & has & size\n"
        "    m = m.set(\"alpha\", 100)\n"
        "    m = m.set(\"beta\", 200)\n"
        "    satellite.statement.if (m.size != 2) { fails = fails + 1 }\n"
        "    satellite.statement.if (!m.has(\"alpha\")) { fails = fails + 1 }\n"
        "    satellite.statement.if (!m.has(\"beta\")) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.has(\"gamma\")) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.get(\"alpha\") != 100) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.get(\"beta\") != 200) { fails = fails + 1 }\n"
        "\n"
        "    // 3. keys & values in insertion order\n"
        "    satellite.container.list ks = m.keys\n"
        "    satellite.container.list vs = m.values\n"
        "    satellite.statement.if (ks.size != 2) { fails = fails + 1 }\n"
        "    satellite.statement.if (vs.size != 2) { fails = fails + 1 }\n"
        "    satellite.statement.if (ks.first != \"alpha\") { fails = fails + 1 }\n"
        "    satellite.statement.if (ks.last != \"beta\") { fails = fails + 1 }\n"
        "    satellite.statement.if (vs.first != 100) { fails = fails + 1 }\n"
        "    satellite.statement.if (vs.last != 200) { fails = fails + 1 }\n"
        "\n"
        "    // 4. remove\n"
        "    m = m.remove(\"alpha\")\n"
        "    satellite.statement.if (m.size != 1) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.has(\"alpha\")) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.get(\"beta\") != 200) { fails = fails + 1 }\n"
        "\n"
        "    // 5. clear\n"
        "    m = m.clear\n"
        "    satellite.statement.if (!m.empty) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.size != 0) { fails = fails + 1 }\n"
        "\n"
        "    // 6. Subscript write\n"
        "    m[\"score\"] = 99\n"
        "    satellite.statement.if (m[\"score\"] != 99) { fails = fails + 1 }\n"
        "    satellite.statement.if (m.size != 1) { fails = fails + 1 }\n"
        "\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "map creation, 9 methods, and subscripting pass");
}

// ---------------------------------------------------------------------------
// Test 2: Full Suite of 25 List Methods (including Keyed Sort)
// ---------------------------------------------------------------------------

void test_25_list_methods()
{
    std::printf("\n--- Test 2: Full Suite of 25 List Methods ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.container.list l = satellite.container.list\n"
        "    satellite.variable.number fails = 0\n"
        "\n"
        "    // 1. empty & size\n"
        "    satellite.statement.if (!l.empty) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.size != 0) { fails = fails + 1 }\n"
        "\n"
        "    // 2. append\n"
        "    l = l.append(30)\n"
        "    l = l.append(10)\n"
        "    l = l.append(20)\n"
        "    satellite.statement.if (l.size != 3) { fails = fails + 1 }\n"
        "\n"
        "    // 3. first & last\n"
        "    satellite.statement.if (l.first != 30) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.last != 20) { fails = fails + 1 }\n"
        "\n"
        "    // 4. contains & index_of\n"
        "    satellite.statement.if (!l.contains(10)) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.contains(999)) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.index_of(10) != 1) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.index_of(999) != -1) { fails = fails + 1 }\n"
        "\n"
        "    // 5. sort() ascending\n"
        "    satellite.container.list s_up = l.sort()\n"
        "    satellite.statement.if (s_up.first != 10) { fails = fails + 1 }\n"
        "    satellite.statement.if (s_up.last != 30) { fails = fails + 1 }\n"
        "\n"
        "    // 6. sort_down() descending\n"
        "    satellite.container.list s_down = l.sort_down()\n"
        "    satellite.statement.if (s_down.first != 30) { fails = fails + 1 }\n"
        "    satellite.statement.if (s_down.last != 10) { fails = fails + 1 }\n"
        "\n"
        "    // 7. sort(direction)\n"
        "    satellite.container.list s_dir = l.sort(\"down\")\n"
        "    satellite.statement.if (s_dir.first != 30) { fails = fails + 1 }\n"
        "\n"
        "    // 8. Keyed sort: sort_up(key) & sort_down(key)\n"
        "    satellite.container.map m1 = satellite.container.map.set(\"val\", 300)\n"
        "    satellite.container.map m2 = satellite.container.map.set(\"val\", 100)\n"
        "    satellite.container.map m3 = satellite.container.map.set(\"val\", 200)\n"
        "    satellite.container.list lm = {m1, m2, m3}\n"
        "    satellite.container.list lm_up = lm.sort_up(\"val\")\n"
        "    satellite.statement.if (lm_up[0].get(\"val\") != 100) { fails = fails + 1 }\n"
        "    satellite.statement.if (lm_up[2].get(\"val\") != 300) { fails = fails + 1 }\n"
        "    satellite.container.list lm_down = lm.sort_down(\"val\")\n"
        "    satellite.statement.if (lm_down[0].get(\"val\") != 300) { fails = fails + 1 }\n"
        "    satellite.statement.if (lm_down[2].get(\"val\") != 100) { fails = fails + 1 }\n"
        "\n"
        "    // 9. reverse\n"
        "    satellite.container.list rev = l.reverse\n"
        "    satellite.statement.if (rev.first != 20) { fails = fails + 1 }\n"
        "    satellite.statement.if (rev.last != 30) { fails = fails + 1 }\n"
        "\n"
        "    // 10. sum, max, min\n"
        "    satellite.statement.if (l.sum != 60) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.max != 30) { fails = fails + 1 }\n"
        "    satellite.statement.if (l.min != 10) { fails = fails + 1 }\n"
        "\n"
        "    // 11. join\n"
        "    satellite.container.list str_list = {\"apple\", \"banana\", \"cherry\"}\n"
        "    satellite.variable.string joined = str_list.join(\", \")\n"
        "    satellite.statement.if (joined != \"apple, banana, cherry\") { fails = fails + 1 }\n"
        "\n"
        "    // 12. insert\n"
        "    satellite.container.list ins = l.insert(1, 15)\n"
        "    satellite.statement.if (ins.size != 4) { fails = fails + 1 }\n"
        "    satellite.statement.if (ins[1] != 15) { fails = fails + 1 }\n"
        "\n"
        "    // 13. remove, remove_at, remove_first, remove_last\n"
        "    satellite.container.list rem = ins.remove(15)\n"
        "    satellite.statement.if (rem.size != 3) { fails = fails + 1 }\n"
        "    satellite.statement.if (rem.contains(15)) { fails = fails + 1 }\n"
        "\n"
        "    satellite.container.list rem_at = ins.remove_at(1)\n"
        "    satellite.statement.if (rem_at.size != 3) { fails = fails + 1 }\n"
        "\n"
        "    satellite.container.list rem_f = l.remove_first()\n"
        "    satellite.statement.if (rem_f.first != 10) { fails = fails + 1 }\n"
        "\n"
        "    satellite.container.list rem_l = l.remove_last()\n"
        "    satellite.statement.if (rem_l.last != 10) { fails = fails + 1 }\n"
        "\n"
        "    // 14. truncate & reserve\n"
        "    satellite.container.list trunc = l.truncate(2)\n"
        "    satellite.statement.if (trunc.size != 2) { fails = fails + 1 }\n"
        "\n"
        "    satellite.container.list res = l.reserve(50)\n"
        "    satellite.statement.if (res.size != 3) { fails = fails + 1 }\n"
        "\n"
        "    // 15. clear\n"
        "    satellite.container.list clr = l.clear\n"
        "    satellite.statement.if (!clr.empty) { fails = fails + 1 }\n"
        "\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "all 25 list methods pass verification");
}

// ---------------------------------------------------------------------------
// Test 3: Search Power 10-Level Scoring Ladder
// ---------------------------------------------------------------------------

void test_search_power_10_ladder()
{
    std::printf("\n--- Test 3: Search Power 10-Level Scoring Ladder ---\n");

    // 1. EXACT (1)
    check(satellite::search_score(satellite::Value::string("target"),
                                  satellite::Value::string("target")) == satellite::SEARCH_EXACT,
          "Level 1: EXACT match");

    // 2. CASE (2)
    check(satellite::search_score(satellite::Value::string("TARGET"),
                                  satellite::Value::string("target")) == satellite::SEARCH_CASE,
          "Level 2: CASE fold match");

    // 3. CROSS-TYPE (3)
    check(satellite::search_score(satellite::Value::number(satellite::Number(123)),
                                  satellite::Value::string("123")) == satellite::SEARCH_CROSS_TYPE,
          "Level 3: CROSS-TYPE match (number vs string)");

    // 4. TRIMMED (4)
    check(satellite::search_score(satellite::Value::string("   padded   "),
                                  satellite::Value::string("padded")) == satellite::SEARCH_TRIMMED,
          "Level 4: TRIMMED whitespace match");

    // 5. PREFIX (5)
    check(satellite::search_score(satellite::Value::string("bolt"),
                                  satellite::Value::string("boltzmann")) == satellite::SEARCH_PREFIX,
          "Level 5: PREFIX match");

    // 6. SUBSTRING (6)
    check(satellite::search_score(satellite::Value::string("orbit"),
                                  satellite::Value::string("satellite_orbit_engine")) == satellite::SEARCH_SUBSTRING,
          "Level 6: SUBSTRING match");

    // 7. ONE TYPO (8)
    check(satellite::search_score(satellite::Value::string("satellite"),
                                  satellite::Value::string("satelite")) == satellite::SEARCH_ONE_TYPO,
          "Level 8: ONE TYPO edit distance <= 1");

    // 8. TWO TYPOS (9)
    check(satellite::search_score(satellite::Value::string("satellite"),
                                  satellite::Value::string("satlite")) == satellite::SEARCH_TWO_TYPOS,
          "Level 9: TWO TYPOS edit distance <= 2");

    // 9. SUBSEQUENCE (10)
    check(satellite::search_score(satellite::Value::string("stlt"),
                                  satellite::Value::string("satellite")) == satellite::SEARCH_SUBSEQUENCE,
          "Level 10: SUBSEQUENCE fuzzy match");
}

// ---------------------------------------------------------------------------
// Test 4: Search Power on Nested Structures & Threshold Dials
// ---------------------------------------------------------------------------

void test_search_power_containers_and_dial()
{
    std::printf("\n--- Test 4: Search Power on Nested Containers & Dials ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number fails = 0\n"
        "\n"
        "    // 1. Threshold read and set (1 22 5, 1 22 6)\n"
        "    satellite.statement.if (satellite.system.threshold() != 1) { fails = fails + 1 }\n"
        "    satellite.system.threshold(8)\n"
        "    satellite.statement.if (satellite.system.threshold() != 8) { fails = fails + 1 }\n"
        "\n"
        "    // 2. Structural search on List of items\n"
        "    satellite.container.list items = {\"satellite\", \"orbit\", \"planet\", \"rocket\"}\n"
        "    satellite.container.list hits = items.search(\"satelite\")\n"
        "    satellite.statement.if (hits.size != 1) { fails = fails + 1 }\n"
        "\n"
        "    // Reset threshold to EXACT\n"
        "    satellite.system.threshold(1)\n"
        "    satellite.container.list exact_hits = items.search(\"orbit\")\n"
        "    satellite.statement.if (exact_hits.size != 1) { fails = fails + 1 }\n"
        "\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "search power on containers and threshold dials pass");
}

// ---------------------------------------------------------------------------
// Test 5: Parameter Binding in satellite.main
// ---------------------------------------------------------------------------

void test_main_parameter_binding()
{
    std::printf("\n--- Test 5: Parameter Binding in satellite.main ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n"
        "{\n"
        "    satellite.statement.if (arguments.size != 0) {\n"
        "        satellite.return(1)\n"
        "    }\n"
        "    satellite.return(0)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "satellite.main list argument binds to empty list by default");
}

// ---------------------------------------------------------------------------
// Test 6: Full Acceptance Pipeline
// ---------------------------------------------------------------------------

void test_acceptance_pipeline()
{
    std::printf("\n--- Test 6: Full Acceptance Programs Pipeline ---\n");

    satellite::Runtime rt;

    for (const char *path : {"example/hello_world.satl", "example/class_test.satl", "example/gui_example.satl"}) {
        auto res = rt.run_file(path);
        check(res.success, (std::string("pipeline run ") + path).c_str());
    }
}

} // namespace

int main()
{
    std::printf("=== Running Milestone 10 (M10) Comprehensive Test Suite ===\n");

    test_map_operations();
    test_25_list_methods();
    test_search_power_10_ladder();
    test_search_power_containers_and_dial();
    test_main_parameter_binding();
    test_acceptance_pipeline();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M10 TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> M10 TESTS FAILED WITH %d FAILURES <<<\n", g_failures);
        return 1;
    }
}
