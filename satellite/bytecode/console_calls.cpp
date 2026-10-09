// satellite/bytecode/console_calls.cpp -- the header lists the words and the options,
// and console_style.hpp what each colour means and which of 003's rules it keeps.

#include "console_calls.hpp"
#include "../display/printing_satellite.hpp"
#include "../machine/console_lock.hpp"

#include "program_walk.hpp"
#include "word_codes.hpp"
#include "../machine/input_source.hpp"
#include "../satellite_variable_infinity/satellite_infinity.hpp"

#include <array>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <sys/ioctl.h>
#include <unistd.h>

namespace satellite004 {
namespace {

using token::Code;


// Which colour word: the console's or the terminal's, foreground or background.
//
// THE BARE ROW IS THE ONE THE LEXER WRITES. `satellite.console.foreground(xFF8800)`
// lexes to `1 5 11`, not to `1 5 11 1`: a path that is a row of its own is taken
// whole, brackets or not (bytecode_registry.cpp, "THE WHOLE PATH FIRST") -- the same
// as satellite.infinity() and satellite.feedback(x). So the COUNT decides: a colour
// sets it, () puts it back. The shaped rows are the same word, spelled.
struct ColourWord {
    bool is = false;
    bool terminal = false;
    bool behind = false;
    int shape = -1;          // -1 the bare row (decided by the count), 0 the () row, 1 the (color) row
};

// EVERY CODE THIS FILE ANSWERS FOR, LOOKED UP ONCE. word::code_of is a binary search
// over the whole table, and call_word asks is_console_word of EVERY word it runs --
// sixteen searches a call made a plain display 45% slower (the review, 2026-09-23,
// measured with callgrind). Built on first use, then one index per question.
enum Which : unsigned char { not_ours, input_word, clear_word, home_word, width_fact, height_fact, a_colour_word };

struct ConsoleWords {
    Code display = 0;
    std::array<Which, 4096> which{};          // by code - 4096: the word range
    std::array<ColourWord, 4096> colour{};

    ConsoleWords()
    {
        display = word::code_of(1, 5, 1);
        mark(word::code_of(1, 5, 2), input_word);
        mark(word::code_of(1, 5, 3), input_word);
        mark(word::code_of(1, 5, 8), clear_word);
        mark(word::code_of(1, 5, 9), home_word);
        mark(word::code_of(1, 5, 6), width_fact);
        mark(word::code_of(1, 5, 7), height_fact);
        const struct { int a, b, c; bool terminal, behind; } words[] = {
            {1, 5, 11, false, false}, {1, 5, 12, false, true}, {1, 29, 1, true, false}, {1, 29, 2, true, true},
        };
        for (const auto &each : words) {
            put(word::code_of(each.a, each.b, each.c), ColourWord{true, each.terminal, each.behind, -1});
            for (int shape = 0; shape <= 1; ++shape)
                put(word::code_of(each.a, each.b, each.c, shape), ColourWord{true, each.terminal, each.behind, shape});
        }
    }
    static bool in_range(Code code) { return code >= 4096 && code < 8192; }
    void mark(Code code, Which what) { if (in_range(code)) which[code - 4096] = what; }
    void put(Code code, ColourWord what)
    {
        if (!in_range(code)) return;
        which[code - 4096] = a_colour_word;
        colour[code - 4096] = what;
    }
    Which of(Code code) const { return in_range(code) ? which[code - 4096] : not_ours; }
};

const ConsoleWords &console_words()
{
    static const ConsoleWords words;
    return words;
}

Code display_word() { return console_words().display; }
bool an_input_word(Code code) { return console_words().of(code) == input_word; }

ColourWord colour_word(Code code)
{
    const ConsoleWords &words = console_words();
    return words.of(code) == a_colour_word ? words.colour[code - 4096] : ColourWord{};
}

const char *const kDisplayOptions[] = {"end", "foreground", "background", "bold", "italic", nullptr};
const char *const kInputOptions[] = {"foreground", "background", "bold", "italic", nullptr};

bool a_colour_option(const std::string &name) { return name == "foreground" || name == "background"; }
bool a_switch_option(const std::string &name) { return name == "bold" || name == "italic"; }

} // namespace

// WHAT display PRINTS FOR A VALUE, as text -- the same text each of call_word's
// scenarios prints, so a styled line reads exactly as the plain one does.
bool display_text(const Value &value, std::string &text, std::string &why)
{
    if (value.is_string()) { text = value.text_utf8(); return true; }
    if (const satellite_number *number = value.as_number()) { text = number->to_text(); return true; }
    if (const bool *flag = value.as_bool()) { text = *flag ? "true" : "false"; return true; }
    if (const satellite_binary_number *bits = value.as_binary()) { text = bits->written(); return true; }
    if (const satellite_hexadecimal_number *hex = value.as_hexadecimal()) { text = hex->written(); return true; }
    if (const satellite_color *colour = value.as_color()) { text = colour->written(); return true; }
    if (const satellite_percentage *part = value.as_percentage()) { text = part->written(); return true; }
    if (value.is_infinity()) { text = satellite_infinity::display(value.as_infinity()); return true; }
    satellite_string written;
    if (value.is_nothing() || value.to_string(written, why) != success) {
        if (why.empty()) why = std::string("it holds ") + value.kind_name() + ", which has no text";
        return false;
    }
    text = written.to_utf8();
    return true;
}

namespace {

// THE OPTIONS, JUDGED BY VALUE, into a style -- and `end=` into what ends the line.
// Every option is judged whether or not anything will be written (003's rule).
bool style_of(const std::string &spelled, const std::vector<NamedOption> &options, TextStyle &style,
              bool &ended, std::string &ending, ExpressionContext &context)
{
    for (const NamedOption &option : options) {
        std::string why;
        if (a_colour_option(option.name)) {
            unsigned int rgb = 0;
            if (!colour_of(option.value, rgb, why)) {
                context.refuse(types_do_not_meet, spelled + "'s " + option.name + "= -- " + why);
                return false;
            }
            (option.name == "foreground" ? style.has_foreground : style.has_background) = true;
            (option.name == "foreground" ? style.foreground : style.background) = rgb;
        } else if (a_switch_option(option.name)) {
            const bool *on = option.value.as_bool();
            if (on == nullptr) {
                context.refuse(types_do_not_meet, spelled + "'s " + option.name +
                                                      "= takes satellite.bool.true or satellite.bool.false, and was given " +
                                                      option.value.kind_name());
                return false;
            }
            (option.name == "bold" ? style.bold : style.italic) = *on;
        } else if (option.name == "end") {
            if (!display_text(option.value, ending, why)) {
                context.refuse(types_do_not_meet, spelled + "'s end= -- " + why);
                return false;
            }
            ended = true;
        }
    }
    return true;
}

int terminal_size(bool across)
{
    winsize size{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &size) == 0 && (across ? size.ws_col : size.ws_row) > 0)
        return across ? size.ws_col : size.ws_row;
    return across ? 80 : 24;       // 003's, and v1's before it: no terminal, the shape of one
}

// EACH LINE IN THE MIDDLE OF THE CONSOLE (.center(), the author 2026-09-25: "it doesn't need
// to change with a resized console, just for that console at that time"). Padded with half
// of what is left of the width the terminal has NOW, counted in characters; a line as wide
// as the console or wider is left as it is. NOT A TERMINAL -- a file, a pipe -- has no width
// to be in the middle of, and the text goes out as written, as colour does there.
std::string centred_text(const std::string &text)
{
    if (!isatty(STDOUT_FILENO))
        return text;
    const std::size_t width = static_cast<std::size_t>(terminal_size(true));
    std::string out;
    std::size_t start = 0;
    for (;;) {
        const std::size_t end = text.find('\n', start);
        const std::string line = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        std::size_t shown = 0;
        for (const char c : line)
            if ((static_cast<unsigned char>(c) & 0xC0) != 0x80) ++shown;
        if (shown < width)
            out.append((width - shown) / 2, ' ');
        out += line;
        if (end == std::string::npos)
            break;
        out += '\n';
        start = end + 1;
    }
    return out;
}

} // namespace

bool an_option_at(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &name, std::size_t &value_at)
{
    if (code_at(row, at) != token::name_token)
        return false;
    // STEPPED OVER, NOT READ, until the `=` says it is an option: every argument that
    // starts with a name comes through here, and building its text each time was the
    // other half of what the review measured.
    std::size_t past = at;
    skip_payload(row, past);
    if (code_at(row, past) != token::assign_token)
        return false;
    std::size_t read = at;
    name = text_at(row, read);
    value_at = past + 1;
    return true;
}

bool options_written_in(const std::vector<std::bitset<16>> &row, std::size_t open,
                        std::vector<WrittenOption> &out, std::string &why)
{
    std::size_t depth = 0;
    bool at_a_start = false;
    for (std::size_t at = open; at < row.size();) {
        const Code code = code_at(row, at);
        if (code == token::line_end_token || code == token::end_of_file_token)
            break;
        if (at_a_start && depth == 1 && code != token::right_parenthesis_token) {
            at_a_start = false;
            WrittenOption option;
            if (an_option_at(row, at, option.name, option.value_at)) {
                option.name_at = at;
                out.push_back(option);
                at = option.value_at;
                continue;
            }
            if (!out.empty()) {
                why = "a plain argument comes before every named one, and this one follows " + out.back().name + "=";
                return false;
            }
        }
        if (token::carries_a_count(code)) { text_at(row, at); continue; }
        if (code == token::left_parenthesis_token || code == token::left_square_bracket_token ||
            code == token::left_brace_token) {
            at_a_start = ++depth == 1;
        } else if (code == token::right_parenthesis_token || code == token::right_square_bracket_token ||
                   code == token::right_brace_token) {
            if (depth == 0 || --depth == 0) return true;
        } else if (code == token::comma_token && depth == 1) {
            at_a_start = true;
        }
        ++at;
    }
    // A BRACKET NEVER CLOSED IS NOT THIS SCAN'S TO REPORT: the checker names the one
    // left open -- `display({1, 2)` is told about its `{` -- so what was found stands.
    return true;
}

const char *const *options_of(Code word)
{
    if (word == display_word()) return kDisplayOptions;
    if (an_input_word(word)) return kInputOptions;
    return nullptr;
}

std::string options_said(Code word)
{
    const char *const *names = options_of(word);
    std::string said;
    if (names == nullptr) return said;
    std::size_t count = 0;
    while (names[count] != nullptr) ++count;
    for (std::size_t at = 0; at < count; ++at)
        said += std::string(at == 0 ? "" : at + 1 == count ? " and " : ", ") + names[at] + "=";
    return said;
}

std::string colour_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t at, const std::string &what)
{
    const Code code = code_at(row, at);
    // A literal is the whole value only when nothing follows it but the end of the
    // argument -- `xFF + x11` is worked out, and judged running.
    std::size_t past = at;
    if (token::carries_a_count(code)) text_at(row, past);
    const Code after = code_at(row, past);
    if (after != token::comma_token && after != token::right_parenthesis_token)
        return "";
    const char *kind = code == token::string_token       ? "text"
                       : code == token::number_token     ? "a number"
                       : code == token::binary_token     ? "a binary"
                       : code == token::percentage_token ? "a percentage"
                                                         : nullptr;
    if (kind != nullptr)
        return what + " takes a colour -- six hex digits like xFF8800, or a satellite.variable.color -- and was given " + kind;
    if (code == token::hexadecimal_token) {
        std::size_t digits_at = at;
        const std::string digits = text_at(row, digits_at);
        if (digits.size() != 6)
            return what + " takes a colour, which is exactly six hex digits like xFF8800, and x" + digits + " has " +
                   std::to_string(digits.size());
    }
    return "";
}

signed long long int option_refused(Code word, const std::vector<std::bitset<16>> &row,
                                    const std::vector<WrittenOption> &options, std::size_t which, std::string &why)
{
    const std::string spelled(word::spelling_of(word));
    const std::string bare = spelled.substr(0, spelled.find('('));
    const WrittenOption &option = options[which];
    const char *const *names = options_of(word);
    bool taken = false;
    for (const char *const *name = names; name != nullptr && *name != nullptr; ++name)
        taken = taken || option.name == *name;
    if (!taken) {
        why = bare + " has no option called " + option.name + "= -- " +
              (names == nullptr ? std::string("it takes no named options") : "it takes " + options_said(word));
        return satl_line_not_understood;
    }
    for (std::size_t before = 0; before < which; ++before) {
        if (options[before].name == option.name) {
            why = bare + "'s " + option.name + "= is given twice -- once is all an option means";
            return satl_line_not_understood;
        }
    }
    if (a_colour_option(option.name))
        why = colour_literal_refused(row, option.value_at, bare + "'s " + option.name + "=");
    if (a_switch_option(option.name)) {
        const Code code = code_at(row, option.value_at);
        if (code == token::string_token || code == token::number_token || code == token::hexadecimal_token ||
            code == token::binary_token || code == token::percentage_token)
            why = bare + "'s " + option.name + "= takes satellite.bool.true or satellite.bool.false";
    }
    return why.empty() ? success : types_do_not_meet;
}

bool is_console_word(Code code)
{
    const Which what = console_words().of(code);
    return what == input_word || what == clear_word || what == home_word || what == a_colour_word;
}

bool a_colour_word_given_one(Code code)
{
    const ColourWord colour = colour_word(code);
    return colour.is && colour.shape != 0;
}

// ONE COMPARE AGAINST A CONSTANT: call_word asks this first of every word call, and a
// display then skips every other family's question (expression.cpp).
bool is_display_word(Code code)
{
    return code == word::fixed_code<1, 5, 1>;
}

bool is_console_fact(Code code)
{
    const Which what = console_words().of(code);
    return what == width_fact || what == height_fact;
}

Value console_fact(Code code)
{
    return Value::of_number(satellite_number(
        static_cast<unsigned long long int>(terminal_size(console_words().of(code) == width_fact))));
}

Value call_console_word(Code code, const std::vector<Value> &arguments, const std::vector<NamedOption> &options,
                        ExpressionContext &context)
{
    const std::string spelled(word::spelling_of(code));

    // THE COLOURS: the console's for every later line, or the terminal's own.
    const ColourWord colour = colour_word(code);
    if (colour.is) {
        const bool set = colour.shape == -1 ? !arguments.empty() : colour.shape == 1;
        unsigned int rgb = 0;
        std::string why;
        if (set && (arguments.size() != 1 || !colour_of(arguments.front(), rgb, why))) {
            context.refuse(types_do_not_meet, spelled + " -- " + (why.empty() ? "it takes one colour" : why));
            return Value();
        }
        if (colour.terminal)
            set_terminal_colour(colour.behind, set, rgb);
        else
            set_console_colour(colour.behind, set, rgb);
        return Value::of_code(success);
    }

    // clear() AND home() -- 003's bytes exactly: clear erases AND homes (terminfo's
    // own meaning), home only moves, for a frame drawn over the last one. Written
    // whatever stdout is, as 003 wrote them, and through the same stream as display
    // so they land between the lines they were written between.
    const Which what = console_words().of(code);
    if (what == clear_word || what == home_word) {
        std::cout << (what == clear_word ? "\033[H\033[2J" : "\033[H");
        if (!std::cout) {
            context.refuse(display_error, spelled + " -- the output refused it");
            return Value();
        }
        return Value::of_code(success);
    }

    // input() AND input(prompt): the prompt, styled by the options, with no newline
    // and flushed so it is on the screen before the wait; then one line. The end of
    // the input is S830, never an empty answer forever (003's S1001).
    TextStyle style;
    bool ended = false;
    std::string ending, prompt, why;
    if (!style_of(spelled, options, style, ended, ending, context))
        return Value();
    if (!arguments.empty() && !display_text(arguments.front(), prompt, why)) {
        context.refuse(types_do_not_meet, spelled + "'s prompt -- " + why);
        return Value();
    }
    // IN THE PROMPT'S SESSION, THROUGH ITS READER (machine/input_source.hpp): the next
    // line may already be in its buffer, and it answers Ctrl-C as the prompt does.
    std::string line;
    const std::string drawn = for_the_screen(styled_line(prompt, style));
    InputAnswer got = InputAnswer::line;
    if (InputSource source = input_source()) {
        got = source(prompt, drawn == prompt ? std::string() : drawn, line);
    } else {
        // THE PROMPT IS ONE LINE AND IS WRITTEN WHOLE (console_lock.hpp); the WAIT for the
        // answer holds nothing, or every other thread's lines would stop until somebody
        // typed. Written unlocked, it lost other threads' lines (the review, 2026-09-23).
        {
            const ConsoleHold one_prompt;
            std::cout << drawn << std::flush;
        }
        if (!std::getline(std::cin, line))
            got = InputAnswer::ended;
        // THE KERNEL ECHOED THE LINE, AND ITS ENTER, INTO THE PTY: in satl's own console the next
        // line is fed past the pty, and must not overtake them (printing_satellite.hpp).
        the_pty_was_written_directly();
    }
    if (got == InputAnswer::interrupted) {
        context.refuse(interrupted, spelled + " was stopped with Ctrl-C before a line was finished");
        return Value();
    }
    if (got == InputAnswer::ended) {
        context.refuse(input_ended, spelled + " waited for a line, and the input has ended");
        return Value();
    }
    Value answer;
    std::size_t bad = 0;
    const signed long long int made = Value::of_utf8(line, answer, bad);
    if (made != success) {
        context.refuse(made, spelled + " read a line whose bytes are not text, at byte " + std::to_string(bad + 1));
        return Value();
    }
    return answer;
}

Value display_with_options(Code code, const Scenarios &scenarios, const Value &argument,
                           const std::vector<NamedOption> &options, ExpressionContext &context, bool centred)
{
    const std::string spelled(word::spelling_of(code));
    // THE CONSOLE'S COLOURS FIRST, and the call's own options over them.
    TextStyle style = console_colours();
    bool ended = false;
    std::string ending, text, why;
    if (!style_of(spelled, options, style, ended, ending, context))
        return Value();
    if (!display_text(argument, text, why)) {
        context.refuse(not_built_yet, spelled + " has no scenario for " + argument.kind_name() +
                                          (why.empty() ? std::string() : " -- " + why));
        return Value();
    }
    if (scenarios.text == nullptr) {
        context.refuse(not_built_yet, spelled + " has no text scenario");
        return Value();
    }
    if (centred)
        text = centred_text(text);
    // ONE CALL, ONE LINE: the style's reset goes before end= or the newline (003). Made here,
    // where its refusals are, and handed to the printing satellite as the bytes it is
    // (display/printing_satellite.hpp).
    const signed long long int answer =
        display_line_bytes(for_the_screen(styled_line(text, style) + (ended ? ending : "\n")));
    if (stops_the_program(answer))
        context.refuse(answer, spelled + " refused");
    return Value::of_code(answer);
}

Value string_coloured(const Value &receiver, Code method, const std::vector<Value> &arguments,
                      bool had_parentheses, const std::string &name, ExpressionContext &context)
{
    const bool behind = method == token::background_token;
    const std::string what = name + (behind ? ".background" : ".foreground");
    if (!had_parentheses || arguments.size() != 1) {
        context.refuse(satl_line_not_understood, what + " takes one colour, in brackets: " + what + "(xFF8800)");
        return Value();
    }
    unsigned int rgb = 0;
    std::string why;
    if (!colour_of(arguments.front(), rgb, why)) {
        context.refuse(types_do_not_meet, what + " -- " + why);
        return Value();
    }
    Value answer;
    std::size_t bad = 0;
    const signed long long int made = Value::of_utf8(coloured_text(receiver.text_utf8(), rgb, behind), answer, bad);
    if (made != success) {
        context.refuse(made, what + " -- the string could not be carried back as text");
        return Value();
    }
    return answer;
}

} // namespace satellite004
