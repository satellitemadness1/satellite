#pragma once
// satellite-004's title lines -- the version, revision and build, as the author
// keeps them in satellite_config.hpp's return_arguments_vector().
//
// (the author, 2026-09-15) arguments.version 4, arguments.revision 4, and
// arguments.build, which make raises by one on every build (build_number.py).
// They are shown by --version, by --help, and every time satl starts unless
// arguments.startup_display is false:
//
//     THE SATELLITE PROGRAMMING LANGUAGE
//     VERSION 004 REVISION 04 BUILD 0052
//     CLANG++ 24 ALMALINUX 10.2
//     ---------------------------------------------------------------
//
//     (the first line of output)
//
// Padded to at least three, two and four digits, never cut: build 12345 shows
// as 12345. VERSION is the language, REVISION is this build of it, and BUILD
// counts every build. satellite 003 revision 07 (old_versions/second_satellite/)
// is a different program with its own numbers.

#include "../arguments/arguments.hpp"

#include <string>

namespace satellite004 {

inline std::string padded(const satellite_number &number, std::string::size_type width)
{
    std::string digits = number.to_text();
    if (digits.size() < width)
        digits.insert(0, width - digits.size(), '0');
    return digits;
}

inline std::string version_line(const Arguments &arguments)
{
    return "VERSION " + padded(arguments.number("arguments.version"), 3) + " REVISION " +
           padded(arguments.number("arguments.revision"), 2) + " BUILD " +
           padded(arguments.number("arguments.build"), 4);
}

// What built this binary: the compiler from its own macros, and the build
// machine's operating system from /etc/os-release, which the Makefile passes as
// SATELLITE_BUILD_OS. "CLANG++ 24 ALMALINUX 10.2".
inline std::string compiler_line()
{
#if defined(__clang__)
    std::string compiler = "CLANG++ " + std::to_string(__clang_major__);
#elif defined(__GNUC__)
    std::string compiler = "G++ " + std::to_string(__GNUC__);
#else
    std::string compiler = "AN UNKNOWN COMPILER";
#endif
#ifdef SATELLITE_BUILD_OS
    return compiler + " " + SATELLITE_BUILD_OS;
#else
    return compiler + " (build system not recorded)";
#endif
}

// --version: the three lines.
inline std::string title_lines(const Arguments &arguments)
{
    return "THE SATELLITE PROGRAMMING LANGUAGE\n" + version_line(arguments) + "\n" + compiler_line() + "\n";
}

// Every start (unless arguments.startup_display is false) and --help: the three
// lines, a rule of 63 dashes, and an empty line before the first line of output
// (the author, 2026-09-15).
inline std::string startup_block(const Arguments &arguments)
{
    return title_lines(arguments) + std::string(63, '-') + "\n\n";
}

} // namespace satellite004
