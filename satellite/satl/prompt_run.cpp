// satellite/satl/prompt_run.cpp -- `interpret <file>` and `run <file>` (prompt_run.hpp).

#include "prompt_run.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/shown.hpp"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <cstring>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

namespace satellite004 {
namespace {

bool starts_with(const std::string &text, const char *head)
{
    return text.compare(0, std::strlen(head), head) == 0;
}

// THE WORDS AFTER THE FILE ARE THE PROGRAM'S, split the way a shell splits
// `satl <file> a b`: on spaces, with a quoted word kept whole -- 003's rule, which
// 003 learned on 2026-09-13 when `run prog.satl --small` was read as one file
// called "prog.satl --small". Answers false for a quote that is never closed.
bool split_like_a_shell(const std::string &rest, std::vector<std::string> &words)
{
    std::string word;
    bool in_word = false;
    char quote = 0;
    for (const char c : rest) {
        if (quote != 0) {
            if (c == quote) quote = 0;
            else word += c;
        } else if (c == '"' || c == '\'') {
            quote = c;
            in_word = true;
        } else if (c == ' ' || c == '\t') {
            if (in_word) words.push_back(word);
            word.clear();
            in_word = false;
        } else {
            word += c;
            in_word = true;
        }
    }
    if (in_word) words.push_back(word);
    return quote == 0;
}

// THIS SAME satl, by the path the kernel ran -- so a prompt started from
// build/satl runs files with build/satl, and an installed one with itself.
std::string this_satl()
{
    std::string path(4096, '\0');
    const ssize_t length = readlink("/proc/self/exe", path.data(), path.size());
    return length > 0 ? path.substr(0, static_cast<std::size_t>(length)) : std::string("satl");
}

} // namespace

// A PROGRAM IS RUN AS `satl --run <file> [words...]`, BY A satl OF ITS OWN, and
// that is the whole of the design. A file is not a typed line: it has its own
// satellite.main, its own arguments, its own includes and windows, and its own
// "close everything" at satellite.return (M23). The prompt is one process that
// outlives many lines, and a program run inside it would have to be taken apart
// again by hand after every run. A child satl is the one way the program runs
// EXACTLY as `satl file.satl` runs it -- same output, same refusals, same exit
// status -- from the folder the prompt is in. `--run` keeps a file whose name
// begins with - a file (command_line.cpp).
//
// CTRL-C BELONGS TO THE PROGRAM WHILE IT RUNS. The terminal sends SIGINT to the
// whole foreground group, so the prompt ignores it until the program ends: its
// own handler counts presses and exits on the second, which would take the
// prompt away with the program. The child puts the default back before it
// starts, because an ignored signal stays ignored across exec.
bool run_a_file_from_the_prompt(const std::string &typed, signed long long int &answer)
{
    // THE WORD ALONE IS THE COMMAND WITH NOTHING AFTER IT, and says so -- read as a
    // statement it would be "interpret has no satellite.variable line", which is
    // true and sends a person the wrong way.
    std::string rest;
    if (starts_with(typed, "interpret ") || typed == "interpret")
        rest = typed.substr(std::min<std::size_t>(typed.size(), 10));
    else if (starts_with(typed, "run ") || typed == "run")
        rest = typed.substr(std::min<std::size_t>(typed.size(), 4));
    else
        return false;

    std::vector<std::string> words;
    if (!split_like_a_shell(rest, words)) {
        std::cerr << "satl(prompt): a quote on the `" << typed.substr(0, typed.find(' '))
                  << "` line is never closed\n";
        answer = satl_line_not_understood;
        return true;
    }
    if (words.empty()) {
        std::cerr << "satl(prompt): `" << typed.substr(0, typed.find(' ')) << "` needs a file after it, like "
                  << typed.substr(0, typed.find(' ')) << " examples/hello_world.satl\n";
        answer = satl_line_not_understood;
        return true;
    }

    const std::string satl = this_satl();
    std::vector<std::string> argv_text{satl, "--run"};
    argv_text.insert(argv_text.end(), words.begin(), words.end());
    std::vector<char *> argv;
    for (std::string &word : argv_text) argv.push_back(word.data());
    argv.push_back(nullptr);

    std::cout.flush();
    std::cerr.flush();
    struct sigaction ignore{}, before{};
    ignore.sa_handler = SIG_IGN;
    sigemptyset(&ignore.sa_mask);
    sigaction(SIGINT, &ignore, &before);

    const pid_t child = fork();
    if (child == 0) {
        signal(SIGINT, SIG_DFL);
        execv(satl.c_str(), argv.data());
        std::cerr << "satl(prompt): " << shown(satl) << " could not be started: " << std::strerror(errno) << "\n";
        _exit(127);
    }
    int status = 0;
    if (child < 0) {
        std::cerr << "satl(prompt): " << shown(words.front()) << " could not be run: " << std::strerror(errno)
                  << "\n";
        status = -1;
    } else {
        while (waitpid(child, &status, 0) < 0 && errno == EINTR) {}
    }
    sigaction(SIGINT, &before, nullptr);

    if (status < 0)
        answer = satl_line_not_understood;
    else if (WIFEXITED(status))
        answer = WEXITSTATUS(status);
    else
        answer = 128 + WTERMSIG(status);   // the status a shell gives a program a signal ended
    return true;
}

} // namespace satellite004
