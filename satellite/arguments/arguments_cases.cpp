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
#include <utility>

int main()
{
    using namespace satellite004;
    int failed = 0;
    const auto check = [&](bool right, const std::string &what) {
        failed += right ? 0 : 1;
        std::printf("%s %s\n", right ? "ok  " : "FAIL", what.c_str());
    };

    // MS-1 (2026-10-03): argument1 is satl itself, argument2 the file, argument3 onwards the
    // words -- under four spellings, and under the old argument_N, all of them satl's.
    for (const char *name : {"arguments.argument1", "arguments.argument3", "arguments.argument100000",
                             "arguments.arg1", "arguments.args1", "arguments.arguments1", "arguments.argument03",
                             "arguments.arg0", "arguments.arguments99999999999999999999",
                             "arguments.argument_1", "arguments.argument_3", "arguments.argument_100000",
                             "arguments.program", "arguments.file", "arguments.length", "arguments.debug_mode",
                             "arguments.session.directory", "arguments.system.hostname", "arguments.disk.free",
                             "arguments.threads_startup"})
        check(filled_in_by_satl(name), std::string(name) + " is satl's");
    for (const char *name : {"arguments.argument_", "arguments.argument_x", "arguments.argument_1x",
                             "arguments.argument", "arguments.arguments", "arguments.args", "arguments.arg",
                             "arguments.arg1x", "arguments.argsx1", "arguments.argss1", "arguments.magic",
                             "arguments.build", "arguments.session", "arguments.threads_startup_per_core"})
        check(!filled_in_by_satl(name), std::string(name) + " may be a row");

    // ONE ROW UNDER EVERY SPELLING, never four -- and the old spelling answers the row it
    // means now, two further on, because argv and the file come first.
    for (const auto &[spelled, row] : {std::pair<const char *, const char *>{"arg3", "argument3"},
                                       {"args3", "argument3"}, {"argument3", "argument3"},
                                       {"arguments3", "argument3"}, {"argument03", "argument3"},
                                       {"arg0", "arg0"}, {"length", "length"}, {"arg", "arg"}})
        check(the_argument_row_spelled(spelled) == row, std::string(spelled) + " reads the row " + row);
    check(names_argument_zero("arg0") && names_argument_zero("arguments00") && !names_argument_zero("arg10"),
          "arg0 and arguments00 name argument 0, arg10 does not");
    for (const auto &[old, now] : {std::pair<const char *, const char *>{"argument_1", "argument3"},
                                   {"argument_8", "argument10"}, {"argument_99", "argument101"},
                                   {"argument_0", ""}, {"argument_x", ""}, {"argument3", ""}})
        check(the_row_argument_underscore_is_now(old) == now,
              std::string(old) + " is " + (*now ? now : "no row") + " now");

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
    // IN THE ORDER IT WAS TYPED (MS-1): satl itself -- "satl" when argv[0] said nothing -- then
    // the file, then its three words, and length counts all five.
    {
        std::string seen;
        for (int n = 1; n <= 6; ++n) {
            const Argument *row = arguments.find("arguments.argument" + std::to_string(n));
            seen += (n == 1 ? "" : "|") + (row != nullptr ? row->text : std::string("(none)"));
        }
        const Argument *length = arguments.find("arguments.length");
        check(seen == "satl|program.satl|one|two|three|(none)" && length != nullptr && length->count == 5,
              "argument1 to argument5 are satl, the file and its three words, and length is 5 (" + seen + ")");
    }
    for (const Argument &argument : arguments.all())
        if (rows.count(argument.name) == 0 && argument.name != "arguments.startup_display" &&
            argument.name != "arguments.infinity" && argument.name != "arguments.infinity_display" &&
            argument.name != "arguments.infinity.counter" &&
            argument.name != "arguments.float.whole" && argument.name != "arguments.float.decimal")
            check(filled_in_by_satl(argument.name), "gather's " + argument.name + " is on filled_in_by_satl's list");

    // THE WARM THREADS ARE THE MACHINE'S CORES TIMES THE AUTHOR'S ROW (2026-10-03: "how many
    // cores the machine has x2"), held as a NUMBER, because number() is how run_satl reads it.
    const Argument *cores = arguments.find("arguments.machine.cores");
    const satellite_number &per_core = arguments.number("arguments.threads_startup_per_core");
    check(cores != nullptr && cores->count > 0 &&
              satellite_number::compare(arguments.number("arguments.threads_startup"),
                                        satellite_number(cores->count) * per_core) == 0,
          "arguments.threads_startup is arguments.machine.cores (" + std::to_string(cores ? cores->count : 0) +
              ") x arguments.threads_startup_per_core (" + per_core.to_text() + ") = " +
              arguments.number("arguments.threads_startup").to_text());
    return failed == 0 ? 0 : 1;
}
