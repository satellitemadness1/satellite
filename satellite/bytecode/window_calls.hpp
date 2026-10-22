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
// THE FILES BEHIND THIS HEADER ARE THE ONES THAT KNOW WHETHER THIS satl HAS A
// WINDOW -- six since the 2026-09-22 split, and window_readers.hpp is where
// the default is set so all six read the same one.
// SATELLITE_HAS_WINDOW is 1 when pkg-config found gtk4 at build time
// (make_support/047-window.mk) and 0 otherwise, and satl builds either way --
// 047-window.mk's rule, older than this file: "a Makefile that dies there has
// made the interpreter unbuildable to deliver a window".

#include "expression.hpp"

#include <functional>
#include <string>
#include <vector>

namespace satellite004 {

// satellite.window 1 27; new(title, width, height) 1 27 1; button(text) 1 27 2;
// label(text) 1 27 3. The .cpp holds the one table every one of those is read
// from (GTK_AND_NO_DEPENDENCIES.md GTK-0), so a widget added there is a widget
// here without a second list to keep true.
bool is_window_word(token::Code code);

// How many arguments a window word takes, and the sentence that says so. The
// CHECKER reads these too, so a wrong count is refused before anything runs.
std::size_t window_word_arity(token::Code code);
std::string window_word_takes(token::Code code);

// How many arguments a window METHOD takes, or -1 for one a window does not
// have. `.append` takes three -- the piece and where its centre goes.
int window_method_arity(token::Code method);

// AND A SECOND COUNT THE SAME METHOD ACCEPTS, or -1 for one that takes exactly
// what window_method_arity says. `.append` was the first: a window and a grid
// take the piece and where it goes, a row and a column take just the piece, and
// which is right is the RECEIVER's -- which the checker cannot know, because a
// satellite.variable.window name may hold either. `.listen` takes a capsule or
// nothing, and `.widgets.display.file` and `.widgets.display.input` a name, or a
// name and a size (2026-10-10).
int window_method_also_takes(token::Code method);

// HOW MANY IT TAKES, AS A REFUSAL SAYS IT: "1 argument", "4 arguments", and both
// counts for a method with two -- "1 or 3 arguments".
std::string window_method_takes(token::Code method);

// AND WHAT TO SAY WHEN IT IS -1: the methods a window and the pieces in one do
// have, in one sentence. The CHECKER prints this, and it is asked for rather
// than copied for the reason the paragraph below gives about capsule names --
// program_check.cpp's own hand-written list of container methods went stale the
// same afternoon it was written, and this is that lesson applied before it can
// happen again.
std::string window_methods_are();

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

// AND WHETHER ANYTHING MAY FOLLOW THAT NAME. False for `.pressed`, `.changed`
// and `.closed`, which take one capsule's name and nothing else; TRUE for
// `.every`, which takes the name and then how often. The checker asks this
// rather than carrying its own list, for the reason above.
bool window_method_takes_more_after_the_name(token::Code method);

// THE AUTHOR'S WORDS OF PARTS (his final window syntax, 2026-10-10 ~21:15), each ONE word with `.satellite.`
// once before it: main_window.files_display.satellite.widgets.display.file("list_of_files", 100%, 100%), and
// w.satellite.scroll.vertical(1). The lexer reads each as one code after `.satellite.`; written any other way --
// bare, `.satellite.` again inside it, the family's old name displays, begun and not finished -- it arrives as its
// parts. True when the method after the period at `dot` is such a part (window_shapes.cpp says which are). Asked
// by the walker and the checker of a window, before the period's own rule, so the sentence names the whole word.
bool a_word_in_parts(const std::vector<std::bitset<16>> &row, std::size_t dot);

// AND THE SENTENCE: `receiver` is the call as written up to the period -- main_window, main_window.files_display --
// and the sentence says how the whole word is written: "main_window.widgets.display.file is written
// main_window.satellite.widgets.display.file now -- a window's own words are under satellite".
std::string word_in_parts_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t dot,
                                        const std::string &receiver);

// `.satellite.widgets.display.file("a_name")` and `.satellite.widgets.display.input("a_name")`: the two that make a
// widget and take its name. A section (2026-10-10) is not one of the displays: `.satellite.section` is written on
// the window or the section, and program_check.cpp judges it where it knows that is what it was written on.
bool makes_a_widget(token::Code method);

// `.section("a_name", 100%, 50%)` WRITTEN ON A WINDOW OR A SECTION: three arguments, the first its name
// in quotes, a name a widget can have. `open` is the bracket. success, or the code and the sentence.
signed long long int section_written_wrong(const std::vector<std::bitset<16>> &row, std::size_t open,
                                          const std::string &spelling, std::string &why);

// WHETHER `name` CAN NAME A WIDGET (window_shapes.cpp; satellite_window.hpp says the rule).
bool a_widget_name_fits(const std::string &name, std::string &why);

// One of those words, its arguments already evaluated.
Value call_window_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

// A WINDOW'S OWN WORDS RUN BY THEIR NUMBERS (NM-8, 2026-10-11; the author: "we have to give everything a number
// and then make it run those numbers"). Every method a window and the pieces in one have is the word
// satellite.variable.window.<it>, 1 6 18 n -- a file under satellite-numbers/words/ -- and this says which method
// runs which, or 0 for one a window does not have. ONE NUMBER A METHOD: a read written bare and its write with
// brackets are one (`.title` and `.title("x")`), `.colour` and `.color` one, and a method's two right counts one.
token::Code window_number_of(token::Code method);

// `window.satellite.method(arguments)`, by its number. What a window refuses before any of its words runs --
// a program's own thread using one, a satl built without a window -- is refused here; everything else is the
// number's, which the function table runs (window_methods.cpp). `had_parentheses` is false for `w.satellite.title`,
// which reads the title back; `w.satellite.title("x")` writes it.
Value run_window_number(token::Code method, token::Code number, const Value &receiver,
                        const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                        ExpressionContext &context);

// `my_window.a_name` -- THE WIDGET MADE IN IT UNDER THAT NAME (the author, 2026-10-09), with
// `.satellite.widgets.display.file("a_name")`. `name` is the chain so far, as a refusal says it.
Value a_widget_by_name(const WindowHandle &which, const std::string &member, const std::string &name,
                       ExpressionContext &context);

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

// THE CONSOLE satl LAUNCHES FOR ITSELF (GTK-17): `satl --console [file]`.
// Called by run_satl once the command line is read and BEFORE a line is
// printed; answers success with satl's own stdin, stdout and stderr on the
// console's pty -- in this process, so the code the run stops on is still the
// exit status -- or the code it has already reported: no_display, or
// not_built_yet for a satl made without a console. `holds_after_a_clean_run`
// is true for the prompt, whose screen is a person's to read when it ends.
//
// A CONSOLE IT CANNOT HAVE IS A REFUSAL, whoever asked -- satl for itself as
// much as `--console`. Until 2026-10-05 satl asking for itself carried on
// where it was pointed when the console failed; the author dropped machines
// with no monitor that day, and the silent path with them.
signed long long int open_satls_own_console(const std::string &title, bool holds_after_a_clean_run);
bool satls_own_console_is_open();

// WHETHER satl SHOULD OPEN ITS OWN CONSOLE WITH NOBODY ASKING -- and since
// 2026-10-05 ("we are dropping the whole 'start satl from a console and it
// runs' thing") that is every run except one whose output is a pipe or a
// regular file, or one under SATL_NO_WINDOW. A controlling terminal and a
// missing display, WIN-9's other two reasons of 2026-09-22, are gone with it.
// window_run.cpp names the two. The same answer on every build.
bool nobody_gave_satl_a_console();

// A SATELLITE OF ITS OWN FOR A PROGRAM THE PROMPT STARTS (the author,
// 2026-10-20: "Just create another satellite, for each program"), STARTED ON
// THE DESK, where the console menu's File > Open… starts one
// (machine/satellite_of_its_own.hpp says what it is given). Not on the
// prompt's own thread, which console_launch.cpp sets to nice 19: a satl forked
// there starts at 19 and its window's desk with it, and no thread comes back
// from 19 -- so 24 programs would draw 24 windows at the priority of the work
// they are doing. The start calls no GTK; the desk is only where it runs.
// Asked only while satl's own console is open, so a satl with no window, which
// has no desk, starts it where it is.
bool start_a_satellite_on_the_desk(const std::vector<std::string> &words, std::string &why);

// AND WHAT BECOMES OF IT WHEN THE RUN IS OVER, called by main() before it
// waits for the windows: a run that STOPPED keeps the console up with the code
// and its name on the last line until a person presses a key; the prompt keeps
// it up the same way; a file that finished closes it at once. Then the wait in
// main() waits for it as it waits for any window.
void satls_own_console_is_done(signed long long int code);

} // namespace satellite004
