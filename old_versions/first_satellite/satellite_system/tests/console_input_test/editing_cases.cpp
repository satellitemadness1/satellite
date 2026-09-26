// What each key does to the line: inserting at the cursor, both kinds of
// delete, the four kill commands, and the two meanings of Ctrl-D.
//
// Part of console_input_test. See console_input_test.hpp.

#include "console_input_test.hpp"

using namespace satellite;

void test_editing()
{
    {
        Editor editor;
        check_eq(type(editor, "abc"), "abc|", "typing lands at the cursor");
    }
    {
        Editor editor;
        // Left twice, then insert: the character goes in the MIDDLE, which is
        // the whole reason a cursor exists.
        check_eq(type(editor, "abc\033[D\033[DX"), "aX|bc",
                 "left arrow then a character inserts in the middle");
    }
    {
        Editor editor;
        check_eq(type(editor, "abc\x7f"), "ab|", "backspace deletes behind");
    }
    {
        Editor editor;
        check_eq(type(editor, "abc\033[D\033[3~"), "ab|",
                 "delete removes the character AT the cursor");
    }
    {
        Editor editor;
        check_eq(type(editor, "abc\x01"), "|abc", "Ctrl-A goes to the start");
        check_eq(type(editor, "\x05"), "abc|", "Ctrl-E goes to the end");
    }
    {
        Editor editor;
        check_eq(type(editor, "one two\x17"), "one |",
                 "Ctrl-W kills the word behind");
    }
    {
        Editor editor;
        // Trailing spaces first, then the word -- which is what makes Ctrl-W
        // useful at the end of a line that ends in one.
        check_eq(type(editor, "one two   \x17"), "one |",
                 "Ctrl-W steps over trailing spaces first");
    }
    {
        Editor editor;
        check_eq(type(editor, "abcdef\033[D\033[D\x15"), "|ef",
                 "Ctrl-U kills back to the start");
    }
    {
        Editor editor;
        check_eq(type(editor, "abcdef\033[D\033[D\x0b"), "abcd|",
                 "Ctrl-K kills forward to the end");
    }
    {
        // A backspace over a multi-byte character removes the whole character
        // and not one byte of it, which would leave an invalid string on
        // screen and in the buffer.
        Editor editor;
        check_eq(type(editor, "a\xc3\xa9\x7f"), "a|",
                 "backspace removes a whole multi-byte character");
    }

    // Ctrl-D means two different things, and which one depends on whether
    // there is anything on the line. That is the convention every prompt has,
    // and the reason is that a Ctrl-D typed by accident mid-line must not end
    // the session.
    {
        Editor editor;
        KeyEvent eof;
        eof.key = Key::EndOfInput;
        check(editor.apply(eof) == Editor::Outcome::EndOfInput,
              "Ctrl-D on an empty line is end of input");
    }
    {
        Editor editor;
        editor.set_line("abc");
        KeyEvent left;
        left.key = Key::Left;
        editor.apply(left);
        KeyEvent eof;
        eof.key = Key::EndOfInput;
        check(editor.apply(eof) == Editor::Outcome::Continue,
              "Ctrl-D on a line with text does not end the session");
        check_eq(editor.line(), "ab", "Ctrl-D mid-line deletes at the cursor");
    }

    {
        Editor editor;
        KeyEvent enter;
        enter.key = Key::Enter;
        check(editor.apply(enter) == Editor::Outcome::Accept, "Enter accepts");
        KeyEvent interrupt;
        interrupt.key = Key::Interrupt;
        check(editor.apply(interrupt) == Editor::Outcome::Interrupt,
              "Ctrl-C abandons the line");
    }
}
