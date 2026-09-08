#pragma once

// What satellite_file's two handler files share. Not a door onto this module;
// satellite_file/handlers.hpp is -- the same split satellite_scalars keeps.
//
// THE CHECKS ARE HERE AND NOT IN EACH ROW BECAUSE THEY ARE THE CONTRACT. A
// file's refusals are the milestone: DESIGN §9 makes a failed OPEN a value, so
// the only things left to raise are program mistakes -- the wrong direction, a
// closed handle, a word that is not a mode -- and each of those has exactly one
// sentence in errors.def and must have exactly one site here.

#include "evaluator/dispatch.hpp"
#include "evaluator/machine.hpp"
#include "satellite_file/file_handle.hpp"
#include "satellite_value/value.hpp"

#include <cstdint>
#include <string>

namespace satellite::file {

// The selector or module word under the caret, for a refusal to quote.
std::string asked(eval::Machine &m);

// An argument as a filesystem path -- a `satellite.variable.string`, decoded
// to bytes -- or a refusal and false.
//
// decode() AND NEVER encode(). A POSIX filename is arbitrary bytes and the
// language's strings hold arbitrary bytes, so the round trip is exact; what
// must not happen is the ESCAPE EXPANSION going the other way, which DESIGN §5
// keeps to string LITERALS. satellite_string's `encode_raw` is the inverse used
// everywhere a filename comes back out.
bool path_at(eval::Machine &m, const Value *arguments, uint32_t who,
             std::string *out);

// The receiver as an open-or-closed handle, or a refusal and false. A slot
// declared `satellite.variable.file` and never assigned holds nothing, which is
// S0714 and not a file -- DESIGN §6.4 qualification 3's second state.
bool handle_at(eval::Machine &m, const Value *arguments, uint32_t who,
               FileHandle **out);

// THE ONE CONSTRUCTOR, and both module rows go through it so that neither can
// drift out of DESIGN §9's contract on its own. `flags` is what to open with
// NOW; `mode.flags` is what `open` `1 6 2 2` reopens with later, and
// `satellite.file.new`'s O_EXCL is exactly why those are two arguments.
//
// A FAILED OPEN IS A VALUE. The handle comes back holding errno and the caller
// asks `ok` `1 6 2 8`. Failing at the call site instead would make "does this
// file exist" unanswerable without killing the program that asked.
Value opened(const std::string &name, const Mode &mode, int flags);

// The direction check, before the syscall. Answers true and refuses with S1202
// when the handle cannot go the way it was asked; `verb` is "read" or "write
// to", and the sentence names the modes that would have worked.
bool wrong_direction(eval::Machine &m, const FileHandle &handle, bool allowed,
                     const char *verb, const std::string &modes);

// The live descriptor, or S1203 and -1. Every row that touches the file asks
// this, so "closed" has one sentence and one site.
int descriptor_of(eval::Machine &m, FileHandle &handle);

// Fill `handle.buffer` from `handle.read_at` onward. Answers false and refuses
// on a real read failure; a short read at end of file is not one.
bool fill(eval::Machine &m, FileHandle &handle, int fd);

// Bytes out of a file, as the string a program sees. `encode_raw` and never
// `encode` -- the body carries the defect that rule exists to stop.
Value as_text(const std::string &bytes);

// THE TWO READING ROWS, DEFINED IN file_reading.cpp AND INSTALLED FROM
// file_methods.cpp. They are declared across the split rather than installed
// beside themselves so that the install list stays ONE list: a table with two
// install sites is a table whose contents nobody can read off a single page,
// and satellite_file already has one such split it could not avoid
// (`satellite.system.delete`, under another parent entirely).
bool file_read_line(eval::Machine &m, const Value *arguments, uint32_t count,
                    Value *answer);
bool file_read_all(eval::Machine &m, const Value *arguments, uint32_t count,
                   Value *answer);

// The install halves handlers.cpp sums.
void install_file_methods();

} // namespace satellite::file
