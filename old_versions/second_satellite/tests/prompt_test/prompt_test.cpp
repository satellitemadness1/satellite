// The prompt suite's main. See prompt_test.hpp for what is being proved.

#include "prompt_test.hpp"

#include <cstdio>

namespace prompt_test {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (ok)
        return;
    failures++;
    fprintf(stderr, "  FAIL  %s\n", what.c_str());
}

bool holds(const std::string &text, const std::string &needle)
{
    return text.find(needle) != std::string::npos;
}

} // namespace prompt_test

int main()
{
    prompt_test::section_keys();
    prompt_test::section_editing();
    prompt_test::section_blocks();
    prompt_test::section_session();

    if (prompt_test::failures != 0) {
        fprintf(stderr, "prompt_test: %d failed\n", prompt_test::failures);
        return 1;
    }
    fputs("prompt_test ok\n", stdout);
    return 0;
}
