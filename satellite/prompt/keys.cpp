// The byte-at-a-time key decoder. See keys.hpp.

#include "keys.hpp"

#include <utility>

namespace satellite004::prompt {

namespace {

KeyEvent plain(Key key)
{
    KeyEvent event;
    event.key = key;
    return event;
}

KeyEvent with_text(Key key, std::string &text)
{
    KeyEvent event;
    event.key = key;
    event.text.swap(text);
    return event;
}

// How many bytes the character starting with this lead byte occupies, or 0 if it
// is not a lead byte: a stray continuation byte, which a truncated paste gives.
int utf8_length(unsigned char lead)
{
    if (lead < 0x80)
        return 1;
    if ((lead & 0xE0) == 0xC0)
        return 2;
    if ((lead & 0xF0) == 0xE0)
        return 3;
    if ((lead & 0xF8) == 0xF0)
        return 4;
    return 0;
}

bool is_control(unsigned char byte)
{
    return byte < 0x20 || byte == 0x7F;
}

// One digit more of a parameter. Past 1000 it stays 1000: no key is numbered
// that high, so the sequence still reads as no key, and the number stops growing.
void add_digit(unsigned &parameter, unsigned char digit)
{
    parameter = parameter >= 1000 ? 1000 : parameter * 10 + (digit - '0');
}

constexpr char paste_end[] = "\033[201~";
constexpr std::size_t paste_end_size = sizeof paste_end - 1;

} // namespace

void KeyDecoder::reset()
{
    state_ = State::Ground;
    first_ = second_ = index_ = 0;
    odd_ = false;
    pending_.clear();
    owed_ = 0;
    pasting_ = false;
    pasted_.clear();
    end_matched_ = 0;
    after_cr_ = false;
}

KeyEvent KeyDecoder::ground(unsigned char byte)
{
    state_ = State::Ground;
    switch (byte) {
    case 0x1b:
        state_ = State::Escape;
        return plain(Key::None);

    // BOTH: ICRNL is off, so Enter arrives as CR, and a terminal without
    // bracketed paste still sends a pasted line's LF.
    case '\r':
    case '\n':
        return plain(Key::Enter);

    case 0x7f:
    case 0x08:
        return plain(Key::Backspace);

    case 0x03: return plain(Key::Interrupt);     // Ctrl-C, as a byte
    case 0x04: return plain(Key::EndOfInput);    // Ctrl-D
    case 0x01: return plain(Key::Home);          // Ctrl-A
    case 0x05: return plain(Key::End);           // Ctrl-E
    case 0x02: return plain(Key::Left);          // Ctrl-B
    case 0x06: return plain(Key::Right);         // Ctrl-F
    case 0x10: return plain(Key::Up);            // Ctrl-P
    case 0x0e: return plain(Key::Down);          // Ctrl-N
    case 0x15: return plain(Key::KillToStart);   // Ctrl-U
    case 0x0b: return plain(Key::KillToEnd);     // Ctrl-K
    case 0x17: return plain(Key::KillWordBack);  // Ctrl-W
    case 0x0c: return plain(Key::ClearScreen);   // Ctrl-L

    case '\t': {
        std::string spaces(tab_width, ' ');
        return with_text(Key::Char, spaces);
    }

    default:
        break;
    }

    if (byte < 0x20)
        return plain(Key::Ignored);

    const int length = utf8_length(byte);
    if (length <= 0)
        return plain(Key::Ignored);
    pending_.assign(1, static_cast<char>(byte));
    if (length == 1)
        return with_text(Key::Char, pending_);
    owed_ = length - 1;
    state_ = State::Utf8;
    return plain(Key::None);
}

KeyEvent KeyDecoder::csi_byte(unsigned char byte)
{
    if (byte >= '0' && byte <= '9') {
        if (index_ == 0)
            add_digit(first_, byte);
        else if (index_ == 1)
            add_digit(second_, byte);
        return plain(Key::None);
    }
    if (byte == ';') {
        index_ = index_ < 2 ? index_ + 1 : 2;  // a third parameter or later is not read
        return plain(Key::None);
    }
    if (byte >= 0x20 && byte <= 0x3F) {  // ':', a private marker or an intermediate
        odd_ = true;
        return plain(Key::None);
    }
    if (byte >= 0x40 && byte <= 0x7E) {
        state_ = State::Ground;
        return csi_final(byte);
    }
    return ground(byte);  // a control byte or a non-ASCII byte: the sequence was cut off
}

KeyEvent KeyDecoder::csi_final(unsigned char final_byte)
{
    if (odd_)
        return plain(Key::Ignored);

    // ESC [ 1 ; 5 C is Ctrl-Right on xterm and everything that copies it: the
    // modifier is the second parameter, and 5 is control.
    const bool control = index_ >= 1 && second_ == 5;

    switch (final_byte) {
    case 'A': return plain(Key::Up);
    case 'B': return plain(Key::Down);
    case 'C': return plain(control ? Key::WordRight : Key::Right);
    case 'D': return plain(control ? Key::WordLeft : Key::Left);
    case 'H': return plain(Key::Home);
    case 'F': return plain(Key::End);
    case '~': break;
    default:  return plain(Key::Ignored);
    }

    switch (first_) {
    case 1: case 7: return plain(Key::Home);
    case 4: case 8: return plain(Key::End);
    case 3: return plain(Key::Delete);
    case 200:
        pasting_ = true;
        pasted_.clear();
        end_matched_ = 0;
        after_cr_ = false;
        return plain(Key::PasteStart);
    default: return plain(Key::Ignored);  // 201 outside a paste included
    }
}

// INSIDE A PASTE, ONLY ESC [ 201 ~ AND A NEWLINE MEAN ANYTHING. Bytes that begin
// like the end and then differ were text, and go into the line; every other byte,
// controls included, is text too (render.cpp shows controls escaped).
KeyEvent KeyDecoder::paste(unsigned char byte)
{
    if (byte == static_cast<unsigned char>(paste_end[end_matched_])) {
        if (++end_matched_ < paste_end_size)
            return plain(Key::None);
        pasting_ = false;
        end_matched_ = 0;
        after_cr_ = false;
        return with_text(Key::PasteEnd, pasted_);
    }
    if (end_matched_ > 0) {
        pasted_.append(paste_end, end_matched_);
        end_matched_ = 0;
        after_cr_ = false;
        if (byte == 0x1b) {
            end_matched_ = 1;
            return plain(Key::None);
        }
    }

    if (byte == '\n' && after_cr_) {
        after_cr_ = false;
        return plain(Key::None);
    }
    after_cr_ = byte == '\r';
    if (byte == '\r' || byte == '\n')
        return with_text(Key::PasteLine, pasted_);
    if (byte == '\t')
        pasted_.append(tab_width, ' ');
    else
        pasted_ += static_cast<char>(byte);
    return plain(Key::None);
}

KeyEvent KeyDecoder::feed(unsigned char byte)
{
    if (pasting_)
        return paste(byte);

    switch (state_) {
    case State::Ground:
        return ground(byte);

    case State::Utf8:
        // A byte that is not a continuation means the character was cut short:
        // drop the half-character and read this byte fresh, never swallow it.
        if ((byte & 0xC0) != 0x80) {
            pending_.clear();
            owed_ = 0;
            return ground(byte);
        }
        pending_ += static_cast<char>(byte);
        if (--owed_ > 0)
            return plain(Key::None);
        state_ = State::Ground;
        return with_text(Key::Char, pending_);

    case State::Escape:
        if (byte == '[' || byte == 'O') {
            state_ = byte == '[' ? State::Csi : State::Ss3;
            first_ = second_ = index_ = 0;
            odd_ = false;
            return plain(Key::None);
        }
        if (is_control(byte) || byte >= 0x80)
            return ground(byte);
        state_ = State::Ground;
        // Alt-b and Alt-f: word-left and word-right from a terminal that does not
        // send the CSI form.
        if (byte == 'b' || byte == 'B')
            return plain(Key::WordLeft);
        if (byte == 'f' || byte == 'F')
            return plain(Key::WordRight);
        return plain(Key::Ignored);

    case State::Csi:
        return csi_byte(byte);

    case State::Ss3:
        if (byte >= 0x40 && byte <= 0x7E) {
            state_ = State::Ground;
            return csi_final(byte);
        }
        return ground(byte);
    }

    reset();
    return plain(Key::Ignored);
}

std::size_t previous_character(const std::string &text, std::size_t offset)
{
    if (offset == 0)
        return 0;
    std::size_t i = offset - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i--;
    return i;
}

std::size_t next_character(const std::string &text, std::size_t offset)
{
    if (offset >= text.size())
        return text.size();
    std::size_t i = offset + 1;
    while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i++;
    return i;
}

} // namespace satellite004::prompt
