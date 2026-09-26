// What each key does to the line. No terminal appears in this file; see
// editor.hpp for why that is the point rather than an accident.

#include "console_input/editor.hpp"
#include "console_input/history.hpp"

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

// Replaces the line with history entry `index`, cursor at the end.
//
// The cursor goes to the END and not to where it was, because a recalled line
// is one the user is about to either run or edit the tail of -- and putting it
// anywhere else means a line recalled while the cursor sat at column 3 comes
// back cut off at column 3 as far as the eye is concerned.
void Editor::recall(size_t index)
{
    if (!history_ || index >= history_->size())
        return;
    buffer_ = history_->at(index);
    cursor_ = buffer_.size();
}

// Ctrl-W: back over any run of spaces, then back over the word before it.
// Spaces first is what makes it useful at the end of a line that ends in one.
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
        // Ctrl-D on an empty line is end of input; on a line with anything in
        // it, it deletes the character at the cursor. That is the convention
        // every prompt has, and the reason for it is that a Ctrl-D typed by
        // accident mid-line should not end the session.
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
        // The live line is saved on the way OUT of it and not on every press,
        // or the second Up would overwrite it with the first entry recalled.
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
            // Back past the newest entry is back to the line that was being
            // typed when Up was first pressed -- not an empty line, and not
            // the newest entry a second time.
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
