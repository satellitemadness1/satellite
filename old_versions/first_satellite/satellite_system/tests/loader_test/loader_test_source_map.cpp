// Part of the loader test binary (loader_test): the two pieces of the loader
// that are examined directly rather than through a program's output — the
// SourceMap a load builds, which is what lets an error name the spaceship it
// is actually in, and search_paths(), which is the order the loader looks in.
//
// These call load() and search_paths() themselves, so unlike the rest of this
// binary they can see the answer without running anything.

#include "loader_test.hpp"

#include "spaceship_loader/loader.hpp"
#include "system_facts/system.hpp"

#include <fstream>
#include <string>
#include <vector>

using namespace satellite;

// --- the SourceMap ends up with one entry per spaceship --------------------
void loader_test_source_map_one_entry_per_spaceship()
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

// --- the search order ------------------------------------------------------
void loader_test_search_order()
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
