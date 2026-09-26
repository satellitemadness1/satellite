// vte_print_race.cpp -- the author's race, 2026-09-26: 1,000 prints of
//
//     500000 + 12349.474 * 457474 + " hello, world!"
//
// into a real VTE window (satl's own vendored GTK 4.24 and VTE), three ways:
//
//   A  THE FIVE STEPS. The main thread works the line out through satellite's own
//      number, float and string code (satelliteObject::multiply and ::add, the calls
//      satl's evaluator makes), then hands the finished satellite_string to the
//      window's thread, which makes it UTF-8 and feeds VTE directly.
//   B  THE BYPASS. The main thread hands over only "one more of this line"; the
//      window's thread works it out through the same satellite code, makes it
//      UTF-8 and feeds VTE directly.
//   C  COMPILED C++. The main thread does the sum in doubles and writes it with
//      std::cout into the window's pty, which is how satl's own console gets its
//      text today.
//   D  THE FIVE STEPS, FED ONCE A FRAME (FAST_PRINTING.md step 5, added
//      2026-09-26 after the /clear). The main thread does what A's does; a
//      hand-off wakes the window's thread only to ask for a frame, and the lines
//      are fed on the frame clock's `before-paint` -- which runs ahead of
//      `update`, where VTE 0.84 parses (scheduler.cc: a tick callback). So what
//      is fed in a frame is parsed in that frame.
//
// Per print: what the main thread paid (its loop / prints), what the window's thread
// paid for its part (A, B and D), and how long until VTE had taken in every line
// (end to end / prints).
//
//     RACE_PRINTS=100000   prints a run (1,000 when not set): 1,000 measure a frame's
//                          phase as much as anything; 100,000 measure throughput.
//     RACE_ROUNDS=15       runs of each row (7 when not set).
//     RACE_TRACE=1         the last round's timeline, per row: every feed, every
//                          frame's `update`, and `contents-changed`, in microseconds.

#include <gtk/gtk.h>
#include <vte/vte.h>
#include <glib-unix.h>

#include "satellite/satellite_object/satellite_object.hpp"
#include "satellite/satellite_variable_float/float_scaled.hpp"
#include "satellite/satellite_variable_number/number_conversions.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <future>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <mutex>
#include <string>
#include <sys/eventfd.h>
#include <thread>
#include <unistd.h>
#include <vector>

using namespace satellite004;
using Value = satellite004::satelliteObject;
using clock_type = std::chrono::steady_clock;

namespace {

int kPrints = 1000;
int kRounds = 7;

enum class Row { five_steps, bypass, cout, once_a_frame };
constexpr Row kRows[] = {Row::five_steps, Row::once_a_frame, Row::cout, Row::bypass};

const char *name_of(Row row)
{
    switch (row) {
    case Row::five_steps: return "A five steps";
    case Row::bypass: return "B bypass    ";
    case Row::cout: return "C C++ cout  ";
    case Row::once_a_frame: return "D a frame   ";
    }
    return "?";
}

// THE TIMELINE (RACE_TRACE=1): what happened on the window's thread, and when.
bool tracing = false;
bool trace_this_run = false;
struct Mark {
    char what;          // f fed, u a frame's update, c contents-changed
    clock_type::time_point at;
    long bytes;
};
std::vector<Mark> marks;   // the window's thread's; read by the race once the run is done
void mark(char what, long bytes = 0)
{
    if (trace_this_run)
        marks.push_back(Mark{what, clock_type::now(), bytes});
}

double ns_since(clock_type::time_point from, clock_type::time_point to)
{
    return std::chrono::duration<double, std::nano>(to - from).count();
}

// THE LINE, THROUGH SATELLITE: the literals made as satl makes them each time the
// line runs, then 12349.474 * 457474, then 500000 + that, then + " hello, world!".
satellite_string make_the_line()
{
    satellite_number five_hundred_thousand, scaled, factor;
    number_fast_path::from_token_text("500000", number_fast_path::kDecimal, five_hundred_thousand);
    number_fast_path::from_token_text("12349474", number_fast_path::kDecimal, scaled);
    number_fast_path::from_token_text("457474", number_fast_path::kDecimal, factor);
    const Value a = Value::of_number(std::move(five_hundred_thousand));
    const Value f = Value::of_float(float_from_scaled(std::move(scaled), 3));
    const Value b = Value::of_number(std::move(factor));
    Value text;
    std::size_t bad = 0;
    Value::of_utf8(" hello, world!", text, bad);
    std::string why;
    Value product, sum, line;
    f.multiply(b, product, why);
    a.add(product, sum, why);
    sum.add(text, line, why);
    return std::move(*line.as_string());
}

struct Job {
    satellite_string text;   // A: the finished line. B: empty -- "one more of this line".
};

// ---- the window, and everything only its thread touches ----
GtkWidget *terminal = nullptr;
GMainLoop *loop = nullptr;
Row running = Row::five_steps;
long start_row = 0;
bool counting = false;
std::string feeding;                      // the bytes of one feed, reused
std::vector<Job> taking;                  // the jobs of one wake, reused

// ---- between the main thread and the window's thread ----
std::mutex hand_lock;
std::vector<Job> waiting;
bool window_is_idle = true;               // under hand_lock: the next hand-off must wake it
GdkFrameClock *frame_clock = nullptr;     // the terminal's, once D has asked for a frame
int wake_fd = -1;
std::atomic<double> window_ns{0};         // what the window's thread spent on its part
std::mutex done_lock;
std::condition_variable done_changed;
bool done = false;
clock_type::time_point done_at;

template <typename Work>
void on_the_window(Work work)
{
    std::promise<void> finished;
    auto *call = new std::pair<Work, std::promise<void> *>(std::move(work), &finished);
    g_main_context_invoke(
        nullptr,
        [](gpointer data) -> gboolean {
            auto *held = static_cast<std::pair<Work, std::promise<void> *> *>(data);
            held->first();
            held->second->set_value();
            delete held;
            return G_SOURCE_REMOVE;
        },
        call);
    finished.get_future().wait();
}

// ONE FEED: everything handed over so far, the window's part done. A and B feed
// every time they are woken; D only on `before-paint`.
void feed_what_waits()
{
    for (;;) {
        taking.clear();
        {
            const std::lock_guard<std::mutex> hold(hand_lock);
            taking.swap(waiting);
            if (taking.empty()) {
                window_is_idle = true;
                break;
            }
        }
        const clock_type::time_point began = clock_type::now();
        feeding.clear();
        for (Job &job : taking) {
            // A feed is not a pty: nothing turns \n into \r\n, so the line ends here.
            if (running == Row::bypass) {
                feeding += make_the_line().to_utf8();
            } else {
                feeding += job.text.to_utf8();
            }
            feeding += "\r\n";
        }
        vte_terminal_feed(VTE_TERMINAL(terminal), feeding.data(), static_cast<gssize>(feeding.size()));
        window_ns.store(window_ns.load() + ns_since(began, clock_type::now()));
        mark('f', static_cast<long>(feeding.size()));
    }
}

void on_before_paint(GdkFrameClock *, gpointer)
{
    if (running == Row::once_a_frame)
        feed_what_waits();
}

void on_update(GdkFrameClock *, gpointer) { mark('u'); }

// THE WINDOW'S THREAD, WOKEN. A and B feed now; D asks for a frame and feeds in it.
gboolean on_wake(gint fd, GIOCondition, gpointer)
{
    std::uint64_t count = 0;
    if (read(fd, &count, sizeof count) < 0) {}
    if (running != Row::once_a_frame) {
        feed_what_waits();
        return G_SOURCE_CONTINUE;
    }
    gdk_frame_clock_request_phase(frame_clock, GDK_FRAME_CLOCK_PHASE_BEFORE_PAINT);
    return G_SOURCE_CONTINUE;
}

long cursor_row()
{
    glong column = 0, row = 0;
    vte_terminal_get_cursor_position(VTE_TERMINAL(terminal), &column, &row);
    return row;
}

// VTE HAS TAKEN THE TEXT IN when the cursor stands 1,000 rows below where it started.
void on_contents_changed(VteTerminal *, gpointer)
{
    mark('c');
    if (!counting || cursor_row() - start_row < kPrints)
        return;
    counting = false;
    const std::lock_guard<std::mutex> hold(done_lock);
    done = true;
    done_at = clock_type::now();
    done_changed.notify_all();
}

// The window's thread: a real VTE with a pty of its own, and stdout on the pty's
// other end for row C.
bool make_the_window()
{
    GtkWidget *window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "vte print race");
    gtk_window_set_default_size(GTK_WINDOW(window), 1000, 700);
    terminal = vte_terminal_new();
    vte_terminal_set_scrollback_lines(VTE_TERMINAL(terminal), 10000);
    gtk_window_set_child(GTK_WINDOW(window), terminal);
    GError *trouble = nullptr;
    VtePty *pty = vte_pty_new_sync(VTE_PTY_DEFAULT, nullptr, &trouble);
    if (pty == nullptr) {
        std::fprintf(stderr, "no pty: %s\n", trouble != nullptr ? trouble->message : "?");
        return false;
    }
    vte_terminal_set_pty(VTE_TERMINAL(terminal), pty);
    char name[256];
    if (ptsname_r(vte_pty_get_fd(pty), name, sizeof name) != 0)
        return false;
    const int slave = open(name, O_RDWR | O_NOCTTY | O_CLOEXEC);
    if (slave < 0 || dup2(slave, STDOUT_FILENO) < 0)
        return false;
    g_signal_connect(terminal, "contents-changed", G_CALLBACK(on_contents_changed), nullptr);
    g_unix_fd_add(wake_fd, G_IO_IN, on_wake, nullptr);
    gtk_window_present(GTK_WINDOW(window));
    return true;
}

// THE MAIN THREAD'S HAND-OFF: into the waiting list, and a wake only when the
// window's thread has gone idle.
void hand_over(Job &&job)
{
    bool wake = false;
    {
        const std::lock_guard<std::mutex> hold(hand_lock);
        waiting.push_back(std::move(job));
        wake = window_is_idle;
        window_is_idle = false;
    }
    if (wake) {
        const std::uint64_t one = 1;
        if (write(wake_fd, &one, sizeof one) < 0) {}
    }
}

struct Result {
    double main_ns = 0, window_ns = 0, end_ns = 0;
};

void tell_the_timeline(Row row, clock_type::time_point began, clock_type::time_point handed,
                       const std::vector<Mark> &seen);

Result run(Row row, bool trace = false)
{
    clock_type::time_point began;
    on_the_window([row, trace, &began] {
        if (frame_clock == nullptr) {   // the window is up and realized by now: its clock is there
            frame_clock = gtk_widget_get_frame_clock(terminal);
            g_signal_connect(frame_clock, "before-paint", G_CALLBACK(on_before_paint), nullptr);
            g_signal_connect(frame_clock, "update", G_CALLBACK(on_update), nullptr);
        }
        running = row;
        start_row = cursor_row();
        counting = true;
        marks.clear();
        trace_this_run = trace;
    });
    {
        const std::lock_guard<std::mutex> hold(done_lock);
        done = false;
    }
    window_ns.store(0);

    volatile double factor = 457474, times = 12349.474, plus = 500000;   // no folding at compile time
    began = clock_type::now();
    if (row == Row::five_steps || row == Row::once_a_frame) {
        for (int i = 0; i < kPrints; ++i)
            hand_over(Job{make_the_line()});
    } else if (row == Row::bypass) {
        for (int i = 0; i < kPrints; ++i)
            hand_over(Job{});
    } else {
        for (int i = 0; i < kPrints; ++i)
            std::cout << plus + times * factor << " hello, world!\n";
    }
    const clock_type::time_point handed = clock_type::now();
    if (row == Row::cout)
        std::cout.flush();

    std::unique_lock<std::mutex> hold(done_lock);
    if (!done_changed.wait_for(hold, std::chrono::seconds(30), [] { return done; })) {
        std::fprintf(stderr, "%s: VTE never showed all %d lines\n", name_of(row), kPrints);
        std::exit(1);
    }
    Result result;
    result.main_ns = ns_since(began, handed) / kPrints;
    result.window_ns = window_ns.load() / kPrints;
    result.end_ns = ns_since(began, done_at) / kPrints;
    hold.unlock();
    if (trace) {
        std::vector<Mark> seen;
        on_the_window([&seen] {
            trace_this_run = false;
            seen.swap(marks);
        });
        tell_the_timeline(row, began, handed, seen);
    }
    return result;
}

// ONE RUN'S TIMELINE: the loop's end, every frame with the feeds since the one before,
// and when VTE said its contents changed -- microseconds from the loop's start.
void tell_the_timeline(Row row, clock_type::time_point began, clock_type::time_point handed,
                       const std::vector<Mark> &seen)
{
    const auto us = [began](clock_type::time_point at) {
        return std::chrono::duration<double, std::micro>(at - began).count();
    };
    std::fprintf(stderr, "%s timeline (us from the loop's start): the loop ended at %.0f, done at %.0f\n", name_of(row),
                 us(handed), us(done_at));
    int feeds = 0, frames = 0, told = 0;
    long bytes = 0;
    std::string line = "   ";
    for (const Mark &m : seen) {
        if (m.what == 'f') {
            ++feeds;
            bytes += m.bytes;
            continue;
        }
        if (told++ > 60)
            continue;
        char one[96];
        if (m.what == 'u') {
            ++frames;
            std::snprintf(one, sizeof one, " frame@%.0f(%d feeds %ldB)", us(m.at), feeds, bytes);
            feeds = 0;
            bytes = 0;
        } else {
            std::snprintf(one, sizeof one, " changed@%.0f", us(m.at));
        }
        line += one;
    }
    if (feeds > 0) {
        char one[64];
        std::snprintf(one, sizeof one, " then %d feeds %ldB", feeds, bytes);
        line += one;
    }
    std::fprintf(stderr, "%s\n", line.c_str());
}

void the_race()
{
    std::this_thread::sleep_for(std::chrono::milliseconds(1500));   // the window up and drawn
    for (Row row : kRows)
        run(row);                                                    // once each, not counted
    std::vector<Result> results[4];
    for (int round = 0; round < kRounds; ++round) {
        for (Row row : kRows) {
            // NOT A WHOLE NUMBER OF FRAMES. GTK starts a frame on its ~16.7 ms cadence
            // (gdkframeclockidle.c's min_next_frame_time), and 300 ms is exactly 18 of them:
            // every round put each row at the same phase of it, so one row always got its
            // second frame 1.7 ms after the first and another always waited 16 ms. The pause
            // walks the rows across the whole frame instead.
            const int step = round * 4 + static_cast<int>(row);
            std::this_thread::sleep_for(std::chrono::microseconds(300000 + (step * 4177) % 16667));
            results[static_cast<int>(row)].push_back(run(row, tracing && round == kRounds - 1));
        }
    }
    std::fprintf(stderr, "%d prints a run, %d runs each, alternating; per print, median (best):\n", kPrints, kRounds);
    std::fprintf(stderr, "                main thread         window thread       end to end (VTE has it)\n");
    for (Row row : kRows) {
        std::vector<Result> &r = results[static_cast<int>(row)];
        const auto median = [&r](double Result::*field) {
            std::vector<double> v;
            for (const Result &one : r) v.push_back(one.*field);
            std::sort(v.begin(), v.end());
            return std::pair<double, double>{v[v.size() / 2], v.front()};
        };
        const auto m = median(&Result::main_ns), w = median(&Result::window_ns), e = median(&Result::end_ns);
        std::fprintf(stderr, "%s  %8.0f ns (%6.0f)   %8.0f ns (%6.0f)   %8.0f ns (%6.0f)\n", name_of(row), m.first,
                     m.second, w.first, w.second, e.first, e.second);
    }
    on_the_window([] { g_main_loop_quit(loop); });
}

} // namespace

int main()
{
    if (const char *prints = std::getenv("RACE_PRINTS"))
        kPrints = std::max(1, std::atoi(prints));
    if (const char *rounds = std::getenv("RACE_ROUNDS"))
        kRounds = std::max(1, std::atoi(rounds));
    tracing = std::getenv("RACE_TRACE") != nullptr;
    std::ios::sync_with_stdio(false);                     // as satl does
    std::cout << std::fixed << std::setprecision(3);
    {   // THE SAME TEXT EVERY WAY: satl printed "5650063268.676 hello, world!"
        volatile double factor = 457474, times = 12349.474, plus = 500000;
        std::ostringstream cpp;
        cpp << std::fixed << std::setprecision(3) << plus + times * factor << " hello, world!";
        std::fprintf(stderr, "A/B through satellite: [%s]\nC in C++:              [%s]\n",
                     make_the_line().to_utf8().c_str(), cpp.str().c_str());
    }
    wake_fd = eventfd(0, EFD_CLOEXEC | EFD_NONBLOCK);
    gtk_init();
    loop = g_main_loop_new(nullptr, FALSE);
    if (!make_the_window())
        return 1;
    std::thread race(the_race);
    g_main_loop_run(loop);
    race.join();
    return 0;
}
