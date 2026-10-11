// The five spellings of satellite.include, read straight out of the bytecode.
#include "bytecode/include_shape.hpp"
#include <cstdio>
#include <string>
#include <vector>
#include <cstdlib>
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
        {"satellite.include(\"/test_dir/final_test_file\")",IncludeShape::Kind::quoted_path, "test_programs/test_dir/final_test_file.satl", "final_test_file"},
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

    printf("\na leading slash is RELATIVE, and only the home directory is absolute:\n");
    {   const IncludeShape s = one("satellite.include(/test)", "main.satl");
        check(s.resolved == "test.satl", "include(/test) is ./test -- the filesystem root is never reached");
    }
    {   const IncludeShape s = one("satellite.include(\"/a/b/c\")", "parts/ship.satl");
        check(s.resolved == "parts/a/b/c.satl", "and a deeper one lands beside the file that wrote it");
    }
    {   const char *home = std::getenv("HOME");
        if (home != nullptr && home[0] != 0) {
            const std::string line = std::string("satellite.include(\"") + home + "/ships/ship\")";
            const IncludeShape s = one(line, "anywhere/main.satl");
            check(s.resolved == std::string(home) + "/ships/ship.satl",
                  "a path under the user's HOME is left alone -- the one absolute form");
        }
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

    printf("\nno globals: a file is runnable only with include(satellite), main and return:\n");
    {   MachineState state; state.debug_mode = false;
        struct Run { const char *what; const char *src; signed long long int want; };
        const Run runs[] = {
            {"a whole program",
             "satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\nsatellite.return(satellite)\n}\n", success},
            {"a spaceship: no include(satellite)",
             "satellite.capsule some_capsule()\n{\nsatellite.return(satellite)\n}\n",
             satl_file_missing_satellite_include_satellite},
            {"marked runnable but has no main",
             "satellite.include(satellite)\nsatellite.return(satellite)\n", satl_file_missing_satellite_main},
            {"a main that never returns",
             "satellite.include(satellite)\nsatellite.capsule satellite.main()\n{\n}\n",
             satl_file_missing_satellite_return_satellite},
            {"an empty file", "", satl_file_missing_satellite_include_satellite},
            {"a STRING saying satellite.main does not count",
             "satellite.include(satellite)\nsatellite.console.display(\"satellite.main\")\n",
             satl_file_missing_satellite_main},
        };
        for (const Run &r : runs) {
            std::vector<std::bitset<16>> row;
            for (std::string line, rest = r.src; !rest.empty(); ) {
                const std::size_t nl = rest.find('\n');
                line = rest.substr(0, nl == std::string::npos ? rest.size() : nl);
                tokenise_one_line(line, row);
                if (nl == std::string::npos) break;
                rest = rest.substr(nl + 1);
            }
            const signed long long int got = file_can_run(row, "t.satl", state);
            check(got == r.want, std::string(r.what) + " -> " + std::to_string(got));
        }
    }

    printf("\n%s\n", bad == 0 ? "all checks passed" : (std::to_string(bad) + " FAILED").c_str());
    return bad != 0;
}
