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
// WHAT DID NOT COME AT M6 AND ARRIVED AT M9, WHICH IS WHERE THIS PARAGRAPH SAID
// IT WOULD. v1's host_facts.cpp also holds username(), home_dir() and cwd().
// They are satellite_string's codes 95, 96 and 100 -- live values resolved at
// decode time, which SCRATCH.md/PORTING.md §3 counts -- and M6 refused them
// because "bringing them now would put three functions in this tree that
// nothing calls, which is exactly the exception 040-sources.mk had to write out
// for satellite_random; one deliberate exception is a decision and two is a
// habit." The caller is satellite_value/render.cpp and it lands with M9, so the
// three land with it, in system_facts/user_facts.cpp. The prediction was forty
// lines; it came out at seventy-five, and the difference is cwd() losing v1's
// fixed 4096-byte buffer.

#include <cstddef>
#include <string>

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
// M9 NO LONGER HAS A CEILING, AS OF 2026-08-31, SO THIS READER LOST A CONSUMER.
// DESIGN §7.5 was rewritten and PLAN §2.5 un-deferred: the language has no depth
// limit and M9 compiles onto an explicit control stack, so nothing derives a
// frame count from this any more. The paragraph below is why it was built and is
// kept -- the reader is still right about what the machine says, and the care it
// takes over RLIM_INFINITY answering UNKNOWN rather than UNBOUNDED is the same
// care either way. `SCRATCH.md/NO_LIMITS.md`.
//
// M9's CEILING CAME FROM HERE AND DESIGN §7.5 WAS WHY. A satellite capsule
// activation is a real chain of C++ calls, so the depth a program may recurse to
// is a property of `ulimit -s` rather than a number somebody picked -- and
// `ulimit -s` is deliberately outside the language, because asking for a 64 GB
// stack is a thing to do on purpose from a shell and not something a program can
// talk itself into partway through a run.
unsigned long long stack_limit_bytes();

// Ask the kernel for a bigger stack, and answer with what it gave.
//
// THE SOFT LIMIT IS A DEFAULT AND THE HARD LIMIT IS THE WALL, and on an
// ordinary Linux the wall is not there: this machine reports 8 MiB soft and
// UNLIMITED hard, so a process may raise its own stack without root and without
// asking anybody. `ulimit -s` is the shell setting a default, not the kernel
// setting a maximum -- which is the opposite of how it reads, and is why
// `facts.hpp` had a paragraph calling it "deliberately outside the language"
// until it was measured on 2026-08-31.
//
// IT RETURNS WHAT IS IN FORCE AFTERWARDS AND NEVER FAILS. A machine that
// refuses -- a container with a hard limit, a distribution that pins it -- is
// not an error and not something to report: satl runs exactly as it did before,
// which is what it did for every milestone up to this one. Clamped to the hard
// limit rather than attempted and failed, because asking for more than the wall
// is a guaranteed EINVAL and the point is to take what is available.
//
// AND IT IS THE MAIN THREAD'S. glibc fixes the default stack size for NEW
// threads at library init, before main() runs, so raising this afterwards does
// not multiply across M6's 24 pool threads -- measured 2026-08-31: VmSize is
// 230.3 MiB with and without, identical. The reservation itself costs nothing
// either, because a stack is lazily committed: 8 GiB reserved moved VmSize by
// 0.0 MiB and VmRSS by 0.2.
unsigned long long widen_stack(unsigned long long want);

// --- who is running this, and where -----------------------------------------
//
// satellite_string's live codes 95, 96 and 100 (DESIGN §5). Read fresh on every
// call, like everything else here -- see the note at the top of this file, and
// note that cwd() is the one where that is load-bearing rather than tidy.
std::string username();
std::string home_dir();
std::string cwd();

// --- a byte count as a person would write it --------------------------------

// `61.9 GiB`, `4.0 MiB`, `512 B`.
//
// BINARY UNITS AND ONE DECIMAL PLACE, and the exact byte count is printed
// BESIDE it everywhere this is used rather than instead of it. PLAN §4.5.4's
// whole question was that 61.9 GiB and 64.9 GB are the same memory, so a report
// that gave only the rounded form would reintroduce the ambiguity the
// configuration file was made to remove. This is the readable half of a pair
// and never the whole answer.
//
// IT LIVED IN machine_limits/limits.hpp UNTIL M9 AND MOVED WHEN IT GOT A SECOND
// CONSUMER. Every caller was inside that one module, so that was the right
// home while it lasted. The evaluator's S0701 says how many bytes a runaway
// recursion is holding and what `max_depth` allowed it, and it must not include
// machine_limits at all -- evaluator/machine.hpp's Policy note is why, and
// MILESTONES/M8.5.md §4.1 is the receipt for what a test binary linking the
// module that raises RLIMIT_STACK costs. A byte count is this module's currency:
// every function above answers in bytes, so the readable form of one belongs
// beside them rather than beside a single consumer. PLAN §6.1 item 3 made the
// same move with `Bits32` and gave the same reason -- one in the tree beats two
// that agree until somebody edits one.
//
// INLINE, SO NOTHING NEW IS LINKED. It reads no file and asks the machine
// nothing, which is what lets it sit in a header the whole tree already
// includes.
inline std::string human_bytes(unsigned long long bytes)
{
    static const char *const kNames[] = {"B", "KiB", "MiB", "GiB", "TiB", "PiB"};
    constexpr size_t kNameCount = sizeof kNames / sizeof kNames[0];

    size_t name = 0;
    unsigned long long whole = bytes;
    unsigned long long remainder = 0;
    while (whole >= 1024 && name + 1 < kNameCount) {
        remainder = whole % 1024;
        whole /= 1024;
        name++;
    }
    if (name == 0)
        return std::to_string(whole) + " B";

    // One decimal place, rounded down, computed from the remainder rather than
    // through a double -- the exact figure is printed beside this everywhere it
    // is used, so what this owes the reader is a number that never rounds UP
    // past a ceiling it is describing.
    const unsigned long long tenth = remainder * 10 / 1024;
    return std::to_string(whole) + "." + std::to_string(tenth) + " " + kNames[name];
}

} // namespace satellite::facts
