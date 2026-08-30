// satc_test -- the proof that src/satellite_cache/ writes what SATC.md
// specifies. See tests/satc_test/satc_test.hpp for the harness.
//
// THE SUBJECT IS A SUBSTITUTION, AND A SUBSTITUTION FAILS QUIETLY. A parser
// that goes wrong produces a tree that will not print; a writer that goes wrong
// produces a file that reads back perfectly and says something else. SATC.md
// §2 states the cost in one line -- "a `.satc` that is merely out-of-date costs
// one walk to rebuild, and a `.satc` that is wrong costs a program that does
// the wrong thing" -- and every check here is aimed at the second half of it.

#include "satc_test.hpp"

#include "abstract_syntax_tree/ast.hpp"
#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>

namespace satc_test {

int failures = 0;
std::string example_directory = "example";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

std::string body(const std::string &source)
{
    satellite::words::Words words;
    const satellite::Parse parsed = satellite::parse(source, words);

    // A FIXTURE THAT DOES NOT PARSE IS A FAILURE OF THIS TEST, not of the
    // writer, and it is reported as one. Returning the partial tree's output
    // would let a mistyped fixture pass a check by accident -- an empty line
    // contains no wrong number.
    if (!parsed.ok()) {
        check(false, "fixture did not parse: " +
                         satellite::errors::sentence(parsed.errors.front()));
        return std::string();
    }
    return satellite::cache::body_text(parsed.ast, words);
}

bool read_example(const std::string &name, std::string &into)
{
    FILE *handle = fopen((example_directory + "/" + name).c_str(), "rb");
    if (handle == nullptr)
        return false;
    char buffer[4096];
    size_t got = 0;
    while ((got = fread(buffer, 1, sizeof buffer, handle)) > 0)
        into.append(buffer, got);
    const bool whole = ferror(handle) == 0;
    fclose(handle);
    return whole;
}

std::string statements(const std::string &source)
{
    const std::string whole =
        body("satellite.capsule fixture()\n{\n" + source + "\n}\n");

    // The capsule head, its `{` and its `}` come off, and four spaces of indent
    // with them, so a check reads as the statement it is about.
    std::string out;
    size_t at = 0;
    int line_number = 0;
    while (at < whole.size()) {
        const size_t end = whole.find('\n', at);
        const std::string line =
            whole.substr(at, end == std::string::npos ? end : end - at);
        at = end == std::string::npos ? whole.size() : end + 1;
        if (line_number++ < 2 || line == "}")
            continue;
        out += line.size() > 4 ? line.substr(4) : line;
        out += "\n";
    }
    return out;
}

std::string one_line(const std::string &statement)
{
    const std::string written = statements(statement);
    const size_t end = written.find('\n');
    return code_of(written.substr(0, end));
}

std::string line_with(const std::string &text, const std::string &needle)
{
    size_t at = 0;
    while (at < text.size()) {
        const size_t end = text.find('\n', at);
        const std::string line =
            text.substr(at, end == std::string::npos ? end : end - at);
        if (line.find(needle) != std::string::npos) {
            const size_t first = line.find_first_not_of(' ');
            return first == std::string::npos ? line : line.substr(first);
        }
        at = end == std::string::npos ? text.size() : end + 1;
    }
    return std::string();
}

std::string comment_of(const std::string &line)
{
    const size_t at = line.find("// ");
    return at == std::string::npos ? std::string() : line.substr(at + 3);
}

std::string code_of(const std::string &line)
{
    const size_t at = line.find("// ");
    std::string out = at == std::string::npos ? line : line.substr(0, at);
    while (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

} // namespace satc_test

int main(int argc, char **argv)
{
    if (argc > 1)
        satc_test::example_directory = argv[1];

    satc_test::section_shapes();
    satc_test::section_ownership();
    satc_test::section_examples();
    satc_test::section_header();
    satc_test::section_reading();
    satc_test::section_writing();

    if (satc_test::failures != 0) {
        printf("satc_test: %d failed\n", satc_test::failures);
        return 1;
    }
    printf("satc_test: ok\n");
    return 0;
}
