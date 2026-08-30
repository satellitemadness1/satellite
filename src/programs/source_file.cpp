// Reading a satellite source file. See programs/source_file.hpp for the seam.
//
// ONE FUNCTION AND A HUNDRED LINES OF COMMENT, deliberately. Everything below
// was measured on this machine rather than reasoned about, because the two
// obvious ways to write this are each wrong in a way that takes months to
// surface: one costs 1.4 MB on every installed copy, and the other is 2.5x
// slower on anything big. The numbers are the whole point of the file.

#include "programs/source_file.hpp"

#include <cstdio>
#include <string>
#include <utility>

namespace satellite {

// A whole file as one string, or false if it could not be read.
//
// SEPARATE FROM readable(), and both are called, because they answer different
// questions and the gap between them is real: a file can pass access(R_OK) and
// still fail to open -- it can be a directory, it can be replaced between the
// two calls, and on a full or failing disk the read itself can stop short.
// ios::binary because a source file's bytes are the lexer's input exactly as
// they sit on disk; DESIGN §5.3's whole argument is that nothing may rewrite a
// program's text before the lexer sees it, and a newline translation is a
// rewrite.
bool read_file(const std::string &path, std::string &into)
{
// <cstdio> AND NOT <fstream>. TWO MEASUREMENTS DECIDED THIS AND THEY POINT
// IN DIFFERENT DIRECTIONS, so both are here rather than the one that agrees.
//
// ---- 1. SIZE. <fstream> costs 1.4 MB and buys nothing back. -----------
//
// Measured on this machine 2026-08-30, same tree, same flags, the obvious
// ifstream-plus-ostringstream version against this one:
//
//     STATIC=full    1,111,528 -> 2,596,120     +1,484,592
//     STATIC=1         367,568 -> 1,657,808     +1,290,240
//
// Touching <fstream>, <sstream> or <iostream> drags in the iostreams
// machinery -- locales, facets, and the static initialisation behind them
// -- none of which this binary otherwise uses, because it prints with fputs
// and fprintf throughout. satl is installed STATIC in both of the
// installer's layouts, so that is 1.4 MB on every copy, to read a file into
// a string. FOR SCALE: the whole of M3 -- the lexer, the alphabet and the
// token dump -- cost 32,696 bytes. One convenient header would have cost
// forty-five times the milestone.
//
// ---- 2. SPEED. fstream's good idiom is 2.5x faster, and the reason is ---
// ----    not fstream. -------------------------------------------------
//
// Same day, best of seven runs, reading a whole file into a std::string.
// The 8.1 MB file is 200,000 lines of generated satellite; the 273-byte one
// is example/hello_world.satl, which is the size everything in example/
// actually is:
//
//                                   273 bytes      8.1 MB
//     fread, growing the string        5.11 us     5,929 us
//     ifstream + ostringstream         4.26 us     8,089 us
//     ifstream + seekg/read            5.09 us     2,325 us
//     fread, sized first (THIS)        5.68 us     2,329 us
//
// THE HONEST READING, AND IT IS NOT THE FLATTERING ONE. The first version
// of this function grew the string with a 64 KB append loop and was 2.5x
// slower than the ifstream idiom it had just been chosen over on size
// grounds. But the last two rows are the same number: the win was never
// fstream's, it was SIZING THE BUFFER ONCE instead of reallocating and
// copying the way up. Taking that technique costs nothing and keeps the
// 1.4 MB. Had only the first two rows been measured, the conclusion would
// have been "fread is faster" -- which is true of the idiom and false of
// the library, and is the kind of comparison that gets quoted for years.
//
// At 273 bytes all four are within 1.4 us and the choice does not matter;
// this is for the milestone that reads a real program, not for today.
//
// ---- 3. WHAT IS NOT MEASURABLE: the close. ----------------------------
//
// An ifstream closes itself however the scope is left. A bare fopen/fclose
// pair leaks the handle if anything between them throws -- and something
// can, because appending allocates and std::bad_alloc is a real answer on a
// big file or a tight machine. That is the ONE thing <fstream> was actually
// buying, and Closer below is six lines and zero bytes of it. Harmless in
// this caller, which is main() and would take the process down anyway; not
// harmless in M4.5's .satc reader or an include chain, which read in a loop.
//
// "b" is explicit even though it does nothing on Linux, because a source
// file's bytes are the lexer's input exactly as they sit on disk -- DESIGN
// §5.3's whole argument is that nothing may rewrite a program's text before
// the lexer sees it, and a newline translation is a rewrite.
struct Closer {
    std::FILE *file;
    ~Closer() { if (file) std::fclose(file); }
} closer{std::fopen(path.c_str(), "rb")};

if (!closer.file)
    return false;

// SIZED FIRST, THEN READ, THEN KEEP READING ANYWAY -- and the third clause
// is what makes the first one safe. Measured 2026-08-30 on this machine,
// 8.1 MB, best of seven runs of 200: growing the string with a 64 KB append
// loop takes 5,929 us and reading into a string sized up front takes 2,329,
// because the loop reallocates and copies its way up. At 273 bytes -- the
// size of every program in example/ -- the two are 5.1 us against 5.7 and
// the difference is noise, so this is for the milestone that reads a real
// program, not for today.
//
// ftell IS A HINT AND NOT AN ANSWER, which is the whole reason the loop
// below survives. It answers -1 on a pipe, 0 on most of /proc, and a stale
// number on a file somebody is still writing. So it is used only to size
// the first read; the loop then continues from wherever that got to and
// ends where the data does. A wrong hint costs one reallocation, and a
// missing one costs nothing at all.
std::string all;
if (std::fseek(closer.file, 0, SEEK_END) == 0) {
    const long size = std::ftell(closer.file);
    if (size > 0)
        all.reserve(static_cast<size_t>(size));
}
std::rewind(closer.file);

char buffer[65536];
size_t got;
while ((got = std::fread(buffer, 1, sizeof buffer, closer.file)) > 0)
    all.append(buffer, got);

// ferror AND NOT just "did we reach the end": a read that stops short on a
// failing disk, or on a path that opened but is not a file at all -- a
// DIRECTORY opens fine on Linux and fails here with EISDIR -- gives a short
// answer rather than no answer, and a truncated program that lexes cleanly
// is worse than one that will not open.
if (std::ferror(closer.file))
    return false;

into = std::move(all);
return true;
}

} // namespace satellite
