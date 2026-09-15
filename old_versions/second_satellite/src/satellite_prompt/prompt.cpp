// The prompt loop. See satellite_prompt/prompt.hpp.

#include "satellite_prompt/prompt.hpp"

#include "programs/opening.hpp"
#include "satellite_console/console.hpp"
#include "satellite_console/handlers.hpp"
#include "satellite_containers/handlers.hpp"
#include "satellite_directory/handlers.hpp"
#include "satellite_file/handlers.hpp"
#include "satellite_help/handlers.hpp"
#include "satellite_prompt/block.hpp"
#include "satellite_prompt/line_reader.hpp"
#include "satellite_prompt/raw_mode.hpp"
#include "satellite_prompt/session.hpp"
#include "satellite_random/handlers.hpp"
#include "satellite_scalars/handlers.hpp"
#include "satellite_system/handlers.hpp"
#include "satellite_time/handlers.hpp"
#include "programs/arms.hpp"
#include "system_facts/facts.hpp"
#include "system_facts/interrupt.hpp"

#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

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
//
// AND THE WORDS AFTER THE FILE ARE THE PROGRAM'S, split the way a shell splits
// `satl <file> a b`: on spaces, with a quoted word kept whole. So a path with a
// space in it is quoted here exactly as it is quoted there -- until 2026-09-13
// the whole rest of the line was the path, which read `run prog.satl --small`
// as a file called "prog.satl --small".
bool run_file_command(const std::string &line, Session &session)
{
    std::string rest;
    if (starts_with(line, "run "))
        rest = line.substr(4);
    else if (starts_with(line, "interpret "))
        rest = line.substr(10);
    else
        return false;

    std::vector<std::string> words;
    std::string word;
    bool in_word = false;
    char quote = 0;
    for (const char c : rest) {
        if (quote != 0) {
            if (c == quote)
                quote = 0;
            else
                word += c;
        } else if (c == '"' || c == '\'') {
            quote = c;
            in_word = true;
        } else if (c == ' ' || c == '\t') {
            if (in_word)
                words.push_back(word);
            word.clear();
            in_word = false;
        } else {
            word += c;
            in_word = true;
        }
    }
    if (quote != 0) {
        std::fputs("satellite: a quote on the `run` line is never closed.\n",
                   stderr);
        return true;
    }
    if (in_word)
        words.push_back(word);

    if (words.empty()) {
        std::fputs("satellite: `run` needs a file after it.\n", stderr);
        return true;
    }
    session.run_file(words);
    return true;
}

// FOUR SPACES, WHICH IS WHAT THIS TREE'S OWN PROGRAMS ARE WRITTEN IN --
// example/hello_world.satl and every other file under example/. A prompt that
// indented by a different amount would produce, from the user's own typing,
// source that does not look like the source they have been reading.
constexpr size_t kIndent = 4;

// The prompt for a line being typed at `depth`, given what is on it so far.
//
// A CONTINUATION PROMPT IS INDENTATION AND NOT A SIGIL, so what the user sees
// while typing a block is the shape the block will have in a file. A `...`
// marker would be characters that are not in the program.
//
// AND A LINE THAT STARTS WITH `}` IS DRAWN ONE LEVEL OUT, live, as the brace is
// typed. The closing brace belongs to the block it ends rather than to the body
// inside it, so it sits where the head sat -- which is what every one of these
// programs does and what a person expects to see. It has to be recomputed per
// keystroke because the prompt is printed BEFORE the character that decides it.
std::string prompt_text(int depth, const std::string &so_far)
{
    if (depth <= 0)
        return facts::username() + ", " + facts::cwd() + ": ";

    int level = depth;
    if (!trimmed(so_far).empty() && trimmed(so_far)[0] == '}')
        level--;
    if (level < 0)
        level = 0;
    return std::string(static_cast<size_t>(level) * kIndent, ' ');
}

// WHAT THE SESSION IS SET TO, printed under the banner.
//
// THE FIRST LINE IS A PATH A PROGRAM CAN TYPE, and that is why it is written as
// a call rather than as prose: what is on the screen is exactly what a person
// types to set it, so reading the banner teaches the command. WORD_NUMBERS §2.7
// assigned it `1 22 7` and `1 22 8` for this.
//
// AND THAT IS WHY IT SAYS `satellite.bool.true` AND NOT `true`. The author drew
// this line as `satellite.system.persist(true)`, and `true` is not a word in
// this language: DESIGN §6.1's module constants are `satellite.bool.true`
// `1 17 2` and `satellite.bool.false` `1 17 1`, and a bare `true` is a name the
// USER owns (§1) -- typing the short form answers S0511, `nothing called
// `true` is in scope here`, which is measured and correct. So the banner would
// have been teaching a command that does not run. A line that shows a path is
// making a claim about that path, and this is the one file whose whole job this
// week has been not to make claims like that.
//
// THE SECOND LINE HAS NO VALUE AFTER THE `=` AND THAT IS DELIBERATE, at the
// author's instruction: *"if it's not built yet just put a line there that says
// `arguments.memory.available = ` and leave it blank for now, so that we
// remember to put it there later."* It is a reminder in the product rather than
// a note in a file nobody opens.
//
// TWO THINGS ABOUT IT ARE WORTH KNOWING BEFORE ANYBODY FILLS IT IN. The NUMBER
// exists today -- `system_facts::mem_available_bytes()` is built and M6 reads
// it -- so what is missing is not the fact but the path: DESIGN §7.7's
// `arguments` object lands at M20. And §7.7 spells free memory
// `arguments.memory`, with `arguments.memory.total` beside it; there is no
// `.available` in that table. So this line reserves a spelling §7.7 has not
// agreed to, and M20 has to settle which of the two names it is before the
// value goes in. It is marked the way every unbuilt thing in this binary is
// marked -- programs/opening.hpp's rule, "lines that describe a milestone that
// has not landed are MARKED, rather than omitted".
std::string status_text()
{
    std::string out = "\n";
    out += "satellite.system.persist(satellite.bool.";
    out += system::persisting() ? "true" : "false";
    out += ")\n";
    out += "arguments.memory.available =                                (M20)\n";
    return out;
}

void install_every_handler()
{
    // THE SAME run_command INSTALLS, AND THE CONSOLE AMONG THEM. `--call`
    // deliberately leaves the console out because it runs one capsule with no
    // printer behind it; a prompt is the other case -- the first thing anybody
    // types is a `display` -- so this is the run arm's list and not that one's.
    //
    // AND "THE SAME AS run_command" HAS TO BE CHECKED AGAINST run_command AND
    // NOT AGAINST THIS COMMENT, which is what went wrong here. M23 added
    // `thread::install_handlers()` to run_command.cpp and to nothing else, so
    // for three weeks this list said it matched the run arm while missing a
    // row, and `satellite.thread.new` typed at the prompt answered S0721 --
    // "a path satellite has a number for and nothing behind yet, a later
    // milestone" -- about a milestone that had in fact landed. The error was
    // truthful about the TABLE and wrong about the LANGUAGE, which is the
    // worst shape a refusal can have: it sent the reader to the milestones to
    // look for work that was already done. Found 2026-09-12 by a program that
    // ran from a file and refused at the prompt, one line apart.
    arms::install_for(arms::Arm::Prompt);
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
    std::fputs(status_text().c_str(), stdout);
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

    // THE PROMPT JUST WROTE A `{` OF ITS OWN, so a `{` on the very next line is
    // the user's habit meeting the prompt's help and is dropped rather than
    // opening a second block. One line's grace and no more -- a `{` anywhere
    // else is a real brace and counts.
    bool auto_braced = false;

    for (;;) {
        // EVERYTHING PRINTED IS ON THE TERMINAL BEFORE THE PROMPT IS DRAWN.
        // The Console owns a printer thread and the renderer writes to the
        // descriptor directly (render.hpp says why), so without this barrier a
        // program's last line and the next prompt race for the same row.
        out.drain();
        // A PROGRAM THAT ENDED MID-LINE -- `display("a", end="")` -- gets its
        // line ended here, or the redraw below erases what it printed.
        if (out.take_mid_line())
            std::fputs("\n", stdout);
        std::fflush(stdout);

        const LineStatus status = reader.read(
            [&depth](const std::string &so_far) {
                return prompt_text(depth, so_far);
            },
            line);

        if (status == LineStatus::EndOfFile) {
            if (depth > 0) {
                std::fputs("satellite: the block was not finished.\n", stderr);
                entry.clear();
                depth = 0;
                auto_braced = false;
                continue;
            }
            break;
        }

        // CTRL-C ABANDONS WHATEVER IS IN HAND AND KEEPS THE SESSION, which is
        // the whole of what the key means here -- a half-typed block included.
        if (status == LineStatus::Interrupted) {
            entry.clear();
            depth = 0;
            auto_braced = false;
            continue;
        }

        const std::string one = trimmed(line);

        if (depth == 0) {
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

        if (auto_braced && one == "{") {
            auto_braced = false;
            continue;
        }
        auto_braced = false;

        const Scan scanned = scan(line);

        // A LINE THAT DID NOT LEX IS RUN AT ONCE so the real reporter answers
        // it -- the session builds it and the lexer's own diagnostic comes out
        // with a caret under it, rather than the prompt inventing a sentence.
        //
        // ONLY WHEN NOTHING IS IN HAND, and the first version of this was wrong
        // in a way worth writing down: it fired whenever `depth == 0`, which was
        // also true of a head line whose body had not arrived -- so an
        // unterminated string on the line after `satellite.statement.if (x)`
        // ran alone AND left the head sitting in `entry`, where the next line
        // joined it. Running one line out of the middle of a block is neither
        // finishing the entry nor abandoning it.
        if (scanned.lex_error && entry.empty() && depth == 0) {
            session.run(line);
            continue;
        }

        // STORED AT THE INDENT IT WAS SHOWN AT, so the program the session
        // builds looks like the program the user was looking at -- and so a
        // capsule kept across lines reads like one when it is re-emitted. The
        // typed text is TRIMMED first: the indent belongs to the prompt, and
        // keeping both would double it.
        int level = depth;
        if (!one.empty() && one[0] == '}')
            level--;
        if (level < 0)
            level = 0;

        if (!entry.empty())
            entry += '\n';
        entry.append(static_cast<size_t>(level) * kIndent, ' ');
        entry += one;
        depth += scanned.depth;

        // THE PROMPT WRITES THE `{` AND THE USER NEVER DOES. This language puts
        // the brace on its own line, so a head line leaves one owed -- and
        // asking a person to type a character the prompt already knows is
        // coming is asking them to do the prompt's work. It goes into the entry
        // at the head's own level AND onto the screen, because a brace that is
        // in the program but not on the terminal is a program the user cannot
        // read back.
        if (scanned.opens_body) {
            entry += '\n';
            entry.append(static_cast<size_t>(level) * kIndent, ' ');
            entry += '{';

            std::string shown(static_cast<size_t>(level) * kIndent, ' ');
            shown += "{\n";
            std::fputs(shown.c_str(), stdout);
            std::fflush(stdout);

            depth++;
            auto_braced = true;
        }

        if (depth > 0)
            continue;

        // A `}` TOO MANY LEAVES THE DEPTH NEGATIVE, and the entry is run anyway
        // so the parser is what says so -- with a caret, on the right line --
        // rather than the prompt inventing a sentence of its own.
        depth = 0;
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
