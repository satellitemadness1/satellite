#pragma once
// satellite/machine/ask_at_the_console.hpp -- a question asked on the terminal a line runs at: a password, read
// with nothing drawn, and a yes or no (FILE_AND_DRIVE_PLAN.md rules 4 and 6, the author's of 2026-10-21).
//
// STRAIGHT FROM THE TERMINAL, NEVER THROUGH THE PROMPT'S READER: the reader keeps every line it answers in the
// console's .c4 file (KEY_PLAN.md KEY-3), and a password must never be kept anywhere. So these read standard
// input themselves, while the terminal is cooked -- as it is while a typed line runs (prompt/line_reader.hpp) --
// with ECHO off for a password and ICANON left on, so Backspace still works as the line is typed.
//
// ONLY AT A TERMINAL: with no terminal on standard input -- a pipe, a file, /dev/null -- nobody is there to
// answer, so nothing is asked and the answer is no. A Ctrl-C while one waits answers no as well -- in a file
// run, where Ctrl-C ends satl, it still does, after the terminal is put back -- and the terminal is put back as
// it was on every way out.

#include <string>

namespace satellite004 {

// Whether there is a terminal on standard input to ask at.
bool a_console_to_ask_at();

// Writes `question` and reads one line with nothing drawn. False with no terminal, on Ctrl-C, or at the end of
// the input; `answer` holds the line without its newline. The caller wipes it (explicit_bzero) when done.
bool hidden_line(const std::string &question, std::string &answer);

// Writes `question` and " [y/N] " and answers true for y or yes alone, in either case.
bool yes_or_no(const std::string &question);

} // namespace satellite004
