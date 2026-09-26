#pragma once
// satellite/bytecode/console_calls.hpp -- satellite.console'S WORDS THAT THE
// INTERPRETER ANSWERS, AND THE NAMED OPTIONS display AND input TAKE.
//
// 003 BUILT THESE AND 004 HAD NONE OF THEM (the author, 2026-09-23: "pull how ncurses
// worked on 003 into 004", then "every single one of those errors is legal"). 003's
// "ncurses" never linked ncurses -- M30 wrote the terminal's own codes, and what it
// built was display's named options; M14 built the screen's four words. All of it:
//
//     satellite.console.display("warning", foreground=xFF8800, background=x000000,
//                               bold=satellite.bool.true, italic=satellite.bool.true)
//     satellite.console.display("Loading", end="...")        -- no newline, "..." instead
//     satellite.variable.string name = satellite.console.input("name? ", foreground=xFF8800)
//     satellite.console.width   satellite.console.height     -- the terminal's size now
//     satellite.console.clear()                              -- the screen cleared, cursor home
//     satellite.console.home()                               -- the cursor home, nothing erased
//
// and what 004 adds, each explained in console_style.hpp:
//
//     satellite.console.foreground(xFF8800)   satellite.console.background(x000000)
//     satellite.console.foreground()          -- back to the terminal's own
//     satellite.terminal.foreground(xFF8800)  satellite.terminal.background(x000000)
//     satellite.terminal.foreground()         -- back to the terminal's own
//     "OK".foreground(x00FF00)                s.background(x000000)
//
// ---------------------------------------------------------------------------
// A NAMED OPTION IS `name=value` AFTER EVERY PLAIN ARGUMENT, once each.
// ---------------------------------------------------------------------------
//
// 003's grammar (its PLAN M30, "NAMED ARGUMENTS BUILT 2026-09-12"): an argument that
// starts with a name and `=` is an option, it comes after every plain one, and a word
// names the options it takes -- anything else is refused before the program runs.
// It never counts as an argument: the lexer's choice of `input(prompt)` over
// `input(prompt, target)` counts plain arguments only (bytecode_registry.cpp), and so
// does the checker. `==` is its own token, so `display(a == b)` is not an option.
//
// NO LIBRARY BEHIND THESE WORDS, for file_calls.hpp's reason turned round: they act on
// the one terminal satl writes to, and the console's colours and the terminal's are
// state that has to outlive a call -- a library is handed a value and answers a code.

#include "expression.hpp"
#include "console_style.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// ONE OPTION OF A CALL, as the walker read it: `foreground=xFF8800`.
struct NamedOption {
    std::string name;
    Value value;
};

// ONE OPTION OF A CALL, as the checker found it: its name, where the name stands, and
// where its value starts.
struct WrittenOption {
    std::string name;
    std::size_t name_at = 0;
    std::size_t value_at = 0;
};

// DOES AN ARGUMENT START HERE WITH `name =`? `at` is the argument's first code; on true,
// `name` is the option's name and `value_at` is where its value begins.
bool an_option_at(const std::vector<std::bitset<16>> &row, std::size_t at, std::string &name,
                  std::size_t &value_at);

// EVERY OPTION OF THE CALL WHOSE `(` IS AT `open`, at its own depth -- an option of a
// call inside it is that call's. Answers false with `why` when a plain argument comes
// after an option; a bracket left open is the checker's to name, not this scan's.
bool options_written_in(const std::vector<std::bitset<16>> &row, std::size_t open,
                        std::vector<WrittenOption> &out, std::string &why);

// THE OPTIONS A WORD TAKES, or nullptr when it takes none -- one list for the checker
// and the walker. Ends with nullptr.
const char *const *options_of(token::Code word);

// "end, foreground, background, bold and italic" -- for a sentence.
std::string options_said(token::Code word);

// WHAT THE CHECKER SAYS about one option before anything runs: a name the word does
// not take or a name given twice (satl_line_not_understood), or a literal that cannot
// be what the option takes -- `foreground="red"`, `foreground=xFFF`, `bold=5`
// (types_do_not_meet). success when it is right.
signed long long int option_refused(token::Code word, const std::vector<std::bitset<16>> &row,
                                    const std::vector<WrittenOption> &options, std::size_t which,
                                    std::string &why);

// A LITERAL WRITTEN WHERE A COLOUR GOES THAT CANNOT BE ONE -- text, a number, a hex that
// is not six digits -- said before anything runs, or "". A name or an expression is
// judged when it has a value.
std::string colour_literal_refused(const std::vector<std::bitset<16>> &row, std::size_t at,
                                   const std::string &what);

// THE WORDS HERE: satellite.console.input() and input(prompt), width, height, clear(),
// home(), foreground()/(color), background()/(color); satellite.terminal.foreground and
// .background, both shapes. input(prompt, target) and typed() are not built.
bool is_console_word(token::Code code);

// THE FOUR COLOUR WORDS GIVEN A COLOUR -- satellite.console/terminal.foreground/
// background(color) -- whose argument the checker judges when it is a literal.
bool a_colour_word_given_one(token::Code code);

// satellite.console.display, asked without a search (call_word asks it of every word).
bool is_display_word(token::Code code);

// satellite.console.width and .height, read with no brackets.
bool is_console_fact(token::Code code);
Value console_fact(token::Code code);

// A console word called. The arguments are evaluated; the options were checked by
// name before the run and are judged by value here.
Value call_console_word(token::Code code, const std::vector<Value> &arguments,
                        const std::vector<NamedOption> &options, ExpressionContext &context);

// display with options, or with console colours set. `scenarios` is display's
// library; what it prints is one line, as 003 queued one unit.
// `centred` is .center() written on the call: each line in the middle of the console.
Value display_with_options(token::Code code, const Scenarios &scenarios, const Value &argument,
                           const std::vector<NamedOption> &options, ExpressionContext &context,
                           bool centred = false);

// A STRING'S .foreground(c) / .background(c). `name` is for the sentence.
Value string_coloured(const Value &receiver, token::Code method, const std::vector<Value> &arguments,
                      bool had_parentheses, const std::string &name, ExpressionContext &context);

} // namespace satellite004
