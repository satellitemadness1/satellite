// The byte -> key state machine and UTF-8 string helpers.
// Milestone 11 Prototype in prototype/M11.

#include "keys.hpp"

#include <utility>

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

int utf8_length(unsigned char lead)
{
    if (lead < 0x80) return 1;
    if ((lead & 0xE0) == 0xC0) return 2;
    if ((lead & 0xF0) == 0xE0) return 3;
    if ((lead & 0xF8) == 0xF0) return 4;
    return 0;
}

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

    case '\t': return character(std::string(TAB_WIDTH, ' '));
    default: break;
    }

    if (byte < 0x20)
        return make(Key::Ignored);

    const int length = utf8_length(byte);
    if (length <= 0)
        return make(Key::Ignored);
    if (length == 1)
        return character(std::string(1, static_cast<char>(byte)));

    pending_.assign(1, static_cast<char>(byte));
    owed_ = length - 1;
    state_ = State::Utf8;
    return make(Key::None);
}

KeyEvent KeyDecoder::csi_final(unsigned char final_byte)
{
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

    const std::string number = params_.substr(0, params_.find(';'));
    if (number == "1" || number == "7") return make(Key::Home);
    if (number == "4" || number == "8") return make(Key::End);
    if (number == "3") return make(Key::Delete);
    return make(Key::Ignored);
}

KeyEvent KeyDecoder::feed(unsigned char byte)
{
    switch (state_) {
    case State::Ground:
        return ground(byte);

    case State::Utf8:
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
        if (byte == 'b' || byte == 'B') return make(Key::WordLeft);
        if (byte == 'f' || byte == 'F') return make(Key::WordRight);
        return make(Key::Ignored);

    case State::Csi:
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

int display_width(const std::string &text)
{
    int columns = 0;
    for (size_t i = 0; i < text.size(); i++) {
        if (text[i] == '\033') {
            // Skip ANSI escape sequence (e.g. \033[1;38;2;...m or \033[0m)
            if (i + 1 < text.size() && text[i + 1] == '[') {
                i += 2;
                while (i < text.size() && text[i] >= 0x20 && text[i] <= 0x3f) {
                    i++;
                }
            }
            continue;
        }
        // Count non-continuation UTF-8 bytes as 1 display column
        if ((static_cast<unsigned char>(text[i]) & 0xC0) != 0x80) {
            columns++;
        }
    }
    return columns;
}

size_t prev_char(const std::string &text, size_t offset)
{
    if (offset == 0) return 0;
    size_t i = offset - 1;
    while (i > 0 && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) {
        i--;
    }
    return i;
}

size_t next_char(const std::string &text, size_t offset)
{
    if (offset >= text.size()) return text.size();
    size_t i = offset + 1;
    while (i < text.size() && (static_cast<unsigned char>(text[i]) & 0xC0) == 0x80) {
        i++;
    }
    return i;
}

} // namespace satellite

