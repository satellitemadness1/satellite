// The byte-at-a-time key decoder. See satellite_prompt/keys.hpp.

#include "satellite_prompt/keys.hpp"

#include <utility>

namespace satellite::prompt {

namespace {

KeyEvent plain(Key key)
{
    KeyEvent event;
    event.key = key;
    return event;
}

KeyEvent character(std::string text)
{
    KeyEvent event;
    event.key = Key::Char;
    event.text = std::move(text);
    return event;
}

// How many bytes the character starting with this lead byte occupies, or 0 if
// it is not a lead byte at all -- a stray continuation byte, which is what a
// truncated paste or a non-UTF-8 locale produces.
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

// A CSI parameter string is digits and semicolons, and a terminal that sent
// more than this is not sending a key. The cap is here so a malformed stream
// cannot grow the buffer without bound while the decoder waits for a final byte
// that never comes -- SCRATCH.md/NO_LIMITS.md's rule is that the LANGUAGE has
// no limits; a terminal escape sequence is not the language.
constexpr size_t kMaxParameters = 16;

} // namespace

void KeyDecoder::reset()
{
    state_ = State::Ground;
    parameters_.clear();
    pending_.clear();
    owed_ = 0;
}

KeyEvent KeyDecoder::ground(unsigned char byte)
{
    switch (byte) {
    case 0x1b:
        state_ = State::Escape;
        return plain(Key::None);

    // BOTH, AND NOT ONE. ICRNL is off (raw_mode.cpp), so Enter arrives as CR
    // and nothing turns it into LF -- but a pasted line brings real LFs with
    // it, and a prompt that accepted only one of them would take a paste as a
    // single line with control characters embedded in it.
    case '\r':
    case '\n':
        return plain(Key::Enter);

    // 0x7f IS BACKSPACE ON EVERY TERMINAL WORTH THE NAME, and 0x08 is what a
    // few still send. Accepting both costs one line and is the difference
    // between a working backspace and a mystery.
    case 0x7f:
    case 0x08:
        return plain(Key::Backspace);

    case 0x03: return plain(Key::Interrupt);     // Ctrl-C, as a byte: DESIGN §10.2
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

    // A TAB BECOMES SPACES HERE AND NOT IN THE EDITOR, so nothing downstream
    // ever holds a tab: render.cpp's column arithmetic would otherwise depend
    // on where the terminal's tab stops are, which is not a property of the
    // line being edited.
    case '\t':
        return character(std::string(kTabWidth, ' '));

    default:
        break;
    }

    if (byte < 0x20)
        return plain(Key::Ignored);

    const int length = utf8_length(byte);
    if (length <= 0)
        return plain(Key::Ignored);
    if (length == 1)
        return character(std::string(1, static_cast<char>(byte)));

    pending_.assign(1, static_cast<char>(byte));
    owed_ = length - 1;
    state_ = State::Utf8;
    return plain(Key::None);
}

KeyEvent KeyDecoder::csi_final(unsigned char final_byte)
{
    // `ESC [ 1;5C` is Ctrl-Right on xterm and everything that copies it. The
    // modifier is parameter 2, and 5 is control.
    const bool control = parameters_.find(";5") != std::string::npos;

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

    // The `~` family: a number, then the tilde. Home and End each have two
    // spellings in the wild and both are answered.
    const std::string number = parameters_.substr(0, parameters_.find(';'));
    if (number == "1" || number == "7")
        return plain(Key::Home);
    if (number == "4" || number == "8")
        return plain(Key::End);
    if (number == "3")
        return plain(Key::Delete);
    return plain(Key::Ignored);
}

KeyEvent KeyDecoder::feed(unsigned char byte)
{
    switch (state_) {
    case State::Ground:
        return ground(byte);

    case State::Utf8:
        // A BYTE THAT IS NOT A CONTINUATION MEANS THE CHARACTER WAS TRUNCATED,
        // and the right answer is to drop the half-character and read this byte
        // as a fresh one -- not to swallow it. A dropped byte is a glyph that
        // does not appear; a swallowed one is a keystroke that does nothing,
        // which reads as a broken keyboard.
        if ((byte & 0xC0) != 0x80) {
            reset();
            return ground(byte);
        }
        pending_ += static_cast<char>(byte);
        if (--owed_ > 0)
            return plain(Key::None);
        {
            std::string text;
            text.swap(pending_);
            state_ = State::Ground;
            return character(std::move(text));
        }

    case State::Escape:
        parameters_.clear();
        if (byte == '[') {
            state_ = State::Csi;
            return plain(Key::None);
        }
        if (byte == 'O') {
            state_ = State::Ss3;
            return plain(Key::None);
        }
        state_ = State::Ground;
        // Alt-b and Alt-f, which is how a terminal sends word-left and
        // word-right when it is not sending the CSI form above.
        if (byte == 'b' || byte == 'B')
            return plain(Key::WordLeft);
        if (byte == 'f' || byte == 'F')
            return plain(Key::WordRight);
        return plain(Key::Ignored);

    case State::Csi:
        if (byte >= 0x20 && byte <= 0x3f) {
            if (parameters_.size() < kMaxParameters)
                parameters_ += static_cast<char>(byte);
            return plain(Key::None);
        }
        state_ = State::Ground;
        return csi_final(byte);

    case State::Ss3:
        state_ = State::Ground;
        return csi_final(byte);
    }

    reset();
    return plain(Key::Ignored);
}

int display_width(const std::string &text)
{
    int columns = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\033') {
            // Skip the whole sequence: `ESC [` then parameters then one final
            // byte, which the loop's own i++ steps over.
            if (i + 1 < text.size() && text[i + 1] == '[') {
                i += 2;
                while (i < text.size() && text[i] >= 0x20 && text[i] <= 0x3f)
                    i++;
            }
            continue;
        }
        // One column per character, which is one per non-continuation byte.
        // A double-width glyph is counted as one and will wrap a column early;
        // that is a known narrowing rather than an oversight -- getting it
        // right needs a width table, and the language's own alphabet is
        // satellite_string's code table rather than Unicode's.
        if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80)
            columns++;
    }
    return columns;
}

size_t previous_character(const std::string &text, size_t offset)
{
    if (offset == 0)
        return 0;
    size_t i = offset - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i--;
    return i;
}

size_t next_character(const std::string &text, size_t offset)
{
    if (offset >= text.size())
        return text.size();
    size_t i = offset + 1;
    while (i < text.size() &&
           (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i++;
    return i;
}

} // namespace satellite::prompt
