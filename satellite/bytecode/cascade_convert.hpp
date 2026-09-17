#pragma once
// satellite/bytecode/cascade_convert.hpp -- EVERY LINE CONVERTED ON 1024 THREADS
// BEFORE A SINGLE LINE RUNS.
//
// (the author, 2026-09-16) "let's create 1024 threads, and have 1024 convert every
// line of the program into 16-bits before a single line runs, that makes it so
// much easier to program, and there's nothing to test that way", and "you don't
// have to convert the lines in any order, convert them in any order that you
// want".
//
// THE 1024 ARE THE WARM THREADS. arguments.threads_startup is 1024, so they are
// started and parked before the program is loaded, and converting a file is
// handing them its lines. No thread is created per line and none per file.
//
// ANY ORDER TO CONVERT, ONE ORDER TO LAY. The lines of a file are cut into
// contiguous chunks, one per thread, and the chunks finish in whatever order they
// finish. Each chunk writes only its own piece, and the pieces are laid into the
// row by chunk index -- so the row is always the program's order, and no two
// threads ever touch the same vector.
//
// MAIN SLEEPS WHILE THEY WORK. It waits on a condition variable, not a loop that
// spins, so the machine is doing conversion and nothing else.

#include "../threads/startup_threads.hpp"

#include <bitset>
#include <cstddef>
#include <string>
#include <vector>

namespace satellite004 {

// Converts every line into `file`, in the program's order. The lines are spread
// over the warm threads; with none warm, they convert on the calling thread.
void convert_every_line(const std::vector<std::string> &lines,
                        std::vector<std::bitset<16>> &file,
                        StartupThreads &threads,
                        unsigned long long int thread_count);

} // namespace satellite004
