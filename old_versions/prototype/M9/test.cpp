// Comprehensive Test Suite for Milestone 9 (M9): Scalars & Control Flow.
// Part of prototype/M9.

#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

#include "runtime.hpp"
#include "dispatch.hpp"
#include "interrupt.hpp"
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
// Test 1: If-Else Conditional Statements
// ---------------------------------------------------------------------------

void test_if_else_statements()
{
    std::printf("\n--- Test 1: If-Else Conditional Statements ---\n");

    satellite::Runtime rt;

    // 1. Basic if-then
    std::string prog1 =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number x = 10\n"
        "    satellite.variable.number res = 0\n"
        "    satellite.statement.if (x > 5) {\n"
        "        res = 42\n"
        "    }\n"
        "    satellite.return(res)\n"
        "}\n";

    auto r1 = rt.run_string(prog1);
    check(r1.success && r1.exit_code == 42, "if condition taken (then branch executed)");

    // 2. if-else with false condition
    std::string prog2 =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number x = 3\n"
        "    satellite.variable.number res = 0\n"
        "    satellite.statement.if (x > 5) {\n"
        "        res = 100\n"
        "    }\n"
        "    satellite.statement.else {\n"
        "        res = 200\n"
        "    }\n"
        "    satellite.return(res)\n"
        "}\n";

    auto r2 = rt.run_string(prog2);
    check(r2.success && r2.exit_code == 200, "if-else condition false (else branch executed)");
}

// ---------------------------------------------------------------------------
// Test 2: While Loops
// ---------------------------------------------------------------------------

void test_while_loops()
{
    std::printf("\n--- Test 2: While Loops ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number i = 0\n"
        "    satellite.variable.number sum = 0\n"
        "    satellite.statement.while (i < 10) {\n"
        "        sum = sum + i\n"
        "        i = i + 1\n"
        "    }\n"
        "    satellite.return(sum)\n"
        "}\n";

    auto r = rt.run_string(prog);
    // sum of 0..9 = 45
    check(r.success && r.exit_code == 45, "while loop accumulator executes exactly 10 times (sum=45)");
}

// ---------------------------------------------------------------------------
// Test 3: For Loops
// ---------------------------------------------------------------------------

void test_for_loops()
{
    std::printf("\n--- Test 3: For Loops ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number sum = 0\n"
        "    satellite.statement.for (satellite.variable.number i = 1; i <= 5; i = i + 1) {\n"
        "        sum = sum + i\n"
        "    }\n"
        "    satellite.return(sum)\n"
        "}\n";

    auto r = rt.run_string(prog);
    // sum of 1..5 = 15
    check(r.success && r.exit_code == 15, "for loop counter executes 5 times (sum=15)");
}

// ---------------------------------------------------------------------------
// Test 4: Boolean Module Constants & Logic
// ---------------------------------------------------------------------------

void test_boolean_constants_and_logic()
{
    std::printf("\n--- Test 4: Boolean Module Constants & Logic ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.bool t = satellite.bool.true\n"
        "    satellite.variable.bool f = satellite.bool.false\n"
        "    satellite.statement.if (t & !f) {\n"
        "        satellite.return(0)\n"
        "    }\n"
        "    satellite.statement.else {\n"
        "        satellite.return(1)\n"
        "    }\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "satellite.bool.true and satellite.bool.false evaluate correctly");
}

// ---------------------------------------------------------------------------
// Test 5: String & Number Comparison Operators
// ---------------------------------------------------------------------------

void test_comparison_operators()
{
    std::printf("\n--- Test 5: Comparison Operators ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.string a = \"apple\"\n"
        "    satellite.variable.string b = \"banana\"\n"
        "    satellite.variable.number failures = 0\n"
        "    satellite.statement.if (!(a < b)) {\n"
        "        failures = failures + 1\n"
        "    }\n"
        "    satellite.statement.if (a == b) {\n"
        "        failures = failures + 1\n"
        "    }\n"
        "    satellite.statement.if (a != \"apple\") {\n"
        "        failures = failures + 1\n"
        "    }\n"
        "    satellite.return(failures)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "string comparison operators (<, ==, !=) evaluate cleanly");
}

// ---------------------------------------------------------------------------
// Test 6: All 16 String Methods
// ---------------------------------------------------------------------------

void test_16_string_methods()
{
    std::printf("\n--- Test 6: Full Suite of 16 String Methods ---\n");

    satellite::Runtime rt;

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.string s = \"Hello, Satellite!\"\n"
        "    satellite.variable.number fails = 0\n"
        "\n"
        "    // 1. size\n"
        "    satellite.statement.if (s.size != 17) { fails = fails + 1 }\n"
        "\n"
        "    // 2. empty\n"
        "    satellite.statement.if (s.empty) { fails = fails + 1 }\n"
        "    satellite.variable.string empty_str = \"\"\n"
        "    satellite.statement.if (!empty_str.empty) { fails = fails + 1 }\n"
        "\n"
        "    // 3. find\n"
        "    satellite.statement.if (s.find(\"Satellite\") != 7) { fails = fails + 1 }\n"
        "    satellite.statement.if (s.find(\"NoSuch\") != -1) { fails = fails + 1 }\n"
        "\n"
        "    // 4. contains\n"
        "    satellite.statement.if (!s.contains(\"Satellite\")) { fails = fails + 1 }\n"
        "    satellite.statement.if (s.contains(\"Moon\")) { fails = fails + 1 }\n"
        "\n"
        "    // 5. substring\n"
        "    satellite.statement.if (s.substring(7, 16) != \"Satellite\") { fails = fails + 1 }\n"
        "\n"
        "    // 6. starts_with\n"
        "    satellite.statement.if (!s.starts_with(\"Hello\")) { fails = fails + 1 }\n"
        "\n"
        "    // 7. ends_with\n"
        "    satellite.statement.if (!s.ends_with(\"!\")) { fails = fails + 1 }\n"
        "\n"
        "    // 8. lower\n"
        "    satellite.statement.if (s.lower != \"hello, satellite!\") { fails = fails + 1 }\n"
        "\n"
        "    // 9. upper\n"
        "    satellite.statement.if (s.upper != \"HELLO, SATELLITE!\") { fails = fails + 1 }\n"
        "\n"
        "    // 10. split\n"
        "    satellite.container.list words = s.split(\" \")\n"
        "    satellite.statement.if (words.size != 2) { fails = fails + 1 }\n"
        "\n"
        "    // 11. trim\n"
        "    satellite.variable.string padded = \"   padded text   \"\n"
        "    satellite.statement.if (padded.trim != \"padded text\") { fails = fails + 1 }\n"
        "\n"
        "    // 12. replace\n"
        "    satellite.statement.if (s.replace(\"Satellite\", \"Orbit\") != \"Hello, Orbit!\") { fails = fails + 1 }\n"
        "\n"
        "    // 13. to_number\n"
        "    satellite.variable.string num_str = \"12345\"\n"
        "    satellite.statement.if (num_str.to_number != 12345) { fails = fails + 1 }\n"
        "\n"
        "    // 14. append\n"
        "    satellite.statement.if (s.append(\" Extra\") != \"Hello, Satellite! Extra\") { fails = fails + 1 }\n"
        "\n"
        "    // 15. clear\n"
        "    satellite.statement.if (!s.clear.empty) { fails = fails + 1 }\n"
        "\n"
        "    // 16. at\n"
        "    satellite.statement.if (s.at(0) != \"H\") { fails = fails + 1 }\n"
        "    satellite.statement.if (s.at(7) != \"S\") { fails = fails + 1 }\n"
        "\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "all 16 string methods pass verification");
}

// ---------------------------------------------------------------------------
// Test 7: SIGINT Interrupt Safety
// ---------------------------------------------------------------------------

void test_interrupt_handling()
{
    std::printf("\n--- Test 7: SIGINT Interrupt Safety ---\n");

    satellite::Runtime rt;

    // Spawn thread to send interrupt after loop has started running
    std::thread interruptor([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        satellite::trigger_interrupt();
    });

    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number count = 0\n"
        "    satellite.statement.while (satellite.bool.true) {\n"
        "        count = count + 1\n"
        "    }\n"
        "    satellite.return(count)\n"
        "}\n";

    auto r = rt.run_string(prog);
    if (interruptor.joinable()) {
        interruptor.join();
    }

    check(r.interrupted && r.exit_code == satellite::INTERRUPT_EXIT_STATUS,
          "infinite loop interrupted gracefully on statement boundary (exit code 130)");

    satellite::clear_interrupt();
}

// ---------------------------------------------------------------------------
// Test 8: Full Acceptance Pipeline
// ---------------------------------------------------------------------------

void test_acceptance_pipeline()
{
    std::printf("\n--- Test 8: Acceptance Programs Pipeline ---\n");

    satellite::Runtime rt;

    for (const char *path : {"example/hello_world.satl", "example/class_test.satl", "example/gui_example.satl"}) {
        auto res = rt.run_file(path);
        check(res.success, (std::string("pipeline run ") + path).c_str());
    }
}

} // namespace

int main()
{
    std::printf("=== Running Milestone 9 (M9) Comprehensive Test Suite ===\n");

    test_if_else_statements();
    test_while_loops();
    test_for_loops();
    test_boolean_constants_and_logic();
    test_comparison_operators();
    test_16_string_methods();
    test_interrupt_handling();
    test_acceptance_pipeline();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M9 TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> M9 TESTS FAILED WITH %d FAILURES <<<\n", g_failures);
        return 1;
    }
}
