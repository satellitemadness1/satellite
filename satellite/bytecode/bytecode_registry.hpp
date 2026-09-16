#pragma once
// satellite/bytecode/bytecode_registry.hpp -- the .satl turned into 16-bit
// tokens, one row a source line.
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
// ONE ROW A SOURCE LINE, and the row is that line's tokens. A row always ends
// with line_end_token; the last row of the file ends with end_of_file_token, so
// a reader that has a row has a whole statement and never has to look back.
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

// One row a source line; each row is that line's 16-bit tokens.
using BytecodeRegistry = std::vector<std::vector<std::bitset<16>>>;

// Turns `source` into `registry`, one row a line, on `threads` in batches.
//
// ALWAYS ANSWERS success, because the lexer never throws (DESIGN §5.6). A
// character the registry has no code for is marked in the stream with
// error_token and counted in a report; the conversion finishes either way, so
// whatever reads the registry next sees the whole program and can name the
// fault with a position instead of the run stopping here.
signed long long int build_bytecode_registry(const std::string &source,
                                             StartupThreads &threads,
                                             unsigned long long int batches,
                                             BytecodeRegistry &registry,
                                             MachineState &state);

// One line, alone and on this thread. The unit every batch runs, and what the
// tests drive directly.
void tokenise_one_line(std::string_view line, std::vector<std::bitset<16>> &row);

// Every code of `row` as sixteen binary digits, space separated -- the form
// REGISTRY.satellite's first column is written in, and what M1.5's converter
// prints. This is why the rows are std::bitset<16>.
std::string row_as_bits(const std::vector<std::bitset<16>> &row);

// How many codes the registry holds, counting every row's tokens and payloads.
unsigned long long int codes_in(const BytecodeRegistry &registry);

} // namespace satellite004
