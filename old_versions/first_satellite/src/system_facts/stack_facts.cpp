// The stack, from both ends: what THIS thread is standing on right now, and
// how far RLIMIT_STACK lets it grow. system.hpp records why the stack is the
// only per-thread memory figure worth reporting, and why an unlimited limit is
// not treated as unbounded.
//
// Part of src/system_facts/, split from a 653-line system.cpp. See
// system_internal.hpp for what the pieces share and why system.cpp kept what
// it kept.

#include "system_facts/system_internal.hpp"

namespace satellite {

bool thread_stack_bytes(unsigned long long *used, unsigned long long *total)
{
    pthread_attr_t attr;
    if (pthread_getattr_np(pthread_self(), &attr) != 0)
        return false;
    void *base = nullptr;
    size_t size = 0;
    const int got = pthread_attr_getstack(&attr, &base, &size);
    pthread_attr_destroy(&attr);
    if (got != 0 || !base || size == 0)
        return false;

    // The stack grows DOWN on every platform this runs on, so what is in use
    // is the distance from the top of the region to where we are standing. A
    // local's address is the closest thing to a stack pointer that is legal to
    // ask for in C++, and it is a frame or two low -- which is a handful of
    // bytes against a figure reported in megabytes.
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
        return STACK_LIMIT_UNKNOWN;
    if (limit.rlim_cur == RLIM_INFINITY)
        return STACK_LIMIT_UNKNOWN;
    return static_cast<unsigned long long>(limit.rlim_cur);
}

} // namespace satellite
