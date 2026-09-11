#pragma once

// The value `satellite.main`'s parameter holds -- DESIGN §7.7, PLAN M20.
//
// THE COMMAND LINE, AND THE NAME OF EVERY WORD ON IT. A program reaches the
// words it was started with by number -- `arguments[1]`, `arguments.length()`
// -- and everything else the runtime can truthfully say about the machine it
// woke up on by NAME. This body holds the first half. The second half is
// `system_facts/arguments_facts.hpp`, and the two are separate for a reason
// this file is the wrong half of: the command line is a fact about THIS RUN
// and costs nothing to gather, while the machine's answers cost a dozen
// syscalls and a file parse, and PLAN M20 asks that a program which never
// looks at one pays for none of them.
//
// SO THE MACHINE'S FACTS ARE NOT IN HERE, AND THAT IS THE LAZY HALF. v1's
// `Arguments` carried all thirty-three, assembled by `arguments_for()` before
// `satellite.main` ran; this one carries the command line and the facts are
// built on first touch, process-wide, behind a function-local static. What
// that buys is measured in MILESTONES/M20.md rather than asserted here.
//
// ONE FACT CAME BACK ANYWAY, AND IT IS THE ONE THE MEASUREMENT WOULD HAVE
// HIDDEN. `session.directory` is `getcwd()`, and `satellite.directory.change`
// `1 18 1` has been built since M19 -- so a program that changes directory
// and THEN reads `arguments.session.directory` would get two different answers
// from an eager build and a lazy one. The arguments object is what the program
// was STARTED with (§7.7: "the machine it woke up on"), so the starting
// directory is captured here, at startup, with the command line. Every other
// fact under `arguments` is fixed for the life of the process -- uname,
// /etc/os-release, the build macros, the pid, the page size, the environment
// satellite has no way to write -- which is what makes the other thirty-two
// safe to defer.
//
// BUILT, THEN FROZEN, which is `MapBody`'s rule and is kept for `MapBody`'s
// reason: an Arguments is fully populated before `make_shared` and never
// written afterwards, so a value two slots see can never change underneath
// either of them (DESIGN §6.4).

#include "satellite_value/value.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace satellite {

// One word of the command line, with the name it answers to.
//
// THE NAMES ARE `program` AND `argument_1`..., WHICH IS v1's ANSWER KEPT.
// argv[0] is the script, so it gets the name a shell would call it by; the
// rest are numbered from 1 so that `argument_1` IS `arguments[1]` and a reader
// never has to work out whether a name is off by one from an index.
struct CommandLineWord {
    std::string name;
    Value value;  // always a string
};

struct Arguments {
    // In argv order, argv[0] first. THIS IS WHAT `.length()` COUNTS AND WHAT
    // `[i]` IS BOUNDED BY, and the two say so together: every program that
    // takes arguments writes `for (i = 1; i < args.length(); i = i + 1)`, and
    // a length that counted the machine's facts as well would walk that loop
    // off the user's words and start reading the kernel release as though it
    // had been typed.
    std::vector<CommandLineWord> words;

    // `getcwd()` at startup -- see the header note. Empty when the directory
    // could not be read, which `add()`'s missing-word rule turns into the
    // fallback text at the one place that renders it.
    std::string directory;
};

// `Arg` -- the handle the variant holds -- IS DECLARED IN value.hpp AND NOT
// HERE, which is `List` and `MapBody`'s arrangement exactly and for their
// reason: a `CommandLineWord` holds a `Value` by value, so this struct cannot
// be defined until `Value` is complete, and the variant cannot be spelled
// until the handle has a name. value.hpp forward-declares the struct and names
// the handle; this file is where the body finally is.

} // namespace satellite
