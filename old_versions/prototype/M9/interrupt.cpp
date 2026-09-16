// Ctrl-C / SIGINT interrupt subsystem implementation.
// Milestone 9 Prototype in prototype/M9.

#include "interrupt.hpp"

#include <atomic>
#include <csignal>
#include <cstring>
#include <unistd.h>

namespace satellite {

namespace {

std::atomic<unsigned> g_interrupts{0};
static_assert(std::atomic<unsigned>::is_always_lock_free,
              "SIGINT atomic flag must be lock-free");

std::atomic<bool> g_installed{false};

void sigint_handler(int)
{
    const unsigned prev = g_interrupts.fetch_add(1, std::memory_order_relaxed);
    if (prev >= 1) {
        const char msg[] = "\nSATELLITE: CTRL+C RECEIVED AGAIN: QUITTING NOW\n";
        (void)!write(STDERR_FILENO, msg, sizeof(msg) - 1);
        _exit(INTERRUPT_EXIT_STATUS);
    }
    const char msg[] = "\nSATELLITE: CTRL+C RECEIVED: QUITTING\n";
    (void)!write(STDERR_FILENO, msg, sizeof(msg) - 1);
}

} // namespace

void install_interrupt_handler()
{
    if (g_installed.exchange(true))
        return;

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Deliberately without SA_RESTART

    sigaction(SIGINT, &sa, nullptr);
}

bool interrupt_requested()
{
    return g_interrupts.load(std::memory_order_relaxed) != 0;
}

void clear_interrupt()
{
    g_interrupts.store(0, std::memory_order_relaxed);
}

void trigger_interrupt()
{
    g_interrupts.fetch_add(1, std::memory_order_relaxed);
}

unsigned interrupt_count()
{
    return g_interrupts.load(std::memory_order_relaxed);
}

} // namespace satellite

