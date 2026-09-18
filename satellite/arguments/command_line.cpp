#include "command_line.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"

#include <cstdlib>
#include <cstring>

namespace satellite004 {

namespace {

signed long long int refuse(const std::string &why)
{
    return report_error("satl(command line): " + why + " -- satl --help lists every way to start satl",
                        command_line_not_understood);
}

bool alone(const std::string &word)
{
    return word == "--version" || word == "-V" || word == "--help" || word == "-h";
}

} // namespace

signed long long int read_command_line(int argc, char **argv, CommandLine &into)
{
    into = CommandLine{};
    int i = 1;
    while (i < argc && std::strcmp(argv[i], "--debug") == 0) {
        into.debug = true;
        ++i;
    }
    if (i >= argc)
        return success;   // `satl`, or `satl --debug`: the opening lines

    const std::string word = argv[i];
    if (alone(word)) {
        if (i > 1)
            return refuse(word + " is the whole command line, and --debug came before it");
        if (i + 1 < argc)
            return refuse(word + " takes no other words, and \"" + argv[i + 1] + "\" was given");
        into.command = word == "--version" || word == "-V" ? Command::version : Command::help;
        return success;
    }
    if (word == "--rebuild") {
        // THE WHOLE COMMAND LINE, like --version, and for a plainer reason:
        // it takes nothing, it runs once, and a word after it is a word somebody
        // meant for something else.
        if (i + 1 < argc)
            return refuse("--rebuild takes no other words, and \"" + std::string(argv[i + 1]) + "\" was given");
        into.command = Command::rebuild;
        return success;
    }
    if (word == "--config") {
        // ONE OPTIONAL WORD, AND THAT IS THE ONLY COMMAND HERE THAT TAKES ONE.
        // `satl --config` probes what the machine allows less headroom; `satl
        // --config 8192` probes exactly that many. The cap is there because the
        // uncapped run is nine seconds and three gigabytes on this machine, and
        // a command nobody can afford to try once is a command nobody tries.
        if (i + 2 < argc)
            return refuse("--config takes at most one number, and \"" + std::string(argv[i + 2]) + "\" came after it");
        into.command = Command::config;
        if (i + 1 < argc) {
            const std::string most = argv[i + 1];
            if (most.empty() || most.find_first_not_of("0123456789") != std::string::npos)
                return refuse("--config takes a count of threads, and \"" + most + "\" is not one");
            into.most = std::strtoull(most.c_str(), nullptr, 10);
            if (into.most == 0)
                return refuse("--config 0 probes nothing -- leave the number off to probe what this machine allows");
        }
        return success;
    }
    if (word == "--feedback") {
        if (i + 1 < argc)
            return refuse("--feedback takes no other words, and \"" + std::string(argv[i + 1]) + "\" was given");
        into.command = Command::feedback;
        return success;
    }
    if (word == "--repl") {
        if (i + 1 < argc)
            return refuse("--repl takes no other words, and \"" + std::string(argv[i + 1]) + "\" was given");
        into.command = Command::repl;
        return success;
    }

    int file_at = i;
    if (word == "--run") {
        if (i + 1 >= argc)
            return refuse("--run needs the file to run after it");
        file_at = i + 1;
    } else if (!word.empty() && word[0] == '-') {
        // NOT TAKEN AS A FILE NAME. A misspelled --rum that ran a file called
        // --rum would complain about a file nobody meant.
        return refuse("\"" + word + "\" is not a word satl takes (a file whose name begins with - runs as satl --run " +
                      word + ")");
    }

    into.command = Command::run;
    into.file = argv[file_at];
    into.words.assign(argv + file_at + 1, argv + argc);
    return success;
}

std::string opening_lines()
{
    return "    satl <file.satl> [words...]           run a program\n"
           "    satl --help                           every way to start satl\n"
           "    satl --version                        the version, revision and build\n";
}

std::string usage_lines()
{
    return "    satl                                  the opening lines\n"
           "    satl <file.satl> [words...]           run a program\n"
           "    satl --run <file> [words...]          the same; the file may begin with -\n"
           "    satl --debug <file.satl> [words...]   run it, showing every state and argument\n"
           "    satl --repl                           the prompt: type a line, see it run\n"
           "    satl --rebuild                        compose every setting into one binary\n"
           "    satl --config [most]                  measure what this machine can do, once\n"
           "    satl --feedback                       show what satellite.feedback has kept here\n"
           "    satl --version, -V                    the version, revision and build\n"
           "    satl --help, -h                       this\n"
           "\n"
           "    --debug comes before the file, --run or --repl. --version and --help\n"
           "    are the whole command line. Every word after the file is the\n"
           "    program's, --version and --debug included: arguments.program,\n"
           "    arguments.argument_1 ... and arguments.length.\n"
           "\n"
           "    satl exits with the machine code the program stopped on, 0 when it ran\n"
           "    to the end. A code below 0 or above 254 exits 255 and is written in\n"
           "    full on stderr.\n"
           "\n"
           "  YOUR FIRST PROGRAM -- save it as hello.satl and run satl hello.satl\n"
           "\n"
           "    satellite.include(satellite)\n"
           "\n"
           "    satellite.capsule satellite.main()\n"
           "    {\n"
           "        satellite.console.display(\"hello\")\n"
           "    }\n"
           "    satellite.return(satellite)\n"
           "\n"
           "    Every program has those three: the include, satellite.main, and the\n"
           "    return. Leave one out and satl says which one and what to type.\n"
           "\n"
           "  WHEN SOMETHING GOES WRONG satl prints a report with an S-code, the line\n"
           "  it happened on and a caret under it. The number says how bad it is:\n"
           "  S0xx is a warning and the run carries on, and it climbs from there to\n"
           "  S999, which is the machine refusing satl any memory at all.\n";
}

} // namespace satellite004
