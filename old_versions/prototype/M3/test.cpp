// Test runner and CLI tool for the Milestone 3 Lexer prototype in flash/

#include "lexer.hpp"
#include "satellite_string/satellite_string.hpp"
#include "satellite_words/words.hpp"

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
    std::fprintf(stderr, "Could not open file: %s (or ../%s or ../../%s)\n",
                 path.c_str(), path.c_str(), path.c_str());
    return "";
}

void test_basic_tokens()
{
    // Empty source
    {
        auto toks = satellite::lex("");
        check(toks.size() == 1, "empty source produces 1 token");
        check(toks[0].kind == satellite::TokenKind::End, "empty source ends with End");
    }

    // Whitespace and newlines
    {
        auto toks = satellite::lex("   \t  \n  \n ");
        check(toks.size() == 3, "2 newlines + End = 3 tokens");
        check(toks[0].kind == satellite::TokenKind::Newline, "token 0 is Newline");
        check(toks[0].line == 1, "first newline is on line 1");
        check(toks[1].kind == satellite::TokenKind::Newline, "token 1 is Newline");
        check(toks[1].line == 2, "second newline is on line 2");
        check(toks[2].kind == satellite::TokenKind::End, "token 2 is End");
        check(toks[2].line == 3, "End token is on line 3");
    }

    // Words and Word IDs
    {
        auto toks = satellite::lex("satellite console display local_var hexadecimal args");
        check(toks.size() == 7, "6 words + End = 7 tokens");

        check(toks[0].kind == satellite::TokenKind::Word && toks[0].text == "satellite", "satellite word");
        check(toks[0].word_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::SATELLITE), "satellite has root PathId 1");

        check(toks[1].kind == satellite::TokenKind::Word && toks[1].text == "console", "console word");
        check(toks[1].word_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::CONSOLE), "console has PathId");

        check(toks[2].kind == satellite::TokenKind::Word && toks[2].text == "display", "display word");
        check(toks[2].word_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::CONSOLE_DISPLAY), "display has PathId");

        check(toks[3].kind == satellite::TokenKind::Word && toks[3].text == "local_var", "local_var word");
        check(toks[3].word_id == satellite::words::kNoPath, "local_var has kNoPath");

        check(toks[4].kind == satellite::TokenKind::Word && toks[4].text == "hexadecimal", "hexadecimal alias");
        check(toks[4].word_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::VARIABLE_HEX), "hexadecimal resolves to VARIABLE_HEX");

        check(toks[5].kind == satellite::TokenKind::Word && toks[5].text == "args", "args alias");
        check(toks[5].word_id == static_cast<satellite::words::PathId>(satellite::words::NodeId::LIBRARY_MAIN_ARGUMENTS), "args resolves to LIBRARY_MAIN_ARGUMENTS");
    }

    // Numbers & Unary minus rule
    {
        auto toks = satellite::lex("123 3.14 -42 3. .14");
        // 123 (Number)
        // 3.14 (Number)
        // - (Punct) 42 (Number) -- minus never folded into number (DESIGN §5.6)
        // 3 (Number) . (Punct)
        // . (Punct) 14 (Number)
        check(toks[0].kind == satellite::TokenKind::Number && toks[0].text == "123", "integer number");
        check(toks[1].kind == satellite::TokenKind::Number && toks[1].text == "3.14", "decimal number");
        check(toks[2].kind == satellite::TokenKind::Punct && toks[2].text == "-", "unary minus is separate punct");
        check(toks[3].kind == satellite::TokenKind::Number && toks[3].text == "42", "unsigned number follows minus");
        check(toks[4].kind == satellite::TokenKind::Number && toks[4].text == "3", "3 is number before dot");
        check(toks[5].kind == satellite::TokenKind::Punct && toks[5].text == ".", "trailing dot is punct");
        check(toks[6].kind == satellite::TokenKind::Punct && toks[6].text == ".", "leading dot is punct");
        check(toks[7].kind == satellite::TokenKind::Number && toks[7].text == "14", "number follows leading dot");
    }

    // Bits (Binary and Hex literals) vs Identifiers
    {
        auto toks = satellite::lex("x00FF b1010 xyz x2_y box b");
        check(toks[0].kind == satellite::TokenKind::Bits && toks[0].radix == 16 && toks[0].text == "x00FF", "hex literal x00FF");
        check(toks[1].kind == satellite::TokenKind::Bits && toks[1].radix == 2 && toks[1].text == "b1010", "binary literal b1010");
        check(toks[2].kind == satellite::TokenKind::Word && toks[2].text == "xyz", "xyz is Word");
        check(toks[3].kind == satellite::TokenKind::Word && toks[3].text == "x2_y", "x2_y is Word (underscore breaks bits)");
        check(toks[4].kind == satellite::TokenKind::Word && toks[4].text == "box", "box is Word");
        check(toks[5].kind == satellite::TokenKind::Word && toks[5].text == "b", "b alone is Word");
    }

    // Strings and Escapes
    {
        auto toks = satellite::lex("\"Hello, \\\"World\\\"!\\nLine 2\"");
        check(toks.size() == 2, "String + End");
        check(toks[0].kind == satellite::TokenKind::String, "is string token");
        check(toks[0].text == "Hello, \\\"World\\\"!\\nLine 2", "raw text preserved");
        std::string decoded = satellite::decode(toks[0].str);
        check(decoded.find("Hello, \"World\"!\nLine 2") != std::string::npos, "string escape expansion worked");
    }

    // Unterminated String
    {
        auto toks = satellite::lex("\"unterminated string\n");
        check(toks.size() == 2, "Error + End");
        check(toks[0].kind == satellite::TokenKind::Error, "unterminated string produces Error token");
        check(toks[0].text == "unterminated string literal", "error message accurate");
    }

    // Operators, Punctuation, and No << or >> rule
    {
        auto toks = satellite::lex("== <= >= != < > << >>");
        check(toks[0].text == "==", "==");
        check(toks[1].text == "<=", "<=");
        check(toks[2].text == ">=", ">=");
        check(toks[3].text == "!=", "!=");
        check(toks[4].text == "<", "<");
        check(toks[5].text == ">", ">");
        check(toks[6].text == "<" && toks[7].text == "<", "no << operator (DESIGN §5.5)");
        check(toks[8].text == ">" && toks[9].text == ">", "no >> operator (DESIGN §5.5)");
    }

    // split_punct
    {
        auto toks = satellite::lex("list<int>=");
        // tokens: list, <, int, >=, End
        check(toks.size() == 5, "5 tokens initially");
        check(toks[3].text == ">=", "token 3 is >=");
        bool ok = satellite::split_punct(toks, 3);
        check(ok, "split_punct returned true");
        check(toks.size() == 6, "now 6 tokens");
        check(toks[3].text == ">", "token 3 is >");
        check(toks[4].text == "=", "token 4 is =");
    }

    // Comments
    {
        auto toks = satellite::lex("// line comment\nx // comment 2\ny");
        check(toks[0].kind == satellite::TokenKind::Newline, "newline after first comment");
        check(toks[1].kind == satellite::TokenKind::Word && toks[1].text == "x", "x word");
        check(toks[2].kind == satellite::TokenKind::Newline, "newline after second comment");
        check(toks[3].kind == satellite::TokenKind::Word && toks[3].text == "y", "y word");
    }
}

void test_example_file(const std::string &path)
{
    std::string src = read_file(path);
    if (src.empty())
        return;

    auto toks = satellite::lex(src);
    check(!toks.empty(), "lexed tokens not empty");
    check(toks.back().kind == satellite::TokenKind::End, "ends with End");

    bool has_error = false;
    for (const auto &t : toks) {
        if (t.kind == satellite::TokenKind::Error) {
            has_error = true;
            std::printf("Lex Error in %s: %s (line %u)\n", path.c_str(), t.text.c_str(), t.line);
        }
    }
    check(!has_error, ("no lex errors in " + path).c_str());
}

} // namespace

int main(int argc, char **argv)
{
    if (argc > 1) {
        // Run as CLI lexer tool on given file
        std::string path = argv[1];
        std::string src = read_file(path);
        if (src.empty())
            return 1;

        std::printf("Lexing '%s' (%zu bytes):\n", path.c_str(), src.size());
        auto toks = satellite::lex(src);
        for (size_t i = 0; i < toks.size(); ++i) {
            const auto &t = toks[i];
            std::printf("[%4zu] Line %3u [%3u..%3u) %-8s %s\n",
                        i, t.line, t.start, t.end,
                        satellite::kind_name(t.kind),
                        satellite::describe(t).c_str());
        }
        return 0;
    }

    std::printf("=== Running M3 Lexer Prototype Unit Tests ===\n");
    test_basic_tokens();

    std::printf("=== Testing all example programs ===\n");
    test_example_file("example/hello_world.satl");
    test_example_file("example/advanced.satl");
    test_example_file("example/class_test.satl");
    test_example_file("example/gui_example.satl");
    test_example_file("example/super_advanced.satl");
    test_example_file("example/thread_test.satl");

    if (g_failures == 0) {
        std::printf("\n>>> ALL M3 LEXER TESTS PASSED CLEANLY <<<\n");
        return 0;
    } else {
        std::printf("\n>>> %d TEST FAILURES <<<\n", g_failures);
        return 1;
    }
}

