// §16's load phase: several spaceships merged into one Program.
//
// Every test here writes real files and loads them, because every interesting
// property of the loader is about the filesystem — where it looks, what it
// canonicalises, and what it declines to read twice. A loader tested against
// an in-memory stub would confirm the merge and none of the rest.
//
// The load-bearing tests are include-once and the cycle. Both are the same one
// line of code and both are the difference between a working program and a
// pile of duplicate-capsule errors, so both are checked from the outside — by
// running the program and looking at what it printed.

#include "interp.hpp"
#include "loader.hpp"
#include "system.hpp"

#include <cstdio>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
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

// One directory per run, so a stale file from a previous run cannot make a
// test pass. Removed at the end; left behind on a failure, which is when
// someone wants to look at it.
static std::string dir()
{
    static std::string path;
    if (path.empty()) {
        path = "/tmp/satellite_loader_test_" + std::to_string(getpid());
        mkdir(path.c_str(), 0700);
    }
    return path;
}

static std::string write_ship(const std::string &name, const std::string &body)
{
    const std::string path = dir() + "/" + name + ".satl";
    std::ofstream out(path);
    out << body;
    return path;
}

// A capsule that displays one word, so a test can tell which spaceships ran
// and in what order by reading the output.
static std::string says(const std::string &name, const std::string &word)
{
    return "satellite.capsule " + name + "()\n"
           "{\n"
           "    satellite.console.display(\"" + word + "\")\n"
           "    satellite.return(satellite)\n"
           "}\n";
}

static std::string main_calling(const std::string &body)
{
    return "satellite.capsule satellite.main("
           "satellite.container.list<satellite.variable.string> argz)\n"
           "{\n" + body +
           "    satellite.return(satellite)\n"
           "}\n";
}

static bool contains(const std::string &haystack, const std::string &needle)
{
    return haystack.find(needle) != std::string::npos;
}

int main()
{
    // --- the merge ---------------------------------------------------------
    //
    // The whole feature in one test: a capsule defined in one spaceship,
    // called from another. Nothing in resolve() or the evaluator changed to
    // make this work — the loader hands them one Program and they cannot tell
    // it came from two files.
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

    // --- include-once, through a diamond ------------------------------------
    //
    // a includes b and c; both include d. Without include-once d is merged
    // twice and every capsule in it is a duplicate, so this is not a test of
    // an optimisation — it is a test that the common shape works at all.
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

    // --- a cycle terminates, and is a feature --------------------------------
    //
    // §16 declines to make this an error: two spaceships that genuinely need
    // each other's declarations are what resolve()'s forward references are
    // for. Include-once is the entire cycle check, so this also proves there
    // is no second mechanism quietly doing the work.
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

    // --- order: an included spaceship's body runs first ----------------------
    //
    // A spaceship that sets up globals depends on this, which is why §16 fixes
    // the order rather than leaving it to whatever the merge happened to do.
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

    // --- satellite.include(satellite) is ceremony ----------------------------
    //
    // §2's hello world opens with it, so it has to stay a no-op forever. The
    // risk the loader introduced is that it starts looking for satellite.satl.
    {
        const std::string path = write_ship(
            "ceremony", "satellite.include(satellite)\n\n" +
                            main_calling("    satellite.console.display"
                                         "(\"hello, world!\")\n"));

        InterpResult r = run_file(path, {});
        check(r.ok && r.output == "hello, world!\n",
              "satellite.include(satellite) is still ceremony");
    }

    // --- errors --------------------------------------------------------------
    {
        const std::string path =
            write_ship("nofile", "satellite.include(no_such_spaceship)\n\n" +
                                     main_calling(""));

        InterpResult r = run_file(path, {});
        check(!r.ok, "an include that names nothing is an error");
        check(contains(r.output, "cannot find spaceship no_such_spaceship"),
              "and says which spaceship it could not find");
        check(contains(r.output, "looked in"),
              "and says where it looked, since the answer is usually the path");
    }

    {
        const std::string path =
            write_ship("badtarget", "satellite.include(3)\n\n" +
                                        main_calling(""));

        InterpResult r = run_file(path, {});
        check(!r.ok, "an include of something that is not a name is an error");
        check(contains(r.output, "takes a spaceship name"),
              "and says what an include does take");
    }

    // A language-owned name is not looked for in the user's directory, so a
    // file called window.satl sitting next to the program cannot answer to
    // satellite.include(satellite.window). §1's rule is that a satellite-rooted
    // name is the language's, and a user file shadowing one would end that.
    {
        write_ship("window", says("should_not_load", "user window"));
        const std::string path =
            write_ship("langowned", "satellite.include(satellite.window)\n\n" +
                                        main_calling(""));

        InterpResult r = run_file(path, {});
        check(!r.ok, "a language-owned include is not satisfied by a user file");
        check(contains(r.output, "satellite.window"),
              "and names the language-owned path it was asked for");
    }

    // --- a parse error inside an included spaceship --------------------------
    //
    // This is what §16's file id was for. The error is on line 1 of the
    // included spaceship, and the includer has its own line 1 saying something
    // completely different.
    {
        write_ship("broken", "satellite.return(\n");
        const std::string path =
            write_ship("usesbroken", "satellite.include(broken)\n\n" +
                                         main_calling(""));

        InterpResult r = run_file(path, {});
        check(!r.ok, "a syntax error in an included spaceship fails the load");
        check(contains(r.output, "broken.satl:1"),
              "and is reported against the spaceship it is actually in");
        check(!contains(r.output, "usesbroken.satl:1"),
              "and not against the includer's line 1");
    }

    // --- a name defined in two spaceships ------------------------------------
    //
    // The namespace is flat (§16), so this is an error — and it names BOTH
    // definitions, because the first one may be in a file the reader has never
    // opened.
    {
        write_ship("dup_b", says("collide", "b"));
        const std::string path =
            write_ship("dup_a", "satellite.include(dup_b)\n\n" +
                                    says("collide", "a") + "\n" +
                                    main_calling(""));

        InterpResult r = run_file(path, {});
        check(!r.ok, "a capsule defined in two spaceships is an error");
        check(contains(r.output, "already defined"), "and says so");
        check(contains(r.output, "dup_a.satl") && contains(r.output, "dup_b.satl"),
              "and names both spaceships, not just the second");
    }

    // --- the SourceMap ends up with one entry per spaceship ------------------
    {
        write_ship("leaf", says("leaf_fn", "leaf"));
        write_ship("mid", "satellite.include(leaf)\n\n" + says("mid_fn", "mid"));
        const std::string path =
            write_ship("root", "satellite.include(mid)\n\n" + main_calling(""));

        std::ifstream in(path);
        std::string source((std::istreambuf_iterator<char>(in)),
                           std::istreambuf_iterator<char>());

        LoadResult loaded = load(source, path);
        check(loaded.ok(), "a three-deep chain loads");
        check(loaded.sources.size() == 3,
              "one SourceMap entry per spaceship, and no more");

        // Id 0 is the entry point; the rest are in load order, which is
        // depth-first and therefore deepest-first.
        check(loaded.sources.path(0) == path, "the entry point is id 0");
        check(contains(loaded.sources.path(1), "mid.satl") &&
                  contains(loaded.sources.path(2), "leaf.satl"),
              "ids follow load order, deepest last");
    }

    // --- the search order ----------------------------------------------------
    {
        std::vector<std::string> user = search_paths("helper", "/proj", false);
        check(!user.empty() && user[0] == "/proj/helper.satl",
              "a user-owned name is looked for beside the including spaceship "
              "first");

        std::vector<std::string> lang = search_paths("window", "/proj", true);
        for (const std::string &path : lang)
            check(!contains(path, "/proj/"),
                  "a language-owned name never looks in a user directory");

        // "." is spelled as no prefix, so an error reads helper.satl rather
        // than ./helper.satl — the way the user spelled it on the command line.
        std::vector<std::string> here = search_paths("helper", ".", false);
        check(!here.empty() && here[0] == "helper.satl",
              "the working directory adds no ./ prefix");

        // Both forms end at the installed library, which is §9's library_path()
        // rather than a second copy of its three tiers.
        const std::string library = library_path();
        if (!library.empty()) {
            check(user.back() == library + "/helper.satl",
                  "a user-owned name falls back to the installed library");
            check(!lang.empty() && lang.back() == library + "/window.satl",
                  "a language-owned name resolves in the installed library");
        }
    }

    if (failures) {
        printf("%d loader check(s) failed (files left in %s)\n", failures,
               dir().c_str());
        return 1;
    }

    // Only on success: a failure wants its evidence left on disk.
    const std::vector<std::string> written = {
        "helper",  "entry",     "d",         "b",       "c",     "diamond",
        "cyc_a",   "cyc_b",     "selfie",    "first",   "order", "ceremony",
        "nofile",  "badtarget", "window",    "langowned", "broken",
        "usesbroken", "dup_a",  "dup_b",     "leaf",    "mid",   "root"};
    for (const std::string &name : written)
        std::remove((dir() + "/" + name + ".satl").c_str());
    rmdir(dir().c_str());

    printf("PASS: loader (merge across spaceships, include-once through a "
           "diamond, cycles and self-include terminating, included bodies "
           "first, ceremony preserved, errors named against their own "
           "spaceship, search order)\n");
    return 0;
}
