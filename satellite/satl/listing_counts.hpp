#pragma once
// satellite/satl/listing_counts.hpp -- what the prompt's directory table counts
// (the author, 2026-09-25): everything under a directory, for its row's size, files
// and sub cells, and a text file's lines, for its sub cell. listing.cpp draws the
// table; this is the reading behind it, kept apart because it is the one part of a
// listing that reads the disk past a stat, and the one part that can take long.

#include <atomic>
#include <csignal>
#include <string>
#include <vector>

namespace satellite004 {

// WHAT A DIRECTORY HOLDS, all the way down: its row's size, files and sub cells
// (the author, 2026-09-25). files + below is every name under it, as find(1)
// counts them.
//
// A DRIVE'S TOP IS ANSWERED BY THE DRIVE (the author, 2026-09-25, after / took two
// minutes in a 13-million-name drive under /run/media): a directory that is where a
// filesystem with blocks is mounted -- a disk, tmpfs, devtmpfs -- has for its size the
// space the filesystem says is used, df's Used, and, when the filesystem counts its
// names, that count for its sub, marked `about` since hard links and the filesystem's
// own reserved names make it near and not exact. Both come back at once, where the
// walk they replace read every name on the drive. A filesystem with no count (vfat,
// exfat) is walked for its counts, and still answers its own size.
//
// A FILESYSTEM THAT STORES NOTHING (filesystems.hpp: /proc, /sys ...) adds nothing
// to a size, except /proc/kcore, which is the machine's memory.
struct Contents {
    bool opened = false;                // false: it could not be read, and all three cells are dashes
    bool whole = true;                  // false: something in it could not be read, so size and sub are "at least"
    bool files_whole = true;            // false: a name directly in it could not be read, so files is too
    bool by_the_filesystem = false;     // true: bytes is the filesystem's own used space
    bool about = false;                 // true: below is the filesystem's own count, near and not exact
    unsigned long long int files = 0;   // names directly in it that are not directories
    unsigned long long int below = 0;   // its subdirectories and everything in them, at every depth
    unsigned long long int bytes = 0;   // every regular file's size, at every depth
};

// ONE DIRECTORY OPEN A LEVEL, NEVER A LINK FOLLOWED, NEVER A MOUNT ENTERED. The
// walk goes down through openat(O_NOFOLLOW) from the directory above, so a link to
// `/` or back up the tree is one name counted and never a walk, and a tree changing
// while it is read cannot send the walk somewhere else. A directory that is the top
// of a mount (statx's STATX_ATTR_MOUNT_ROOT) is one name and is not entered, as
// find -xdev does: what is mounted there is another filesystem's to count. A name
// gone between the read and the stat is not counted; a directory that cannot be
// opened is counted, and makes the total "at least". The type readdir gives saves a
// stat for everything but a regular file, whose size is the one thing the walk needs
// from it.
//
// `counted`, when not null, has one added for every name passed -- what the progress
// line reads (listing_progress.hpp). Answers false when Ctrl-C was pressed, checked
// between names as the library's read checks it.
bool count_contents(const std::string &path, const volatile sig_atomic_t *stop,
                    std::atomic<unsigned long long int> *counted, Contents &out);

// A TEXT FILE'S LINES (the author, 2026-09-25), the number satellite.file.open(path)
// .size() answers for the same file -- satellite_file.cpp's reader has the rules: a
// byte-order mark at the start is no part of line 1, \n ends a line, the piece after
// the last \n is a line when there is one, and a \r alone or bytes that are not
// UTF-8 make it not text. ONE RULE MORE: A NUL MAKES IT NOT TEXT, as it did in 003's
// listing and does in file(1). satellite.file reads a NUL as a character, so a sparse
// 5 GB file of zeros would be one line of text there; here it is a dash, found in
// the first 256 KiB.
//
// The whole file is read, in 256 KiB pieces, because a count is of all of it; it
// is never held. O_NONBLOCK and O_NOFOLLOW for the row's lstat: only a regular file
// is ever opened, and it cannot turn into a fifo that hangs the prompt.
//
// `piece` is the TABLE'S ONE READ BUFFER, sized here the first time a file is read:
// 256 KiB made and zeroed once a table, not once a file -- a folder of 100,000 small
// files would otherwise zero 26 GB to read a few MB.
//
// Answers false when Ctrl-C was pressed; otherwise `text` says whether it is, and
// `lines` is the count when it is.
bool count_lines(const std::string &path, const volatile sig_atomic_t *stop, std::vector<unsigned char> &piece,
                 bool &text, unsigned long long int &lines);

} // namespace satellite004
