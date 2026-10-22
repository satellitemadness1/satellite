#pragma once
// satellite/satellite_variable_file/file_moves.hpp -- what satellite.file.delete DOES (FILE_AND_DRIVE_PLAN.md
// FD-2, the author's of 2026-10-21), shared by the word in a program (bytecode/file_calls.cpp) and the line
// typed alone at the prompt (satl/session.cpp), which prints `why` when it was not done.
//
// IT GOES THROUGH, OR IT ASKS (the author: "we must make the command always go through, or ask for the
// password to the account that owns it ... if a user is going to go through the trouble of typing in
// satellite.file.delete("filename.ext") then obviously they are sure"). So there is no "are you sure": a file
// the person may delete is deleted, a write-protected one too. One they may not is deleted as the account that
// may, with that account's password, asked on the console (rule 3):
//   - the folder's owner, who may take any name out of their folder -- or, in a sticky folder such as /tmp,
//     the file's own owner;
//   - with the person's own password, through sudo, when that account is the person's own (a folder of theirs
//     they made read-only) or one nobody logs into (no shell to run a command in).
// A folder is not deleted (delete takes files), and a link is deleted itself, never what it points to.
//
// A VALUE, NEVER A STOP: the word answers true or false, as every file word does (SATELLITE_FILE_OPERATIONS.md).

#include <string>
#include <sys/stat.h>
#include <sys/types.h>

namespace satellite004 {

struct DiskAnswer {
    bool done = false;
    std::string why;   // why it was not done, in words; empty when it was
};

// `su` and `sudo` are the programs it runs; only the cases give it others, stand-ins of their own.
DiskAnswer delete_a_file(const std::string &path, const std::string &su = "/usr/bin/su",
                         const std::string &sudo = "/usr/bin/sudo");

// WHO MAY DELETE A FILE THE PERSON MAY NOT, from the file's and its folder's stat alone: the folder's owner, or in
// a sticky folder the file's -- through su with that account's password, or through sudo with the person's own
// when the account is the person's or one with no shell to run a command in (or none at all).
struct DeleteAsker {
    uid_t owner = 0;
    std::string account;      // its name, or its number when it has none
    bool through_su = false;
};
DeleteAsker who_may_delete(const struct stat &file, const struct stat &folder, uid_t me);

} // namespace satellite004
