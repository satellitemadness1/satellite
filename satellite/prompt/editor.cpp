// What each key does to the line. See editor.hpp.

#include "editor.hpp"

#include "console_lines.hpp"
#include "earlier_consoles.hpp"

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

Editor::Editor(const ConsoleLines *lines, EarlierConsoles *earlier) : lines_(lines), earlier_(earlier)
{
    browse_ = lines_ != nullptr ? lines_->stops() : 0;
}

void Editor::reset()
{
    buffer_.clear();
    cursor_ = 0;
    live_.clear();
    browsing_ = false;
    shown_ = 0;
    browse_ = lines_ != nullptr ? lines_->stops() : 0;
}

void Editor::set_line(std::string text)
{
    buffer_ = std::move(text);
    cursor_ = buffer_.size();
}

// A LINE BROUGHT BACK IS ITS UTF-8, every byte as it was typed, less what the console's lines never
// keep (console_lines.hpp's what_is_kept) -- the Editor's line stays bytes -- with the cursor at its end.
// It is a line of the console shown (KEY-6): this console's, or one before it.
void Editor::recall(std::size_t index)
{
    const ConsoleLines *const lines = console(shown_);
    if (lines == nullptr || index >= lines->stops())
        return;
    buffer_ = lines->stop_at(index);
    cursor_ = buffer_.size();
}

const ConsoleLines *Editor::console(std::size_t shown) const
{
    if (shown == 0)
        return lines_;
    return earlier_ != nullptr ? earlier_->lines(shown - 1) : nullptr;
}

// THE HALF-TYPED LINE IS KEPT AS UP KEEPS IT, the first time this console is left -- by Up, or by Left
// past the line's start -- and only then: a line Up brought back is this console's, never what was being
// typed. The console is shown at its last stop, as Up shows this console first at its newest.
void Editor::show(std::size_t shown, const ConsoleLines &lines)
{
    if (!browsing_) {
        live_ = buffer_;
        browsing_ = true;
    }
    shown_ = shown;
    browse_ = lines.stops() - 1;
    recall(browse_);
}

void Editor::back_to_this_console()
{
    shown_ = 0;
    browse_ = lines_ != nullptr ? lines_->stops() : 0;
    browsing_ = false;
    buffer_ = live_;
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

    // LEFT MOVES BACK THROUGH THE LINE, AND FROM ITS START TO THE CONSOLE BEFORE (KEY_PLAN.md KEY-6, rule 5;
    // the author's answer 6 of 2026-10-20: "it moves the cursor through the input that has already been
    // typed, until it is all the way to the left, then it switches at last to the left most set of inputs
    // from the previous console file"). That console is shown at its last line, with the cursor at the
    // line's end, so Left goes on through it. An empty line has its cursor at the start already, and Home
    // puts it there. A console with no line to stop at -- every line empty, spaces or a repeat, or its
    // file gone -- is stepped over; at the oldest, Left does nothing, as at a line's start before.
    case Key::Left:
        if (cursor_ > 0) {
            cursor_ = previous_character(buffer_, cursor_);
            return Outcome::Continue;
        }
        for (std::size_t before = shown_ + 1; earlier_ != nullptr && before <= earlier_->size(); ++before)
            if (const ConsoleLines *lines = console(before); lines != nullptr && lines->stops() > 0) {
                show(before, *lines);
                return Outcome::Continue;
            }
        return Outcome::Continue;

    // RIGHT MOVES FORWARD THROUGH THE LINE, AND FROM ITS END TO THE CONSOLE AFTER (rule 5), the mirror of
    // Left: shown at its last line, as Left showed it, with the cursor at the line's start. Past the
    // newest console before this one comes this console, and the half-typed line returns, the cursor at
    // its start. At the end of this console's own line, Right does nothing, as before.
    case Key::Right:
        if (cursor_ < buffer_.size()) {
            cursor_ = next_character(buffer_, cursor_);
            return Outcome::Continue;
        }
        if (shown_ == 0)
            return Outcome::Continue;
        for (std::size_t after = shown_ - 1; after > 0; --after)
            if (const ConsoleLines *lines = console(after); lines != nullptr && lines->stops() > 0) {
                show(after, *lines);
                cursor_ = 0;
                return Outcome::Continue;
            }
        back_to_this_console();
        cursor_ = 0;
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

    // UP KEEPS THE HALF-TYPED LINE AND GOES BACK ONE STOP, and stops at the first (KEY_PLAN.md rule 4) --
    // of the console shown, this one or one before it (KEY-6). Up never goes on into another console.
    case Key::Up:
        if (console(shown_) == nullptr || browse_ == 0)
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
    // AND PAST THE LAST LINE OF A CONSOLE BEFORE THIS ONE, the same: this console, and the half-typed
    // line (KEY-6), never the next console's first line.
    case Key::Down: {
        if (!browsing_)
            return Outcome::Continue;
        const ConsoleLines *const lines = console(shown_);
        if (lines == nullptr || browse_ + 1 >= lines->stops()) {
            back_to_this_console();
            return Outcome::Continue;
        }
        recall(++browse_);
        return Outcome::Continue;
    }
    }

    return Outcome::Continue;
}

} // namespace satellite004::prompt
