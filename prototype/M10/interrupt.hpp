#pragma once

// Ctrl-C / SIGINT interrupt subsystem -- Milestone 10 Prototype.
//
// Installed without SA_RESTART so a running loop or long walk stops at the next
// statement and reports its state honestly.

#include <atomic>
#include <cstdint>

namespace satellite {

// Installs SIGINT signal handler (idempotent, no SA_RESTART).
void install_interrupt_handler();

// Returns true if a SIGINT has arrived and has not yet been cleared.
bool interrupt_requested();

// Forgets any interrupt signal received so far.
void clear_interrupt();

// Manually signal an interrupt (useful for testing and deterministic cancellation).
void trigger_interrupt();

// Number of interrupt signals received.
unsigned interrupt_count();

constexpr int INTERRUPT_EXIT_STATUS = 130;

} // namespace satellite

