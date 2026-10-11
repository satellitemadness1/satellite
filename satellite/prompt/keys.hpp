#pragma once
// Bytes from a terminal, turned into keys (PLAN M0.6, ported from 003's
// satellite_prompt/keys).
//
// A STATE MACHINE FED ONE BYTE AT A TIME. An arrow key is ESC [ A, and nothing
// promises those three bytes come from one read(): on a slow link they arrive
// apart. So the decoder answers Key::None until it has a whole key, and the
// caller never needs to know how long a key is.
//
// WHAT CHANGED FROM 003:
//   - AN ESCAPE SEQUENCE OF ANY LENGTH IS READ TO ITS END. 003 kept its
//     parameters in a buffer and stopped storing after 16 bytes (DESIGN §1: no
//     limits). Here the parameters are read as NUMBERS while they arrive, so a
//     100,000-byte sequence costs no memory and still ends where it ends.
//   - A CONTROL BYTE ENDS A HALF-READ SEQUENCE AND IS READ AS ITSELF. In 003,
//     ESC followed by Ctrl-C was one ignored "key": a bare ESC swallowed the
//     Ctrl-C after it. Now Ctrl-C, Enter and Backspace always arrive.
//   - A PASTE IS TEXT. Between ESC [ 200 ~ and ESC [ 201 ~ (raw_mode.hpp turns
//     bracketed paste on) no byte is a key: Ctrl-C is the character 0x03, and
//     only a newline ends a line. The paste comes out as PasteLine events, one
//     per newline, and a PasteEnd holding the text after the last one.
//
// ESC ALONE IS NOT A KEY. Escape and the start of an arrow key are the same
// byte; telling them apart needs a timeout, and satl uses none. A bare ESC waits
// for the next byte: a control byte, ESC or a non-ASCII byte is then read as
// itself, and a printable one is taken as an Alt-key, which the prompt ignores.
//
// UTF-8 IS COUNTED, NOT DECODED. A multi-byte character arrives as ONE Key::Char
// with all of its bytes, so the editor moves over it as one character.

#include <cstddef>
#include <string>

namespace satellite004::prompt {

enum class Key {
    None,        // not a whole key yet: feed another byte
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
    Interrupt,   // Ctrl-C: the byte 0x03, because ISIG is off (raw_mode.hpp)
    EndOfInput,  // Ctrl-D
    PasteStart,  // ESC [ 200 ~
    PasteLine,   // pasted text up to a newline, in `text`: a line ends here
    PasteEnd,    // ESC [ 201 ~, with the pasted text after the last newline
    Ignored,     // a whole key, and one this prompt does not use
};

struct KeyEvent {
    Key key = Key::None;
    std::string text;
};

// A tab is not a character in the buffer -- it is this many spaces, inserted,
// typed or pasted. A real tab would make render.cpp's column arithmetic depend
// on where the terminal's tab stops are.
inline constexpr int tab_width = 4;

class KeyDecoder {
public:
    // One byte in, at most one key out.
    KeyEvent feed(unsigned char byte);

    // Forget a half-read sequence (after Ctrl-C, so a stray ESC from an
    // abandoned line cannot take the first byte of the next).
    void reset();

    // Between keys: no sequence, character or paste half-read.
    bool idle() const { return state_ == State::Ground && !pasting_; }

private:
    enum class State { Ground, Escape, Csi, Ss3, Utf8 };

    KeyEvent ground(unsigned char byte);
    KeyEvent csi_byte(unsigned char byte);
    KeyEvent csi_final(unsigned char final_byte);
    KeyEvent paste(unsigned char byte);

    State state_ = State::Ground;

    // THE PARAMETERS OF ONE CSI, READ AS THEY ARRIVE. A number past 1000 is kept
    // as 1000, which is no key's number, so it still reads as "not a key" and
    // stops growing. `odd_` marks a private or intermediate byte, which no key
    // this prompt knows is spelled with.
    unsigned first_ = 0;
    unsigned second_ = 0;
    unsigned index_ = 0;
    bool odd_ = false;

    std::string pending_;  // the bytes of a UTF-8 character so far
    int owed_ = 0;

    bool pasting_ = false;
    std::string pasted_;          // this paste's text since its last newline
    std::size_t end_matched_ = 0; // how much of ESC [ 201 ~ has just arrived
    bool after_cr_ = false;       // a pasted CR LF is one newline, not two
};

// The byte offset of the character before / after `offset`, stepping over UTF-8
// continuation bytes. These are what make Left and Right move by character.
std::size_t previous_character(const std::string &text, std::size_t offset);
std::size_t next_character(const std::string &text, std::size_t offset);

} // namespace satellite004::prompt
