#pragma once
// satellite/bytecode/file_calls.hpp -- satellite.file's words and a file's
// methods, run by the interpreter. SATELLITE_FILE_OPERATIONS.md Part 3.
//
// A FILE'S METHODS RUN BY THEIR NUMBERS SINCE 2026-10-11 (NM-8): each is the word
// satellite.variable.file.<it>, 1 6 2 n, whose code is a file under
// satellite-numbers/words/ (1.6.2.12.cpp is append), and is written
// f.satellite.append("x"). This file keeps what they share, the refusals made
// before one runs (run_file_number), and satellite.file's own words.
//
// THESE WORDS HAVE NO LIBRARY, and that is a departure from DESIGN §3.4 that is
// said here rather than hidden. `satellite.file.new(path)` answers a HANDLE --
// a std::shared_ptr to a satellite_file -- and a library's scenarios only
// consume values and answer machine codes (number_row.hpp). A handle made inside
// a library would carry that library's copy of satellite_file's code into the
// interpreter's object model. So the file words belong to the object model, the
// way satellite.variable.number's declaration and satellite.statement.while
// belong to the walker, and the checker knows them by is_file_word() instead of
// by a library row. If the author wants them as libraries, the handle crossing
// the boundary is the question to rule on first.
//
// A FILE THAT WOULD NOT OPEN IS A VALUE (satellite_file.hpp): making and opening
// never stop a program. What does stop one is a MISUSE the program itself made:
// a word given the wrong number or kind of arguments, and READING A LINE THAT
// CANNOT BE READ -- `f[n]`, `f.first`, `f.last` past the end, on a closed file or
// on a binary one -- because an answer made up for it would be "an answer that
// is wrong and does not say so".

#include "expression.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

// satellite.file.new(path) 1 8 1, open(path, mode) 1 8 2, clear(path) 1 8 3,
// new(path, mode) 1 8 4, exists(path) 1 8 5, open(path) 1 8 6, delete(path) 1 8 7, copy(path) 1 8 8,
// paste() 1 8 9, paste(d) 1 8 10, move(path, d) 1 8 11.
bool is_file_word(token::Code code);

// How many arguments a file word takes (0 for paste(), 1 or 2), and the sentence that says so.
// The CHECKER reads these too, so a wrong count is refused before anything runs.
std::size_t file_word_arity(token::Code code);
std::string file_word_takes(token::Code code);

// How many arguments a file METHOD takes, or -1 for a method a file does not have. The checker reads it,
// and run_file_number refuses a wrong count by it before a number runs.
int file_method_arity(token::Code method);

// WHERE A PATH A PROGRAM WROTE POINTS: relative to the .satl the call is written in, and a leading / under
// the program's own folder first (SATELLITE_FILE_OPERATIONS R2) -- at the prompt, the working directory's.
// `making` is true when the path is to be made, so only the folder it goes in must be there; `folders` when
// every missing folder is made, so only its first folder must be. The words of FILE_AND_DRIVE_PLAN.md find
// their paths here too, satellite.directory.create among them.
std::string resolved_path(const std::string &written, bool making, const std::vector<std::bitset<16>> &row,
                          ExpressionContext &context, bool folders = false);

// EVERY OTHER HANDLE'S APPENDED LINES ON THE DISK BEFORE `path` IS READ (ERRORS4 006): an open and
// satellite.info.file read what a handle that appended to that file is holding, as SATELLITE_FILE_OPERATIONS
// 3.7 says they do. `path` is resolved already; each handle is written under its own lock.
void write_held_appends(const std::string &path, ExpressionContext &context);

// One of those words, its arguments already evaluated. `row` is the row the call
// is written in, which is how a relative path finds the file that wrote it.
Value call_file_word(token::Code code, const std::vector<Value> &arguments,
                     const std::vector<std::bitset<16>> &row, ExpressionContext &context);

// THE NUMBER A FILE'S METHOD RUNS -- the code of satellite.variable.file.<it>, 1 6 2 n (NM-8; the author,
// 2026-10-11: "we still need to get the word satellite onto everything ... everything builtin needs the word
// satellite still") -- or 0 for a method a file does not have. Every method file_method_arity counts is one:
// 003's seven kept their numbers (open 2, read_all 5, close 6, exists 7, ok 8, path 9, error 10) and the
// rest took 12 to 30 that day, in SATELLITE_FILE_OPERATIONS.md 3.3's order.
token::Code file_number_of(token::Code method);

// A FILE'S METHOD, RUN BY ITS NUMBER: the count and the brackets refused as they always were, the eight
// questions about lines refused on a file that is not open (file_method_parts::open_or_stop says why), and
// then the function its file under satellite-numbers/words/ put in the number's row. `number` is
// file_number_of(method), already asked, never 0. `receiver` holds the handle, and a file is one thing to
// every name for it -- so no method needs the name's own object, as a list's and a string's changes do.
// The walker runs .lock() and .unlock() through here BEFORE it takes the file's hold, and every other
// method inside it (expression.cpp).
Value run_file_number(token::Code method, token::Code number, const Value &receiver,
                      const std::vector<Value> &arguments, bool had_parentheses, const std::string &name,
                      ExpressionContext &context);

// A METHOD A FILE DOES NOT HAVE, written with a `.` -- its only business since its methods run their
// numbers (2026-10-11): said, never run. `name` is the variable, for the sentence.
Value call_file_method(token::Code method, const std::string &name, ExpressionContext &context);

// THE PIECES A FILE'S NUMBERED METHODS SHARE with satellite.file's words and with f[n], in a namespace of
// their own (as a string's string_method_parts and a list's container_parts).
namespace file_method_parts {

// HOW A SENTENCE SAYS THE CALL -- `f.satellite.append`, `satellite.file.new`, `f[...]` -- worked out only
// when one is said: a method that is right spells nothing, as a list's numbered methods spell nothing.
// `word` is the method, said after `.satellite.`; with none, `name` is the whole call.
struct Said {
    const std::string &name;
    const char *word = nullptr;
    std::string text() const { return word == nullptr ? name : name + ".satellite." + word; }
};

// A count or a line number coming out, as a number.
Value a_number(std::size_t n);

// Text coming OUT of a file, as a string; refused (file_not_text) if a line is not UTF-8.
Value a_string(const std::string &utf8, ExpressionContext &context);

// TEXT GOING IN: a string, or a number's digits with S020 in satellite.log.
bool text_of(const Value &value, std::string &out, const Said &said, ExpressionContext &context);

// A LINE NUMBER GOING IN: a number no less than 0; one too large for any line is past every file's end.
bool number_of(const Value &value, std::size_t &out, const Said &said, ExpressionContext &context);

// A file's own words about itself -- its path and its last error -- never a reason to stop.
Value a_string_always(const std::string &bytes);

// A LINE THAT WAS READ, or the program stopped with the file's own code and reason.
Value a_line_or_stop(bool read, const std::string &text, const satellite_file &file, const Said &said,
                     ExpressionContext &context);

// A QUESTION ASKED OF A FILE THAT IS NOT OPEN STOPS THE PROGRAM; true when it is open.
bool open_or_stop(const satellite_file &file, const std::string &name, const Said &said, ExpressionContext &context);

} // namespace file_method_parts

// `file[n]`: line n, counting from 1. Stops the program when the line cannot be read.
Value read_file_line(satellite_file &file, const Value &index, const std::string &name, ExpressionContext &context);

// A method code's spelling, for any refusal that names one.
const char *method_spelling(token::Code method);

// WHOSE A METHOD IS, for the refusal of one called on a type that does not have
// it: "so far it is a file's" when a file has it, "so far no type has it" when
// nothing does yet -- the infinity's .power_of, .resize and .nines, numbered in
// INF-1 and built from INF-2 (SATELLITE_INFINITY.md). Asked of file_method_arity,
// never a list of its own, so it cannot go stale when a type gains a method.
const char *so_far_whose(token::Code method);

} // namespace satellite004
