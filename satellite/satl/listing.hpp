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
// file, link, fifo, sock, dev. 003 read up to 64 KB of every file to say whether
// it was text, which makes listing a directory of a thousand files a thousand
// reads -- a listing should cost one stat an entry. If the author wants 003's
// column back it is one function here and no change anywhere else.
//
// A NAME THAT CANNOT BE STAT'ED still lists, with `-` in every column but its
// own: a file deleted between the read and the stat is not a reason to refuse
// the whole table.

#include <string>
#include <vector>

namespace satellite004 {

// The table for `names`, each a leaf inside `directory`, in the order given.
std::string listing_table(const std::string &directory, const std::vector<std::string> &names);

} // namespace satellite004
