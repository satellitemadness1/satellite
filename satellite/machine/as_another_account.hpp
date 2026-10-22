#pragma once
// satellite/machine/as_another_account.hpp -- a command run as another account, with a password the person typed
// (FILE_AND_DRIVE_PLAN.md rules 3, 4 and 9, the author's of 2026-10-21).
//
// TWO WAYS, ONE FOR EACH KIND OF PASSWORD:
//   as_owner    su <account>, with THAT ACCOUNT's password -- satellite.file.delete on a file that is not the
//               person's to delete ("ask for the password to the account that owns it");
//   with_sudo   sudo, with the PERSON'S OWN password -- mounting a drive the system will not let them mount
//               ("My own password, through sudo"), and a delete no account's own password could do.
//
// NEVER A SHELL OF satl'S MAKING: every word reaches the program as one argument, so a file named `; rm -rf ~`
// is a name and nothing more. su runs a command through the account's own shell, so that command is the fixed
// text `exec "$0" "$@"` and the words follow it as arguments. Every program is named by its full path.
//
// su ON A TERMINAL OF ITS OWN: su reads a password from a terminal, so it is given a new pty as its controlling
// terminal -- never satl's, which may be the GTK console's -- and the password is typed into it only once su has
// turned ECHO off, which is also the moment its TCSAFLUSH has passed (a password typed before that is thrown
// away, and su would wait forever). sudo reads one from a pipe (-S), and -k keeps it from remembering the
// password afterwards: a program satl starts later must not inherit a sudo it was never asked for.
//
// THE PASSWORD IS WRITTEN ONCE AND NOT KEPT: the copies made to write it are wiped (explicit_bzero); the
// caller wipes its own.
//
// Both run with LC_ALL=C, so their words about a wrong password are the same on every machine.

#include <string>
#include <vector>

namespace satellite004 {

struct AccountRun {
    bool done = false;             // the command ran and answered 0
    bool wrong_password = false;   // su or sudo refused the password
    bool not_allowed = false;      // sudo: the account may not use sudo at all
    std::string why;               // what went wrong, in the programs' own words, on one line
};

// `words` run as `account` through su (or `su`, a stand-in for the cases), `password` typed when it asks.
AccountRun as_owner(const std::string &account, const std::string &password, const std::vector<std::string> &words,
                    const std::string &su = "/usr/bin/su");

// `words` run as root through sudo (or `sudo`, a stand-in for the cases), with the person's own password.
AccountRun with_sudo(const std::string &password, const std::vector<std::string> &words,
                     const std::string &sudo = "/usr/bin/sudo");

} // namespace satellite004
