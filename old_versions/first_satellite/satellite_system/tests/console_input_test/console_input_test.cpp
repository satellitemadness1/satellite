// console_input: the byte decoder, the editing, the measuring, and the
// history.
//
// EVERY PIECE OF THE MODULE EXCEPT THE READ LOOP IS DRIVEN HERE, WITH NO
// TERMINAL ANYWHERE. That is not a happy accident of how it was written -- it
// is the reason keys.cpp, editor.cpp, render.cpp and history.cpp exist as
// separate files from line_reader.cpp at all. An arrow key is three bytes that
// arrive one read() at a time; the only way to test that decoding without a
// pty is for the decoding to be a function of bytes, so it is one.
//
// What this CANNOT reach is line_reader.cpp's loop and raw_mode.cpp's termios
// calls, both of which need a real terminal. What is left in them after
// everything above was lifted out is a read(), a switch, and a tcsetattr --
// which is as small as that untested surface can be made.
//
// This file is the harness and main(); the cases are in the three beside it.

#include "console_input_test.hpp"

#include <cstdio>

using namespace satellite;

int failures = 0;

void check(bool ok, const char *what)
{
    if (!ok) {
        printf("FAIL: %s\n", what);
        failures++;
    }
}

void check_eq(const std::string &got, const std::string &want,
              const char *what)
{
    if (got != want) {
        printf("FAIL: %s\n  got  [%s]\n  want [%s]\n", what, got.c_str(),
               want.c_str());
        failures++;
    }
}

std::vector<KeyEvent> decode(const std::string &bytes)
{
    KeyDecoder decoder;
    std::vector<KeyEvent> events;
    for (unsigned char byte : bytes) {
        KeyEvent event = decoder.feed(byte);
        if (event.key != Key::None)
            events.push_back(event);
    }
    return events;
}

Key one_key(const std::string &bytes)
{
    std::vector<KeyEvent> events = decode(bytes);
    return events.size() == 1 ? events[0].key : Key::None;
}

std::string type(Editor &editor, const std::string &bytes)
{
    KeyDecoder decoder;
    for (unsigned char byte : bytes) {
        KeyEvent event = decoder.feed(byte);
        if (event.key != Key::None)
            editor.apply(event);
    }
    std::string shown = editor.line();
    shown.insert(editor.cursor(), "|");
    return shown;
}

int main()
{
    test_keys();
    test_measuring();
    test_editing();
    test_history_browsing();
    test_history_store();

    printf("console_input_test: %s\n", failures == 0 ? "PASS" : "FAIL");
    return failures == 0 ? 0 : 1;
}
