// Part of the loader test binary (loader_test): the merge itself, and the
// three shapes of include graph that only terminate because a spaceship is
// marked seen before it is parsed — a diamond, a cycle, and a self-include.
// Also the two rules about ORDER and ceremony, which sit here because they are
// about what a plain include does rather than about how a path is spelled.
//
// The harness these sections share — the check helper, write_ship, says and
// the per-run directory — is declared in loader_test.hpp and defined in
// loader_test.cpp, which also calls the functions below in this same order.

#include "loader_test.hpp"

#include "interpreter/interp.hpp"

#include <string>

using namespace satellite;

// --- the merge -------------------------------------------------------------
//
// The whole feature in one test: a capsule defined in one spaceship,
// called from another. Nothing in resolve() or the evaluator changed to
// make this work — the loader hands them one Program and they cannot tell
// it came from two files.
void loader_test_the_merge()
{
    write_ship("helper", says("greet", "hello from helper"));
    const std::string path =
        write_ship("entry", "satellite.include(helper)\n\n" +
                                main_calling("    greet()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a program that includes another spaceship runs");
    check(r.output == "hello from helper\n",
          "a capsule from an included spaceship is callable");
}

// --- include-once, through a diamond ---------------------------------------
//
// a includes b and c; both include d. Without include-once d is merged
// twice and every capsule in it is a duplicate, so this is not a test of
// an optimisation — it is a test that the common shape works at all.
void loader_test_include_once_diamond()
{
    write_ship("d", says("shared", "d"));
    write_ship("b", "satellite.include(d)\n\n" + says("from_b", "b"));
    write_ship("c", "satellite.include(d)\n\n" + says("from_c", "c"));
    const std::string path = write_ship(
        "diamond", "satellite.include(b)\nsatellite.include(c)\n\n" +
                       main_calling("    shared()\n    from_b()\n"
                                    "    from_c()\n"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a diamond of includes is not a duplicate-capsule error");
    check(r.output == "d\nb\nc\n", "every spaceship in the diamond ran once");
}

// --- a cycle terminates, and is a feature ----------------------------------
//
// §16 declines to make this an error: two spaceships that genuinely need
// each other's declarations are what resolve()'s forward references are
// for. Include-once is the entire cycle check, so this also proves there
// is no second mechanism quietly doing the work.
void loader_test_cycle_terminates()
{
    write_ship("cyc_b", "satellite.include(cyc_a)\n\n"
                        "satellite.capsule ping()\n"
                        "{\n"
                        "    satellite.console.display(\"ping\")\n"
                        "    pong()\n"
                        "    satellite.return(satellite)\n"
                        "}\n");
    const std::string path =
        write_ship("cyc_a", "satellite.include(cyc_b)\n\n" +
                                main_calling("    ping()\n") + "\n" +
                                says("pong", "pong"));

    InterpResult r = run_file(path, {});
    check(r.ok, "a -> b -> a terminates instead of looping");
    check(r.output == "ping\npong\n",
          "capsules call each other across a cycle of includes");
}

// The smallest cycle there is. It terminates for the same reason, and only
// because the entry point is marked seen before it is parsed rather than
// after.
void loader_test_self_include()
{
    const std::string path =
        write_ship("selfie", "satellite.include(selfie)\n\n" +
                                 main_calling(
                                     "    satellite.console.display"
                                     "(\"survived\")\n"));

    InterpResult r = run_file(path, {});
    check(r.ok && r.output == "survived\n",
          "a spaceship that includes itself loads once");
}

// --- order: an included spaceship's body runs first ------------------------
//
// A spaceship that sets up globals depends on this, which is why §16 fixes
// the order rather than leaving it to whatever the merge happened to do.
void loader_test_included_body_runs_first()
{
    write_ship("first", "satellite.console.display(\"included\")\n");
    const std::string path = write_ship(
        "order", "satellite.include(first)\n"
                 "satellite.console.display(\"includer\")\n\n" +
                     main_calling("    satellite.console.display"
                                  "(\"main\")\n"));

    InterpResult r = run_file(path, {});
    check(r.ok && r.output == "included\nincluder\nmain\n",
          "an included spaceship's top-level statements run first");
}

// --- satellite.include(satellite) is ceremony ------------------------------
//
// §2's hello world opens with it, so it has to stay a no-op forever. The
// risk the loader introduced is that it starts looking for satellite.satl.
void loader_test_include_satellite_is_ceremony()
{
    const std::string path = write_ship(
        "ceremony", "satellite.include(satellite)\n\n" +
                        main_calling("    satellite.console.display"
                                     "(\"hello, world!\")\n"));

    InterpResult r = run_file(path, {});
    check(r.ok && r.output == "hello, world!\n",
          "satellite.include(satellite) is still ceremony");
}
