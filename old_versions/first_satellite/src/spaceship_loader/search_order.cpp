// Where a spaceship is looked for: the search order for a name, the candidate
// list for a quoted path, and the file extension both of them are built out of.
//
// Part of src/spaceship_loader/, split out of a 412-line loader.cpp. Both
// functions here are declared in loader.hpp rather than in an internal header
// because they were already public before the split — the not-found error is
// rendered out of the order they return, and a test checks that order.

#include "spaceship_loader/loader.hpp"

#include "system_facts/system.hpp"

namespace satellite {
namespace {

// The extension every spaceship has. A BARE include names `helper`, never
// `helper.satl`: §1 makes a bare name the user's, and a bare name with a file
// extension in it would be a path wearing a name's clothes.
//
// A QUOTED include is the other half of that rule, and the quotes are the whole
// of the distinction: `satellite.include(helper)` says WHAT a spaceship is
// called and lets the search order find it, while
// `satellite.include("object/helper.satl")` says WHERE it sits and is looked for
// exactly there. §1 is not weakened by this — it is completed by it. A bare word
// is still a name the user owns, a satellite-rooted path is still ours, and a
// string literal was never a name at all, so it can carry a location without
// any name ever having to mean a place. C draws the same line with <> against
// "", and for the same reason.
constexpr const char *EXTENSION = ".satl";

// Whether a quoted include already names the file it wants. A spelling that
// does not is completed with EXTENSION, so "object/helper" and
// "object/helper.satl" are one include rather than two spellings of it.
bool has_extension(const std::string &spelling)
{
    const std::string suffix = EXTENSION;
    return spelling.size() > suffix.size() &&
           spelling.compare(spelling.size() - suffix.size(), suffix.size(),
                            suffix) == 0;
}

} // namespace

std::vector<std::string> search_paths(const std::string &name,
                                      const std::string &from_dir,
                                      bool language_owned)
{
    std::vector<std::string> paths;
    const std::string leaf = name + EXTENSION;

    // The including spaceship's directory first, so a project's own files find
    // each other with no configuration. Skipped for a language-owned name: if
    // a directory holding a file called window.satl could satisfy
    // satellite.include(satellite.window), then a user file would be answering
    // to a language-owned path, and §1's rule that a satellite-rooted name is
    // OURS would hold only until someone picked the wrong filename.
    //
    // "." is spelled as no prefix at all, so an error reads `helper.satl`
    // rather than `./helper.satl`. That is the same file, and it is how the
    // user spelled the entry point on the command line — a message that says
    // ./ back at someone who typed `satl --run a.satl` is answering in a
    // dialect they did not use.
    if (!language_owned && !from_dir.empty())
        paths.push_back(from_dir == "." ? leaf : from_dir + "/" + leaf);

    // Then the installed library, which is library_path()'s three tiers
    // ($SATELLITE_PATH, then the location relative to the binary, then the
    // compiled-in prefix) already collapsed into one answer by §9. Reusing it
    // rather than re-deriving the tiers here is what keeps `satl --where` an
    // honest answer to "where would an include come from".
    const std::string library = library_path();
    if (!library.empty())
        paths.push_back(library + "/" + leaf);

    return paths;
}

std::vector<std::string> path_candidates(const std::string &spelling,
                                         const std::string &from_dir)
{
    std::vector<std::string> paths;
    if (spelling.empty())
        return paths;

    // An absolute path is the one spelling that means the same thing from
    // everywhere, so it is taken as written and nothing is prepended to it.
    const bool absolute = spelling[0] == '/';

    // RELATIVE TO THE INCLUDING SPACESHIP, not to the working directory. This
    // is the decision the whole form turns on. A project is a tree of files
    // that include each other by their positions in that tree, and those
    // positions do not change when someone runs the program from one directory
    // up. Resolving against the cwd would make `satl view_forge/main.satl` and
    // `cd view_forge && satl main.satl` two different programs, and the failure
    // would be a not-found error naming a file that is plainly there.
    //
    // `..` needs no code of its own: the kernel walks it, and canonical()
    // collapses it for the include-once key, so a file reached as `../a.satl`
    // from one spaceship and `a.satl` from another is ONE spaceship.
    const std::string base =
        absolute || from_dir.empty() || from_dir == "."
            ? spelling
            : from_dir + "/" + spelling;

    paths.push_back(base);

    // Then the same spelling with the extension supplied, so a quoted include
    // may leave it off exactly as a bare name does.
    if (!has_extension(base))
        paths.push_back(base + EXTENSION);

    // Then the installed library, for a relative spelling only — an absolute
    // path has already said where it wants to be found, and going on to look
    // somewhere else would make `/etc/passwd.satl` resolvable to a library file
    // of the same shape. This is what lets a library ship a subdirectory and a
    // program reach into it as "container/list_helpers.satl".
    if (!absolute) {
        const std::string library = library_path();
        if (!library.empty()) {
            paths.push_back(library + "/" + spelling);
            if (!has_extension(spelling))
                paths.push_back(library + "/" + spelling + EXTENSION);
        }
    }

    return paths;
}

} // namespace satellite
