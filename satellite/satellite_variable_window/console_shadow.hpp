#pragma once
// satellite/satellite_variable_window/console_shadow.hpp -- THE SHADOW UNDER A
// CONSOLE'S TEXT (the author, 2026-10-04 and 2026-10-05), the ground it falls on,
// and the console's two colours. console_shadow.cpp says how, and why the ground
// has to be there.
//
// GTK AND VTE ONLY, on purpose: a harness can compile console_shadow.cpp beside
// them and draw a console's text with the very code satl draws it with.

#include <gtk/gtk.h>
#include <vte/vte.h>

namespace satellite004 {

// THE CONSOLE'S ROWS OF ~/.satl/config.ini, and the author's colours when a row is
// absent: "the text is still 000000", on "our light blue color console background".
inline constexpr const char *kConsoleBackgroundRow = "console.background";
inline constexpr const char *kConsoleTextRow = "console.text";
inline constexpr const char *kConsoleShadowRow = "console.shadow";
inline constexpr const char *kAuthorsBackground = "#90D5FF";
inline constexpr const char *kAuthorsText = "#000000";

// A CONSOLE COLOUR: what `row` says, or `authors` when it is absent or GTK cannot
// read it.
GdkRGBA a_console_colour(const char *row, const char *authors);

// WHETHER console.shadow LEAVES THE SHADOW ON: on unless it says none, false or off.
bool the_console_shadow_is_on();

// WHAT GOES INTO A CONSOLE'S COLUMN IN PLACE OF ITS TERMINAL: a ground holding the
// terminal, painting the terminal's background whatever changes it, with the
// shadow under its text when console.shadow has it on. ON THE DESK.
GtkWidget *a_ground_under(GtkWidget *terminal);

// THE SHADOW ON OR OFF ON A CONSOLE ALREADY UP -- File > Settings…'s switch.
// Nothing, for a terminal with no ground. ON THE DESK.
void the_ground_casts_shadows(GtkWidget *terminal, bool on);

} // namespace satellite004
