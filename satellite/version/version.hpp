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
//
// satl-term shows the same lines through title_lines.hpp, which needs no
// satellite_number (PLAN M0.5).

#include "../arguments/arguments.hpp"
#include "title_lines.hpp"

#include <string>

namespace satellite004 {

inline std::string version_line(const Arguments &arguments)
{
    return version_line_of(arguments.number("arguments.version").to_text(),
                           arguments.number("arguments.revision").to_text(),
                           arguments.number("arguments.build").to_text());
}

// --version: the three lines.
inline std::string title_lines(const Arguments &arguments)
{
    return title_lines_of(version_line(arguments));
}

// Every start (unless arguments.startup_display is false) and --help: the three
// lines, a rule of 63 dashes, and an empty line before the first line of output
// (the author, 2026-09-15).
inline std::string startup_block(const Arguments &arguments)
{
    return title_lines(arguments) + std::string(63, '-') + "\n\n";
}

} // namespace satellite004
