#pragma once

#include <string>

// Bytes from a terminal, turned into keys.
//
// This is a PURE state machine with no file descriptor in it, and that is the
// design rather than a tidiness: an arrow key is three bytes and a Ctrl-arrow
// is six, they arrive one read() at a time, and the only way to test that
// decoding is to be able to feed it bytes from a test rather than from a
// keyboard. Everything in console_input/ that can be a function of bytes is
// one, and line_reader.cpp -- the only file here that touches a terminal --
// is what is left over.
//
// WHAT A TERMINAL ACTUALLY SENDS, since the table below is otherwise a list of
// magic numbers. A control character is the letter with bit 6 and 7 cleared,
// so Ctrl-A is 1 and Ctrl-Z is 26. A function or arrow key is an ESCAPE
// SEQUENCE: ESC, then '[' (CSI) or 'O' (SS3, which is what a terminal in
// "application cursor" mode sends -- vi and less put terminals into it, so a
// reader that handles only CSI works until it does not), then optional
// numeric parameters separated by ';', then one final letter. Both forms are
// decoded here because both are sent by terminals people actually use.

namespace satellite {

enum class Key {
    // The decoder has consumed the byte and has no key yet: it is partway
    // through an escape sequence or a multi-byte character. NOT an error.
    None,

    Char,        // `text` holds one whole character -- one byte, or a UTF-8 run
    Enter,
    Backspace,   // delete the character BEFORE the cursor
    Delete,      // delete the character AT the cursor
    Left,
    Right,
    Up,
    Down,
    Home,
    End,
    WordLeft,
    WordRight,
    KillToStart,
    KillToEnd,
    KillWordBack,
    ClearScreen,
    Interrupt,    // Ctrl-C
    EndOfInput,   // Ctrl-D

    // A key that was decoded successfully and means nothing here -- Page Up,
    // an unknown escape sequence, a control character with no binding. It is
    // distinct from None because None means "ask me again after another byte"
    // and this means "that was a whole key, and it does nothing".
    Ignored,
};

struct KeyEvent {
    Key key = Key::None;
    std::string text;   // Key::Char only; empty otherwise
};

// A tab is turned into FOUR SPACES rather than passed through, and that is a
// decision the renderer forces rather than a preference. Every column this
// module computes is a count of characters, so a byte whose width depends on
// where it lands on the screen would put the cursor in the wrong place for the
// rest of the line. Spaces are what the prompt already indents a block with.
constexpr int TAB_WIDTH = 4;

class KeyDecoder {
public:
    // Feeds one byte and returns what it completed, which is usually nothing.
    KeyEvent feed(unsigned char byte);

    // Throws away a half-read sequence. The reader calls this when a read()
    // comes back with EINTR, because the bytes that were going to finish the
    // sequence are not coming.
    void reset();

private:
    enum class State {
        Ground,
        Escape,     // saw ESC
        Csi,        // saw ESC [
        Ss3,        // saw ESC O
        Utf8,       // partway through a multi-byte character
    };

    KeyEvent ground(unsigned char byte);
    KeyEvent csi_final(unsigned char final_byte);

    State state_ = State::Ground;

    // CSI parameter bytes, without the ESC [ and without the final letter --
    // "1;5" out of ESC [ 1 ; 5 C. Capped, so a terminal sending nonsense
    // cannot grow this without bound.
    std::string params_;

    // The bytes of a character being assembled, and how many are still owed.
    std::string pending_;
    int owed_ = 0;
};

// How many columns a string occupies, counting a UTF-8 character as one and
// an escape sequence as none.
//
// A continuation byte is 10xxxxxx, so every byte that is NOT one starts a
// character -- which is the whole of UTF-8 that this needs to know. Escape
// sequences are skipped because THE PROMPT CONTAINS THEM: it is bold-on,
// username, bold-off, and a width that counted those eight bytes as columns
// would put the cursor eight columns right of where the user is typing on
// every redraw. It is
// deliberately not a full width table: a CJK ideograph and most emoji are two
// columns wide and are counted here as one, so a line containing them redraws
// with the cursor one column left of where it belongs per character. That is a
// visible flaw and it is recorded rather than hidden, because the fix is a
// wcwidth table and the language has no business carrying one yet.
int display_width(const std::string &text);

// The byte offset of the character before / after `offset`, skipping over
// UTF-8 continuation bytes so that a left arrow moves by a CHARACTER and never
// lands in the middle of one. Clamped at both ends.
size_t prev_char(const std::string &text, size_t offset);
size_t next_char(const std::string &text, size_t offset);

} // namespace satellite
