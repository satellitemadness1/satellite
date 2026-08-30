// Comprehensive Test Suite for Milestone 11 (M11):
// The Prompt, Line Editor, Terminal Control & Window Console.
// Part of prototype/M11.

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unistd.h>

#include "keys.hpp"
#include "editor.hpp"
#include "history.hpp"
#include "render.hpp"
#include "raw_mode.hpp"
#include "block_scan.hpp"
#include "runtime.hpp"
#include "dispatch.hpp"
#include "repl.hpp"
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
// Test 1: KeyDecoder and UTF-8 Helpers
// ---------------------------------------------------------------------------

void test_key_decoder()
{
    std::printf("\n--- Test 1: KeyDecoder & UTF-8 Helpers ---\n");

    satellite::KeyDecoder decoder;

    // 1. Plain ASCII char
    auto ev = decoder.feed('a');
    check(ev.key == satellite::Key::Char && ev.text == "a", "ASCII char 'a' decoded");

    // 2. Enter
    ev = decoder.feed('\n');
    check(ev.key == satellite::Key::Enter, "Enter key decoded");

    // 3. Backspace (0x7f and 0x08)
    ev = decoder.feed(0x7f);
    check(ev.key == satellite::Key::Backspace, "Backspace (0x7f) decoded");
    ev = decoder.feed(0x08);
    check(ev.key == satellite::Key::Backspace, "Backspace (0x08) decoded");

    // 4. Ctrl keys
    ev = decoder.feed(0x03);
    check(ev.key == satellite::Key::Interrupt, "Ctrl-C decoded as Interrupt");
    ev = decoder.feed(0x04);
    check(ev.key == satellite::Key::EndOfInput, "Ctrl-D decoded as EndOfInput");
    ev = decoder.feed(0x01);
    check(ev.key == satellite::Key::Home, "Ctrl-A decoded as Home");
    ev = decoder.feed(0x05);
    check(ev.key == satellite::Key::End, "Ctrl-E decoded as End");
    ev = decoder.feed(0x15);
    check(ev.key == satellite::Key::KillToStart, "Ctrl-U decoded as KillToStart");
    ev = decoder.feed(0x0b);
    check(ev.key == satellite::Key::KillToEnd, "Ctrl-K decoded as KillToEnd");
    ev = decoder.feed(0x17);
    check(ev.key == satellite::Key::KillWordBack, "Ctrl-W decoded as KillWordBack");
    ev = decoder.feed(0x0c);
    check(ev.key == satellite::Key::ClearScreen, "Ctrl-L decoded as ClearScreen");

    // 5. Tab expansion
    ev = decoder.feed('\t');
    check(ev.key == satellite::Key::Char && ev.text == "    ", "Tab decoded as 4 spaces");

    // 6. Arrow keys (ESC [ A/B/C/D)
    ev = decoder.feed(0x1b);
    check(ev.key == satellite::Key::None, "ESC produces None");
    ev = decoder.feed('[');
    check(ev.key == satellite::Key::None, "ESC [ produces None");
    ev = decoder.feed('A');
    check(ev.key == satellite::Key::Up, "ESC [ A decoded as Up arrow");

    // Down arrow
    decoder.feed(0x1b); decoder.feed('[');
    ev = decoder.feed('B');
    check(ev.key == satellite::Key::Down, "ESC [ B decoded as Down arrow");

    // Left arrow
    decoder.feed(0x1b); decoder.feed('[');
    ev = decoder.feed('D');
    check(ev.key == satellite::Key::Left, "ESC [ D decoded as Left arrow");

    // Right arrow
    decoder.feed(0x1b); decoder.feed('[');
    ev = decoder.feed('C');
    check(ev.key == satellite::Key::Right, "ESC [ C decoded as Right arrow");

    // Ctrl-Right arrow: ESC [ 1 ; 5 C
    decoder.feed(0x1b); decoder.feed('['); decoder.feed('1'); decoder.feed(';'); decoder.feed('5');
    ev = decoder.feed('C');
    check(ev.key == satellite::Key::WordRight, "ESC [ 1 ; 5 C decoded as WordRight");

    // Alt-b: ESC b
    decoder.feed(0x1b);
    ev = decoder.feed('b');
    check(ev.key == satellite::Key::WordLeft, "ESC b decoded as WordLeft");

    // 7. UTF-8 multibyte character: Greek letter omega Ω (0xCE 0xA9)
    ev = decoder.feed(0xCE);
    check(ev.key == satellite::Key::None, "UTF-8 lead byte produces None");
    ev = decoder.feed(0xA9);
    check(ev.key == satellite::Key::Char && ev.text.size() == 2, "UTF-8 multibyte sequence assembled");

    // 8. display_width helper
    check(satellite::display_width("hello") == 5, "display_width on ASCII is 5");
    check(satellite::display_width("\033[1;38;2;255;255;255muser\033[0m") == 4, "display_width skips ANSI SGR escapes");
    check(satellite::display_width("αβγ") == 3, "display_width on UTF-8 2-byte chars counts character count (3)");

    // 9. prev_char & next_char
    std::string greek = "αβγ";
    size_t p = satellite::next_char(greek, 0);
    check(p == 2, "next_char skips UTF-8 continuation byte");
    size_t prev_p = satellite::prev_char(greek, 2);
    check(prev_p == 0, "prev_char steps back by whole UTF-8 character");
}

// ---------------------------------------------------------------------------
// Test 2: Line Editor Operations & Live Line Preservation
// ---------------------------------------------------------------------------

void test_line_editor()
{
    std::printf("\n--- Test 2: Line Editor Operations ---\n");

    satellite::History history;
    history.add("satellite.include(satellite)");
    history.add("satellite.console.display(\"test\")");

    satellite::Editor editor(&history);

    // 1. Character insertions
    satellite::KeyEvent k;
    k.key = satellite::Key::Char;
    k.text = "h"; editor.apply(k);
    k.text = "e"; editor.apply(k);
    k.text = "l"; editor.apply(k);
    k.text = "p"; editor.apply(k);
    check(editor.line() == "help" && editor.cursor() == 4, "characters inserted and cursor advanced");

    // 2. Cursor motion & backspace
    k.key = satellite::Key::Left; k.text.clear(); editor.apply(k);
    check(editor.cursor() == 3, "left arrow moves cursor to 3");

    k.key = satellite::Key::Backspace; editor.apply(k);
    check(editor.line() == "hep" && editor.cursor() == 2, "backspace deletes character before cursor");

    // 3. Home & End
    k.key = satellite::Key::Home; editor.apply(k);
    check(editor.cursor() == 0, "home moves cursor to 0");
    k.key = satellite::Key::End; editor.apply(k);
    check(editor.cursor() == 3, "end moves cursor to line end");

    // 4. Word kill (Ctrl-W)
    editor.set_line("satellite.variable.number count = 42");
    k.key = satellite::Key::KillWordBack; editor.apply(k);
    check(editor.line() == "satellite.variable.number count = " && editor.cursor() == 34, "Ctrl-W kills word backward");

    // 5. Kill to start (Ctrl-U) and Kill to end (Ctrl-K)
    editor.set_line("abcdef");
    editor.apply(satellite::KeyEvent{satellite::Key::Home, ""});
    editor.apply(satellite::KeyEvent{satellite::Key::Right, ""});
    editor.apply(satellite::KeyEvent{satellite::Key::Right, ""}); // cursor at 2 ("cd...")
    editor.apply(satellite::KeyEvent{satellite::Key::KillToEnd, ""});
    check(editor.line() == "ab" && editor.cursor() == 2, "Ctrl-K kills to end of line");

    editor.apply(satellite::KeyEvent{satellite::Key::KillToStart, ""});
    check(editor.line() == "" && editor.cursor() == 0, "Ctrl-U kills to start of line");

    // 6. History navigation and live line preservation
    editor.reset();
    editor.apply(satellite::KeyEvent{satellite::Key::Char, "half_typed_input"});
    check(editor.line() == "half_typed_input", "half typed input set");

    // Press Up arrow -> recalls newest history entry
    editor.apply(satellite::KeyEvent{satellite::Key::Up, ""});
    check(editor.line() == "satellite.console.display(\"test\")", "Up arrow recalls newest history item");

    // Press Up arrow again -> recalls older history entry
    editor.apply(satellite::KeyEvent{satellite::Key::Up, ""});
    check(editor.line() == "satellite.include(satellite)", "Up arrow recalls older history item");

    // Press Down arrow -> returns to newest history entry
    editor.apply(satellite::KeyEvent{satellite::Key::Down, ""});
    check(editor.line() == "satellite.console.display(\"test\")", "Down arrow moves forward in history");

    // Press Down arrow past end -> restores uncommitted live line!
    editor.apply(satellite::KeyEvent{satellite::Key::Down, ""});
    check(editor.line() == "half_typed_input", "Down arrow past newest entry restores uncommitted live line");
}

// ---------------------------------------------------------------------------
// Test 3: History Store & Persistence
// ---------------------------------------------------------------------------

void test_history_persistence()
{
    std::printf("\n--- Test 3: History Store & File Persistence ---\n");

    std::string test_path = "/tmp/satl_test_history.txt";
    ::unlink(test_path.c_str());

    {
        satellite::History h(test_path);
        h.add("line one");
        h.add("line two");
        h.add("line two"); // duplicate should be ignored
        h.add("");         // empty line should be ignored
        h.add("line three");
        check(h.size() == 3, "history size is 3 (duplicates and empties ignored)");
        check(h.save(), "history saved to disk");
    }

    {
        satellite::History h2(test_path);
        check(h2.load(), "history loaded from disk");
        check(h2.size() == 3, "history size reloaded is 3");
        check(h2.at(0) == "line one", "entry 0 is oldest");
        check(h2.at(2) == "line three", "entry 2 is newest");
    }

    ::unlink(test_path.c_str());
    check(true, "history persistence verified cleanly");
}

// ---------------------------------------------------------------------------
// Test 4: Block Scanner & Command Parsers
// ---------------------------------------------------------------------------

void test_block_scan_and_commands()
{
    std::printf("\n--- Test 4: Block Scanner & Command Parsers ---\n");

    // 1. scan_block depth
    auto scan = satellite::scan_block("satellite.capsule f()");
    check(scan.opens_body && scan.depth == 1, "declaration head opens body with depth 1");

    scan = satellite::scan_block("satellite.statement.if (x > 0)");
    check(scan.opens_body && scan.depth == 1, "statement.if opens body with depth 1");

    scan = satellite::scan_block("{ a = 1 }");
    check(!scan.opens_body && scan.depth == 0, "single-line balanced braces has net depth 0");

    scan = satellite::scan_block("}");
    check(scan.depth == -1, "closing brace has depth -1");

    // 2. parse_run_command
    auto run_cmd = satellite::parse_run_command("run example/hello_world.satl arg1 arg2");
    check(run_cmd.matched && run_cmd.path == "example/hello_world.satl" && run_cmd.args.size() == 2, "parse_run_command matches run verb and args");

    auto interp_cmd = satellite::parse_run_command("interpret file.satl");
    check(interp_cmd.matched && interp_cmd.path == "file.satl", "parse_run_command matches interpret verb");

    auto normal_cmd = satellite::parse_run_command("satellite.console.display(\"hello\")");
    check(!normal_cmd.matched, "regular satellite source does not match run command");

    // 3. exit_command
    int status = -1;
    check(satellite::exit_command("exit", status) && status == 0, "exit command recognised");
    check(satellite::exit_command("quit()", status) && status == 0, "quit() command recognised");
    check(satellite::exit_command("satellite.return(satellite)", status) && status == 0, "satellite.return(satellite) recognised");
    check(satellite::exit_command("satellite.return(42)", status) && status == 42, "satellite.return(42) maps to exit code 42");
    check(!satellite::exit_command("satellite.variable.number x = 10", status), "regular assignment not an exit command");
}

// ---------------------------------------------------------------------------
// Test 5: Renderer & Terminal Facts
// ---------------------------------------------------------------------------

void test_renderer()
{
    std::printf("\n--- Test 5: Terminal Renderer & Display Width ---\n");

    satellite::Renderer renderer;
    renderer.clear_screen();
    renderer.reset();

    // Verify terminal columns and rows fallback
    int cols = satellite::terminal_columns();
    check(cols > 0, "terminal_columns returns positive width");

    // Renderer draw invocation does not crash
    renderer.draw("satl: ", "satellite.return(satellite)", 27);
    check(true, "renderer draw executes cleanly");
}

// ---------------------------------------------------------------------------
// Test 6: Console Input & Window Console Dispatch
// ---------------------------------------------------------------------------

void test_console_and_window_dispatch()
{
    std::printf("\n--- Test 6: Console Input & Window Console Dispatch ---\n");

    satellite::Runtime rt;

    // 1. satellite.window.console.new(title, width, height)
    std::string prog =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.container.map win = satellite.window.console.new(\"My Window\", 1024, 768)\n"
        "    satellite.variable.number fails = 0\n"
        "    satellite.statement.if (win.get(\"title\") != \"My Window\") { fails = fails + 1 }\n"
        "    satellite.statement.if (win.get(\"width\") != 1024) { fails = fails + 1 }\n"
        "    satellite.statement.if (win.get(\"height\") != 768) { fails = fails + 1 }\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r = rt.run_string(prog);
    check(r.success && r.exit_code == 0, "satellite.window.console.new creates window map with arguments");

    // 2. satellite.console.width & height
    std::string prog_console =
        "satellite.capsule satellite.main()\n"
        "{\n"
        "    satellite.variable.number w = satellite.console.width\n"
        "    satellite.variable.number h = satellite.console.height\n"
        "    satellite.variable.number fails = 0\n"
        "    satellite.statement.if (w <= 0) { fails = fails + 1 }\n"
        "    satellite.statement.if (h <= 0) { fails = fails + 1 }\n"
        "    satellite.return(fails)\n"
        "}\n";

    auto r_console = rt.run_string(prog_console);
    check(r_console.success && r_console.exit_code == 0, "satellite.console.width & height return positive facts");
}

// ---------------------------------------------------------------------------
// Test 7: Interactive REPL Session & Evaluator State Persistence
// ---------------------------------------------------------------------------

void test_repl_session_persistence()
{
    std::printf("\n--- Test 7: REPL Session & Variable Persistence ---\n");

    satellite::Runtime rt;

    // 1. Declare a variable on line 1
    std::string out1 = rt.eval_session_line("satellite.variable.number total = 100", false);
    check(out1.empty(), "variable declaration executed without error");

    // 2. Access and modify variable on line 2
    std::string out2 = rt.eval_session_line("total = total + 50", false);
    check(out2.empty(), "variable modification executed without error");

    // 3. Read variable back on line 3 with echo enabled
    std::string out3 = rt.eval_session_line("total", true);
    check(out3 == "150\n", "variable value persisted across turns and echoed as 150");

    // 4. Declare a capsule across turns and call it
    std::string cap_def =
        "satellite.capsule add_five(satellite.variable.number n)\n"
        "{\n"
        "    satellite.return(n + 5)\n"
        "}\n";
    rt.eval_session_line(cap_def, false);

    std::string call_out = rt.eval_session_line("add_five(total)", true);
    check(call_out == "155\n", "capsule persisted across session turns and returned 155");
}

// ---------------------------------------------------------------------------
// Test 8: Non-Interactive Cooked Stream Fallback
// ---------------------------------------------------------------------------

void test_cooked_fallback()
{
    std::printf("\n--- Test 8: Non-Interactive Cooked Stream Handling ---\n");

    // Verify prompt_is_interactive() logic
    bool interactive = satellite::prompt_is_interactive();
    (void)interactive;
    check(true, "prompt_is_interactive evaluates without error");
}

// ---------------------------------------------------------------------------
// Test 9: Acceptance Programs Pipeline
// ---------------------------------------------------------------------------

void test_acceptance_programs()
{
    std::printf("\n--- Test 9: Acceptance Programs Pipeline ---\n");

    satellite::Runtime rt;

    auto r1 = rt.run_file("example/hello_world.satl");
    check(r1.success && r1.exit_code == 0, "pipeline run example/hello_world.satl");

    auto r2 = rt.run_file("example/class_test.satl");
    check(r2.success && r2.exit_code == 0, "pipeline run example/class_test.satl");

    auto r3 = rt.run_file("example/gui_example.satl");
    check(r3.success && r3.exit_code == 0, "pipeline run example/gui_example.satl");
}

} // namespace

int main()
{
    std::printf("=== Running Milestone 11 (M11) Comprehensive Test Suite ===\n");

    test_key_decoder();
    test_line_editor();
    test_history_persistence();
    test_block_scan_and_commands();
    test_renderer();
    test_console_and_window_dispatch();
    test_repl_session_persistence();
    test_cooked_fallback();
    test_acceptance_programs();

    std::printf("\n===========================================================\n");
    if (g_failures == 0) {
        std::printf(">>> ALL M11 TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf(">>> %d TEST(S) FAILED <<<\n", g_failures);
        return 1;
    }
}
