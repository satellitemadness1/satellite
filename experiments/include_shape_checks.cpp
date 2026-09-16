// The five spellings of satellite.include, read straight out of the bytecode.
#include "bytecode/include_shape.hpp"
#include <cstdio>
#include <string>
using namespace satellite004;

static int bad = 0;
static void check(bool ok, const std::string &what) {
    printf("  %s  %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) ++bad;
}
static const char *kind_name(IncludeShape::Kind k) {
    switch (k) {
    case IncludeShape::Kind::main_marker: return "main_marker";
    case IncludeShape::Kind::bare_name:   return "bare_name";
    case IncludeShape::Kind::bare_path:   return "bare_path";
    case IncludeShape::Kind::quoted_path: return "quoted_path";
    default: return "none";
    }
}
static IncludeShape one(const std::string &line, const std::string &from) {
    std::vector<std::bitset<16>> row;
    tokenise_one_line(line, row);
    std::size_t at = 0;
    return include_at(row, at, from);
}

int main() {
    printf("the author's five spellings, from test_programs/hello_world.satl:\n");
    struct Case { const char *line; IncludeShape::Kind kind; const char *resolved; const char *name; };
    const Case cases[] = {
        {"satellite.include(satellite)",                    IncludeShape::Kind::main_marker, "",                                   ""},
        {"satellite.include(test_file)",                    IncludeShape::Kind::bare_name,   "test_programs/test_file.satl",       "test_file"},
        {"satellite.include(\"another_test_file.satl\")",   IncludeShape::Kind::quoted_path, "test_programs/another_test_file.satl", "another_test_file"},
        {"satellite.include(test_dir/test_file)",           IncludeShape::Kind::bare_path,   "test_programs/test_dir/test_file.satl", "test_file"},
        {"satellite.include(\"/test_dir/final_test_file\")",IncludeShape::Kind::quoted_path, "/test_dir/final_test_file.satl",     "final_test_file"},
    };
    for (const Case &c : cases) {
        const IncludeShape s = one(c.line, "test_programs/hello_world.satl");
        printf("    %-52s -> %-12s %s\n", c.line, kind_name(s.kind), s.resolved.c_str());
        check(s.kind == c.kind, std::string("  kind is ") + kind_name(c.kind));
        check(s.resolved == c.resolved, std::string("  resolves to ") + c.resolved);
        check(s.name == c.name, std::string("  names the spaceship \"") + c.name + "\"");
    }

    printf("\n003's rules, ported as decisions (revision 07, be50b10):\n");
    {   const IncludeShape s = one("satellite.include(\"../shared/log\")", "parts/ship.satl");
        check(s.resolved == "parts/../shared/log.satl",
              "relative to the INCLUDING file's directory, not the working one");
    }
    {   const IncludeShape s = one("satellite.include(\"parts/ship.satl\")", "main.satl");
        check(s.resolved == "parts/ship.satl", "the extension is not doubled when written");
        check(s.name == "ship", "named by the file STEM, not the path");
    }
    {   const IncludeShape s = one("satellite.include(ship)", "main.satl");
        check(s.resolved == "ship.satl", "a bare name beside a file with no directory");
    }
    {   const IncludeShape s = one("satellite.include(\"parts/ship\"(1, \"two\"))", "main.satl");
        check(s.resolved == "parts/ship.satl", "arguments after the path do not disturb it");
    }

    printf("\nthe whitespace sensitivity of the unquoted path:\n");
    {   const IncludeShape spaced = one("satellite.include(test_dir / test_file)", "main.satl");
        check(spaced.kind != IncludeShape::Kind::bare_path,
              "test_dir / test_file with spaces is DIVISION, not a path (so not bare_path)");
    }

    printf("\nnot an include:\n");
    {   std::vector<std::bitset<16>> row;
        tokenise_one_line("satellite.console.display(\"x\")", row);
        std::size_t at = 0;
        const IncludeShape s = include_at(row, at, "main.satl");
        check(s.kind == IncludeShape::Kind::none && at == 0, "a different word answers none and does not move");
    }

    printf("\n%s\n", bad == 0 ? "all checks passed" : (std::to_string(bad) + " FAILED").c_str());
    return bad != 0;
}
