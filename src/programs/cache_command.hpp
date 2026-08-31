#pragma once

// `satl --satc <file>` -- PLAN M4.5's consumer. See SATC.md for the format and
// src/satellite_cache/cache.hpp for the module this drives.
//
// WHY IT IS NOT THREE LINES IN main.cpp LIKE THE OTHER FLAGS. --words prints a
// table, --tokens prints a list and --unparse prints a program; each is one
// call and a choice of stream. This one is SATC.md §4's reading order, which is
// three steps that have to happen in that order and a fourth thing to say when
// none of them worked, and the file it exercises is on the user's disk rather
// than in front of them. A milestone whose consumer is a loop needs somewhere
// for the loop to be.

#include "parser/parser.hpp"
#include "satellite_cache/cache.hpp"
#include "satellite_words/words.hpp"

#include <string>

namespace satellite {

// SATC.md §4's READING ORDER, ONCE, FOR THE TWO COMMANDS THAT WANT IT.
//
// M7 IS WHY THIS IS A FUNCTION AND NOT A PARAGRAPH INSIDE satc_command(). That
// milestone's `satl --resolve` needs the same three steps for a different
// reason -- it wants the TREE and the numbers the file already carried, where
// `--satc` wants the file printed back -- and a second copy of "which check
// runs first" would be the thing read.cpp's own header says this module exists
// to have exactly one of. M10 will be the third caller and will want neither.
struct Reading {
    Parse parsed;

    // What the `.satc` had already numbered, for M7. Empty on a miss, which is
    // the honest answer: the numbers were never there to take.
    cache::Marks marks;

    cache::Source stamp;
    std::string cache_file;

    // THE TEXT THE TREE'S SPANS INDEX INTO, and on a cache hit it is NOT the
    // file the user wrote -- cache.hpp's Reading says why. Anything rendered
    // about this tree has to be rendered against this string, and
    // programs/resolve_command.cpp is where the consequence of that is taken.
    std::string text;

    // The `.satc` this run would write, built from the tree either way -- which
    // is what makes `--satc` a check rather than a `cat`, and is the same text
    // on a hit and on a miss or the reader lost something.
    std::string satc;

    bool from_cache = false;
    bool ok = false;
};

// Steps 1, 2 and 3, and it SAYS ON STDERR which of them happened -- because on
// a warm run that line is the only evidence a cache was used at all.
//
// `writing` IS THE CALLER'S AND THAT IS SATC.md §5. "The run does not wait for
// the write" -- the PROCESS does, in the destructor -- so the object has to
// live in the frame that knows when the run is over, which at M10 is after the
// program has run rather than after one fputs.
Reading read_program(const std::string &path, words::Words &words,
                     cache::Save &writing);

// Run the cache over `path`: read the `.satc` if there is a usable one, walk
// the source and write a fresh one if there is not, print the `.satc` on stdout
// either way, and say on stderr which of the two happened.
//
// A COMMAND'S EXIT STATUS IS ABOUT THE PROGRAM AND NEVER ABOUT THE CACHE, which
// is §4's "a missing, stale or unreadable `.satc` is never an error" arriving at
// the one place a script can see it. A cache that could not be read, could not
// be written, or was thrown away costs a walk and nothing else; what decides
// the status is whether the user's program parsed.
int satc_command(const std::string &path);

} // namespace satellite
