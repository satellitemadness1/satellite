#pragma once
// satellite/display/printing_satellite.hpp -- THE PRINTING SATELLITE AND THE DISPLAY THREAD.
// FAST_PRINTING.md step 3; the author, 2026-09-26: "satl>mini_satl_with_builtin_buffer>
// display_thread_here", then "Let's just do your 5 step thing".
//
//   the interpreter ---a display's finished value, MOVED--->  1 THE PRINTING SATELLITE
//   std::cout's 8 KB (satl's own text) --------------------->     his buffer: a count and his
//                                                                 limit; a number's digits, a
//                                                                 float's, a string's UTF-8,
//                                                                 \033 left out off a terminal
//                                                                        |  UTF-8 pieces
//                                                                        v
//                                                             2 THE DISPLAY THREAD --> fd 1
//
// THE INTERPRETER WORKS OUT THE PARENTHESES AND NOTHING AFTER. Every read, `+`, capsule call and
// refusal happens on its own thread in the program's order (expression.cpp's call_word); then the
// finished value is MOVED into a slot of the hand-off ring and the interpreter goes on to its next
// line. Turning the value into text, UTF-8, the colour codes a pipe must not get, and the newline
// are the printing satellite's.
//
// ONE DOOR, ONE ORDER. std::cout writes into the same ring (display_stream.cpp), so satl's own
// words -- a report's flush, the prompt, a styled line, satellite.access -- land between the
// program's lines in the order they were written. A flush waits until the display thread has
// written everything before it, and std::cerr flushes std::cout first, so a report still comes
// after the lines it explains.
//
// HIS BUFFER AND HIS LIMIT. The author: "if the buffer is holding 131072 ... objects then it
// crashes the interpreter", "arguments.display.buffer(131072) is the default ... it's loaded as
// an unsigned long long int", "if(buffer.size() > x) { // critical error report }", "the buffer
// has to check it's size, it's free to run that check", "keep the size of it in another
// unsigned long long int". The printing satellite keeps the displays the screen has not taken
// yet, counted beside them -- one added as it arrives, one taken off as it goes -- and past the
// limit the program is stopped with S840 and his philosophy.

#include "../bytecode/value.hpp"

#include <cstddef>
#include <string>

namespace satellite004 {

// "131072 is the default" -- arguments.display.buffer.
constexpr unsigned long long int kDisplayBufferDefault = 131072;

// SATL'S START-UP, before anything prints: the printing satellite -- "a thread off of the satl
// process" -- which starts the display thread, and std::cout pointed at them. A machine that
// will not make the threads gets the same output written on the interpreter's own thread.
void start_the_printing_satellite();

// arguments.display.buffer, read ONCE, when the arguments are gathered: "we load this value so
// we don't have to keep getting it from arguments".
void set_the_display_buffer(unsigned long long int most_waiting);
unsigned long long int the_display_buffer();

// A PLAIN display: the finished value, moved in, with a newline after it. The value is one a
// plain display can print without refusing -- call_word has already refused the rest, and made
// a container's text itself. Answers success, display_error once the screen has refused a line
// (said once, as std::cout's badbit was), or out_of_memory.
signed long long int display_value(Value &&value);

// Bytes already made -- std::cout's 8 KB -- written as they are, in turn. The string handed in
// comes back empty (with an old slot's room in it).
signed long long int display_bytes(std::string &&bytes);
// A LINE MADE BY THE INTERPRETER -- a styled display's -- after whatever std::cout holds, so a
// clear() or home() written just before it is not overtaken (the review, 2026-09-26).
signed long long int display_line_bytes(std::string &&bytes);

// A FLUSH: once this returns, everything handed over before it has been written -- or refused, or
// let go after an overrun. Answers success, or display_error when the screen refused some of it.
signed long long int display_drain();

// THE OVERRUN. Set by the printing satellite, which also sets program_quit(), so every walker
// stops between two statements (program_walk.cpp). Reported ONCE, by whichever asks first.
bool display_overran();
bool display_overrun_is_mine_to_report();
std::string display_overrun_sentence();
// THE PROMPT, BEFORE ITS NEXT LINE: the overrun stopped that line, and the next one starts clean.
void display_overrun_let_go();
// CTRL-C STOPPED A LINE AT THE PROMPT: what it displayed and is still waiting is let go, as a
// terminal lets go of its output on Ctrl-C -- up to 131,072 lines would otherwise scroll on, and
// the second press that stops them ends the whole session (the review, 2026-09-26).
void display_let_go_of_what_waits();

// display_stream.cpp: std::cout's 8 KB handed over now, when it holds any. display_value calls it
// first, so a line never overtakes satl's own words written before it.
void hand_over_what_std_cout_holds();
// ...and std::cout pointed at the printing satellite.
void std_cout_goes_to_the_printing_satellite();

} // namespace satellite004
