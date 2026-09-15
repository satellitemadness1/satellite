// Reading one line. See satellite_prompt/line_reader.hpp.

#include "satellite_prompt/line_reader.hpp"

#include "satellite_prompt/editor.hpp"
#include "satellite_prompt/keys.hpp"
#include "satellite_prompt/raw_mode.hpp"
#include "satellite_prompt/render.hpp"

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <string>
#include <unistd.h>

namespace satellite::prompt {

void LineReader::remember(const std::string &line)
{
    history_.add(line);
}

LineStatus LineReader::read_cooked(const std::string &prompt, std::string &line)
{
    // THE PROMPT STILL GOES OUT, even down a pipe, and that is deliberate: a
    // transcript of a session is worth having, and a test that asserts on the
    // screen (SCRATCH.md's pty rule) needs the prompt in it to know which
    // answer belongs to which line. It goes to stdout un-newlined and flushed,
    // the same shape M14's `input(prompt)` uses.
    if (!prompt.empty()) {
        std::fputs(prompt.c_str(), stdout);
        std::fflush(stdout);
    }

    line.clear();
    if (!std::getline(std::cin, line))
        return LineStatus::EndOfFile;
    return LineStatus::Line;
}

LineStatus LineReader::read(const std::string &prompt, std::string &line)
{
    return read([&prompt](const std::string &) { return prompt; }, line);
}

LineStatus LineReader::read(const PromptFor &prompt, std::string &line)
{
    if (!is_interactive())
        return read_cooked(prompt(std::string()), line);

    RawMode raw;
    if (!raw.active())
        return read_cooked(prompt(std::string()), line);

    Editor editor(&history_);
    KeyDecoder decoder;
    Renderer renderer;

    renderer.draw(prompt(editor.line()), editor.line(), editor.cursor());

    for (;;) {
        unsigned char byte = 0;
        const ssize_t got = ::read(STDIN_FILENO, &byte, 1);

        // EINTR IS NOT THE END OF INPUT, and treating it as one is how a prompt
        // ends a session because the window was resized. SIGWINCH and every
        // other handler in this process interrupt a blocking read; the answer
        // is to read again, not to conclude anything about the user.
        if (got < 0) {
            if (errno == EINTR)
                continue;
            return LineStatus::EndOfFile;
        }
        if (got == 0)
            return LineStatus::EndOfFile;

        const KeyEvent event = decoder.feed(byte);
        if (event.key == Key::None)
            continue;

        switch (editor.apply(event)) {
        case Editor::Outcome::Continue:
            renderer.draw(prompt(editor.line()), editor.line(), editor.cursor());
            break;

        // THE NEWLINE IS OURS TO PRINT, because ECHO is off -- the terminal
        // will not move to the next row on its own, and without this every
        // answer would be drawn over the line that asked for it.
        case Editor::Outcome::Accept:
            line = editor.line();
            std::fputs("\n", stdout);
            std::fflush(stdout);
            return LineStatus::Line;

        // CTRL-C ABANDONS THE LINE AND KEEPS THE SESSION. DESIGN §10.2 is the
        // argument: at the prompt this key is the byte 0x03 and means "not
        // that line, I have changed my mind", while inside a running program
        // it is the SIGINT M11 built that stops the walk. Two meanings, one
        // key, and which one applies is decided by what is running.
        case Editor::Outcome::Interrupt:
            line.clear();
            std::fputs("\n", stdout);
            std::fflush(stdout);
            decoder.reset();
            return LineStatus::Interrupted;

        case Editor::Outcome::EndOfInput:
            line.clear();
            std::fputs("\n", stdout);
            std::fflush(stdout);
            return LineStatus::EndOfFile;

        case Editor::Outcome::ClearScreen:
            renderer.clear_screen();
            renderer.draw(prompt(editor.line()), editor.line(), editor.cursor());
            break;
        }
    }
}

} // namespace satellite::prompt
