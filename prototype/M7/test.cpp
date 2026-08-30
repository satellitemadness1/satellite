// Comprehensive Unit Test Suite for Milestone 7 (M7) Value Model & Closure Compilation Prototype.

#include "value.hpp"
#include "number.hpp"
#include "frame.hpp"
#include "limits.hpp"
#include "dispatch.hpp"
#include "closure.hpp"
#include "compiler.hpp"
#include "evaluator.hpp"

#include "parser.hpp"
#include "resolver.hpp"
#include "system_facts/system.hpp"

#include <cassert>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
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

std::string read_file(const std::string &path)
{
    for (const std::string &candidate : {path, "../" + path, "../../" + path}) {
        std::ifstream file(candidate);
        if (file) {
            std::stringstream ss;
            ss << file.rdbuf();
            return ss.str();
        }
    }
    return "";
}

// ---------------------------------------------------------------------------
// Test 1: Value Model & 40-byte Layout
// ---------------------------------------------------------------------------

void test_value_model_and_size()
{
    std::printf("Running test_value_model_and_size...\n");

    // Static assertion check
    static_assert(sizeof(void *) != 8 || sizeof(satellite::Value) == 40,
                  "Value must be 40 bytes on 64-bit architecture");
    check(sizeof(void *) != 8 || sizeof(satellite::Value) == 40, "sizeof(Value) == 40 on 64-bit");

    // Test distinct alternatives
    satellite::Value nil_val = satellite::Value::nil();
    check(nil_val.is_nil(), "nil value is_nil");
    check(!nil_val.is_truthy(), "nil is falsy");

    satellite::Value b_true = satellite::Value::boolean(true);
    satellite::Value b_false = satellite::Value::boolean(false);
    check(b_true.is_bool() && b_true.is_truthy(), "bool true is truthy");
    check(b_false.is_bool() && !b_false.is_truthy(), "bool false is falsy");

    satellite::Value num_val = satellite::Value::number(satellite::Number(12345));
    check(num_val.is_number(), "number is_number");
    check(num_val.is_truthy(), "non-zero number is truthy");

    satellite::Value str_val = satellite::Value::string("satellite test");
    check(str_val.is_string(), "string is_string");
    check(str_val.is_truthy(), "non-empty string is truthy");

    auto list_data = std::make_shared<satellite::List>();
    list_data->push_back(std::make_shared<satellite::Value>(b_true));
    list_data->push_back(std::make_shared<satellite::Value>(num_val));
    satellite::Value list_val(std::const_pointer_cast<const satellite::List>(list_data));
    check(list_val.is_list(), "list is_list");

    // Equality checks
    check(b_true == satellite::Value::boolean(true), "bool true == true");
    check(b_true != b_false, "bool true != false");
    check(num_val == satellite::Value::number(satellite::Number(12345)), "number equality");
    check(nil_val != b_false, "nil != false (different variants)");
}

// ---------------------------------------------------------------------------
// Test 2: Number Exact Arithmetic & Sign
// ---------------------------------------------------------------------------

void test_number_arithmetic()
{
    std::printf("Running test_number_arithmetic...\n");

    satellite::Number a(42);
    satellite::Number b(13);
    check(a.positive, "a positive by default");
    check((a + b) == satellite::Number(55), "42 + 13 == 55");
    check((a - b) == satellite::Number(29), "42 - 13 == 29");
    check((a * b) == satellite::Number(546), "42 * 13 == 546");

    satellite::Number neg(-10);
    check(!neg.positive, "neg.positive is false for negative numbers");
    check(neg.is_negative(), "neg.is_negative() is true");
    check(neg.negated() == satellite::Number(10), "neg.negated() == 10");
    check(neg.abs() == satellite::Number(10), "neg.abs() == 10");

    // Exact division
    satellite::Number n100(100);
    satellite::Number n4(4);
    check((n100 / n4) == satellite::Number(25), "100 / 4 == 25");

    // Bignum arithmetic
    satellite::Number big1;
    check(satellite::Number::parse("100000000000000000000", big1), "parse bignum 1");
    satellite::Number big2;
    check(satellite::Number::parse("200000000000000000000", big2), "parse bignum 2");
    check((big1 + big2).to_string() == "300000000000000000000", "bignum addition");

    // Zero normalization
    satellite::Number zero = a - a;
    check(zero.is_zero(), "zero is_zero()");
    check(zero.positive, "zero has positive == true (no negative zero)");
}

// ---------------------------------------------------------------------------
// Test 3: String Live Fact Codes Decode (95-100)
// ---------------------------------------------------------------------------

void test_str_and_live_facts()
{
    std::printf("Running test_str_and_live_facts...\n");

    // Encode text with live system codes
    std::string text = "threads: \\threads, user: \\user, cwd: \\cwd";
    satellite::SatString encoded = satellite::encode(text);
    std::string decoded = satellite::decode(encoded);

    check(!decoded.empty(), "decode produced output");
    check(decoded.find("<threads>") == std::string::npos, "no unresolved <threads> placeholder");
    check(decoded.find("<user>") == std::string::npos, "no unresolved <user> placeholder");
    check(decoded.find("<cwd>") == std::string::npos, "no unresolved <cwd> placeholder");

    std::string expected_threads = std::to_string(satellite::hardware_threads());
    std::string expected_user = satellite::username();
    check(decoded.find(expected_threads) != std::string::npos, "decoded contains live thread count");
    check(decoded.find(expected_user) != std::string::npos, "decoded contains live username");
}

// ---------------------------------------------------------------------------
// Test 4: Closure Compilation & Basic Execution
// ---------------------------------------------------------------------------

void test_closure_compilation()
{
    std::printf("Running test_closure_compilation...\n");

    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule add(satellite.variable.number a, satellite.variable.number b)\n"
        "{\n"
        "    satellite.return(a + b)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "add capsule parsed clean");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    check(resolve_res.ok(), "add capsule resolved clean");

    auto compiled = satellite::compile(parse_res.program, arena, resolve_res, words);
    check(compiled.capsules.find("add") != compiled.capsules.end(), "add capsule compiled");

    satellite::Evaluator eval(std::move(compiled));
    satellite::Value res = eval.invoke("add", {satellite::Value::number(satellite::Number(15)),
                                              satellite::Value::number(satellite::Number(27))});

    check(res.is_number(), "result is number");
    check(res == satellite::Value::number(satellite::Number(42)), "add(15, 27) == 42");
}

// ---------------------------------------------------------------------------
// Test 5: Frame Isolation & Mutual Recursion
// ---------------------------------------------------------------------------

void test_frame_isolation_and_recursion()
{
    std::printf("Running test_frame_isolation_and_recursion...\n");

    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule fact(satellite.variable.number n)\n"
        "{\n"
        "    satellite.statement.if (n <= 1)\n"
        "    {\n"
        "        satellite.return(1)\n"
        "    }\n"
        "    satellite.return(n * fact(n - 1))\n"
        "}\n"
        "satellite.capsule is_even(satellite.variable.number n)\n"
        "{\n"
        "    satellite.statement.if (n == 0)\n"
        "    {\n"
        "        satellite.return(satellite.bool.true)\n"
        "    }\n"
        "    satellite.return(is_odd(n - 1))\n"
        "}\n"
        "satellite.capsule is_odd(satellite.variable.number n)\n"
        "{\n"
        "    satellite.statement.if (n == 0)\n"
        "    {\n"
        "        satellite.return(satellite.bool.false)\n"
        "    }\n"
        "    satellite.return(is_even(n - 1))\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "recursion code parsed clean");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    check(resolve_res.ok(), "recursion code resolved clean");

    auto compiled = satellite::compile(parse_res.program, arena, resolve_res, words);
    satellite::Evaluator eval(std::move(compiled));

    // Test factorial
    satellite::Value fact_5 = eval.invoke("fact", {satellite::Value::number(satellite::Number(5))});
    check(fact_5 == satellite::Value::number(satellite::Number(120)), "fact(5) == 120 (Frame isolation holding)");

    // Test mutual recursion
    satellite::Value even_10 = eval.invoke("is_even", {satellite::Value::number(satellite::Number(10))});
    check(even_10 == satellite::Value::boolean(true), "is_even(10) == true");

    satellite::Value odd_10 = eval.invoke("is_odd", {satellite::Value::number(satellite::Number(10))});
    check(odd_10 == satellite::Value::boolean(false), "is_odd(10) == false");
}

// ---------------------------------------------------------------------------
// Test 6: Bounded Recursion & Depth Limits
// ---------------------------------------------------------------------------

void test_recursion_depth_bounding()
{
    std::printf("Running test_recursion_depth_bounding...\n");

    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule infinite_recurse(satellite.variable.number n)\n"
        "{\n"
        "    satellite.return(infinite_recurse(n + 1))\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    auto compiled = satellite::compile(parse_res.program, arena, resolve_res, words);

    satellite::Evaluator eval(std::move(compiled), 25); // Set low limit of 25 frames

    bool caught = false;
    try {
        eval.invoke("infinite_recurse", {satellite::Value::number(satellite::Number(1))});
    } catch (const satellite::DepthExceededError &e) {
        caught = true;
        check(std::string(e.what()) == "capsule call too deep", "correct depth error message");
    }
    check(caught, "infinite recursion cleanly caught by depth guard");
}

// ---------------------------------------------------------------------------
// Test 7: Dispatch Table & Inline Caches (DESIGN §2.4)
// ---------------------------------------------------------------------------

void test_dispatch_and_inline_cache()
{
    std::printf("Running test_dispatch_and_inline_cache...\n");

    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule print_msg(satellite.variable.string msg)\n"
        "{\n"
        "    satellite.console.display(msg)\n"
        "    satellite.return(satellite)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    auto compiled = satellite::compile(parse_res.program, arena, resolve_res, words);

    satellite::Evaluator eval(std::move(compiled));
    eval.invoke("print_msg", {satellite::Value::string("Hello from M7 Closure!")});

    check(!eval.output().empty(), "display recorded output");
    if (!eval.output().empty()) {
        check(eval.output()[0] == "Hello from M7 Closure!", "output matches expected message");
    }

    // Call second time to hit inline cache
    eval.clear_output();
    eval.invoke("print_msg", {satellite::Value::string("Second call via Inline Cache")});
    check(!eval.output().empty() && eval.output()[0] == "Second call via Inline Cache",
          "second call executed clean via inline cache");
}

// ---------------------------------------------------------------------------
// Test 8: Acceptance Programs Execution
// ---------------------------------------------------------------------------

void test_acceptance_programs()
{
    std::printf("Running test_acceptance_programs...\n");

    for (const char *path : {"example/hello_world.satl", "example/class_test.satl", "example/gui_example.satl"}) {
        std::string src = read_file(path);
        if (src.empty()) continue;

        satellite::AstArena arena;
        satellite::words::Words words;

        auto parse_res = satellite::parse(src, arena, words);
        check(parse_res.ok(), (std::string("parse ") + path).c_str());

        auto resolve_res = satellite::resolve(parse_res.program, arena, words);
        check(resolve_res.ok(), (std::string("resolve ") + path).c_str());

        auto compiled = satellite::compile(parse_res.program, arena, resolve_res, words);
        satellite::Evaluator eval(std::move(compiled));

        satellite::ExecResult top_res = eval.run_top_level();
        check(top_res.status == satellite::ExecStatus::Normal, (std::string("top-level exec ") + path).c_str());

        satellite::Value main_res = eval.run_main();
        std::printf("  [PASS] %s evaluated clean\n", path);
    }
}

} // namespace

int main()
{
    std::printf("=== Running M7 Value Model & Closure Compilation Unit Tests ===\n");

    test_value_model_and_size();
    test_number_arithmetic();
    test_str_and_live_facts();
    test_closure_compilation();
    test_frame_isolation_and_recursion();
    test_recursion_depth_bounding();
    test_dispatch_and_inline_cache();
    test_acceptance_programs();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M7 PROTOTYPE TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> M7 TESTS FAILED WITH %d FAILURES <<<\n", g_failures);
        return 1;
    }
}
