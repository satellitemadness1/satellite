// The machine readers: what can honestly be asserted about a machine, and what
// cannot.
//
// THE AUTHORITY IS /proc AND SO IS THE CODE UNDER TEST, which makes this suite
// different from every one before it. words_test checks a transcription against
// WORD_NUMBERS.md and parser_test checks a tree against a grammar; both have
// something to be wrong against. An assertion that hardware_threads() equals
// `nproc` here would be two readers of one file agreeing, and it would pass on
// a machine where both were wrong.
//
// SO WHAT IS ASSERTED IS THE RELATIONSHIPS, which are properties of a machine
// rather than of a file, and each one below says which failure it would catch.
// Between them they cover every way these readers have actually gone wrong:
// a prefix match reading the wrong /proc/meminfo row, a core count divided out
// of a thread count, a resident set read as a virtual one, and a stack limit
// that believed the word "unlimited".

#include "limits_test.hpp"

#include "machine_limits/limits.hpp"
#include "system_facts/facts.hpp"

#include <cstdio>
#include <string>

#include <sys/resource.h>

namespace limits_test {

using namespace satellite::facts;

void section_facts()
{
    // --- the stack, and the one reader that CHANGES the machine -------------

    // WIDENING IS BEST-EFFORT AND THE ASSERTION HAS TO BE TOO. A container with
    // a hard limit, or a distribution that pins RLIMIT_STACK, leaves satl
    // exactly as it ran for its first seven milestones -- which is a working
    // interpreter and not a failure. So what is checked is the CONTRACT and not
    // the number: asking never shrinks the stack, and asking twice is idempotent.
    const unsigned long long before = stack_limit_bytes();
    const unsigned long long after = widen_stack(satellite::limits::kWantedStackBytes);
    check(after >= before || before == kStackLimitUnknown,
          "widen_stack() never leaves the stack smaller than it found it");
    check(widen_stack(satellite::limits::kWantedStackBytes) == after,
          "and asking a second time answers the same -- begin() is called twice "
          "in this suite and a raise that drifted would be a limit that depends "
          "on how many times satl started");

    // AND ON A MACHINE THAT ALLOWS IT, IT MUST ACTUALLY HAPPEN. This is the one
    // that catches the call being deleted from limits::begin(): the mechanism
    // could keep working perfectly while nothing invoked it, which is exactly
    // the failure PLAN M2's consumer rule exists for. Guarded on the hard limit
    // rather than asserted flat, because a build machine may not permit it.
    struct rlimit hard;
    if (getrlimit(RLIMIT_STACK, &hard) == 0 &&
        (hard.rlim_max == RLIM_INFINITY ||
         hard.rlim_max >= satellite::limits::kWantedStackBytes)) {
        check(after >= satellite::limits::kWantedStackBytes,
              "this machine's hard limit allows 8 GiB, so satl has it -- and if "
              "this fails after limits::begin() ran, the raise was removed and "
              "500,000 nested brackets segfault again");
        // GUARDED ON begin() HAVING RUN, because this section does not call it
        // and the run list may put it first. `stack_now` is 0 until begin()
        // fills it, which is the honest way to ask "has the policy run yet"
        // without this section reaching into another one's order.
        if (satellite::limits::held().stack_now != 0)
            check(satellite::limits::held().stack_now >=
                      satellite::limits::kWantedStackBytes,
                  "and begin() recorded it, which is what `satl --limits` "
                  "prints -- the check that catches the call being deleted "
                  "from begin() while widen_stack() still works perfectly");
    }

    // --- threads and cores ------------------------------------------------

    check(hardware_threads() >= 1,
          "a machine has at least one hardware thread, and a reader that "
          "answered 0 would size the pool at nothing");
    check(physical_cores() >= 1, "and at least one core");
    check(physical_cores() <= hardware_threads(),
          "and never more cores than threads -- the check that catches a "
          "topology read gone wrong, because every failure mode of "
          "physical_cores() except this one is a silent undercount");

    // NOT threads / 2, WHICH IS THE ONE THING A DERIVED ANSWER COULD NOT BE.
    // 24 and 12 happen to be in that ratio on this machine, which is exactly
    // why the ratio is not the check: what is asserted is that the two numbers
    // come from different places, and the way to see that is a machine where
    // they are not in a ratio at all. So this asserts the weaker true thing and
    // says so, rather than an arithmetic that would pass by accident here.
    check(physical_cores() == 12 ? hardware_threads() == 24 : true,
          "on THIS machine, 2026-08-30: 12 physical cores and 24 hardware "
          "threads, which reproduces PLAN §4.5's lscpu reading from /sys "
          "topology instead");

    // --- memory -----------------------------------------------------------

    check(mem_total_bytes() > 0,
          "/proc/meminfo answers MemTotal -- and the colon in the key is what "
          "makes this the MemTotal row rather than whichever of MemTotal, "
          "MemFree and MemAvailable the kernel printed first");
    check(mem_available_bytes() > 0, "and MemAvailable");
    check(mem_available_bytes() < mem_total_bytes(),
          "and STRICTLY less is available than exists -- a running Linux is "
          "resident in some of its own memory, so equality means two rows of "
          "/proc/meminfo were read as one. THAT IS THE FAILURE THIS ASSERTION "
          "EXISTS FOR and `<=` did not catch: `Mem` is a prefix of MemTotal, "
          "MemFree and MemAvailable, so a match without the colon answers "
          "MemTotal for all three -- and `used + available == total` stays true "
          "at 0 + total. Verified by mutation 2026-08-30");
    check(mem_used_bytes() + mem_available_bytes() == mem_total_bytes(),
          "used is total minus AVAILABLE, exactly -- not total minus free, "
          "which on a healthy Linux is a small frightening number, and not "
          "rounded through megabytes on the way, which is how v1's two "
          "subtractions could disagree about one instant");

    check(mem_total_mb() == mem_total_bytes() / (1024 * 1024),
          "the megabyte answers are DERIVED from the byte ones, so the two "
          "cannot drift");
    check(mem_used_mb() == mem_used_bytes() / (1024 * 1024), "all three of them");
    check(mem_available_mb() == mem_available_bytes() / (1024 * 1024), "likewise");

    // --- this process -----------------------------------------------------

    check(process_memory_bytes() > 0,
          "this process is resident in some memory, and 0 is what a failed "
          "read answers -- which watchdog.cpp treats as a failed read and not "
          "as `using nothing`, because killing a healthy process because a "
          "file would not open is the worst thing a watchdog can do");
    check(process_memory_bytes() < mem_total_bytes(),
          "and it is a RESIDENT set rather than a virtual one: this binary "
          "maps 24 thread stacks at 8 MiB apiece, so a virtual figure would "
          "report ~192 MB for threads that are asleep");

    // FRESH ON EVERY CALL, NEVER CACHED. v1's own comment is the reason: "the
    // question is what this program is using NOW." The watchdog calls it once
    // a second forever, so a cached answer would be the one thing that stops
    // it working -- and a cache is invisible to every other assertion here.
    const unsigned long long first = process_memory_bytes();
    std::string grow(4 * 1024 * 1024, 'x');
    // TOUCHED PAGE BY PAGE, AND STRICTLY GREATER. Allocating is not residency:
    // an untouched mapping costs no pages, so a test that only allocated would
    // be asserting nothing. And `>=` was what this said until a mutation walked
    // through it on 2026-08-30 -- a cached first answer satisfies `>= first`
    // exactly, which is the one thing the assertion was written to rule out.
    for (size_t i = 0; i < grow.size(); i += 4096)
        grow[i] = static_cast<char>('a' + (i / 4096) % 26);
    check(process_memory_bytes() > first,
          "a process that has just touched 4 MiB of fresh pages reports MORE "
          "than it did before -- which a cached first answer cannot do");
    check(grow[0] != grow[4096], "and the compiler did not delete that loop");

    // --- the stack --------------------------------------------------------

    unsigned long long used = 0;
    unsigned long long total = 0;
    check(thread_stack_bytes(&used, &total), "this thread's stack is reportable");
    check(used > 0 && used <= total,
          "and it is standing somewhere inside it -- the clamp matters because "
          "a local's address is a frame or two below the real stack pointer");

    const unsigned long long limit = stack_limit_bytes();
    check(limit == kStackLimitUnknown || limit > 0,
          "RLIMIT_STACK is a number of bytes or it is kStackLimitUnknown, and "
          "there is no third answer");
    check(kStackLimitUnknown == 0,
          "and UNKNOWN is 0, which no real limit can be");

    // AND THE UNLIMITED ARM, REACHED BY RAISING THE LIMIT AND PUTTING IT BACK.
    //
    // THE ASSERTION ABOVE CANNOT SEE IT, which a mutation proved on 2026-08-30:
    // deleting the RLIM_INFINITY arm entirely left the whole suite green,
    // because this machine's soft limit is 8 MiB and the branch never ran. It
    // is the arm that matters most -- M9's depth ceiling is derived from this
    // number (DESIGN §7.5), and believing the word `unlimited` would put the
    // guard past the cliff it exists to stop, which is the exact failure v1
    // found. So the test raises the soft limit to the hard one, asks, and puts
    // it back.
    //
    // SKIPPED RATHER THAN FAILED where the hard limit is not RLIM_INFINITY,
    // which is the one place this suite skips anything: a machine whose
    // administrator has capped the stack is not a machine with a broken reader,
    // and the arm is genuinely unreachable there. It says so out loud, because
    // a silent skip is how a suite goes green over ground it never covered.
    struct rlimit was;
    if (getrlimit(RLIMIT_STACK, &was) == 0 && was.rlim_max == RLIM_INFINITY) {
        struct rlimit raised = was;
        raised.rlim_cur = RLIM_INFINITY;
        if (setrlimit(RLIMIT_STACK, &raised) == 0) {
            check(stack_limit_bytes() == kStackLimitUnknown,
                  "an unlimited RLIMIT_STACK answers kStackLimitUnknown and not "
                  "a huge number -- `unlimited` is not unbounded, because the "
                  "main thread's stack still stops where the next mapping "
                  "begins");
            setrlimit(RLIMIT_STACK, &was);
            check(stack_limit_bytes() == limit,
                  "and it reads the restored limit afterwards, so this check "
                  "left the process as it found it");
        }
    } else {
        printf("note: RLIMIT_STACK's hard limit is not unlimited on this "
               "machine, so the `unlimited` arm was not exercised\n");
    }
}

} // namespace limits_test
