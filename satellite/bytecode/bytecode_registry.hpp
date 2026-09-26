#pragma once
// satellite/bytecode/bytecode_registry.hpp -- the .satl turned into 16-bit
// tokens, one row a file.
//
// (the author, 2026-09-16) "First start 256 threads, then load the tiny C++
// libraries, then convert the .satl to 16-bit, store as
// std::vector<std::vector<std::bitset<16>>> bytecode_registry."
//
// THE FIRST THING THE INTERPRETER DOES WITH A PROGRAM. structured-library.cpp
// runs it after the start-up threads are warm and the number index is loaded,
// before anything looks at what the program says. REGISTRY.satellite is the one
// definition of every code, and token_codes.hpp is generated from it.
//
// ONE ROW A FILE, AND THE ROW IS THAT WHOLE FILE'S TOKENS. (the author,
// 2026-09-16) "The reason for the second vector is we have to take in other
// satellite files, like other includes, so that's why that's that." So row 0 is
// the main .satl and every spaceship it includes gets a row of its own, in the
// order they were taken in.
//
// Statements are found INSIDE a row, by line_end_token, which every line ends
// with; a row's last code is end_of_file_token. This is why line_end_token is a
// token at all -- with a row a file, nothing else says where a line stopped.
//
// A row a file is also much cheaper than a row a line: one allocation for a
// 100,000-line program instead of 100,001, which was most of what the first
// version of this spent its time on.
//
// std::bitset<16> IS 8 BYTES, NOT 2, and the author chose it knowing that
// (measured 2026-09-16: elements 8 bytes apart, 4x uint16_t; a pass over a
// stored program costs 3.5-5x the time of the same tokens as uint16_t, which at
// a million tokens is 0.67 ms against 0.18 ms). What it buys is to_string():
// sixteen binary digits, which is REGISTRY.satellite's own first column and
// exactly what PLAN M1.5-M3.6's converters print. The cost was accepted on
// purpose -- "8 bytes for a 16 bit token is fine" -- and this note is here so
// nobody rediscovers it as a defect.
//
// THE THREADS GET BATCHES OF LINES, NEVER ONE LINE EACH. A line costs about
// 402 ns to tokenise and a recall costs about 12,486 ns, so one job per line is
// 5.6x SLOWER than one thread doing all of it (measured 2026-09-16 through the
// real StartupThreads: 227 ms against 40 ms for 100,000 lines). The same 256
// threads given 256 batches run it in 1.5 ms, 27x faster than one thread. This
// is DESIGN §6 and PLAN M1's "one thread per line would be about 400 times
// slower", now measured rather than quoted.

#include "../machine/machine_codes.hpp"
#include "../machine/machine_state.hpp"
#include "../threads/startup_threads.hpp"
#include "token_codes.hpp"

#include <bitset>
#include <string>
#include <string_view>
#include <vector>

namespace satellite004 {

// One row a FILE; each row is that whole file's 16-bit tokens.
using BytecodeRegistry = std::vector<std::vector<std::bitset<16>>>;

// Which file each row came from, in the same order (the author, 2026-09-16:
// "create a std::vector<std::string> and keep the filenames in order inside of
// that vector"). Row n of the registry is file n here, always the same size.
//
// BESIDE THE REGISTRY AND NOT INSIDE IT, so a row stays nothing but codes and a
// name never has to be spelled in tokens to be carried.
using BytecodeFilenames = std::vector<std::string>;

// A STATEMENT MAY SPAN LINES (the author, 2026-09-25: "it should accept anything that is
// valid satellite regardless of how many spaces or lines are in it, we should accept strings
// that span 90 lines"). A line goes on into the next while a string, a ( , a [ or a list's {
// is open, while it ends in a comma or in an operator waiting for its right side, or when
// the next line begins with a dot (a method chain). The lines of one statement are joined
// onto its FIRST line -- a line break inside a string stays in the string, anywhere else
// it becomes a space, and a // comment at the end of a joined line is dropped -- and each
// line after the first is left EMPTY, so every line_end_token still stands where its line
// does: line numbers stay the ones an editor shows, and a report inside a joined statement
// names the line it began on.
//
// Answers what the file ENDS inside -- a string, a (, a [ or a list that never closes, or a
// list closed with the wrong bracket -- by the physical line (0-based) where it opened.
// A LIST'S { STILL OPEN AT A LINE'S END, WITH A NEW STATEMENT ON THE NEXT LINE, is answered
// too, and ends its statement there (2026-09-26): the file does not end inside it, but the
// next } it would take is a block's -- `l = {1, 2` took main's own } and the satellite.return
// between, and was refused as satellite.return. `column` is where the opener stands in its
// line (npos for a string); `at_the_end` is false for one the file does not end inside, so
// the prompt knows a statement that is refused from one that is still being typed.
struct NeverClosed {
    std::size_t line = 0;
    std::string why;
    std::size_t column = std::string::npos;
    bool at_the_end = true;
};
std::vector<NeverClosed> join_statements_across_lines(std::vector<std::string> &lines);

// WHERE ONE OF THEM IS REFUSED: its opener's code in `row` -- the caret under the { itself --
// when the opener stands on a statement's first line; otherwise its line's first code.
// `lines` is what join_statements_across_lines left of the text `row` was made from.
std::size_t never_closed_at(const std::vector<std::bitset<16>> &row, const std::vector<std::string> &lines,
                            const NeverClosed &each);

// Appends one row -- this file, tokenised on `threads` in batches of lines --
// and its name to `filenames`, so the two stay row for row. The main .satl goes
// in first; a spaceship taken in by satellite.include() adds its own row behind
// it, and the row's INDEX is which file it is.
void add_file_to_bytecode_registry(const std::string &filename,
                                   const std::string &source,
                                   StartupThreads &threads,
                                   unsigned long long int batches,
                                   BytecodeRegistry &registry,
                                   BytecodeFilenames &filenames);

// Clears both and puts `source` in as row 0 -- the main program.
//
// ALWAYS ANSWERS success, because the lexer never throws (DESIGN §5.6). A
// character the registry has no code for is marked in the stream with
// error_token and counted in a report; the conversion finishes either way, so
// whatever reads the registry next sees the whole program and can name the
// fault with a position instead of the run stopping here.
signed long long int build_bytecode_registry(const std::string &filename,
                                             const std::string &source,
                                             StartupThreads &threads,
                                             unsigned long long int batches,
                                             BytecodeRegistry &registry,
                                             BytecodeFilenames &filenames,
                                             MachineState &state);

// One line, alone and on this thread. The unit every batch runs, and what the
// tests drive directly.
// `offsets`, when given, is filled index for index with `row`: the byte offset in
// `line` where each code's TOKEN began. Only the error reporter asks for it, on
// one line, after something has already failed -- see source_position.hpp. Every
// other caller passes nothing and pays one null test a token.
void tokenise_one_line(std::string_view line, std::vector<std::bitset<16>> &row,
                       std::vector<std::size_t> *offsets = nullptr);

// Every code of `row` as sixteen binary digits, space separated -- the form
// REGISTRY.satellite's first column is written in, and what M1.5's converter
// prints. This is why the rows are std::bitset<16>.
std::string row_as_bits(const std::vector<std::bitset<16>> &row);

// How many codes the registry holds, counting every row's tokens and payloads.
unsigned long long int codes_in(const BytecodeRegistry &registry);

// How many characters the lexer had no code for: each is an error_token, and one
// is counted only where a token stands, never inside a payload or its count.
unsigned long long int characters_with_no_code(const BytecodeRegistry &registry);

// READING A ROW BACK. Every [COUNTED] token is followed by a count and then
// that many codes, and these two are the only places that rule is implemented,
// so nothing else has to know how a count is spelled. put_count, below them,
// is where a payload's count is written.

// The count at `at` (which must be a [COUNTED] token's position), and moves
// `at` to the first code of the payload. Answers 0 and does not move for
// anything else, so a caller can ask without checking first.
unsigned long long int count_at(const std::vector<std::bitset<16>> &row, std::size_t &at);

// WRITING ONE, the other half: the count of `written` codes into row[at], the
// blank code a writer left there, spreading into extra codes when one is not
// enough. Every payload's count goes through it except wide_run_token's count of
// 1, which character_codes writes itself: 1 can never be long_count_token.
void put_count(std::vector<std::bitset<16>> &row, std::size_t at, unsigned long long int written);

// The payload at `at` as text -- the argument a library is handed. `at` must be
// the [COUNTED] token itself, and it is moved PAST the whole payload, so a
// walker can carry straight on. Characters above 127 come back through their
// wide runs, so a string with an emoji in it survives the trip.
// Move `at` past a counted payload without building its text. Exactly what
// text_at() does to `at`, and nothing else -- for the callers that dropped the
// string on the floor. See the note above its definition.
void skip_payload(const std::vector<std::bitset<16>> &row, std::size_t &at);

std::string text_at(const std::vector<std::bitset<16>> &row, std::size_t &at);

// A string literal's payload as the text it STANDS FOR: text_at(), then \" \\ \n
// \t \r and \' worked out. Every reader of a literal's value comes through here;
// text_at() stays the reader of names and digits as written. See the note
// above its definition.
std::string string_at(const std::vector<std::bitset<16>> &row, std::size_t &at);

} // namespace satellite004
