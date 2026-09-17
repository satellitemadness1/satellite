// Raw mode, and the paths that undo it. See raw_mode.hpp.

#include "raw_mode.hpp"

#include <atomic>
#include <cstdlib>
#include <termios.h>
#include <unistd.h>

namespace satellite004::prompt {

namespace {

// The settings to go back to, captured the FIRST time raw mode is entered and
// never again: re-capturing after a failure that left the terminal raw would
// save the raw settings as the ones to restore. Capture once, restore many.
struct termios cooked;
std::atomic<bool> captured{false};

// Read by restore_terminal(), which may run in a signal handler.
std::atomic<bool> raw{false};
std::atomic<int> raw_in{-1};
std::atomic<int> raw_out{-1};
static_assert(std::atomic<bool>::is_always_lock_free && std::atomic<int>::is_always_lock_free,
              "restore_terminal() runs in a signal handler; no locks here");

std::atomic<bool> hooked{false};

constexpr char paste_on[] = "\033[?2004h";
constexpr char paste_off[] = "\033[?2004l";

void say(int out, const char *text, std::size_t size)
{
    while (size > 0) {
        const ssize_t wrote = write(out, text, size);
        if (wrote <= 0)
            return;
        text += wrote;
        size -= static_cast<std::size_t>(wrote);
    }
}

} // namespace

bool is_interactive(int in, int out)
{
    return isatty(in) != 0 && isatty(out) != 0;
}

RawMode::RawMode(int in, int out)
{
    if (!is_interactive(in, out))
        return;

    if (!captured.load(std::memory_order_relaxed)) {
        if (tcgetattr(in, &cooked) != 0)
            return;
        captured.store(true, std::memory_order_relaxed);
    }
    if (!hooked.exchange(true))
        std::atexit(restore_terminal);

    struct termios attributes = cooked;

    // ICANON off  bytes arrive as they are typed, not a line at a time
    // ECHO off    render.cpp draws the line; the driver must not draw it too
    // ISIG off    Ctrl-C at the prompt is the byte 0x03
    // IEXTEN off  the driver no longer eats Ctrl-V as literal-next
    attributes.c_lflag &= ~static_cast<tcflag_t>(ICANON | ECHO | ISIG | IEXTEN);

    // IXON off    Ctrl-S and Ctrl-Q are keys, not flow control
    // ICRNL off   Enter (CR) stays apart from Ctrl-J (LF)
    // BRKINT, ISTRIP, INPCK off: eight clean bits, no break as an interrupt
    attributes.c_iflag &= ~static_cast<tcflag_t>(IXON | ICRNL | BRKINT | ISTRIP | INPCK);

    // OPOST STAYS ON, unlike the usual recipe: a program's '\n' must still become
    // CR LF, or its output staircases down the screen. The prompt owns input.
    attributes.c_cc[VMIN] = 1;
    attributes.c_cc[VTIME] = 0;

    // THE FLAG FIRST, THE TERMINAL SECOND (raw_mode.hpp): a handler that lands
    // between the two restores a terminal that is still cooked, which is harmless.
    raw_in.store(in, std::memory_order_relaxed);
    raw_out.store(out, std::memory_order_relaxed);
    raw.store(true, std::memory_order_relaxed);
    if (tcsetattr(in, TCSANOW, &attributes) != 0) {
        raw.store(false, std::memory_order_relaxed);
        return;
    }
    say(out, paste_on, sizeof paste_on - 1);
    active_ = true;
}

RawMode::~RawMode()
{
    if (active_)
        restore_terminal();
}

void restore_terminal()
{
    if (!raw.load(std::memory_order_relaxed))
        return;
    say(raw_out.load(std::memory_order_relaxed), paste_off, sizeof paste_off - 1);
    if (captured.load(std::memory_order_relaxed))
        tcsetattr(raw_in.load(std::memory_order_relaxed), TCSANOW, &cooked);
    raw.store(false, std::memory_order_relaxed);
}

} // namespace satellite004::prompt
