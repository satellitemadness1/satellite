// Reading one line. See line_reader.hpp.

#include "line_reader.hpp"

#include "editor.hpp"
#include "keys.hpp"
#include "raw_mode.hpp"
#include "render.hpp"

#include "../machine/shown.hpp"

#include <cerrno>
#include <csignal>
#include <poll.h>
#include <pthread.h>

namespace satellite004::prompt {

namespace {

constexpr std::size_t chunk = 65536;

// A byte is waiting, or arrives within `patience_ms` (-1: however long it takes).
bool byte_waiting(int in, int patience_ms)
{
    pollfd ask{in, POLLIN, 0};
    return poll(&ask, 1, patience_ms) > 0;
}

// More bytes than one keystroke sends (an escape sequence, a UTF-8 character):
// input arriving faster than a person types.
constexpr std::size_t keystroke_bytes = 16;

// SIGWINCH IS HELD OFF WHILE A LINE IS BEING READ, and let through only inside
// the wait (ppoll). A resize that lands between the check of the flag and the
// wait would otherwise leave the line drawn at the old width until the next key.
struct HoldResizes {
    sigset_t before;
    HoldResizes()
    {
        sigset_t resize;
        sigemptyset(&resize);
        sigaddset(&resize, SIGWINCH);
        pthread_sigmask(SIG_BLOCK, &resize, &before);
    }
    ~HoldResizes() { pthread_sigmask(SIG_SETMASK, &before, nullptr); }
};

} // namespace

LineReader::LineReader(int in, int out) : in_(in), out_(out), interactive_(is_interactive(in, out)) {}

void LineReader::remember(const std::string &line)
{
    history_.add(line);
}

void LineReader::discard_pending()
{
    pasted_.clear();
    carried_.clear();
    input_.clear();
    taken_ = scanned_ = 0;
}

void LineReader::drop_what_was_typed()
{
    KeyDecoder decoder;
    for (int patience = 0; byte_waiting(in_, patience);) {
        if (!fill())
            return;  // the end of input stays for the reader to find
        for (; taken_ < input_.size(); ++taken_)
            decoder.feed(static_cast<unsigned char>(input_[taken_]));
        patience = decoder.idle() ? 0 : 100;
    }
    last_read_ = 0;  // what was dropped says nothing about how the next line is typed
}

LineStatus LineReader::read(const std::string &prompt, std::string &line)
{
    line.clear();
    return interactive_ ? read_typed(prompt, line) : read_piped(line);
}

bool LineReader::fill()
{
    // THE UNUSED BYTES MOVE TO THE FRONT ONLY WHEN THEY ARE FEWER THAN THE USED
    // ONES, so a pipe of 100,000 short lines does not copy its buffer a line.
    if (taken_ == input_.size()) {
        input_.clear();
        taken_ = scanned_ = 0;
    } else if (taken_ > input_.size() - taken_) {
        input_.erase(0, taken_);
        scanned_ = scanned_ > taken_ ? scanned_ - taken_ : 0;
        taken_ = 0;
    }
    const std::size_t had = input_.size();
    input_.resize(had + chunk);
    for (;;) {
        const ssize_t got = ::read(in_, input_.data() + had, chunk);
        if (got < 0 && errno == EINTR)
            continue;
        if (got < 0 && (errno == EAGAIN || errno == EWOULDBLOCK) && byte_waiting(in_, -1))
            continue;
        last_read_ = got > 0 ? static_cast<std::size_t>(got) : 0;
        input_.resize(had + last_read_);
        return got > 0;
    }
}

LineStatus LineReader::read_piped(std::string &line)
{
    if (scanned_ < taken_)
        scanned_ = taken_;  // the typed path, which this falls back from, moves only taken_
    for (;;) {
        const std::size_t newline = input_.find('\n', scanned_);
        if (newline != std::string::npos) {
            line.assign(input_, taken_, newline - taken_);
            taken_ = scanned_ = newline + 1;
            return LineStatus::Line;
        }
        scanned_ = input_.size();
        if (!fill()) {
            if (taken_ == input_.size())
                return LineStatus::EndOfFile;
            line.assign(input_, taken_);
            taken_ = scanned_ = input_.size();
            return LineStatus::Line;
        }
    }
}

LineStatus LineReader::read_typed(const std::string &prompt, std::string &line)
{
    // A LINE A PASTE ALREADY BROUGHT is answered without reading, drawn after the
    // prompt as if it had been typed.
    if (!pasted_.empty()) {
        line = std::move(pasted_.front());
        pasted_.pop_front();
        write_all(out_, shown(prompt) + shown(line) + "\r\n");
        return LineStatus::Line;
    }

    RawMode raw(in_, out_);
    if (!raw.active())
        return read_piped(line);
    if (taken_ == input_.size())
        drop_what_was_typed();
    watch_resizes();
    const HoldResizes held;
    take_resize();  // the renderer below asks the width now

    Editor editor(&history_);
    editor.set_line(std::move(carried_));
    carried_.clear();
    KeyDecoder decoder;
    Renderer renderer(out_);
    bool pasting = false;
    bool drawn = false;

    // THE CURSOR GOES TO THE END OF THE LINE BEFORE THE NEWLINE, so whatever runs
    // next prints below all of a wrapped line, not over its last rows. A line that
    // filled its last row exactly is already below it (render.cpp's held wrap).
    const auto leave = [&](const std::string &text) {
        if (!drawn || editor.cursor() != text.size() || &text != &editor.line())
            renderer.draw(prompt, text, text.size());
        if (!renderer.below_the_text())
            write_all(out_, "\r\n");
    };

    for (;;) {
        if (taken_ == input_.size()) {
            if (take_resize()) {
                renderer.resize();
                drawn = false;
            }
            const int patience = last_read_ > keystroke_bytes ? 20 : 0;
            if (!drawn && !pasting && !byte_waiting(in_, patience)) {
                renderer.draw(prompt, editor.line(), editor.cursor());
                drawn = true;
            }
            pollfd ask{in_, POLLIN, 0};
            if (ppoll(&ask, 1, nullptr, &held.before) < 0) {
                if (errno == EINTR)
                    continue;
                write_all(out_, "\r\n");
                return LineStatus::EndOfFile;
            }
            if (!fill()) {  // the terminal closed: a hang-up, or the far end of the pty
                write_all(out_, "\r\n");
                return LineStatus::EndOfFile;
            }
        }

        KeyEvent event = decoder.feed(static_cast<unsigned char>(input_[taken_++]));
        const Key key = event.key;
        if (key == Key::None)
            continue;
        pasting = key == Key::PasteStart || (pasting && key != Key::PasteEnd);

        switch (editor.apply(event)) {
        case Editor::Outcome::Continue:
            drawn = false;
            if (key == Key::PasteEnd && !pasted_.empty()) {
                carried_ = editor.line();
                line = std::move(pasted_.front());
                pasted_.pop_front();
                leave(line);
                return LineStatus::Line;
            }
            break;

        case Editor::Outcome::Accept:
            if (key == Key::PasteLine) {
                pasted_.push_back(editor.line());
                editor.reset();
                break;
            }
            leave(editor.line());
            line = editor.line();
            return LineStatus::Line;

        // CTRL-C ABANDONS THE LINE AND KEEPS THE SESSION.
        case Editor::Outcome::Interrupt:
            leave(editor.line());
            return LineStatus::Interrupted;

        case Editor::Outcome::EndOfInput:
            leave(editor.line());
            return LineStatus::EndOfFile;

        case Editor::Outcome::ClearScreen:
            renderer.clear_screen();
            drawn = false;
            break;
        }
    }
}

} // namespace satellite004::prompt
