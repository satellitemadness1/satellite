#pragma once

// The harness, and the two sections. Three functions and a counter, no
// framework -- the shape every suite in this tree uses.
//
// WHAT IS BEING PROVED. PLAN M10 and DESIGN §10.1 make four claims about the
// console, and each is invisible in the others:
//
//   1. A LINE IS ATOMIC. "The unit queued is a whole string", so threads
//      printing at once interleave lines and never characters. Nothing about
//      the API says this; only two threads and a reader can tell.
//   2. NOTHING IS LOST. Everything queued before the shutdown is written, in
//      order, and the printer thread is JOINED rather than detached -- which is
//      the same argument satc_test/writing.cpp makes about the `.satc` writer.
//   3. A DRAIN MEANS WRITTEN AND FLUSHED. "There is a `drain()` barrier before
//      reading input, so a prompt cannot appear before the output that explains
//      it" -- and a prompt is a write with no newline, which line buffering
//      would hold back on a tty as well.
//   4. `satellite.console.display` `1 5 1` IS IN handlers[path_id]. The first
//      real row the table has ever held, reached through a compiled program.
//
// IT REDIRECTS ITS OWN STDOUT AND THAT IS THE ONLY HONEST WAY. The claims are
// about bytes reaching a file descriptor, so a fake sink would be a test of the
// queue with the half that can fail removed -- glibc's buffering IS the thing
// DESIGN §10.1's flush exists for. capture() below dup2s a temporary over fd 1,
// which is what satc_test/writing.cpp's "it touches a disk, once, in /tmp" is
// for the file writer one module over.
//
// AND IT LINKS machine_limits, WHICH eval_test DELIBERATELY DOES NOT. That
// suite's omission is about a raised RLIMIT_STACK its depth fixtures must not
// get for free (M8.5 §4.1); nothing here is about depth, and the console's own
// module links nothing but the standard library. What this binary needs beyond
// it is the evaluator, so that the row can be reached the way a program reaches
// it rather than by calling the handler directly.

#include <functional>
#include <string>

namespace console_test {

extern int failures;

void check(bool ok, const std::string &what);

// Everything written to stdout while `body` runs, with the console shut down
// before the descriptor goes back.
std::string capture(const std::function<void()> &body);

// Whether `text` holds `needle`.
bool holds(const std::string &text, const std::string &needle);

// How many lines are in `text`, and whether every one of them is `line`.
bool every_line_is(const std::string &text, const std::string &line,
                   size_t *count);

void section_printing();    // atomicity, order, the barrier, the shutdown
void section_dispatching(); // `1 5 1` through a compiled program

} // namespace console_test
