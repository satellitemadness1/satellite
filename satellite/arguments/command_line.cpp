#include "command_line.hpp"

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"

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
           "    satl --repl                           the prompt (not built yet: M0.6)\n"
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
           "    full on stderr.\n";
}

} // namespace satellite004
