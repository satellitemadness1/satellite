#pragma once

// The opening information, the usage text, and what satl exits with.
//
// One file, because these are one subject: how satl answers the command line
// before any satellite source has been read. Nothing here knows what a
// satellite program is.

#include <string>

namespace satellite {

// What satl exits with, in one place so that two arms cannot disagree about
// what a given failure is worth.
//
// A REQUEST THAT WAS CORRECTLY MADE IS NOT AN ERROR. --version and --help both
// exit 0, and that is a deliberate change from the first satellite, where
// --help exited 2: a packaging script that greps --version is entitled to a
// zero, and so is a user who asked a program to explain itself. An exit status
// says whether what was asked for happened, not whether the program did any
// work.
//
// EXIT_MALFORMED IS M5's, AND THREE ARMS HAD BEEN WAITING FOR IT. MILESTONES
// /M3.md §6 item 2 opened it, M4.md §6 item 3 added the second arm and M4.5.md
// §6 item 5 the third: `satl --tokens`, `--unparse` and `--satc` all exited
// EXIT_USAGE on a program that would not parse, and EXIT_USAGE's own definition
// one line below is "the command line did not name something satl can do",
// which a bad program is not. Each of the three declined to invent a code,
// because this enum exists so that "two arms cannot disagree about what a given
// failure is worth" and inventing one in an arm is exactly that disagreement
// happening.
//
// IT IS 1 AND NOT 4, WHICH IS THE ONE PLACE THIS ENUM DEFERS TO CONVENTION
// RATHER THAN TO ITS OWN ORDER. 1 is what every compiler in the world returns
// for a source file it could not compile, and a `satl --check` in a Makefile or
// a CI script is read by tools that already know that. The numbers here are
// assigned by what a failure IS and not by the order they were invented in --
// which is why 1 sits above 2 in this list and below it in the history.
enum ExitStatus {
    EXIT_FINE = 0,       // what was asked for, happened
    EXIT_MALFORMED = 1,  // the file is not a satellite program
    EXIT_USAGE = 2,      // the command line did not name something satl can do
    EXIT_NOT_YET = 3,    // a correct request this milestone cannot serve yet
};

// The banner satl shows when it is started with nothing to do.
//
// RETURNS A STRING RATHER THAN PRINTING, because it has a second caller coming.
// At M11.B this same text becomes the REPL's banner, and the first satellite's
// mistake was to write that banner as a separate literal -- which then drifted,
// and said 0.1 for months while --version said 002. One function, two callers,
// no way to drift.
std::string opening_text();

// Every way to start satl, one line each.
//
// Lines that describe a milestone that has not landed are MARKED, rather than
// omitted. Omitting them makes satl look finished and leaves a user guessing
// what it will eventually do; marking them says what is coming and what is not
// here, which is the same honesty the file arm below is built on.
std::string usage_text();

} // namespace satellite
