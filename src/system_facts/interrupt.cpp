// SIGINT: the handler, the flag, and the escalation. See interrupt.hpp for
// why this is in system_facts and what may safely happen inside a handler --
// this file is v1's, ported at M11 close to unchanged, because every line of
// it was hard-won there.

#include "system_facts/interrupt.hpp"

#include <atomic>
#include <csignal>
#include <cstring>
#include <unistd.h>

namespace satellite {

namespace {

// One of the two things C++ lets a signal handler touch, and the static_assert
// is what makes that true rather than assumed: an atomic that is not ALWAYS
// lock-free may take a lock, and a lock taken in a handler on a thread that
// already holds it is a deadlock with no stack to read.
std::atomic<unsigned> interrupts{0};
static_assert(std::atomic<unsigned>::is_always_lock_free,
              "the SIGINT handler stores into this; it must not take a lock");

// Whether the handler is already installed, so a second
// install_interrupt_handler() is free. Every entry point that can run a
// program calls it, and today that is `satl file.satl` and `satl --call`; M22's
// prompt is the third.
std::atomic<bool> installed{false};

// The one thing to run before an _exit(2). See interrupt.hpp. A bare function
// pointer in an atomic, because the SIGINT handler dereferences it: a load and
// an indirect call are signal-safe where a std::function's storage is not.
std::atomic<void (*)()> emergency_hook{nullptr};
static_assert(std::atomic<void (*)()>::is_always_lock_free,
              "the SIGINT handler reads this; it must not take a lock");

// Saying so out loud is the point, and it goes to STDERR rather than stdout
// for a reason worth stating: a run whose stdout is redirected -- piped, or
// sent to a file, or read by a program in another terminal -- would otherwise
// swallow the one line that explains why it stopped. stderr is the channel
// that stays with the person, so the courtesy actually reaches them.
//
// write(2) and not fprintf: the handler may have interrupted a thread that is
// inside stdio holding its lock, and printf would then deadlock rather than
// print. The return value is deliberately ignored -- there is nothing useful
// to do about a failed write on the way out -- and (void) says so to -Wall.
void say(const char *text)
{
    (void)!write(STDERR_FILENO, text, std::strlen(text));
}

void handler(int)
{
    // fetch_add returns the value BEFORE the add, so `previous` is how many
    // had already arrived. One already there means this is the second.
    const unsigned previous = interrupts.fetch_add(1, std::memory_order_relaxed);

    if (previous >= 1) {
        // The first one did not land, which means the walk is somewhere with
        // no statement boundary in it. Nothing here can unwind that, so leave.
        //
        // The hook first, because a raw terminal outliving the process is the
        // one failure the user cannot recover from without typing `reset`
        // blind.
        run_emergency_exit_hook();
        say("\nSATELLITE: CTRL+C RECEIVED AGAIN: QUITTING NOW\n");
        _exit(INTERRUPT_EXIT_STATUS);
    }

    say("\nSATELLITE: CTRL+C RECEIVED: QUITTING\n");
}

} // namespace

void install_interrupt_handler()
{
    // exchange rather than load-then-store: two threads calling this at once
    // would both see false and both install, which is harmless but makes the
    // idempotence a coincidence rather than a property.
    if (installed.exchange(true))
        return;

    struct sigaction action;
    std::memset(&action, 0, sizeof action);
    action.sa_handler = handler;
    sigemptyset(&action.sa_mask);

    // NO SA_RESTART, deliberately. See interrupt.hpp: a blocked read has to
    // come back with EINTR so that M14's `satellite.console.input` can tell a
    // Ctrl-C from a closed stdin, and so that a program waiting for a line can
    // be interrupted at all.
    action.sa_flags = 0;

    sigaction(SIGINT, &action, nullptr);
}

bool interrupt_requested()
{
    return interrupts.load(std::memory_order_relaxed) != 0;
}

unsigned interrupt_count()
{
    return interrupts.load(std::memory_order_relaxed);
}

void clear_interrupt()
{
    interrupts.store(0, std::memory_order_relaxed);
}

void set_emergency_exit_hook(void (*hook)())
{
    emergency_hook.store(hook, std::memory_order_relaxed);
}

void run_emergency_exit_hook()
{
    if (void (*hook)() = emergency_hook.load(std::memory_order_relaxed))
        hook();
}

} // namespace satellite
