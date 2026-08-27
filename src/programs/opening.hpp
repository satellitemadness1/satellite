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
enum ExitStatus {
    EXIT_FINE = 0,       // what was asked for, happened
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
