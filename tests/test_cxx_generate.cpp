// How a block's text becomes a translation unit.
//
// These are all regressions from one bug: a whole line was hoisted to file
// scope whenever it began with '#', which destroyed the most natural way to
// write a short block --
//
//     satellite.cxx() { #include <iostream> std::cout << "hello"; }
//
// -- because the preprocessor takes <iostream> and throws the rest of the line
// away as "extra tokens at end of #include directive".  That is a WARNING.  The
// block compiled, ran, printed nothing, and reported success.
//
// Nothing here invokes a compiler; generate() is a pure text transform, which
// is exactly why it is worth testing directly.
#include "satellite/cxx.hpp"

#include <cstdio>
#include <string>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

// Is `needle` inside the generated FUNCTION BODY rather than at file scope?
// The body starts at the try, so everything after it is inside the function.
static bool in_body(const std::string& unit, const std::string& needle) {
    size_t tryat = unit.find("    try {");
    size_t at    = unit.find(needle);
    return tryat != std::string::npos && at != std::string::npos && at > tryat;
}

static bool in_prologue(const std::string& unit, const std::string& needle) {
    size_t tryat = unit.find("    try {");
    size_t at    = unit.find(needle);
    return tryat != std::string::npos && at != std::string::npos && at < tryat;
}

int main() {
    init_tables();
    printf("satellite.cxx code generation tests\n");

    // ---- the bug: one line, include then code -----------------------------
    {
        std::string u = cxx::generate("#include <iostream> std::cout<<\"hello\";\n");
        check(in_prologue(u, "#include <iostream>"),
              "the include is lifted to file scope");
        check(in_body(u, "std::cout<<\"hello\";"),
              "the code sharing its line stays in the function");
        check(u.find("#include <iostream> std::cout") == std::string::npos,
              "and the two are no longer on the same line");
    }
    {
        // The same, with the stray semicolon a person naturally types.
        std::string u = cxx::generate("#include <iostream>; std::cout<<\"A\";\n");
        check(in_prologue(u, "#include <iostream>"), "include lifted, semicolon form");
        check(in_body(u, "std::cout<<\"A\";"), "code survives the semicolon");
        check(u.find("#include <iostream>;") == std::string::npos,
              "the semicolon does not travel with the directive");
    }
    {
        std::string u = cxx::generate(
            "#include <iostream> #include <vector> std::vector<int> v; v.size();\n");
        check(in_prologue(u, "#include <iostream>"), "first of two includes lifted");
        check(in_prologue(u, "#include <vector>"),   "second of two includes lifted");
        check(in_body(u, "std::vector<int> v;"),     "trailing code still lands in the body");
    }
    {
        std::string u = cxx::generate("#include \"local.hpp\" int x = 1;\n");
        check(in_prologue(u, "#include \"local.hpp\""), "the quoted include form splits too");
        check(in_body(u, "int x = 1;"), "and its trailing code survives");
    }

    // ---- directives that genuinely run to end of line ----------------------
    {
        // There is no honest way to tell where a #define stops and code begins,
        // so these still take the whole line.  Splitting them would be a guess.
        std::string u = cxx::generate("#define N 7\n#include <cstdio>\nprintf(\"%d\", N);\n");
        check(in_prologue(u, "#define N 7"), "#define stays whole and at file scope");
        check(in_prologue(u, "#include <cstdio>"), "a normal include still works");
        check(in_body(u, "printf"), "and the body is unaffected");
    }

    // ---- `using namespace std; code` on one line ---------------------------
    {
        // This one failed LOUDLY rather than silently -- trailing code at file
        // scope does not compile -- but it is the same mistake.
        std::string u = cxx::generate("using namespace foo; bar();\n");
        check(in_prologue(u, "using namespace foo;"), "the using-directive is lifted");
        check(in_body(u, "bar();"), "the code after it is not");
    }

    // ---- std:: is optional inside a block ----------------------------------
    {
        // A block is a snippet that nothing ever #includes, so the usual
        // objection to a using-directive does not apply.  The JIT preamble
        // already did this; without it here the same line behaved differently
        // depending on which compiler ran it.
        std::string u = cxx::generate("cout << 1;\n");
        check(in_prologue(u, "using namespace std;"),
              "using namespace std is injected");
        check(u.find("using namespace satellite;") != std::string::npos,
              "alongside the satellite namespaces");
    }

    // ---- the ordinary multi-line shape still works -------------------------
    {
        std::string u = cxx::generate(
            "#include <string>\n"
            "std::string s = \"x\";\n"
            "satellite.return(s)\n");
        check(in_prologue(u, "#include <string>"), "a lone include line is lifted");
        check(in_body(u, "std::string s"), "ordinary statements stay in the body");
        check(in_body(u, "to_container(s)"), "satellite.return becomes to_container");
        check(u.find("satellite.return") == std::string::npos,
              "and the marker itself is gone");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
