// reporter_test -- the proof that src/error_reporter/ says what DESIGN §9
// specifies. See tests/reporter_test/reporter_test.hpp for the harness.
//
// THE SUBJECT IS THE ONE THING EVERY LATER MILESTONE WILL TOUCH, which is why
// this suite is written against the RENDERED TEXT rather than against the
// Diagnostic struct. §9's requirement is "rendering in exactly one place", and
// a test that asserted on fields would pass while that one place printed the
// caret two columns to the left -- which is the defect the first satellite's
// own format_error carried until it grew a clamp, and the reason its comment
// about it is quoted into report.cpp rather than paraphrased.

#include "reporter_test.hpp"

#include "error_reporter/report.hpp"
#include "parser/parser.hpp"
#include "satellite_words/words.hpp"

#include <cstdio>
#include <string>
#include <vector>

namespace reporter_test {

int failures = 0;
std::string example_directory = "example";

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

std::string line(const std::string &text, size_t n)
{
    size_t at = 0;
    for (size_t i = 0; i < n; i++) {
        const size_t end = text.find('\n', at);
        if (end == std::string::npos)
            return std::string();
        at = end + 1;
    }
    const size_t end = text.find('\n', at);
    return text.substr(at, end == std::string::npos ? end : end - at);
}

namespace {

// report.cpp's kNarrowestGutter, which is a floor and not a constant this can
// read -- so it is stated once here and the two builders below are the only
// place in the suite that knows it.
constexpr size_t kGutter = 4;

} // namespace

std::string source_row(unsigned line_number, const std::string &text)
{
    const std::string number = std::to_string(line_number);
    return std::string(kGutter - number.size(), ' ') + number + " | " + text;
}

std::string caret_row(size_t column, size_t width)
{
    return std::string(kGutter, ' ') + " | " + std::string(column - 1, ' ') +
           std::string(width, '^');
}

std::vector<satellite::errors::Diagnostic> problems_in(const std::string &source)
{
    satellite::words::Words words;
    return satellite::parse(source, words).errors;
}

std::string rendered(const std::string &source, const std::string &path)
{
    return satellite::errors::render(problems_in(source),
                                     satellite::errors::Source{path, source});
}

} // namespace reporter_test

int main(int argc, char **argv)
{
    if (argc > 1)
        reporter_test::example_directory = argv[1];

    reporter_test::section_codes();
    reporter_test::section_rendering();
    reporter_test::section_suggesting();
    reporter_test::section_lexing();
    reporter_test::section_parsing();

    if (reporter_test::failures != 0) {
        printf("reporter_test: %d failed\n", reporter_test::failures);
        return 1;
    }
    printf("reporter_test: ok\n");
    return 0;
}
