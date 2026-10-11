// satellite/satellite_variable_window/console_feed.cpp -- FAST_PRINTING.md STEP 5: SATL'S OWN
// CONSOLE, FED STRAIGHT FROM THE DISPLAY THREAD. The author, 2026-09-26: "we have our own libvte
// window ... libvte is the very fastest option and we're crippling the entire libvte purpose as
// we have it currently", then "wire it into libvte as fast as we can get it to go into libvte".
//
// BEFORE, the display thread wrote the console's pty and VTE read it back on the desk: a kernel
// copy each way, a pty that holds about twelve kilobytes, and VTE reading at most
// m_max_input_bytes of it a frame -- so the display thread waited on the pty whenever VTE was
// behind. NOW the display thread hands its piece here, and the desk calls vte_terminal_feed().
//
// WHAT VTE 0.84 DOES WITH IT (vendor/vte's vte.cc and scheduler.cc, and the race in
// SCRATCH.md/FAST_PRINTING): vte_terminal_feed() only copies the bytes into VTE's queue, and VTE
// parses that queue ONCE A FRAME, all of it, in a tick callback -- whichever road the bytes came
// by. So the desk feeds once a frame too, on the frame clock's `before-paint`, which runs ahead
// of `update`, where VTE parses: what is fed in a frame is parsed in that frame. The race, 100,000
// lines: fed once a frame 1.04 us a line end to end, through the pty 1.96 us.
//
// THREE EXCEPTIONS TO ONCE A FRAME:
//   - A FLUSH WAITS: fed at once. Every std::cout.flush() -- before each prompt, each report,
//     satellite.access -- waits until VTE has been fed (printing_satellite.hpp), a frame is up to
//     16.7 ms, and a program flushing in a loop would otherwise run at sixty lines a second. The
//     flush says so itself (hurry), and a piece handed over while one waits says so too (take).
//   - NO FRAME COMES: GTK's frame clock may stop for a minimized window (scheduler.cc says so), so
//     what waits is fed after kNoFrame anyway -- VTE's own fallback, ten times a second.
//   - THE WINDOW IS NOT DRAWN YET, so it has no frame clock: fed at once.
//
// NOTHING OVERTAKES THE PTY. satl still writes the pty -- every report (std::cerr), the prompt's
// line, the listing's progress line, the console's hold message -- and VTE reads the pty on the desk
// at G_PRIORITY_DEFAULT_IDLE, BELOW the frame clock, so bytes written there before a display may
// still be unread when the display is fed, and would land after it. So a feed waits while the master still holds bytes
// (FIONREAD), and for kPtySettles after a direct write (the_pty_was_written_directly): a byte
// written to a pty's slave reaches the master's side only when the kernel's flip-buffer work runs,
// measured at up to ~45 us here, and FIONREAD cannot see it before. The other way round is the
// flush's: once it returns VTE has what came before, and a pty write after it queues behind -- and
// std::cerr flushes std::cout before every report.
//
// HOW MUCH A FRAME: VTE parses everything it was fed in one go, with no budget of its own for a
// feed -- a 100 MB display handed over whole would stop the window, Ctrl-C and the close button
// with it, for a second and a half (a fresh reader, 2026-09-26). Its pty was read at most
// m_max_input_bytes a frame, which VTE tunes so one frame's parsing takes about
// VTE_MAX_PROCESS_TIME, 100 ms (vte.cc's time_process_incoming). The feed keeps VTE's own rule: at
// most `budget` bytes between two frames, tuned the same way from how long each frame took.
//
// HOW FAR BEHIND IT MAY GET: a frame's budget waiting (or one piece bigger than that, alone). Past
// it the display thread waits, the printing satellite's pieces fill, and his buffer counts -- so a
// program that outruns VTE by 131,072 displays still stops with S840, as it did on the pty.
//
// CTRL-S STILL PAUSES IT: while the pty takes Ctrl-S as flow control, the keys that stop and start
// the pty stop and start the feed too (console_launch.cpp's the_keys).

#include "console_feed.hpp"

#if SATELLITE_HAS_CONSOLE

#include "../display/printing_satellite.hpp"

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib-unix.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <sys/eventfd.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

namespace satellite004 {
namespace {

using Clock = std::chrono::steady_clock;

// A FRAME'S BUDGET starts at 4 MiB -- about 60 ms of the ~66 MB/s VTE parsed in the race -- and is
// tuned toward kFrameWork a frame, between kLeastBudget and kMostBudget.
constexpr std::size_t kFirstBudget = 4 * 1024 * 1024;
constexpr std::size_t kLeastBudget = 64 * 1024;
constexpr std::size_t kMostBudget = 64 * 1024 * 1024;
constexpr double kFrameWorkMs = 100.0;                         // vtedefines.hh's VTE_MAX_PROCESS_TIME
constexpr guint kNoFrameMs = 100;                              // scheduler.cc's NEXT_UPDATE_USEC
constexpr guint kLookAgainMs = 1;
constexpr Clock::duration kPtySettles = std::chrono::milliseconds(1);
constexpr Clock::duration kGiveUpOnThePty = std::chrono::milliseconds(100);

// BETWEEN THE DISPLAY THREAD AND THE DESK, under feed_lock.
std::mutex feed_lock;
std::condition_variable room_for_more;
std::string waiting;                 // the bytes for the next feeds, from waiting_from on
std::size_t waiting_from = 0;        // what is before this was fed already
std::uint64_t waiting_through = 0;   // every job before this is in `waiting`, or fed already
bool called = false;                 // the desk has been woken for what waits
bool at_once = false;                // ...and a flush waits: feed without waiting for a frame
bool gone = true;                    // no console to feed: the display thread writes fd 1
int wake_fd = -1;                    // made once, on the desk; after that only written and read
std::atomic<std::size_t> budget{kFirstBudget};   // written on the desk, read by the display thread too
std::atomic<bool> stopped{false};    // Ctrl-S: nothing is fed until the pty flows again

std::size_t unfed() { return waiting.size() - waiting_from; }

// THE DESK'S OWN.
VteTerminal *terminal = nullptr;
int master = -1;
GdkFrameClock *clock = nullptr;      // held with a reference while its signals are connected
gulong before_paint = 0;
gulong after_paint = 0;
guint wake_watch = 0;
guint no_frame = 0;                  // the fallback when no frame comes
guint look_again = 0;                // the pty still held bytes: another look soon
bool asked = false;                  // a feed is owed at the next before-paint
std::size_t fed_this_frame = 0;      // bytes fed since VTE last parsed
bool feeding_the_frame = false;      // feed() is running in before-paint
std::size_t fed_at_before_paint = 0; // ...and fed this much there,
bool held_back_at_before_paint = false;   // ...and the budget held some back
Clock::time_point frame_began{};
Clock::time_point held_back_since{};
std::string feeding;

bool the_pty_may_hold_bytes()
{
    int unread = 0;
    if (master >= 0 && ioctl(master, FIONREAD, &unread) == 0 && unread > 0)
        return true;
    const std::int64_t written = when_the_pty_was_last_written_directly();
    return written != 0 && Clock::now().time_since_epoch().count() - written < kPtySettles.count();
}

// A PTY WITH IXON OFF IS NEVER STOPPED: the kernel starts one Ctrl-S stopped when IXON is turned
// off (n_tty_set_termios), as the prompt's raw mode does. A master answers with its slave's termios.
bool the_pty_takes_no_flow_control()
{
    struct termios now;
    return master >= 0 && tcgetattr(master, &now) == 0 && (now.c_iflag & IXON) == 0;
}

void feed();

gboolean look_again_now(gpointer)
{
    look_again = 0;
    feed();
    return G_SOURCE_REMOVE;
}

// NO FRAME CAME (a minimized window): VTE's own fallback parses ten times a second, and so a
// budget's worth is fed ten times a second.
gboolean no_frame_came(gpointer)
{
    no_frame = 0;
    fed_this_frame = 0;
    if (asked)
        feed();
    return G_SOURCE_REMOVE;
}

void before_paint_came(GdkFrameClock *, gpointer)
{
    frame_began = Clock::now();
    // FRAMES ARE COMING, so the fallback for none is not wanted; a feed that stops short asks again.
    if (no_frame != 0) {
        g_source_remove(no_frame);
        no_frame = 0;
    }
    if (asked) {
        feeding_the_frame = true;
        feed();
        feeding_the_frame = false;
    }
}

// VTE HAS PARSED WHAT THIS FRAME HAD (its `update` came between), so the next frame's budget
// starts again -- tuned as VTE tunes its pty's: what would have taken kFrameWorkMs at this frame's
// pace, averaged with what it was. ONLY ON WHAT THIS FRAME'S before-paint FED, AND ONLY WHEN THE
// BUDGET HELD SOME BACK: those bytes are certainly parsed in this frame, while bytes fed between
// frames may have been parsed already by VTE's own fallback, and timing a frame that had nothing
// left to parse against them sent the budget to its most (the second fresh reader, 2026-09-26). A
// frame of a few lines says nothing about how much a frame can take. And never more than double.
void after_paint_came(GdkFrameClock *, gpointer)
{
    const bool tune = held_back_at_before_paint && fed_at_before_paint > 0;
    const std::size_t bytes = fed_at_before_paint;
    fed_this_frame = 0;
    fed_at_before_paint = 0;
    held_back_at_before_paint = false;
    if (!tune)
        return;
    const double ms = std::max(1.0, std::chrono::duration<double, std::milli>(Clock::now() - frame_began).count());
    const double was = static_cast<double>(budget.load(std::memory_order_relaxed));
    const double target = static_cast<double>(bytes) * kFrameWorkMs / ms;
    const double tuned = std::min((was + target) / 2, 2 * was);
    budget.store(static_cast<std::size_t>(std::clamp(tuned, static_cast<double>(kLeastBudget),
                                                     static_cast<double>(kMostBudget))),
                 std::memory_order_relaxed);
}

// THE TERMINAL'S FRAME CLOCK, once it has one -- which is once the window is drawn.
bool a_frame_clock()
{
    if (clock == nullptr) {
        clock = gtk_widget_get_frame_clock(GTK_WIDGET(terminal));
        if (clock == nullptr)
            return false;
        g_object_ref(clock);
        before_paint = g_signal_connect(clock, "before-paint", G_CALLBACK(before_paint_came), nullptr);
        after_paint = g_signal_connect(clock, "after-paint", G_CALLBACK(after_paint_came), nullptr);
    }
    return true;
}

// THE REST AT THE NEXT FRAME -- or after kNoFrameMs, if none comes.
void ask_for_a_frame()
{
    asked = true;
    if (a_frame_clock())
        gdk_frame_clock_request_phase(clock, GDK_FRAME_CLOCK_PHASE_BEFORE_PAINT);
    if (no_frame == 0)
        no_frame = g_timeout_add(kNoFrameMs, no_frame_came, nullptr);
}

// WHAT WAITS, INTO VTE, up to what is left of this frame's budget; the jobs in it settled once all
// of it is in.
void feed()
{
    if (terminal == nullptr)
        return;
    if (stopped.load(std::memory_order_acquire)) {
        held_back_since = Clock::time_point{};   // the wait for the pty starts again after the hold
        if (!the_pty_takes_no_flow_control())
            return;
        stopped.store(false, std::memory_order_release);
    }
    if (the_pty_may_hold_bytes()) {
        const Clock::time_point now = Clock::now();
        if (held_back_since == Clock::time_point{})
            held_back_since = now;
        // BELOW VTE's PTY READER, so it reads first. A VTE that has not read its pty in
        // kGiveUpOnThePty is not waited on any longer: the program's lines are fed.
        if (now - held_back_since < kGiveUpOnThePty) {
            if (look_again == 0)
                look_again = g_timeout_add_full(G_PRIORITY_DEFAULT_IDLE + 10, kLookAgainMs, look_again_now, nullptr,
                                                nullptr);
            return;
        }
    }
    held_back_since = Clock::time_point{};
    const std::size_t frame = budget.load(std::memory_order_relaxed);
    if (fed_this_frame >= frame) {
        ask_for_a_frame();
        return;
    }
    const std::size_t allowed = frame - fed_this_frame;
    bool all = false;
    std::uint64_t through = 0;
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        if (unfed() <= allowed) {
            if (waiting_from == 0) {
                feeding.swap(waiting);
            } else {
                feeding.assign(waiting, waiting_from, std::string::npos);
                waiting.clear();
            }
            waiting_from = 0;
            through = waiting_through;
            called = false;
            at_once = false;
            all = true;
        } else {
            feeding.assign(waiting, waiting_from, allowed);
            waiting_from += allowed;
        }
    }
    room_for_more.notify_one();
    if (!feeding.empty())
        vte_terminal_feed(terminal, feeding.data(), static_cast<gssize>(feeding.size()));
    fed_this_frame += feeding.size();
    if (feeding_the_frame) {
        fed_at_before_paint += feeding.size();
        held_back_at_before_paint = held_back_at_before_paint || !all;
    }
    if (feeding.capacity() > kMostBudget)
        std::string().swap(feeding);
    else
        feeding.clear();
    if (!all) {
        ask_for_a_frame();
        return;
    }
    asked = false;
    if (no_frame != 0) {
        g_source_remove(no_frame);
        no_frame = 0;
    }
    display_fed_through(through);
}

// THE DISPLAY THREAD HANDED SOMETHING OVER: a frame asked for, or fed now.
gboolean woken(gint fd, GIOCondition, gpointer)
{
    std::uint64_t count = 0;
    if (read(fd, &count, sizeof count) < 0) {
    }
    bool now = false;
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        now = at_once;
    }
    if (now || !a_frame_clock())
        feed();
    else
        ask_for_a_frame();
    return G_SOURCE_CONTINUE;
}

void wake_the_desk()
{
    const std::uint64_t one = 1;
    if (write(wake_fd, &one, sizeof one) < 0) {
    }
}

// ON THE DISPLAY THREAD: a piece, "\r\n" already, and every job before `through` in it. The desk is
// woken once for whatever waits -- and again when a flush comes to wait on it. Whether a flush
// waits is asked HERE, under the lock hurry() takes too: a flush that begins as this piece is
// handed over is seen by one of the two. A piece bigger than a frame's budget waits until nothing
// else does, and goes in without a copy.
bool take(std::string &bytes, std::uint64_t through)
{
    bool wake = false;
    {
        std::unique_lock<std::mutex> hold(feed_lock);
        room_for_more.wait(hold, [&bytes] {
            return gone || unfed() == 0 || unfed() + bytes.size() <= budget.load(std::memory_order_relaxed);
        });
        if (gone)
            return false;
        if (unfed() == 0) {
            waiting.clear();
            waiting_from = 0;
            waiting.swap(bytes);
        } else {
            if (waiting_from != 0) {
                waiting.erase(0, waiting_from);
                waiting_from = 0;
            }
            waiting.append(bytes);
        }
        waiting_through = through;
        const bool a_flush_waits = display_a_flush_waits();
        wake = !called || (a_flush_waits && !at_once);
        called = true;
        at_once = at_once || a_flush_waits;
    }
    bytes.clear();
    if (wake)
        wake_the_desk();
    return true;
}

// ON THE INTERPRETER'S THREAD (or a program's), A FLUSH BEGINNING TO WAIT: what the desk holds is
// fed now. Nothing held: the pieces still on their way see the flush themselves, in take().
void hurry()
{
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        if (gone || !called || at_once)
            return;
        at_once = true;
    }
    wake_the_desk();
}

// ANY THREAD: the prompt took the pty raw, and the kernel started it if Ctrl-S had stopped it --
// so the feed goes on too. The desk is woken if something waits for it.
void the_pty_flows_again_here()
{
    if (!stopped.exchange(false, std::memory_order_acq_rel))
        return;
    bool wake = false;
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        wake = called && !gone;
    }
    if (wake)
        wake_the_desk();
}

} // namespace

void console_feed_starts(void *widget, int pty_master)
{
    if (wake_fd < 0)
        wake_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    if (wake_fd < 0 || widget == nullptr)
        return;   // NO EVENTFD: the pty carries the output, as it did before step 5
    terminal = VTE_TERMINAL(static_cast<GtkWidget *>(widget));
    master = pty_master;
    wake_watch = g_unix_fd_add(wake_fd, G_IO_IN, woken, nullptr);
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        gone = false;
        called = false;
        at_once = false;
        waiting.clear();
        waiting_from = 0;
    }
    display_goes_to_the_console(&take, &hurry, &the_pty_flows_again_here);
}

void console_feed_holds(bool held)
{
    if (terminal == nullptr || stopped.exchange(held, std::memory_order_acq_rel) == held)
        return;
    if (held)
        return;
    // CTRL-Q: what the stopped pty held goes on its way now too, and the kernel takes a moment to
    // pass it to VTE -- the fed lines wait for it, as for any direct write, from now.
    held_back_since = Clock::time_point{};
    the_pty_was_written_directly();
    ask_for_a_frame();
}

void console_feed_stops()
{
    if (terminal == nullptr)
        return;
    display_goes_to_the_console(nullptr, nullptr, nullptr);
    std::uint64_t through = 0;
    {
        const std::lock_guard<std::mutex> hold(feed_lock);
        gone = true;
        waiting.clear();
        waiting_from = 0;
        through = waiting_through;
        called = false;
        at_once = false;
    }
    room_for_more.notify_all();
    for (guint *source : {&wake_watch, &no_frame, &look_again}) {
        if (*source != 0)
            g_source_remove(*source);
        *source = 0;
    }
    if (clock != nullptr) {
        g_signal_handler_disconnect(clock, before_paint);
        g_signal_handler_disconnect(clock, after_paint);
        g_object_unref(clock);
        clock = nullptr;
    }
    terminal = nullptr;
    master = -1;
    asked = false;
    stopped.store(false, std::memory_order_release);
    fed_this_frame = 0;
    fed_at_before_paint = 0;
    held_back_at_before_paint = false;
    held_back_since = Clock::time_point{};
    // WHAT WAITED IS LET GO -- the window it was for is gone -- and a flush waiting on it goes on.
    display_fed_through(through);
}

} // namespace satellite004

#else

namespace satellite004 {

void console_feed_starts(void *, int) {}
void console_feed_holds(bool) {}
void console_feed_stops() {}

} // namespace satellite004

#endif
