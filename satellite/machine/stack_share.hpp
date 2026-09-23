#pragma once
// satellite/machine/stack_share.hpp -- the stack a program runs on: 32 KiB of it
// for every MiB of memory the machine has.
//
// (the author, 2026-09-22) "we dealt with this already in 003, and found a way to
// set the number to 7.5% of the users total ram or some number, it was exactly 32
// kilobytes per 1 megabyte of ram ... so... let's set it for 004 then, 32 kb for 1
// megabyte of ram". 003's rule whole (its DESIGN §7.5, stack_facts.cpp's
// widen_stack()): 32 KiB a MiB, never under 128 MiB -- the floor is where the
// share stands at 4 GiB, so it only ever lifts a small machine.
//
// WHY. A capsule call is a chain of C++ calls in program_walk.cpp -- call_capsule,
// run_site, run_statements, and run_if and run_statements again for a call inside
// an if -- about 3.3 KB of stack a level. On the 8 MiB a login shell hands out, a
// capsule that calls itself from inside an if died with signal 11 at 2,526 deep
// (measured 2026-09-22; 2,525 ran), with no code and no sentence.
//
// THE 8 MiB IS A SOFT DEFAULT WITH AN UNLIMITED HARD LIMIT BEHIND IT on an
// ordinary Linux, so satl raises its own without root. Measured on this machine
// (62 GiB, so 1.94 GiB of stack): raised INSIDE the running process, 1 KiB frames
// reached the whole 1.94 GiB, the same as `ulimit -s` before the start and as a
// thread made with that stack -- so raising it from main() is enough, and nothing
// re-executes. A machine that refuses (a hard limit, a container) keeps what it
// gave; satl runs exactly as before, and there is nothing to report.
//
// ONLY THE MAIN THREAD'S, and the main thread is where a program runs: the file,
// the prompt, and every window event's capsule (the desk is GTK's thread and runs
// no capsule). glibc fixes the stack NEW threads get when it starts, before
// main(), so satl's 1,024 start-up threads keep 8 MiB each.
//
// AND A CHILD GETS BACK THE STACK satl WAS GIVEN. A child starts its glibc on the
// limit it inherits and makes that EVERY one of its threads' stack: a satl started
// on 1.94 GiB reserved 1.94 TiB of address space for its 1,024 threads, against
// 8.1 GiB on 8 MiB (measured 2026-09-22, resident memory the same). The two places
// satl starts a process -- `interpret` at the prompt, and the console menu's New
// Window and Open -- call the second function below in the child, and a child
// satl then raises its own.

#include <sys/resource.h>

namespace satellite004 {

constexpr unsigned long long int kStackPerMegabyte = 32ULL * 1024;
constexpr unsigned long long int kStackFloorBytes = 128ULL * 1024 * 1024;

// The share of a machine with memory_total bytes. The division comes first, so it
// cannot overflow at any size a machine can have.
inline unsigned long long int stack_share_of(unsigned long long int memory_total)
{
    const unsigned long long int share = memory_total / (1024 * 1024) * kStackPerMegabyte;
    return share < kStackFloorBytes ? kStackFloorBytes : share;
}

// Raise this process's stack to the share, first thing in main(). A stack already
// at least that big -- `ulimit -s` set higher, or unlimited -- is left alone.
void widen_the_stack();

// In a child between fork and exec: the limit satl started with. Only setrlimit,
// so it is safe to call there.
void hand_a_child_the_stack_satl_was_given();

} // namespace satellite004
