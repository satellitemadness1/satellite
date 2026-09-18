#pragma once
// `satl --config` -- what it prints. SATELLITE_ARGUMENTS C1 and C8.
//
// Split from machine_probe.hpp because that file MEASURES and this one SAYS, and
// the measuring is what anything else would want to call. arguments.machine.threads
// (C6) calls threads_this_machine_allows() and prints nothing at all.
//
// THE COMMAND TAKES AN OPTIONAL CAP: `satl --config 8192`.
//
// C1 says "--config is the whole line", and this is one word more than that, for
// a reason the milestone could not have known: the full probe on this machine
// asks for 379,924 threads, takes about nine seconds and holds three gigabytes,
// and there is no way to try the command without committing to that. A cap makes
// it possible to see exactly what it will do at a cost of 203 ms, and it is the
// right answer for a container or a shared machine besides. Without a cap it
// behaves exactly as C1 wrote it.
//
// IT REFUSES A CAP ABOVE THE CEILING rather than quietly lowering it. Asking for
// more threads than the kernel allows is a person who believes something untrue
// about their machine, and the useful answer is the number, not a silent clamp.

#include "machine_probe.hpp"
#include "../machine/critical_report.hpp"
#include "../machine/machine_codes.hpp"

#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace satellite004 {

namespace machine_probe {

// 1234567 -> "1,234,567". Every number here is large enough that the groups are
// the difference between reading it and counting digits.
inline std::string grouped(unsigned long long int value)
{
    std::string digits = std::to_string(value);
    for (std::size_t at = digits.size(); at > 3;) {
        at -= 3;
        digits.insert(at, ",");
    }
    return digits;
}

inline std::string megabytes(unsigned long long int bytes)
{
    std::ostringstream said;
    said << std::fixed << std::setprecision(0) << double(bytes) / (1024.0 * 1024.0) << " MB";
    return said.str();
}

} // namespace machine_probe

// C8 -- run it, and say what it measured and where it went.
inline signed long long int run_config(unsigned long long int cap = 0)
{
    using namespace machine_probe;

    std::cout << "satl --config: measuring what this machine can do\n\n";

    // D7 -- every ceiling, named, so a person can see which one binds and go
    // raise THAT one rather than guessing.
    const std::vector<ThreadCeiling> ceilings = read_thread_ceilings();
    std::string binding;
    const unsigned long long int lowest = lowest_thread_ceiling(ceilings, &binding);

    std::cout << "    every thread ceiling this system states\n";
    std::cout << "    " << std::string(68, '-') << "\n";
    for (const ThreadCeiling &ceiling : ceilings) {
        std::cout << "    " << std::left << std::setw(36) << ceiling.source << std::right << std::setw(14);
        if (ceiling.read == false)
            std::cout << "not stated";
        else
            std::cout << grouped(ceiling.threads);
        if (ceiling.read == true && ceiling.threads == lowest && ceiling.source == binding)
            std::cout << "   <- binds";
        std::cout << "\n";
    }

    if (lowest == 0) {
        CriticalReport none;
        none.code = "S0725";
        none.name = "NO_THREAD_CEILING_STATED";
        none.description =
            "satl --config could not read a single thread ceiling from this system, so it has no "
            "number to stop short of and will not probe. A probe with no ceiling runs until the "
            "machine cannot make another thread, and that takes the last one from the desktop too.";
        none.directory = "/proc/sys/kernel/threads-max";
        print_critical(none);
        return machine_fact_not_read;
    }

    const unsigned long long int target = cap != 0 ? cap : probe_target(lowest);
    std::cout << "\n    lowest ceiling      " << std::setw(14) << grouped(lowest) << "   (" << binding << ")\n";
    std::cout << "    probing up to       " << std::setw(14) << grouped(target);
    if (cap != 0)
        std::cout << "   (the cap you asked for)\n";
    else
        std::cout << "   (three quarters of it -- the rest stays free)\n";

    if (cap != 0 && cap > lowest) {
        CriticalReport too_many;
        too_many.code = "S0726";
        too_many.name = "CAP_ABOVE_THE_CEILING";
        too_many.description =
            "satl --config was asked to probe " + grouped(cap) + " threads and this machine's "
            "lowest ceiling is " + grouped(lowest) + " (" + binding + "). Nothing was probed. "
            "Raise that ceiling if you meant it, or ask for fewer.";
        too_many.directory = binding;
        print_critical(too_many);
        return setting_out_of_range;
    }

    std::cout << "\n    every thread parks on a condition variable and never spins. Parked\n";
    std::cout << "    threads cost the scheduler nothing; runnable ones cost the machine.\n\n";
    std::cout.flush();

    ProbeResult result = probe_threads(target);
    result.lowest_ceiling = lowest;
    result.binding_source = binding;
    result.capped = (cap != 0);

    std::cout << "    threads          held for       resident    ns a thread\n";
    std::cout << "    " << std::string(56, '-') << "\n";
    for (const ProbeStep &step : result.steps) {
        std::cout << "    " << std::right << std::setw(12) << grouped(step.threads)
                  << std::setw(16) << (step.reached ? "reached" : "REFUSED")
                  << std::setw(13) << megabytes(step.resident_bytes)
                  << std::setw(15) << grouped(step.nanoseconds_each) << "\n";
    }

    // A REFUSAL IS REPORTED AND IS NOT A FAILURE. The probe stopped short of a
    // ceiling /proc stated, and the machine still said no -- which means
    // something else on it is holding threads. The number reached is true and
    // is worth keeping; what would be wrong is claiming the target.
    if (result.refused_early == true) {
        std::cout << "\n    the machine refused before the target, so something else here is\n";
        std::cout << "    holding threads. " << grouped(result.measured)
                  << " is what it really allowed, and is what was saved.\n";
    }

    if (write_machine_conf(result) == false) {
        CriticalReport not_written;
        not_written.code = "S0727";
        not_written.name = "MACHINE_CONF_NOT_WRITTEN";
        not_written.description =
            "satl --config measured " + grouped(result.measured) + " threads and could not save it, "
            "so every run will keep answering from the /proc ceilings instead. That answer is still "
            "true -- it is the ceiling rather than the measurement -- so nothing is broken.";
        not_written.directory = machine_conf_path().empty() ? std::string("$HOME is not set, so there is no ~/.satl")
                                                            : machine_conf_path();
        print_critical(not_written);
        return machine_conf_unwritable;
    }

    std::cout << "\n    measured " << grouped(result.measured) << " threads at once\n";
    std::cout << "    written to " << machine_conf_path() << "\n";
    std::cout << "    arguments.machine.threads answers from that file from now on.\n";
    if (cap != 0) {
        // SAID HERE AND SAID IN THE FILE. A capped run's number becomes what
        // arguments.machine.threads answers, and a cap asked for once while
        // trying the command would otherwise be indistinguishable from a
        // measurement -- a quietly wrong number that survives every later run.
        std::cout << "    it was a capped run, so that number is your cap and not this machine's.\n";
        std::cout << "    run satl --config with no number for what this machine really allows.\n";
    }
    return success;
}

} // namespace satellite004
