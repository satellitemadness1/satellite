// Ctrl-C / SIGINT interrupt subsystem implementation.
// Milestone 11 Prototype in prototype/M11.

#include "interrupt.hpp"

#include <csignal>
#include <cstring>
#include <unistd.h>

namespace satellite {

namespace {

std::atomic<bool> g_interrupt_requested{false};
std::atomic<unsigned> g_interrupt_count{0};
std::atomic<EmergencyHookFn> g_emergency_hook{nullptr};

void sigint_handler(int /*sig*/)
{
    g_interrupt_requested.store(true, std::memory_order_relaxed);
    g_interrupt_count.fetch_add(1, std::memory_order_relaxed);
}

} // namespace

void install_interrupt_handler()
{
    static bool installed = false;
    if (installed) return;
    installed = true;

    struct sigaction sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; // Explicitly NO SA_RESTART

    sigaction(SIGINT, &sa, nullptr);
}

bool interrupt_requested()
{
    return g_interrupt_requested.load(std::memory_order_relaxed);
}

void clear_interrupt()
{
    g_interrupt_requested.store(false, std::memory_order_relaxed);
}

void trigger_interrupt()
{
    g_interrupt_requested.store(true, std::memory_order_relaxed);
    g_interrupt_count.fetch_add(1, std::memory_order_relaxed);
}

unsigned interrupt_count()
{
    return g_interrupt_count.load(std::memory_order_relaxed);
}

void set_emergency_exit_hook(EmergencyHookFn hook)
{
    g_emergency_hook.store(hook, std::memory_order_relaxed);
}

void run_emergency_exit_hook()
{
    EmergencyHookFn hook = g_emergency_hook.load(std::memory_order_relaxed);
    if (hook) {
        hook();
    }
}

} // namespace satellite

