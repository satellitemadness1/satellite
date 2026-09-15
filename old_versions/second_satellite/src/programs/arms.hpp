#pragma once

// The arms, and what each of them installs -- ERROR_HANDLING.md §4.3.
//
// WHAT AN ARM IS. satl runs a program from more than one entry point, and they
// are not the same program: `--run` has a command line and a printer, the
// prompt has a printer and a command line per `run <file>`, `--call` has only
// the handlers. Each one
// builds the handler table it needs.
//
// WHY THIS FILE EXISTS, AND IT IS A BUG RATHER THAN A TIDINESS ARGUMENT.
// Until 2026-09-12 each arm kept its own hand-written list of
// `x::install_handlers()` calls. M23 added threads to run_command.cpp and to
// neither of the others, so for three weeks `satellite.thread.new` worked in a
// file and answered S0721 at the prompt -- "a path satellite has a number for
// and nothing behind yet, a later milestone" -- about a milestone that had
// landed. The message was truthful about the table and false about the
// language, and it sent its reader to the milestones to look for finished work.
//
// SO THE LISTS ARE ONE LIST AND THE DIFFERENCES ARE DATA. An arm no longer says
// which groups it installs; it says which ARM it is, and the differences that
// are on purpose are rows in `kExceptions` with the reason attached. Anything
// not written there is installed everywhere, and tests/arms_test fails the day
// that stops being true.
//
//     PROSE MAY EXPLAIN A DIFFERENCE. IT MAY NEVER BE THE ONLY PLACE THE
//     DIFFERENCE LIVES.
//
// which is WORD_NUMBERS.md's rule about numbers, pointed at this instead. The
// reasons below were prose in three files and are now the thing the build
// checks.

#include <cstddef>
#include <string>

namespace satellite::arms {

// The entry points that build a handler table. `Check` is deliberately absent:
// `--check` resolves and never evaluates, so it installs nothing and has no
// row to get wrong.
enum class Arm : unsigned char {
    Run,      // satl file.satl, satl --run file.satl
    Prompt,   // satl --repl
    Call,     // satl --call capsule
    Count
};

// One installable group. The order is this enum's own and means nothing to the
// language -- unlike words.def, where a number IS a position, these are not
// written down anywhere a program can see.
enum class Group : unsigned char {
    Console,
    Scalars,
    Containers,
    Random,
    Time,
    Thread,
    System,
    Help,
    File,
    Directory,
    Arguments,
    Count
};

// A difference that is ON PURPOSE, and the reason it is.
//
// THE REASON IS A FIELD AND NOT A COMMENT, because `satl --arms` prints it: a
// person asking why the prompt has no `arguments` should get the answer from
// the program rather than from a file they have to know to open.
struct Exception {
    Arm arm;
    Group group;
    const char *why;
};

const Exception *exceptions(std::size_t *count);

// True when this arm is DECLARED not to install this group; `why` is filled in.
bool declared_absent(Arm arm, Group group, const char **why);

const char *arm_name(Arm arm);
const char *group_name(Group group);

// Install exactly the groups this arm should have. The one place any arm calls
// an install_handlers().
void install_for(Arm arm);

// The matrix a person reads -- `satl --arms`.
std::string report();

// Every arm installs every group except the declared exceptions. Answers false
// and fills `complaints` when it does not. tests/arms_test is the caller that
// makes this a build guarantee rather than a command nobody runs.
bool audit(std::string *complaints);

} // namespace satellite::arms
