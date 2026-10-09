// What each key does to the line. See editor.hpp.

#include "editor.hpp"

#include "history.hpp"

#include <utility>

namespace satellite004::prompt {

namespace {

// A WORD BOUNDARY IS A SPACE AND NOTHING CLEVERER, and that is the right rule
// for this language rather than a simplification of a better one. A satellite
// path is `satellite.console.display` -- one word, written with dots -- and a
// word-left that stopped at every dot would take five presses to cross what a
// person reads as one name. Space is what separates the things a user means.
bool is_space(char c) { return c == ' '; }

} // namespace

Editor::Editor(const History *history) : history_(history)
{
    browse_ = history_ != nullptr ? history_->size() : 0;
}

void Editor::reset()
{
    buffer_.clear();
    cursor_ = 0;
    live_.clear();
    browsing_ = false;
    browse_ = history_ != nullptr ? history_->size() : 0;
}

void Editor::set_line(std::string text)
{
    buffer_ = std::move(text);
    cursor_ = buffer_.size();
}

void Editor::recall(std::size_t index)
{
    if (history_ == nullptr || index >= history_->size())
        return;
    buffer_ = history_->at(index);
    cursor_ = buffer_.size();
}

void Editor::kill_word_back()
{
    // Two loops and not one: skip the spaces the cursor is sitting in, THEN
    // take the word. Ctrl-W at the end of `display   ` deletes the spaces and
    // the word, which is what every other line editor does and what a user who
    // has just typed a trailing space expects.
    std::size_t end = cursor_;
    while (end > 0 && is_space(buffer_[previous_character(buffer_, end)]))
        end = previous_character(buffer_, end);
    while (end > 0 && !is_space(buffer_[previous_character(buffer_, end)]))
        end = previous_character(buffer_, end);

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
        // A } TYPED ON A LINE THAT IS ONLY ITS INDENTATION steps back one level first: the
        // prompt starts a block's next line one level in (session.cpp), and the } that closes
        // the block belongs under the line that opened it (2026-09-25).
        if (event.text == "}" && cursor_ == buffer_.size() && buffer_.size() >= 4 &&
            buffer_.find_first_not_of(' ') == std::string::npos) {
            buffer_.erase(buffer_.size() - 4);
            cursor_ = buffer_.size();
        }
        buffer_.insert(cursor_, event.text);
        cursor_ += event.text.size();
        return Outcome::Continue;

    case Key::Enter:
        return Outcome::Accept;

    // A PASTE IS INSERTED AS IT COMES, and a pasted newline accepts the line the
    // way Enter does. PasteEnd brings the text after the last newline, which
    // waits for the person to finish it.
    case Key::PasteStart:
        return Outcome::Continue;

    case Key::PasteLine:
    case Key::PasteEnd:
        buffer_.insert(cursor_, event.text);
        cursor_ += event.text.size();
        return event.key == Key::PasteLine ? Outcome::Accept : Outcome::Continue;

    case Key::Interrupt:
        return Outcome::Interrupt;

    // CTRL-D IS TWO KEYS DEPENDING ON WHERE IT IS PRESSED, which is inherited
    // from every shell rather than invented: on an empty line it ends the
    // session, and anywhere else it is a forward delete. Merging them into one
    // meaning would either make the session impossible to leave with the key
    // everybody reaches for, or make a mis-press end it.
    case Key::EndOfInput:
        if (buffer_.empty())
            return Outcome::EndOfInput;
        if (cursor_ < buffer_.size())
            buffer_.erase(cursor_, next_character(buffer_, cursor_) - cursor_);
        return Outcome::Continue;

    case Key::Backspace: {
        if (cursor_ == 0)
            return Outcome::Continue;
        const std::size_t start = previous_character(buffer_, cursor_);
        buffer_.erase(start, cursor_ - start);
        cursor_ = start;
        return Outcome::Continue;
    }

    case Key::Delete:
        if (cursor_ < buffer_.size())
            buffer_.erase(cursor_, next_character(buffer_, cursor_) - cursor_);
        return Outcome::Continue;

    case Key::Left:
        cursor_ = previous_character(buffer_, cursor_);
        return Outcome::Continue;

    case Key::Right:
        cursor_ = next_character(buffer_, cursor_);
        return Outcome::Continue;

    case Key::Home:
        cursor_ = 0;
        return Outcome::Continue;

    case Key::End:
        cursor_ = buffer_.size();
        return Outcome::Continue;

    case Key::WordLeft:
        while (cursor_ > 0 && is_space(buffer_[previous_character(buffer_, cursor_)]))
            cursor_ = previous_character(buffer_, cursor_);
        while (cursor_ > 0 && !is_space(buffer_[previous_character(buffer_, cursor_)]))
            cursor_ = previous_character(buffer_, cursor_);
        return Outcome::Continue;

    case Key::WordRight:
        while (cursor_ < buffer_.size() && !is_space(buffer_[cursor_]))
            cursor_ = next_character(buffer_, cursor_);
        while (cursor_ < buffer_.size() && is_space(buffer_[cursor_]))
            cursor_ = next_character(buffer_, cursor_);
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
        if (history_ == nullptr || browse_ == 0)
            return Outcome::Continue;
        if (!browsing_) {
            live_ = buffer_;
            browsing_ = true;
        }
        recall(--browse_);
        return Outcome::Continue;

    // DOWN PAST THE NEWEST ENTRY RETURNS THE HALF-TYPED LINE rather than
    // stopping at the last one. Stopping there strands whatever was being
    // typed when Up was first pressed, with no key that brings it back.
    case Key::Down:
        if (history_ == nullptr || !browsing_)
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

} // namespace satellite004::prompt
