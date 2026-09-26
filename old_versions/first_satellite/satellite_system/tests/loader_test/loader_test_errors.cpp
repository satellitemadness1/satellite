// Part of the loader test binary (loader_test): everything the loader refuses,
// and what it says while refusing. A bare name it cannot find, an include of
// something that is not a name at all, a language-owned name a user file tries
// to answer for, a syntax error inside an included spaceship, and one capsule
// name defined in two of them.
//
// The message text is checked, not just the failure, because for all of these
// the message IS the feature: a load that stops without naming the spaceship
// it stopped in leaves the reader opening files at random.

#include "loader_test.hpp"

#include "interpreter/interp.hpp"

#include <string>

using namespace satellite;

// --- errors ----------------------------------------------------------------
void loader_test_bare_include_that_names_nothing()
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

void loader_test_include_of_a_non_name()
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
void loader_test_language_owned_include()
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

// --- a parse error inside an included spaceship ----------------------------
//
// This is what §16's file id was for. The error is on line 1 of the
// included spaceship, and the includer has its own line 1 saying something
// completely different.
void loader_test_parse_error_in_included_spaceship()
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

// --- a name defined in two spaceships --------------------------------------
//
// The namespace is flat (§16), so this is an error — and it names BOTH
// definitions, because the first one may be in a file the reader has never
// opened.
void loader_test_name_defined_in_two_spaceships()
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
