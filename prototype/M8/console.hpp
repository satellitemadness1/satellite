#pragma once

// The satellite Console and Dedicated Printer Thread -- Milestone 8 Prototype.
//
// DESIGN §10.1: Producer threads push whole strings into a locked queue; one
// printer thread consumes. A line stays atomic because the unit queued is a
// whole string.
//
// There is a drain() barrier before reading input, so a prompt cannot appear
// before the output that explains it. Output is flushed to handle pipes, files,
// and prompts without trailing newlines.

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace satellite {

class Console {
public:
    using OutputSink = std::function<void(const std::string &)>;

    // Singleton instance for global console access
    static Console &instance();

    Console();
    ~Console();

    // Non-copyable, non-movable (owns thread and mutexes)
    Console(const Console &) = delete;
    Console &operator=(const Console &) = delete;
    Console(Console &&) = delete;
    Console &operator=(Console &&) = delete;

    // Display output. When newline is true, appends a newline to the text.
    // The message is queued atomically and processed by the background printer thread.
    void display(const std::string &text, bool newline = true);

    // Raw display without appending newline (e.g. for prompts or un-newlined display under 1 5 1).
    void display_raw(const std::string &text);

    // DESIGN §10.1: drain() barrier blocks until all queued items up to this
    // moment have been printed and flushed to the destination stream.
    void drain();

    // Shut down the printer thread cleanly, flushing all pending output.
    void stop();

    // Start or restart the printer thread if not currently running.
    void start();

    // Custom output sink redirection for testing or embedded environments.
    void set_sink(OutputSink sink);
    void reset_sink();

    // Test capture functionality
    void enable_capture(bool enable = true);
    std::vector<std::string> captured_lines() const;
    void clear_captured();

    // State inspection
    bool is_running() const { return running_.load(std::memory_order_acquire); }
    size_t pending_count() const;

private:
    void thread_loop();

    struct Message {
        std::string text;
        uint64_t seq;
    };

    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::condition_variable drain_cv_;

    std::queue<Message> queue_;
    uint64_t next_seq_ = 0;
    uint64_t processed_seq_ = 0;

    std::atomic<bool> running_{false};
    std::thread printer_thread_;

    OutputSink sink_;
    bool capture_enabled_ = false;
    std::vector<std::string> captured_;
};

} // namespace satellite

