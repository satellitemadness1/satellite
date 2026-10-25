#pragma once
// satellite/bytecode/window_readers.hpp -- WHAT THE SIX WINDOW FILES SHARE: the
// readers that turn a Value into a C++ argument, the one sentence a satl built
// without a window says, and the two things the halves ask of each other.
//
// window_calls.cpp WAS 1278 LINES ON 2026-09-22, against the author's "try to
// build for 300 lines", and GTK_AND_NO_DEPENDENCIES.md GTK-0 had said on
// 2026-09-21 that it "splits the same way when GTK-2 lands, into the words and
// the methods". It did not, and fourteen milestones later it was split here,
// at the seams the recipe named and two more the file had grown:
//
//   window_calls.cpp      the WORDS: kWords, the one table, and call_window_word
//   window_shapes.cpp     what each METHOD takes -- asked by the checker before
//                         a program runs, and by nobody else
//   window_questions.cpp  a piece ASKED something: every method read bare
//   window_methods.cpp    a piece TOLD to do something: call_window_method
//   window_run.cpp        the run: events off the desk's queue into capsules
//   window_readers.cpp    these readers
//
// AND ON 2026-10-11 (NM-8) THE METHODS BECAME THEIR NUMBERS: every one a window has is a file of its own,
// satellite-numbers/words/1.6.18.<n>.cpp, holding its bare read from window_questions.cpp and its doing from
// call_window_method, moved whole; window_questions.cpp went with them, and window_methods.cpp is what runs a
// number now (run_window_number). Those files share these readers and window_method_parts below.
//
// THE #if IS IN EVERY FILE THAT DRAWS, AND EACH FILE'S TWO HALVES ANSWER THE
// SAME SHAPES. The old file kept both halves under one #if "rather than in two
// files that could drift apart"; that rule is kept per FUNCTION now -- a
// function's #else stub sits in the same file as its body, never in another.
//
// SET BY make_support/047-window.mk. 0 means pkg-config found no gtk4 when this
// satl was built, and the window sources were not compiled at all -- so every
// word here still LEXES, still CHECKS, and refuses at the moment it would draw.
// Defined HERE so that every one of the six files reads the same default.

#include "expression.hpp"
#include "window_calls.hpp"
#include "../satellite_variable_window/satellite_window.hpp"

#include <string>
#include <vector>

#ifndef SATELLITE_HAS_WINDOW
#define SATELLITE_HAS_WINDOW 0
#endif

namespace satellite004 {

// A VALUE GOING IN, AS THE C++ THE DESK WANTS. Each refuses with a sentence
// naming what was asked for and what was given. window_readers.cpp says why a
// number where text is expected is its digits, why a number where a bool is
// expected is 0 or not, and why a place may be negative but a size may not.
bool text_of(const Value &value, std::string &out, const std::string &what, ExpressionContext &context);
bool on_of(const Value &value, bool &out, const std::string &what, ExpressionContext &context);
bool size_of(const Value &value, unsigned long long int &out, const std::string &what,
             ExpressionContext &context);
bool place_of(const Value &value, long long int &out, const std::string &what,
              ExpressionContext &context, const char *units = "a number of pixels");

// A PROGRESS BAR'S PERCENTAGE, BOTH WAYS (GTK-4): the desk speaks millionths.
Value a_percentage_of(long long int millionths);
bool millionths_of(const satellite_percentage &from, long long int &out);

// A SECTION'S WIDTH OR HEIGHT (the author, 2026-10-10 and 2026-10-20): a percentage or a fraction is a share of
// what holds it, above 0 and up to all of it, and a whole number is pixels, 1 to 32767 -- SATELLITE_WINDOW_SECTIONS.md
// rule 2. extent_refused needs no run, so the checker asks it of a size written as a literal; extent_of is it at the
// run, refusing through `context`.
signed long long int extent_refused(const Value &value, AnExtent &out, const std::string &what, std::string &why);
bool extent_of(const Value &value, AnExtent &out, const std::string &what, ExpressionContext &context);

// THE WORDS THAT MAKE A PIECE, in one sentence, written out of the table in
// window_calls.cpp -- so `.append`'s refusal in window_methods.cpp names the
// widget added this morning.
std::string the_words_that_make_a_piece();

#if SATELLITE_HAS_WINDOW == 0
// THE ONE SENTENCE THIS satl SAYS WHEN IT WAS BUILT WITHOUT A WINDOW.
Value no_window_here(const std::string &what, ExpressionContext &context);
#endif

// WHAT EVERY ONE OF A WINDOW'S NUMBERS SHARES (NM-8, 2026-10-11) -- satellite-numbers/words/1.6.18.<n>.cpp. They
// were the cases of one function, call_window_method, and these are the steps that function took for every
// doing, in its order, so each number refuses as it always did (window_methods.cpp).
namespace window_method_parts {

#if SATELLITE_HAS_WINDOW
// A DOING'S THREE CHECKS, taken after its bare read has had its turn: a doing that takes nothing is written with
// its brackets, it was given a count it takes, and the window is still there. The window -- or nullptr, with the
// refusal made. `what` is the call as a refusal says it, `main_window.satellite.title`.
satellite_window *ready_to_do(token::Code method, const WindowHandle &which, const std::vector<Value> &arguments,
                              bool had_parentheses, const std::string &what, ExpressionContext &context);

// AND A DOING'S TAIL: the window, so `w.satellite.title("x").satellite.focus()` goes on -- or, when it did not
// go, the refusal by the code that says what happened: S505 when the window had gone, and a kind that does not
// meet when it is still open.
Value done(bool went, const std::string &why, const satellite_window &window, const WindowHandle &which,
           const std::string &what, ExpressionContext &context);

// A PIECE MADE, AND ANSWERED SO A CHAIN GOES ON WITH IT: .satellite.window.display.file("a_name"),
// .satellite.window.display.text.input("a_name") and .satellite.window.section("a_name", 100%, 50%) (SEC-0) -- ONE
// case of call_window_method's for the three, moved here whole for the three numbers that ran it: the name, a size
// when there is one, the refusal when the desk would not make it, and the name declared in the running capsule.
Value a_piece_made(satellite_window::Piece kind, satellite_window &window, const std::vector<Value> &arguments,
                   const std::string &what, ExpressionContext &context);
#else
// IN A satl BUILT WITHOUT A WINDOW, EVERY NUMBER'S FUNCTION IS THIS ONE. run_window_number says no window was
// built before a number could run, so it is never reached; it is in the row so the row is a method's, which no
// program calls by the word's name (function_table.cpp) -- the same in both builds.
Value no_window(const Value &receiver, Value *home, const TypeShape *shape, const std::vector<Value> &arguments,
                bool had_parentheses, const std::string &name, ExpressionContext &context);
#endif

} // namespace window_method_parts

} // namespace satellite004
