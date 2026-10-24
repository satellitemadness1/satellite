#pragma once
// satellite/bytecode/drive_calls.hpp -- satellite.drive's words, run by the interpreter (FILE_AND_DRIVE_PLAN.md,
// FD-4 and FD-5; the author, 2026-10-21: "satellite.drive.list() // lists all drives").
//
//     satellite.drive.list()     a table of every drive that holds a filesystem, mounted or not      1 36 1
//
// BUILT: list. mount(d), release(d) and remount(d) -- 1 36 2 to 1 36 4 -- are FD-5's, numbered and not built,
// so the checker still says each has no library built for it yet.
//
// THE WORDS HAVE NO LIBRARY, as satellite.file's have none (file_calls.hpp): list reads the machine's /sys and
// udev records and prints, and FD-5's will ask on the console and run udisksctl or sudo -- the interpreter's
// to do, not a library's. So the checker knows them by is_drive_word().
//
// list PRINTS THE SAME TABLE AT THE PROMPT AND IN A PROGRAM (rule 7), and answers nothing. Refused before
// anything runs: any argument (it takes none).

#include "expression.hpp"

#include <string>
#include <vector>

namespace satellite004 {

// satellite.drive.list() 1 36 1 -- the one built.
bool is_drive_word(token::Code code);

// WHAT THE CHECKER REFUSES BEFORE ANYTHING RUNS, and the walker asks again on its way in: `given` is how many
// arguments the call was written with. success, or the code and the sentence.
signed long long int drive_word_refused(token::Code code, std::size_t given, std::string &why);

// The call: the table printed, and nothing answered -- or a refusal in `context`.
Value call_drive_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
