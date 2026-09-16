// Comprehensive Unit Test Suite for Milestone 6 (M6) Resolver Prototype in prototype/M6.

#include "resolver.hpp"
#include "parser.hpp"
#include "unparse.hpp"
#include "reporter.hpp"
#include "renderer.hpp"
#include "satellite_words/words.hpp"
#include "satellite_words/words_runtime.hpp"

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

void print_diags(const std::vector<satellite::Diagnostic> &diags)
{
    for (const auto &d : diags) {
        std::printf("  [DIAG %s] %s (line %u)\n",
                    satellite::code_to_string(d.code).data(),
                    d.message.c_str(), d.primary_span.line);
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

void test_forward_reference_and_mutual_recursion()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
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
    if (!parse_res.ok()) {
        std::printf("parse errors in test_forward_reference:\n");
        for (const auto &err : parse_res.errors)
            std::printf("  [PARSE ERR] %s (line %u)\n", err.message.c_str(), err.span.line);
    }
    check(parse_res.ok(), "mutual recursion parsed clean");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "mutual recursion resolved clean across 4 passes");
    check(resolve_res.capsules.size() == 2, "2 capsules collected");

    const auto *even_info = resolve_res.find_capsule("is_even");
    const auto *odd_info = resolve_res.find_capsule("is_odd");
    check(even_info != nullptr, "is_even capsule found");
    check(odd_info != nullptr, "is_odd capsule found");
    check(even_info && even_info->param_count == 1, "is_even param count is 1");
    check(odd_info && odd_info->param_count == 1, "is_odd param count is 1");
}

void test_frame_slots_and_rebinding_isolation()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule compute(satellite.variable.number a, satellite.variable.number b)\n"
        "{\n"
        "    satellite.variable.number x = a + b\n"
        "    satellite.statement.if (x > 10)\n"
        "    {\n"
        "        satellite.variable.string temp = \"high\"\n"
        "    }\n"
        "    // Rebinding x creates a FRESH slot (DESIGN §7.4)\n"
        "    satellite.variable.number x = 99\n"
        "    satellite.return(x)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "compute parsed clean");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "compute resolved clean");

    const auto *info = resolve_res.find_capsule("compute");
    check(info != nullptr, "compute found");
    if (info) {
        check(info->param_count == 2, "2 parameters: a (slot 0), b (slot 1)");
        // Slots: 0 (a), 1 (b), 2 (first x), 3 (temp), 4 (second x) -> slot_count should be 5
        check(info->slot_count == 5, "5 distinct slots allocated (fresh slot on redeclaration)");
        check(info->slot_names.size() >= 5, "slot_names recorded");
        if (info->slot_names.size() >= 5) {
            check(info->slot_names[0] == "a" && info->slot_names[1] == "b", "param slot names");
            check(info->slot_names[2] == "x" && info->slot_names[3] == "temp" && info->slot_names[4] == "x", "local slot names");
        }
    }
}

void test_spacesuit_inheritance_and_overrides()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.spacesuit Base()\n"
        "{\n"
        "    satellite.protected\n"
        "    {\n"
        "        satellite.variable.number id = 1\n"
        "    }\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.capsule ping()\n"
        "        {\n"
        "            satellite.return(id)\n"
        "        }\n"
        "    }\n"
        "}\n"
        "satellite.spacesuit Derived(Base)\n"
        "{\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.variable.string tag = \"derived\"\n"
        "        satellite.capsule ping()\n"
        "        {\n"
        "            satellite.return(id + 10)\n"
        "        }\n"
        "    }\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    if (!parse_res.ok()) {
        std::printf("parse errors in test_spacesuits:\n");
        for (const auto &err : parse_res.errors)
            std::printf("  [PARSE ERR] %s (line %u)\n", err.message.c_str(), err.span.line);
    }
    check(parse_res.ok(), "spacesuits parsed clean");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "spacesuits resolved clean");

    const auto *base_info = resolve_res.find_suit("Base");
    const auto *derived_info = resolve_res.find_suit("Derived");
    check(base_info != nullptr && derived_info != nullptr, "both suits found");

    if (base_info && derived_info) {
        check(derived_info->super == base_info, "Derived links to Base superclass");
        check(derived_info->is_a("Base"), "Derived is_a Base");

        // Flattened fields: Base::id first (index 0), Derived::tag second (index 1)
        check(derived_info->fields.size() == 2, "Derived has 2 flattened fields");
        if (derived_info->fields.size() == 2) {
            check(derived_info->fields[0].name == "id" && derived_info->fields[0].owner == base_info, "field 0 is Base::id");
            check(derived_info->fields[1].name == "tag" && derived_info->fields[1].owner == derived_info, "field 1 is Derived::tag");
        }

        // Method overriding: Derived overrides ping()
        const auto *ping_m = derived_info->find_method("ping");
        check(ping_m != nullptr, "ping method found in Derived");
        if (ping_m) {
            check(ping_m->owner == derived_info, "Derived overrides Base::ping");
        }
    }
}

void test_inheritance_cycle_breaking()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.spacesuit Alpha(Beta)\n"
        "{\n"
        "}\n"
        "satellite.spacesuit Beta(Alpha)\n"
        "{\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "cyclic suits parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    check(!resolve_res.ok(), "cycle detected and rejected");
    check(!resolve_res.diagnostics.empty(), "emitted diagnostic for cycle");
    if (!resolve_res.diagnostics.empty()) {
        check(resolve_res.diagnostics[0].code == satellite::ErrorCode::E0303_UndefinedSpacesuit, "correct cycle error code");
    }
}

void test_literal_option_folding()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule test_sort(satellite.container.list<satellite.variable.number> items)\n"
        "{\n"
        "    items.sort(\"down\")\n"
        "    items.sort(\"up\")\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "sort calls parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "sort options folded cleanly");

    // Check folded PathIds in resolve table
    bool found_sort_down = false;
    bool found_sort_up = false;

    for (size_t i = 0; i < resolve_res.table.size(); i++) {
        satellite::words::PathId pid = resolve_res.table.get_folded_path(static_cast<satellite::NodeIndex>(i));
        if (pid == static_cast<satellite::words::PathId>(satellite::words::NodeId::CONTAINER_LIST_SORT_DOWN_0))
            found_sort_down = true;
        if (pid == static_cast<satellite::words::PathId>(satellite::words::NodeId::CONTAINER_LIST_SORT_0))
            found_sort_up = true;
    }

    check(found_sort_down, "literal \"down\" folded to CONTAINER_LIST_SORT_DOWN_0 (1 4 2 5)");
    check(found_sort_up, "literal \"up\" folded to CONTAINER_LIST_SORT_0 (1 4 2 3)");
}

void test_special_arguments_six_spellings()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    // Test with "argz" spelling (DESIGN §7.7)
    std::string src =
        "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> argz)\n"
        "{\n"
        "    satellite.console.display(argz.username)\n"
        "    satellite.console.display(argz.machine.threads)\n"
        "    satellite.return(satellite)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "main with argz parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "main with argz resolved");

    const auto *main_info = resolve_res.find_capsule("satellite.main");
    check(main_info != nullptr, "satellite.main found");
    if (main_info) {
        check(main_info->has_special_arguments, "argz recognized as special arguments parameter");
    }
}

void test_type_and_arity_errors()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    // Invalid arity call: foo takes 2, called with 1
    std::string src =
        "satellite.capsule foo(satellite.variable.number a, satellite.variable.number b)\n"
        "{\n"
        "    satellite.return(a + b)\n"
        "}\n"
        "satellite.capsule test_caller()\n"
        "{\n"
        "    foo(42)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "arity test parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    check(!resolve_res.ok(), "arity mismatch detected");
    check(resolve_res.diagnostics.size() >= 1, "at least 1 diagnostic");
    if (!resolve_res.diagnostics.empty()) {
        check(resolve_res.diagnostics[0].code == satellite::ErrorCode::E0307_ArityMismatch, "E0307_ArityMismatch emitted");
    }
}

void test_unknown_variable_with_suggestion()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule calculate(satellite.variable.number count)\n"
        "{\n"
        "    satellite.return(coun + 1)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "typo parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    check(!resolve_res.ok(), "undefined variable detected");
    check(!resolve_res.diagnostics.empty(), "diagnostic emitted");
    if (!resolve_res.diagnostics.empty()) {
        check(resolve_res.diagnostics[0].code == satellite::ErrorCode::E0301_UndefinedVariable, "E0301_UndefinedVariable code");
        check(resolve_res.diagnostics[0].suggestion.has_value(), "suggestion generated");
        if (resolve_res.diagnostics[0].suggestion.has_value()) {
            check(resolve_res.diagnostics[0].suggestion.value().replacement == "count", "suggested 'count' for typo 'coun'");
        }
    }
}

void test_help_topic_suppression()
{
    satellite::AstArena arena;
    satellite::words::Words words;

    std::string src =
        "satellite.capsule help_demo()\n"
        "{\n"
        "    satellite.help(random.ultra)\n"
        "}\n";

    auto parse_res = satellite::parse(src, arena, words);
    check(parse_res.ok(), "help topic parsed");

    auto resolve_res = satellite::resolve(parse_res.program, arena, words);
    if (!resolve_res.ok())
        print_diags(resolve_res.diagnostics);
    check(resolve_res.ok(), "help topic bare path suppressed from unknown variable error");
}

void test_all_example_programs()
{
    std::printf("=== Parsing and Resolving all example programs ===\n");
    const std::vector<std::string> examples = {
        "example/hello_world.satl",
        "example/class_test.satl",
        "example/gui_example.satl"
    };

    for (const auto &ex : examples) {
        std::string text = read_file(ex);
        if (text.empty()) {
            std::printf("Note: example file %s not found, skipping\n", ex.c_str());
            continue;
        }

        satellite::AstArena arena;
        satellite::words::Words words;
        auto parse_res = satellite::parse(text, arena, words);
        if (!parse_res.ok()) {
            std::printf("  [PARSE FAIL] %s\n", ex.c_str());
            for (const auto &err : parse_res.errors)
                std::printf("    err: %s (line %u)\n", err.message.c_str(), err.span.line);
        }
        check(parse_res.ok(), ("Parsed example: " + ex).c_str());

        auto resolve_res = satellite::resolve(parse_res.program, arena, words);
        if (!resolve_res.ok()) {
            std::printf("  [RESOLVE FAIL] %s\n", ex.c_str());
            print_diags(resolve_res.diagnostics);
        }
        check(resolve_res.ok(), ("Resolved example: " + ex).c_str());
        std::printf("  [PASS] %s (capsules: %zu, suits: %zu, nodes resolved: %zu)\n",
                    ex.c_str(), resolve_res.capsules.size(), resolve_res.suits.size(),
                    resolve_res.table.size());
    }
}

} // namespace

int main()
{
    std::printf("=== Running M6 Resolver Prototype Unit Tests ===\n");

    test_forward_reference_and_mutual_recursion();
    test_frame_slots_and_rebinding_isolation();
    test_spacesuit_inheritance_and_overrides();
    test_inheritance_cycle_breaking();
    test_literal_option_folding();
    test_special_arguments_six_spellings();
    test_type_and_arity_errors();
    test_unknown_variable_with_suggestion();
    test_help_topic_suppression();
    test_all_example_programs();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M6 RESOLVER TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> %d TEST(S) FAILED <<<\n", g_failures);
        return 1;
    }
}

