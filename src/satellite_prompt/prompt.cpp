// The prompt loop. See satellite_prompt/prompt.hpp.

#include "satellite_prompt/prompt.hpp"

#include "programs/opening.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_prompt/block.hpp"
#include "satellite_prompt/line_reader.hpp"
#include "satellite_prompt/raw_mode.hpp"
#include "satellite_prompt/session.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "system_facts/facts.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>

namespace satellite::prompt {

namespace {

std::string trimmed(const std::string &line)
{
    size_t begin = line.find_first_not_of(" \t");
    if (begin == std::string::npos)
        return std::string();
    size_t end = line.find_last_not_of(" \t");
    return line.substr(begin, end - begin + 1);
}

bool starts_with(const std::string &text, const char *prefix)
{
    return text.rfind(prefix, 0) == 0;
}

// THE `help` REFUSAL, AND IT IS A REFUSAL RATHER THAN A SECOND SPELLING. Under
// DESIGN §1's generating rule a bare word is the user's, so `help` cannot MEAN
// `satellite.help` without making the language own a bare identifier -- which it
// does in exactly one place today (§7.7's `arguments`) and should not do twice
// by accident. What the prompt can do without costing anything is say where the
// thing they want actually lives, which is what every other refusal in this tree
// does: name the boundary instead of guessing across it.
//
// IT FIRES ONLY WHEN NOTHING DECLARES `help`. A session that has declared a name
// called `help` means that name, and this check runs before any of it -- so the
// day the prompt keeps declarations, this has to move behind the lookup. Said
// here because it is the kind of thing that is obvious now and invisible later.
bool refuse_bare_help(const std::string &line)
{
    if (line != "help" && !starts_with(line, "help(") &&
        !starts_with(line, "help ")) {
        return false;
    }
    std::fputs("satellite: `help` is a name you own, and nothing declares it "
               "-- did you mean `satellite.help()`?\n",
               stderr);
    return true;
}

// `run <file>` and `interpret <file>`, which is how a program is started from
// the prompt. NOT a language path and not pretending to be one: these are the
// prompt's own words, in the same class as the exit words, and they take a file
// the way `satl <file>` does.
bool run_file_command(const std::string &line, Session &session)
{
    std::string path;
    if (starts_with(line, "run "))
        path = trimmed(line.substr(4));
    else if (starts_with(line, "interpret "))
        path = trimmed(line.substr(10));
    else
        return false;

    if (path.empty()) {
        std::fputs("satellite: `run` needs a file after it.\n", stderr);
        return true;
    }
    session.run_file(path);
    return true;
}

std::string prompt_text(int depth)
{
    // A CONTINUATION PROMPT IS INDENTATION AND NOT A SIGIL, so that what the
    // user sees while typing a block is the shape the block will have in a
    // file. A `...` marker would be characters that are not in the program.
    if (depth > 0)
        return std::string(static_cast<size_t>(depth) * 4, ' ');
    return facts::username() + ", " + facts::cwd() + ": ";
}

void install_every_handler()
{
    // THE SAME SIX run_command INSTALLS, AND THE CONSOLE AMONG THEM. `--call`
    // deliberately leaves the console out because it runs one capsule with no
    // printer behind it; a prompt is the other case -- the first thing anybody
    // types is a `display` -- so this is the run arm's list and not that one's.
    console::install_handlers();
    scalars::install_handlers();
    containers::install_handlers();
    random::install_handlers();
    time::install_handlers();
    system::install_handlers();
}

} // namespace

bool is_exit_word(const std::string &line)
{
    const std::string word = trimmed(line);
    return word == "exit" || word == "quit" || word == "exit()" ||
           word == "quit()";
}

int run_prompt()
{
    std::fputs(opening_text().c_str(), stdout);
    std::fputs("\nType `exit` to leave. Ctrl-C abandons a line; Ctrl-D ends "
               "the session.\n\n",
               stdout);
    std::fflush(stdout);

    install_every_handler();

    // CTRL-C, INSTALLED FOR THE SAME REASON EVERY OTHER ENTRY POINT THAT RUNS
    // USER CODE INSTALLS IT (interrupt.hpp's rule) AND NOT FOR THE PROMPT.
    // While a line is being READ the handler cannot fire -- raw mode turns ISIG
    // off, so the key arrives as a byte -- and while a line is RUNNING the
    // terminal is cooked again and this is what stops the walk. DESIGN §10.2's
    // two meanings, and the two mechanisms sitting side by side.
    install_interrupt_handler();
    clear_interrupt();

    console::Console &out = console::Console::the();
    out.start();

    LineReader reader;
    Session session;

    std::string entry;
    std::string line;
    int depth = 0;

    // THE ENTRY IS STILL OWED A BODY -- a head line was typed and the `{` that
    // belongs to it has not arrived. Separate from `depth` because they mean
    // different things: depth counts braces that ARE there, and this remembers
    // one that is not there YET. Cleared the moment any brace is seen.
    bool owed_body = false;

    for (;;) {
        // EVERYTHING PRINTED IS ON THE TERMINAL BEFORE THE PROMPT IS DRAWN.
        // The Console owns a printer thread and the renderer writes to the
        // descriptor directly (render.hpp says why), so without this barrier a
        // program's last line and the next prompt race for the same row.
        out.drain();
        std::fflush(stdout);

        const LineStatus status = reader.read(prompt_text(depth), line);

        if (status == LineStatus::EndOfFile) {
            if (depth > 0 || owed_body) {
                std::fputs("satellite: the block was not finished.\n", stderr);
                entry.clear();
                depth = 0;
                owed_body = false;
                continue;
            }
            break;
        }

        // CTRL-C ABANDONS WHATEVER IS IN HAND AND KEEPS THE SESSION, which is
        // the whole of what the key means here -- a half-typed block included.
        if (status == LineStatus::Interrupted) {
            entry.clear();
            depth = 0;
            owed_body = false;
            continue;
        }

        const std::string one = trimmed(line);

        if (depth == 0 && !owed_body) {
            if (one.empty())
                continue;
            reader.remember(line);
            if (is_exit_word(one))
                break;
            if (refuse_bare_help(one))
                continue;
            if (run_file_command(one, session))
                continue;
        } else {
            reader.remember(line);
        }

        const Scan scanned = scan(line);
        if (scanned.lex_error && depth == 0) {
            // Let the real reporter answer it: the session builds the line and
            // the lexer's own diagnostic comes out with a caret under it. This
            // branch exists so a lex error does not silently open a block.
            session.run(line);
            continue;
        }

        if (!entry.empty())
            entry += '\n';
        entry += line;
        depth += scanned.depth;

        if (scanned.opens_body)
            owed_body = true;
        else if (scanned.depth != 0)
            owed_body = false;

        if (depth > 0 || owed_body)
            continue;

        // A `}` TOO MANY LEAVES THE DEPTH NEGATIVE, and the entry is run anyway
        // so the parser is what says so -- with a caret, on the right line --
        // rather than the prompt inventing a sentence of its own.
        depth = 0;
        owed_body = false;
        session.run(entry);
        entry.clear();
    }

    // THE FULL SHUTDOWN HERE AND NOWHERE ELSE. Session::run drains between
    // lines because another is coming; this is the end of the process, so the
    // four steps run and the printer thread is joined.
    out.shutdown();
    return EXIT_FINE;
}

} // namespace satellite::prompt
