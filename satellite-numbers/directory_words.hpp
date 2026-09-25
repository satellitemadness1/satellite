#pragma once
// satellite-004/satellite-numbers/directory_words.hpp -- what satellite.directory's
// words DO, shared by their libraries (PLAN M0.6): `change(d)` 1 18 1, `list()`
// 1 18 4 and `list(d)` 1 18 5, their behaviour ported from 003 06's
// src/satellite_directory/handlers.cpp -- and `system()` 1 18 6, the author's of
// 2026-09-25, which 003 did not have.
//
// ONE HEADER AND A LIBRARY A WORD, because build_libraries.py compiles one .cpp a
// word and a word with arguments cannot share an object file with its sibling.
// The .cpp files are the describe functions; the work is here.
//
// `change` ANSWERS A VALUE AND NEVER AN ERROR (003's rule, kept): true when the
// move happened, false when it did not, whatever the reason -- a program may ask
// about a directory without being stopped. It is checked BEFORE the move rather
// than reporting chdir's failure after, because chdir collapses "nothing is
// there", "not a directory" and "not allowed" into one errno.
//
// THE THREE ANSWERS `list` KEEPS APART, and why it opens where 003 stat()ed
// first: nothing is there (directory_not_found), something is there and is not a
// directory (not_a_directory), and it IS a directory whose entries cannot be read
// (directory_unreadable, with the system's own reason). O_DIRECTORY tells all
// three apart through errno in ONE syscall, and the descriptor it answers is the
// one fdopendir reads -- so a directory deeper than PATH_MAX still lists (PLAN
// M0.6) and nothing is re-resolved between the check and the read, which 003's
// second open by name allowed.
//
// A HALF-READ DIRECTORY IS WORSE THAN NONE (003): a read that fails answers its
// code with NO names, because a caller would believe a short list and its size.
//
// A PATH HOLDING A NUL IS REFUSED, because c_str() would act on the part before
// it -- a different directory than the one that was asked for.
//
// THE ORDER IS THE BYTES' HERE, AND 003's WAS ITS CHARACTER TABLE'S: 003 sorted
// decoded satellite strings, so dotfiles sorted last and a capital after a
// lowercase. A library is compiled alone, with no satellite symbols to resolve
// (build_libraries.py), so the language's own string is out of reach until the
// numbered libraries can link it -- MILESTONES M21's question. Byte order is
// stable and the same on every machine, which is what a listing owes; matching
// 003's order exactly is owed with it and named in PROGRESS.

#include "number_row.hpp"

#include "../satellite/machine/filesystems.hpp"
#include "../satellite/machine/machine_codes.hpp"

#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>

namespace satellite004 {
namespace directory_words {

inline bool holds_a_nul(const std::string &path)
{
    return path.find('\0') != std::string::npos;
}

inline DirectoryReply change(const std::string &path, bool given, const volatile sig_atomic_t *)
{
    DirectoryReply reply;
    if (!given) {
        reply.code = satl_line_not_understood;
        reply.reason = "satellite.directory.change needs the directory to move to";
        return reply;
    }
    if (holds_a_nul(path)) {
        reply.code = path_holds_a_nul;
        return reply;
    }
    struct stat about;
    const bool a_directory = ::stat(path.c_str(), &about) == 0 && S_ISDIR(about.st_mode);
    reply.flag = a_directory && ::chdir(path.c_str()) == 0;
    return reply;   // the code stays success: this word answers a value
}

inline DirectoryReply list(const std::string &path, bool given, const volatile sig_atomic_t *stop)
{
    DirectoryReply reply;
    const std::string where = given ? path : std::string(".");
    if (holds_a_nul(where)) {
        reply.code = path_holds_a_nul;
        return reply;
    }

    const int descriptor = ::open(where.c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC);
    if (descriptor < 0) {
        reply.code = errno == ENOENT ? directory_not_found : errno == ENOTDIR ? not_a_directory : directory_unreadable;
        reply.reason = std::strerror(errno);
        return reply;
    }
    DIR *folder = ::fdopendir(descriptor);
    if (folder == nullptr) {
        reply.code = directory_unreadable;
        reply.reason = std::strerror(errno);
        ::close(descriptor);
        return reply;
    }

    int failure = 0;
    for (;;) {
        // BETWEEN ENTRIES AND NOT INSIDE ONE: a listing of a million names is
        // stoppable, and stops with nothing rather than with half a directory.
        if (stop != nullptr && *stop != 0) {
            ::closedir(folder);
            reply.names.clear();
            reply.code = interrupted;
            return reply;
        }
        // Cleared before EVERY read: a null answer means both "the directory
        // ended" and "the read failed", and errno is what tells them apart --
        // readdir does not touch it on success, so a stale value would read as a
        // failure that never happened (003).
        errno = 0;
        const struct dirent *entry = ::readdir(folder);
        if (entry == nullptr) {
            failure = errno;
            break;
        }
        const std::string leaf = entry->d_name;
        // "." IS this directory and ".." is its parent, so neither is a thing IN
        // it -- and a caller walking down would descend forever. REAL DOTFILES
        // STAY: a language with no flags has one answer, so it is the true one.
        if (leaf == "." || leaf == "..")
            continue;
        reply.names.push_back(leaf);
    }
    // Before errno is looked at again: closedir may fail and set its own.
    ::closedir(folder);
    if (failure != 0) {
        reply.names.clear();
        reply.code = directory_unreadable;
        reply.reason = std::strerror(failure);
        return reply;
    }

    std::sort(reply.names.begin(), reply.names.end());
    return reply;
}

// `system()` 1 18 6: where the machine's drives are mounted, one place a drive, in
// the order the kernel mounted them (filesystems.hpp says what a drive is). Typed
// alone at the prompt it draws their table instead (satellite/satl/drives.hpp).
inline DirectoryReply drives(const std::string &, bool, const volatile sig_atomic_t *)
{
    DirectoryReply reply;
    for (const filesystems::MountedDrive &drive : filesystems::mounted_drives())
        reply.names.push_back(drive.point);
    return reply;
}

} // namespace directory_words
} // namespace satellite004
