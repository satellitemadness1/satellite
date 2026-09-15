// Loading a program of more than one file. See programs/spaceships.hpp.

#include "programs/spaceships.hpp"

#include "error_reporter/report.hpp"
#include "programs/source_file.hpp"
#include "satellite_spaceship/shape.hpp"

#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <utility>
#include <vector>

namespace satellite {

namespace {

// The directory a file's spaceships are found in -- the part of its path up to
// and including the last `/`, or nothing for a bare name and for `<prompt>`.
std::string directory_of(const std::string &path)
{
    if (!path.empty() && path.front() == '<')
        return {};
    const size_t slash = path.rfind('/');
    return slash == std::string::npos ? std::string() : path.substr(0, slash + 1);
}

// THE SAME FILE HOWEVER IT WAS REACHED. `ship.satl` from `example/host.satl` and
// `../example/ship.satl` from somewhere else are one spaceship, and a file that
// includes the file satl was given is file 0 rather than a second copy of it.
std::string canonical_of(const std::string &path)
{
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) == nullptr)
        return {};
    return resolved;
}

bool a_program_name(const Ast &ast, std::string_view name)
{
    const Node &program = ast[ast.root()];
    for (uint32_t i = 0; i < ast.list_size(program.a); i++) {
        const NodeIndex item = ast.list_at(program.a, i);
        const NodeKind kind = ast[item].kind;
        if ((kind == NodeKind::Capsule || kind == NodeKind::Spacesuit ||
             kind == NodeKind::Global) &&
            ast.text_of(item) == name)
            return true;
    }
    return false;
}

const char *kind_word(const Ast &ast, std::string_view name)
{
    const Node &program = ast[ast.root()];
    for (uint32_t i = 0; i < ast.list_size(program.a); i++) {
        const NodeIndex item = ast.list_at(program.a, i);
        if (ast.text_of(item) != name)
            continue;
        switch (ast[item].kind) {
        case NodeKind::Capsule:   return "capsule";
        case NodeKind::Spacesuit: return "spacesuit";
        case NodeKind::Global:    return "global";
        default:                  break;
        }
    }
    return "name";
}

// EVERY SPAN IN A DIAGNOSTIC, SAID TO BE IN `file`. The parser does not know
// which file of a run it is reading and has no reason to; its diagnostics are
// told afterwards, once, here.
void place_in(std::vector<errors::Diagnostic> &problems, uint32_t file)
{
    for (errors::Diagnostic &problem : problems) {
        problem.at.file = file;
        for (errors::Note &note : problem.notes)
            note.at.file = file;
    }
}

// `ship` for `dir/ship.satl` -- the name an include beside the file would use.
std::string stem_of(const std::string &path)
{
    const size_t slash = path.rfind('/');
    std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
    if (base.size() > 5 && base.compare(base.size() - 5, 5, ".satl") == 0)
        base.resize(base.size() - 5);
    return base;
}

errors::Span span_in(const Ast &ast, NodeIndex node, uint32_t file)
{
    const Token &at = ast.token_of(node);
    return errors::Span{at.start, at.end, at.line, file};
}

// One file of the run while it is being loaded.
struct Loading {
    const Ast *ast = nullptr;
    std::string path;
    std::vector<resolve::Spaceship> ships;
    std::vector<std::pair<NodeIndex, uint32_t>> includes;
};

} // namespace

bool includes_a_spaceship(const Ast &ast)
{
    for (NodeIndex i = 1; i < ast.size(); i++)
        if (ast[i].kind == NodeKind::Include) {
            const spaceship::Named named = spaceship::shape_of(ast, i).named;
            // A BAD PATH TOO (revision 07): the loader is where S1607 is said,
            // and a program that skipped it would be told S0720 instead.
            if (named == spaceship::Named::Spaceship ||
                named == spaceship::Named::NotAName ||
                named == spaceship::Named::BadPath)
                return true;
        }
    return false;
}

uint32_t top_level_spaceships(const Ast &ast)
{
    if (ast.root() == kNoNode)
        return 0;
    uint32_t count = 0;
    const Node &program = ast[ast.root()];
    for (uint32_t i = 0; i < ast.list_size(program.a); i++) {
        const NodeIndex item = ast.list_at(program.a, i);
        if (ast[item].kind == NodeKind::Include &&
            spaceship::shape_of(ast, item).named == spaceship::Named::Spaceship)
            count++;
    }
    return count;
}

bool build_with_spaceships(const std::string &name, Built &out, bool report)
{
    std::vector<errors::Diagnostic> problems;
    std::vector<Loading> files(1);
    files[0].ast = &out.parsed.ast;
    files[0].path = name;
    const std::string host = canonical_of(name);
    const std::string host_name = stem_of(name);

    // FILE 0's SPACESHIPS ARE BESIDE THE FILE ITSELF, NOT BESIDE A LINK TO IT.
    // `bin/tool.satl` pointing at `src/host.satl` includes `src/ship.satl` --
    // found by review, where the loader looked in `bin/`. Every other file's
    // path was built from file 0's directory, so this one choice decides all.
    std::string first_directory = directory_of(name);
    if (!host.empty() &&
        canonical_of(first_directory.empty() ? "." : first_directory) !=
            canonical_of(directory_of(host)))
        first_directory = directory_of(host);

    // --- every file the program reaches, parsed ----------------------------
    //
    // A QUEUE AND NOT A RECURSION, for DESIGN §7.5's reason: a chain of files
    // each including the next is a depth the program chooses.
    for (uint32_t f = 0; f < files.size(); f++) {
        const Ast &ast = *files[f].ast;
        const std::string directory =
            f == 0 ? first_directory : directory_of(files[f].path);

        for (NodeIndex node = 1; node < ast.size(); node++) {
            if (ast[node].kind != NodeKind::Include)
                continue;
            const spaceship::Shape shape = spaceship::shape_of(ast, node);
            if (shape.named == spaceship::Named::NotAName) {
                problems.push_back(errors::make<errors::Code::SPACESHIP_NOT_A_NAME>(
                    span_in(ast, node, f)));
                continue;
            }
            if (shape.named == spaceship::Named::BadPath) {
                const std::string_view written = spaceship::written_of(ast, shape);
                const size_t slash = written.rfind('/');
                problems.push_back(errors::make<errors::Code::SPACESHIP_PATH_NOT_A_NAME>(
                    span_in(ast, shape.name, f), std::string(written),
                    std::string(slash == std::string_view::npos ? written : written.substr(slash + 1))));
                continue;
            }
            if (shape.named != spaceship::Named::Spaceship)
                continue;

            // THE FILE. A bare name is `<name>.satl` beside the file that wrote the
            // include. A quoted path (003 revision 07) is joined to that same
            // directory -- so "parts/ship", "../shared/ship" and "../../ship" are
            // all relative to the INCLUDING file, whoever included it -- unless it
            // starts with `/`, and `.satl` is added when it is not written.
            const std::string ship_name(spaceship::name_of(ast, shape));
            std::string path;
            if (shape.path) {
                std::string written(spaceship::written_of(ast, shape));
                if (!(written.size() > 5 && written.compare(written.size() - 5, 5, ".satl") == 0))
                    written += ".satl";
                path = written.front() == '/' ? written : directory + written;
            } else {
                path = directory + ship_name + ".satl";
            }
            const std::string canonical = canonical_of(path);
            if (canonical.empty()) {
                problems.push_back(errors::make<errors::Code::SPACESHIP_NOT_FOUND>(
                    span_in(ast, shape.name, f), ship_name, path));
                continue;
            }

            // A NAME THE LANGUAGE OWNS UNDER `satellite.library` CANNOT BE A
            // SPACESHIP'S -- `main` and `system` are nodes there, and a file
            // called `system.satl` would number its names under the language's
            // `satellite.library.system`. Found by review.
            if (const words::PathId owned = out.words.find(
                    static_cast<words::PathId>(words::NodeId::LIBRARY), ship_name);
                owned != words::kNoPath && words::is_language_word(owned)) {
                problems.push_back(errors::make<errors::Code::PARSE_NAME_IS_LANGUAGE_OWNED>(
                    span_in(ast, shape.name, f), ship_name, "satellite.library",
                    "spaceship"));
                continue;
            }

            // AND NOT A NAME THE FILE THAT WRITES THE INCLUDE ALREADY USES,
            // asked of EVERY include in every file -- found by review, where a
            // nested file's own capsule silently shadowed the spaceship it
            // included and the program failed when it ran.
            if (a_program_name(ast, ship_name)) {
                problems.push_back(errors::make<errors::Code::SPACESHIP_NAME_TAKEN>(
                    span_in(ast, shape.name, f), ship_name, kind_word(ast, ship_name)));
                continue;
            }

            // WHICH FILE OF THE RUN IT IS, loading it the first time. A name
            // is a file here: every include looks beside the file that wrote
            // it and every file was found beside the first, so one name is one
            // directory entry and errors.def's gap at 1603 says why.
            uint32_t id = 0;
            words::PathId ship_node =
                static_cast<words::PathId>(words::NodeId::LIBRARY);
            bool known = canonical == host;
            for (size_t k = 0; !known && k < out.ships.size(); k++)
                if (out.ships[k]->canonical == canonical) {
                    id = static_cast<uint32_t>(k + 1);
                    ship_node = out.ships[k]->node;
                    known = true;
                }

            // ONE FILE, ONE NAME. A link that reaches a loaded file under
            // another name would give it two -- one numbered, one not -- and the
            // second resolved to nothing. Found by review.
            if (known) {
                const std::string &loaded = id == 0 ? host_name : out.ships[id - 1]->name;
                if (loaded != ship_name) {
                    problems.push_back(errors::make<errors::Code::SPACESHIP_TWO_NAMES>(
                        span_in(ast, shape.name, f), ship_name, loaded));
                    continue;
                }
            }

            // TWO FILES, ONE NAME -- S1603, which only a path can reach: the
            // spaceship is reached by its name, so the second file could never
            // be. (The same file under two paths is `known` above, and fine.)
            if (!known) {
                bool clash = false;
                for (const auto &loaded : out.ships)
                    if (loaded->name == ship_name) {
                        problems.push_back(errors::make<errors::Code::SPACESHIP_TWO_FILES_ONE_NAME>(
                            span_in(ast, shape.name, f), ship_name, loaded->path, path));
                        clash = true;
                        break;
                    }
                if (clash)
                    continue;
            }

            if (!known) {
                // THE NODE IS THE PROGRAM'S, SO IT MAY NOT BE ONE FILE 0 ALREADY
                // USES either -- `satellite.library.<ship>` sits beside file 0's
                // own capsules, spacesuits and globals, whichever file wrote
                // the include.
                if (a_program_name(out.parsed.ast, ship_name)) {
                    problems.push_back(errors::make<errors::Code::SPACESHIP_NAME_TAKEN>(
                        span_in(ast, shape.name, f), ship_name,
                        kind_word(out.parsed.ast, ship_name)));
                    continue;
                }

                auto ship = std::make_unique<LoadedSpaceship>();
                ship->path = path;
                ship->canonical = canonical;
                ship->name = ship_name;
                if (!read_file(path, ship->text)) {
                    problems.push_back(errors::make<errors::Code::SPACESHIP_UNREADABLE>(
                        span_in(ast, shape.name, f), ship_name, path,
                        std::strerror(errno)));
                    continue;
                }
                ship->node = out.words.intern(
                    static_cast<words::PathId>(words::NodeId::LIBRARY), ship_name);
                ship->parsed = parse(ship->text, out.words, ship->node);
                id = static_cast<uint32_t>(out.ships.size() + 1);
                place_in(ship->parsed.errors, id);
                problems.insert(problems.end(), ship->parsed.errors.begin(),
                                ship->parsed.errors.end());
                ship_node = ship->node;

                Loading next;
                next.ast = &ship->parsed.ast;
                next.path = path;
                out.ships.push_back(std::move(ship));
                files.push_back(std::move(next));
            }

            files[f].includes.push_back({node, id});
            bool seen = false;
            for (const resolve::Spaceship &each : files[f].ships)
                seen = seen || each.file == id;
            if (!seen)
                files[f].ships.push_back(
                    {id == 0 ? spaceship::name_of(ast, shape)
                             : std::string_view(out.ships[id - 1]->name),
                     ship_node, id});
        }
    }

    for (const auto &ship : out.ships)
        out.others.push_back(errors::Source{ship->path, ship->text});
    const errors::Source against = out.source(name);

    // A SPACESHIP IS NAMED BY THE INCLUDE, SO A FILE THAT INCLUDES ITSELF UNDER
    // ANOTHER NAME IS NOT A LOAD PROBLEM -- file 0 reached again is file 0.
    if (report && !problems.empty())
        fputs(errors::render(problems, against).c_str(), stderr);
    if (errors::any_error(problems)) {
        out.parsed.errors.insert(out.parsed.errors.end(), problems.begin(),
                                 problems.end());
        return false;
    }

    // --- every file resolved, together ---------------------------------------
    std::vector<resolve::File> run(files.size());
    for (uint32_t f = 0; f < files.size(); f++) {
        run[f].ast = files[f].ast;
        run[f].library = f == 0 ? static_cast<words::PathId>(words::NodeId::LIBRARY)
                                : out.ships[f - 1]->node;
        run[f].ships = files[f].ships;
    }
    std::vector<resolve::Resolved> resolved = resolve::resolve_run(run, out.words);
    out.resolved = std::move(resolved[0]);
    for (uint32_t f = 1; f < files.size(); f++)
        out.ships[f - 1]->resolved = std::move(resolved[f]);

    std::vector<errors::Diagnostic> unresolved = out.resolved.problems;
    for (const auto &ship : out.ships)
        unresolved.insert(unresolved.end(), ship->resolved.problems.begin(),
                          ship->resolved.problems.end());
    if (report && !unresolved.empty())
        fputs(errors::render(unresolved, against).c_str(), stderr);
    if (errors::any_error(unresolved)) {
        out.resolved.problems = std::move(unresolved);
        return false;
    }

    // --- every file compiled into one program ----------------------------
    //
    // EACH FILE'S GLOBALS AFTER THE GLOBALS OF EVERY FILE IT INCLUDES -- a
    // post-order over the includes, which a cycle cannot loop because a file
    // is marked before its includes are followed.
    std::vector<uint32_t> order;
    std::vector<uint8_t> marked(files.size(), 0);
    std::vector<std::pair<uint32_t, size_t>> stack{{0, 0}};
    marked[0] = 1;
    while (!stack.empty()) {
        auto &[file, next] = stack.back();
        if (next < files[file].includes.size()) {
            const uint32_t to = files[file].includes[next++].second;
            if (!marked[to]) {
                marked[to] = 1;
                stack.push_back({to, 0});
            }
            continue;
        }
        order.push_back(file);
        stack.pop_back();
    }

    std::vector<eval::Unit> units(files.size());
    for (uint32_t f = 0; f < files.size(); f++) {
        units[f].ast = files[f].ast;
        units[f].resolved = f == 0 ? &out.resolved : &out.ships[f - 1]->resolved;
        units[f].name = f == 0 ? host_name : out.ships[f - 1]->name;
        units[f].includes = files[f].includes;
        if (f == 0)
            units[f].already_included = out.already_included;
    }
    out.program = eval::compile_run(units, out.words, order);
    if (report && !out.program.problems.empty())
        fputs(errors::render(out.program.problems, against).c_str(), stderr);
    out.ok = out.program.ok();
    return out.ok;
}

} // namespace satellite
