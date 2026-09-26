#include "interpreter/interp.hpp"
#include "interpreter/interp_internal.hpp"

#include "console_output/console.hpp"
#include "environment/env.hpp"
#include "evaluator/eval.hpp"
#include "spaceship_loader/loader.hpp"
#include "system_facts/interrupt.hpp"
#include "satellite_string/satellite_string.hpp"

#include <fstream>
#include <memory>
#include <sstream>
#include <vector>

// The --run path, out of interp.cpp: a whole program rather than a line, from a
// string (run_program) or from a file (run_file), with argv reaching it through
// args_to_list. The session path and the retained-image machinery both of these
// use stayed in interp.cpp; see interp_internal.hpp for what crosses.

namespace satellite {

List args_to_list(const std::vector<std::string> &args)
{
    List out;
    out.reserve(args.size());
    for (const std::string &arg : args)
        out.push_back(std::make_shared<const Value>(make_string(encode_raw(arg))));
    return out;
}

InterpResult run_program(const std::string &source,
                         const std::vector<std::string> &args,
                         const std::string &path, Console *console)
{
    InterpResult result;

    std::shared_ptr<Image> image = std::make_shared<Image>();
    image->loaded = load(source, path);
    if (!image->loaded.ok()) {
        report(result.output, image->loaded.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    image->resolved = resolve(image->loaded.program);
    if (!image->resolved.ok()) {
        report(result.output, image->resolved.errors, image->loaded.sources);
        result.status = 1;
        return result;
    }

    Evaluator evaluator(image->resolved, "main", false);
    evaluator.set_console(console);
    evaluator.run_entry(image->loaded.program, args_to_list(args));

    // Drained BEFORE the error report is built, and that order is the whole
    // reason drain() exists as a separate call rather than being folded into
    // the Console's destructor. A runtime error is reported "after whatever
    // output preceded it" — which is only true if the output has actually left
    // by the time the caller prints the report.
    if (console) {
        // An interrupted run stops PACING before it drains. The pace travels
        // with each queued line (§9), so a program that queued ten thousand
        // lines at 100 ms a line still owes seventeen minutes of terminal at
        // the moment Ctrl-C is pressed -- and a drain that honoured it would
        // make the interrupt look like it did nothing. The output is still
        // printed, in full: it was produced, so it is owed. Only the spacing
        // is dropped, and only for the run that was stopped.
        if (interrupt_requested())
            console->abandon_pace();
        console->drain();
    }

    result.output = evaluator.output();
    report(result.output, evaluator.errors(), image->loaded.sources);
    result.ok = evaluator.ok();
    result.status = status_of(evaluator.returned(), result.ok);

    // 130 for an interrupted run, which is 128 + SIGINT and what every shell
    // reports for a program stopped this way. It overrides the ordinary
    // failure status because "somebody stopped this" and "this went wrong" are
    // different facts, and a script that runs a satellite program is entitled
    // to tell them apart -- the same 130 the double-Ctrl-C escalation exits
    // with, so one key produces one number however far the walk got.
    if (interrupt_requested())
        result.status = INTERRUPT_EXIT_STATUS;

    if (!image->resolved.suits.empty())
        retain(std::move(image));
    return result;
}

InterpResult run_file(const std::string &path,
                      const std::vector<std::string> &args, Console *console)
{
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        InterpResult result;
        result.output = "satellite: cannot read " + path + "\n";
        result.status = 2;
        return result;
    }

    std::ostringstream buffer;
    buffer << in.rdbuf();

    // argv[0] is the program, so argz[0] is the script.
    std::vector<std::string> full;
    full.reserve(args.size() + 1);
    full.push_back(path);
    full.insert(full.end(), args.begin(), args.end());

    // The path goes to run_program as well as into argz, so an error names the
    // file it happened in instead of a bare line number. That is §16's payoff
    // arriving early: it needs no loader, only a Span that can carry a file id.
    return run_program(buffer.str(), full, path, console);
}

} // namespace satellite
