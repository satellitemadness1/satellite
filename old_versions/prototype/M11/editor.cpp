// What each key does to the line editor buffer.
// Milestone 11 Prototype in prototype/M11.

#include "editor.hpp"
#include "history.hpp"

#include <utility>

namespace satellite {

Editor::Editor(const History *history) : history_(history)
{
    browse_ = history_ ? history_->size() : 0;
}

void Editor::reset()
{
    buffer_.clear();
    cursor_ = 0;
    live_.clear();
    browsing_ = false;
    browse_ = history_ ? history_->size() : 0;
}

void Editor::set_line(std::string text)
{
    buffer_ = std::move(text);
    cursor_ = buffer_.size();
}

void Editor::recall(size_t index)
{
    if (!history_ || index >= history_->size())
        return;
    buffer_ = history_->at(index);
    cursor_ = buffer_.size();
}

void Editor::kill_word_back()
{
    size_t end = cursor_;
    while (end > 0 && buffer_[prev_char(buffer_, end)] == ' ')
        end = prev_char(buffer_, end);
    while (end > 0 && buffer_[prev_char(buffer_, end)] != ' ')
        end = prev_char(buffer_, end);

    buffer_.erase(end, cursor_ - end);
    cursor_ = end;
}

Editor::Outcome Editor::apply(const KeyEvent &event)
{
    switch (event.key) {
    case Key::None:
    case Key::Ignored:
        return Outcome::Continue;

    case Key::Char:
        buffer_.insert(cursor_, event.text);
        cursor_ += event.text.size();
        return Outcome::Continue;

    case Key::Enter:
        return Outcome::Accept;

    case Key::Interrupt:
        return Outcome::Interrupt;

    case Key::EndOfInput:
        if (buffer_.empty())
            return Outcome::EndOfInput;
        if (cursor_ < buffer_.size())
            buffer_.erase(cursor_, next_char(buffer_, cursor_) - cursor_);
        return Outcome::Continue;

    case Key::Backspace: {
        if (cursor_ == 0)
            return Outcome::Continue;
        const size_t start = prev_char(buffer_, cursor_);
        buffer_.erase(start, cursor_ - start);
        cursor_ = start;
        return Outcome::Continue;
    }

    case Key::Delete:
        if (cursor_ < buffer_.size())
            buffer_.erase(cursor_, next_char(buffer_, cursor_) - cursor_);
        return Outcome::Continue;

    case Key::Left:
        cursor_ = prev_char(buffer_, cursor_);
        return Outcome::Continue;

    case Key::Right:
        cursor_ = next_char(buffer_, cursor_);
        return Outcome::Continue;

    case Key::Home:
        cursor_ = 0;
        return Outcome::Continue;

    case Key::End:
        cursor_ = buffer_.size();
        return Outcome::Continue;

    case Key::WordLeft:
        while (cursor_ > 0 && buffer_[prev_char(buffer_, cursor_)] == ' ')
            cursor_ = prev_char(buffer_, cursor_);
        while (cursor_ > 0 && buffer_[prev_char(buffer_, cursor_)] != ' ')
            cursor_ = prev_char(buffer_, cursor_);
        return Outcome::Continue;

    case Key::WordRight:
        while (cursor_ < buffer_.size() && buffer_[cursor_] != ' ')
            cursor_ = next_char(buffer_, cursor_);
        while (cursor_ < buffer_.size() && buffer_[cursor_] == ' ')
            cursor_ = next_char(buffer_, cursor_);
        return Outcome::Continue;

    case Key::KillToStart:
        buffer_.erase(0, cursor_);
        cursor_ = 0;
        return Outcome::Continue;

    case Key::KillToEnd:
        buffer_.erase(cursor_);
        return Outcome::Continue;

    case Key::KillWordBack:
        kill_word_back();
        return Outcome::Continue;

    case Key::ClearScreen:
        return Outcome::ClearScreen;

    case Key::Up:
        if (!history_ || browse_ == 0)
            return Outcome::Continue;
        if (!browsing_) {
            live_ = buffer_;
            browsing_ = true;
        }
        recall(--browse_);
        return Outcome::Continue;

    case Key::Down:
        if (!history_ || !browsing_)
            return Outcome::Continue;
        if (browse_ + 1 >= history_->size()) {
            browse_ = history_->size();
            browsing_ = false;
            buffer_ = live_;
            cursor_ = buffer_.size();
            return Outcome::Continue;
        }
        recall(++browse_);
        return Outcome::Continue;
    }

    return Outcome::Continue;
}

} // namespace satellite

