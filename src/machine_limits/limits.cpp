// What satl is holding itself to: where the file is, what the machine says when
// there is none, and the one place the answer lives. See
// machine_limits/limits.hpp.
//
// THERE IS NO ORDER ANY MORE, AND THAT IS THE CORRECTION THIS FILE CARRIES.
// Until 2026-08-31 it read "the order is MACHINE FIRST, THEN FILE, and that is
// what makes a one-line config change one thing": every value was filled in
// from the machine before the file was opened, so a file naming only
// `THREAD_COUNT` left the other two as the machine had reported them.
//
// THE ORDER EXISTED BECAUSE THE FILE COULD NOT SAY "THE MACHINE", and it cost
// 0.42 ms on every run of satl to work around that -- `facts::physical_cores()`
// reads two sysfs files per CPU, 48 of them here, and at M6 the only thing that
// wants the answer is one row of `satl --limits`. It was paid by `satl
// --version`, by `satl --help`, by every command in the program.
// make_support/040-sources.mk has the measurement.
//
// SO THE FILE CAN SAY IT NOW. `CORE_COUNT=arguments.machine.cores` is the
// machine's own answer, named the way a satellite program names it (DESIGN
// §7.7), and `CORE_COUNT=12` is twelve. A row nobody wrote a line for reads
// from the machine because that is what a `Setting` defaults to, not because
// anything filled it in. One pass, no order, and nothing is read from the
// machine until somebody asks for the value -- which is what `value()` below
// is, and what a one-line config still changing exactly one thing now falls
// out of rather than being arranged.
//
// WITH ONE EXCEPTION, WHICH IS THE STACK AND NOT A SETTING. begin() raises
// RLIMIT_STACK to a share of total memory, so it reads /proc/meminfo before it
// does anything else -- the argument for why that is a different thing from
// what this paragraph refuses is beside the call.

#include "machine_limits/limits.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/pool.hpp"
#include "machine_limits/watchdog.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "system_facts/facts.hpp"

#include <climits>
#include <cstdio>
#include <string>
#include <vector>

#include <limits.h>
#include <unistd.h>

namespace satellite::limits {

namespace {

// THE ONE HOLDER, AND IT IS A PROCESS-WIDE STATIC WHERE words::Words IS A LOCAL.
// words_runtime.hpp makes the point that "a run's names end with the run",
// because M22 runs many programs in one process and each needs its own
// numbering. These are the opposite kind of fact: THREAD_COUNT and MEMORY_MAX
// are properties of the process and of the machine it is on, and the pool and
// the watchdog they configure are started once and outlive every program the
// prompt will run. One holder is right here for exactly the reason a local is
// right there.
Held &store()
{
    static Held held;
    return held;
}

// The machine's own answer to one of the three, asked NOW.
//
// NOTHING IS CACHED HERE AND system_facts/facts.hpp's HEADER IS THE ARGUMENT:
// "the question is what this program is using NOW, while it runs, so a cached
// answer would be the wrong one by definition." Two of these three could not
// change during a run -- a machine does not grow cores -- and caching them
// would still buy nothing, because the expensive one is asked for exactly once
// per run by exactly one command. A cache that saves nothing is a second place
// for the answer to live.
//
// §4.5.4 ASKED WHETHER THE SHIPPED DEFAULT FOR MEMORY_MAX IS THE WHOLE MACHINE
// OR A FRACTION, and this is where it is answered: the whole machine. A
// fraction would be a number satl invented about a program it has never seen --
// 50% is generous for a parser and absurd for the thing QUAD.md exists to run
// -- and DESIGN §1.1 refuses exactly that kind of quiet policy. The machine's
// own total is the one bound that is true without knowing anything: past it the
// run is not slow, it is over.
unsigned long long from_the_machine(Fact fact)
{
    switch (fact) {
    case Fact::Threads:     return facts::hardware_threads();
    case Fact::Cores:       return facts::physical_cores();
    case Fact::MemoryTotal: return facts::mem_total_bytes();
    }
    return 0;
}

} // namespace

// THREE OF THE FOUR ORIGINS ANSWER FROM THE MACHINE AND ONE ANSWERS FROM THE
// FILE, which is the whole shape of this function and is worth reading as a
// sentence: satl holds itself to the machine unless somebody wrote a number.
//
// Clamped IS ON THE MACHINE'S SIDE AND THAT IS NOT AN OPTIMISATION. A clamped
// row is one where the file asked for more memory than exists, so the value IS
// the machine's total -- and computing it from the fact rather than copying it
// into `written` leaves the file's own over-large figure intact, which is the
// only place that number survives at all.
unsigned long long Setting::value() const
{
    // ASKED ONLY WHEN THE ANSWER IS THE MACHINE'S. A row the file wrote a
    // number for never opens /sys or /proc at all, which is the ordinary case
    // for a machine that HAS a config and the whole of why value_given() is
    // separate for the one command that does not.
    return origin == Origin::File ? written : value_given(from_the_machine(fact));
}

// THE SHARE OF THE MACHINE satl ASKS FOR AS A STACK. limits.hpp is the policy
// and what it is worth; this is the arithmetic, and the two decisions
// SCRATCH.md/NO_LIMITS.md §4.1.2 left open.
//
// OF TOTAL MEMORY AND NOT OF MEMORY_MAX, which was the first of them. DESIGN
// §7.7 pairs the two, and they are not the same kind of thing:
// `arguments.memory.total` is a fact about the machine and MEMORY_MAX is a
// setting somebody may write a number for -- one that is read at the BOTTOM of
// begin(), after the raise, so a share of it would mean reordering startup so
// that how deep a program may go depends on a line in a file. And it would be
// the wrong quantity anyway: this is address space, MEMORY_MAX is a promise
// about resident memory, and the watchdog is already counting the pages a deep
// recursion actually touches.
//
// THE FLOOR IS APPLIED HERE AND NOT AT THE CALL, so that every caller gets one
// answer -- begin() asks the kernel for this number and dump.cpp prints it, and
// a floor on one side only is how those two come to disagree. A total of 0 is a
// machine that would not say what it has, and it lands on the floor by the same
// line rather than by a case of its own.
//
// AND THE DIVISION COMES BEFORE THE MULTIPLICATION, which is what keeps this
// exact at any size a machine can have: a byte count that is divided before it
// is scaled cannot overflow 64 bits, where 512 TiB scaled first would. What
// the truncation costs is whatever the machine has past a whole megabyte --
// under 32 KiB of stack, which is not a number anybody has to think about.
unsigned long long wanted_stack_bytes_given(unsigned long long memory_total)
{
    const unsigned long long megabytes = memory_total / (1024 * 1024);
    const unsigned long long share = megabytes * kStackPerMegabyte;
    return share < kStackFloorBytes ? kStackFloorBytes : share;
}

unsigned long long wanted_stack_bytes()
{
    return wanted_stack_bytes_given(facts::mem_total_bytes());
}

std::string found_config_path()
{
    // dirname(/proc/self/exe), which IS the install: PLAN §5 puts the binary
    // directly in $HOME/.satl rather than in a bin/ underneath it, so a file
    // beside satl is that install's settings. readlink and not argv[0], because
    // argv[0] is whatever the caller felt like and a symlink on PATH -- which
    // §5 says is how `satl` is normally reached -- would answer with the link's
    // directory rather than the install's.
    char exe[PATH_MAX];
    const ssize_t got = readlink("/proc/self/exe", exe, sizeof exe - 1);
    if (got <= 0)
        return {};
    exe[got] = '\0';

    std::string path(exe);
    const size_t slash = path.find_last_of('/');
    if (slash == std::string::npos)
        return {};
    path.resize(slash + 1);
    path += "satellite_config.ini";

    // F_OK AND NOT R_OK, WHICH IS main.cpp's OWN ARGUMENT ONE MODULE OVER RUN
    // DELIBERATELY BACKWARDS. There, "it is there" and "I may open it" are
    // different answers and only the second is useful, because the caller is
    // about to be told their file cannot be run. The question here is not
    // whether the file can be opened, it is whether this install HAS a config
    // at all -- and a config that exists and cannot be read must not be
    // silently skipped. So it is FOUND here and open_source() in begin() fails
    // on it, reporting S0401 with the file's name. R_OK would make an
    // unreadable config indistinguishable from no config, which is the one
    // outcome this file may not produce.
    return access(path.c_str(), F_OK) == 0 ? path : std::string();
}

int begin(const std::string &named)
{
    Held &into = store();

    // A DEFAULT-CONSTRUCTED Held IS ALREADY A COMPLETE ANSWER -- every row
    // reading from the machine, which is what satl holds to when there is no
    // file, and no file is the ordinary case. Assigned rather than assumed
    // because begin() is called twice in tests/limits_test and a second call
    // must not see the first one's file.
    into = Held{};

    // FIRST, AND BEFORE THE POOL. satl asks the kernel for a bigger stack --
    // limits.hpp says what for and what it does not buy. It is here rather than
    // in main() because this file is the policy over what system_facts/ reports,
    // which is the seam LAYOUT.md draws between the two directories; and it is
    // FIRST because everything after it may recurse and because the pool starts
    // at the end of this function.
    //
    // IT CANNOT FAIL AND IS NOT REPORTED. A machine that refuses leaves satl
    // exactly as it ran for its first seven milestones, which is a working
    // interpreter and not an error -- so there is nothing to say and no code to
    // say it with. What there IS, is a row in `satl --limits`, because M6's rule
    // is that every value satl holds to says where it came from.
    //
    // AND THIS IS THE ONE THING THAT DOES ASK THE MACHINE AT STARTUP, WHICH IS
    // AN EXCEPTION TO THIS FILE'S HEADER AND IS SAID THERE TOO. The stack is a
    // share of memory now, so wanted_stack_bytes() opens /proc/meminfo on every
    // run of satl -- including `satl --help`, which needs nothing else from the
    // machine at all. Measured 2026-08-31: 0.05 ms for the one cold read a run
    // makes, 0.013 ms warm. What the header refuses is physical_cores()'s
    // 0.42 ms of walking 48 CPUs' worth of sysfs to answer a question ONE
    // command asks -- this is a tenth of that for a number every run needs, and
    // `make startup` cannot pick it out of the noise.
    into.stack_before = facts::stack_limit_bytes();
    into.stack_now = facts::widen_stack(wanted_stack_bytes());

    const std::string path = named.empty() ? found_config_path() : named;
    if (!path.empty()) {
        std::string text;
        // open_source() IS THE SAME DOOR EVERY OTHER ARM USES, and it reports
        // S0401 with the file's name. A config that cannot be opened is a
        // command line that named something satl cannot do -- EXIT_USAGE --
        // which is what `satl --tokens nosuch.satl` already answers. It cannot
        // be reached by the FOUND file, which was stat'd a moment ago; it is
        // the `--limits nosuch.ini` case.
        if (!open_source(path, text))
            return EXIT_USAGE;

        std::vector<errors::Diagnostic> problems;
        if (!read_config(text, into, problems)) {
            report(path, text, problems);
            // Everything the half-read file touched is put back, so nothing
            // downstream holds half a config's worth of settings. Nothing runs
            // after this today -- main returns -- and a partly-applied config
            // is the kind of state that becomes a defect the moment something
            // does.
            into = Held{};
            return EXIT_MALFORMED;
        }
        into.config_path = path;
    }

    // §4.5.4's SECOND HALF: "does a machine with less than the file claims
    // win?" It does. A ceiling above what the machine has is not a ceiling --
    // the run reaches the OOM killer first and the watchdog never speaks -- so
    // a file asking for 128 GiB on this 61.9 GiB machine is holding satl to
    // nothing. It is CLAMPED AND SAID rather than clamped quietly, which is the
    // whole difference: `satl --limits` prints the row as coming from "the
    // machine, over the file", so the person who wrote 128 GiB finds out.
    //
    // ONLY A NUMBER CAN BE CLAMPED, AND THAT IS WHAT MAKES THE READ CONDITIONAL
    // RATHER THAN UNCONDITIONAL. A row that already reads from the machine
    // cannot be above what the machine has, so there is nothing to compare and
    // /proc/meminfo is not opened at all -- which is the ordinary case, because
    // most runs have no file. `Origin::File` is the only origin this test can
    // be true for.
    if (into.memory_max.origin == Origin::File) {
        const unsigned long long total = facts::mem_total_bytes();
        if (total != 0 && into.memory_max.written > total)
            into.memory_max.origin = Origin::Clamped;
    }

    // THREAD_COUNT IS NOT CLAMPED, AND THE ASYMMETRY IS THE POINT. Asking for
    // more threads than the machine has hardware for is oversubscription, which
    // is a real technique with real uses -- work that blocks on a disk or a
    // socket wants more threads than cores -- so refusing it would be satl
    // inventing a policy about a program it has not seen. Asking for more
    // memory than exists is not a technique; it is a number that cannot happen.
    // `satl --limits` prints the setting beside the machine's own answer so the
    // difference is visible either way, which is §4.5.4's third open question
    // answered: a setting MAY differ from a fact, and the fix is to show both
    // rather than to make one of them lie.
    pool::start(static_cast<unsigned>(into.thread_count.value()));
    start_watchdog();
    return EXIT_FINE;
}

const Held &held()
{
    return store();
}

unsigned division_digits()
{
    // THE DIAL IS THE AUTHORITY AT RUN TIME AND THE FILE IS NEVER RE-READ, per
    // this module's own header: the config seeds the dials once at startup and
    // everything reads them afterwards. So this is a lookup and not a decision
    // about where to look.
    const Dial &dial = held().dial(DialId::DivisionDigits);
    if (!dial.set)
        return kDivisionDigitsDefault;

    // AND WHAT IS SET IS WHAT IS RETURNED, WHICH IT WAS NOT UNTIL 2026-08-31.
    // Two branches stood here: a zero became 34 and anything past UINT_MAX
    // became UINT_MAX, both silently, and both were this function deciding
    // something about a file it cannot point at. config.cpp refuses each of them
    // now, on the line that wrote it and with a caret under the value --
    // kDivisionDigitsLeast and kDivisionDigitsMost in the header are the pair,
    // and that is why this is a lookup with nothing left in it.
    return static_cast<unsigned>(dial.value);
}

unsigned long long max_depth_bytes()
{
    const Dial &dial = held().dial(DialId::MaxDepth);
    if (dial.set)
        return dial.value;

    // THE SIBLING AND NOT THE MACHINE DIRECTLY. limits.hpp has the argument:
    // MEMORY_MAX is the whole machine when nobody has set it, so this is "the
    // machine" in the ordinary case and is the smaller, correct number in the
    // case where somebody has said how much of it satl may have.
    return held().memory_max.value();
}

} // namespace satellite::limits
