// time_test/satellite.library/main.cpp -- satl, Python and C++ raced on the same programs.
//
// The author, 2026-09-23: "let's build a /time_test/satellite.library/main.cpp that races
// in C++, so I can just... run every test myself, and I can compile the C++ myself", then
// "build the equivalent in python and in C++ and build it all together, so we can see
// everything in python and in C++ too".
//
// BUILD (either compiler):
//     clang++ -std=c++20 -O2 main.cpp -o race
//     g++     -std=c++20 -O2 main.cpp -o race
//
// RUN, from this folder:
//     ./race                                   every program, 5 rounds: satl on PATH, Python, C++
//     ./race 3 ~/.satl/satl ../../build/satl   3 rounds, two satls, Python and C++
//     ./race 5 ~/.satl/satl ints long_add      only the programs named
//     ./race --no-python --no-cpp              satl alone
//     ./race --python=/usr/bin/pypy3           another Python (PyPy is a JIT: say so when you quote it)
//     ./race --cxx=g++                         another C++ compiler (clang++ is used when it is there)
//
// THE THREE VERSIONS OF EACH PROGRAM: programs/<name>.satl, python/<name>.py and
// cpp/<name>.cpp. Each Python and C++ version does the same work the same number of times
// and prints exactly what satl prints -- the race checks that on every run. Numbers that
// satl never lets overflow are Python ints and cpp/bignum.hpp; satl's 128-place floats are
// decimal.Decimal and a number scaled by 10^128. Each file's first comment says wherever it
// is not a line-for-line translation.
//
// WHAT IS TIMED, so the numbers can be trusted or doubted for the right reasons:
//   - wall clock around the whole run, start-up included, each started by the shell with
//     stdin closed (a satl waiting at its prompt would be timed waiting), SATL_NO_WINDOW=1;
//   - every contestant runs each program once first, untimed (the disk cache is warm after);
//   - then ROUNDS rounds, and each round starts with a different contestant;
//   - C++ is compiled once per program before the rounds, and the compile is timed on its own.
//     "with its compile" adds it to C++'s best run: the race the author asked for on
//     2026-09-16, "race compiled C++ by including the compile step, which makes it a fair race";
//   - every run's time is printed, then best, median and worst.
//
// THE PROGRAMS: `empty` is the loop and nothing else, so the difference between it and
// `ints`, `short_float`, `long_add` or `long_multiply` is what one statement costs; all five
// run 200,000 turns.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <sys/stat.h>
#include <vector>

namespace {

struct Program {
    const char *name;
    const char *what;
};

const Program kPrograms[] = {
    {"race_program", "the author's race: a 200-digit number times a 280-digit one, 1,000,000 turns"},
    {"everything", "every kind of work each turn: numbers, strings, lists, capsules, spacesuits; 40,000 turns"},
    {"arithmetic", "small whole numbers, 1,000,000 turns"},
    {"big_numbers", "x = x * 3 past a machine word, 150,000 turns"},
    {"strings", "a 100-character string joined and compared, 300,000 turns"},
    {"lists", "300,000 appends, then sum, max and sort"},
    {"empty", "the loop alone, 200,000 turns -- subtract this from the four below"},
    {"ints", "n = n + 1"},
    {"short_float", "f = f + 0.5"},
    {"long_add", "t = t + third, a float of 128 places"},
    {"long_multiply", "m = third * third"},
};

enum Kind { kind_satl, kind_python, kind_cpp };

struct Contestant {
    Kind kind;
    std::string path;     // the satl, the python, or (for C++) the compiler
    std::string label;
};

bool exists(const std::string &path)
{
    struct stat about {};
    return stat(path.c_str(), &about) == 0;
}

// The first line a command prints, for saying which Python and which compiler ran.
std::string first_line_of(const std::string &command)
{
    const std::string file = "out/.said";
    std::system((command + " > " + file + " 2>&1").c_str());
    std::ifstream in(file);
    std::string line;
    std::getline(in, line);
    return line;
}

double seconds_for(const std::string &command)
{
    const auto started = std::chrono::steady_clock::now();
    const int status = std::system(command.c_str());
    const auto ended = std::chrono::steady_clock::now();
    if (status != 0)
        std::printf("    (exited with status %d: %s)\n", status, command.c_str());
    return std::chrono::duration<double>(ended - started).count();
}

std::string command_for(const Contestant &who, const std::string &program, const std::string &output)
{
    const std::string quiet = " </dev/null >'" + output + "' 2>&1";
    switch (who.kind) {
    case kind_satl: return "SATL_NO_WINDOW=1 '" + who.path + "' 'programs/" + program + ".satl'" + quiet;
    case kind_python: return "'" + who.path + "' 'python/" + program + ".py'" + quiet;
    case kind_cpp: return "'out/" + program + ".cpp.bin'" + quiet;
    }
    return "false";
}

// WHAT A PROGRAM SAID. satl's own start-up lines end with a line of dashes, and everything
// after it is the program's; Python and C++ print only the program's (starting with the
// blank line satl prints after its dashes), so all of theirs is the answer.
std::string answer_in(const std::string &file, Kind kind)
{
    std::ifstream in(file);
    std::string line, answer;
    bool past_dashes = kind != kind_satl;
    while (std::getline(in, line)) {
        if (!past_dashes) {
            if (line.size() >= 10 && line.find_first_not_of('-') == std::string::npos) past_dashes = true;
            continue;
        }
        answer += line + "\n";
    }
    return answer;
}

double median(std::vector<double> times)
{
    std::sort(times.begin(), times.end());
    return times[times.size() / 2];
}

} // namespace

int main(int argc, char **argv)
{
    int rounds = 5;
    bool with_python = true, with_cpp = true;
    std::string python = exists("/usr/bin/python3") ? "/usr/bin/python3" : "python3";
    std::string compiler = std::system("command -v clang++ >/dev/null 2>&1") == 0 ? "clang++" : "c++";
    std::vector<Contestant> contestants;
    std::vector<std::string> chosen;
    for (int i = 1; i < argc; i++) {
        const std::string word = argv[i];
        if (word == "--no-python") with_python = false;
        else if (word == "--no-cpp") with_cpp = false;
        else if (word.rfind("--python=", 0) == 0) python = word.substr(9);
        else if (word.rfind("--cxx=", 0) == 0) compiler = word.substr(6);
        else if (i == 1 && word.find_first_not_of("0123456789") == std::string::npos) rounds = std::max(1, std::atoi(word.c_str()));
        else if (word.find('/') != std::string::npos || exists(word)) contestants.push_back({kind_satl, word, ""});
        else chosen.push_back(word);
    }
    // THE satl ON PATH, AND ~/.satl/satl WHEN THERE IS NONE: std::system runs /bin/sh, which
    // does not see a shell's alias -- and the author's `satl` is one.
    if (contestants.empty()) {
        const char *home = std::getenv("HOME");
        const std::string installed = std::string(home != nullptr ? home : "") + "/.satl/satl";
        const bool on_path = std::system("command -v satl >/dev/null 2>&1") == 0;
        contestants.push_back({kind_satl, on_path || !exists(installed) ? std::string("satl") : installed, ""});
    }
    for (std::size_t s = 0; s < contestants.size(); s++) contestants[s].label = "satl " + std::to_string(s + 1);
    std::system("mkdir -p out");
    if (with_python) contestants.push_back({kind_python, python, "python"});
    if (with_cpp) contestants.push_back({kind_cpp, compiler, "c++"});

    std::printf("%d rounds.\n", rounds);
    for (const Contestant &who : contestants) {
        if (who.kind == kind_satl) std::printf("  [%s] %s\n", who.label.c_str(), who.path.c_str());
        if (who.kind == kind_python)
            std::printf("  [python] %s -- %s\n", who.path.c_str(),
                        first_line_of("'" + who.path + "' -c 'import platform, sys; "
                                      "print(platform.python_implementation(), sys.version.split()[0])'").c_str());
        if (who.kind == kind_cpp)
            std::printf("  [c++] %s -std=c++20 -O2 -- %s\n", who.path.c_str(),
                        first_line_of("'" + who.path + "' --version").c_str());
    }
    std::printf("\n");

    for (const Program &program : kPrograms) {
        if (!chosen.empty() && std::find(chosen.begin(), chosen.end(), program.name) == chosen.end()) continue;
        const std::string name = program.name;
        std::printf("%s -- %s\n", program.name, program.what);

        // C++ IS BUILT FIRST, and the build is timed on its own.
        std::vector<Contestant> here;
        double compile_seconds = 0;
        for (const Contestant &who : contestants) {
            if (who.kind == kind_cpp) {
                compile_seconds = seconds_for("'" + who.path + "' -std=c++20 -O2 'cpp/" + name + ".cpp' -o 'out/" + name +
                                              ".cpp.bin' > 'out/" + name + ".compile' 2>&1");
                if (!exists("out/" + name + ".cpp.bin")) {
                    std::printf("  [c++] did not compile -- out/%s.compile says why\n", program.name);
                    continue;
                }
                std::printf("  [c++] compiled in %.3f s\n", compile_seconds);
            }
            here.push_back(who);
        }

        std::vector<std::vector<double>> times(here.size());
        for (std::size_t c = 0; c < here.size(); c++)
            seconds_for(command_for(here[c], name, "out/" + name + ".warm." + here[c].label));
        for (int r = 0; r < rounds; r++) {
            std::printf("  round %d:", r + 1);
            for (std::size_t k = 0; k < here.size(); k++) {
                const std::size_t c = (r + k) % here.size();
                const double t = seconds_for(command_for(here[c], name, "out/" + name + "." + here[c].label));
                times[c].push_back(t);
                std::printf("  [%s] %.3f s", here[c].label.c_str(), t);
            }
            std::printf("\n");
        }
        for (std::size_t c = 0; c < here.size(); c++) {
            const auto [low, high] = std::minmax_element(times[c].begin(), times[c].end());
            std::printf("  [%s] best %.3f  median %.3f  worst %.3f s", here[c].label.c_str(), *low, median(times[c]),
                        *high);
            if (here[c].kind == kind_cpp) std::printf("   with its compile: %.3f s", *low + compile_seconds);
            std::printf("\n");
        }
        // EVERY CONTESTANT'S ANSWER AGAINST THE FIRST satl'S.
        const std::string first = answer_in("out/" + name + "." + here[0].label, here[0].kind);
        std::string differ;
        for (std::size_t c = 1; c < here.size(); c++)
            if (answer_in("out/" + name + "." + here[c].label, here[c].kind) != first)
                differ += " " + here[c].label;
        if (here.size() > 1)
            std::printf("  answers: %s\n", differ.empty() ? "the same from every one"
                                                          : ("DIFFERENT from [satl 1]:" + differ + " -- see out/").c_str());
        std::printf("\n");
    }
    return 0;
}
