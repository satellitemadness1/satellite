// The four passes, in order. See programs/built_program.hpp.

#include "programs/built_program.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"
#include "programs/check_command.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>

namespace satellite {

bool build_program(const std::string &path, Built &out)
{
    if (!open_source(path, out.text))
        return false;
    out.opened = true;
    return build_source(path, out, true);
}

// THE SAME FOUR PASSES OVER TEXT THAT CAME FROM SOMEWHERE ELSE -- M22's prompt,
// which builds a program in memory out of what somebody typed and never puts it
// on a disk. SPLIT OUT RATHER THAN COPIED for this file's own stated reason:
// the ORDER of the passes is decided in one place, and a second runner that
// resolved a tree that did not parse would bury the one real mistake under
// twenty carets. `name` is what a diagnostic is headed with -- a path for a
// file, `<prompt>` for a typed line.
//
// `report` IS FALSE FOR THE PROMPT AND THE REASON IS A LINE NUMBER. A typed line
// is wrapped in a capsule before it is a program, so a mistake on what the user
// sees as line 1 sits on line 4 of the text these passes are given. The prompt
// therefore takes the diagnostics rather than the printing, rebases their line
// numbers onto the line the person typed, and renders them itself -- which it
// can only do if nothing has printed them already. Every other caller wants the
// printing, and passes true.
bool build_source(const std::string &name, Built &out, bool report)
{
    const errors::Source against{name, out.text, &out.words};

    out.parsed = parse(out.text, out.words);
    if (report && !out.parsed.errors.empty())
        fputs(errors::render(out.parsed.errors, against).c_str(), stderr);
    if (!out.parsed.ok())
        return false;

    out.resolved = resolve::resolve(out.parsed.ast, out.words);
    if (report && !out.resolved.problems.empty())
        fputs(errors::render(out.resolved.problems, against).c_str(), stderr);
    if (!out.resolved.ok())
        return false;

    out.program = eval::compile(out.parsed.ast, out.resolved, out.words);
    if (report && !out.program.problems.empty())
        fputs(errors::render(out.program.problems, against).c_str(), stderr);
    out.ok = out.program.ok();
    return out.ok;
}

eval::Policy policy_from_the_limits()
{
    eval::Policy policy;
    policy.max_depth = limits::max_depth_bytes();
    policy.division_digits = limits::division_digits();
    policy.float_digits = limits::float_digits();

    // THE THIRD HAND-IN IS A FUNCTION AND NOT A NUMBER -- M11. The machine
    // asks it at every statement boundary, and wiring it here rather than
    // inside the evaluator is the same seam the two numbers above keep:
    // `satl`'s runs listen for Ctrl-C, tests/eval_test listens to whatever
    // its fixture hands in, and the evaluator includes neither module.
    policy.interrupted = interrupt_requested;
    return policy;
}

} // namespace satellite
