#pragma once

// Reading a satellite source file into memory.
//
// SPLIT FROM main.cpp BY SUBJECT, NOT BY LINE COUNT -- the same seam
// programs/terminal.hpp was split on. programs/opening.hpp says of itself that
// "nothing here knows what a satellite program is", and that is the line: main
// answers the command line and decides WHAT TO SAY when a file will not open,
// which is a program's business; getting the bytes off the disk unchanged is
// this file's, and it is what M3's --tokens, M4's parser, M4.5's .satc reader
// and M8's runner all need the same answer to.
//
// The line count is what prompted the look -- main.cpp reached 396 against
// PLAN §3's target of 300 -- and the seam is what justified the split. The .cpp
// carries a long comment because the two obvious ways to write this function
// are each wrong in a way that does not show up until much later, and both were
// measured rather than argued.

#include <string>

namespace satellite {

// The whole file as one string, or false if it could not be read.
//
// FALSE COVERS MORE THAN "NOT THERE". A directory opens fine on Linux and fails
// on the first read; a file can pass access(R_OK) and still not open; a read
// can stop short on a failing disk. All of those come back false, and the
// caller says so in its own words rather than this function printing anything.
//
// NO NEWLINE TRANSLATION, EVER. DESIGN §5.3's argument is that nothing may
// rewrite a program's text before the lexer sees it, and a newline translation
// is a rewrite -- so the bytes arrive exactly as they sit on disk.
bool read_file(const std::string &path, std::string &into);

} // namespace satellite
