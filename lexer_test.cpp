// Lexer tests: the code table's character classes, the token shapes the
// grammar depends on, and the two traps the code table sets — underscore
// living in the punctuation block, and whitespace living in the raw area.

#include "lexer.hpp"
#include "satellite_string.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;

static int failures = 0;

static void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

// Join a lexed line into "Word(a) Punct(.) Word(b) End" for comparison.
static std::string shape(const std::string &source)
{
    std::string out;
    for (const Token &t : lex(source)) {
        if (!out.empty())
            out += " ";
        out += describe(t);
    }
    return out;
}

static void check_shape(const std::string &source, const std::string &want)
{
    std::string got = shape(source);
    if (got != want) {
        printf("FAIL: %s\n  want: %s\n   got: %s\n", source.c_str(),
               want.c_str(), got.c_str());
        failures++;
    }
}

static size_t count(const std::string &source)
{
    return lex(source).size();
}

int main()
{
    // ---- underscore is an identifier character ----------------------------
    // '_' is punctuation code 74 in the table. If the lexer classified by the
    // table alone, my_time would be three tokens and every declaration in the
    // language would break.
    check(encode("_") == SatString{SAT_UNDERSCORE}, "'_' encodes to 74");
    check(SAT_UNDERSCORE == 74, "SAT_UNDERSCORE is 74");

    check_shape("my_time", "Word(my_time) End");
    check_shape("list_name", "Word(list_name) End");
    check_shape("some_number_start", "Word(some_number_start) End");
    check_shape("_leading", "Word(_leading) End");
    check_shape("x2_y", "Word(x2_y) End");
    // A digit still cannot start a word.
    check_shape("2my", "Number(2) Word(my) End");

    // ---- whitespace lives in the raw area ---------------------------------
    // Space has no code-table entry, so it arrives as SAT_RAW_BASE + ' '.
    // A lexer testing isspace() on a SatChar would skip nothing and emit an
    // Error per space.
    check_shape("  a\tb  ", "Word(a) Word(b) End");
    for (const Token &t : lex("   x   y   "))
        check(t.kind != TokenKind::Error, "spaces produce no Error tokens");
    check(lex("\n\n\n").size() == 1, "a blank source is just End");

    // ---- the declaration form --------------------------------------------
    check_shape(
        "satellite.variable.time my_time = satellite.time.now()",
        "Word(satellite) Punct(.) Word(variable) Punct(.) Word(time) "
        "Word(my_time) Punct(=) Word(satellite) Punct(.) Word(time) "
        "Punct(.) Word(now) Punct(() Punct()) End");

    // Two adjacent Words with nothing between them is the signal that the
    // type path ended and the name began. Whitespace is load-bearing without
    // being a token: remove it and the two Words glue into one.
    check_shape("satellite.variable.timemy_time",
                "Word(satellite) Punct(.) Word(variable) Punct(.) "
                "Word(timemy_time) End");
    // Spaces around the dots, however, are free.
    check(shape("satellite . variable . time my_time") ==
              shape("satellite.variable.time my_time"),
          "spaces around dots do not change the token stream");

    // An uninitialised declaration still ends in two adjacent Words, a shape
    // no expression can produce.
    check_shape("satellite.variable.file my_file",
                "Word(satellite) Punct(.) Word(variable) Punct(.) Word(file) "
                "Word(my_file) End");

    // ---- receiver method calls -------------------------------------------
    check_shape("my_time.some_function()",
                "Word(my_time) Punct(.) Word(some_function) "
                "Punct(() Punct()) End");

    // ---- generics ---------------------------------------------------------
    // '>>' must lex as two independent '>' so parse_type's recursion can
    // consume one per nesting level.
    check_shape("satellite.container.list<satellite.variable.string> argz",
                "Word(satellite) Punct(.) Word(container) Punct(.) Word(list) "
                "Punct(<) Word(satellite) Punct(.) Word(variable) Punct(.) "
                "Word(string) Punct(>) Word(argz) End");
    {
        std::vector<Token> t = lex(
            "satellite.container.list<satellite.container.list"
            "<satellite.variable.string>> grid");
        size_t closers = 0;
        for (const Token &tok : t)
            if (tok.kind == TokenKind::Punct && tok.text == ">")
                closers++;
        check(closers == 2, "nested generics close with two '>' tokens");
        for (const Token &tok : t)
            check(tok.text != ">>", "'>>' is never one token");
    }

    // ---- numbers ----------------------------------------------------------
    check_shape("3.14", "Number(3.14) End");
    check_shape("main.x", "Word(main) Punct(.) Word(x) End");
    check_shape("3.", "Number(3) Punct(.) End");
    // A sign is never folded in, or a-1 would become two adjacent operands.
    check_shape("-1", "Punct(-) Number(1) End");
    check_shape("a-1", "Word(a) Punct(-) Number(1) End");
    // Exact on the way in: the digits become an exact decimal, so a literal
    // longer than a double's 17 significant digits keeps every one of them.
    check(lex("3.14")[0].number.to_string() == "3.14", "3.14 parses to 3.14");
    check(lex("42")[0].number == Number(42), "42 parses to 42");
    check(lex("3.141592653589793238462643383279")[0].number.to_string() ==
              "3.141592653589793238462643383279",
          "a 31-digit literal keeps all 31 digits");

    // ---- indexing and slicing --------------------------------------------
    check_shape("list_name[some_number]",
                "Word(list_name) Punct([) Word(some_number) Punct(]) End");
    check_shape("l[2:5]",
                "Word(l) Punct([) Number(2) Punct(:) Number(5) Punct(]) End");
    check_shape("l[:]", "Word(l) Punct([) Punct(:) Punct(]) End");

    // ---- strings ----------------------------------------------------------
    check(lex("\"hello, world!\"")[0].kind == TokenKind::String,
          "a quoted run is a String token");
    check(decode(lex("\"hello, world!\"")[0].str) == "hello, world!",
          "string body round-trips");
    // Escapes are expanded in the body, and only in the body.
    check(decode(lex("\"a\\nb\"")[0].str) == "a\nb", "\\n is a real newline");
    check(decode(lex("\"a\\tb\"")[0].str) == "a\tb", "\\t is a real tab");
    check(decode(lex("\"C:\\\\home\"")[0].str) == "C:\\home",
          "\\\\ is a literal backslash, not the home directory");
    // An escaped quote must not end the literal.
    check(lex("\"say \\\"hi\\\"\"")[0].kind == TokenKind::String,
          "\\\" does not close the string");
    check(decode(lex("\"say \\\"hi\\\"\"")[0].str) == "say \"hi\"",
          "\\\" body round-trips");
    // A system char inside a string is one satellite character that expands
    // when displayed.
    check(lex("\"\\user\"")[0].str == SatString{SAT_LINUX_USERNAME},
          "\\user inside a string is one satellite char");

    // ---- errors never throw ----------------------------------------------
    {
        std::vector<Token> t = lex("\"unterminated");
        check(t.size() == 2, "an error still ends the stream with End");
        check(t[0].kind == TokenKind::Error, "unterminated string is an Error");
        check(t.back().kind == TokenKind::End, "the last token is always End");
        check(t[0].start == 0, "the Error carries its position");
    }
    check(lex("").size() == 1 && lex("")[0].kind == TokenKind::End,
          "empty source lexes to End alone");

    // ---- comments and lines ----------------------------------------------
    check_shape("a // trailing comment", "Word(a) End");
    check_shape("// whole line", "End");
    check(count("a\nb\nc") == 4, "newlines separate but do not tokenize");
    {
        std::vector<Token> t = lex("a\nb\n\nc");
        check(t[0].line == 1 && t[1].line == 2 && t[2].line == 4,
              "line numbers survive comments and blank lines");
    }
    {
        // The postfix-'[' rule the parser needs depends on this.
        std::vector<Token> t = lex("display(x)\n[1,2,3]");
        check(t[0].line == 1, "first line is 1");
        check(t[4].line == 2, "the '[' is known to be on the next line");
    }

    // ---- two-character operators -----------------------------------------
    check_shape("a == b", "Word(a) Punct(==) Word(b) End");
    check_shape("a != b", "Word(a) Punct(!=) Word(b) End");
    check_shape("a <= b", "Word(a) Punct(<=) Word(b) End");
    check_shape("a >= b", "Word(a) Punct(>=) Word(b) End");
    check_shape("a = b", "Word(a) Punct(=) Word(b) End");
    check_shape("a < b", "Word(a) Punct(<) Word(b) End");

    // split_punct lets a parser expecting a generic close take the '>' out
    // of a greedily-matched '>='.
    {
        std::vector<Token> t = lex("a >= b");
        check(split_punct(t, 1), "a two-char Punct splits");
        check(t[1].text == ">" && t[2].text == "=", "'>=' splits into '>' '='");
        check(t[1].end == t[2].start, "the split halves stay adjacent");
        check(!split_punct(t, 0), "a Word does not split");
        check(!split_punct(t, 1), "a one-char Punct does not split");
        check(!split_punct(t, 999), "an out-of-range index does not split");
    }

    // ---- encode_raw is what source must be lexed with --------------------
    // encode() expands escapes everywhere, so lexing with it would rewrite a
    // program's text before the lexer ever saw it.
    check(encode_raw("C:\\home").size() == 7,
          "encode_raw leaves a backslash alone");
    check(encode("C:\\home") != encode_raw("C:\\home"),
          "encode and encode_raw genuinely differ");
    check(decode(encode_raw("my_time = list_name[a_b:c_d]")) ==
              "my_time = list_name[a_b:c_d]",
          "decode inverts encode_raw exactly");
    // One SatChar per input byte is what makes token offsets byte offsets,
    // which is what error carets need.
    {
        const std::string src = "satellite.variable.time my_time";
        check(encode_raw(src).size() == src.size(),
              "encode_raw is one SatChar per byte");
        std::vector<Token> t = lex(src);
        check(src.substr(t[5].start, t[5].end - t[5].start) == "my_time",
              "token offsets index the original source directly");
    }

    // ---- hello world ------------------------------------------------------
    {
        const std::string hello =
            "satellite.include(satellite)\n"
            "\n"
            "satellite.capsule satellite.main("
            "satellite.container.list<satellite.variable.string> argz)\n"
            "{\n"
            "    // \"capsules\" are just functions in satellite\n"
            "    satellite.console.display(\"hello, world!\")\n"
            "    satellite.return(satellite) // return(0); in satellite\n"
            "}\n";
        std::vector<Token> t = lex(hello);
        for (const Token &tok : t)
            check(tok.kind != TokenKind::Error, "hello world lexes cleanly");
        check(t.back().kind == TokenKind::End, "hello world ends with End");

        size_t strings = 0;
        for (const Token &tok : t)
            if (tok.kind == TokenKind::String)
                strings++;
        check(strings == 1,
              "the commented-out quotes are not a string literal");

        bool found = false;
        for (const Token &tok : t)
            if (tok.kind == TokenKind::String &&
                decode(tok.str) == "hello, world!")
                found = true;
        check(found, "hello, world! survives the lexer");
    }

    if (failures) {
        printf("%d lexer check(s) failed\n", failures);
        return 1;
    }
    printf("PASS: lexer (underscore in words, whitespace in the raw area, "
           "'>>' as two tokens, escapes only inside strings)\n");
    return 0;
}
