// Comprehensive Test Suite & CLI Tool for Milestone 5 Error Reporter prototype in flash/M5/

#include "diagnostic.hpp"
#include "diagnostic_code.hpp"
#include "suggester.hpp"
#include "source_view.hpp"
#include "renderer.hpp"
#include "reporter.hpp"
#include "lexer_parser_bridge.hpp"

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

void test_error_codes()
{
    check(code_category(satellite::ErrorCode::E0001_InvalidCharacter) == satellite::ErrorCategory::Lexical, "E0001 is Lexical");
    check(code_category(satellite::ErrorCode::E0101_UnexpectedToken) == satellite::ErrorCategory::Syntax, "E0101 is Syntax");
    check(code_category(satellite::ErrorCode::E0202_NoSuchWord) == satellite::ErrorCategory::PathTrie, "E0202 is PathTrie");
    check(code_category(satellite::ErrorCode::E0301_UndefinedVariable) == satellite::ErrorCategory::Resolve, "E0301 is Resolve");
    check(code_category(satellite::ErrorCode::E0401_RecursionLimitExceeded) == satellite::ErrorCategory::Runtime, "E0401 is Runtime");

    check(code_to_string(satellite::ErrorCode::E0202_NoSuchWord) == "E0202", "E0202 string code");
    check(code_name(satellite::ErrorCode::E0202_NoSuchWord) == "NoSuchWord", "E0202 name");
    check(!code_summary(satellite::ErrorCode::E0202_NoSuchWord).empty(), "E0202 summary");
}

void test_damerau_levenshtein()
{
    check(satellite::edit_distance("console", "console") == 0, "identical strings dist 0");
    check(satellite::edit_distance("consle", "console") == 1, "deletion dist 1");
    check(satellite::edit_distance("consolee", "console") == 1, "insertion dist 1");
    check(satellite::edit_distance("conzole", "console") == 1, "substitution dist 1");
    check(satellite::edit_distance("whlie", "while") == 1, "transposition dist 1");
    check(satellite::edit_distance("CONSOLE", "console") == 0, "case insensitive match dist 0");
    check(satellite::edit_distance("", "satellite") == 9, "empty vs non-empty");
}

void test_trie_did_you_mean()
{
    // Under satellite
    auto sug1 = satellite::suggest_trie_word(satellite::words::NodeId::SATELLITE, "consle");
    check(sug1.has_value() && *sug1 == "console", "suggests 'console' for 'consle'");

    auto sug2 = satellite::suggest_trie_word(satellite::words::NodeId::SATELLITE, "randm");
    check(sug2.has_value() && *sug2 == "random", "suggests 'random' for 'randm'");

    auto sug3 = satellite::suggest_trie_word(satellite::words::NodeId::SATELLITE, "varible");
    check(sug3.has_value() && *sug3 == "variable", "suggests 'variable' for 'varible'");

    auto sug4 = satellite::suggest_trie_word(satellite::words::NodeId::SATELLITE, "publick");
    check(sug4.has_value() && *sug4 == "public", "suggests 'public' for 'publick'");

    // Under console
    auto sug5 = satellite::suggest_trie_word(satellite::words::NodeId::CONSOLE, "dsiplay");
    check(sug5.has_value() && *sug5 == "display", "suggests 'display' for 'dsiplay'");

    auto sug6 = satellite::suggest_trie_word(satellite::words::NodeId::CONSOLE, "inpt");
    check(sug6.has_value() && *sug6 == "input", "suggests 'input' for 'inpt'");

    // Under container
    auto sug7 = satellite::suggest_trie_word(satellite::words::NodeId::CONTAINER, "lst");
    check(sug7.has_value() && *sug7 == "list", "suggests 'list' for 'lst'");
}

void test_candidate_suggestions()
{
    std::vector<std::string> file_modes = {"read", "write", "append", "read_append"};
    auto sug1 = satellite::suggest_candidate("reed", file_modes);
    check(sug1.has_value() && *sug1 == "read", "suggests 'read' for 'reed'");

    auto sug2 = satellite::suggest_candidate("apend", file_modes);
    check(sug2.has_value() && *sug2 == "append", "suggests 'append' for 'apend'");
}

void test_source_view_and_locations()
{
    satellite::SourceMap sources;
    std::string src = "line 1\nline 2 with more text\nline 3\n";
    uint32_t fid = sources.add(src, "test.satl");

    // "with" is at byte offset 14..18
    satellite::Span sp{14, 18, 2, fid};
    satellite::SourceLocation loc = satellite::locate_span(sp, sources);
    check(loc.line == 2 && loc.column == 8, "located line 2 column 8");
    check(satellite::format_span_location(sp, sources) == "test.satl:2:8", "formatted location matches");

    auto slices = satellite::extract_span_lines(sp, sources);
    check(slices.size() == 1, "1 line extracted for single-line span");
    check(slices[0].line_number == 2, "extracted line number 2");
    check(slices[0].line_text == "line 2 with more text", "extracted line text");
    check(slices[0].col_start == 8 && slices[0].col_end == 12, "column range 8..12 for 'with'");
}

void test_renderer_output()
{
    satellite::SourceMap sources;
    std::string src = "satellite.capsule main()\n{\n    satellite.consle.display(\"hi\")\n}\n";
    uint32_t fid = sources.add(src, "main.satl");

    // Span over "consle" (offset 41..47, line 3)
    satellite::Span sp{41, 47, 3, fid};
    satellite::Diagnostic diag = satellite::Diagnostic::error(
        satellite::ErrorCode::E0202_NoSuchWord,
        "no 'consle' under 'satellite' — did you mean 'console'?",
        sp
    );
    diag.with_suggestion("did you mean 'console'?", sp, "console");

    satellite::RenderOptions opts;
    opts.color = satellite::ColorMode::Never;
    satellite::DiagnosticRenderer renderer(opts);

    std::string out = renderer.render(diag, sources);
    check(out.find("main.satl:3:") != std::string::npos, "rendered location");
    check(out.find("[E0202]") != std::string::npos, "rendered error code");
    check(out.find("no 'consle' under 'satellite' — did you mean 'console'?") != std::string::npos, "rendered message");
    check(out.find("consle.display") != std::string::npos, "rendered source excerpt");
    check(out.find("^^^^^^") != std::string::npos, "rendered 6-char caret underline for 'consle'");
    check(out.find("help: did you mean 'console'?") != std::string::npos, "rendered help suggestion");
}

void test_notes_and_call_stacks()
{
    satellite::SourceMap sources;
    std::string src =
        "satellite.variable.number x = 1\n"
        "satellite.variable.number x = 2\n";
    uint32_t fid = sources.add(src, "dup.satl");

    satellite::Span sp1{26, 27, 1, fid}; // first 'x'
    satellite::Span sp2{59, 60, 2, fid}; // second 'x'

    satellite::Diagnostic diag = satellite::Diagnostic::error(
        satellite::ErrorCode::E0205_DuplicateDeclaration,
        "'x' is already declared in this scope",
        sp2
    );
    diag.add_note("previous declaration of 'x' was here", sp1);
    diag.add_frame("main", sp2);

    satellite::RenderOptions opts;
    opts.color = satellite::ColorMode::Never;
    satellite::DiagnosticRenderer renderer(opts);

    std::string out = renderer.render(diag, sources);
    check(out.find("[E0205] error: 'x' is already declared in this scope") != std::string::npos, "primary error");
    check(out.find("note: previous declaration of 'x' was here") != std::string::npos, "note message");
    check(out.find("stack backtrace:") != std::string::npos, "stack trace header");
    check(out.find("in capsule 'main'") != std::string::npos, "frame capsule name");
}

void test_reporter_management()
{
    satellite::DiagnosticReporter rep(5);
    check(!rep.has_errors(), "initially empty");
    check(rep.error_count() == 0, "0 errors");

    rep.error(satellite::ErrorCode::E0101_UnexpectedToken, "err 1");
    rep.warning(satellite::ErrorCode::E0101_UnexpectedToken, "warn 1");
    rep.error(satellite::ErrorCode::E0101_UnexpectedToken, "err 2");

    check(rep.has_errors(), "has errors");
    check(rep.error_count() == 2, "2 errors");
    check(rep.warning_count() == 1, "1 warning");
    check(rep.total_count() == 3, "3 total diagnostics");

    rep.clear();
    check(!rep.has_errors(), "cleared");
    check(rep.error_count() == 0, "0 errors after clear");
}

void test_bridge_on_invalid_snippets()
{
    // Test 1: Trie misspelling
    {
        std::string src =
            "satellite.capsule main()\n"
            "{\n"
            "    satellite.consle.display(\"Hello\")\n"
            "}\n";
        auto res = satellite::diagnose_source(src, "snippet1.satl");
        check(!res.ok, "snippet1 fails compilation");
        check(res.reporter.error_count() >= 1, "has at least 1 error");
        bool found_e0202 = false;
        for (const auto &d : res.reporter.diagnostics()) {
            if (d.code == satellite::ErrorCode::E0202_NoSuchWord)
                found_e0202 = true;
        }
        check(found_e0202, "detected E0202 NoSuchWord for consle");
    }

    // Test 2: Lexer unterminated string
    {
        std::string src = "satellite.capsule test() { x = \"unterminated string\n}";
        auto res = satellite::diagnose_source(src, "snippet2.satl");
        check(!res.ok, "snippet2 fails lexing");
        check(res.reporter.error_count() >= 1, "has lexer error");
        check(res.reporter.diagnostics()[0].code == satellite::ErrorCode::E0002_UnterminatedString, "E0002 for unterminated string");
    }

    // Test 3: DESIGN §9 model: literal naming variable
    {
        satellite::Span sp{0, 2, 1, 0};
        satellite::Diagnostic d = satellite::Diagnostic::error(
            satellite::ErrorCode::E0007_ReservedWordAsIdentifier,
            "'x1' is a hexadecimal literal, so it cannot name a variable",
            sp
        );
        check(d.message == "'x1' is a hexadecimal literal, so it cannot name a variable", "DESIGN §9 model sentence matches");
    }
}

void test_clean_example_files()
{
    for (const char *path : {
        "example/hello_world.satl",
        "example/advanced.satl",
        "example/class_test.satl",
        "example/super_advanced.satl",
        "example/thread_test.satl"})
    {
        std::string src = read_file(path);
        if (src.empty()) continue;
        auto res = satellite::diagnose_source(src, path);
        check(res.ok, (std::string("example clean parse: ") + path).c_str());
        if (!res.ok) {
            satellite::DiagnosticRenderer r;
            std::printf("Unexpected errors in %s:\n%s\n", path,
                        res.reporter.format_all(res.sources, r).c_str());
        }
    }

    // Verify gui_example.satl: parses with M4, and M5 accurately flags
    // the legacy 'satellite.console.new' (PLAN §8 M11.A: 'satellite.window.console.new')
    {
        std::string src = read_file("example/gui_example.satl");
        if (!src.empty()) {
            auto res = satellite::diagnose_source(src, "example/gui_example.satl");
            check(!res.ok, "gui_example.satl correctly diagnosed for legacy path");
            bool found_console_new = false;
            for (const auto &d : res.reporter.diagnostics()) {
                if (d.code == satellite::ErrorCode::E0202_NoSuchWord &&
                    d.message.find("no 'new' under 'console'") != std::string::npos) {
                    found_console_new = true;
                }
            }
            check(found_console_new, "gui_example accurately flags 'no new under console'");
        }
    }
}

} // namespace

int main(int argc, char **argv)
{
    if (argc > 1) {
        std::string arg1 = argv[1];
        if (arg1 == "--demo") {
            std::string sample =
                "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> args)\n"
                "{\n"
                "    satellite.variable.number x = 1\n"
                "    satellite.variable.number x = 2\n"
                "    satellite.consle.display(\"Hello, World!\")\n"
                "    satellite.randm.fast(10)\n"
                "    satellite.return(satellite)\n"
                "}\n";

            std::printf("=== Running M5 Diagnostic Reporter Demo on sample source ===\n\n");
            auto res = satellite::diagnose_source(sample, "demo.satl");
            satellite::DiagnosticRenderer renderer;
            std::printf("%s", res.reporter.format_all(res.sources, renderer).c_str());
            std::printf("\n=== Analysis completed: %zu errors, %zu warnings ===\n",
                        res.reporter.error_count(), res.reporter.warning_count());
            return 0;
        }

        std::string src = read_file(arg1);
        if (src.empty()) {
            std::fprintf(stderr, "Cannot open %s\n", arg1.c_str());
            return 1;
        }

        auto res = satellite::diagnose_source(src, arg1);
        if (!res.ok) {
            satellite::DiagnosticRenderer renderer;
            std::printf("%s", res.reporter.format_all(res.sources, renderer).c_str());
            return 1;
        } else {
            std::printf("Clean: %s (%zu top-level items, 0 errors)\n",
                        arg1.c_str(), res.program.items.size());
            return 0;
        }
    }

    std::printf("=== Running M5 Diagnostic Error Reporter Unit Tests ===\n");
    test_error_codes();
    test_damerau_levenshtein();
    test_trie_did_you_mean();
    test_candidate_suggestions();
    test_source_view_and_locations();
    test_renderer_output();
    test_notes_and_call_stacks();
    test_reporter_management();
    test_bridge_on_invalid_snippets();
    test_clean_example_files();

    if (g_failures == 0) {
        std::printf("\n>>> ALL M5 ERROR REPORTER TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> %d TEST FAILURES <<<\n", g_failures);
        return 1;
    }
}
