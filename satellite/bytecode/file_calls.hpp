#pragma once
// satellite/bytecode/file_calls.hpp -- satellite.file's words and a file's
// methods, run by the interpreter. SATELLITE_FILE_OPERATIONS.md Part 3.
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
// new(path, mode) 1 8 4, exists(path) 1 8 5, open(path) 1 8 6.
bool is_file_word(token::Code code);

// How many arguments a file word takes (1 or 2), and the sentence that says so.
// The CHECKER reads these too, so a wrong count is refused before anything runs.
std::size_t file_word_arity(token::Code code);
std::string file_word_takes(token::Code code);

// How many arguments a file METHOD takes, or -1 for a method a file does not have.
int file_method_arity(token::Code method);

// One of those words, its arguments already evaluated. `row` is the row the call
// is written in, which is how a relative path finds the file that wrote it.
Value call_file_word(token::Code code, const std::vector<Value> &arguments,
                     const std::vector<std::bitset<16>> &row, ExpressionContext &context);

// `file.method(arguments)`. `had_parentheses` is false for `f.size`, which the
// zero-argument methods allow. `name` is the variable, for the refusals.
Value call_file_method(token::Code method, satellite_file &file, const std::vector<Value> &arguments,
                       bool had_parentheses, const std::string &name, ExpressionContext &context);

// `file[n]`: line n, counting from 1. Stops the program when the line cannot be read.
Value read_file_line(satellite_file &file, const Value &index, const std::string &name, ExpressionContext &context);

// A method code's spelling, for any refusal that names one.
const char *method_spelling(token::Code method);

} // namespace satellite004
