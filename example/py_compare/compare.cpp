// satellite against CPython, one command.
//
// Both are interpreters, so unlike example/cxx_compare there is no build step
// on either side and nothing to argue about: the number is the wall clock of
// the whole process, front end included, which is what a person waits for.
//
// Two programs, because they measure different things:
//
//   * the feature tour — 74 lines of output and almost no work, so it is
//     dominated by interpreter STARTUP: reading the file, parsing it, and for
//     CPython importing its own standard library.
//   * the object benchmark — 300000 constructions, 300000 prints and 300000
//     method calls, so it is dominated by EXECUTION.
//
// The two programs are verified to produce byte-identical output before either
// is timed. A benchmark that measures two different programs measures nothing,
// so the check is not a formality — it runs first and refuses to time anything
// that disagrees.
//
// Everything is fork + execvp, so no shell is in the measurement.
//
//   make -C example/py_compare run          (or `make python` from the root)

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include <fcntl.h>
#include <limits.h>
#include <sys/wait.h>
#include <unistd.h>

namespace {

using Clock = std::chrono::steady_clock;

std::string directory_of_this_program()
{
    char buffer[PATH_MAX];
    const ssize_t length = readlink("/proc/self/exe", buffer, sizeof buffer - 1);
    if (length <= 0)
        return ".";
    buffer[length] = '\0';
    char *slash = strrchr(buffer, '/');
    if (!slash)
        return ".";
    *slash = '\0';
    return buffer;
}

// Runs a command to completion, sending stdout to `capture` (or /dev/null when
// empty), and returns the wall-clock seconds. Negative on failure.
double timed(const std::vector<std::string> &argv,
             const std::string &capture = std::string())
{
    std::vector<char *> raw;
    raw.reserve(argv.size() + 1);
    for (const std::string &arg : argv)
        raw.push_back(const_cast<char *>(arg.c_str()));
    raw.push_back(nullptr);

    const auto start = Clock::now();

    const pid_t child = fork();
    if (child < 0)
        return -1;

    if (child == 0) {
        const int out = capture.empty()
                            ? open("/dev/null", O_WRONLY)
                            : open(capture.c_str(),
                                   O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (out >= 0) {
            dup2(out, STDOUT_FILENO);
            close(out);
        }
        execvp(raw[0], raw.data());
        _exit(127);
    }

    int status = 0;
    if (waitpid(child, &status, 0) < 0)
        return -1;

    const double seconds =
        std::chrono::duration<double>(Clock::now() - start).count();

    if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
        fprintf(stderr, "compare: %s failed (status %d)\n", raw[0], status);
        return -1;
    }
    return seconds;
}

double timed_mean(const std::vector<std::string> &argv, int repeats)
{
    double total = 0;
    for (int i = 0; i < repeats; i++) {
        const double one = timed(argv);
        if (one < 0)
            return -1;
        total += one;
    }
    return total / repeats;
}

bool files_match(const std::string &left, const std::string &right)
{
    FILE *a = fopen(left.c_str(), "rb");
    FILE *b = fopen(right.c_str(), "rb");
    if (!a || !b) {
        if (a)
            fclose(a);
        if (b)
            fclose(b);
        return false;
    }

    bool same = true;
    char pa[65536];
    char pb[65536];
    for (;;) {
        const size_t na = fread(pa, 1, sizeof pa, a);
        const size_t nb = fread(pb, 1, sizeof pb, b);
        if (na != nb || memcmp(pa, pb, na) != 0) {
            same = false;
            break;
        }
        if (na == 0)
            break;
    }
    fclose(a);
    fclose(b);
    return same;
}

} // namespace

int main(int argc, char *argv[])
{
    const int repeats = argc > 1 ? std::max(1, atoi(argv[1])) : 5;

    const std::string here = directory_of_this_program();
    const std::string root = here + "/../..";
    const std::string satellite = root + "/satl";

    if (access(satellite.c_str(), X_OK) != 0) {
        fprintf(stderr, "compare: %s is not built. Run `make` in the "
                        "repository root first.\n",
                satellite.c_str());
        return 2;
    }

    struct Case {
        const char *name;
        const char *what;
        std::string python;
        std::string sat;
    };
    const Case cases[] = {
        {"feature tour", "74 lines out, almost no work — startup dominates",
         here + "/tour.py", root + "/example/website/tour.satl"},
        {"object benchmark",
         "300000 constructions, prints and method calls — execution dominates",
         here + "/bench.py", here + "/bench.satl"},
    };

    printf("satellite vs CPython — mean of %d run%s, whole-process wall clock\n",
           repeats, repeats == 1 ? "" : "s");

    for (const Case &one : cases) {
        printf("\n%s\n", one.name);
        for (size_t i = 0; i < strlen(one.name); i++)
            printf("-");
        printf("\n  %s\n\n", one.what);

        // Same output, or the numbers below compare two different programs.
        const std::string py_out = here + "/.python.out";
        const std::string sat_out = here + "/.satellite.out";
        if (timed({"python3", one.python}, py_out) < 0 ||
            timed({satellite, "--run", one.sat}, sat_out) < 0)
            return 1;
        if (!files_match(py_out, sat_out)) {
            fprintf(stderr,
                    "compare: %s and %s do not produce the same output; "
                    "refusing to time them\n",
                    one.python.c_str(), one.sat.c_str());
            return 1;
        }

        const double python = timed_mean({"python3", one.python}, repeats);
        const double sat = timed_mean({satellite, "--run", one.sat}, repeats);
        if (python < 0 || sat < 0)
            return 1;

        printf("  python3    %12.6f s\n", python);
        printf("  satellite  %12.6f s\n", sat);

        const double ratio = sat / python;
        printf("\n  satellite is %.2fx %s\n", ratio < 1 ? 1 / ratio : ratio,
               ratio < 1 ? "FASTER" : "slower");

        unlink(py_out.c_str());
        unlink(sat_out.c_str());
    }

    printf("\nboth programs' output was verified byte-identical before timing.\n");
    return 0;
}
