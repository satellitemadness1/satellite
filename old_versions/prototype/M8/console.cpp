// The satellite Console and Dedicated Printer Thread implementation.
//
// DESIGN §10.1: Dedicated background printer thread consuming strings from a
// thread-safe queue. Guarantees line atomicity across threads, provides drain()
// barriers before input prompts, and flushes stdout for un-newlined messages.

#include "console.hpp"

#include <cstdio>
#include <iostream>

namespace satellite {

Console &Console::instance()
{
    static Console s_instance;
    return s_instance;
}

Console::Console()
{
    start();
}

Console::~Console()
{
    stop();
}

void Console::start()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (running_.load(std::memory_order_acquire))
        return;

    running_.store(true, std::memory_order_release);
    printer_thread_ = std::thread(&Console::thread_loop, this);
}

void Console::stop()
{
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!running_.load(std::memory_order_acquire))
            return;
        running_.store(false, std::memory_order_release);
        cv_.notify_all();
    }

    if (printer_thread_.joinable()) {
        printer_thread_.join();
    }
}

void Console::display(const std::string &text, bool newline)
{
    std::string unit = newline ? (text + "\n") : text;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        ++next_seq_;
        queue_.push(Message{std::move(unit), next_seq_});

        if (capture_enabled_) {
            captured_.push_back(text);
        }
    }
    cv_.notify_one();
}

void Console::display_raw(const std::string &text)
{
    display(text, false);
}

void Console::drain()
{
    uint64_t target_seq = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        target_seq = next_seq_;
    }

    std::unique_lock<std::mutex> lock(mutex_);
    drain_cv_.wait(lock, [this, target_seq]() {
        return processed_seq_ >= target_seq;
    });

    // Explicitly flush destination standard output stream
    std::fflush(stdout);
}

void Console::set_sink(OutputSink sink)
{
    std::lock_guard<std::mutex> lock(mutex_);
    sink_ = std::move(sink);
}

void Console::reset_sink()
{
    std::lock_guard<std::mutex> lock(mutex_);
    sink_ = nullptr;
}

void Console::enable_capture(bool enable)
{
    std::lock_guard<std::mutex> lock(mutex_);
    capture_enabled_ = enable;
}

std::vector<std::string> Console::captured_lines() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return captured_;
}

void Console::clear_captured()
{
    std::lock_guard<std::mutex> lock(mutex_);
    captured_.clear();
}

size_t Console::pending_count() const
{
    std::lock_guard<std::mutex> lock(mutex_);
    return queue_.size();
}

void Console::thread_loop()
{
    while (true) {
        Message msg;
        OutputSink current_sink;

        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this]() {
                return !running_.load(std::memory_order_acquire) || !queue_.empty();
            });

            if (queue_.empty() && !running_.load(std::memory_order_acquire)) {
                break;
            }

            msg = std::move(queue_.front());
            queue_.pop();
            current_sink = sink_;
        }

        // Output unit without holding the lock
        if (current_sink) {
            current_sink(msg.text);
        } else {
            std::fwrite(msg.text.data(), 1, msg.text.size(), stdout);
            std::fflush(stdout);
        }

        // Mark processed under lock and notify waiting drain() calls
        {
            std::lock_guard<std::mutex> lock(mutex_);
            processed_seq_ = msg.seq;
            drain_cv_.notify_all();
        }
    }
}

} // namespace satellite

