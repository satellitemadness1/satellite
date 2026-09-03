// console_test -- the proof that src/satellite_console/ does what PLAN M10 and
// DESIGN §10.1 specify. See tests/console_test/console_test.hpp.

#include "console_test.hpp"

#include "satellite_console/console.hpp"

#include <cstdio>
#include <string>
#include <vector>

#include <unistd.h>

namespace console_test {

int failures = 0;

void check(bool ok, const std::string &what)
{
    if (!ok) {
        printf("FAIL: %s\n", what.c_str());
        failures++;
    }
}

namespace {

// The whole of a file. The temporary is this process's and nothing else reads
// it, so a failure to open is a failure of the test rather than a case.
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
    const std::string path =
        "/tmp/console_test." + std::to_string(getpid());

    // FLUSH BEFORE THE SWAP, or this suite's own `printf`s land in the file
    // being captured. The harness prints only on failure, which is exactly when
    // somebody is reading the output and least wants it moved.
    fflush(stdout);

    const int saved = dup(1);
    FILE *sink = fopen(path.c_str(), "w+b");
    if (sink == nullptr || saved < 0) {
        check(false, "the capture could not make a temporary in /tmp");
        return std::string();
    }
    dup2(fileno(sink), 1);

    body();

    // THE CONSOLE IS SHUT DOWN BEFORE THE DESCRIPTOR GOES BACK, always. A
    // fixture that tests the shutdown has already called this and it is safe
    // twice; a fixture that does not would otherwise leave a printer thread
    // holding a `stdout` that is about to become the terminal again.
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

bool every_line_is(const std::string &text, const std::string &line, size_t *count)
{
    size_t lines = 0;
    size_t at = 0;
    while (at < text.size()) {
        const size_t end = text.find('\n', at);
        if (end == std::string::npos)
            return false; // a last line with no terminator is a torn write
        if (text.compare(at, end - at, line) != 0)
            return false;
        lines++;
        at = end + 1;
    }
    *count = lines;
    return true;
}

} // namespace console_test

int main()
{
    console_test::section_printing();
    console_test::section_dispatching();

    if (console_test::failures != 0) {
        printf("console_test: %d failed\n", console_test::failures);
        return 1;
    }
    printf("console_test: ok\n");
    return 0;
}
