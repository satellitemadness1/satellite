#pragma once

// Bytes from a terminal, turned into keys.
//
// WHY A STATE MACHINE AND NOT A read()-AND-COMPARE. An arrow key is three bytes
// -- ESC [ A -- and they can arrive in three separate read() calls, because a
// terminal is a stream and nothing promises otherwise. Code that reads one byte,
// sees ESC and then reads two more assumes an atomicity it does not have; on a
// slow link it blocks forever holding half a key. This decoder is fed ONE byte
// at a time and answers Key::None until it has a whole one, so the caller never
// has to know how many bytes a key is.
//
// AND IT IS WHY ESC ALONE IS NOT A KEY HERE. Escape and "the start of an arrow
// key" are the same byte, and telling them apart needs a timeout -- a decision
// about how long to wait that belongs to the reader, not to the decoder. So a
// bare ESC that is followed by nothing meaningful comes back Ignored rather than
// being invented into a key nobody pressed.
//
// UTF-8 IS COUNTED, NOT DECODED. The prompt buffer holds bytes; what the decoder
// owes the editor is that a multi-byte character arrives as ONE Key::Char event
// with all of its bytes in `text`, so a left-arrow over an accented letter moves
// one character and not one byte. DESIGN §5's code table is the LANGUAGE's
// alphabet and is a different question from what a terminal sends.

#include <cstddef>
#include <string>

namespace satellite::prompt {

enum class Key {
    None,        // not a whole key yet -- feed another byte
    Char,        // `text` holds the character, one byte or a UTF-8 run
    Enter,
    Backspace,   // the character BEFORE the cursor
    Delete,      // the character AT the cursor
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
    Interrupt,   // Ctrl-C -- the byte 0x03, because ISIG is off (raw_mode.hpp)
    EndOfInput,  // Ctrl-D
    Ignored,     // a whole key, and one this prompt does not use
};

struct KeyEvent {
    Key key = Key::None;
    std::string text;
};

// A tab is not a character in the buffer -- it is this many spaces, inserted.
// A literal tab would make the cursor arithmetic in render.cpp depend on where
// the tab stops fall, which is a property of the terminal and not of the line.
inline constexpr int kTabWidth = 4;

class KeyDecoder {
public:
    // One byte in, at most one key out.
    KeyEvent feed(unsigned char byte);

    // Forget a half-read sequence. The reader calls this after an interrupt,
    // so a stray ESC left over from an abandoned line cannot swallow the first
    // byte of the next one.
    void reset();

private:
    enum class State { Ground, Escape, Csi, Ss3, Utf8 };

    KeyEvent ground(unsigned char byte);
    KeyEvent csi_final(unsigned char final_byte);

    State state_ = State::Ground;
    std::string parameters_;
    std::string pending_;
    int owed_ = 0;
};

// How many columns this text occupies, counting a UTF-8 character as one and
// skipping any ANSI escape sequence inside it. The prompt string is allowed to
// carry colour, and a prompt whose width counted its own escape bytes would
// wrap the line in the wrong place.
int display_width(const std::string &text);

// The byte offset of the character before / after `offset`, stepping over UTF-8
// continuation bytes. These are what make Left and Right move by character.
size_t previous_character(const std::string &text, size_t offset);
size_t next_character(const std::string &text, size_t offset);

} // namespace satellite::prompt
