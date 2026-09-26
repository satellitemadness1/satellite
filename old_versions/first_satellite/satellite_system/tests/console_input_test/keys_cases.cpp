// The byte -> key state machine, and the two measurements every redraw rests
// on: how wide a string is, and where the previous and next characters start.
//
// Part of console_input_test. See console_input_test.hpp.

#include "console_input_test.hpp"

using namespace satellite;

void test_keys()
{
    // The control characters, both spellings of backspace included: a terminal
    // sends 0x7f for the key marked Backspace on nearly every keyboard and
    // 0x08 on the rest, and there is no way to ask which this one is.
    check(one_key("\r") == Key::Enter, "CR is Enter");
    check(one_key("\n") == Key::Enter, "LF is Enter");
    check(one_key("\x7f") == Key::Backspace, "DEL is Backspace");
    check(one_key("\x08") == Key::Backspace, "Ctrl-H is Backspace");
    check(one_key("\x03") == Key::Interrupt, "Ctrl-C is Interrupt");
    check(one_key("\x04") == Key::EndOfInput, "Ctrl-D is EndOfInput");
    check(one_key("\x01") == Key::Home, "Ctrl-A is Home");
    check(one_key("\x05") == Key::End, "Ctrl-E is End");
    check(one_key("\x0c") == Key::ClearScreen, "Ctrl-L clears the screen");

    // The arrows, in BOTH forms a terminal sends them. CSI is the ordinary
    // one; SS3 is what a terminal in application-cursor mode sends, which is
    // the mode vi and less leave behind, so a reader that handled only CSI
    // would work until somebody quit less.
    check(one_key("\033[A") == Key::Up,    "CSI A is Up");
    check(one_key("\033[B") == Key::Down,  "CSI B is Down");
    check(one_key("\033[C") == Key::Right, "CSI C is Right");
    check(one_key("\033[D") == Key::Left,  "CSI D is Left");
    check(one_key("\033OA") == Key::Up,    "SS3 A is Up");
    check(one_key("\033OD") == Key::Left,  "SS3 D is Left");

    // Home, End and Delete have a numeric spelling as well as a letter one,
    // and which a terminal sends is a property of the terminal.
    check(one_key("\033[H")  == Key::Home,   "CSI H is Home");
    check(one_key("\033[F")  == Key::End,    "CSI F is End");
    check(one_key("\033[1~") == Key::Home,   "CSI 1~ is Home");
    check(one_key("\033[4~") == Key::End,    "CSI 4~ is End");
    check(one_key("\033[3~") == Key::Delete, "CSI 3~ is Delete");

    // Ctrl and Alt with the arrows, which is how a word is skipped.
    check(one_key("\033[1;5C") == Key::WordRight, "Ctrl-Right is WordRight");
    check(one_key("\033[1;5D") == Key::WordLeft,  "Ctrl-Left is WordLeft");
    check(one_key("\033b") == Key::WordLeft,  "Alt-b is WordLeft");
    check(one_key("\033f") == Key::WordRight, "Alt-f is WordRight");

    // Page Up decodes cleanly and means nothing. Ignored is not None: the
    // difference is "that was a whole key and it does nothing" against "ask me
    // again after another byte", and a reader that confused them would hang.
    check(one_key("\033[5~") == Key::Ignored, "Page Up is a key that does nothing");

    // A tab is four spaces, because every column this module counts is a
    // character and a tab's width depends on where it lands.
    {
        std::vector<KeyEvent> events = decode("\t");
        check(events.size() == 1 && events[0].key == Key::Char, "tab is a Char");
        if (events.size() == 1)
            check_eq(events[0].text, "    ", "tab is four spaces");
    }

    // A multi-byte character is assembled from its bytes and delivered whole,
    // which is what keeps a left arrow from landing inside one.
    {
        std::vector<KeyEvent> events = decode("\xc3\xa9");   // e-acute, 2 bytes
        check(events.size() == 1, "a 2-byte character is one key");
        if (events.size() == 1)
            check_eq(events[0].text, "\xc3\xa9", "the whole character arrives");
    }
    {
        std::vector<KeyEvent> events = decode("\xe2\x9c\x93");  // check mark
        check(events.size() == 1, "a 3-byte character is one key");
    }

    // A truncated character does not swallow the byte that truncated it. A
    // dropped character is a nuisance; a swallowed keystroke is a bug.
    {
        std::vector<KeyEvent> events = decode("\xc3" "a");
        check(events.size() == 1 && events[0].key == Key::Char,
              "a truncated character does not eat the next byte");
        if (events.size() == 1)
            check_eq(events[0].text, "a", "the byte after a truncation survives");
    }

    // reset() throws away a half-read sequence, which is what the read loop
    // does when a signal cuts a read short: the bytes that would have finished
    // the escape are not coming, and leaving them to combine with the next
    // keystroke is how a stray key becomes a phantom arrow.
    {
        KeyDecoder decoder;
        decoder.feed(0x1b);
        decoder.feed('[');
        decoder.reset();
        check(decoder.feed('A').key == Key::Char,
              "after reset, a stray A is the letter A and not an Up arrow");
    }
}

void test_measuring()
{
    check(display_width("abc") == 3, "three bytes are three columns");
    check(display_width("") == 0, "nothing is no columns");

    // A multi-byte character is ONE column and not two or three, which is what
    // keeps the cursor in the right place on a line containing one.
    check(display_width("\xc3\xa9") == 1, "a 2-byte character is one column");
    check(display_width("a\xe2\x9c\x93z") == 3, "a 3-byte character is one column");

    // THE PROMPT'S ESCAPES ARE NOT COLUMNS. This is the one that matters most
    // in practice: the prompt is bold-on, a username, bold-off, and counting
    // those eight bytes would put the cursor eight columns right of where the
    // user is typing, on every single redraw.
    check(display_width("\033[1m") == 0, "an SGR sequence is no columns at all");
    check(display_width("\033[1mroot\033[0m") == 4,
          "a bold username is as wide as the username");
    check(display_width("\033[1;38;2;255;255;255mx\033[0m") == 1,
          "a 24-bit colour sequence is no columns either");

    // Moving by CHARACTERS and never by bytes.
    const std::string text = "a\xc3\xa9z";     // 4 bytes, 3 characters
    check(next_char(text, 0) == 1, "next over an ascii byte");
    check(next_char(text, 1) == 3, "next skips a 2-byte character whole");
    check(prev_char(text, 3) == 1, "prev skips a 2-byte character whole");
    check(prev_char(text, 0) == 0, "prev clamps at the start");
    check(next_char(text, 4) == 4, "next clamps at the end");
}
