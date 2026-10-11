#pragma once
// satellite/machine/stack_segments.hpp -- A CAPSULE THAT CALLS ITSELF IN THE MIDDLE NEVER RUNS
// OUT OF STACK. It moves onto a fresh one.
//
// THE AUTHOR, 2026-09-25, on the one limit left: a capsule calling itself mid-body "still works
// at like 1 million calls, it's the only limitation in the interpreter -- we could build code that
// ONLY applies to this special circumstance so the interpreter doesnt' crash ... it will only
// slow the interpreter down every 1 million calls, so that would be fine". This is that code.
// It overrules his ruling of 2026-09-22 ("specifically leave it broken"); the tail call that
// ruling built still runs in constant memory, untouched.
//
// NOT A COUNTER BUT A MEASURE. How deep a capsule may go depends on how big each level's frame
// is -- a capsule with ten variables in the middle of an expression takes more than one with
// none -- so counting calls would guess. Every capsule call instead asks how much of THIS
// stack is left: one address compared with one other (the_stack_floor()), on every call.
//
// WHEN LESS THAN kStackHeadroom IS LEFT, the call runs on a fresh segment of kSegmentBytes --
// ON THE SAME THREAD (getcontext/makecontext/swapcontext), so a lock it holds, the console it
// is printing to and every thread_local it reads are still its own -- and comes back when the
// call returns. A segment's memory is taken from the kernel only as it is touched
// (MAP_NORESERVE), and handed back when the deep call returns (MADV_DONTNEED), so a recursion
// a million deep costs the memory of a million frames while it is that deep, and nothing after.
// The one real limit is the machine's memory, which is the limit the language has always
// accepted (DESIGN 7.5). A C++ exception thrown on a segment is carried back across and thrown
// again on the stack it left, never let loose past a context switch.
//
// THE MAIN THREAD IS TAKEN AS 64 MiB DEEP, whatever its limit says. widen_the_stack() raises
// the limit after satl has started, and memory the kernel placed below the stack when it
// started can stop the stack growing into all of it -- so a floor worked out from the raised
// limit could promise stack that is not there. 64 MiB is always there (the kernel keeps at
// least 128 MiB below a new process's stack) and is still thousands of levels between moves.
// A program thread's stack is exact: pthread_getattr_np says where it ends.

#include <sys/mman.h>
#include <pthread.h>
#include <ucontext.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <cstddef>
#include <exception>
#include <functional>
#include <vector>

namespace satellite004 {

constexpr std::size_t kStackHeadroom = std::size_t(1) << 20;        // move before less than 1 MiB is left
constexpr std::size_t kSegmentBytes = std::size_t(256) << 20;       // 256 MiB a segment
constexpr std::size_t kMainThreadStack = std::size_t(64) << 20;     // what the main thread is taken to have

// THE LOWEST ADDRESS THIS THREAD'S CURRENT STACK MAY SAFELY REACH, worked out the first time it
// is asked on each thread, and moved while a call runs on a segment.
inline thread_local char *stack_floor_here = nullptr;

inline char *the_stack_floor()
{
    if (stack_floor_here != nullptr)
        return stack_floor_here;
    pthread_attr_t attributes;
    void *lowest = nullptr;
    std::size_t size = 0;
    if (pthread_getattr_np(pthread_self(), &attributes) == 0) {
        pthread_attr_getstack(&attributes, &lowest, &size);
        pthread_attr_destroy(&attributes);
    }
    char *const top = static_cast<char *>(lowest) + size;
    const bool main_thread = getpid() == static_cast<pid_t>(syscall(SYS_gettid));
    const std::size_t usable = main_thread && size > kMainThreadStack ? kMainThreadStack : size;
    stack_floor_here = lowest == nullptr ? nullptr : top - usable;
    return stack_floor_here;
}

// Whether the next call should move: false when the floor is unknown, which leaves a machine
// that will not say where its stack ends exactly where satl was before this file.
inline bool stack_is_running_low()
{
    char here;
    char *const floor = the_stack_floor();
    return floor != nullptr && &here - floor < static_cast<std::ptrdiff_t>(kStackHeadroom);
}

// THIS THREAD'S SEGMENTS, kept for reuse (a recursion that goes up and down across one depth
// would otherwise map and unmap a segment every time), and unmapped when the thread ends.
struct StackSegments {
    std::vector<char *> held;
    std::size_t in_use = 0;
    ~StackSegments()
    {
        for (char *each : held)
            munmap(each, kSegmentBytes);
    }
};
inline thread_local StackSegments stack_segments_here;

struct FreshStackCall {
    std::function<void()> body;
    std::exception_ptr failed;
    ucontext_t back{};
};
inline thread_local FreshStackCall *fresh_stack_call = nullptr;

inline void fresh_stack_start()
{
    FreshStackCall *const call = fresh_stack_call;
    try {
        call->body();
    } catch (...) {
        call->failed = std::current_exception();
    }
    // returning resumes uc_link, which is call->back
}

// Runs `body` on a fresh segment and comes back. With no memory left for another segment it
// runs `body` where it is -- as satl did before this file, and no worse.
template <typename Body>
inline void on_a_fresh_stack(Body &&body)
{
    StackSegments &segments = stack_segments_here;
    if (segments.in_use == segments.held.size()) {
        void *const memory = mmap(nullptr, kSegmentBytes, PROT_READ | PROT_WRITE,
                                  MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE | MAP_STACK, -1, 0);
        if (memory == MAP_FAILED) {
            body();
            return;
        }
        segments.held.push_back(static_cast<char *>(memory));
    }
    char *const segment = segments.held[segments.in_use++];

    FreshStackCall call;
    call.body = std::forward<Body>(body);
    FreshStackCall *const outer_call = fresh_stack_call;
    char *const outer_floor = the_stack_floor();
    fresh_stack_call = &call;
    stack_floor_here = segment + (std::size_t(64) << 10);       // a margin at the bottom of the segment

    ucontext_t there;
    getcontext(&there);
    there.uc_stack.ss_sp = segment;
    there.uc_stack.ss_size = kSegmentBytes;
    there.uc_link = &call.back;
    makecontext(&there, fresh_stack_start, 0);
    swapcontext(&call.back, &there);

    stack_floor_here = outer_floor;
    fresh_stack_call = outer_call;
    --segments.in_use;
    madvise(segment, kSegmentBytes, MADV_DONTNEED);             // the deep frames' memory, handed back
    if (call.failed)
        std::rethrow_exception(call.failed);
}

} // namespace satellite004
