#pragma once
// satellite/bytecode/delete_calls.hpp -- satellite.delete(name): A NAME, AND EVERYTHING ABOUT IT,
// GONE FROM MEMORY. NEW_MILESTONES.md NM-2.
//
// The author, 2026-10-06: "satellite.delete(name) will delete everything about "name" from
// everywhere except for in the actual file where the text is written, and the copy of that file
// that exists inside of the running interpreter ... we will not erase the value inside of the
// bytecode", and "everywhere else, the "name" needs deleting, which is only in memory and in
// history, and we will not delete it from inside of .history".
//
// SO THE LINE TAKES THE NAME OUT OF THE BODY THAT HOLDS IT (program_walk.cpp's run_delete): the
// table's entry, which is the name, the word that declared it, what was between its < and > and its
// value -- and a file it held alone, saved and closed as a body's end saves and closes one. At the
// prompt that body is the session's own table, so no later line sees it. AND THE CHECKER FORGETS IT
// from that line on (program_check.cpp's judge_a_delete), so a later line that names it is refused
// before anything runs, telling the line it was deleted on, and the name may be declared again, as
// anything. The file on the disk and the bytecode read from it are not touched; there is no
// satellite.history yet, and its .history files will keep the name.
//
//     satellite.variable.string big = "a long text"
//     satellite.delete(big)
//     satellite.console.display(big)         // refused before anything runs: big was deleted on line 2
//
// A LINE OF ITS OWN, WITH ONE NAME IN IT: it answers nothing. AND IN THE BLOCK THAT DECLARED THE NAME:
// the checker reads a body once, top to bottom, so a delete inside an if or a loop of a name declared
// outside it would leave every line after it depending on whether that block ran -- and a loop's next
// pass finding the name already gone. Refused before anything runs too: a field of the object a
// capsule runs on (it goes with its object), satellite.statement.for's own number inside its loop,
// and `arguments`, the run's own rows.
//
// NOT satellite.system.delete(x) (1 22 1, numbered and not built): that is 003's word for deleting a
// FILE from the disk. This deletes a NAME from memory.

#include "token_codes.hpp"

#include <bitset>
#include <string>
#include <vector>

namespace satellite004 {

class satelliteObject;

// satellite.delete 1 32, and satellite.delete(name) 1 32 1, which a call lexes to.
bool is_delete_word(token::Code code);

// THE ONE READER OF THE LINE'S SHAPE, the checker's and the walker's -- two readers of one shape
// drift apart (program_walk.hpp). `at` on the word. Answers the name, with `past` on the code after
// its `)`, or "" with `why` said: for anything but one name in the brackets, or anything after them.
std::string the_name_to_delete(const std::vector<std::bitset<16>> &row, std::size_t at, std::size_t &past,
                               std::string &why);

// WHAT IS SAID WHEN satellite.delete STANDS INSIDE ANOTHER LINE.
extern const char kDeleteIsALineOfItsOwn[];

// THE NAME TAKEN BACK FROM WHAT THE NAME HELD (the fresh reader, 2026-10-06): a program keeps the name it
// was started under, for the S742 that says it was never joined (program_stop.cpp) -- so every program
// the deleted name held, itself or in a list, a map or an object's fields, forgets a name that is this
// one, or this one followed by [ or a dot. A thread keeps no variable's name at all: thread_calls.cpp
// labels one by its capsule.
void forget_the_name_in(const satelliteObject &held, const std::string &name);

} // namespace satellite004
