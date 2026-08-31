#pragma once

// What this machine is: its memory, its hardware threads, its physical cores,
// and the stack a thread is standing on. PLAN M6, and PLAN §4.5 is the
// specification.
//
// PORTED FROM THE FIRST SATELLITE'S src/system_facts/, WHICH IS WHERE PLAN M6
// SENDS A READER, and it is three files there: memory_facts.cpp (236 lines),
// host_facts.cpp (44) and stack_facts.cpp (52). What arrives here is what has a
// consumer at M6 plus what PLAN names as M8's and M9's, and the rest was left
// behind deliberately -- see WHAT DID NOT COME below.
//
// THE SEAM IS BETWEEN THESE READERS AND THE LANGUAGE, and PLAN M6 draws it in
// one sentence: "the readers are here, and satellite.system's twenty-eight paths
// are M20." So nothing in this file knows what a satellite program is, nothing
// here reads the namespace, and nothing here has an opinion about what a number
// should be -- it reports what the machine says. machine_limits/ is the module
// that turns these into a POLICY, and that separation is why the same
// mem_total_bytes() can answer a ceiling, a `satl --limits` row and (at M9)
// satellite_string's code 98 without three copies of the read.
//
// EVERY READ IS FRESH AND NOTHING IS CACHED, which is v1's decision carried over
// with its reason: "the question is what this program is using NOW, while it
// runs, so a cached answer would be the wrong one by definition." The watchdog
// calls process_memory_bytes() once a second forever, so a cache here would be
// the one thing that stops it working.
//
// WHAT DID NOT COME, AND WHY IT IS SAID RATHER THAN LEFT TO BE NOTICED.
// v1's host_facts.cpp also holds username(), home_dir() and cwd(). They are
// satellite_string's codes 95, 96 and 100 -- live values resolved at decode
// time, which SCRATCH.md/PORTING.md §3 counts -- and their milestone is M9, not
// this one. Bringing them now would put three functions in this tree that
// nothing calls, which is exactly the exception 040-sources.mk had to write out
// for satellite_random; one deliberate exception is a decision and two is a
// habit. They are forty lines and they come with M9.

#include <cstddef>

namespace satellite::facts {

// --- the machine ------------------------------------------------------------

// Hardware threads, never 0.
//
// WHAT THE OS REPORTS, AND **NOT** CORES x 2. PLAN §4.5.1.2 names that
// explicitly as the fallback's rule: 24 is what this machine answers and 2 is
// this CPU's SMT ratio rather than a law. A machine with SMT off answers 12,
// and a machine that never had it answers its core count.
unsigned hardware_threads();

// Physical cores, never 0. 12 on this machine against 24 threads.
//
// COUNTED FROM THE TOPOLOGY AND NOT DIVIDED OUT OF THE THREAD COUNT, for the
// reason above: threads / 2 is right on this Xeon, wrong on anything without
// SMT, and wrong again on a POWER machine with four or eight threads a core.
// The pair (physical_package_id, core_id) under /sys is what the kernel itself
// uses to mean "a core", so the count of distinct pairs IS the answer rather
// than an estimate of it.
//
// Falls back to hardware_threads() when /sys will not answer -- a container
// with a masked sysfs, or a machine that reports no topology at all. That is a
// wrong answer on an SMT machine and it is the honest one available: it never
// claims MORE cores than the machine has threads, which is the direction a
// caller can survive.
unsigned physical_cores();

// --- memory -----------------------------------------------------------------
//
// BYTES ARE THE REAL ANSWER AND THE MEGABYTE PAIR IS A CONVENIENCE, which is
// v1's argument kept whole: every unit the language offers is a power of 1024
// away from a byte, so every conversion from bytes terminates and is exact,
// while converting from an already-rounded megabyte has thrown that away before
// anybody sees it. `satl --limits` prints bytes and a human form BOTH, and the
// human form is derived from the bytes.
//
// USED IS TOTAL MINUS **AVAILABLE**, not total minus free. Linux spends every
// spare page on cache, so "free" on a healthy machine is a small frightening
// number, and available is the figure that answers what a program can still
// have. v1 made this call twice, in mem_used_mb() and mem_used_bytes(), and it
// is one function here.
unsigned long long mem_total_bytes();
unsigned long long mem_available_bytes();
unsigned long long mem_used_bytes();

unsigned long mem_total_mb();
unsigned long mem_available_mb();
unsigned long mem_used_mb();

// This process's own resident set, right now.
//
// RESIDENT AND NOT VIRTUAL. Virtual size counts address space the program
// reserved and never touched -- on a tree walker that is most of it, and worse
// here than in v1: the pool maps 24 thread stacks at 8 MB apiece, so a virtual
// figure would report ~192 MB of ceiling used by threads that are asleep and
// have touched a page each. A number that goes up when nothing was allocated is
// a number nobody can act on, and this is the number MEMORY_MAX is compared
// against.
//
// 0 MEANS THE READ FAILED, and machine_limits/watchdog.cpp treats it as such
// rather than as "using nothing" -- see the note at that call.
unsigned long long process_memory_bytes();

// --- the stack --------------------------------------------------------------

// This THREAD's stack: what it is using, and how big it may get. False when the
// platform will not say.
//
// THE STACK IS THE ONLY MEMORY A THREAD HAS OF ITS OWN. Linux accounts an
// address space per PROCESS and every thread shares it, so a per-thread "used
// memory" that meant heap would be the same number for all of them --
// /proc/self/task/<tid>/statm reports exactly that process-wide figure. The
// stack is the real per-thread quantity and it is also what decides how deep the
// interpreter may recurse.
bool thread_stack_bytes(unsigned long long *used, unsigned long long *total);

// RLIM_INFINITY and a failed getrlimit both answer this, and a caller reads it
// as "assume the ordinary 8 MB".
//
// UNLIMITED IS NOT TREATED AS UNBOUNDED, ON PURPOSE. The main thread's stack
// still stops where the next mapping begins, so believing the word would put a
// depth guard past the cliff again -- which is the exact failure the guard
// exists to prevent. v1 found this and the reason travels with the constant.
inline constexpr unsigned long long kStackLimitUnknown = 0;

// The stack this process may grow, in bytes -- RLIMIT_STACK's soft limit.
//
// M9's CEILING COMES FROM HERE AND DESIGN §7.5 IS WHY. A satellite capsule
// activation is a real chain of C++ calls, so the depth a program may recurse to
// is a property of `ulimit -s` rather than a number somebody picked -- and
// `ulimit -s` is deliberately outside the language, because asking for a 64 GB
// stack is a thing to do on purpose from a shell and not something a program can
// talk itself into partway through a run.
unsigned long long stack_limit_bytes();

} // namespace satellite::facts
