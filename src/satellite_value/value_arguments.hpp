#pragma once

#include "satellite_value/value_types.hpp"

namespace satellite {

// The value satellite.main is handed: the command line, and then everything
// the runtime can truthfully say about the machine it is running on.
//
// A LIST OF STRINGS THAT KNOWS WHAT EACH STRING IS. matches() accepts one
// wherever satellite.container.list<satellite.variable.string> is declared, so
// §2's signature does not move and hello world keeps its five lines; what it
// adds is that every element has a NAME, and that names past the command line
// carry facts (argv, cwd, the user, the OS, the compiler that built satl)
// which a program would otherwise have no way to ask for at all.
//
// .length() AND NUMERIC [i] COVER THE COMMAND LINE AND NOTHING ELSE. That is
// the whole reason `command_line_count` is stored rather than derived. Every
// program that takes arguments writes `for (i = 1; i < args.length(); i++)`,
// and if .length() counted the environment entries that loop would walk off
// the user's arguments and start reading the kernel release as though it had
// been typed. The extra entries are reached by NAME -- args.cxx_compiler,
// args["cxx_compiler"], args.get("cxx_compiler") -- and .count() is the total
// for anyone who wants it.
//
// BUILT, THEN FROZEN, exactly as MapBody is and for the same reason: an
// Arguments is fully populated before make_shared and never written afterwards,
// which is what keeps the immutability contract and the lock-free publish
// protocol intact. The index is built eagerly for the same reason MapBody's is
// -- a lazily built one is a data race nothing in the test suite would catch.
struct ArgumentEntry {
    // ASCII, lower case, underscores. Chosen to be disjoint from every method
    // name a list or an Arguments answers to, because a method wins over an
    // entry and a colliding name would be unreachable rather than ambiguous.
    std::string name;

    // Always a string Value. An Arguments is a list<string> to the type system
    // and this is what makes that true rather than nearly true.
    ValuePtr value;
};

struct Arguments {
    // In display order: the command line first, in argv order, then the
    // environment entries in the order §8's table lists them.
    std::vector<ArgumentEntry> entries;

    // name -> position in `entries`. Eager, see above.
    std::unordered_map<std::string, size_t> index;

    // How many leading entries came from (argc, argv), argv[0] included. This
    // is what .length() answers and what [i] is bounded by.
    size_t command_line_count = 0;
};
using ArgsRef = std::shared_ptr<const Arguments>;

} // namespace satellite
