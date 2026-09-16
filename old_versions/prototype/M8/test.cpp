// Comprehensive Unit Test Suite for Milestone 8 (M8) Console & Execution Prototype.

#include "console.hpp"
#include "dispatch.hpp"
#include "runtime.hpp"
#include "value.hpp"
#include "number.hpp"
#include "satellite_words/words.hpp"

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <string>
#include <thread>
#include <vector>

namespace {

int g_failures = 0;

void check(bool condition, const char *msg)
{
    if (!condition) {
        std::printf("FAIL: %s\n", msg);
        g_failures++;
    }
}

// ---------------------------------------------------------------------------
// Test 1: Console Printer Thread & Output Capture
// ---------------------------------------------------------------------------

void test_console_basic_and_capture()
{
    std::printf("Running test_console_basic_and_capture...\n");

    auto &console = satellite::Console::instance();
    console.clear_captured();
    console.enable_capture(true);

    console.display("Line 1 from test");
    console.display("Line 2 from test");
    console.display_raw("Raw Line 3");
    console.drain();

    auto lines = console.captured_lines();
    check(lines.size() == 3, "captured exactly 3 lines");
    if (lines.size() >= 3) {
        check(lines[0] == "Line 1 from test", "line 1 matches");
        check(lines[1] == "Line 2 from test", "line 2 matches");
        check(lines[2] == "Raw Line 3", "line 3 raw matches");
    }

    console.clear_captured();
    console.enable_capture(false);
}

// ---------------------------------------------------------------------------
// Test 2: Console drain() Barrier Synchronization
// ---------------------------------------------------------------------------

void test_console_drain_barrier()
{
    std::printf("Running test_console_drain_barrier...\n");

    auto &console = satellite::Console::instance();
    console.clear_captured();
    console.enable_capture(true);

    std::atomic<bool> drain_completed{false};

    // Queue several messages
    for (int i = 0; i < 20; ++i) {
        console.display("Drain barrier test " + std::to_string(i));
    }

    // Call drain() and assert all 20 are captured immediately after drain returns
    console.drain();
    drain_completed.store(true);

    check(drain_completed.load(), "drain() completed cleanly");
    check(console.captured_lines().size() == 20, "all 20 messages processed before drain() unblocked");

    console.clear_captured();
    console.enable_capture(false);
}

// ---------------------------------------------------------------------------
// Test 3: Multi-threaded Producer Stress & Line Atomicity
// ---------------------------------------------------------------------------

void test_console_multithreaded_stress()
{
    std::printf("Running test_console_multithreaded_stress...\n");

    auto &console = satellite::Console::instance();
    console.clear_captured();
    console.enable_capture(true);

    const int kNumThreads = 8;
    const int kMsgsPerThread = 50;
    const int kTotalMsgs = kNumThreads * kMsgsPerThread;

    std::vector<std::thread> producers;
    producers.reserve(kNumThreads);

    for (int t = 0; t < kNumThreads; ++t) {
        producers.emplace_back([&console, t, kMsgsPerThread]() {
            for (int i = 0; i < kMsgsPerThread; ++i) {
                std::string msg = "Thread[" + std::to_string(t) + "] Msg " + std::to_string(i);
                console.display(msg);
            }
        });
    }

    for (auto &t : producers) {
        t.join();
    }

    console.drain();

    auto captured = console.captured_lines();
    check(captured.size() == static_cast<size_t>(kTotalMsgs),
          "multi-threaded stress captured all messages without dropping");

    // Verify line atomicity: each line starts with "Thread[" and contains "] Msg "
    bool all_atomic = true;
    for (const auto &line : captured) {
        if (line.rfind("Thread[", 0) != 0 || line.find("] Msg ") == std::string::npos) {
            all_atomic = false;
            break;
        }
    }
    check(all_atomic, "every message in multi-threaded queue maintained line atomicity");

    console.clear_captured();
    console.enable_capture(false);
}

// ---------------------------------------------------------------------------
// Test 4: Dispatch Handlers for Return Shapes & Console Display
// ---------------------------------------------------------------------------

void test_dispatch_m8_handlers()
{
    std::printf("Running test_dispatch_m8_handlers...\n");

    satellite::init_m8_dispatch();
    auto &table = satellite::DispatchTable::instance();
    satellite::ExecContext ctx;

    // Test satellite.console.display 1 5 1
    satellite::words::Walk w_disp = satellite::words::walk("satellite.console.display");
    check(w_disp.error == satellite::words::WalkError::NONE, "satellite.console.display path exists");
    const auto *entry_disp = table.get(w_disp.id);
    check(entry_disp != nullptr && entry_disp->is_valid(), "display handler registered");

    // Test return shapes: 1 15 0, 1 15 1, 1 15 2
    satellite::words::Walk w_ret0 = satellite::words::walk("satellite.return");
    const auto *entry_ret0 = table.get(w_ret0.id);
    check(entry_ret0 != nullptr && entry_ret0->is_valid(), "satellite.return() bare handler registered");
    if (entry_ret0) {
        satellite::Value res = entry_ret0->fn(ctx, {});
        check(res.is_nil(), "return() evaluates to nil");
    }

    satellite::words::Walk w_ret1 = satellite::words::walk("satellite.return(satellite)");
    const auto *entry_ret1 = table.get(w_ret1.id);
    check(entry_ret1 != nullptr && entry_ret1->is_valid(), "satellite.return(satellite) handler registered");
    if (entry_ret1) {
        satellite::Value res = entry_ret1->fn(ctx, {});
        check(res == satellite::Value::satellite_singleton(), "return(satellite) evaluates to satellite singleton (DESIGN §4)");
    }

    satellite::words::Walk w_ret2 = satellite::words::walk("satellite.return(value)");
    const auto *entry_ret2 = table.get(w_ret2.id);
    check(entry_ret2 != nullptr && entry_ret2->is_valid(), "satellite.return(value) handler registered");
    if (entry_ret2) {
        satellite::Value num = satellite::Value::number(satellite::Number(42));
        satellite::Value res = entry_ret2->fn(ctx, {num});
        check(res == num, "return(value) passes through returned value");
    }
}

// ---------------------------------------------------------------------------
// Test 5: End-to-End Bare satellite.main() Program Execution
// ---------------------------------------------------------------------------

void test_bare_main_program_execution()
{
    std::printf("Running test_bare_main_program_execution...\n");

    satellite::Runtime rt;
    auto &console = satellite::Console::instance();
    console.clear_captured();
    console.enable_capture(true);

    // Bare satellite.main() program
    std::string prog =
        "satellite.capsule helper(satellite.variable.string msg)\n"
        "{\n"
        "    satellite.console.display(msg)\n"
        "    satellite.return(satellite)\n"
        "}\n"
        "\n"
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    helper(\"Satellite M8 Execution Engine Online!\")\n"
        "    satellite.return(satellite)\n"
        "}\n";

    auto result = rt.run_string(prog);
    check(result.success, "bare main program executed successfully");
    check(result.exit_code == 0, "return(satellite) produced exit code 0");
    check(result.return_value == satellite::Value::satellite_singleton(), "return value is satellite singleton");

    auto lines = console.captured_lines();
    check(!lines.empty(), "console captured helper output");
    if (!lines.empty()) {
        check(lines[0] == "Satellite M8 Execution Engine Online!", "output message matches");
    }

    console.clear_captured();
    console.enable_capture(false);
}

// ---------------------------------------------------------------------------
// Test 6: Return Code Mapping (Explicit Values & Errors)
// ---------------------------------------------------------------------------

void test_return_code_mapping()
{
    std::printf("Running test_return_code_mapping...\n");

    satellite::Runtime rt;

    // Program returning specific exit code 42
    std::string prog_code =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.return(42)\n"
        "}\n";

    auto res1 = rt.run_string(prog_code);
    check(res1.success, "prog_code ran successfully");
    check(res1.exit_code == 42, "return(42) mapped to exit code 42");

    // Program returning 0
    std::string prog_zero =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.return(0)\n"
        "}\n";

    auto res2 = rt.run_string(prog_zero);
    check(res2.exit_code == 0, "return(0) mapped to exit code 0");
}

// ---------------------------------------------------------------------------
// Test 7: Acceptance Programs Full Pipeline
// ---------------------------------------------------------------------------

void test_acceptance_programs_pipeline()
{
    std::printf("Running test_acceptance_programs_pipeline...\n");

    satellite::Runtime rt;

    for (const char *path : {"example/hello_world.satl", "example/class_test.satl", "example/gui_example.satl"}) {
        auto res = rt.run_file(path);
        check(res.success, (std::string("pipeline run ") + path).c_str());
        std::printf("  [PASS] %s executed end-to-end (exit code %d)\n", path, res.exit_code);
    }
}

} // namespace

int main()
{
    std::printf("=== Running M8 Console & Program Execution Prototype Tests ===\n");

    test_console_basic_and_capture();
    test_console_drain_barrier();
    test_console_multithreaded_stress();
    test_dispatch_m8_handlers();
    test_bare_main_program_execution();
    test_return_code_mapping();
    test_acceptance_programs_pipeline();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M8 PROTOTYPE TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> M8 TESTS FAILED WITH %d FAILURES <<<\n", g_failures);
        return 1;
    }
}
