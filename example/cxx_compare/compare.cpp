// satellite against compiled C++, with the compiler's own time counted.
//
// example/class_bench.cpp reports 0.077 s against satellite's 2.88 s and that
// number is true, but it is not the whole cost of running a C++ program. Before
// clang's output can run, clang has to produce it — and for anything smaller
// than the benchmark, that step is most of the wall clock. This driver measures
// the thing a person actually waits for:
//
//     C++        compile + run
//     satellite  run, which already contains lex, parse and resolve
//
// Two programs, because the answer reverses between them and only reporting one
// would be picking a side:
//
//   * hello world — one line of output. satellite finishes before clang has
//     finished reading <iostream>.
//   * the million-object benchmark — enough work that the interpreter's
//     per-node cost dominates everything, including the compile.
//
// The BREAK-EVEN line is the honest summary of both. C++ pays for the compiler
// once and satellite pays for the tree walk every run, so the question is how
// many runs it takes for the compile to pay for itself. Under one run means C++
// is ahead immediately; a large number means you would have to run the program
// that many times before compiling was worth doing at all.
//
// Everything is fork + execvp rather than system(), so no shell is measured.
//
//   cd example/cxx_compare && make run

#include <chrono>
#include <cstdio>
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

// Runs a command to completion with its stdout sent to /dev/null, and returns
// the wall-clock seconds it took. Negative on failure.
//
// The output goes to /dev/null rather than a pipe because the benchmark writes
// a million lines and a pipe would measure this process's ability to drain
// them. Both sides are redirected identically, so whatever the null device
// costs, it costs both.
bool files_match(const std::string &left, const std::string &right)
{
    FILE *a = fopen(left.c_str(), "rb");
    FILE *b = fopen(right.c_str(), "rb");
    if (!a || !b) {
        if (a) fclose(a);
        if (b) fclose(b);
        return false;
    }
    bool same = true;
    char pa[65536], pb[65536];
    for (;;) {
        const size_t na = fread(pa, 1, sizeof pa, a);
        const size_t nb = fread(pb, 1, sizeof pb, b);
        if (na != nb || memcmp(pa, pb, na) != 0) { same = false; break; }
        if (na == 0) break;
    }
    fclose(a);
    fclose(b);
    return same;
}

double timed(const std::vector<std::string> &argv, bool quiet = true,
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
        if (!capture.empty()) {
            const int out =
                open(capture.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out >= 0) { dup2(out, STDOUT_FILENO); close(out); }
        } else if (quiet) {
            const int null = open("/dev/null", O_WRONLY);
            if (null >= 0) {
                dup2(null, STDOUT_FILENO);
                dup2(null, STDERR_FILENO);
                close(null);
            }
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

// The mean of `repeats` runs. Compiling is deterministic enough that one is
// usually right, but running is not, and a single sample of a 3-second walk is
// not a measurement.
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

struct Result {
    std::string name;
    double compile = 0;     // C++ only
    double cxx_run = 0;
    double satellite_run = 0;

    double cxx_total() const { return compile + cxx_run; }
};

void report(const Result &r)
{
    printf("\n%s\n", r.name.c_str());
    for (size_t i = 0; i < r.name.size(); i++)
        printf("-");
    printf("\n");

    printf("  c++        compile          %12.6f s\n", r.compile);
    printf("  c++        run              %12.6f s\n", r.cxx_run);
    printf("  c++        compile + run    %12.6f s\n", r.cxx_total());
    printf("  satellite  run              %12.6f s\n", r.satellite_run);

    const double first_run = r.satellite_run / r.cxx_total();
    printf("\n  first run:  satellite is %.2fx %s than c++ compile + run\n",
           first_run < 1 ? 1 / first_run : first_run,
           first_run < 1 ? "FASTER" : "slower");

    const double penalty = r.satellite_run - r.cxx_run;
    if (penalty <= 0) {
        printf("  break-even: never — satellite runs faster than the compiled "
               "program too\n");
        return;
    }
    // How many runs before the compile has paid for itself. Below one, C++ is
    // ahead from the very first run; above one, that many runs is the price of
    // admission.
    printf("  break-even: c++ overtakes after %.2f run%s "
           "(compile %.3f s / per-run penalty %.3f s)\n",
           r.compile / penalty, r.compile / penalty == 1 ? "" : "s", r.compile,
           penalty);
}

} // namespace

int main(int argc, char *argv[])
{
    const int repeats = argc > 1 ? std::max(1, atoi(argv[1])) : 3;

    const std::string here = directory_of_this_program();
    const std::string root = here + "/../..";
    const std::string satellite = root + "/satl";

    if (access(satellite.c_str(), X_OK) != 0) {
        fprintf(stderr,
                "compare: %s is not built. Run `make` in the repository root "
                "first — building the interpreter is not part of what this "
                "measures, because you do it once and then never again.\n",
                satellite.c_str());
        return 2;
    }

    printf("satellite vs compiled c++, with the compiler's own time counted\n");
    printf("mean of %d run%s; compile timed from a clean build each time\n",
           repeats, repeats == 1 ? "" : "s");

    struct Case {
        const char *name;
        const char *target;         // make target, and the binary it produces
        const char *source;         // the satellite program
    };
    const Case cases[] = {
        {"hello world", "hello", "/example/hello_world.satl"},
        {"300k objects, 300k prints, 300k string transfers", "bench",
         "/example/py_compare/bench.satl"},
    };

    for (const Case &one : cases) {
        Result result;
        result.name = one.name;

        // Clean first, every time: make is perfectly happy to report success
        // in no time at all for a target that is already built, and a compile
        // time of zero would be the most flattering possible lie.
        double compile_total = 0;
        for (int i = 0; i < repeats; i++) {
            if (timed({"rm", "-f", here + "/" + one.target}) < 0)
                return 1;
            const double once = timed({"make", "-C", here, one.target});
            if (once < 0)
                return 1;
            compile_total += once;
        }
        result.compile = compile_total / repeats;

        // Verify the two programs agree before timing either. py_compare has
        // always done this; this one did not, so every ratio it printed rested
        // on an assumption nobody had checked. A benchmark comparing two
        // different programs measures nothing.
        const std::string cxx_out = here + "/.cxx.out";
        const std::string sat_out = here + "/.satellite.out";
        if (timed({here + "/" + one.target}, true, cxx_out) < 0 ||
            timed({satellite, "--run", root + one.source}, true, sat_out) < 0)
            return 1;
        if (!files_match(cxx_out, sat_out)) {
            fprintf(stderr, "compare: %s and %s do not produce the same "
                            "output; refusing to time them\n",
                    one.target, one.source);
            return 1;
        }
        unlink(cxx_out.c_str());
        unlink(sat_out.c_str());

        result.cxx_run = timed_mean({here + "/" + one.target}, repeats);
        result.satellite_run =
            timed_mean({satellite, "--run", root + one.source}, repeats);
        if (result.cxx_run < 0 || result.satellite_run < 0)
            return 1;

        report(result);
    }

    printf("\nnote: `make` itself is inside the compile figure, because "
           "invoking the build\n"
           "      is part of what compiling costs. satellite's figure already "
           "contains its\n"
           "      own front end — lex, parse and resolve run on every "
           "invocation.\n");
    return 0;
}
