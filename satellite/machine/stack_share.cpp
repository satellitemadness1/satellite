// satellite/machine/stack_share.cpp -- see stack_share.hpp.

#include "stack_share.hpp"

#include <unistd.h>

namespace satellite004 {
namespace {

struct rlimit stack_satl_was_given{};
bool stack_was_widened = false;

// arguments.memory.total's own figure (arguments.cpp), so the share and the row a
// program reads are the same number. 0 when the machine will not say, which lands
// on the floor.
unsigned long long int memory_total()
{
    const long page = sysconf(_SC_PAGESIZE);
    const long pages = sysconf(_SC_PHYS_PAGES);
    if (page <= 0 || pages <= 0)
        return 0;
    return static_cast<unsigned long long int>(pages) * static_cast<unsigned long long int>(page);
}

} // namespace

void widen_the_stack()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) != 0)
        return;
    stack_satl_was_given = limit;

    rlim_t want = static_cast<rlim_t>(stack_share_of(memory_total()));
    if (limit.rlim_cur == RLIM_INFINITY || limit.rlim_cur >= want)
        return;
    // CLAMPED TO THE HARD LIMIT RATHER THAN REFUSED BY IT: asking past it is a
    // certain EINVAL, and the point is to take what there is.
    if (limit.rlim_max != RLIM_INFINITY && want > limit.rlim_max)
        want = limit.rlim_max;
    if (want <= limit.rlim_cur)
        return;
    limit.rlim_cur = want;
    stack_was_widened = setrlimit(RLIMIT_STACK, &limit) == 0;
}

void hand_a_child_the_stack_satl_was_given()
{
    if (stack_was_widened)
        setrlimit(RLIMIT_STACK, &stack_satl_was_given);
}

} // namespace satellite004
