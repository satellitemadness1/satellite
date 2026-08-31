// limits_test -- the proof that src/machine_limits/ and src/system_facts/ do
// what PLAN §4.5 specifies. See tests/limits_test/limits_test.hpp.
//
// THE SUBJECT IS A MACHINE AND THAT MAKES THIS SUITE DIFFERENT FROM THE FIVE
// BEFORE IT. words_test checks a transcription against a document, and
// parser_test checks a tree against a grammar; both have an authority to be
// wrong against. Here the authority is /proc, which is also what the code under
// test reads -- so an assertion that hardware_threads() equals nproc is really
// two readers of one file agreeing, and it would pass on a machine where both
// were wrong.
//
// WHAT IS CHECKABLE ANYWAY, AND IT IS MOST OF IT: the RELATIONSHIPS between the
// facts (cores never exceed threads, used plus available is total, a resident
// set is not zero), the POLICY over them (the fallback order, the clamp, the
// origins), and everything about the file, the codes and the pool -- none of
// which is a machine reading at all. section_facts says at each assertion which
// kind it is.

#include "limits_test.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace limits_test {

int failures = 0;
std::string example_directory = "example";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

bool reads(const std::string &text, satellite::limits::Held &into,
           std::vector<satellite::errors::Diagnostic> &problems)
{
    return satellite::limits::read_config(text, into, problems);
}

std::vector<satellite::errors::Code> codes_in(const std::string &text)
{
    satellite::limits::Held into;
    std::vector<satellite::errors::Diagnostic> problems;
    reads(text, into, problems);

    std::vector<satellite::errors::Code> codes;
    for (const satellite::errors::Diagnostic &problem : problems) {
        codes.push_back(problem.code);
        for (const satellite::errors::Note &remark : problem.notes)
            codes.push_back(remark.code);
    }
    return codes;
}

std::string example(const std::string &name)
{
    const std::string path = example_directory + "/" + name;
    std::FILE *file = std::fopen(path.c_str(), "rb");
    if (!file) {
        check(false, "example/" + name + " could not be opened -- this suite's "
                     "subject is that file, so a missing one is a failure and "
                     "never a skip");
        return std::string();
    }
    std::string all;
    char buffer[4096];
    size_t got;
    while ((got = std::fread(buffer, 1, sizeof buffer, file)) > 0)
        all.append(buffer, got);
    std::fclose(file);
    return all;
}

} // namespace limits_test

int main(int argc, char **argv)
{
    if (argc > 1)
        limits_test::example_directory = argv[1];

    // THE ORDER IS LOAD-BEARING IN ONE PLACE AND IT IS THE LAST TWO.
    // section_pool starts the pool with a size of its own choosing, and
    // section_examples calls limits::begin(), which starts one sized from the
    // machine -- and pool::start() is a no-op once a pool exists. Run the other
    // way round, section_pool's assertions are about a 24-thread pool it did
    // not ask for, and two of them fail. Written down rather than left as an
    // ordering somebody tidies alphabetically.
    limits_test::section_reading();
    limits_test::section_facts();
    limits_test::section_pool();
    limits_test::section_examples();

    if (limits_test::failures != 0) {
        printf("limits_test: %d failed\n", limits_test::failures);
        return 1;
    }
    printf("limits_test: ok\n");
    return 0;
}
