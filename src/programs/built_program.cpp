// The four passes, in order. See programs/built_program.hpp.

#include "programs/built_program.hpp"

#include "error_reporter/report.hpp"
#include "machine_limits/limits.hpp"
#include "programs/check_command.hpp"

#include <cstdio>

namespace satellite {

bool build_program(const std::string &path, Built &out)
{
    if (!open_source(path, out.text))
        return false;
    out.opened = true;

    const errors::Source against{path, out.text, &out.words};

    out.parsed = parse(out.text, out.words);
    if (!out.parsed.errors.empty())
        fputs(errors::render(out.parsed.errors, against).c_str(), stderr);
    if (!out.parsed.ok())
        return false;

    out.resolved = resolve::resolve(out.parsed.ast, out.words);
    if (!out.resolved.problems.empty())
        fputs(errors::render(out.resolved.problems, against).c_str(), stderr);
    if (!out.resolved.ok())
        return false;

    out.program = eval::compile(out.parsed.ast, out.resolved, out.words);
    if (!out.program.problems.empty())
        fputs(errors::render(out.program.problems, against).c_str(), stderr);
    out.ok = out.program.ok();
    return out.ok;
}

eval::Policy policy_from_the_limits()
{
    eval::Policy policy;
    policy.max_depth = limits::max_depth_bytes();
    policy.division_digits = limits::division_digits();
    return policy;
}

} // namespace satellite
