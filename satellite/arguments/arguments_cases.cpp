// Every name satl fills in is refused as a config row, whatever this run adds
// (review of M0.5: arguments.argument_3 was a config value on a run given two
// words and a refusal on a run given three).
//
//     make build/arguments_cases && build/arguments_cases    (check.sh runs it)
//
// Two halves: filled_in_by_satl() answers the names it must and no others, and a
// REAL gather -- the config, then a command line of three words -- adds no name
// that is neither a config row nor on that list, so the list cannot fall behind
// what gather() does.

#include "arguments.hpp"

#include "../config/satellite_config.hpp"
#include "../machine/machine_codes.hpp"

#include <cstdio>
#include <set>
#include <string>

int main()
{
    using namespace satellite004;
    int failed = 0;
    const auto check = [&](bool right, const std::string &what) {
        failed += right ? 0 : 1;
        std::printf("%s %s\n", right ? "ok  " : "FAIL", what.c_str());
    };

    for (const char *name : {"arguments.argument_1", "arguments.argument_3", "arguments.argument_100000",
                             "arguments.program", "arguments.file", "arguments.length", "arguments.debug_mode",
                             "arguments.session.directory", "arguments.system.hostname", "arguments.disk.free"})
        check(filled_in_by_satl(name), std::string(name) + " is satl's");
    for (const char *name : {"arguments.argument_", "arguments.argument_x", "arguments.argument_1x",
                             "arguments.argument", "arguments.magic", "arguments.build", "arguments.session"})
        check(!filled_in_by_satl(name), std::string(name) + " may be a row");

    Arguments arguments;
    check(arguments.gather_config() == success, "the author's satellite_config.hpp reads");
    std::set<std::string> rows;
    for (const satellite_argument_row &row : return_arguments_vector())
        rows.insert(row.name);
    CommandLine command_line;
    command_line.command = Command::run;
    command_line.file = "program.satl";
    command_line.words = {"one", "two", "three"};
    check(arguments.gather(command_line) == success, "a gather of three words");
    for (const Argument &argument : arguments.all())
        if (rows.count(argument.name) == 0 && argument.name != "arguments.startup_display" &&
            argument.name != "arguments.infinity" && argument.name != "arguments.infinity_display" &&
            argument.name != "arguments.infinity.counter" &&
            argument.name != "arguments.float.whole" && argument.name != "arguments.float.decimal")
            check(filled_in_by_satl(argument.name), "gather's " + argument.name + " is on filled_in_by_satl's list");
    return failed == 0 ? 0 : 1;
}
