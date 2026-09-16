// The read loop implementation.
// Milestone 11 Prototype in prototype/M11.

#include "line_reader.hpp"
#include "editor.hpp"
#include "keys.hpp"
#include "raw_mode.hpp"
#include "render.hpp"
#include "interrupt.hpp"

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <string>
#include <unistd.h>

namespace satellite {

namespace {

void say(const char *text)
{
    (void)!write(STDOUT_FILENO, text, std::strlen(text));
}

} // namespace

LineReader::LineReader()
    : history_(prompt_is_interactive() ? History::default_path()
                                       : std::string())
{
    history_.load();
}

LineReader::~LineReader()
{
    history_.save();
}

void LineReader::remember(const std::string &line)
{
    history_.add(line);
}

LineStatus LineReader::read_cooked(const std::string &prompt,
                                   std::string &line)
{
    fputs(prompt.c_str(), stdout);
    fflush(stdout);

    line.clear();
    for (;;) {
        const int c = getchar();
        if (c == EOF) {
            if (ferror(stdin) && errno == EINTR) {
                clearerr(stdin);
                if (interrupt_requested()) {
                    clear_interrupt();
                    return LineStatus::Interrupted;
                }
                continue;
            }
            return line.empty() ? LineStatus::EndOfFile : LineStatus::Line;
        }
        if (c == '\n')
            return LineStatus::Line;
        line += static_cast<char>(c);
    }
}

LineStatus LineReader::read(const std::string &prompt, std::string &line)
{
    line.clear();

    RawMode raw;
    if (!raw.active())
        return read_cooked(prompt, line);

    Editor editor(&history_);
    KeyDecoder decoder;
    Renderer renderer;

    renderer.draw(prompt, editor.line(), editor.cursor());

    for (;;) {
        unsigned char byte = 0;
        const ssize_t got = ::read(STDIN_FILENO, &byte, 1);

        if (got == 0)
            return LineStatus::EndOfFile;

        if (got < 0) {
            if (errno == EINTR) {
                decoder.reset();
                continue;
            }
            return LineStatus::EndOfFile;
        }

        const KeyEvent event = decoder.feed(byte);
        if (event.key == Key::None)
            continue;

        switch (editor.apply(event)) {
        case Editor::Outcome::Continue:
            break;

        case Editor::Outcome::ClearScreen:
            renderer.clear_screen();
            break;

        case Editor::Outcome::Accept:
            renderer.draw(prompt, editor.line(), editor.line().size());
            say("\r\n");
            line = editor.line();
            return LineStatus::Line;

        case Editor::Outcome::Interrupt:
            renderer.draw(prompt, editor.line(), editor.line().size());
            say("^C\r\n");
            return LineStatus::Interrupted;

        case Editor::Outcome::EndOfInput:
            say("\r\n");
            return LineStatus::EndOfFile;
        }

        renderer.draw(prompt, editor.line(), editor.cursor());
    }
}

} // namespace satellite

