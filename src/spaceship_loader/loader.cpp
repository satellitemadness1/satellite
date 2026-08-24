#include "spaceship_loader/loader.hpp"

#include "system_facts/system.hpp"

#include <climits>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

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

// Include-once bounds the recursion by the number of distinct spaceships, so
// this only fires on a project with a chain thousands deep. It exists for the
// same reason MAX_RESOLVE_DEPTH does: a stack overflow is not an error message.
constexpr int MAX_INCLUDE_DEPTH = 200;

std::string directory_of(const std::string &path)
{
    size_t slash = path.rfind('/');
    if (slash == std::string::npos)
        return ".";
    return slash == 0 ? "/" : path.substr(0, slash);
}

// The include-once key. realpath() resolves symlinks, `..` and duplicate
// slashes, so `./helper.satl` and `../project/helper.satl` are one spaceship
// rather than two — which is the entire point, since loading one file twice is
// a duplicate-capsule error rather than a working program.
//
// Falls back to the path as written when realpath fails, which happens only
// for a file that does not exist. A caller has already opened the file by
// then, so the fallback is unreachable in practice and is here so that a race
// (the file removed between open and canonicalise) degrades to a slightly
// worse key instead of losing include-once entirely.
std::string canonical(const std::string &path)
{
    char buffer[PATH_MAX];
    if (realpath(path.c_str(), buffer))
        return std::string(buffer);
    return path;
}

bool is_file(const std::string &path)
{
    struct stat info;
    return stat(path.c_str(), &info) == 0 && S_ISREG(info.st_mode);
}

bool read_file(const std::string &path, std::string &out)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        return false;
    std::ostringstream buffer;
    buffer << in.rdbuf();
    out = buffer.str();
    return true;
}

// What an include names. §1 decides all four forms with no new rule: a
// satellite-rooted path is language-owned, a bare name is user-owned, the bare
// runtime is neither, and a quoted string is not a name at all — see EXTENSION.
struct Target {
    enum Kind {
        Ceremony,   // satellite.include(satellite) — the runtime itself
        User,       // satellite.include(my_parser)
        Language,   // satellite.include(satellite.window)
        Path,       // satellite.include("object/forge_object.satl")
        Bad,        // anything else
    } kind = Bad;

    // The spaceship's name, or — for Path — the spelling as the program wrote
    // it. Both are `name` because everything downstream of classify() only ever
    // turns this into candidate paths and back into an error message.
    std::string name;
};

Target classify(const ExprPtr &what)
{
    if (!what)
        return Target{Target::Bad, {}};

    // satellite.include(satellite) is ceremony and stays ceremony (§2). It
    // means "include the runtime", which is already included by virtue of the
    // program running at all, so there is nothing to do and that is not a
    // defect — it is the hello world in §2 and it must keep working.
    if (std::holds_alternative<SatelliteLit>(*what))
        return Target{Target::Ceremony, {}};

    if (const Name *name = std::get_if<Name>(&*what))
        return Target{Target::User, name->text};

    // A quoted path. The EXPANDED value rather than `source`, so a path is
    // spelled with the same escapes as every other string in the language and
    // does not become the one string literal where \\ means two characters.
    if (const StringLit *literal = std::get_if<StringLit>(&*what))
        return Target{Target::Path, decode(literal->value)};

    // Exactly `satellite.<one segment>`. A deeper path is not grammar for an
    // include: §16's namespace is flat, so satellite.a.b would have to mean
    // something no rule defines yet.
    if (const Member *member = std::get_if<Member>(&*what))
        if (member->target && std::holds_alternative<SatelliteLit>(*member->target))
            return Target{Target::Language, member->name};

    return Target{Target::Bad, {}};
}

// Renders the search order into an error, so "cannot find" says where it
// looked. A not-found error without this is a guessing game, and the answer is
// nearly always that the file is one directory over.
std::string looked_in(const std::vector<std::string> &paths)
{
    if (paths.empty())
        return " (nowhere to look: no library directory is installed)";
    std::string out = " (looked in ";
    for (size_t i = 0; i < paths.size(); i++) {
        if (i)
            out += i + 1 == paths.size() ? " and " : ", ";
        out += paths[i];
    }
    return out + ")";
}

class Loader {
public:
    explicit Loader(LoadResult &out) : out_(out) {}

    void load(std::string text, const std::string &path)
    {
        // The entry point goes into seen_ before it is parsed, exactly as an
        // included spaceship does. Without this a file that includes ITSELF
        // loads twice — once as the entry point, once through its own include,
        // which reports every capsule in it as already defined. The self-cycle
        // is the smallest cycle there is and it must terminate like any other.
        if (!path.empty())
            seen_.push_back(canonical(path));
        load_one(std::move(text), path);
    }

private:
    LoadResult &out_;
    std::vector<std::string> seen_;
    int depth_ = 0;

    bool already_seen(const std::string &key) const
    {
        for (const std::string &s : seen_)
            if (s == key)
                return true;
        return false;
    }

    void fail(Span span, const std::string &message)
    {
        out_.errors.push_back(ParseError{message, span});
    }

    // Parses one spaceship, loads what it includes, then appends its items.
    //
    // The ORDER of those three is the contract: everything a spaceship
    // includes is merged before the spaceship itself, so a top-level statement
    // in an included file runs before the includer's own (§16). A file that
    // sets up globals depends on exactly that.
    void load_one(std::string text, const std::string &path)
    {
        // Parsed straight out of the SourceMap, so the text an error is
        // rendered against is literally the object that was parsed rather than
        // a copy of it that could drift.
        const uint32_t file = out_.sources.add(std::move(text), path);
        ParseResult parsed = parse(out_.sources.text(file), file);

        const std::string from_dir =
            path.empty() ? std::string(".") : directory_of(path);

        // Includes are followed even when this spaceship failed to parse. The
        // parser recovers and keeps going, so the include list is still worth
        // having, and abandoning the load here would report the includer's
        // mistakes while hiding every one in what it includes until the first
        // was fixed.
        for (const TopLevel &item : parsed.program.items) {
            const Include *include = std::get_if<Include>(&item);
            if (include)
                follow(*include, from_dir);
        }

        // This spaceship's own errors AFTER the ones from what it includes, so
        // errors come out in the same order the items do: everything about an
        // included spaceship precedes the includer, and the reader fixes the
        // dependency before the thing that depends on it.
        for (ParseError &error : parsed.errors)
            out_.errors.push_back(std::move(error));

        for (TopLevel &item : parsed.program.items)
            out_.program.items.push_back(std::move(item));
    }

    void follow(const Include &node, const std::string &from_dir)
    {
        // No expression at all means the parser already failed on this line
        // and said so. Adding "takes a spaceship name" on top would be a
        // second complaint about one mistake, and the less useful of the two.
        if (!node.what)
            return;

        const Target target = classify(node.what);

        if (target.kind == Target::Ceremony)
            return;

        if (target.kind == Target::Bad) {
            fail(node.span,
                 "satellite.include takes a spaceship name, either bare "
                 "(my_parser) or satellite-rooted (satellite.window)");
            return;
        }

        const bool language = target.kind == Target::Language;
        const bool quoted = target.kind == Target::Path;

        // An include of "" names nothing, and the candidate list for it would
        // be the includer's own directory — which is a directory, so is_file
        // rejects it and the reader gets "cannot find spaceship " with nothing
        // after it. Say the real thing instead.
        if (quoted && target.name.empty()) {
            fail(node.span, "satellite.include(\"\") names no spaceship");
            return;
        }

        const std::vector<std::string> candidates =
            quoted ? path_candidates(target.name, from_dir)
                   : search_paths(target.name, from_dir, language);

        std::string found;
        for (const std::string &candidate : candidates) {
            if (is_file(candidate)) {
                found = candidate;
                break;
            }
        }

        if (found.empty()) {
            // A quoted path is quoted back, so the error shows the same
            // characters the program wrote and the reader can compare them to
            // the line without translating a spelling.
            const std::string what = language ? "satellite." + target.name
                                   : quoted   ? "\"" + target.name + "\""
                                              : target.name;
            fail(node.span, "cannot find spaceship " + what +
                                looked_in(candidates));
            return;
        }

        // Include-once, keyed by canonical path. Without it, `a` including both
        // `b` and `c` where both include `d` is a duplicate-capsule error
        // rather than a working program.
        //
        // This is also the whole of the cycle check, and there is no other one.
        // `a -> b -> a` terminates here: by the time `b` asks for `a`, `a` is
        // already in seen_ because it was added on the way IN rather than on
        // the way out, so the second request is skipped. That is what C's
        // include guards do, and §16 calls it a feature rather than an error —
        // two spaceships that genuinely need each other's declarations are
        // exactly what resolve()'s forward references are for.
        const std::string key = canonical(found);
        if (already_seen(key))
            return;
        seen_.push_back(key);

        std::string text;
        if (!read_file(found, text)) {
            // Found by stat and unreadable by open: a permission problem, or a
            // race. Worth its own message, because "cannot find" would send
            // the reader looking for a file that is right there.
            fail(node.span, "cannot read spaceship " + found);
            return;
        }

        if (depth_ >= MAX_INCLUDE_DEPTH) {
            fail(node.span, "includes nest deeper than the loader can follow (" +
                                std::to_string(MAX_INCLUDE_DEPTH) + ")");
            return;
        }
        depth_++;
        load_one(std::move(text), found);
        depth_--;
    }
};

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

LoadResult load(const std::string &source, const std::string &path)
{
    LoadResult result;
    Loader(result).load(source, path);
    return result;
}

} // namespace satellite
