#pragma once
// satellite/bytecode/sate_file.hpp -- THE ONE FILE. The 16-bit bytecode, saved.
//
// (the author, 2026-09-16) "We skipped .satc and .satb in place of something
// else, we are just saving the 16-bit bytecode as .sate and skipping
// everything... there is only argument.sate=true/false".
//
// THREE FILES BECAME ONE, AND NOTHING WAS LOST BY IT. PLAN M1-M3 built up to
// three: `.satc` the program as numbers, `.satb` combine's batch marks, `.sati`
// the strings as bits. The 16-bit bytecode is already all three at once --
//
//   - it IS the numbered program, one code a word, which is what `.satc` was for
//   - its strings are 16-bit codes INLINE and counted, which is what `.sati` was
//     for, and D3.1 (bits as binary or as 0/1 text) dies with it
//   - the registry already reserves batch_start, batch_end, wait and batch_size,
//     so combine's marks live in the codes and `.satb` survives as WORK but not
//     as a FILE
//
// PROGRESS §6.5 recommended exactly this and could not apply it without the
// author. He has now taken it.
//
// SIXTEEN BINARY DIGITS A CODE, one to a line, and that is the whole format.
// It is why `std::bitset<16>` was chosen over `uint16_t` knowing it costs 8
// bytes and 3.5-5x a pass: `.to_string()` is the sixteen digits
// REGISTRY.satellite's own column is written in, so a `.sate` and the registry
// can be read side by side. A `>` line names each file, because the registry is
// ONE ROW A FILE and a reader has to be able to tell where one ends.
//
// IT IS TEXT AND THAT IS A COST WORTH NAMING: seventeen bytes a code against
// two. A 100,000-code program is 1.7 MB. If that ever matters the format
// changes and nothing else does -- this file is the only thing that knows it.

#include "bytecode_registry.hpp"

#include <string>

namespace satellite004 {

// `program.satl` -> `program.sate`. A name with no extension just gains one.
std::string sate_path_of(const std::string &main_file);

// Writes every row. Answers success, or display_error (2) if the file cannot be
// written -- checked at the FLUSH and not only per line, because a full disk
// succeeds line by line and fails once at the end (the same trap
// satellite.console.display already fell into once).
signed long long int write_sate_file(const std::string &path,
                                     const BytecodeRegistry &registry,
                                     const BytecodeFilenames &filenames,
                                     MachineState &state);

} // namespace satellite004
