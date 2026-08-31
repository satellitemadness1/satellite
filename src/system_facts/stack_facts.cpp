// The stack, from both ends: what this thread is standing on now, and how far
// RLIMIT_STACK lets it grow. See system_facts/facts.hpp.
//
// PORTED FROM THE FIRST SATELLITE'S stack_facts.cpp, 52 lines, whole and
// unchanged in behaviour. It is the one of the three files that needed nothing
// done to it, and that is because it asks the kernel a question with no policy
// in it: getrlimit and pthread_attr_getstack each have exactly one answer.
//
// NOTHING AT M6 RECURSES, AND THAT IS WHY THIS IS HERE. PLAN M6's own argument
// for landing before M7 is that "M9 bounds recursion, and DESIGN §7.5 derives
// that ceiling from RLIMIT_STACK rather than fixing it -- which is
// stack_facts.cpp's stack_limit_bytes()". `satl --limits` prints both numbers,
// so the reader lands with a consumer rather than waiting three milestones for
// one, which is the rule PLAN M2 set and this tree has kept since.

#include "system_facts/facts.hpp"

#include <pthread.h>
#include <sys/resource.h>

namespace satellite::facts {

bool thread_stack_bytes(unsigned long long *used, unsigned long long *total)
{
    pthread_attr_t attributes;
    if (pthread_getattr_np(pthread_self(), &attributes) != 0)
        return false;

    void *base = nullptr;
    size_t size = 0;
    const int got = pthread_attr_getstack(&attributes, &base, &size);
    pthread_attr_destroy(&attributes);
    if (got != 0 || !base || size == 0)
        return false;

    // THE STACK GROWS DOWN on every platform this runs on, so what is in use is
    // the distance from the top of the region to where we are standing. A
    // local's address is the closest thing to a stack pointer that is legal to
    // ask for in C++, and it is a frame or two low -- a handful of bytes against
    // a figure reported in megabytes.
    char here = 0;
    const char *top = static_cast<char *>(base) + size;
    const unsigned long long depth =
        static_cast<unsigned long long>(top - &here);
    if (used)
        *used = depth > size ? size : depth;
    if (total)
        *total = static_cast<unsigned long long>(size);
    return true;
}

unsigned long long stack_limit_bytes()
{
    struct rlimit limit;
    if (getrlimit(RLIMIT_STACK, &limit) != 0)
        return kStackLimitUnknown;
    if (limit.rlim_cur == RLIM_INFINITY)
        return kStackLimitUnknown;
    return static_cast<unsigned long long>(limit.rlim_cur);
}

} // namespace satellite::facts
