#pragma once
// satellite/prompt/console_lines.hpp -- THE CONSOLE'S LINES (KEY_PLAN.md KEY-3): every line Entered at
// one console, each a satellite.variable.string of its own, oldest first. The author's third milestone
// of 2026-10-20: "when enter is pressed, create a new satellite.variable.string to save the input
// into" ... "one enter press per line".
//
// ONE LINE AN ENTER PRESS, AS ENTER SENT IT (rule 3): after Backspace, Delete and the rest have
// changed it -- `pritn`, two Backspaces and `nt` is `print` -- and an empty Enter is an empty line.
// Nothing is left out here: a line of spaces and a repeat are lines too. Stepping over them is for
// Up and Down to do (KEY-4), which go from stop to stop (stops(), below).
//
// IT KEEPS NO TERMINAL AND NO FILE, as the Editor draws nothing and reads nothing (editor.hpp): the
// reader adds to it (line_reader.cpp), and the console's store owns satl's and writes each line into
// the console's file (satl/console_files.hpp). So build/prompt_cases and build/prompt_reader hand a
// reader lines of their own, and link nothing of satl/ for it.
//
// ADDED TO UNDER A LOCK: satellite.console.input() is called from a program's threads as well as the
// main one (expression.cpp holds no console lock around it, because it waits for a person), and every
// reader that brings a line -- the session's, and a file run's own (KEY-4) -- ends here.
//
// UP AND DOWN GO THROUGH IT (KEY_PLAN.md KEY-4): the Editor is handed it, as it was handed the
// History it replaces -- a list of its own the prompt kept beside these lines, which never held an
// input() answer.

#include "../satellite_variable_string/satellite_string.hpp"

#include <cstddef>
#include <mutex>
#include <string>
#include <vector>

namespace satellite004::prompt {

// WHAT OF A LINE IS KEPT (KEY_PLAN.md rule 3): every character but the control characters -- U+0000
// to U+001F (TAB, CR and ESC among them), U+007F, and U+0080 to U+009F -- and no byte that is not part
// of a whole UTF-8 character. The rest is kept byte for byte, so a line typed in ASCII is ASCII, and
// `café` and an emoji are the bytes that were typed (the author's answer 4, "you decide", decided in
// KEY_PLAN.md Section 6).
//
// A WHOLE UTF-8 CHARACTER IS ONE OF THE UNICODE STANDARD'S TABLE 3-7, the table from_utf8 decodes by
// (satellite_variable_string/conversion_loops.hpp), so from_utf8 takes whatever this answers. Every
// other byte goes, ALONE: a stray continuation byte, a first byte with too few continuations after it,
// an overlong form (C0, C1, E0 80-9F, F0 80-8F), a surrogate (ED A0-BF), anything above U+10FFFF
// (F4 90-BF), and F5 to FF. A bad byte never takes the character after it with it -- `\xE2A` keeps
// the A, and `\xFF` before an emoji keeps the emoji -- because the next byte is read again as the
// start of a character of its own.
//
// KEY-5 WILL READ A CONSOLE'S FILE BACK THROUGH IT TOO, so a file edited by hand still cannot put an
// ESC on the screen.
std::string what_is_kept(const std::string &entered);

class ConsoleLines {
public:
    ConsoleLines() = default;
    ConsoleLines(const ConsoleLines &) = delete;
    ConsoleLines &operator=(const ConsoleLines &) = delete;

    // A LINE WAS ENTERED: what_is_kept of it, made a satellite_string with from_utf8 -- which cannot
    // fail on what is left -- and added after every line before it. Answers the line as kept, in
    // UTF-8: what the console's file is given.
    std::string add(const std::string &entered);

    std::size_t size() const;
    // THE LINE AT `index`, oldest first, as a copy taken under the lock: another thread may add while
    // it is read. An index past the end answers an empty string.
    satellite_string at(std::size_t index) const;

    // THE LINES UP AND DOWN STOP AT (KEY_PLAN.md KEY-4, rule 4), oldest first: every line, less a line
    // that is empty or holds only spaces, and less a line the same as the nearest earlier line they stop
    // at -- what History kept, with the session's trim before it, moved from keeping to browsing,
    // because every Enter is kept now. So `a`, `b`, four spaces, `b` are two stops, `b` then `a` going
    // up, and `A`, an empty line, `A` are one.
    //
    // FOUND AS EACH LINE IS ADDED, one comparison with the last stop, and kept as where each stop is in
    // the lines: Up and Down take one step a press whatever is stepped over, a run of 100,000 repeats
    // included, and the lines have no ceiling.
    std::size_t stops() const;
    // THE STOP AT `stop`, as the Editor's line holds it -- the to_utf8 of its satellite_string -- taken
    // under the lock. A stop past the end answers an empty string.
    std::string stop_at(std::size_t stop) const;

private:
    mutable std::mutex lock_;
    std::vector<satellite_string> lines_;
    std::vector<std::size_t> stops_;   // where each stop is in lines_, oldest first
};

} // namespace satellite004::prompt
