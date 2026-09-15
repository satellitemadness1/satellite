#pragma once

// The arms that DUMP A REGISTRY -- `satl --words` and `satl --errors`.
//
// THIS IS THE SEAM MILESTONES/M6.md §6.1 NAMED AND DECLINED, TAKEN AT M7. That
// note says it plainly: "the arms that DUMP a registry (`--words`, `--errors`,
// `--limits`) and the arms that read a FILE (`--tokens`, `--unparse`, `--satc`,
// `--check`) are two subjects, and splitting them is a reshaping of M2 through
// M5's arms rather than of this milestone's." It was right to decline -- M6 had
// no business reshaping four other milestones' commands -- and it was also
// counting: `main.cpp` was 427 lines, 127 over what PLAN §3 asks a file to be
// built toward, and M7 adds a fourteenth arm to the same switch. A seam does
// not get better by waiting.
//
// WHAT THE TWO SUBJECTS ACTUALLY ARE, said once here rather than inferred from
// the split. A registry dump takes NO OPERAND OR A KEY INTO ITSELF, answers out
// of tables compiled into the binary, opens nothing, and cannot fail in a way
// that is about the user's program. A file arm takes a PATH, reads a file that
// may not exist, and has an exit status about what was in it. They share only
// the switch they hang off.
//
// AND THE OPERAND SPLIT IS THE SAME IN BOTH OF THESE. A key that resolves is an
// answer and goes to stdout; a key the language does not have is a command line
// naming something satl cannot do, so it goes to stderr with the status a bad
// option gets. That is exactly what a FILE arm must NOT do -- a program with a
// bad token in it is not a bad command line -- and `--tokens` shipped with the
// wrong one of those for a day. Having the two subjects in two files is what
// makes the difference visible instead of adjacent.

#include <string>

namespace satellite {

// `satl --words [path]`: the whole numbering, or the one path `key` names.
int words_command(const std::string &key);

// `satl --errors [code]`: every message satl can say, or the one `key` names.
int errors_command(const std::string &key);

// The key a dump was given when it was given none -- an empty string is a legal
// operand nowhere, so it needs no second flag.
inline const std::string kNoKey;

} // namespace satellite
