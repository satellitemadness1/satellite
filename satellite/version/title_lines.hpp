#pragma once
// The title lines, from nothing but the author's rows -- for satl-term, which
// links the window and nothing of the runtime (PLAN M0.5: "satl-term ... reads
// 004's version.hpp").
//
//     THE SATELLITE PROGRAMMING LANGUAGE
//     VERSION 004 REVISION 04 BUILD 0100
//     CLANG++ 24 ALMALINUX 10.2
//
// version.hpp builds the same lines for satl from the rows as satellite_numbers;
// both end in title_lines_of() below, so satl and satl-term cannot disagree
// about how a version is written. What they can disagree about is the NUMBER, and
// only when one was built before the other: each carries the rows it was
// compiled with, which is what a build number is for.

#include "../config/satellite_config.hpp"

#include <string>

namespace satellite004 {

// "4" -> "004" at width 3. Padded, never cut: build 12345 shows as 12345.
inline std::string padded_digits(const std::string &digits, std::string::size_type width)
{
    return digits.size() < width ? std::string(width - digits.size(), '0') + digits : digits;
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

inline std::string version_line_of(const std::string &version, const std::string &revision, const std::string &build)
{
    return "VERSION " + padded_digits(version, 3) + " REVISION " + padded_digits(revision, 2) + " BUILD " +
           padded_digits(build, 4);
}

inline std::string title_lines_of(const std::string &version_line)
{
    return "THE SATELLITE PROGRAMMING LANGUAGE\n" + version_line + "\n" + compiler_line() + "\n";
}

// A row's number as satellite_number writes it: "0051" in quotes reads as 51.
// "?" for a row that is not there, which satl itself refuses to start without.
inline std::string config_row_digits(const char *name)
{
    for (const satellite_argument_row &row : return_arguments_vector()) {
        if (row.name != name || row.is_flag)
            continue;
        std::string digits = row.number.digits;
        const std::string::size_type sign = !digits.empty() && digits[0] == '-' ? 1 : 0;
        const std::string::size_type first = digits.find_first_not_of('0', sign);
        digits.erase(sign, (first == std::string::npos ? digits.size() - 1 : first) - sign);
        return digits.empty() ? "?" : digits;
    }
    return "?";
}

inline std::string title_lines_from_config()
{
    return title_lines_of(version_line_of(config_row_digits("arguments.version"),
                                          config_row_digits("arguments.revision"),
                                          config_row_digits("arguments.build")));
}

// WHAT satl IS MADE OF, LEGALLY, IN FOUR LINES.
//
// satellite is MIT and that is the licence for the language. It is NOT the only
// licence in the binary: `make GTK=vendor` compiles twenty-four other projects IN,
// and several of them are copyleft -- GTK and pango under the GNU Library GPL, glib,
// gdk-pixbuf and cairo under the LGPL. Static linking is what gives those teeth
// (LGPL-2.1 section 6), so saying so is an obligation and not a courtesy.
//
// IT IS NOT IN THE STARTUP BANNER, on purpose. title_lines() prints on every run of
// every program; a licence notice there is noise that gets scrolled past, and a
// notice nobody reads satisfies nothing. --version and --help are where somebody is
// actually asking what this is.
//
// The full texts are one file per project under licenses/, copied VERBATIM out of
// each vendored tree -- a licence text that has been reformatted is no longer the
// licence. licenses/README.md is the index and records the two elections made where
// upstream offered a choice.
inline std::string licence_lines()
{
    return "\n"
           "satellite is MIT. This binary also carries 24 other projects, each under\n"
           "its own licence -- GTK and pango under the GNU Library GPL, glib, gdk-pixbuf\n"
           "and cairo under the LGPL, FreeType under the FreeType Licence, and others.\n"
           "Full texts: licenses/ in the satellite distribution, one folder per project.\n"
           "\n"
           // NOT A COURTESY -- THE ONE MANDATORY CREDIT SENTENCE IN THE WHOLE SET.
           // docs/FTL.TXT section 3: "This credit MUST appear in the documentation
           // and/or other materials", with <year> to be replaced from the version
           // actually shipped. 2026 is the end year of freetype 2.14.3's copyright
           // line. If freetype is ever bumped, THIS YEAR MOVES WITH IT.
           "Portions of this software are copyright (c) 2026 The FreeType Project\n"
           "(https://freetype.org).  All rights reserved.\n";
}

} // namespace satellite004
