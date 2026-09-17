#pragma once
// satellite/bytecode/cascade_convert.hpp -- THE CASCADE. A thread converts a
// line and starts a thread that converts a line.
//
// (the author, 2026-09-16) "there are 256 threads, so we start a thread, it
// starts a thread and converts a piece, then main starts the next thread, every
// thread that it starts does this: converts a line and starts a thread that
// converts a line... so we gang on the entire file, while main is assured that
// it receives the first 256 lines... then the 256 warm threads continue
// conversion... so this has to work over multiple files and stuff, so the entire
// program is converted, until the entire program is converted".
//
// THE TWO HALVES, AND EACH IS FOR A DIFFERENT THING:
//
//   THE CASCADE covers the FRONT of a file. Main starts one thread; that thread
//   converts line 0 and starts the thread that converts line 1, which starts the
//   thread that converts line 2. Each converts BEFORE it spawns, so a line is
//   ready as early as it can be -- spawning first would put a thread start in
//   front of the conversion it is waiting on.
//
//   THE WARM THREADS cover the REST. They are already parked, so handing them a
//   batch costs a recall and never a thread start, and PROGRESS already measured
//   256 batches at 27x one thread on 100,000 lines.
//
// WHY THE CASCADE STOPS. It is bounded by `cascade_depth` -- the author's "main
// is assured that it receives the first 256 lines" -- because a cascade that ran
// the whole way would hold one live thread per line, and a thread costs 34.3 KB
// (DESIGN §13). A 100,000-line file would be 3.4 GB of threads. The front of the
// file is where the latency matters, and that is exactly what it covers.
//
// EVERY LINE GETS ITS OWN PIECE, so no two threads write to one vector and
// nothing needs a lock. The pieces are laid end to end IN ORDER afterwards,
// which is what keeps the author's "the step can never go out of order" true by
// construction rather than by a check: order is the piece's index, not the order
// the threads happened to finish in.
//
// OVER MULTIPLE FILES: this converts ONE file. load_program calls it once a
// file, walking the include tree, so the whole program is converted by the same
// machinery -- the cascade runs again at the front of each file.

#include "../threads/startup_threads.hpp"

#include <atomic>
#include <bitset>
#include <cstddef>
#include <string_view>
#include <vector>

namespace satellite004 {

// Converts every line into `file`, in order. `cascade_depth` lines go through
// the cascade; the rest go to the warm threads in batches.
//
// `codes_final`, when given, is raised as each line's codes are appended: every
// code below it is final and safe to read while later lines are still arriving.
// That is the author's "the thread that gets line 2 is handing it to main", and
// the reason `file` is reserved up front -- storage that never moves is what
// makes reading it while it grows safe at all.
void cascade_convert(const std::vector<std::string_view> &lines,
                     std::vector<std::bitset<16>> &file,
                     StartupThreads &threads,
                     std::size_t cascade_depth,
                     unsigned long long int batches,
                     std::atomic<std::size_t> *codes_final = nullptr);

} // namespace satellite004
