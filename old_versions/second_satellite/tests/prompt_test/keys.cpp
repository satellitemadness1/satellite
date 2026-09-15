// Bytes to keys. See prompt_test.hpp clause 1.

#include "prompt_test.hpp"

#include "satellite_prompt/keys.hpp"

#include <string>

using namespace satellite::prompt;

namespace prompt_test {

namespace {

// Feed a whole string and answer the LAST key that came out, which is the one
// the sequence was spelling.
KeyEvent feed_all(KeyDecoder &decoder, const std::string &bytes)
{
    KeyEvent last;
    for (const char c : bytes) {
        const KeyEvent got = decoder.feed(static_cast<unsigned char>(c));
        if (got.key != Key::None)
            last = got;
    }
    return last;
}

} // namespace

void section_keys()
{
    // 1 -- AN ARROW KEY SPLIT ACROSS THREE FEEDS. The whole reason the decoder
    // is a state machine: each byte is offered alone and only the third may
    // answer.
    {
        KeyDecoder decoder;
        check(decoder.feed(0x1b).key == Key::None, "ESC alone is not yet a key");
        check(decoder.feed('[').key == Key::None, "ESC [ is not yet a key");
        check(decoder.feed('A').key == Key::Up, "ESC [ A is Up");
    }

    // 2 -- THE FOUR ARROWS, HOME AND END, AND DELETE.
    {
        KeyDecoder decoder;
        check(feed_all(decoder, "\033[B").key == Key::Down, "ESC [ B is Down");
        check(feed_all(decoder, "\033[C").key == Key::Right, "ESC [ C is Right");
        check(feed_all(decoder, "\033[D").key == Key::Left, "ESC [ D is Left");
        check(feed_all(decoder, "\033[H").key == Key::Home, "ESC [ H is Home");
        check(feed_all(decoder, "\033[F").key == Key::End, "ESC [ F is End");
        check(feed_all(decoder, "\033[3~").key == Key::Delete, "ESC [ 3 ~ is Delete");
        check(feed_all(decoder, "\033[1~").key == Key::Home, "ESC [ 1 ~ is Home");
        check(feed_all(decoder, "\033[4~").key == Key::End, "ESC [ 4 ~ is End");
    }

    // 3 -- CONTROL-MODIFIED ARROWS ARE WORD MOVES, which is the `;5` parameter
    // and the one place the parameter text is read for meaning.
    {
        KeyDecoder decoder;
        check(feed_all(decoder, "\033[1;5C").key == Key::WordRight,
              "ESC [ 1;5 C is WordRight");
        check(feed_all(decoder, "\033[1;5D").key == Key::WordLeft,
              "ESC [ 1;5 D is WordLeft");
        check(feed_all(decoder, "\033b").key == Key::WordLeft, "Alt-b is WordLeft");
        check(feed_all(decoder, "\033f").key == Key::WordRight, "Alt-f is WordRight");
    }

    // 4 -- CTRL-C IS A BYTE. DESIGN §10.2, and the clause that says why raw
    // mode turns ISIG off: nothing here raises or handles a signal.
    {
        KeyDecoder decoder;
        check(decoder.feed(0x03).key == Key::Interrupt, "0x03 is Interrupt");
        check(decoder.feed(0x04).key == Key::EndOfInput, "0x04 is EndOfInput");
        check(decoder.feed(0x0c).key == Key::ClearScreen, "0x0c is ClearScreen");
        check(decoder.feed(0x7f).key == Key::Backspace, "0x7f is Backspace");
        check(decoder.feed(0x08).key == Key::Backspace, "0x08 is Backspace too");
        check(decoder.feed('\r').key == Key::Enter, "CR is Enter");
        check(decoder.feed('\n').key == Key::Enter, "LF is Enter as well");
    }

    // 5 -- A UTF-8 CHARACTER ARRIVES AS ONE KEY WITH ALL ITS BYTES. Two bytes
    // in, one Key::Char out, and `text` holds both -- which is what makes a
    // left arrow over it move one character rather than one byte.
    {
        KeyDecoder decoder;
        check(decoder.feed(0xC3).key == Key::None, "a UTF-8 lead byte waits");
        const KeyEvent got = decoder.feed(0xA9);
        check(got.key == Key::Char && got.text == "\xC3\xA9",
              "the two bytes of e-acute are one Char event");
    }

    // 6 -- A TRUNCATED CHARACTER DROPS THE HALF AND KEEPS THE NEXT BYTE, which
    // is the difference between a glyph that does not appear and a keystroke
    // that does nothing.
    {
        KeyDecoder decoder;
        decoder.feed(0xC3);
        const KeyEvent got = decoder.feed('x');
        check(got.key == Key::Char && got.text == "x",
              "a byte that is not a continuation is read as itself");
    }

    // 7 -- A TAB IS SPACES BEFORE ANYTHING ELSE SEES IT.
    {
        KeyDecoder decoder;
        const KeyEvent got = decoder.feed('\t');
        check(got.key == Key::Char &&
                  got.text == std::string(static_cast<size_t>(kTabWidth), ' '),
              "a tab arrives as kTabWidth spaces");
    }

    // 8 -- display_width SKIPS AN ESCAPE SEQUENCE, so a coloured prompt does
    // not wrap the line early.
    {
        check(display_width("abc") == 3, "three characters are three columns");
        check(display_width("\033[1mabc\033[0m") == 3,
              "colour around three characters is still three columns");
        check(display_width("\xC3\xA9") == 1, "e-acute is one column");
    }

    // 9 -- CHARACTER STEPPING OVER MULTI-BYTE TEXT.
    {
        const std::string text = "a\xC3\xA9z";  // a, e-acute, z
        check(next_character(text, 0) == 1, "next over `a` lands on the e-acute");
        check(next_character(text, 1) == 3, "next over the e-acute skips both bytes");
        check(previous_character(text, 3) == 1, "previous lands on the lead byte");
        check(previous_character(text, 0) == 0, "previous at the start stays");
    }
}

} // namespace prompt_test
