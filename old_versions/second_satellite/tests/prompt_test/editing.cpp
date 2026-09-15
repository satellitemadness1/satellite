// The buffer, the cursor, and history browsing. See prompt_test.hpp clause 2.

#include "prompt_test.hpp"

#include "satellite_prompt/editor.hpp"
#include "satellite_prompt/history.hpp"
#include "satellite_prompt/keys.hpp"

#include <string>

using namespace satellite::prompt;

namespace prompt_test {

namespace {

KeyEvent character(const std::string &text)
{
    KeyEvent event;
    event.key = Key::Char;
    event.text = text;
    return event;
}

KeyEvent plain(Key key)
{
    KeyEvent event;
    event.key = key;
    return event;
}

void type(Editor &editor, const std::string &text)
{
    for (const char c : text)
        editor.apply(character(std::string(1, c)));
}

void press(Editor &editor, Key key, int times = 1)
{
    for (int i = 0; i < times; i++)
        editor.apply(plain(key));
}

} // namespace

void section_editing()
{
    // 1 -- TYPING, AND THE CURSOR FOLLOWS.
    {
        Editor editor;
        type(editor, "display");
        check(editor.line() == "display", "typing builds the line");
        check(editor.cursor() == 7, "the cursor is at the end");
    }

    // 2 -- INSERTING IN THE MIDDLE, which is the case a naive append breaks.
    {
        Editor editor;
        type(editor, "dislay");
        press(editor, Key::Left, 3);
        type(editor, "p");
        check(editor.line() == "display", "an insert lands at the cursor");
    }

    // 3 -- BACKSPACE TAKES THE CHARACTER BEFORE, DELETE THE ONE AT.
    {
        Editor editor;
        type(editor, "abc");
        press(editor, Key::Backspace);
        check(editor.line() == "ab", "backspace takes the last character");
        press(editor, Key::Home);
        press(editor, Key::Delete);
        check(editor.line() == "b", "delete takes the character at the cursor");
        press(editor, Key::Backspace);
        check(editor.line() == "b", "backspace at the start does nothing");
    }

    // 4 -- BACKSPACE OVER A MULTI-BYTE CHARACTER TAKES ALL OF IT. A byte-wise
    // erase would leave a lead byte behind and the line would render as a
    // replacement glyph from then on.
    {
        Editor editor;
        type(editor, "a");
        editor.apply(character("\xC3\xA9"));
        check(editor.line() == "a\xC3\xA9", "the character is in the line");
        press(editor, Key::Backspace);
        check(editor.line() == "a", "backspace removed both of its bytes");
    }

    // 5 -- WORD MOVES STOP AT SPACES AND NOT AT DOTS, which is the decision
    // editor.cpp writes down: a satellite path is one word.
    {
        Editor editor;
        type(editor, "satellite.console.display hello");
        press(editor, Key::WordLeft);
        check(editor.cursor() == 26, "word-left stops at the space, not a dot");
        press(editor, Key::WordLeft);
        check(editor.cursor() == 0, "the whole path is one word");
    }

    // 6 -- THE KILLS.
    {
        Editor editor;
        type(editor, "one two three");
        press(editor, Key::KillWordBack);
        check(editor.line() == "one two ", "Ctrl-W takes the last word");
        press(editor, Key::KillWordBack);
        check(editor.line() == "one ", "and the space before it");

        Editor other;
        type(other, "keep this");
        press(other, Key::Home);
        press(other, Key::KillToEnd);
        check(other.line().empty(), "Ctrl-K from the start empties the line");

        Editor third;
        type(third, "drop this");
        press(third, Key::KillToStart);
        check(third.line().empty(), "Ctrl-U to the end empties the line");
    }

    // 7 -- THE OUTCOMES. Enter accepts; Ctrl-C interrupts; Ctrl-D ends the
    // session ONLY on an empty line and is a forward delete otherwise.
    {
        Editor editor;
        check(editor.apply(plain(Key::Enter)) == Editor::Outcome::Accept,
              "Enter accepts");
        check(editor.apply(plain(Key::Interrupt)) == Editor::Outcome::Interrupt,
              "Ctrl-C interrupts");
        check(editor.apply(plain(Key::EndOfInput)) == Editor::Outcome::EndOfInput,
              "Ctrl-D on an empty line ends the session");

        type(editor, "abc");
        press(editor, Key::Home);
        check(editor.apply(plain(Key::EndOfInput)) == Editor::Outcome::Continue,
              "Ctrl-D on a line with text does not end the session");
        check(editor.line() == "bc", "it deleted forwards instead");
    }

    // 8 -- HISTORY: UP RECALLS, DOWN COMES BACK, AND THE HALF-TYPED LINE
    // SURVIVES THE ROUND TRIP. The last clause is the one people notice when
    // it is missing.
    {
        History history;
        history.add("first");
        history.add("second");

        Editor editor(&history);
        type(editor, "half typed");
        press(editor, Key::Up);
        check(editor.line() == "second", "up recalls the newest entry");
        press(editor, Key::Up);
        check(editor.line() == "first", "up again reaches the one before");
        press(editor, Key::Up);
        check(editor.line() == "first", "up past the oldest stays there");
        press(editor, Key::Down);
        check(editor.line() == "second", "down comes forward again");
        press(editor, Key::Down);
        check(editor.line() == "half typed",
              "down past the newest restores what was being typed");
    }

    // 9 -- HISTORY DOES NOT STORE A LINE TWICE IN A ROW, NOR AN EMPTY ONE.
    {
        History history;
        history.add("same");
        history.add("same");
        check(history.size() == 1, "a repeated line is stored once");
        history.add("");
        check(history.size() == 1, "an empty line is not stored");
        history.add("other");
        history.add("same");
        check(history.size() == 3, "a line repeated LATER is stored again");
    }
}

} // namespace prompt_test
