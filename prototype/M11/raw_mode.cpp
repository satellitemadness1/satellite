// Raw mode, and the four paths that undo it.
// Milestone 11 Prototype in prototype/M11.

#include "raw_mode.hpp"
#include "interrupt.hpp"

#include <atomic>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

namespace satellite {

namespace {

// The settings to go back to, captured the first time raw mode is entered.
struct termios cooked;

// Read by restore_terminal(). Lock-free atomic for signal safety.
std::atomic<bool> raw{false};
static_assert(std::atomic<bool>::is_always_lock_free,
              "restore_terminal() runs in a signal handler; no locks here");

// Registered once, so normal exit(), return from main(), and emergency hook undo raw mode.
std::atomic<bool> hooked{false};

void install_hooks_once()
{
    if (hooked.exchange(true))
        return;
    std::atexit(restore_terminal);
    set_emergency_exit_hook(restore_terminal);
}

} // namespace

bool prompt_is_interactive()
{
    return isatty(STDIN_FILENO) && isatty(STDOUT_FILENO);
}

RawMode::RawMode()
{
    if (!prompt_is_interactive())
        return;
    if (tcgetattr(STDIN_FILENO, &cooked) != 0)
        return;

    install_hooks_once();

    struct termios rawattr = cooked;

    // ICANON off: byte-at-a-time input
    // ECHO off: editor manages display
    // ISIG off: Ctrl-C becomes byte 0x03 at prompt instead of killing session
    // IEXTEN off: stops literal-next-character driver handling
    rawattr.c_lflag &= ~(unsigned)(ICANON | ECHO | ISIG | IEXTEN);

    // IXON off: frees Ctrl-S / Ctrl-Q
    // ICRNL off: preserves Enter and Ctrl-M distinctness
    // BRKINT, ISTRIP, INPCK off: clean byte transmission
    rawattr.c_iflag &= ~(unsigned)(IXON | ICRNL | BRKINT | ISTRIP | INPCK);

    // OPOST stays ON: so '\n' behaves normally in program output
    // VMIN = 1, VTIME = 0: blocking single-byte read
    rawattr.c_cc[VMIN] = 1;
    rawattr.c_cc[VTIME] = 0;

    // TCSAFLUSH: flush typed-ahead input from previous execution
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &rawattr) != 0)
        return;

    raw.store(true, std::memory_order_relaxed);
    active_ = true;
}

RawMode::~RawMode()
{
    if (active_)
        restore_terminal();
}

void restore_terminal()
{
    if (!raw.exchange(false, std::memory_order_relaxed))
        return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

} // namespace satellite

