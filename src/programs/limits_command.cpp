// Starting the machine limits from the command line. See
// programs/limits_command.hpp.

#include "programs/limits_command.hpp"

#include "machine_limits/limits.hpp"

#include <string>
#include <vector>

namespace satellite {

namespace {

// The config file named on the command line, or empty for the one satl finds.
//
// argv[1] AND NOT A SCAN OF THE WHOLE LINE. Every flag in this program takes
// its operand immediately after itself and none of them combine, so a scan
// would find `--limits` inside `satl --check --limits.satl` and read a program
// as a config. main.cpp's own arms index args[2] for the same reason.
std::string named_config(const std::vector<std::string> &args)
{
    if (args.size() < 3)
        return std::string();
    if (args[1] == "--limits" || args[1] == "--watchdog")
        return args[2];
    return std::string();
}

} // namespace

int start_limits(const std::vector<std::string> &args)
{
    return limits::begin(named_config(args));
}

} // namespace satellite
