#include "satellite/cxx.hpp"
#include "satellite/lexer.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

static void eqs(const std::string& got, const std::string& want, const std::string& what) {
    ++checks;
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n",
               what.c_str(), want.c_str(), got.c_str());
    }
}

// the raw text of the first cxx block in a source string
static std::string block_of(const std::string& src, bool* found = nullptr) {
    Lexer lx(src);
    auto ts = lx.scan();
    for (const auto& t : ts)
        if (t.kind == Tok::CxxBlock) { if (found) *found = true; return t.text; }
    if (found) *found = false;
    return "";
}

int main() {
    init_tables();
    printf("satellite.cxx tests\n");

    // =======================================================================
    // the lexer hands the block through raw, without lexing it
    // =======================================================================
    {
        bool found = false;
        std::string b = block_of("satellite.cxx { int x = 1 }", &found);
        check(found, "a cxx block is recognised");
        eqs(b, " int x = 1 ", "the body comes through verbatim");
    }
    {
        std::string b = block_of("satellite.cxx {\n  if (a) { b() }\n}");
        check(b.find("if (a) { b() }") != std::string::npos,
              "nested braces do not end the block early");
    }
    {
        std::string b = block_of("satellite.cxx { s = \"}\" }");
        check(b.find("\"}\"") != std::string::npos,
              "a brace inside a string does not end the block");
    }
    {
        std::string b = block_of("satellite.cxx { c = '}' }");
        check(b.find("'}'") != std::string::npos,
              "a brace inside a char literal does not end the block");
    }
    {
        std::string b = block_of("satellite.cxx { // }\n x = 1 }");
        check(b.find("x = 1") != std::string::npos,
              "a brace inside a line comment does not end the block");
    }
    {
        std::string b = block_of("satellite.cxx { /* } */ x = 1 }");
        check(b.find("x = 1") != std::string::npos,
              "a brace inside a block comment does not end the block");
    }
    {
        std::string b = block_of("satellite.cxx { s = R\"(})\" \n y = 2 }");
        check(b.find("y = 2") != std::string::npos,
              "a brace inside a raw string does not end the block");
    }
    {
        // satellite code that merely mentions cxx without a block is untouched
        bool found = true;
        block_of("satellite.cxx(3)", &found);
        check(!found, "satellite.cxx without a brace is not a block");
    }
    {
        Lexer lx("satellite.cxx { unterminated");
        lx.scan();
        check(!lx.ok(), "an unterminated block is an error");
    }
    {
        // the satellite code around a block still lexes normally
        Lexer lx("a = 1\nsatellite.cxx { int q = 2 }\nb = 3\n");
        auto ts = lx.scan();
        check(lx.ok(), "surrounding satellite code lexes cleanly");
        int blocks = 0, idents = 0;
        for (const auto& t : ts) {
            if (t.kind == Tok::CxxBlock) ++blocks;
            if (t.kind == Tok::Ident)    ++idents;
        }
        check(blocks == 1, "exactly one block");
        check(idents == 2, "a and b are still identifiers -- q is not, it is C++");
    }

    // =======================================================================
    // satellite.return(EXPR) extraction
    // =======================================================================
    eqs(cxx::return_expression("satellite.return(my_str)"), "my_str", "simple name");
    eqs(cxx::return_expression("x = 1\nsatellite.return( total )"), "total",
        "surrounding whitespace is trimmed");
    eqs(cxx::return_expression("satellite.return(f(x, g(y)))"), "f(x, g(y))",
        "nested parentheses are matched");
    eqs(cxx::return_expression("int x = 1"), "", "a block may return nothing");

    // =======================================================================
    // code generation
    // =======================================================================
    {
        std::string unit = cxx::generate(
            "#include <string>\n"
            "using namespace std\n"
            "std::string s = \"hi\";\n"
            "satellite.return(s)\n");

        check(unit.find("extern \"C\" satellite::Container satellite_cxx_block") !=
                  std::string::npos, "an extern \"C\" entry point is generated");
        check(unit.find("satellite_cxx_abi") != std::string::npos, "ABI symbol emitted");
        check(unit.find("to_container(s)") != std::string::npos,
              "the return runs through to_container -- overload resolution converts");
        check(unit.find("catch (...)") != std::string::npos,
              "exceptions are caught so none escape into the interpreter");

        // #include must be at file scope, above the function
        size_t inc = unit.find("#include <string>");
        size_t fn  = unit.find("satellite_cxx_block");
        check(inc != std::string::npos && inc < fn,
              "#include is hoisted above the function body");
        size_t un = unit.find("using namespace std");
        check(un != std::string::npos && un < fn, "using namespace is hoisted too");
    }
    {
        std::string unit = cxx::generate("int x = 1\n");
        check(unit.find("Container::nil()") != std::string::npos,
              "a block with no return yields nil");
    }

    // =======================================================================
    // compile and run for real
    // =======================================================================
    const char* tmp = std::getenv("TMPDIR");
    cxx::CxxConfig cfg = cxx::default_config();
    cfg.cache_dir = std::string(tmp ? tmp : "/tmp") + "/satellite-cxx-test";
    cxx::Bridge bridge(cfg);

    {
        std::string err;
        Container r = bridge.run(
            "#include <string>\n"
            "std::string my_str = \"something\";\n"
            "my_str += \" from C++\";\n"
            "satellite.return(my_str)\n", &err);
        check(err.empty(), "the block compiles: " + err);
        eqs(r.to_string(), "something from C++", "std::string converts to a satellite string");
        check(r.type == Type::Str, "and arrives as Type::Str");
    }
    {
        std::string err;
        Container r = bridge.run("int total = 0;\n"
                                 "for (int k = 1; k <= 100; ++k) total += k;\n"
                                 "satellite.return(total)\n", &err);
        check(err.empty(), "an int block compiles: " + err);
        eqs(r.to_string(), "5050", "int converts to a satellite Int");
        check(r.type == Type::Int, "and arrives as Type::Int");
    }
    {
        std::string err;
        Container r = bridge.run("double d = 1.5 * 2.0;\nsatellite.return(d)\n", &err);
        check(err.empty(), "a double block compiles: " + err);
        check(r.type == Type::Real, "double converts to Type::Real");
    }
    {
        // a std::vector becomes a satellite list, through the template overload
        std::string err;
        Container r = bridge.run(
            "#include <vector>\n"
            "std::vector<int> v = {1, 2, 3};\n"
            "satellite.return(v)\n", &err);
        check(err.empty(), "a vector block compiles: " + err);
        eqs(r.to_string(), "[1, 2, 3]", "std::vector converts to a satellite list");
        check(r.type == Type::List, "and arrives as Type::List");
    }
    {
        // templates and RAII are fine INSIDE the block -- only the boundary is C
        std::string err;
        Container r = bridge.run(
            "#include <vector>\n"
            "#include <algorithm>\n"
            "template <typename T> T biggest(std::vector<T> v) {\n"
            "    return *std::max_element(v.begin(), v.end());\n"
            "}\n"
            "satellite.return(biggest<int>({4, 9, 2}))\n", &err);
        check(err.empty(), "templates work inside a block: " + err);
        eqs(r.to_string(), "9", "template result converts");
    }

    // ---- caching -----------------------------------------------------------
    {
        size_t before_compiles = bridge.compiles();
        size_t before_hits     = bridge.cache_hits();
        std::string src = "int x = 7;\nsatellite.return(x)\n";
        bridge.run(src);
        size_t mid = bridge.compiles();
        bridge.run(src);                       // identical source
        check(bridge.compiles() == mid, "identical source is not recompiled");
        check(bridge.cache_hits() > before_hits, "it is served from the cache");
        (void)before_compiles;
    }

    // ---- errors ------------------------------------------------------------
    {
        std::string err;
        Container r = bridge.run("this is not valid C++ at all !!!\n", &err);
        check(!err.empty(), "a compile error is reported");
        check(r.type == Type::Nil, "and the result is nil");
        check(err.find("error") != std::string::npos,
              "the compiler's own message comes through");
    }
    {
        // an exception thrown inside must not escape into the interpreter
        std::string err;
        Container r = bridge.run(
            "#include <stdexcept>\n"
            "throw std::runtime_error(\"boom\");\n"
            "satellite.return(1)\n", &err);
        check(err.empty(), "the throwing block still compiles");
        check(r.to_string().find("boom") != std::string::npos,
              "the exception is caught and returned as a value");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
