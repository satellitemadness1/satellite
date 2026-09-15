#pragma once

// The arms that READ A FILE and print what a pass made of it -- `satl --tokens`
// and `satl --unparse`. programs/dump_commands.hpp is the other half of the seam
// and carries the argument for the split.
//
// THE THREE OTHER FILE ARMS ALREADY HAVE FILES OF THEIR OWN, and that is what
// says this pair belongs together rather than each alone. `--satc` is a loop,
// `--check` is a status with no output, `--resolve` has a decision about which
// text a caret may be drawn into -- each needed somewhere to put a paragraph.
// These two are one call and a choice of stream, and the paragraph they need is
// the SHARED one below, which is why they are the two that stayed inline until
// there was somewhere for both.
//
// THE RULE THEY SHARE, AND IT COST A DAY TO GET RIGHT ONCE. The dump goes to
// STDOUT even when the program is malformed. `--words` sends a bad operand to
// stderr because there the operand IS the command line; here the operand is a
// FILE, and a program with a bad token in it is not a bad command line. The
// question asked was "what does this pass make of this file", the answer
// includes the Error token, and `satl --tokens bad.satl > tokens.txt` must put
// it in the file. Copying `--words`' split was this arm's first version and it
// produced an EMPTY tokens.txt with the whole dump on stderr.

#include <string>

namespace satellite {

// `satl --tokens <file>`: the token stream, DESIGN §5's decisions made visible.
int tokens_command(const std::string &path);

// `satl --unparse <file>`: the tree printed back as a satellite program.
int unparse_command(const std::string &path);

} // namespace satellite
