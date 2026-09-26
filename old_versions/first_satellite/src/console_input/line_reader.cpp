// The read loop: the ONE file in console_input/ that touches a terminal.
// Everything it decides with is a pure function living in one of the others.

#include "console_input/console_input.hpp"
#include "console_input/editor.hpp"
#include "console_input/keys.hpp"
#include "console_input/raw_mode.hpp"
#include "console_input/render.hpp"
#include "system_facts/interrupt.hpp"

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

// A HISTORY FILE ONLY WHEN SOMEBODY IS TYPING. `printf '...' | satl` and
// `satl < lines.txt` are scripts, not sessions: nobody there can press the up
// arrow, so nothing there has anything to recall -- and a run that wrote
// ~/.satl_history anyway would be leaving a dotfile in somebody's home
// directory as a side effect of a pipeline. That is the thing history.hpp says
// this must not do, arriving by a route the $SATL_HISTORY escape hatch does not
// cover, because a script does not know to set it.
//
// It is also what every shell does: bash keeps no history for a
// non-interactive shell, for the same reason.
//
// Memory-only, not disabled -- the History still exists and still records, so
// the read loop and everything above it have exactly one shape. It simply has
// no path, which is what History::save() already reads as "write nothing".
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

// The fallback for `satl < script`, `satl | cat`, and anything else where one
// end of the prompt is not a terminal.
//
// It is the loop main_repl.cpp had before this module existed, moved rather
// than rewritten, so a non-interactive run behaves exactly as it always did:
// getchar() to the newline, EOF is end of input. There is nothing to edit
// without a terminal and nothing to draw onto, and pretending otherwise would
// write escape sequences into somebody's pipe.
LineStatus LineReader::read_cooked(const std::string &prompt,
                                   std::string &line)
{
    fputs(prompt.c_str(), stdout);
    fflush(stdout);

    line.clear();
    for (;;) {
        const int c = getchar();
        if (c == EOF) {
            // A read cut short by our own SIGINT handler is NOT end of input,
            // and telling them apart matters: without this, a Ctrl-C would end
            // the session silently, which is worse than what happened before
            // there was a handler at all. See interrupt.hpp on why SA_RESTART
            // is off and this case therefore exists.
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

    // Raw for the length of this call and cooked again on every path out of
    // it, including the two returns below and any exception. That scope is the
    // whole mitigation for giving up the kernel's line discipline -- see
    // raw_mode.hpp -- and it is what leaves a satellite program that reads
    // input with ordinary echo and backspace when the prompt runs it.
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
            return LineStatus::EndOfFile;       // stdin closed under us

        if (got < 0) {
            if (errno == EINTR) {
                // A signal landed mid-sequence. The bytes that would have
                // finished it are not coming, so the half-read escape is
                // dropped rather than left to combine with whatever is typed
                // next -- which is how a stray keystroke becomes a phantom
                // arrow key.
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
            // Redraw with the cursor at the END before the newline, whatever
            // column it was actually in. Without it, a line accepted with the
            // cursor in the middle of a wrapped line leaves the newline on the
            // wrong row and the next prompt lands on top of what was typed.
            renderer.draw(prompt, editor.line(), editor.line().size());
            say("\r\n");
            line = editor.line();
            return LineStatus::Line;

        case Editor::Outcome::Interrupt:
            renderer.draw(prompt, editor.line(), editor.line().size());
            // ^C where the cursor was, then a fresh row. This is what every
            // prompt does and the reason is that it leaves a record on screen
            // of what was abandoned and where.
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
