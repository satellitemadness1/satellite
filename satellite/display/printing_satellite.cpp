// satellite/display/printing_satellite.cpp -- the printing satellite, the display thread, and
// the ring between the interpreter and them. printing_satellite.hpp is the design and the
// author's words; this is the machinery, and every choice in it answers a measurement from
// SCRATCH.md/FAST_PRINTING.md and DISPLAY_THREADS.md.
//
// ONE FRESH READER WENT THROUGH IT BEFORE IT WAS COMMITTED (2026-09-26) and found eleven
// things, every one fixed here: a styled line overtaking std::cout's bytes, an overrun that
// left the printing satellite dropping for the rest of a session, a flush after an overrun
// that did not wait for the lines already on their way, SIGTTOU still blocked on the display
// thread, Ctrl-C at the prompt leaving 131,072 lines to scroll, a count of finished jobs that
// could stop short and hang a flush, a wake-up lost between two flushers, out of memory made
// permanent and leaving half a line, std::cout pointing into freed memory if one allocation
// failed, a membarrier failure fenced on one side only, and oversized buffers kept for good.

#include "printing_satellite.hpp"

#include "../bytecode/console_style.hpp"
#include "../machine/console_lock.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/thread_stop.hpp"
#include "../satellite_variable_infinity/satellite_infinity.hpp"
#include "../satellite_variable_number/number_conversions.hpp"

#include <atomic>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <cstring>
#include <deque>
#include <mutex>
#include <new>
#include <poll.h>
#include <pthread.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <utility>

#if defined(__linux__)
#include <linux/membarrier.h>
#include <sys/syscall.h>
#endif

namespace satellite004 {
namespace {

using Clock = std::chrono::steady_clock;

// THE HAND-OFF RING. 1,024 displays can be on their way before the interpreter would wait for
// room -- which only happens if the printing satellite is not being run at all, because it
// takes every job at once, into his buffer if the screen is behind.
constexpr std::size_t kSlots = 1024;
static_assert((kSlots & (kSlots - 1)) == 0, "a power of two");
// A PIECE is what one write() takes. The printing satellite hands one over when it reaches
// 64 KiB, or when nothing new has come for kHandAfter, or when somebody is waiting for a flush.
constexpr std::size_t kPieceBytes = 64 * 1024;
constexpr std::size_t kPieces = 4;                  // being filled, waiting, being written
constexpr auto kHandAfter = std::chrono::microseconds(10);
// HOW LONG EACH LOOKS FOR WORK BEFORE IT SLEEPS. Waking a sleeping thread cost the interpreter
// ~420 ns a display in the first attempt (DISPLAY_THREADS.md), so the printing satellite keeps
// looking long enough that a loop printing a line every few microseconds never finds it asleep.
// The display thread is woken by the printing satellite, never by the interpreter.
constexpr auto kPrinterLooks = std::chrono::microseconds(50);
constexpr auto kDisplayLooks = std::chrono::microseconds(20);
// AFTER AN OVERRUN, how long a flush waits on a screen that has stopped taking text.
constexpr auto kScreenStopped = std::chrono::seconds(1);
// A buffer grown past this by one huge line is given back once that line is written.
constexpr std::size_t kKeepAtMost = 4 * kPieceBytes;
// satl widens its own stack limit to gigabytes (machine/stack_share.hpp), which a thread would
// otherwise take as its own size.
constexpr std::size_t kThreadStack = 1024 * 1024;

enum class JobKind : unsigned char { value, bytes };

struct Job {
    Value value;             // a plain display's finished value
    std::string bytes;       // or bytes already made: std::cout's, a styled display's line
    JobKind kind = JobKind::bytes;
};

// ONE THREAD'S SLEEP -- or several threads', for a flush -- AND THE BELL THAT ENDS IT. `asleep`
// counts the sleepers, so one that wakes cannot say "nobody sleeps" over another that has just
// gone to sleep. It is read by the waker on every hand-off, so it has a cache line of its own
// that is written only when somebody sleeps.
struct Sleeper {
    alignas(64) std::atomic<std::uint32_t> asleep{0};
    std::atomic<std::uint32_t> bell{0};
};

// THE BARRIER, PAID BY THE ONE GOING TO SLEEP AND NEVER BY THE INTERPRETER. A sleeper must not
// miss a hand-off made while it decided to sleep: it says "asleep" and looks again for work,
// while the interpreter publishes work and looks for "asleep" -- and without a full barrier on
// BOTH sides each can miss the other's store. A fence on every display was the interpreter's
// cost to pay (tens of nanoseconds, the price of a whole conversion); membarrier makes every
// thread of this process pass a full barrier inside the sleeper's one system call instead, so
// the interpreter's side is a plain store and a plain load. Where membarrier is not there (an
// old kernel, another system), or fails, both sides take the fence from then on.
std::atomic<bool> heavy_barrier_works{false};

void register_the_heavy_barrier()
{
#if defined(__linux__) && defined(MEMBARRIER_CMD_PRIVATE_EXPEDITED)
    const long offered = syscall(SYS_membarrier, MEMBARRIER_CMD_QUERY, 0, 0);
    if (offered > 0 && (offered & MEMBARRIER_CMD_PRIVATE_EXPEDITED) != 0 &&
        syscall(SYS_membarrier, MEMBARRIER_CMD_REGISTER_PRIVATE_EXPEDITED, 0, 0) == 0)
        heavy_barrier_works.store(true, std::memory_order_release);
#endif
}

void sleeper_side_barrier()
{
#if defined(__linux__) && defined(MEMBARRIER_CMD_PRIVATE_EXPEDITED)
    if (heavy_barrier_works.load(std::memory_order_relaxed)) {
        if (syscall(SYS_membarrier, MEMBARRIER_CMD_PRIVATE_EXPEDITED, 0, 0) == 0)
            return;
        heavy_barrier_works.store(false, std::memory_order_relaxed);   // the wakers fence from now on
    }
#endif
    std::atomic_thread_fence(std::memory_order_seq_cst);
}

inline void waker_side_barrier()
{
    if (heavy_barrier_works.load(std::memory_order_relaxed)) [[likely]]
        std::atomic_signal_fence(std::memory_order_seq_cst);   // the compiler's order only
    else
        std::atomic_thread_fence(std::memory_order_seq_cst);
}

inline void ring(Sleeper &sleeper, bool everyone = false)
{
    if (sleeper.asleep.load(std::memory_order_relaxed) == 0) [[likely]]
        return;
    sleeper.bell.fetch_add(1, std::memory_order_release);
    if (everyone)
        sleeper.bell.notify_all();
    else
        sleeper.bell.notify_one();
}

template <typename WorkWaits>
void sleep_until_rung(Sleeper &sleeper, WorkWaits work_waits)
{
    const std::uint32_t rung = sleeper.bell.load(std::memory_order_acquire);
    sleeper.asleep.fetch_add(1, std::memory_order_relaxed);
    sleeper_side_barrier();
    if (!work_waits())
        sleeper.bell.wait(rung, std::memory_order_acquire);
    sleeper.asleep.fetch_sub(1, std::memory_order_relaxed);
}

inline void pause_a_moment()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#endif
}

void raise_to(std::atomic<std::uint64_t> &counter, std::uint64_t to)
{
    std::uint64_t now = counter.load(std::memory_order_relaxed);
    while (now < to && !counter.compare_exchange_weak(now, to, std::memory_order_release)) {
    }
}

// EVERYTHING THE THREE THREADS SHARE -- never destroyed: std::cout is flushed after main()
// returns, through this, and a slot's value would otherwise be destroyed under a thread
// still reading it.
struct Satellite {
    // 1. THE RING, interpreter -> printing satellite.
    Job slots[kSlots];
    alignas(64) std::atomic<std::uint64_t> handed{0};   // jobs handed over; only the interpreter writes it
    std::uint64_t free_seen = 0;                         // the interpreter's last look at `taken`
    std::mutex handing;                                  // held only once a program has started a thread
    alignas(64) std::atomic<std::uint64_t> taken{0};    // jobs the printing satellite is done with

    // 2. THE PIECES, printing satellite -> display thread, round and round: piece n % kPieces.
    // A piece may hold no bytes at all and still carry `piece_through`: jobs let go travel to
    // the display thread that way, so everything before a flush is settled in order.
    std::string pieces[kPieces];
    std::uint64_t piece_through[kPieces] = {};           // every job before this is in the piece, or let go
    alignas(64) std::atomic<std::uint64_t> made{0};      // pieces handed to the display thread
    alignas(64) std::atomic<std::uint64_t> written{0};   // pieces written (or refused)

    // 3. WHAT A FLUSH WAITS FOR: every job before `settled` is on the screen, refused, or let go.
    // Only the display thread moves it, a piece at a time, in order.
    alignas(64) std::atomic<std::uint64_t> settled{0};
    std::atomic<std::uint64_t> flush_wanted{0};          // a flush is waiting for jobs before this
    std::atomic<std::uint64_t> gave_up_at{UINT64_MAX};   // `written` when a flush last gave up on the screen

    Sleeper printer, display, flushers;

    // HIS LIMIT, read once from arguments.display.buffer.
    std::atomic<unsigned long long int> most_waiting{kDisplayBufferDefault};

    // THE OVERRUN, AND LETTING GO.
    alignas(64) std::atomic<bool> overran{false};
    std::atomic<bool> dropping{false};                   // letting go of every job as it comes
    std::atomic<std::uint64_t> drop_before{0};           // ...or of every job before this
    std::atomic<bool> reported{false};
    std::atomic<bool> quit_was_ours{false};              // program_quit() was set by the overrun

    std::atomic<bool> refused{false};                    // the screen refused a write: said once
    std::atomic<bool> out_of_memory_here{false};         // a line had no memory to be made in: said once
    bool display_runs = false;                           // else the printing satellite writes itself

    // STEP 5: satl's own console, fed straight -- and the piece as the pty would have sent it.
    std::atomic<ConsoleTaker> console{nullptr};
    std::atomic<ConsoleHurry> hurry{nullptr};
    std::atomic<ConsoleFlows> flows{nullptr};
    std::string fed;                                     // the display thread's (or the printer's, in turn)
    std::atomic<std::int64_t> pty_written_at{0};         // the last direct write to the pty
};

Satellite &the_satellite()
{
    static Satellite *const one = new Satellite;
    return *one;
}

std::atomic<bool> started{false};                        // the printing satellite is running
std::atomic<bool> in_a_forked_child{false};              // and in a fork's child it is not

// write(1) until it is all out. EAGAIN -- an output somebody made non-blocking -- waits for room.
bool write_all(const char *bytes, std::size_t size)
{
    while (size > 0) {
        const ssize_t wrote = ::write(STDOUT_FILENO, bytes, size);
        if (wrote > 0) {
            bytes += wrote;
            size -= static_cast<std::size_t>(wrote);
        } else if (wrote < 0 && errno == EINTR) {
            continue;
        } else if (wrote < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            pollfd one{STDOUT_FILENO, POLLOUT, 0};
            ::poll(&one, 1, -1);
        } else {
            return false;
        }
    }
    return true;
}

// A PLAIN display's LINE, exactly as display's library wrote it
// (satellite-numbers/satellite.console.display): a string as its UTF-8 -- its colour codes left
// out when colours do not reach a terminal (console_style.hpp's screen_text) -- a number that
// fits a count as its digits and any other as its text, and the rest as each kind is written.
void append_the_line(const Value &value, std::string &out)
{
    if (const satellite_string *text = value.as_string()) {
        const std::size_t start = out.size();
        text->append_utf8_to(out);
        if (!colours_reach_a_terminal() && out.find('\033', start) != std::string::npos) {
            const std::string line = out.substr(start);
            out.resize(start);
            out += for_the_screen(line);
        }
    } else if (const satellite_number *number = value.as_number()) {
        if (number_fast_path::fits_a_count(*number)) {
            char digits[24];
            const std::to_chars_result made =
                std::to_chars(digits, digits + sizeof digits, number_fast_path::as_count(*number));
            out.append(digits, made.ptr);
        } else {
            out += number->to_text();
        }
    } else if (const bool *flag = value.as_bool()) {
        out += *flag ? "true" : "false";
    } else if (const satellite_binary_number *bits = value.as_binary()) {
        out += bits->written();
    } else if (const satellite_hexadecimal_number *hex = value.as_hexadecimal()) {
        out += hex->written();
    } else if (const satellite_color *colour = value.as_color()) {
        out += colour->written();
    } else if (const satellite_percentage *percentage = value.as_percentage()) {
        out += percentage->written();
    } else if (value.is_infinity()) {
        out += satellite_infinity::display(value.as_infinity());
    } else {
        // A FLOAT OR A FRACTION: the text `.string` answers (object_float.cpp, object_fraction.cpp),
        // which cannot be refused.
        satellite_string written;
        std::string why;
        if (value.to_string(written, why) == success)
            written.append_utf8_to(out);
    }
    out += '\n';
}

void append_the_job(const Job &job, std::string &out)
{
    if (job.kind == JobKind::bytes)
        out += job.bytes;
    else
        append_the_line(job.value, out);
}

// ---------------------------------------------------------------------------------------------
// 2. THE DISPLAY THREAD: "a thread to display std::string -- the display thread will receive a
// single std::string, display it, and then get the next one". One piece, one write().
// ---------------------------------------------------------------------------------------------

// A PTY'S ONLCR, done here because a feed goes past the pty: every '\n' becomes "\r\n" -- every
// one, as the pty does, so a "\r\n" satl wrote itself arrives as "\r\r\n" exactly as before.
void as_the_pty_sends_it(const std::string &piece, std::string &out)
{
    out.clear();
    out.reserve(piece.size() + piece.size() / 8);
    const char *at = piece.data();
    const char *const end = at + piece.size();
    while (at < end) {
        const void *found = std::memchr(at, '\n', static_cast<std::size_t>(end - at));
        if (found == nullptr) {
            out.append(at, end);
            break;
        }
        const char *newline = static_cast<const char *>(found);
        out.append(at, newline);
        out += "\r\n";
        at = newline + 1;
    }
}

// STEP 5: the piece into satl's own console, when there is one to take it. False: write it.
bool fed_to_the_console(Satellite &s, const std::string &piece, std::uint64_t through)
{
    const ConsoleTaker take = s.console.load(std::memory_order_acquire);
    if (take == nullptr)
        return false;
    bool fed = false;
    try {
        as_the_pty_sends_it(piece, s.fed);
        fed = take(s.fed, through);
    } catch (const std::bad_alloc &) {
        // NO MEMORY FOR THE "\r\n" COPY: the piece is let go, as the printing satellite lets go of
        // a line it has no memory for, and the next display says out_of_memory. Its jobs still
        // settle in order, through an empty piece -- which takes no memory to hand over.
        std::string().swap(s.fed);
        s.out_of_memory_here.store(true, std::memory_order_release);
        fed = take(s.fed, through);
    }
    if (s.fed.capacity() > kKeepAtMost)
        std::string().swap(s.fed);
    return fed;
}

void write_the_piece(std::uint64_t which)
{
    Satellite &s = the_satellite();
    std::string &piece = s.pieces[which % kPieces];
    const std::uint64_t through = s.piece_through[which % kPieces];
    // FED, IT IS SETTLED BY THE WINDOW'S THREAD once VTE has it (display_fed_through), and the
    // piece comes back now; written, it is settled here.
    const bool fed = fed_to_the_console(s, piece, through);
    if (!fed && !piece.empty() && !write_all(piece.data(), piece.size()))
        s.refused.store(true, std::memory_order_release);
    if (piece.capacity() > kKeepAtMost)
        std::string().swap(piece);   // one huge line does not keep a huge piece for the rest of the run
    else
        piece.clear();
    s.written.store(which + 1, std::memory_order_release);
    if (!fed)
        raise_to(s.settled, through);
    std::atomic_thread_fence(std::memory_order_seq_cst);   // the sleepers' "asleep" is read after this
    if (!fed)
        ring(s.flushers, true);
    ring(s.printer);   // it may be waiting for a piece to fill
}

void *display_thread(void *)
{
    Satellite &s = the_satellite();
    std::uint64_t next = 0;
    for (;;) {
        if (next < s.made.load(std::memory_order_acquire)) {
            write_the_piece(next++);
            continue;
        }
        const Clock::time_point until = Clock::now() + kDisplayLooks;
        while (next >= s.made.load(std::memory_order_acquire) && Clock::now() < until)
            pause_a_moment();
        if (next >= s.made.load(std::memory_order_acquire))
            sleep_until_rung(s.display, [&s, next] { return next < s.made.load(std::memory_order_acquire); });
    }
    return nullptr;
}

// ---------------------------------------------------------------------------------------------
// 1. THE PRINTING SATELLITE: "stores it, pushes the next one in line into the converter, then
// readies itself for the next one". It takes every job the moment it arrives, makes its line
// in the piece being filled, and when every piece is out being written it keeps the job in his
// buffer, counted, until one comes back.
// ---------------------------------------------------------------------------------------------

struct Printer {
    Satellite &s = the_satellite();
    std::uint64_t taken = 0;                 // jobs finished with (the ring's slots handed back)
    std::uint64_t made = 0;                  // pieces handed to the display thread
    // EVERY JOB BEFORE THIS IS IN A PIECE, OR LET GO -- one added for every job, whichever way
    // it goes, so it is always the index of the next job in line: his buffer's front, or the
    // ring's next when his buffer is empty.
    std::uint64_t formatted = 0;
    std::uint64_t handed_through = 0;        // `formatted` as the last piece handed carried it
    std::deque<Job> held;                    // HIS BUFFER: jobs the screen has not taken yet
    unsigned long long int held_count = 0;   // "keep the size of it in another unsigned long long int"

    // THE PIECE BEING FILLED IS OURS ONLY WHILE ONE IS FREE: with every piece out, pieces[made %
    // kPieces] is the one the display thread is writing. So a_piece_is_free() comes first, always.
    bool a_piece_is_free() const { return made - s.written.load(std::memory_order_acquire) < kPieces; }
    std::string &piece() { return s.pieces[made % kPieces]; }
    // SOMETHING FOR THE DISPLAY THREAD: bytes, or jobs let go that a flush may be waiting on.
    bool a_piece_waits_to_go()
    {
        return held.empty() && a_piece_is_free() && (!piece().empty() || formatted > handed_through);
    }
    bool a_flush_waits() const
    {
        return s.flush_wanted.load(std::memory_order_acquire) > s.settled.load(std::memory_order_acquire);
    }
    bool letting_go_of(std::uint64_t job) const
    {
        return s.dropping.load(std::memory_order_acquire) || job < s.drop_before.load(std::memory_order_acquire);
    }

    void hand_the_piece()
    {
        if (piece().empty() && formatted <= handed_through)
            return;
        s.piece_through[made % kPieces] = formatted;
        handed_through = formatted;
        ++made;
        s.made.store(made, std::memory_order_release);
        if (s.display_runs) {
            waker_side_barrier();
            ring(s.display);
        } else {
            write_the_piece(made - 1);       // no display thread: written here, in turn
        }
    }

    // His buffer let go, every job in it counted as done with.
    void let_go_of_his_buffer()
    {
        formatted += held.size();
        held.clear();
        held_count = 0;
    }

    // THE OVERRUN. "if the buffer is holding 131072 std::string objects then it crashes the
    // interpreter" -- his buffer and everything after it are let go, and every walker stops
    // between two statements with S840 (program_walk.cpp).
    void overrun()
    {
        s.dropping.store(true, std::memory_order_release);
        s.overran.store(true, std::memory_order_release);
        let_go_of_his_buffer();
        if (!program_quit().exchange(true))
            s.quit_was_ours.store(true, std::memory_order_release);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        ring(s.flushers, true);
    }

    // A job's line into the piece. With no memory to make it, the half-made line is taken back
    // out, the job is let go, and the next display says out_of_memory.
    void append(const Job &job)
    {
        std::string &out = piece();
        const std::size_t before = out.size();
        try {
            append_the_job(job, out);
        } catch (const std::bad_alloc &) {
            out.resize(before);
            s.out_of_memory_here.store(true, std::memory_order_release);
        }
        ++formatted;
        if (out.size() >= kPieceBytes)
            hand_the_piece();
    }

    // A job from the ring: let go, into his buffer, or into the piece.
    void take(Job &job, std::uint64_t index)
    {
        if (held.empty() && letting_go_of(index)) {
            ++formatted;
            return;
        }
        if (!held.empty() || !a_piece_is_free()) {
            try {
                held.push_back(std::move(job));
            } catch (const std::bad_alloc &) {
                let_go_of_his_buffer();
                ++formatted;
                s.out_of_memory_here.store(true, std::memory_order_release);
                return;
            }
            // "if(buffer.size() > x)" -- x loaded once, as he asked.
            if (++held_count > s.most_waiting.load(std::memory_order_relaxed))
                overrun();
            return;
        }
        append(job);
    }

    // His buffer, oldest first: the jobs being let go, then lines into whatever pieces came back.
    // A job taken out of his buffer is freed here, on this thread: the screen is behind then, and
    // it is the screen that sets the pace.
    void empty_his_buffer()
    {
        while (!held.empty() && letting_go_of(formatted)) {
            held.pop_front();
            --held_count;
            ++formatted;
        }
        while (!held.empty() && a_piece_is_free()) {
            append(held.front());
            held.pop_front();
            --held_count;
        }
    }

    bool work_waits() const
    {
        return s.handed.load(std::memory_order_acquire) > taken ||
               (!held.empty() && (a_piece_is_free() || letting_go_of(formatted)));
    }

    void run()
    {
        Clock::time_point last_job = Clock::now();
        const auto hand_when_it_is_time = [this, &last_job] {
            if (a_piece_waits_to_go() && (a_flush_waits() || Clock::now() - last_job >= kHandAfter))
                hand_the_piece();
        };
        for (;;) {
            empty_his_buffer();
            const std::uint64_t handed = s.handed.load(std::memory_order_acquire);
            if (taken < handed) {
                for (; taken < handed; ++taken) {
                    take(s.slots[taken % kSlots], taken);
                    s.taken.store(taken + 1, std::memory_order_release);
                }
                last_job = Clock::now();
                continue;
            }
            // NOTHING NEW. The piece goes to the screen when a flush waits for it, or when nothing
            // has come for kHandAfter -- a steady stream fills pieces instead of writing a line at a
            // time -- and always before this sleeps.
            hand_when_it_is_time();
            const Clock::time_point until = Clock::now() + kPrinterLooks;
            while (!work_waits() && Clock::now() < until) {
                hand_when_it_is_time();
                pause_a_moment();
            }
            if (work_waits())
                continue;
            if (a_piece_waits_to_go())
                hand_the_piece();
            sleep_until_rung(s.printer, [this] { return work_waits(); });
        }
    }
};

// THE THREADS' SIGNALS: every one blocked, so Ctrl-C and a closed terminal land on the thread the
// program runs on, whose waits they are meant to break -- BUT SIGTTOU OPEN ON THE DISPLAY THREAD,
// which writes the terminal: a satl in the background under `stty tostop` must still stop there,
// as the interpreter's own write stopped it before (DISPLAY_THREADS.md, the review's 9). The mask
// is SET, not added to, because the display thread is started from the printing satellite, whose
// own mask already blocks everything.
bool start_thread(void *(*body)(void *), const char *name, bool let_sigttou_through)
{
    pthread_attr_t shape;
    if (pthread_attr_init(&shape) != 0)
        return false;
    pthread_attr_setstacksize(&shape, kThreadStack);
    pthread_attr_setdetachstate(&shape, PTHREAD_CREATE_DETACHED);
    sigset_t wanted, before;
    sigfillset(&wanted);
    if (let_sigttou_through)
        sigdelset(&wanted, SIGTTOU);
    pthread_sigmask(SIG_SETMASK, &wanted, &before);
    pthread_t id;
    const bool made = pthread_create(&id, &shape, body, nullptr) == 0;
    pthread_sigmask(SIG_SETMASK, &before, nullptr);
    pthread_attr_destroy(&shape);
    if (made)
        pthread_setname_np(id, name);
    return made;
}

void *printing_satellite(void *)
{
    Satellite &s = the_satellite();
    // "that one thread creates ... a thread to display std::string".
    s.display_runs = start_thread(display_thread, "satl-display", true);
    for (std::string &piece : s.pieces)
        piece.reserve(kPieceBytes + kPieceBytes / 4);
    Printer printer;
    for (;;) {
        try {
            printer.run();
        } catch (const std::bad_alloc &) {
            // THE LAST RESORT: memory refused somewhere the printer did not expect. Its buffer is let
            // go, and the next display says out_of_memory.
            printer.let_go_of_his_buffer();
            s.out_of_memory_here.store(true, std::memory_order_release);
        }
    }
    return nullptr;
}

void after_a_fork_in_the_child() { in_a_forked_child.store(true, std::memory_order_relaxed); }

bool writes_directly()
{
    return !started.load(std::memory_order_acquire) || in_a_forked_child.load(std::memory_order_relaxed);
}

// THE ANSWER A DISPLAY GIVES: the screen's refusal, or the printing satellite's want of memory --
// each said ONCE (std::cout's badbit, set by the stream, says a refusal after that).
signed long long int what_the_screen_said()
{
    Satellite &s = the_satellite();
    if (s.out_of_memory_here.load(std::memory_order_relaxed) &&
        s.out_of_memory_here.exchange(false, std::memory_order_acq_rel)) [[unlikely]]
        return out_of_memory;
    if (s.refused.load(std::memory_order_relaxed) && s.refused.exchange(false, std::memory_order_acq_rel)) [[unlikely]]
        return display_error;
    return success;
}

// THE HAND-OFF. Under `handing` only once a program has started a thread -- the rule
// machine/console_lock.hpp gives a line -- so a program that never does pays nothing for it.
template <typename Fill>
void hand_over(Fill fill)
{
    Satellite &s = the_satellite();
    std::unique_lock<std::mutex> hold;
    if (a_thread_was_started().load(std::memory_order_acquire))
        hold = std::unique_lock<std::mutex>(s.handing);
    const std::uint64_t n = s.handed.load(std::memory_order_relaxed);
    if (n - s.free_seen >= kSlots) {
        s.free_seen = s.taken.load(std::memory_order_acquire);
        while (n - s.free_seen >= kSlots) {   // 1,024 on their way and none taken: it is not being run
            ring(s.printer);
            std::this_thread::yield();
            s.free_seen = s.taken.load(std::memory_order_acquire);
        }
    }
    // THE SLOT'S OLD JOB IS LET GO HERE, on the interpreter's own thread, as `fill` writes over it:
    // a string made on one thread and freed on another cost ~100 ns more (glibc's per-thread
    // caches, DISPLAY_THREADS.md), so the printing satellite reads a job where it lies and frees
    // one only when the screen is behind and the job has waited in his buffer.
    fill(s.slots[n % kSlots]);
    s.handed.store(n + 1, std::memory_order_release);
    waker_side_barrier();
    ring(s.printer);
}

std::uint64_t handed_so_far()
{
    Satellite &s = the_satellite();
    std::unique_lock<std::mutex> hold;
    if (a_thread_was_started().load(std::memory_order_acquire))
        hold = std::unique_lock<std::mutex>(s.handing);
    return s.handed.load(std::memory_order_relaxed);
}

// Every job handed over so far let go: the printing satellite drops them, oldest first, as it
// meets them, and a flush still waits for the pieces already on their way.
void let_go_of_everything_handed()
{
    Satellite &s = the_satellite();
    raise_to(s.drop_before, handed_so_far());
    std::atomic_thread_fence(std::memory_order_seq_cst);
    ring(s.printer);
}

// AFTER AN OVERRUN THE SCREEN MAY NEVER TAKE ANOTHER BYTE -- a fifo nobody reads -- so a flush
// waits only while it is still taking them: until everything before it is out, or until
// kScreenStopped passes with no piece written. A screen given up on once is not waited on again
// until it moves.
void wait_while_the_screen_takes_text(std::uint64_t target)
{
    Satellite &s = the_satellite();
    std::uint64_t seen = s.written.load(std::memory_order_acquire);
    if (seen == s.gave_up_at.load(std::memory_order_acquire))
        return;
    Clock::time_point moved = Clock::now();
    while (s.settled.load(std::memory_order_acquire) < target) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        const std::uint64_t now = s.written.load(std::memory_order_acquire);
        if (now != seen) {
            seen = now;
            moved = Clock::now();
        } else if (Clock::now() - moved >= kScreenStopped) {
            s.gave_up_at.store(seen, std::memory_order_release);
            return;
        }
    }
}

} // namespace

void start_the_printing_satellite()
{
    register_the_heavy_barrier();
    the_satellite();
    pthread_atfork(nullptr, nullptr, after_a_fork_in_the_child);
    // "a thread off of the satl process" -- at start-up, before anything prints.
    if (start_thread(printing_satellite, "satl-print", false))
        started.store(true, std::memory_order_release);
    std_cout_goes_to_the_printing_satellite();
}

void set_the_display_buffer(unsigned long long int most_waiting)
{
    the_satellite().most_waiting.store(most_waiting, std::memory_order_relaxed);
}

unsigned long long int the_display_buffer()
{
    return the_satellite().most_waiting.load(std::memory_order_relaxed);
}

signed long long int display_value(Value &&value)
{
    // satl's OWN WORDS WRITTEN BEFORE THIS LINE GO FIRST.
    hand_over_what_std_cout_holds();
    if (writes_directly()) [[unlikely]] {
        std::string line;
        append_the_line(value, line);
        return write_all(line.data(), line.size()) ? success : display_error;
    }
    const signed long long int said = what_the_screen_said();
    if (said != success) [[unlikely]]
        return said;
    hand_over([&value](Job &slot) {
        slot.value = std::move(value);
        slot.kind = JobKind::value;
        if (slot.bytes.capacity() > 15)
            std::string().swap(slot.bytes);
    });
    return success;
}

signed long long int display_bytes(std::string &&bytes)
{
    if (bytes.empty())
        return success;
    if (writes_directly()) [[unlikely]] {
        const bool wrote = write_all(bytes.data(), bytes.size());
        bytes.clear();
        return wrote ? success : display_error;
    }
    const signed long long int said = what_the_screen_said();
    hand_over([&bytes](Job &slot) {
        slot.bytes.swap(bytes);              // the old slot's bytes come back, and are let go here
        slot.kind = JobKind::bytes;
        if (!slot.value.is_nothing())
            slot.value = Value();
    });
    bytes.clear();
    return said;
}

signed long long int display_line_bytes(std::string &&bytes)
{
    hand_over_what_std_cout_holds();
    return display_bytes(std::move(bytes));
}

signed long long int display_drain()
{
    if (writes_directly())
        return success;
    Satellite &s = the_satellite();
    const std::uint64_t target = handed_so_far();
    const auto done = [&s, target] { return s.settled.load(std::memory_order_acquire) >= target; };
    const auto stopped = [&s] {
        return s.overran.load(std::memory_order_acquire) || s.out_of_memory_here.load(std::memory_order_acquire);
    };
    if (!done()) {
        raise_to(s.flush_wanted, target);
        std::atomic_thread_fence(std::memory_order_seq_cst);
        ring(s.printer);
        // satl's OWN CONSOLE FEEDS ONCE A FRAME; a flush waiting has it fed now (console_feed.cpp).
        if (const ConsoleHurry hurry = s.hurry.load(std::memory_order_acquire))
            hurry();
        const Clock::time_point until = Clock::now() + kPrinterLooks;
        while (!done() && !stopped() && Clock::now() < until)
            pause_a_moment();
        while (!done()) {
            if (stopped()) {
                wait_while_the_screen_takes_text(target);
                break;
            }
            sleep_until_rung(s.flushers, [&done, &stopped] { return done() || stopped(); });
        }
    }
    return what_the_screen_said();
}

bool display_overran() { return the_satellite().overran.load(std::memory_order_acquire); }

bool display_overrun_is_mine_to_report()
{
    Satellite &s = the_satellite();
    if (!s.overran.load(std::memory_order_acquire) || s.reported.exchange(true, std::memory_order_acq_rel))
        return false;
    // FROM HERE ON satl's OWN WORDS GET THROUGH -- the report's flush, the console's "stopped on
    // machine code 65", the prompt -- and only what the program handed over before is let go.
    raise_to(s.drop_before, handed_so_far());
    s.dropping.store(false, std::memory_order_release);
    return true;
}

std::string display_overrun_sentence()
{
    // The author, 2026-09-26: "the user needs to be given a message that it is our philosophy that
    // programming is not meant to overrun the console with messages faster than it can print, and
    // the code that they have programmed has overrun the 131,072 item buffer, so they need to
    // re-work their code so that it does not do that, and display less".
    const std::string most = std::to_string(the_display_buffer());
    return "more than " + most + " displays were waiting for the console. It is satellite's philosophy that "
           "programming is not meant to overrun the console with messages faster than it can print, and this "
           "program has overrun the " + most + " item buffer -- rework it so that it displays less, or raise "
           "arguments.display.buffer (display.buffer = ... in ~/.satl/config.ini)";
}

void display_overrun_let_go()
{
    Satellite &s = the_satellite();
    if (!s.overran.load(std::memory_order_acquire))
        return;
    // AN OVERRUN NOBODY REPORTED -- a program's thread met it, and ends quietly -- lets go of what
    // was handed over and stops dropping, exactly as a report would have.
    if (!s.reported.load(std::memory_order_acquire)) {
        raise_to(s.drop_before, handed_so_far());
        s.dropping.store(false, std::memory_order_release);
    }
    if (s.quit_was_ours.exchange(false, std::memory_order_acq_rel))
        program_quit().store(false, std::memory_order_release);
    s.reported.store(false, std::memory_order_release);
    s.overran.store(false, std::memory_order_release);
    s.gave_up_at.store(UINT64_MAX, std::memory_order_release);
}

void display_let_go_of_what_waits()
{
    if (!writes_directly())
        let_go_of_everything_handed();
}

void display_goes_to_the_console(ConsoleTaker take, ConsoleHurry hurry, ConsoleFlows flows)
{
    Satellite &s = the_satellite();
    s.flows.store(flows, std::memory_order_release);
    s.hurry.store(hurry, std::memory_order_release);
    s.console.store(take, std::memory_order_release);
}

bool display_a_flush_waits()
{
    Satellite &s = the_satellite();
    return s.flush_wanted.load(std::memory_order_acquire) > s.settled.load(std::memory_order_acquire);
}

void display_fed_through(std::uint64_t through)
{
    Satellite &s = the_satellite();
    raise_to(s.settled, through);
    std::atomic_thread_fence(std::memory_order_seq_cst);   // the sleepers' "asleep" is read after this
    ring(s.flushers, true);
}

void the_pty_was_written_directly()
{
    the_satellite().pty_written_at.store(Clock::now().time_since_epoch().count(), std::memory_order_release);
}

std::int64_t when_the_pty_was_last_written_directly()
{
    return the_satellite().pty_written_at.load(std::memory_order_acquire);
}

void the_pty_flows_again()
{
    if (const ConsoleFlows flows = the_satellite().flows.load(std::memory_order_acquire))
        flows();
}

} // namespace satellite004
