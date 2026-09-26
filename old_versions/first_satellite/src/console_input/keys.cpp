// The byte -> key state machine, and the three UTF-8 helpers every other file
// here measures a line with. No file descriptor appears in this file.

#include "console_input/keys.hpp"

namespace satellite {

namespace {

KeyEvent make(Key key)
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

// How many bytes a UTF-8 character starting with `lead` occupies, or 0 if
// `lead` is not a legal lead byte. A stray continuation byte answers 0 and is
// dropped, which is the only sane thing to do with the middle of a character
// whose beginning never arrived.
int utf8_length(unsigned char lead)
{
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 0;
}

// A CSI sequence's parameters, capped. A terminal that sends thousands of
// parameter bytes is malfunctioning, and the cap is what stops a malfunction
// from being an allocation.
constexpr size_t MAX_PARAMS = 16;

} // namespace

void KeyDecoder::reset()
{
    state_ = State::Ground;
    params_.clear();
    pending_.clear();
    owed_ = 0;
}

KeyEvent KeyDecoder::ground(unsigned char byte)
{
    switch (byte) {
    case 0x1b: state_ = State::Escape; return make(Key::None);
    case '\r':
    case '\n': return make(Key::Enter);

    // BOTH spellings of backspace. A terminal sends 0x7f (DEL) for the key
    // marked Backspace on nearly every keyboard, and 0x08 (Ctrl-H) on the rest
    // -- the two have been swapped by convention for forty years and there is
    // no way to know which this one is, so both mean the same thing here.
    case 0x7f:
    case 0x08: return make(Key::Backspace);

    case 0x03: return make(Key::Interrupt);     // Ctrl-C
    case 0x04: return make(Key::EndOfInput);    // Ctrl-D
    case 0x01: return make(Key::Home);          // Ctrl-A
    case 0x05: return make(Key::End);           // Ctrl-E
    case 0x02: return make(Key::Left);          // Ctrl-B
    case 0x06: return make(Key::Right);         // Ctrl-F
    case 0x10: return make(Key::Up);            // Ctrl-P
    case 0x0e: return make(Key::Down);          // Ctrl-N
    case 0x15: return make(Key::KillToStart);   // Ctrl-U
    case 0x0b: return make(Key::KillToEnd);     // Ctrl-K
    case 0x17: return make(Key::KillWordBack);  // Ctrl-W
    case 0x0c: return make(Key::ClearScreen);   // Ctrl-L

    // Tab, as spaces. See TAB_WIDTH in keys.hpp for why it cannot stay a tab.
    case '\t': return character(std::string(TAB_WIDTH, ' '));
    default: break;
    }

    if (byte < 0x20)
        return make(Key::Ignored);

    const int length = utf8_length(byte);
    if (length <= 0)
        return make(Key::Ignored);      // a continuation byte with no lead
    if (length == 1)
        return character(std::string(1, static_cast<char>(byte)));

    pending_.assign(1, static_cast<char>(byte));
    owed_ = length - 1;
    state_ = State::Utf8;
    return make(Key::None);
}

// The final letter of a CSI or SS3 sequence, with params_ holding whatever
// came before it. Both forms end the same way, so both arrive here.
KeyEvent KeyDecoder::csi_final(unsigned char final_byte)
{
    // Ctrl and the arrows: ESC [ 1 ; 5 C. The modifier is the parameter after
    // the ';' -- 5 is Ctrl, 3 is Alt -- and Ctrl-left/right is the one
    // combination worth reading, because moving by a word is what people
    // expect from it.
    const bool ctrl = params_.find(";5") != std::string::npos;

    switch (final_byte) {
    case 'A': return make(Key::Up);
    case 'B': return make(Key::Down);
    case 'C': return make(ctrl ? Key::WordRight : Key::Right);
    case 'D': return make(ctrl ? Key::WordLeft : Key::Left);
    case 'H': return make(Key::Home);
    case 'F': return make(Key::End);
    case '~': break;
    default: return make(Key::Ignored);
    }

    // The numeric forms. A terminal that does not send ESC [ H for Home sends
    // ESC [ 1 ~ or ESC [ 7 ~ instead, and which one depends on the terminal
    // rather than on anything a program can ask about, so all of them are read.
    const std::string number = params_.substr(0, params_.find(';'));
    if (number == "1" || number == "7") return make(Key::Home);
    if (number == "4" || number == "8") return make(Key::End);
    if (number == "3") return make(Key::Delete);
    return make(Key::Ignored);          // Page Up, Insert, a function key
}

KeyEvent KeyDecoder::feed(unsigned char byte)
{
    switch (state_) {
    case State::Ground:
        return ground(byte);

    case State::Utf8:
        // Anything that is not a continuation byte means the character was
        // truncated. Drop what was collected and read this byte as a fresh
        // start rather than swallowing it -- a dropped character is a
        // nuisance, a swallowed keystroke is a bug.
        if ((byte & 0xC0) != 0x80) {
            reset();
            return ground(byte);
        }
        pending_ += static_cast<char>(byte);
        if (--owed_ > 0)
            return make(Key::None);
        {
            std::string text;
            text.swap(pending_);
            state_ = State::Ground;
            return character(std::move(text));
        }

    case State::Escape:
        params_.clear();
        if (byte == '[') { state_ = State::Csi; return make(Key::None); }
        if (byte == 'O') { state_ = State::Ss3; return make(Key::None); }
        state_ = State::Ground;
        // Alt-b and Alt-f, which is how a terminal sends them: ESC then the
        // letter. Worth having because they are the other spelling of the
        // Ctrl-arrows above, and the one that works over ssh to anywhere.
        if (byte == 'b' || byte == 'B') return make(Key::WordLeft);
        if (byte == 'f' || byte == 'F') return make(Key::WordRight);
        // A lone ESC, or a sequence this does not know. There is no timeout
        // here to tell those apart, which is deliberate: a timeout makes the
        // decoder depend on a clock and stops it being testable from a string.
        return make(Key::Ignored);

    case State::Csi:
        // Parameter and intermediate bytes are 0x20-0x3f; the final byte is
        // 0x40-0x7e. That split is the CSI grammar, so this loop terminates on
        // the sequence's own terms rather than on a guess about its length.
        if (byte >= 0x20 && byte <= 0x3f) {
            if (params_.size() < MAX_PARAMS)
                params_ += static_cast<char>(byte);
            return make(Key::None);
        }
        state_ = State::Ground;
        return csi_final(byte);

    case State::Ss3:
        state_ = State::Ground;
        return csi_final(byte);
    }

    reset();
    return make(Key::Ignored);
}

// ---------------------------------------------------------------------------
// Measuring a line
// ---------------------------------------------------------------------------

int display_width(const std::string &text)
{
    int columns = 0;

    for (size_t i = 0; i < text.size(); i++) {
        const unsigned char byte = static_cast<unsigned char>(text[i]);

        // AN ESCAPE SEQUENCE OCCUPIES NO COLUMNS, and getting this wrong is
        // what makes a coloured prompt put the cursor in the wrong place. The
        // prompt is "\033[1m" + username + "\033[0m" + ", " + ... -- eight
        // bytes of SGR that the terminal consumes and never draws. Counted as
        // characters they would make every redraw believe the prompt is eight
        // columns wider than it is, and the cursor would sit eight columns
        // right of where the user is typing.
        //
        // Only the sequence's own grammar is used to find its end, not a guess
        // at its length: ESC, then '[' or another intro byte, then parameter
        // and intermediate bytes (0x20-0x3f), then one final byte
        // (0x40-0x7e). The same rule KeyDecoder reads sequences by, in the
        // other direction.
        if (byte == 0x1b) {
            i++;
            if (i < text.size() && (text[i] == '[' || text[i] == 'O'))
                i++;
            while (i < text.size()) {
                const unsigned char c = static_cast<unsigned char>(text[i]);
                if (c >= 0x40 && c <= 0x7e)
                    break;              // the final byte; the for() steps past
                i++;
            }
            continue;
        }

        if ((byte & 0xC0) != 0x80)      // not a UTF-8 continuation byte
            columns++;
    }

    return columns;
}

size_t prev_char(const std::string &text, size_t offset)
{
    if (offset == 0)
        return 0;
    size_t i = offset - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i--;
    return i;
}

size_t next_char(const std::string &text, size_t offset)
{
    if (offset >= text.size())
        return text.size();
    size_t i = offset + 1;
    while (i < text.size() &&
           (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80)
        i++;
    return i;
}

} // namespace satellite
