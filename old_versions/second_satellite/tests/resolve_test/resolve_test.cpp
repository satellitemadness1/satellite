// resolve_test -- the proof that src/name_resolver/ does what DESIGN §7
// specifies. See tests/resolve_test/resolve_test.hpp for the harness and for
// what is being proved.
//
// THE SUBJECT IS INVISIBLE IN A WAY EVERY EARLIER MILESTONE'S WAS NOT, which is
// what shapes this suite. A token stream can be read against the file and an
// unparsed tree can be read as a program; a frame produces no text at all until
// M9 puts values in it. DESIGN §7.1 is the argument: the first satellite's
// registry gave a recursive capsule ONE slot for the whole program and returned
// 1 for every input, and nothing about that program's source, tokens or tree
// says so. The slot table is where it would have been visible, so the slot
// table is what these assertions are about.

#include "resolve_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace resolve_test {

int failures = 0;
std::string example_directory = "example";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

void resolve_source(const std::string &source, Run &into)
{
    into.parsed = satellite::parse(source, into.words);
    into.resolved = satellite::resolve::resolve(into.parsed.ast, into.words);
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

bool raised(const Run &run, satellite::errors::Code code)
{
    for (const satellite::errors::Diagnostic &at : run.resolved.problems)
        if (at.code == code)
            return true;
    return false;
}

bool only_problem(const Run &run, satellite::errors::Code code)
{
    // ONE ERROR AND NOT ONE DIAGNOSTIC, because a note rides in the same vector
    // and belongs to the error it is about -- errors.def's S0590 is attached by
    // two sites and is a second span rather than a second problem.
    size_t errors = 0;
    for (const satellite::errors::Diagnostic &at : run.resolved.problems)
        if (at.is_error())
            errors++;
    return errors == 1 && raised(run, code);
}

const satellite::resolve::Frame *frame_of(const Run &run, const std::string &name)
{
    for (const satellite::resolve::Frame &frame : run.resolved.frames) {
        const satellite::words::PathId id = frame.capsule;
        const std::string spelled =
            satellite::words::is_language_word(id)
                ? satellite::words::path_text(
                      static_cast<satellite::words::NodeId>(id))
                : std::string(run.words.name_of(id));
        if (spelled == name || holds(spelled, "." + name))
            return &frame;
    }
    return nullptr;
}

std::vector<std::string> numbers_of(const Run &run)
{
    std::vector<std::string> out;
    for (satellite::NodeIndex node = 1; node < run.parsed.ast.size(); node++) {
        const satellite::words::PathId id = run.resolved.at(node).path;
        if (id == satellite::words::kNoPath ||
            !satellite::words::is_language_word(id))
            continue;
        out.push_back(satellite::words::number_text(
            static_cast<satellite::words::NodeId>(id)));
    }
    return out;
}

bool resolved_to(const Run &run, const std::string &number)
{
    for (const std::string &at : numbers_of(run))
        if (at == number)
            return true;
    return false;
}

// HOW MANY NODES REACHED `number` THAT WAY -- M19.6, and it exists because
// origin_of() below answers about the FIRST node and one number can now be
// reached two ways in one program. `l.sort()` and `l.sort("up")` both land on
// `1 4 2 3` -- the bare row and M16's fold onto it -- and the whole point of
// the option token is that the second is taken from the file while the first
// is still walked for. A first-match answer cannot see that and reported the
// walk, which is the honest answer to a question that was too coarse.
size_t count_origin(const Run &run, const std::string &number,
                    satellite::resolve::Origin origin)
{
    size_t found = 0;
    for (satellite::NodeIndex node = 1; node < run.parsed.ast.size(); node++) {
        const satellite::resolve::Info &info = run.resolved.at(node);
        if (info.path == satellite::words::kNoPath ||
            !satellite::words::is_language_word(info.path))
            continue;
        if (satellite::words::number_text(
                static_cast<satellite::words::NodeId>(info.path)) == number &&
            info.origin == origin)
            found++;
    }
    return found;
}

satellite::resolve::Origin origin_of(const Run &run, const std::string &number)
{
    for (satellite::NodeIndex node = 1; node < run.parsed.ast.size(); node++) {
        const satellite::resolve::Info &info = run.resolved.at(node);
        if (info.path == satellite::words::kNoPath ||
            !satellite::words::is_language_word(info.path))
            continue;
        if (satellite::words::number_text(
                static_cast<satellite::words::NodeId>(info.path)) == number)
            return info.origin;
    }
    return satellite::resolve::Origin::Parsed;
}

} // namespace resolve_test

int main(int argc, char **argv)
{
    if (argc > 1)
        resolve_test::example_directory = argv[1];

    resolve_test::section_frames();
    resolve_test::section_names();
    resolve_test::section_paths();
    resolve_test::section_arguments();
    resolve_test::section_cache();
    resolve_test::section_option_token();
    resolve_test::section_examples();

    if (resolve_test::failures != 0) {
        printf("resolve_test: %d failed\n", resolve_test::failures);
        return 1;
    }
    printf("resolve_test: ok\n");
    return 0;
}
