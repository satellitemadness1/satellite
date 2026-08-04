// The satellite.cxx tunables.
//
// The property that actually matters here is the LAST one: every setting that
// changes generated code must change the cache key.  The cache is content-
// addressed, so a setting left out of the key is a setting that silently does
// nothing on every run after the first -- the worst kind of bug, because the
// feature appears to work and the evidence is a file on disk nobody looks at.
#include "satellite/cxx.hpp"

#include <cstdio>
#include <filesystem>
#include <cstdlib>
#include <string>

using namespace satellite;

static int failures = 0;
static int checks   = 0;

static void check(bool ok, const std::string& what) {
    ++checks;
    if (!ok) { ++failures; printf("  FAIL  %s\n", what.c_str()); }
}

static void eqs(const std::string& got, const std::string& want,
                const std::string& what) {
    ++checks;
    if (got != want) {
        ++failures;
        printf("  FAIL  %s\n        want [%s]\n        got  [%s]\n",
               what.c_str(), want.c_str(), got.c_str());
    }
}

int main() {
    init_tables();
    printf("satellite.cxx settings tests\n");

    // ---- flag assembly -----------------------------------------------------
    {
        cxx::CxxConfig c;
        eqs(c.compile_flags(), "-O2 -std=c++20", "defaults");

        c.opt_level = 3;
        eqs(c.compile_flags(), "-O3 -std=c++20", "opt_level reaches -O");

        c.native_arch = true;
        eqs(c.compile_flags(), "-O3 -std=c++20 -march=native", "native adds -march");

        c.fast_math = true;
        eqs(c.compile_flags(), "-O3 -std=c++20 -march=native -ffast-math",
            "fast_math adds -ffast-math");

        c.extra_flags = "-DFOO=1";
        eqs(c.compile_flags(), "-O3 -std=c++20 -march=native -ffast-math -DFOO=1",
            "extra flags are appended last");

        // The order is fixed on purpose: this string is part of the cache key,
        // so a set that reordered itself between runs would never hit cache.
        cxx::CxxConfig d = c;
        eqs(d.compile_flags(), c.compile_flags(), "identical settings, identical string");
    }

    // ---- environment overrides ---------------------------------------------
    {
        setenv("SATELLITE_CXX_OPT", "3", 1);
        setenv("SATELLITE_CXX_STD", "c++23", 1);
        setenv("SATELLITE_CXX_NATIVE", "on", 1);
        setenv("SATELLITE_CXX_CACHE", "off", 1);
        setenv("SATELLITE_CXX_COMPILER", "clang++", 1);
        cxx::CxxConfig c = cxx::default_config();
        check(c.opt_level == 3,          "SATELLITE_CXX_OPT is read");
        eqs(c.std_version, "c++23",      "SATELLITE_CXX_STD is read");
        check(c.native_arch,             "`on` is true");
        check(!c.cache_enabled,          "`off` is false");
        eqs(c.compiler, "clang++",       "the compiler can be swapped");

        // A typo must not silently flip a setting.  SATELLITE_CXX_CACHE=maybe
        // leaving the cache ON is the safe reading; turning it off would make
        // every run slow for a reason nobody could find.
        setenv("SATELLITE_CXX_CACHE", "maybe", 1);
        check(cxx::default_config().cache_enabled,
              "an unrecognised boolean leaves the default alone");

        // Out-of-range optimisation levels clamp rather than reaching the
        // compiler as -O9 and failing there.
        setenv("SATELLITE_CXX_OPT", "9", 1);
        check(cxx::default_config().opt_level == 3, "opt clamps to 3");
        setenv("SATELLITE_CXX_OPT", "-4", 1);
        check(cxx::default_config().opt_level == 0, "opt clamps to 0");

        unsetenv("SATELLITE_CXX_OPT");
        unsetenv("SATELLITE_CXX_STD");
        unsetenv("SATELLITE_CXX_NATIVE");
        unsetenv("SATELLITE_CXX_CACHE");
        unsetenv("SATELLITE_CXX_COMPILER");
        check(cxx::default_config().opt_level == 2, "unset restores the default");
    }

    // ---- THE ONE THAT MATTERS: settings are in the cache key ---------------
    {
        const char* tmp = std::getenv("TMPDIR");
        std::string dir = std::string(tmp ? tmp : "/tmp") + "/satellite-cfg-test";

        // Start from an empty cache.  Without this the test passes the first
        // time and fails every time after, because the modules it expects to
        // be compiled are already sitting on disk from the previous run --
        // which is exactly the behaviour under test, so it cannot also be the
        // starting state.
        std::error_code ec;
        std::filesystem::remove_all(dir, ec);

        const char* block = "satellite.return(6*7)\n";

        cxx::CxxConfig a = cxx::default_config();
        a.cache_dir = dir;
        a.opt_level = 2;
        cxx::Bridge ba(a);
        std::string so_a;
        check(ba.build(block, &so_a), "a block builds at -O2");

        // Same block, different optimisation level -> a DIFFERENT module.
        cxx::CxxConfig b = a;
        b.opt_level = 3;
        cxx::Bridge bb(b);
        std::string so_b;
        check(bb.build(block, &so_b), "the same block builds at -O3");
        check(so_a != so_b, "-O2 and -O3 are different cache entries");
        check(bb.compiles() == 1 && bb.cache_hits() == 0,
              "changing the optimisation level forces a recompile");

        // Same block, different compiler -> different module again.
        cxx::CxxConfig c = a;
        c.compiler = "clang++";
        cxx::Bridge bc(c);
        std::string so_c;
        bc.build(block, &so_c);
        check(so_a != so_c, "a different compiler is a different cache entry");

        // ...and going back to the original settings hits the cache.
        cxx::Bridge ba2(a);
        std::string so_a2;
        check(ba2.build(block, &so_a2), "the original settings build again");
        eqs(so_a2, so_a, "and land on the same module");
        check(ba2.cache_hits() == 1 && ba2.compiles() == 0,
              "served from the cache, not recompiled");
    }

    printf("\n%d checks, %d failures\n", checks, failures);
    return failures == 0 ? 0 : 1;
}
