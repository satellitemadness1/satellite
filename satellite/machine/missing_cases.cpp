// THE MISSING FAMILY'S REGISTER, BEFORE ANY OF IT IS RAISED (SCRATCH.md/MISSING_SYNTAX.md MS-3,
// 2026-10-04): every code's number, name and exit, and both shapes printing the lines a program is
// missing -- through the same s_code_for(), exit_status_of(), render() and notice_text() that satl
// itself reports through. Most of these codes are raised by no line of satl yet (MS-5 to MS-17
// raise them), so this is where they are proven, as exit_status_cases proves an exit no program
// can stop on.
//
//     make build/missing_cases && build/missing_cases    (check.sh runs it, one row a code)
//
// Prints one line a check -- a code's line is "ok   S151 MISSING_CAPSULE_LINES exits 78
// (missing_capsule_lines)", or "... a notice: the run carries on" for an S03x -- and exits 0 when
// every one is what section 4 of the plan says. print_notice is asked too, writing into a
// satellite.log of the harness's own, which it removes.

#include "exit_status.hpp"
#include "s_codes.hpp"

#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <set>
#include <sstream>
#include <string>

#include <stdlib.h>
#include <unistd.h>

namespace {

int failed = 0;

void check(bool right, const std::string &what)
{
    failed += right ? 0 : 1;
    std::printf("%s %s\n", right ? "ok  " : "FAIL", what.c_str());
}

} // namespace

int main()
{
    using namespace satellite004;

    // THE RUN STOPS -- S101, S150 and S151-S156: a machine code each, and the exit it becomes.
    struct Stop {
        signed long long int code;
        const char *s_code;
        const char *name;
        int exit;
    };
    const Stop stops[] = {
        {satl_file_missing_satellite_include_satellite, "S101", "MISSING_INCLUDE", 10},
        {satl_file_missing_satellite_main, "S150", "MISSING_MAIN", 11},
        {missing_capsule_lines, "S151", "MISSING_CAPSULE_LINES", 78},
        {missing_declaration, "S152", "MISSING_DECLARATION", 79},
        {missing_spacesuit, "S153", "MISSING_SPACESUIT", 80},
        {missing_part, "S154", "MISSING_PART", 81},
        {missing_close, "S155", "MISSING_CLOSE", 82},
        {missing_argument, "S156", "MISSING_ARGUMENT", 83},
    };
    for (const Stop &stop : stops) {
        const SCode named = s_code_for(stop.code);
        const int status = exit_status_of(stop.code);
        const std::string said = std::string(named.code) + " " + named.name + " exits " + std::to_string(status) +
                                 " (" + machine_code_name(stop.code) + ")";
        check(std::string(named.code) == stop.s_code && std::string(named.name) == stop.name &&
                  status == stop.exit && named.means[0] != '\0' && stops_the_program(stop.code),
              said);
    }

    // THE RUN CARRIES ON -- S030-S034: no machine code, so no exit of their own; a notice is said and
    // the run goes on, exit 0 unless something else stops it.
    const char *const notices[][2] = {{"S030", "MISSING_PROGRAM"}, {"S031", "MISSING_RETURN"},
                                      {"S032", "MISSING_WINDOW"}, {"S033", "MISSING_OBJECT"},
                                      {"S034", "MISSING_CAPSULE"}};
    std::size_t n = 0;
    for (const SCode &notice : kMissingNotices) {
        const bool named = n < 5 && std::string(notice.code) == notices[n][0] &&
                           std::string(notice.name) == notices[n][1] && notice.means[0] != '\0';
        check(named, std::string(notice.code) + " " + notice.name + " is a notice: the run carries on");
        ++n;
    }
    check(n == 5, "five notices, S030 to S034");

    // NO NUMBER MEANS TWO THINGS. Every machine code a person can reach is asked for its S-code; the
    // MISSING family's numbers are each one code's, S102 is nobody's (moved to S150, never reused),
    // and S103 is still exit 12's until MS-8 makes it the S031 notice.
    std::set<std::string> family = {"S101", "S150", "S151", "S152", "S153", "S154", "S155", "S156"};
    std::set<std::string> seen;
    bool once = true, s102_free = true;
    for (signed long long int code = 0; code <= 255; ++code) {
        const SCode named = s_code_for(code);
        const std::string number = named.code;
        if (number == "S102")
            s102_free = false;
        if (family.count(number) != 0) {
            once = once && seen.count(number) == 0;
            seen.insert(number);
        }
    }
    for (const SCode &notice : kMissingNotices)
        for (signed long long int code = 0; code <= 255; ++code)
            once = once && std::string(s_code_for(code).code) != notice.code;
    check(once && seen == family, "each MISSING number belongs to one code, and no machine code says an S03x");
    check(s102_free, "S102 is nobody's: FILE_HAS_NO_MAIN moved to S150, and the number is not reused");
    check(std::string(s_code_for(satl_file_missing_satellite_return_satellite).code) == "S103",
          "S103 is still exit 12's, until MS-8 makes a missing return the S031 notice");
    // 77 IS NEVER GIVEN: automake's and meson's test drivers read it as SKIPPED.
    check(std::string(machine_code_name(77)) == "not_on_the_list",
          "exit 77 is no code's -- a test driver would count it as skipped, not failed");

    // THE STOP SHAPE: the lines, corrected, under the sentence -- four columns in, never wrapped, a
    // blank line kept blank -- and before where it happened.
    CriticalReport stop;
    stop.code = "S151";
    stop.name = "MISSING_CAPSULE_LINES";
    stop.description = "satl(check): in satellite.main, no capsule named greet";
    stop.directory = "prog.satl:5";
    stop.syntax = "greet()";
    stop.caret_at = 0;
    stop.corrected = {"satellite.capsule greet()", "{",
                      "    satellite.console.display(\"Hello, World! This line is longer than the eighty columns "
                      "a report keeps\")",
                      "}"};
    const std::string rendered = render(stop);
    const std::string block = std::string("satl(check): in satellite.main, no capsule named greet\n\n") +
                              kCorrectedHeading + "\n\n" +
                              "    satellite.capsule greet()\n    {\n"
                              "        satellite.console.display(\"Hello, World! This line is longer than the "
                              "eighty columns a report keeps\")\n    }\n\ndirectory: prog.satl:5\n";
    check(rendered.find(block) != std::string::npos,
          "the stop shape: the lines, corrected, under the sentence and before the directory, none wrapped");
    CriticalReport plain = stop;
    plain.corrected.clear();
    check(render(plain).find(kCorrectedHeading) == std::string::npos &&
              render(plain).find("no capsule named greet\n\ndirectory: prog.satl:5\n") != std::string::npos,
          "a report with no corrected lines is the report it always was");

    // THE QUIET SHAPE: the one line, then the heading four in and the lines eight, as the plan's
    // section 5 drew it, with a blank line in the lines kept blank.
    CriticalReport quiet;
    quiet.code = kMissingCapsule.code;
    quiet.name = kMissingCapsule.name;
    quiet.description = "open() -- my_window is declared on line 5 and never opened";
    quiet.corrected = {"satellite.variable.window my_window = satellite.window.new(\"gtkcar\", 450, 250)", "",
                       "my_window.open()"};
    const std::string wanted = std::string("[satellite] S034 MISSING_CAPSULE: open() -- my_window is declared on "
                                           "line 5 and never opened\n    ") +
                               kCorrectedHeading + "\n\n        satellite.variable.window my_window = "
                               "satellite.window.new(\"gtkcar\", 450, 250)\n\n        my_window.open()";
    check(notice_text(quiet) == wanted, "the quiet shape: the notice, then the lines, corrected, under it");
    quiet.corrected.clear();
    check(notice_text(quiet) == notice_line(quiet), "a notice with no corrected lines is one line, as it was");

    // A LINE OF THE PROGRAM'S OWN IS SHOWN, NOT WRITTEN RAW: an escape in a name the corrected lines
    // repeat must not reach the terminal of the person being told about it.
    quiet.corrected = {"\x1b[2J"};
    check(notice_text(quiet).find('\x1b') == std::string::npos && render(quiet).find('\x1b') == std::string::npos,
          "the corrected lines are shown(), so an escape in them is never written raw");
    // BUT A TAB IS KEPT A TAB: shown() would write it \x09, and a tab-indented line printed so no
    // longer runs (the fresh reader of MS-3).
    quiet.corrected = {"satellite.capsule satellite.main()", "{", "\tsatellite.return(satellite)", "}"};
    check(notice_text(quiet).find("\n        \tsatellite.return(satellite)") != std::string::npos &&
              render(quiet).find("\n    \tsatellite.return(satellite)\n") != std::string::npos &&
              notice_text(quiet).find("\\x09") == std::string::npos,
          "a tab in the corrected lines is written as a tab, so a tab-indented line still runs");

    // PRINTED, AND KEPT: print_notice puts the notice and its lines on stderr and into satellite.log as
    // ONE entry -- through a log of the harness's own, so no person's satellite.log is written.
    char log_path[] = "/tmp/missing_cases.log.XXXXXX";
    const int log_fd = ::mkstemp(log_path);
    if (log_fd >= 0)
        ::close(log_fd);
    set_log_path(log_path);
    CriticalReport said;
    said.code = kMissingCapsule.code;
    said.name = kMissingCapsule.name;
    said.description = "open() -- my_window is declared on line 5 and never opened";
    said.directory = "prog.satl:5";
    said.corrected = {"satellite.variable.window my_window = satellite.window.new(\"gtkcar\", 450, 250)", "",
                      "my_window.open()"};
    std::ostringstream screen;
    std::streambuf *const was = std::cerr.rdbuf(screen.rdbuf());
    print_notice(said);
    // TWO NOTICES OF ONE CODE AND NO PLACE, WHICH WRITE DIFFERENT LINES, ARE TWO THINGS MISSING; THE
    // SAME ONE AGAIN IS THE SAME THING, COUNTED (the tally's key holds the lines).
    CriticalReport one = said, other = said;
    one.directory.clear();
    other.directory.clear();
    other.description = "open() -- other_window is declared on line 9 and never opened";
    other.corrected = {"other_window.open()"};
    print_notice(one);
    print_notice(other);
    print_notice(other);
    std::cerr.rdbuf(was);
    std::ifstream kept(log_path);
    const std::string log((std::istreambuf_iterator<char>(kept)), std::istreambuf_iterator<char>());
    ::unlink(log_path);
    check(screen.str().find(notice_text(said) + "\n") == 0,
          "print_notice writes the notice and its lines, corrected, on the screen");
    check(log.find(notice_text(said) + "\n") != std::string::npos && log.find("\n[/entry]") != std::string::npos,
          "... and into satellite.log, the notice and its lines in one entry");
    std::size_t entries = 0;
    for (std::size_t at = log.find("[entry]"); at != std::string::npos; at = log.find("[entry]", at + 1))
        ++entries;
    check(entries == 3 && screen.str().find("other_window.open()") != std::string::npos &&
              screen.str().find(notice_text(one)) != std::string::npos,
          "two notices of one code and no place, missing different lines, are both said; the same one again "
          "is counted, not said (" + std::to_string(entries) + " entries)");

    return failed == 0 ? 0 : 1;
}
