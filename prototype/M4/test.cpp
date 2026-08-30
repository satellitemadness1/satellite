// Test runner and CLI tool for the Milestone 4 Arena AST & Parser prototype in flash/

#include "ast.hpp"
#include "parser.hpp"
#include "unparse.hpp"
#include "satellite_string/satellite_string.hpp"
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

void test_arena_basics()
{
    satellite::AstArena arena;
    check(arena.size() == 1, "arena starts with sentinel kNullNode at index 0");
    check(arena.get(satellite::kNullNode).kind == satellite::NodeKind::None, "kNullNode has None kind");

    satellite::Span sp{10, 20, 2, 0};
    satellite::NodeIndex num = arena.make_number("42.5", sp);
    check(num == 1, "first allocated node is index 1");
    check(arena.get(num).kind == satellite::NodeKind::NumberLit, "node 1 is NumberLit");
    check(std::get<satellite::NumberLit>(arena.get(num).data).text == "42.5", "node 1 text matches");
    check(arena.get(num).span.start == 10 && arena.get(num).span.line == 2, "span preserved");

    satellite::NodeIndex dur = arena.make_duration("100ms", 100'000'000ULL, sp);
    check(dur == 2, "second allocated node is index 2");
    check(arena.get(dur).kind == satellite::NodeKind::DurationLit, "node 2 is DurationLit");
    check(std::get<satellite::DurationLit>(arena.get(dur).data).nanoseconds == 100'000'000ULL, "duration nanoseconds match");

    satellite::NodeIndex name = arena.make_name("my_var", sp);
    satellite::NodeIndex member = arena.make_member(name, "field", sp);
    check(arena.get(member).kind == satellite::NodeKind::Member, "member kind");
    check(std::get<satellite::MemberExpr>(arena.get(member).data).target == name, "target index matches");
    check(std::get<satellite::MemberExpr>(arena.get(member).data).name == "field", "field name matches");
}

void test_dynamic_words_allocation()
{
    satellite::words::Words words;
    satellite::AstArena arena;

    std::string src =
        "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n"
        "{\n"
        "    satellite.return(satellite)\n"
        "}\n"
        "satellite.capsule user_func(satellite.variable.number x)\n"
        "{\n"
        "    satellite.return(x)\n"
        "}\n"
        "satellite.spacesuit CustomSuit()\n"
        "{\n"
        "    satellite.public\n"
        "    {\n"
        "        satellite.variable.number field1\n"
        "    }\n"
        "}\n"
        "satellite.library.shared_config = 42\n";

    auto res = satellite::parse(src, arena, words);
    check(res.ok(), "parsed successfully with dynamic words allocation");
    check(res.program.items.size() == 4, "4 top-level items");

    // satellite.main uses LIBRARY_MAIN
    const auto &main_node = arena.get(res.program.items[0]);
    check(main_node.kind == satellite::NodeKind::Capsule, "item 0 is Capsule");
    const auto &main_cap = std::get<satellite::CapsuleDecl>(main_node.data);
    check(main_cap.reserved && main_cap.name == "main", "main is reserved");
    check(main_cap.path_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::LIBRARY_MAIN), "main has LIBRARY_MAIN PathId");

    // user_func takes sequential dynamic PathId
    const auto &user_node = arena.get(res.program.items[1]);
    const auto &user_cap = std::get<satellite::CapsuleDecl>(user_node.data);
    check(!user_cap.reserved && user_cap.name == "user_func", "user_func name");
    check(user_cap.path_id > satellite::words::kNodeCount, "user_func has dynamic user PathId");

    // CustomSuit takes sequential dynamic PathId
    const auto &suit_node = arena.get(res.program.items[2]);
    const auto &suit = std::get<satellite::SpacesuitDecl>(suit_node.data);
    check(suit.name == "CustomSuit", "CustomSuit name");
    check(suit.path_id > satellite::words::kNodeCount, "CustomSuit has dynamic user PathId");

    // shared_config takes sequential dynamic PathId
    const auto &glob_node = arena.get(res.program.items[3]);
    const auto &glob = std::get<satellite::GlobalDecl>(glob_node.data);
    check(glob.name == "shared_config", "shared_config name");
    check(glob.path_id > satellite::words::kNodeCount, "shared_config has dynamic user PathId");
}

void test_expressions()
{
    satellite::words::Words words;
    satellite::AstArena arena;

    // Precedence climbing test
    {
        std::string src = "satellite.capsule test() { x = a + b * c == d - e / f }";
        auto res = satellite::parse(src, arena, words);
        check(res.ok(), "precedence expression parsed");
        std::string unparsed = satellite::unparse(res.program, arena);
        check(unparsed.find("a + b * c == d - e / f") != std::string::npos, "precedence unparsed cleanly without redundant parens");
    }

    // Associativity and grouping
    {
        std::string src = "satellite.capsule test() { x = (a + b) * c }";
        auto res = satellite::parse(src, arena, words);
        check(res.ok(), "grouped expression parsed");
        std::string unparsed = satellite::unparse(res.program, arena);
        check(unparsed.find("(a + b) * c") != std::string::npos, "grouping parens preserved in unparse");
    }

    // Slicing and indexing
    {
        std::string src = "satellite.capsule test() { a = list[0]; b = list[1:5]; c = list[:3]; d = list[2:] }";
        auto res = satellite::parse(src, arena, words);
        check(res.ok(), "slicing expressions parsed");
        std::string unparsed = satellite::unparse(res.program, arena);
        check(unparsed.find("list[0]") != std::string::npos, "index unparsed");
        check(unparsed.find("list[1:5]") != std::string::npos, "slice 1:5 unparsed");
        check(unparsed.find("list[:3]") != std::string::npos, "slice :3 unparsed");
        check(unparsed.find("list[2:]") != std::string::npos, "slice 2: unparsed");
    }

    // List literals, Bits, Durations
    {
        std::string src = "satellite.capsule test() { l = {1, 2, 3}; empty = {}; bits = x00FF; dur = 100ms }";
        auto res = satellite::parse(src, arena, words);
        check(res.ok(), "literals parsed");
        std::string unparsed = satellite::unparse(res.program, arena);
        check(unparsed.find("{1, 2, 3}") != std::string::npos, "list literal unparsed");
        check(unparsed.find("{}") != std::string::npos, "empty list unparsed");
        check(unparsed.find("x00FF") != std::string::npos, "bits literal unparsed");
        check(unparsed.find("100ms") != std::string::npos, "duration literal unparsed");
    }
}

void test_statements()
{
    satellite::words::Words words;
    satellite::AstArena arena;

    std::string src =
        "satellite.capsule test()\n"
        "{\n"
        "    satellite.statement.if (x > 0)\n"
        "    {\n"
        "        y = 1\n"
        "    }\n"
        "    satellite.statement.else\n"
        "    {\n"
        "        y = 0\n"
        "    }\n"
        "    satellite.statement.while (y < 10)\n"
        "    {\n"
        "        y = y + 1\n"
        "    }\n"
        "    satellite.statement.for (satellite.variable.number i = 0; i < 5; i = i + 1)\n"
        "    {\n"
        "        satellite.console.display(i)\n"
        "    }\n"
        "    satellite.return(y)\n"
        "}\n";

    auto res = satellite::parse(src, arena, words);
    check(res.ok(), "control flow statements parsed");
    std::string unp = satellite::unparse(res.program, arena);
    check(unp.find("satellite.statement.if (x > 0)") != std::string::npos, "if unparsed");
    check(unp.find("satellite.statement.else") != std::string::npos, "else unparsed");
    check(unp.find("satellite.statement.while (y < 10)") != std::string::npos, "while unparsed");
    check(unp.find("satellite.statement.for (satellite.variable.number i = 0; i < 5; i = i + 1)") != std::string::npos, "for unparsed");
    check(unp.find("satellite.return(y)") != std::string::npos, "return unparsed");
}

void test_example_file(const std::string &path)
{
    std::string src = read_file(path);
    if (src.empty()) {
        std::printf("Could not read %s, skipping.\n", path.c_str());
        return;
    }

    satellite::words::Words words1;
    satellite::AstArena arena1;
    auto res1 = satellite::parse(src, arena1, words1);
    if (!res1.ok()) {
        std::printf("Parse Error in %s (%zu errors):\n", path.c_str(), res1.errors.size());
        satellite::SourceMap sources(src, path);
        for (const auto &e : res1.errors)
            std::printf("%s\n", satellite::format_error(e, sources).c_str());
        check(false, ("parsing failed on " + path).c_str());
        return;
    }
    check(res1.ok(), ("parsed cleanly: " + path).c_str());

    // Round-trip unparsing test: unparse(parse(unparse(parse(src)))) == unparse(parse(src))
    std::string unparsed1 = satellite::unparse(res1.program, arena1);

    satellite::words::Words words2;
    satellite::AstArena arena2;
    auto res2 = satellite::parse(unparsed1, arena2, words2);
    check(res2.ok(), ("re-parsing unparsed source succeeded for " + path).c_str());

    std::string unparsed2 = satellite::unparse(res2.program, arena2);
    check(unparsed1 == unparsed2, ("round-trip fixpoint holds for " + path).c_str());
}

} // namespace

int main(int argc, char **argv)
{
    if (argc > 1) {
        std::string path = argv[1];
        std::string src = read_file(path);
        if (src.empty()) {
            std::fprintf(stderr, "Cannot open %s\n", path.c_str());
            return 1;
        }

        satellite::words::Words words;
        satellite::AstArena arena;
        satellite::SourceMap sources(src, path);

        auto res = satellite::parse(src, arena, words);
        if (!res.ok()) {
            std::fprintf(stderr, "=== %zu Parse Errors in %s ===\n", res.errors.size(), path.c_str());
            for (const auto &err : res.errors)
                std::fprintf(stderr, "%s\n", satellite::format_error(err, sources).c_str());
            return 1;
        }

        std::printf("=== Successfully parsed %s (%zu top-level items, %zu arena nodes) ===\n",
                    path.c_str(), res.program.items.size(), arena.size());

        if (argc > 2 && std::string(argv[2]) == "--unparse") {
            std::printf("=== Unparsed Source ===\n%s\n", satellite::unparse(res.program, arena).c_str());
        }
        return 0;
    }

    std::printf("=== Running M4 Arena AST & Parser Prototype Unit Tests ===\n");
    test_arena_basics();
    test_dynamic_words_allocation();
    test_expressions();
    test_statements();

    std::printf("=== Testing and Round-tripping all example programs ===\n");
    test_example_file("example/hello_world.satl");
    test_example_file("example/advanced.satl");
    test_example_file("example/class_test.satl");
    test_example_file("example/gui_example.satl");
    test_example_file("example/super_advanced.satl");
    test_example_file("example/thread_test.satl");

    if (g_failures == 0) {
        std::printf("\n>>> ALL M4 AST & PARSER TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> %d TEST FAILURES <<<\n", g_failures);
        return 1;
    }
}

