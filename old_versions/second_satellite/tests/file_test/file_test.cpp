// file_test -- the proof that src/satellite_file/ and src/satellite_directory/
// do what PLAN M19 specifies. See tests/file_test/file_test.hpp.

#include "file_test.hpp"

#include "error_reporter/report.hpp"
#include "evaluator/evaluate.hpp"
#include "evaluator/machine.hpp"
#include "name_resolver/resolve.hpp"
#include "parser/parser.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

#include <unistd.h>

namespace file_test {

using namespace satellite;

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

void install_every_module()
{
    // THE CONSOLE IS AMONG THEM because every fixture below reports through
    // `display`, and a suite that installed the file rows without it would
    // refuse at the line that says what happened rather than at the line under
    // test -- which is a suite that can only fail one way.
    console::install_handlers();
    scalars::install_handlers();
    containers::install_handlers();
    file::install_handlers();
    directory::install_handlers();
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
    const std::string path = "/tmp/file_test." + std::to_string(getpid());

    // FLUSH BEFORE THE SWAP: this suite's own printf goes out only on failure,
    // which is exactly when somebody is reading it.
    fflush(stdout);

    const int saved = dup(1);
    FILE *sink = fopen(path.c_str(), "w+b");
    if (sink == nullptr || saved < 0) {
        check(false, "the capture could not make a temporary in /tmp");
        return std::string();
    }
    dup2(fileno(sink), 1);

    body();

    console::Console::the().shutdown();
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

// COMPILED AND RUN, NOT CALLED. Calling a handler directly would skip the trie
// walk, the receiver binding, the array index and the arity check -- which is
// to say, all of the milestone except the syscalls.
std::string run(const std::string &source, bool *complained, int *code)
{
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

std::string program(const std::string &body)
{
    std::string out = "satellite.include(satellite)\n"
                      "\n"
                      "satellite.capsule satellite.main()\n"
                      "{\n";
    out += body;
    out += "\n}\n";
    return out;
}

std::string ran(const std::string &body, const std::string &what)
{
    bool complained = false;
    int code = 0;
    const std::string out = run(program(body), &complained, &code);
    if (complained)
        check(false, what + " -- refused with S0" + std::to_string(code));
    return out;
}

void refuses(const std::string &body, int expected, const std::string &what)
{
    bool complained = false;
    int code = 0;
    run(program(body), &complained, &code);
    check(complained, what + " -- nothing refused");
    if (complained)
        check(code == expected, what + " -- expected S" +
                                    std::to_string(expected) + ", got S" +
                                    std::to_string(code));
}

} // namespace file_test

int main()
{
    // A DIRECTORY OF ITS OWN, AND IT GOES AWAY. The subject is a filesystem and
    // `satellite.file.new` is O_EXCL, so a suite that wrote into the tree would
    // pass once and fail forever after -- which is the exact defect PLAN M19
    // forbids in the acceptance program, arriving in the thing that checks it.
    char pattern[] = "/tmp/file_test_XXXXXX";
    const char *where = mkdtemp(pattern);
    if (where == nullptr) {
        printf("file_test: could not make a temporary directory\n");
        return 1;
    }
    if (chdir(where) != 0) {
        printf("file_test: could not enter %s\n", where);
        return 1;
    }

    file_test::install_every_module();
    file_test::section_round_trip();
    file_test::section_reading();
    file_test::section_refusals();
    file_test::section_listing();

    // Back out before removing it, so the directory being removed is not the
    // one this process is standing in.
    if (chdir("/") != 0)
        file_test::check(false, "could not leave the temporary directory");
    const std::string cleanup = std::string("rm -rf ") + where;
    if (system(cleanup.c_str()) != 0)
        file_test::check(false, "the temporary directory was not removed");

    if (file_test::failures != 0) {
        printf("file_test: %d failed\n", file_test::failures);
        return 1;
    }
    printf("file_test: ok\n");
    return 0;
}
