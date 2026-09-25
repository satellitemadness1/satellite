#pragma once
// satellite/satl/listing.hpp -- the table a person sees when
// `satellite.directory.list()` or `list(d)` is a whole typed line (PLAN M0.6).
//
// THE SESSION DRAWS IT, NOT THE LIBRARY, and that is the difference from 003.
// There, the handler asked a thread_local "was a table wanted?" before it ran and
// drew the table itself; a mode set from outside is a thing anything else can
// trip. Here the session sees on the COMPILED LINE that the statement is exactly
// `1 18 4` or `1 18 5` -- never on its text -- calls the library, and draws the
// names it answered. In a program the same call answers names and no table is
// drawn, because nothing asked for one.
//
// NOTHING REACHES THE TERMINAL RAW (D0.6.3). Every name, every owner and the
// directory's own path go through shown(): a file named `ESC]2;...BEL` shows as
// \x1b]2;...\x07 and cannot retitle the window. Columns are counted in CELLS, so
// a CJK name lines up -- the same counting the prompt's renderer does, through
// the same place().
//
// THE TYPE COLUMN IS WHAT THE SYSTEM KNOWS, and this is D0.6.7 answered: dir,
// file, link, fifo, sock, dev -- never a file read to guess.
//
// THE SIZE COLUMN FOLLOWS TYPE (the author, 2026-09-24): bytes under 1024, then kb
// and mb to three places -- size_of() in listing.cpp has the rule.
//
// THEN files AND sub, AND A DIRECTORY HAS A SIZE (the author, 2026-09-25). For a
// directory: size is every file under it, at every depth, added up; files is how
// many names directly in it are not directories; sub is its subdirectories and
// everything in them, a dash when it has none -- so files + sub is every name under
// it. For a text file, sub is its line count in parentheses, "(12)", the only
// number in the table written that way. A count that met something it could not
// read ends in "+": at least that many.
//
// SO A LISTING READS NOW (listing_counts.hpp), where it cost one stat an entry
// until this: a directory row walks the tree under it (a stat for every regular
// file, for its size) and a file row reads the file (text or not is the whole
// file's answer). Ctrl-C stops it between names and between 256 KiB pieces, and
// stops with no table, as the library's own read stops with no names.
//
// ABOVE THE TABLE, ONE LINE (the author, 2026-09-25): "FREE SPACE IN DIRECTORY:
// 10,024.080 mb", for the directory list() was given and not for each row. Always
// mb -- "gb is just too big" -- to three places, with a comma every three digits.
// The free space is what df(1) calls Avail: f_bavail, what a person who is not root
// may still write. A dash when the system will not say. The session writes it white
// at a terminal, as the prompt's letters are, and before the walk begins, so a table
// that takes seconds to count has something on the screen at once.
//
// A NAME THAT CANNOT BE STAT'ED still lists, with `-` in every column but its
// own: a file deleted between the read and the stat is not a reason to refuse
// the whole table.

#include <csignal>
#include <string>
#include <vector>

namespace satellite004 {

// 10,024 -- a comma between every three digits.
std::string with_commas(unsigned long long int number);

// 10,024.080 mb -- always mb, to three places, with commas: the author's way of
// writing space (2026-09-25), shared with satellite.directory.system()'s table.
std::string megabytes_with_commas(unsigned long long int bytes);

// Rows of cells lined up in columns two spaces apart, counted in CELLS through the
// renderer's place(); a column marked in `from_the_right` is written from the right.
std::string lined_up(const std::vector<std::vector<std::string>> &rows, const std::vector<bool> &from_the_right);

// "FREE SPACE IN DIRECTORY: 10,024.080 mb" for `directory`, with no colour and no newline.
std::string free_space_line(const std::string &directory);

// The table for `names`, each a leaf inside `directory`, in the order given, into
// `table`. False, with `table` empty, when `stop` was raised while it was counted.
// `at_a_terminal`: whether a long count may show its progress line.
bool listing_table(const std::string &directory, const std::vector<std::string> &names,
                   const volatile sig_atomic_t *stop, bool at_a_terminal, std::string &table);

} // namespace satellite004
