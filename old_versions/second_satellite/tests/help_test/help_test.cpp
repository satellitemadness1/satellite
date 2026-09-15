// help_test -- the proof that src/satellite_help/ does what PLAN M18 specifies.
// See tests/help_test/help_test.hpp.

#include "help_test.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_console/console.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

#include <unistd.h>

namespace help_test {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

namespace {

std::string read_whole(const std::string &path)
{
    std::string out;
    FILE *handle = fopen(path.c_str(), "rb");
    if (handle == nullptr)
        return out;
    char buffer[4096];
    size_t got = 0;
    while ((got = fread(buffer, 1, sizeof buffer, handle)) > 0)
        out.append(buffer, got);
    fclose(handle);
    return out;
}

} // namespace

std::string capture(const std::function<void()> &body)
{
    const std::string path = "/tmp/help_test." + std::to_string(getpid());

    // FLUSH BEFORE THE SWAP, console_test's note: this suite's own printf goes
    // out only on failure, which is exactly when somebody is reading it.
    fflush(stdout);

    const int saved = dup(1);
    FILE *sink = fopen(path.c_str(), "w+b");
    if (sink == nullptr || saved < 0) {
        check(false, "the capture could not make a temporary in /tmp");
        return std::string();
    }
    dup2(fileno(sink), 1);

    body();

    satellite::console::Console::the().shutdown();
    fflush(stdout);

    dup2(saved, 1);
    close(saved);
    fclose(sink);

    const std::string out = read_whole(path);
    remove(path.c_str());
    return out;
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

// COMPILED AND RUN, NOT CALLED. Calling the handler directly would skip the
// trie walk, the compiler arm that declines to compile the argument, the array
// index and the arity check -- which is to say, all of M18 except the prose.
std::string run(const std::string &source, bool *complained, int *code)
{
    using namespace satellite;

    *complained = false;
    *code = 0;

    words::Words words;
    Parse parsed = parse(source, words);
    if (!parsed.ok()) {
        check(false, "a fixture did not parse");
        return std::string();
    }
    resolve::Resolved resolved = resolve::resolve(parsed.ast, words);
    if (!resolved.ok()) {
        // A RESOLVE-TIME REFUSAL IS AN ANSWER AND NOT A BROKEN FIXTURE. It is
        // how `satellite.help(satellite.consle)` is supposed to end, and the
        // code is what the caller wants to see.
        *complained = true;
        if (!resolved.problems.empty())
            *code = static_cast<int>(resolved.problems.front().code);
        return std::string();
    }
    eval::Program program = eval::compile(parsed.ast, resolved, words);
    if (!program.ok()) {
        *complained = true;
        if (!program.problems.empty())
            *code = static_cast<int>(program.problems.front().code);
        return std::string();
    }

    const int which =
        program.find(static_cast<words::PathId>(words::NodeId::MAIN));
    if (which < 0) {
        check(false, "a fixture declares no satellite.main");
        return std::string();
    }

    eval::Policy policy;
    policy.max_depth = 64ull * 1024 * 1024;
    eval::Machine machine(program.closures, parsed.ast, policy);

    std::string printed = capture([&] {
        machine.run_top_level();
        if (machine.ok())
            machine.call(static_cast<uint32_t>(which), {});
    });

    if (!machine.ok()) {
        *complained = true;
        if (!machine.problems().empty())
            *code = static_cast<int>(machine.problems().front().code);
    }
    return printed;
}

} // namespace help_test

int main()
{
    help_test::section_predicate();
    help_test::section_entries();
    help_test::section_walking();

    if (help_test::failures != 0) {
        printf("help_test: %d failed\n", help_test::failures);
        return 1;
    }
    printf("help_test: ok\n");
    return 0;
}
