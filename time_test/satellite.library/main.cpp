// time_test/satellite.library/main.cpp -- satl raced against satl, on satellite programs.
//
// The author, 2026-09-23: "let's build a /time_test/satellite.library/main.cpp that races
// in C++, so I can just... run every test myself, and I can compile the C++ myself".
// Every timing Claude quotes about satl should be one this prints on the author's machine.
//
// BUILD (either compiler):
//     clang++ -std=c++20 -O2 main.cpp -o race
//     g++     -std=c++20 -O2 main.cpp -o race
//
// RUN, from this folder:
//     ./race                                    every program, 5 rounds, the satl on PATH
//     ./race 3 ~/.satl/satl ../../build/satl    3 rounds, two satls against each other
//     ./race 5 ~/.satl/satl ints long_add       only the programs named
//
// WHAT IT DOES, so the numbers can be trusted or doubted for the right reasons:
//   - each program runs once per satl first, untimed (the disk cache is warm after that);
//   - then ROUNDS rounds, and round r starts with a different satl, so no satl always
//     goes first;
//   - a run's time is wall clock around the whole run, start-up included, exactly as
//     haswell_test/racing.cpp measured -- satl started by the shell, stdin closed
//     (a satl waiting at its prompt would be timed waiting), SATL_NO_WINDOW=1;
//   - every run's own time is printed, then best, median and worst;
//   - the program's output (everything after satl's line of dashes) is compared between
//     the satls, so a faster satl that answers differently says so.
//
// THE PROGRAMS are in programs/. `empty` is the loop and nothing else, so the difference
// between it and `ints`, `short_float`, `long_add` or `long_multiply` is what one
// statement costs; all five run 200,000 turns.

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
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

double run_once(const std::string &satl, const std::string &program, const std::string &output)
{
    const std::string command = "SATL_NO_WINDOW=1 '" + satl + "' 'programs/" + program + ".satl' </dev/null >'" +
                                output + "' 2>&1";
    const auto started = std::chrono::steady_clock::now();
    const int status = std::system(command.c_str());
    const auto ended = std::chrono::steady_clock::now();
    if (status != 0)
        std::printf("    (%s on %s exited with status %d -- see %s)\n", satl.c_str(), program.c_str(), status,
                    output.c_str());
    return std::chrono::duration<double>(ended - started).count();
}

// WHAT THE PROGRAM SAID: every line after satl's line of dashes, which is where its own
// start-up lines end. A build number differs between satls; the answer must not.
std::string answer_in(const std::string &file)
{
    std::ifstream in(file);
    std::string line, answer;
    bool past_dashes = false;
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
    std::vector<std::string> satls, chosen;
    for (int i = 1; i < argc; i++) {
        const std::string word = argv[i];
        struct stat about {};
        if (i == 1 && word.find_first_not_of("0123456789") == std::string::npos)
            rounds = std::max(1, std::atoi(word.c_str()));
        else if (word.find('/') != std::string::npos || stat(word.c_str(), &about) == 0)
            satls.push_back(word);
        else
            chosen.push_back(word);
    }
    if (satls.empty()) satls.push_back("satl");
    std::system("mkdir -p out");

    std::printf("%d rounds; satls:", rounds);
    for (std::size_t s = 0; s < satls.size(); s++) std::printf("  [%zu] %s", s + 1, satls[s].c_str());
    std::printf("\n\n");

    for (const Program &program : kPrograms) {
        if (!chosen.empty() && std::find(chosen.begin(), chosen.end(), program.name) == chosen.end()) continue;
        std::printf("%s -- %s\n", program.name, program.what);
        std::vector<std::vector<double>> times(satls.size());
        for (std::size_t s = 0; s < satls.size(); s++)
            run_once(satls[s], program.name, "out/" + std::string(program.name) + ".warm." + std::to_string(s + 1));
        for (int r = 0; r < rounds; r++) {
            std::printf("  round %d:", r + 1);
            for (std::size_t k = 0; k < satls.size(); k++) {
                const std::size_t s = (r + k) % satls.size();
                const double t = run_once(satls[s], program.name,
                                          "out/" + std::string(program.name) + "." + std::to_string(s + 1));
                times[s].push_back(t);
                std::printf("  [%zu] %.3f s", s + 1, t);
            }
            std::printf("\n");
        }
        for (std::size_t s = 0; s < satls.size(); s++) {
            const auto [low, high] = std::minmax_element(times[s].begin(), times[s].end());
            std::printf("  [%zu] best %.3f  median %.3f  worst %.3f s\n", s + 1, *low, median(times[s]), *high);
        }
        if (satls.size() > 1) {
            const std::string first = answer_in("out/" + std::string(program.name) + ".1");
            bool same = true;
            for (std::size_t s = 1; s < satls.size(); s++)
                same = same && answer_in("out/" + std::string(program.name) + "." + std::to_string(s + 1)) == first;
            std::printf("  answers: %s\n", same ? "the same from every satl" : "DIFFERENT -- compare the files in out/");
        }
        std::printf("\n");
    }
    return 0;
}
