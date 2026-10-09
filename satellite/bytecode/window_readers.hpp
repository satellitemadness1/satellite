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

// THE WORDS THAT MAKE A PIECE, in one sentence, written out of the table in
// window_calls.cpp -- so `.append`'s refusal in window_methods.cpp names the
// widget added this morning.
std::string the_words_that_make_a_piece();

#if SATELLITE_HAS_WINDOW == 0
// THE ONE SENTENCE THIS satl SAYS WHEN IT WAS BUILT WITHOUT A WINDOW.
Value no_window_here(const std::string &what, ExpressionContext &context);
#else
// A METHOD READ BARE, OR A QUESTION READ EITHER WAY, ANSWERED (window_questions.cpp).
// True when it was one of those and `answered` holds the answer -- or the
// refusal, in which case context.code says so; false when the method is a doing
// and window_methods.cpp carries on.
bool answer_a_question(token::Code method, const WindowHandle &which, bool had_parentheses,
                       const std::string &name, ExpressionContext &context, Value &answered);
#endif

} // namespace satellite004
