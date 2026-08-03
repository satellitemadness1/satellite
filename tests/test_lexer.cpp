#include "satellite/lexer.hpp"

#include <cstdio>
#include <string>
#include <vector>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

// renders a token stream compactly:  satellite . ident(main) ( ) ;
static std::string render(const std::vector<Token>& ts) {
    std::string s;
    for (const auto& t : ts) {
        if (t.kind == Tok::End) break;
        if (!s.empty()) s += ' ';
        switch (t.kind) {
            case Tok::Ident: s += "id(" + t.text + ")";        break;
            case Tok::Str:   s += "str(" + t.text + ")";       break;
            case Tok::Int:   s += "int(" + t.text + ")";       break;
            case Tok::Real:  s += "real(" + t.text + ")";      break;
            case Tok::Term:  s += ";";                         break;
            case Tok::Satellite: s += "SAT";                   break;
            default:         s += tok_name(t.kind);            break;
        }
    }
    return s;
}

static void expect(const std::string& src, const std::string& want, const std::string& what) {
    ++checks;
    Lexer lx(src);
    auto ts = lx.scan();
    std::string got = render(ts);
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want %s\n        got  %s\n",
               what.c_str(), want.c_str(), got.c_str());
    }
}

int main() {
    printf("satellite lexer tests\n");

    // ---- the one reserved word --------------------------------------------
    expect("satellite", "SAT ;", "bare satellite is the keyword (and ends a statement)");
    expect("satellite.include(satellite)",
           "SAT . id(include) ( SAT ) ;",
           "satellite.include(satellite)");

    // THE rule that makes the design work: after a dot it is just a name
    expect("satellite.library.my_capsule.satellite",
           "SAT . id(library) . id(my_capsule) . id(satellite) ;",
           "satellite after a dot is an identifier, not the keyword");
    {
        Lexer lx("satellite.library.x.satellite");
        auto ts = lx.scan();
        check(ts[0].kind == Tok::Satellite, "leading satellite is the keyword");
        check(ts[6].kind == Tok::Ident && ts[6].text == "satellite",
              "trailing satellite carries its name as an identifier");
    }

    // every other word belongs to the user
    expect("capsule spacesuit class return include if while for",
           "id(capsule) id(spacesuit) id(class) id(return) id(include) "
           "id(if) id(while) id(for) ;",
           "no word other than satellite is reserved");

    // ---- newline as terminator, Go style ----------------------------------
    expect("a = 1\nb = 2\n",
           "id(a) = int(1) ; id(b) = int(2) ;",
           "a newline after a complete statement terminates it");

    expect("a = 1 +\n2\n",
           "id(a) = int(1) + int(2) ;",
           "a line ending in '+' continues onto the next");

    expect("f(\n  1,\n  2\n)\n",
           "id(f) ( int(1) , int(2) ) ;",
           "newlines inside parentheses never terminate");
    expect("x = a[\n  0\n]\n",
           "id(x) = id(a) [ int(0) ] ;",
           "newlines inside brackets never terminate");
    expect("{\n  a = 1\n  b = 2\n}\n",
           "{ id(a) = int(1) ; id(b) = int(2) ; } ;",
           "braces do NOT suppress terminators -- statements in a body still end");

    expect("a = 1", "id(a) = int(1) ;",
           "end of file terminates the last statement without a newline");

    expect("\n\n\na = 1\n\n\n", "id(a) = int(1) ;",
           "blank lines produce no stray terminators");

    // ---- comments ----------------------------------------------------------
    expect("a = 1 // trailing comment\nb = 2\n",
           "id(a) = int(1) ; id(b) = int(2) ;",
           "line comment does not eat the terminator");
    expect("// whole line\na = 1\n", "id(a) = int(1) ;", "full-line comment");
    expect("a /* inline */ = 1\n", "id(a) = int(1) ;", "block comment");

    // ---- literals ----------------------------------------------------------
    expect("\"hello\"", "str(hello) ;", "string literal");
    expect("\"tab\\there\"", "str(tab\there) ;", "escape sequences decode");
    expect("\"\"", "str() ;", "empty string");
    expect("42", "int(42) ;", "integer");
    expect("3.5", "real(3.5) ;", "real");
    expect("1_000_000", "int(1000000) ;", "digit separators");
    expect("1e3", "real(1e3) ;", "exponent");
    // a dot after digits is member access unless a digit follows
    expect("5.to_string()", "int(5) . id(to_string) ( ) ;",
           "5.to_string is not a decimal");

    // ---- operators ---------------------------------------------------------
    expect("a == b != c <= d >= e < f > g",
           "id(a) == id(b) != id(c) <= id(d) >= id(e) < id(f) > id(g) ;",
           "comparison operators");
    expect("a + b - c * d / e % f",
           "id(a) + id(b) - id(c) * id(d) / id(e) % id(f) ;",
           "arithmetic operators");

    // ---- a real satellite program ------------------------------------------
    const char* prog =
        "satellite.include(satellite)\n"
        "\n"
        "satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)\n"
        "{\n"
        "    satellite.console.display(arguments[0])\n"
        "    satellite.return(satellite)\n"
        "}\n";
    {
        Lexer lx(prog);
        auto ts = lx.scan();
        check(lx.ok(), "the example program lexes without errors");

        int sat = 0, ident = 0, terms = 0;
        for (const auto& t : ts) {
            if (t.kind == Tok::Satellite) ++sat;
            if (t.kind == Tok::Ident)     ++ident;
            if (t.kind == Tok::Term)      ++terms;
        }
        check(sat == 9, "nine bare `satellite` keywords in the example");
        check(ident > 0, "identifiers were produced");
        // one per statement line; the generic '>' and ']' also close statements
        check(terms >= 4, "statements were terminated");

        // the generic type argument tokenizes as angle brackets
        std::string r = render(ts);
        check(r.find("< SAT . id(variable) . id(string) >") != std::string::npos,
              "list<satellite.variable.string> tokenizes as angle brackets");
    }

    // ---- errors are reported, not thrown -----------------------------------
    {
        Lexer lx("a = \"unterminated\nb = 1\n");
        auto ts = lx.scan();
        check(!lx.ok(), "unterminated string is an error");
        check(lx.errors()[0].message == "unterminated string", "error message");
        check(render(ts).find("id(b)") != std::string::npos,
              "lexing continues past a bad string");
    }
    {
        Lexer lx("a # b");
        lx.scan();
        check(!lx.ok(), "unexpected character is an error");
    }
    {
        Lexer lx("a /* never closed\n");
        lx.scan();
        check(!lx.ok(), "unterminated block comment is an error");
    }

    // ---- line numbers ------------------------------------------------------
    {
        Lexer lx("a\nb\nc\n");
        auto ts = lx.scan();
        check(ts[0].line == 1 && ts[2].line == 2 && ts[4].line == 3,
              "line numbers track across newlines");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
