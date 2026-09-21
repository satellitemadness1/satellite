#pragma once
// satellite/bytecode/window_calls.hpp -- satellite.window's words and a window's
// own methods, run by the interpreter. SATELLITE_WINDOW.md WIN-3.
//
// THESE WORDS HAVE NO LIBRARY, for file_calls.hpp's reason, and this is the
// third word family to land on it -- which is why it is now a rule rather than
// a departure. `satellite.window.new(...)` answers a HANDLE, and a library's
// scenarios only consume values and answer machine codes (number_row.hpp). A
// handle made inside a dlopened `.so` would carry that library's copy of
// satellite_window's code -- and, here, its own GTK -- into the interpreter's
// object model.
//
// SO WIN-6 IS ANSWERED FOR THE WORDS, and the answer was not a preference:
// (i) link the widget words into satl. (ii) `--export-dynamic` with the words
// still dlopened and (iii) one GUI `.so` with GTK inside it both require the
// handle to cross a library boundary, which is the thing file_calls.hpp already
// ruled out for a file. A widget's DRAWING could still live in a `.so` one day;
// the word that answers a handle cannot.
//
// THIS IS THE ONE FILE THAT KNOWS WHETHER THIS satl HAS A WINDOW.
// SATELLITE_HAS_WINDOW is 1 when pkg-config found gtk4 at build time
// (make_support/047-window.mk) and 0 otherwise, and satl builds either way --
// 047-window.mk's rule, older than this file: "a Makefile that dies there has
// made the interpreter unbuildable to deliver a window".

#include "expression.hpp"

#include <functional>
#include <string>
#include <vector>

namespace satellite004 {

// satellite.window 1 27, new(title, width, height) 1 27 1, button(text) 1 27 2.
bool is_window_word(token::Code code);

// How many arguments a window word takes, and the sentence that says so. The
// CHECKER reads these too, so a wrong count is refused before anything runs.
std::size_t window_word_arity(token::Code code);
std::string window_word_takes(token::Code code);

// How many arguments a window METHOD takes, or -1 for one a window does not
// have. `.append` takes three -- the piece and where its centre goes.
int window_method_arity(token::Code method);

// TRUE FOR A METHOD WHOSE ARGUMENT IS A CAPSULE'S NAME, READ AS WRITTEN --
// `my_button.pressed(when_pressed)` (WIN-11). ASKED, NEVER COPIED: the reader
// (expression.cpp) and the checker (program_check.cpp) both ask this rather than
// each holding a list of which methods are special, which is how the container
// method list in program_check.cpp went stale the afternoon it was written.
//
// WHY THERE IS SUCH A THING AT ALL. A capsule is arm 5 of the object model and
// nothing in the language makes one yet, so there is no expression that answers
// a capsule -- `when_pressed` worked out as a value is "a name with no
// satellite.variable line declaring it". The name is what is wanted, and the
// checker proves it names a real capsule before the program runs.
bool window_method_takes_a_capsule_name(token::Code method);

// One of those words, its arguments already evaluated.
Value call_window_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

// `window.method(arguments)`. `had_parentheses` is false for `w.title`, which
// reads the title back; `w.title("x")` writes it.
Value call_window_method(token::Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                         bool had_parentheses, const std::string &name, ExpressionContext &context);

// THE RUN DOES NOT END WHILE A WINDOW IS OPEN -- called from main() once the
// program has returned. Answers at once when this satl has no window built in
// or no window was ever opened, so nothing else pays for it.
void windows_hold_the_run_open(bool the_program_finished);

// THE WINDOW'S OWN RUN (WIN-11): every press, one at a time, in the order they
// were made, on THIS thread -- until the last window is closed and nothing is
// waiting. Answers success, or the machine code a capsule stopped on.
//
// CALLED FROM run_satl AND NOT FROM main(), unlike the wait above, and for a
// reason that is not a preference: the registry, the capsules and the walker's
// state are run_satl's own locals and are gone by the time main() sees anything.
//
// IT TAKES A WAY TO RUN A CAPSULE RATHER THAN THE WALKER ITSELF. Handed a
// std::function, this file needs no program_walk.hpp -- which would otherwise
// put the whole walker in front of expression.cpp and program_check.cpp, both of
// which include this header for three integers about arity.
//
// ANSWERS success AT ONCE when this satl has no window built in, or no window
// was ever opened, so a program that draws nothing pays a function call.
// `run_a_capsule` IS HANDED THE CAPSULE'S NAME, THE PIECE THAT WAS PRESSED AND
// THE WINDOW IT WAS PRESSED IN. What it does with them is the caller's: a
// capsule that declared no parameter is given nothing, one that declared one is
// given the piece, and one that declared two is given the piece and the window
// (SATELLITE_WINDOW.md Part 2b).
signed long long int windows_run_until_they_are_closed(
    const std::function<signed long long int(const std::string &, const WindowHandle &,
                                             const WindowHandle &)> &run_a_capsule);

} // namespace satellite004
