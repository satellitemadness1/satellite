// eval_test -- the proof that src/evaluator/ and src/satellite_value/ do what
// PLAN M9 specifies. See tests/eval_test/eval_test.hpp for what is being
// proved and why this binary links no machine_limits.

#include "eval_test.hpp"

#include "evaluator/dispatch.hpp"
#include "satellite_value/render.hpp"

#include <cstdio>
#include <string>
#include <utility>
#include <vector>

namespace eval_test {

int failures = 0;
std::string example_directory = "example";
satellite::eval::Ending last_ending = satellite::eval::Ending::Finished;
std::vector<satellite::errors::Diagnostic> last_problems;
unsigned long long last_peak = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

void build(const std::string &source, Run &into)
{
    into.parsed = satellite::parse(source, into.words);
    if (!into.parsed.ok())
        return;
    into.resolved = satellite::resolve::resolve(into.parsed.ast, into.words);
    if (!into.resolved.ok())
        return;
    into.program = satellite::eval::compile(into.parsed.ast, into.resolved, into.words);
    into.built = true;
}

satellite::Value call(Run &run, const std::string &capsule,
                      const std::vector<long long> &arguments,
                      unsigned long long ceiling)
{
    using namespace satellite;

    last_ending = eval::Ending::Finished;
    last_problems.clear();
    last_peak = 0;
    if (!run.built)
        return Value::nothing();

    // UNDER `library` FIRST, WHICH IS WHERE A USER'S CAPSULE IS DEFINED --
    // parser_declarations.cpp's top_level() hands `satellite.library` in as the
    // owner, and WORD_NUMBERS §3's worked example is that a user's first name
    // there is `1 14 3`.
    words::PathId path = run.words.find(words::NodeId::LIBRARY, capsule);
    if (path == words::kNoPath)
        path = run.words.find(words::NodeId::SATELLITE, capsule);
    const int which = path == words::kNoPath ? -1 : run.program.find(path);
    if (which < 0) {
        check(false, "the fixture declares no capsule called `" + capsule + "`");
        return Value::nothing();
    }

    eval::Policy policy;
    policy.max_depth = ceiling;
    policy.division_digits = 34;

    eval::Machine machine(run.program.closures, run.parsed.ast, policy);
    machine.run_top_level();
    Value answer;
    if (machine.ok()) {
        std::vector<Value> given;
        for (const long long number : arguments)
            given.push_back(Value::number(Number(number)));
        answer = machine.call(static_cast<uint32_t>(which), given);
    }
    last_ending = machine.ending();
    last_problems = machine.problems();
    last_peak = machine.peak_bytes();
    return answer;
}

std::string answer_of(Run &run, const std::string &capsule,
                      const std::vector<long long> &arguments)
{
    return satellite::text_of(call(run, capsule, arguments));
}

bool raised(const Run &run, satellite::errors::Code code)
{
    for (const satellite::errors::Diagnostic &at : run.program.problems)
        if (at.code == code)
            return true;
    return false;
}

bool ran_into(satellite::errors::Code code)
{
    for (const satellite::errors::Diagnostic &at : last_problems)
        if (at.code == code)
            return true;
    return false;
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

} // namespace eval_test

int main(int argc, char **argv)
{
    if (argc > 1)
        eval_test::example_directory = argv[1];

    eval_test::section_compile();
    eval_test::section_arithmetic();
    eval_test::section_control();
    eval_test::section_calls();
    eval_test::section_depth();
    eval_test::section_dispatch();
    eval_test::section_scalars();
    eval_test::section_variant();
    eval_test::section_floats();
    eval_test::section_interrupted();
    eval_test::section_clock_and_dice();

    if (eval_test::failures != 0) {
        printf("eval_test: %d failed\n", eval_test::failures);
        return 1;
    }
    printf("eval_test: ok\n");
    return 0;
}
