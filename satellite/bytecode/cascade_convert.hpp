#pragma once
// satellite/bytecode/cascade_convert.hpp -- A THREAD FOR EACH LINE.
//
// (the author, 2026-09-16) "First main calculates the line count of the program
// as the program is returned in a std::vector<std::vector<std::string>> if it
// isn't programmed like that -- make it like that. Main creates a thread for each
// line, gives it a string, and that thread returns it's single line."
//
// And: "let's keep our design of loading the entire program into 16-bit before we
// run anything... that will simplify this... so for each line, create a thread,
// it converts 1 line and returns that 1 line."
//
// SO THIS IS THE WHOLE DESIGN, AND IT IS DELIBERATELY SIMPLE. No cascade of
// threads starting threads, no watermark, no running while converting. The entire
// program is loaded, every line gets a thread, every thread converts its one line
// and hands it back, and nothing runs until all of them have.
//
// WHY IT IS BUILT FOR A PROGRAM THAT DOES NOT EXIST YET. The author: "I have had
// programs that are 13k, but this is designed for AI -- the AI is going to be...
// at least 200,000 lines minimum, so this process will pay off". QUAD writes
// satellite, and a 200,000-line program is the case this is for, not a 200-line
// one.
//
// THE NUMBERS FOR 200,000 THREADS, on this machine, so they are written down
// somewhere other than a message: a thread costs 34.3 KB (8.3 KB program + 26 KB
// kernel), so 200,000 of them is about 6.9 GB of 61 GB. `ulimit -u` is 252,845
// and TasksMax is `max`, so the count fits. A thread START is about 28,254 ns,
// so creating 200,000 is about 5.6 s of creation on the thread doing the
// creating -- that is the cost to watch, not the memory.
//
// ORDER CANNOT GO WRONG. Every line converts into its OWN piece and the pieces
// are laid end to end BY INDEX, never by the order the threads finished in.

#include "../threads/startup_threads.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

// THE PROGRAM AS TEXT: one vector a file, one string a line (the author).
using ProgramText = std::vector<std::vector<std::string>>;

// Every line in the program, so main can say how many threads it is about to
// make before it makes them.
std::size_t lines_in(const ProgramText &text);

// One thread a line. Each converts its own line into its own piece; the pieces
// are laid into `file` in order afterwards.
//
// `threads` is the warm pool, used only when a line's own thread cannot be
// started -- the machine refusing a thread must not lose a line.
void convert_a_thread_for_each_line(const std::vector<std::string> &lines,
                                    std::vector<std::bitset<16>> &file,
                                    StartupThreads &threads);

} // namespace satellite004
