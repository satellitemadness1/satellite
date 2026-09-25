#pragma once
// satellite/bytecode/info_calls.hpp -- satellite.info's words, and what a
// satellite.variable.info holds (the author, 2026-09-25, satellite.variable.info/
// README.md):
//
//     satellite.container.list
//         satellite.container.map
//             satellite.variable.string (file_name, directory_name)
//             satellite.variable.float (size)
//             satellite.variable.string (size_type) (b/kb/mb/gb/tb)
//             satellite.variable.number (line_count, only for text files)
//             satellite.variable.string (permissions)
//
//     satellite.variable.info my_infO_variable = satellite.info.file OR
//     satellite.info.directory or dir for short
//
// AN INFO IS A LIST, ONE INDEX A NAME. `satellite.info.file(path)` 1 30 1 is a list of
// one, about the file (or directory) at the path; `satellite.info.directory(path)`
// 1 30 2, or `satellite.info.dir(path)`, one for every name in the directory, in
// satellite.directory.list(d)'s order -- the names come from that word's own library.
// 004's map is satellite.container.index, so the entries are indexes, read by key:
// `info[0]["name"]`. `satellite.variable.info` is a type word for a list, as
// satellite.variable.arguments is one for an index (type_shape.hpp).
//
// THE KEYS, the author's five first and in his order, then what else the prompt's
// table shows, so a program can draw that table itself:
//
//     name         string   the name, as the directory holds it
//     size         float    in size_type's unit, to the thousandth -- the size column's rule
//     size_type    string   b, kb, mb, gb, tb (then pb, eb)
//     line_count   number   a text file's lines, the number satellite.file.open(path).size()
//                           answers; only a text file has this key
//     permissions  string   rwxr-xr-x
//     type         string   dir, file, link, fifo, sock, dev
//     bytes        number   the size exactly, in bytes
//     files        number   a directory's: the names directly in it that are not directories
//     sub          number   a directory's: its subdirectories and everything in them
//     complete     bool     a directory's: false when something in it could not be read,
//                           so size, files and sub are "at least" (the table's +)
//     approximate  bool     a directory's: true when it is a drive's top and sub is the
//                           drive's own count (the table's ~)
//     owner, created, modified   strings, as the table writes them
//
// A name with no size -- a link, a device, a file in /proc or /sys -- has no size,
// size_type or bytes key; asking an index for a key it does not hold is refused by
// name, so a program asks `.contains("size")` first.
//
// THE RULES ARE THE TABLE'S, from the same code (satellite/satl/listing_counts.hpp):
// the walk never follows a link or enters a mount, a drive's top answers for itself,
// and what stores nothing has no size, except /proc/kcore, which is the memory. At the
// prompt, Ctrl-C stops a long walk and the line with it (130 interrupted).

#include "expression.hpp"

#include <vector>

namespace satellite004 {

// satellite.info.file(path) 1 30 1, satellite.info.directory(path) 1 30 2 -- and
// satellite.directory.free(d) 1 18 7 (the author, 2026-09-25: "satellite.directory.free
// ("/home/madness") will give you a float of the free space available"): a float, in mb
// to the thousandth, the number the listing's FREE SPACE IN DIRECTORY line writes --
// df's Avail, what a person who is not root may still write there.
bool is_info_word(token::Code code);

// One of those three words, its one argument already evaluated.
Value call_info_word(token::Code code, const std::vector<Value> &arguments, ExpressionContext &context);

} // namespace satellite004
