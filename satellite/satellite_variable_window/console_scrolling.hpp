#pragma once
// satellite/satellite_variable_window/console_scrolling.hpp -- THE TWO SCROLL ROWS, as the run
// holds them (the author, 2026-10-07): arguments.scroll.vertical and arguments.scroll.horizontal.
// Read where a console or a window is made, written where a setting is written
// (bytecode/setting_writes.cpp) and as satl starts (structured-library.cpp). No GTK header in here,
// so the interpreter's files may include it; window_console.cpp defines them.

#include <atomic>

namespace satellite004 {

std::atomic<bool> &console_scrolls_vertically();
std::atomic<bool> &console_scrolls_horizontally();

// A ROW WAS WRITTEN: satl's own console, if it is open, takes the new answer at once. A program's
// own consoles and windows keep what they were made with, or what their own .scroll line said.
void scrolling_rows_changed();

} // namespace satellite004
