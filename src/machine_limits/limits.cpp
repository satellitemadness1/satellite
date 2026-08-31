// What satl is holding itself to: where the file is, what the machine says when
// there is none, and the one place the answer lives. See
// machine_limits/limits.hpp.
//
// THE ORDER IS MACHINE FIRST, THEN FILE, AND THAT IS WHAT MAKES A ONE-LINE
// CONFIG CHANGE ONE THING. Every value is filled in from the machine before the
// file is opened, so a file that sets `THREAD_COUNT` and nothing else leaves
// the memory ceiling and the core count exactly as the machine reported them --
// and `satl --limits` can say, of every row, whether anybody chose it.

#include "machine_limits/limits.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/pool.hpp"
#include "machine_limits/watchdog.hpp"
#include "programs/check_command.hpp"
#include "programs/opening.hpp"
#include "system_facts/facts.hpp"

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

// Fill in what the machine says. Everything the file does not mention keeps
// these answers, and `satl --limits` prints them as coming from the machine.
void from_the_machine(Held &into)
{
    into.thread_count = Setting{facts::hardware_threads(), Origin::Default, 0};
    into.core_count = Setting{facts::physical_cores(), Origin::Default, 0};

    // §4.5.4 ASKED WHETHER THE SHIPPED DEFAULT IS THE WHOLE MACHINE OR A
    // FRACTION. It is the whole machine. A fraction is a number satl would have
    // invented about a program it has never seen -- 50% is generous for a
    // parser and absurd for the thing QUAD.md exists to run -- and DESIGN §1.1
    // refuses exactly that kind of quiet policy. The machine's own total is the
    // one bound that is true without knowing anything: past it the run is not
    // slow, it is over.
    into.memory_max = Setting{facts::mem_total_bytes(), Origin::Default, 0};

    // min_free_mb IS UNSET BY DEFAULT, AND THAT IS A CORRECTION TO v1 RATHER
    // THAN A PORT OF IT. v1 defaulted it to 4096 MB and compared the MACHINE's
    // available memory against it once a second -- which on any machine with
    // less than 4 GB free means satl kills itself one second after starting,
    // every time, having done nothing wrong. That is not a conservative default;
    // it is a machine-sized assumption written as a number, and this tree has
    // 61.9 GiB so it would never have been noticed here. Unset means the
    // machine's free memory is not watched, MEMORY_MAX carries the promise on
    // its own, and a person who wants v1's check writes one line.
    //
    // The other three dials arrive unset too, and PLAN M6 is why: "min_free_mb
    // is the only one whose meaning is this milestone's." A default for
    // division_digits would be M8 deciding what a division does, three
    // milestones early.
}

} // namespace

std::string human_bytes(unsigned long long bytes)
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
    return std::to_string(whole) + "." + std::to_string(tenth) + " " +
           kNames[name];
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
    from_the_machine(into);

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
            // The machine's answers are restored before returning, so nothing
            // downstream holds half a file's worth of settings. Nothing runs
            // after this today -- main returns -- and a partly-applied config
            // is the kind of state that becomes a defect the moment something
            // does.
            from_the_machine(into);
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
    const unsigned long long total = facts::mem_total_bytes();
    if (total != 0 && into.memory_max.value > total) {
        into.memory_max.value = total;
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
    pool::start(static_cast<unsigned>(into.thread_count.value));
    start_watchdog();
    return EXIT_FINE;
}

const Held &held()
{
    return store();
}

} // namespace satellite::limits
