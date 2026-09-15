// Raw mode, and the four paths that undo it. See satellite_prompt/raw_mode.hpp.

#include "satellite_prompt/raw_mode.hpp"

#include "system_facts/interrupt.hpp"

#include <atomic>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

namespace satellite::prompt {

namespace {

// The settings to go back to, captured the FIRST time raw mode is entered and
// never re-captured. Re-capturing on every entry would, after any failure that
// left the terminal raw, save the RAW settings as the ones to restore -- so the
// second line read would make the damage permanent. Capture once, restore many.
struct termios cooked;
std::atomic<bool> captured{false};

// Read by restore_terminal(), which may run in a signal handler.
std::atomic<bool> raw{false};
static_assert(std::atomic<bool>::is_always_lock_free,
              "restore_terminal() runs in a signal handler; no locks here");

std::atomic<bool> hooked{false};

// PATHS 2 AND 3, REGISTERED ONCE AND NEVER REMOVED. Both are idempotent because
// restore_terminal() exchanges the flag, so a process that exits normally after
// the watchdog has already restored does not call tcsetattr twice.
void install_hooks_once()
{
    if (hooked.exchange(true))
        return;
    std::atexit(restore_terminal);
    set_emergency_exit_hook(restore_terminal);
}

} // namespace

bool is_interactive()
{
    return isatty(STDIN_FILENO) != 0 && isatty(STDOUT_FILENO) != 0;
}

RawMode::RawMode()
{
    if (!is_interactive())
        return;

    if (!captured.load(std::memory_order_relaxed)) {
        if (tcgetattr(STDIN_FILENO, &cooked) != 0)
            return;
        captured.store(true, std::memory_order_relaxed);
    }

    install_hooks_once();

    struct termios raw_attributes = cooked;

    // ICANON off -- bytes arrive as they are typed rather than a line at a time
    // ECHO   off -- render.cpp draws the line; the driver must not draw it too
    // ISIG   off -- DESIGN §10.2: Ctrl-C becomes the byte 0x03 at the prompt
    // IEXTEN off -- stops the driver eating Ctrl-V as literal-next
    raw_attributes.c_lflag &=
        ~static_cast<tcflag_t>(ICANON | ECHO | ISIG | IEXTEN);

    // IXON   off -- frees Ctrl-S and Ctrl-Q, which are keys and not flow control
    // ICRNL  off -- keeps Enter (0x0D) distinct from Ctrl-J (0x0A)
    // BRKINT, ISTRIP, INPCK off -- eight clean bits, no break-as-interrupt
    raw_attributes.c_iflag &=
        ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | ISTRIP | INPCK);

    // OPOST STAYS ON, WHICH IS A DEPARTURE FROM THE USUAL RECIPE AND IS
    // DELIBERATE. Every "enter raw mode" example turns output post-processing
    // off too, and that would break the one thing this tree already has: the
    // Console's printer thread (M10) writes '\n' and expects the driver to turn
    // it into CR-LF. With OPOST off, a program's output staircases down the
    // screen. The prompt owns INPUT here; it does not own the console.
    raw_attributes.c_cc[VMIN] = 1;
    raw_attributes.c_cc[VTIME] = 0;

    // TCSAFLUSH -- discard whatever was typed while the last program was
    // running. A user who pressed keys during a slow run did not mean them as
    // input to the next prompt line.
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw_attributes) != 0)
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
    if (captured.load(std::memory_order_relaxed))
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

} // namespace satellite::prompt
