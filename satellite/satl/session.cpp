// The prompt's session. See session.hpp.

#include "session.hpp"

#include "listing.hpp"
#include "prompt_help.hpp"
#include "prompt_run.hpp"

#include "../bytecode/bytecode_registry.hpp"
#include "../bytecode/console_style.hpp"
#include "../bytecode/program_walk.hpp"
#include "../bytecode/word_codes.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../machine/run_state.hpp"
#include "../machine/shown.hpp"
#include "../machine/stop_flag.hpp"
#include "../machine/input_source.hpp"
#include "../prompt/line_reader.hpp"
#include "../prompt/raw_mode.hpp"
#include "../prompt/render.hpp"

#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <pwd.h>
#include <string>
#include <unistd.h>

namespace satellite004 {

namespace {

using token::Code;

// The session's own Ctrl-C flag. A library reads it between entries through
// stop_flag(); the handler is the only thing that writes it.
volatile sig_atomic_t asked_to_stop = 0;
volatile sig_atomic_t presses = 0;

void on_interrupt(int)
{
    asked_to_stop = 1;
    // Read, added to, then written: `++` on a volatile is deprecated in C++20 and
    // means exactly this, which is the same one instruction a handler may use.
    // THE SECOND PRESS DOES NOT WAIT TO BE NOTICED. A line that is not looking at
    // the flag -- anything but a listing, today -- would otherwise hold the
    // session open with no way out but another terminal.
    presses = presses + 1;
    if (presses >= 2) {
        prompt::restore_terminal();
        put_the_terminal_back_now();   // satellite.terminal's colours, if a line changed them
        _exit(static_cast<int>(interrupted));
    }
}

void on_hangup(int)
{
    prompt::restore_terminal();
    put_the_terminal_back_now();
    _exit(128 + SIGHUP);   // the status a shell gives for a closed terminal
}

// satellite.console.input() READS THROUGH THE SESSION'S OWN READER while a session
// runs (machine/input_source.hpp): a line piped or pasted after the one that asked may
// already be in its buffer, and a typed one is edited and drawn as the prompt's are.
// Piped, the reader draws nothing, so the prompt is written here, as a file run writes it.
prompt::LineReader *the_sessions_reader = nullptr;

InputAnswer read_for_the_program(const std::string &text, const std::string &drawn, std::string &line)
{
    if (!the_sessions_reader->interactive())
        std::cout << (drawn.empty() ? text : drawn);
    std::cout.flush();
    switch (the_sessions_reader->read(prompt::Prompt{text, drawn}, line)) {
    case prompt::LineStatus::Line: return InputAnswer::line;
    case prompt::LineStatus::Interrupted: return InputAnswer::interrupted;
    case prompt::LineStatus::EndOfFile: break;
    }
    return InputAnswer::ended;
}

// SA_RESTART ON PURPOSE: a listing's getdents must not fail half way through
// because a key was pressed. The flag is what stops the work, and the reader's
// own wait is ppoll, which answers EINTR whatever SA_RESTART says.
void watch_for_keys()
{
    struct sigaction action{};
    sigemptyset(&action.sa_mask);
    action.sa_flags = SA_RESTART;
    action.sa_handler = on_interrupt;
    sigaction(SIGINT, &action, nullptr);
    action.sa_handler = on_hangup;
    sigaction(SIGHUP, &action, nullptr);
}

// THE PROMPT, the author's own spelling of 2026-09-22:
//
//     [satellite][linux_username][cwd]>>
//
// *"all in white lettering with black [ and ] and the white lettering is bold"*.
// BUILT AGAIN FOR EVERY LINE, because the folder is the one part that changes
// while a session runs -- satellite.directory.change moves it -- and a prompt
// naming the folder a person left is the answer that is wrong and does not say so.
//
// TWO FORMS FROM THE SAME PIECES (render.hpp's Prompt): `text` is what the
// renderer counts, and `drawn` is each piece through shown() with the colour
// between them. A user name or a folder may hold any byte a file system allows,
// and shown() is what keeps an ESC in a folder's name from reaching the terminal
// -- the colour here is the only escape that does.
//
// WHITE IS 97 AND NOT 37: 37 is the palette's light grey, which is what "white"
// draws as on most terminals. BLACK IS 30, and on a terminal whose background is
// black the brackets are there and cannot be seen -- asked for exactly, and said
// so in the record.
std::string linux_username()
{
    if (const passwd *who = getpwuid(geteuid()); who != nullptr && who->pw_name != nullptr && *who->pw_name)
        return who->pw_name;
    if (const char *user = std::getenv("USER"); user != nullptr && *user)
        return user;
    return std::to_string(static_cast<unsigned long long int>(geteuid()));
}

std::string where_the_session_is()
{
    std::string here(256, '\0');
    while (getcwd(here.data(), here.size()) == nullptr) {
        if (errno != ERANGE)
            return "a folder that is no longer there";
        here.resize(here.size() * 2);
    }
    here.resize(here.find('\0'));
    return here;
}

prompt::Prompt the_prompt_now()
{
    static const std::string bracket = "\033[0;30m";     // black, not bold
    static const std::string lettering = "\033[0;1;97m"; // bold, bright white
    prompt::Prompt made;
    for (const std::string &field : {std::string("satellite"), linux_username(), where_the_session_is()}) {
        made.text += "[" + field + "]";
        made.drawn += bracket + "[" + lettering + shown(field) + bracket + "]";
    }
    made.text += ">> ";
    made.drawn += lettering + ">>" + "\033[0m" + " ";
    return made;
}

std::string without_spaces_around(const std::string &line)
{
    std::size_t first = 0, last = line.size();
    while (first < last && (line[first] == ' ' || line[first] == '\t' || line[first] == '\r')) ++first;
    while (last > first && (line[last - 1] == ' ' || line[last - 1] == '\t' || line[last - 1] == '\r')) --last;
    return line.substr(first, last - first);
}

// REFUSED BY NAME AND NEVER SKIPPED (ERROR #1's shape, PLAN M0.6). Each of these
// is a whole program's spelling, or a block, and a typed line is neither. They
// are found by CODE, so a brace inside a string literal is text and not a brace.
signed long long int refuse_by_name(const std::vector<std::bitset<16>> &row, MachineState &state)
{
    struct Refused { Code code; const char *why; signed long long int answer; };
    const Refused list[] = {
        {token::left_brace_token, "a block has nowhere to live at the prompt: one statement a line until M6",
         satl_line_not_understood},
        {token::right_brace_token, "a block has nowhere to live at the prompt: one statement a line until M6",
         satl_line_not_understood},
        {word::code_of(1, 1), "a session has already taken satellite in", satl_line_not_understood},
        {word::code_of(1, 1, 1), "a session has already taken satellite in", satl_line_not_understood},
        {word::code_of(1, 2), "a capsule belongs to a program, not to a line", satl_line_not_understood},
        {word::code_of(1, 15), "there is nothing here to return from", satl_line_not_understood},
        {word::code_of(1, 15, 1), "there is nothing here to return from", satl_line_not_understood},
        // satellite.help() and satellite.help(topic) are answered before this list
        // is read (prompt_help.hpp); what reaches it is help written some other way.
        {word::code_of(1, 19), "satellite.help is written satellite.help() or satellite.help(topic), on a line of its own",
         satl_line_not_understood},
        {word::code_of(1, 19, 0), "satellite.help is written satellite.help() or satellite.help(topic), on a line of its own",
         satl_line_not_understood},
        {word::code_of(1, 19, 1), "satellite.help is written satellite.help() or satellite.help(topic), on a line of its own",
         satl_line_not_understood},
    };

    for (std::size_t at = 0; at < row.size(); ) {
        const Code code = code_at(row, at);
        if (code == token::end_of_file_token)
            break;
        for (const Refused &refused : list)
            if (code == refused.code && refused.code != 0)
                return report_error(std::string("satl(prompt): ") + refused.why, refused.answer);
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        ++at;
    }
    (void)state;
    return success;
}

// A WHOLE LINE THAT IS ONE LISTING, decided on the compiled line and never on its
// text (PLAN M0.6): the statement is exactly `1 18 4` or `1 18 5` with nothing
// after it. That is the one line whose answer would otherwise be thrown away, so
// it is the one line that draws the table.
bool is_one_listing(const std::vector<std::bitset<16>> &row, std::string &path, bool &given, Code &word_code)
{
    const Code code = code_at(row, 0);
    if (code != word::code_of(1, 18, 4) && code != word::code_of(1, 18, 5))
        return false;
    std::size_t at = 1;
    if (code_at(row, at) != token::left_parenthesis_token)
        return false;
    ++at;
    given = false;
    if (code_at(row, at) == token::string_token) {
        path = text_at(row, at);
        given = true;
    }
    if (code_at(row, at) != token::right_parenthesis_token)
        return false;
    ++at;
    const Code after = code_at(row, at);
    if (after != token::line_end_token && after != token::end_of_file_token && after != token::comment_token)
        return false;
    word_code = code;
    return true;
}

signed long long int draw_the_listing(const std::string &path, bool given, Code word_code,
                                      const FunctionTable &functions)
{
    const NumberRow *library = functions[word_code];
    if (library == nullptr || library->scenarios.directory == nullptr)
        return report_error(std::string("satl(prompt): ") + word::spelling_of(word_code) +
                                " has no library built for it yet",
                            not_built_yet);

    const DirectoryReply reply = library->scenarios.directory(path, given, &asked_to_stop);
    if (stops_the_program(reply.code))
        return report_error(std::string("satl(prompt): ") + word::spelling_of(word_code) + " " +
                                shown(given ? path : std::string(".")) +
                                (reply.reason.empty() ? std::string() : ": " + reply.reason),
                            reply.code);

    std::cout << listing_table(given ? path : std::string("."), reply.names);
    return success;
}

// ONE TYPED LINE, from its text to its answer.
signed long long int run_one_line(const std::string &line, const FunctionTable &functions,
                                  StartupThreads &threads, unsigned long long int batches, MachineState &state)
{
    BytecodeRegistry registry;
    BytecodeFilenames filenames;
    build_bytecode_registry("<typed>", line, threads, batches, registry, filenames, state);
    if (registry.empty())
        return success;

    if (signed long long int helped = success; answer_help(registry.front(), helped))
        return helped;
    const signed long long int refused = refuse_by_name(registry.front(), state);
    if (stops_the_program(refused))
        return refused;

    // NOTHING RUNS BEFORE THE LINE IS JUDGED, which is what a file gets too.
    const signed long long int checked = check_typed_line(registry, functions, state);
    if (stops_the_program(checked))
        return checked;

    std::string path;
    bool given = false;
    Code word_code = 0;
    if (is_one_listing(registry.front(), path, given, word_code))
        return draw_the_listing(path, given, word_code, functions);

    return run_typed_line(registry, functions, state);
}

} // namespace

signed long long int run_session(const Arguments &arguments, const FunctionTable &functions,
                                 StartupThreads &threads, MachineState &state)
{
    prompt::LineReader reader;
    stop_flag() = &asked_to_stop;
    the_sessions_reader = &reader;
    input_source() = read_for_the_program;
    watch_for_keys();

    const unsigned long long int batches = arguments.number("arguments.threads_startup").fits_one_limb()
                                               ? arguments.number("arguments.threads_startup").limb(0)
                                               : 1;
    if (reader.interactive())
        std::cout << "One statement a line. interpret <file> runs a program. exit, quit or Ctrl-D leaves.\n";

    signed long long int first_failure = success;
    std::string line;
    for (;;) {
        // FLUSHED BEFORE THE PROMPT IS DRAWN, and cleared if a write was refused:
        // one refused write would otherwise fail every later line, and the
        // renderer's first draw clears the row it starts on (render.hpp).
        std::cout.flush();
        if (!std::cout) {
            std::cout.clear();
            report_error("satl(prompt): the output refused a line", display_error);
            if (first_failure == success)
                first_failure = display_error;
        }

        // IDLE WHILE IT WAITS FOR A LINE, RUNNING FROM THE MOMENT ONE ARRIVES
        // (machine/run_state.hpp -- the status bar across satl's own console).
        the_interpreter_is_running().store(false, std::memory_order_relaxed);
        const prompt::LineStatus status = reader.read(the_prompt_now(), line);
        the_interpreter_is_running().store(true, std::memory_order_relaxed);
        if (status == prompt::LineStatus::EndOfFile)
            break;
        if (status == prompt::LineStatus::Interrupted) {
            reader.discard_pending();   // the rest of a pasted block is not the person's next wish
            continue;
        }

        const std::string typed = without_spaces_around(line);
        if (typed.empty())
            continue;
        reader.remember(line);
        if (typed == "exit" || typed == "quit")
            break;
        // `interpret <file>` AND `run <file>`, the prompt's own words as `exit` is
        // (prompt_run.hpp): a whole program, run exactly as `satl <file>` runs it.
        if (signed long long int ran = success; run_a_file_from_the_prompt(typed, ran)) {
            if (stops_the_program(ran) && first_failure == success)
                first_failure = ran;
            continue;
        }

        asked_to_stop = 0;
        presses = 0;
        const signed long long int answer = run_one_line(line, functions, threads, batches, state);
        if (stops_the_program(answer) && first_failure == success)
            first_failure = answer;
    }

    std::cout.flush();
    stop_flag() = nullptr;
    input_source() = nullptr;
    the_sessions_reader = nullptr;
    // A PERSON HAS ALREADY SEEN EVERY REFUSAL, so leaving is 0 for them: `exit`
    // after a line that was refused is not itself a failure, and satl-term closes
    // a tab on 0. D0.6.2's "the first failing line's code" is about PIPED input,
    // where nobody watched it go by and the status is the only word about it.
    return reader.interactive() ? success : first_failure;
}

} // namespace satellite004
