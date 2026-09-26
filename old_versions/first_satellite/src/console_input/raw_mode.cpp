// Raw mode, and the four paths that undo it. See raw_mode.hpp for what raw
// mode costs and why the restore is on four paths rather than one.

#include "console_input/raw_mode.hpp"
#include "system_facts/interrupt.hpp"

#include <atomic>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

namespace satellite {

namespace {

// The settings to go back to, captured the first time raw mode is entered.
// Written once under `raw` being false and read only when it is true, so the
// flag is the whole of the synchronisation.
struct termios cooked;

// Read by restore_terminal(), which the SIGINT handler calls. Lock-free for
// the reason interrupt.cpp gives about its own count: an atomic that might
// take a lock is one a signal handler must not touch.
std::atomic<bool> raw{false};
static_assert(std::atomic<bool>::is_always_lock_free,
              "restore_terminal() runs in a signal handler; no locks here");

// Registered once, so a normal exit(), a return from main() and an uncaught
// exception all put the terminal back. It does NOT cover _exit(2), which is
// the whole reason the emergency hook exists as well.
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

    // ICANON off is the one that matters: without it, read() returns bytes as
    // they are typed instead of waiting for a newline, which is the only way
    // an arrow key can be seen at all. ECHO off follows from it -- the editor
    // draws the line itself, and leaving the kernel to echo as well would
    // print every character twice.
    //
    // ISIG off is what turns Ctrl-C into the byte 0x03 rather than a SIGINT,
    // and that is deliberate: at a prompt, Ctrl-C should abandon the line
    // being typed, not the session. The signal half is for a program that is
    // RUNNING, when the terminal is cooked again -- see interrupt.hpp.
    //
    // IEXTEN off stops Ctrl-V from being read as "quote the next character" by
    // the driver before this code sees it.
    rawattr.c_lflag &= ~(unsigned)(ICANON | ECHO | ISIG | IEXTEN);

    // IXON off frees Ctrl-S and Ctrl-Q, which otherwise stop and start the
    // terminal and look exactly like a hang. ICRNL off is what keeps Ctrl-M
    // and Enter distinguishable at the byte level; the decoder reads both as
    // Enter, so nothing above notices, and the driver is no longer rewriting
    // input on the way past. BRKINT and ISTRIP are the rest of the classic
    // set: a break should not raise SIGINT here, and the eighth bit of every
    // byte is UTF-8 and must survive.
    rawattr.c_iflag &= ~(unsigned)(IXON | ICRNL | BRKINT | ISTRIP | INPCK);

    // OPOST stays ON, which is where this departs from the textbook raw mode.
    // Turning it off means a '\n' no longer implies a carriage return, so
    // every newline this process writes -- including a satellite program's
    // whole output, which is not this module's to reformat -- would have to
    // become "\r\n" by hand. The prompt is the only thing drawn here and it
    // manages its own carriage returns.

    // One byte at a time, and block until there is one. VTIME 0 with VMIN 1 is
    // the combination that makes read() a blocking read of exactly what was
    // typed, which is what the decoder wants: no timeout means no clock in the
    // decoding, which is what keeps it testable from a string.
    rawattr.c_cc[VMIN] = 1;
    rawattr.c_cc[VTIME] = 0;

    // TCSAFLUSH rather than TCSANOW: anything typed ahead while the previous
    // line was being evaluated is discarded rather than replayed into a
    // prompt it was not typed at.
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
    // exchange, so two paths racing to restore -- the destructor and the
    // atexit handler on the way out of main -- do it once. The loser doing it
    // twice would be harmless; doing it once is simply the truth.
    if (!raw.exchange(false, std::memory_order_relaxed))
        return;
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &cooked);
}

} // namespace satellite
