// The prompt's session. See session.hpp.

#include "session.hpp"

#include "drives.hpp"
#include "listing.hpp"
#include "prompt_help.hpp"
#include "prompt_run.hpp"

#include "../bytecode/bytecode_registry.hpp"
#include "../bytecode/capsule_scopes.hpp"
#include "../bytecode/console_style.hpp"
#include "../bytecode/program_walk.hpp"
#include "../bytecode/word_codes.hpp"
#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../machine/run_state.hpp"
#include "../machine/s_codes.hpp"
#include "../machine/shown.hpp"
#include "../machine/source_position.hpp"
#include "../machine/stop_flag.hpp"
#include "../machine/input_source.hpp"
#include "../prompt/line_reader.hpp"
#include "../prompt/raw_mode.hpp"
#include "../prompt/render.hpp"

#include <cctype>
#include <cerrno>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <vector>
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

// The prompt's letters, and the listing's free-space line: bold, bright white.
const std::string lettering = "\033[0;1;97m";

prompt::Prompt the_prompt_now()
{
    static const std::string bracket = "\033[0;30m";     // black, not bold
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
//
// A BRACE IS A BLOCK ONLY WHERE A VALUE CANNOT START (2026-09-24). Refusing every `{`
// refused every list literal too, so `l = {"a", "b"}` could not be typed at the prompt
// that now remembers l. A `{` at the start of the line, or after `)`, a name or a word,
// opens a block, as `satellite.statement.if(x) {` does; after `=`, `(`, `,`, `[`, `:`,
// an operator or another `{` it opens a list or an index, and the checker judges it. A
// `}` is a block's unless a list's `{` is still open.
signed long long int refuse_by_name(const std::vector<std::bitset<16>> &row, MachineState &state)
{
    struct Refused { Code code; const char *why; signed long long int answer; };
    const Refused list[] = {
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

    // A BLOCK IS NOT REFUSED ANY MORE (2026-09-25): the prompt gathers one until its braces
    // close, and an if, a while or a for runs whole (run_session, below).
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
// after it -- or `1 18 6`, satellite.directory.system(), whose table is the drives'.
// That is the one line whose answer would otherwise be thrown away, so it is the
// one line that draws the table.
bool is_one_listing(const std::vector<std::bitset<16>> &row, std::string &path, bool &given, Code &word_code)
{
    const Code code = code_at(row, 0);
    if (code != word::code_of(1, 18, 4) && code != word::code_of(1, 18, 5) && code != word::code_of(1, 18, 6))
        return false;
    std::size_t at = 1;
    if (code_at(row, at) != token::left_parenthesis_token)
        return false;
    ++at;
    given = false;
    if (code_at(row, at) == token::string_token) {
        path = string_at(row, at);
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

    // THE HEAD LINES FIRST, before any walk (listing.hpp): white at a terminal, as the
    // prompt's letters are; plain in a pipe, where the prompt is not drawn either.
    const bool at_a_terminal = the_sessions_reader != nullptr && the_sessions_reader->interactive();
    const auto head = [at_a_terminal](const std::string &line) {
        if (at_a_terminal)
            std::cout << lettering << line << "\033[0m\n" << std::flush;
        else
            std::cout << line << '\n' << std::flush;
    };

    // satellite.directory.system(): the drives the library named, and their space.
    if (word_code == word::code_of(1, 18, 6)) {
        for (const std::string &line : drives_lines(reply.names))
            head(line);
        std::cout << drives_table(reply.names);
        return success;
    }

    const std::string where = given ? path : std::string(".");
    head(free_space_line(where));

    // THE SAME KEY STOPS THE TABLE'S COUNTING, and answers the same line.
    std::string table;
    if (!listing_table(where, reply.names, &asked_to_stop, at_a_terminal, table))
        return report_error(std::string("satl(prompt): ") + word::spelling_of(word_code) + " " + shown(where),
                            interrupted);
    std::cout << table;
    return success;
}

// ===========================================================================================
// BLOCKS AT THE PROMPT (the author, 2026-09-25): "if I enter "satellite.spacesuit
// my_class_name()" and press enter, it should automatically enter a {, then start me on a
// newline with the word "satellite.constructor" already written, then auto write another {
// and then start me on a new line with 2 tabs, and then the user has to enter the } but after
// the user enters } on a line, it gives the next thing: satellite.protected with a {, ...
// finally, it should give satellite.public with a { ... when satellite.capsule is entered at
// the prnmpot, it should auto write { and start me on a tabbed line" -- and "satellite.statement
// needs { and a tabbed line". The prompt gathers a block's lines until its braces close, writes
// the { and the sections for the person, and then keeps what it declared (a capsule, a
// spacesuit, a namespace) or runs what it does (an if, a while, a for). A LEVEL IS FOUR SPACES,
// the way every satellite program is indented, where he wrote "tabs".
// ===========================================================================================

// A line's code: its // comment and the spaces around it gone; a string's text is kept whole.
std::string code_part(const std::string &line)
{
    bool in_string = false;
    std::size_t end = line.size();
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (in_string && line[i] == '\\') { ++i; continue; }
        if (line[i] == '"') in_string = !in_string;
        else if (!in_string && line[i] == '/' && i + 1 < line.size() && line[i + 1] == '/') { end = i; break; }
    }
    return without_spaces_around(line.substr(0, end));
}

bool begins(const std::string &text, const char *with) { return text.rfind(with, 0) == 0; }

// HOW MANY BLOCKS A LINE OPENS, LESS HOW MANY IT CLOSES -- its { and } outside strings and
// comments, a list's left out: a { where a value goes (after = ( , [ or a list's own {) opens a
// list, and a } closes the innermost list while one is open.
int blocks_opened_by(const std::string &line)
{
    int blocks = 0, lists = 0;
    char before = '\0';
    bool in_string = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        const char c = line[i];
        if (in_string) {
            if (c == '\\') ++i;
            else if (c == '"') { in_string = false; before = '"'; }
            continue;
        }
        if (c == '/' && i + 1 < line.size() && line[i + 1] == '/') break;
        if (c == '"') { in_string = true; continue; }
        if (c == ' ' || c == '\t') continue;
        if (c == '{') {
            if (lists > 0 || before == '=' || before == '(' || before == ',' || before == '[') ++lists;
            else ++blocks;
        } else if (c == '}') {
            if (lists > 0) --lists;
            else --blocks;
        }
        before = c;
    }
    return blocks;
}

// A WORD THEN ITS BRACKETS, `satellite.statement.if(x)` or `satellite.statement.if (x)`.
bool word_then_brackets(const std::string &code, const char *word)
{
    if (!begins(code, word) || code.back() != ')') return false;
    std::size_t at = std::strlen(word);
    while (at < code.size() && code[at] == ' ') ++at;
    return at < code.size() && code[at] == '(';
}

// WHAT A LINE TYPED BY HAND OPENS when it has not written its own { -- the prompt writes it.
enum class Opens { nothing, block, spacesuit };
Opens what_it_opens(const std::string &code)
{
    if (code.empty() || code.find('{') != std::string::npos) return Opens::nothing;
    // A capsule's and a spacesuit's NAME stands between the word and the brackets.
    const auto named_then_brackets = [&code](const char *word) {
        return begins(code, word) && code.back() == ')' && code.find('(') != std::string::npos;
    };
    if (named_then_brackets("satellite.spacesuit ") || named_then_brackets("satellite.class "))
        return Opens::spacesuit;
    const bool opens = named_then_brackets("satellite.capsule ") || begins(code, "satellite.namespace ") ||
                       begins(code, "satellite.space ") || word_then_brackets(code, "satellite.statement.if") ||
                       word_then_brackets(code, "satellite.statement.while") ||
                       word_then_brackets(code, "satellite.statement.for") ||
                       word_then_brackets(code, "satellite.statement.switch") ||
                       word_then_brackets(code, "satellite.statement.case") ||
                       word_then_brackets(code, "satellite.constructor") || begins(code, "satellite.statement.else") ||
                       begins(code, "satellite.statement.finally") || code == "satellite.protected" ||
                       code == "satellite.public";
    return opens ? Opens::block : Opens::nothing;
}

// WHETHER A STATEMENT IS STILL OPEN at the end of these lines: a string, a ( , a [ or a list the
// lexer's own join says is open (bytecode_registry.hpp), or a last line waiting for the rest --
// a comma, or an operator with a space before it. A list the join says was never closed, with
// a statement typed after it, is not waiting for anything: it runs, and is refused.
bool still_open(const std::vector<std::string> &lines)
{
    std::vector<std::string> copy = lines;
    for (const NeverClosed &each : join_statements_across_lines(copy))
        if (each.at_the_end) return true;
    const std::string last = code_part(lines.back());
    if (last.empty()) return false;
    const char c = last.back();
    if (c == ',' || c == '=' || c == '&' || c == '|') return true;
    return (c == '+' || c == '-' || c == '*' || c == '/' || c == '%' || c == '^') && last.size() > 1 &&
           last[last.size() - 2] == ' ';
}

// WHAT THE SESSION HAS DECLARED: each capsule, spacesuit and namespace typed at the prompt, in
// the order it was typed. A second one of a name takes the first one's place, where it stood, so
// what was declared after it keeps its place -- and the objects already made keep their shape.
struct PromptDeclaration {
    std::string name;
    std::string text;
};
struct PromptProgram {
    std::vector<PromptDeclaration> declared;
};

std::string declared_name(const std::string &code)
{
    for (const char *word : {"satellite.spacesuit ", "satellite.class ", "satellite.capsule ", "satellite.namespace ",
                             "satellite.space "}) {
        if (!begins(code, word)) continue;
        std::size_t from = std::strlen(word);
        while (from < code.size() && code[from] == ' ') ++from;
        std::size_t to = from;
        while (to < code.size() && (std::isalnum(static_cast<unsigned char>(code[to])) || code[to] == '_' || code[to] == '.'))
            ++to;
        return code.substr(from, to - from);
    }
    return std::string();
}

std::string the_declarations(const PromptProgram &program)
{
    std::string text;
    for (const PromptDeclaration &each : program.declared) text += each.text + "\n";
    return text;
}

// The hidden capsule a statement runs in when the session has declared something.
const char *const kPromptStatement = "prompt__statement__";
const char *const kPromptFile = "<prompt>";

// A DECLARATION TYPED AT THE PROMPT, judged as a program's would be -- every capsule body, every
// spacesuit, what the file's scan refuses -- together with everything declared before it, and
// kept only when all of it passes. Its capsule bodies see no kept name: there are no globals.
signed long long int keep_declaration(PromptProgram &program, const std::string &text, const FunctionTable &functions,
                                      StartupThreads &threads, unsigned long long int batches, MachineState &state)
{
    std::string first;
    for (std::size_t start = 0; start < text.size() && first.empty();) {
        const std::size_t stop = text.find('\n', start);
        first = code_part(text.substr(start, stop == std::string::npos ? std::string::npos : stop - start));
        start = stop == std::string::npos ? text.size() : stop + 1;
    }
    const std::string name = declared_name(first);
    if (name == "satellite.main")
        return report_error("satl(prompt): satellite.main belongs to a program you run -- at the prompt, declare "
                            "capsules of your own and call them",
                            satl_line_not_understood);
    PromptProgram candidate = program;
    bool replaced = false;
    for (PromptDeclaration &each : candidate.declared)
        if (each.name == name) { each.text = text; replaced = true; }
    if (!replaced) candidate.declared.push_back({name, text});

    const std::string source = the_declarations(candidate);
    loaded_sources()[kPromptFile] = source;
    BytecodeRegistry registry;
    BytecodeFilenames filenames;
    build_bytecode_registry(kPromptFile, source, threads, batches, registry, filenames, state);
    const CapsuleTable table = capsules_in(registry, filenames);
    const signed long long int checked = check_program(registry, table, functions, state);
    if (stops_the_program(checked))
        return checked;
    program = std::move(candidate);
    return success;
}

// A STATEMENT (A LINE, OR A BLOCK) RUN INSIDE WHAT THE SESSION DECLARED: the body of a hidden
// capsule at the end of it, checked with the kept names and run in their table.
signed long long int run_in_the_prompt_program(const std::string &text, const PromptProgram &program,
                                               const FunctionTable &functions, StartupThreads &threads,
                                               unsigned long long int batches, TypedLineMemory &kept, MachineState &state)
{
    const std::string source = the_declarations(program) + "satellite.capsule " + kPromptStatement + "()\n{\n" + text +
                               "\n}\n";
    loaded_sources()[kPromptFile] = source;
    BytecodeRegistry registry;
    BytecodeFilenames filenames;
    build_bytecode_registry(kPromptFile, source, threads, batches, registry, filenames, state);
    const CapsuleTable table = capsules_in(registry, filenames);
    if (!table.troubles.empty()) {
        const ScopeTrouble *first = &table.troubles.front();
        for (const ScopeTrouble &each : table.troubles)
            if (each.row < first->row || (each.row == first->row && each.at < first->at)) first = &each;
        return raise_at(first->code, first->why, std::string(), state, registry[first->row], first->at, "satl(prompt)");
    }
    const CapsuleSite *site = nullptr;
    for (const CapsuleSite &each : table.sites)
        if (each.name == kPromptStatement) site = &each;
    if (site == nullptr)
        return report_error("satl(prompt): that line could not be read as a statement", satl_line_not_understood);
    const signed long long int checked = check_prompt_statements(registry, table, *site, functions, kept, state);
    if (stops_the_program(checked))
        return checked;
    return run_prompt_statements(registry, table, *site, functions, kept, state);
}

// ONE TYPED LINE, from its text to its answer.
signed long long int run_one_line(const std::string &line, const FunctionTable &functions,
                                  StartupThreads &threads, unsigned long long int batches, TypedLineMemory &kept,
                                  MachineState &state, const PromptProgram &program)
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

    std::string path;
    bool given = false;
    Code word_code = 0;

    // WHAT THE SESSION DECLARED IS AROUND THE LINE, when it has declared anything.
    if (!program.declared.empty()) {
        if (is_one_listing(registry.front(), path, given, word_code))
            return draw_the_listing(path, given, word_code, functions);
        return run_in_the_prompt_program(line, program, functions, threads, batches, kept, state);
    }

    // NOTHING RUNS BEFORE THE LINE IS JUDGED, which is what a file gets too.
    const signed long long int checked = check_typed_line(registry, functions, kept, state);
    if (stops_the_program(checked))
        return checked;

    if (is_one_listing(registry.front(), path, given, word_code))
        return draw_the_listing(path, given, word_code, functions);

    return run_typed_line(registry, functions, kept, state);
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
        std::cout << "A statement a line, or a block -- a capsule, a spacesuit, an if -- and the prompt writes the { "
                     "for you. What you declare is kept until you leave. interpret <file> runs a program. exit, quit "
                     "or Ctrl-D leaves.\n";

    // EVERY NAME A LINE DECLARES LIVES HERE UNTIL THE SESSION ENDS (program_walk.hpp).
    TypedLineMemory kept;
    // ...AND EVERY CAPSULE, SPACESUIT AND NAMESPACE A BLOCK DECLARES (2026-09-25).
    PromptProgram program;

    // A BLOCK BEING GATHERED: its lines, how many of its blocks are open, the sections of each
    // spacesuit still to be written for the person, and whether an if's } has just closed it
    // (an else may follow, so it waits one line before it runs).
    struct Scaffold {
        int level;   // the depth inside the spacesuit, where its sections stand
        int next;    // 0: protected next, 1: public next, 2: none left
    };
    std::vector<std::string> block;
    int depth = 0;
    std::vector<Scaffold> scaffolds;
    bool else_may_follow = false;
    bool plain_else = false;
    static const char *const sections[] = {"satellite.protected", "satellite.public"};
    prompt::Prompt more;
    more.text = "... ";
    more.drawn = "\033[0;90m... \033[0m";

    signed long long int first_failure = success;
    const auto counts = [&](signed long long int answer) {
        if (stops_the_program(answer) && first_failure == success)
            first_failure = answer;
    };
    const auto forget_the_block = [&]() {
        block.clear();
        depth = 0;
        scaffolds.clear();
        else_may_follow = plain_else = false;
    };
    // WHAT THE BLOCK HOLDS IS KEPT (a declaration) OR RUN (anything else), and the block ends.
    const auto finish = [&]() {
        std::string text, first;
        for (const std::string &each : block) {
            text += each + "\n";
            if (first.empty()) first = code_part(each);
        }
        forget_the_block();
        asked_to_stop = 0;
        presses = 0;
        counts(declared_name(first).empty()
                   ? run_one_line(text, functions, threads, batches, kept, state, program)
                   : keep_declaration(program, text, functions, threads, batches, state));
    };
    // A LINE THE PROMPT TYPES FOR THE PERSON, drawn as though typed, at the block's depth.
    const auto write_for_them = [&](const std::string &text) {
        const std::string written = std::string(4 * static_cast<std::size_t>(depth > 0 ? depth : 0), ' ') + text;
        std::cout << more.drawn << written << "\n";
        block.push_back(written);
        depth += blocks_opened_by(written);
    };

    std::string line;
    for (;;) {
        // FLUSHED BEFORE THE PROMPT IS DRAWN, and cleared if a write was refused:
        // one refused write would otherwise fail every later line, and the
        // renderer's first draw clears the row it starts on (render.hpp).
        std::cout.flush();
        if (!std::cout) {
            std::cout.clear();
            report_error("satl(prompt): the output refused a line", display_error);
            counts(display_error);
        }

        const bool gathering = !block.empty();
        if (gathering && depth > 0)
            reader.preset(std::string(4 * static_cast<std::size_t>(depth), ' '));

        // IDLE WHILE IT WAITS FOR A LINE, RUNNING FROM THE MOMENT ONE ARRIVES
        // (machine/run_state.hpp -- the status bar across satl's own console).
        the_interpreter_is_running().store(false, std::memory_order_relaxed);
        const prompt::LineStatus status = reader.read(gathering ? more : the_prompt_now(), line);
        the_interpreter_is_running().store(true, std::memory_order_relaxed);
        if (status == prompt::LineStatus::EndOfFile) {
            if (else_may_follow)
                finish();
            else if (gathering) {
                report_error("satl(prompt): the input ended inside a block, so it was not run -- its } is missing",
                             satl_line_not_understood);
                counts(satl_line_not_understood);
            }
            break;
        }
        if (status == prompt::LineStatus::Interrupted) {
            forget_the_block();        // Ctrl-C abandons a block being typed, as it abandons a line
            reader.discard_pending();  // the rest of a pasted block is not the person's next wish
            continue;
        }

        const std::string typed = without_spaces_around(line);
        const std::string code = code_part(line);

        // AN if's } CLOSED THE BLOCK: an else carries it on, and anything else runs it first.
        if (else_may_follow) {
            if (begins(code, "satellite.statement.else")) {
                else_may_follow = false;
            } else {
                finish();
                if (typed.empty())
                    continue;
            }
        }

        if (block.empty()) {
            if (typed.empty())
                continue;
            reader.remember(line);
            if (typed == "exit" || typed == "quit")
                break;
            // `interpret <file>` AND `run <file>`, the prompt's own words as `exit` is
            // (prompt_run.hpp): a whole program, run exactly as `satl <file>` runs it.
            if (signed long long int ran = success; run_a_file_from_the_prompt(typed, ran)) {
                counts(ran);
                continue;
            }
            // A } WITH NO BLOCK OPEN closes nothing, and says so.
            if (blocks_opened_by(line) < 0) {
                counts(report_error("satl(prompt): this } closes nothing -- no block is open", satl_line_not_understood));
                continue;
            }
            // A LINE THAT OPENS NOTHING AND LEAVES NOTHING OPEN IS ONE STATEMENT, as ever.
            if (what_it_opens(code) == Opens::nothing && blocks_opened_by(line) <= 0 && !still_open({line}) &&
                declared_name(code).empty()) {
                asked_to_stop = 0;
                presses = 0;
                counts(run_one_line(line, functions, threads, batches, kept, state, program));
                continue;
            }
        } else if (!typed.empty()) {
            reader.remember(line);
        }

        // THE LINE JOINS THE BLOCK.
        const int before = depth;
        block.push_back(line);
        depth += blocks_opened_by(line);
        // AN else WITH NO if AFTER IT ENDS THE CHAIN: nothing more can follow it.
        if (before == 0 && begins(code, "satellite.statement.else") && code.find("satellite.statement.if") == std::string::npos)
            plain_else = true;

        if (reader.last_line_was_typed()) {
            // ITS { IS WRITTEN FOR IT, and a spacesuit's constructor with it.
            const Opens opens = what_it_opens(code);
            if (opens != Opens::nothing) {
                write_for_them("{");
                if (opens == Opens::spacesuit) {
                    write_for_them("satellite.constructor()");
                    write_for_them("{");
                    scaffolds.push_back({depth - 1, 0});
                }
            }
            // A SPACESUIT'S SECTION CLOSED: the next one is written for them.
            while (!scaffolds.empty() && depth < scaffolds.back().level)
                scaffolds.pop_back();   // the spacesuit itself closed
            if (depth < before && !scaffolds.empty() && depth == scaffolds.back().level) {
                Scaffold &now = scaffolds.back();
                if (now.next < 2) {
                    write_for_them(sections[now.next++]);
                    write_for_them("{");
                } else {
                    scaffolds.pop_back();
                }
            }
        }

        // THE BLOCK IS WHOLE: its braces closed, nothing left open -- and its last line is not a
        // header still waiting for its {, which a paste or a pipe brings on the next line.
        std::string last_code;
        for (const std::string &each : block)
            if (!code_part(each).empty()) last_code = code_part(each);
        if (depth <= 0 && !still_open(block) && what_it_opens(last_code) == Opens::nothing) {
            std::string first;
            for (const std::string &each : block)
                if (first.empty()) first = code_part(each);
            if (begins(first, "satellite.statement.if") && !plain_else && typed.find('}') != std::string::npos) {
                else_may_follow = true;   // it runs when the next line is not an else
                continue;
            }
            finish();
        }
    }

    // THE FILES THE SESSION KEPT ARE SAVED ON THE WAY OUT, and a save that fails is said
    // and counts as a failing line -- the one place a person could otherwise lose one.
    const signed long long int saved = forget_typed_lines(kept);
    if (stops_the_program(saved) && first_failure == success)
        first_failure = saved;

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
