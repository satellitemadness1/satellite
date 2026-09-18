#pragma once
// satellite_config.hpp -- the author's configuration. The interpreter compiles this
// file and calls return_arguments_vector() once at start-up
// (satellite/arguments/arguments.cpp), so it needs <string> and <vector>.
//
// make raises arguments.build by one on every build (build_number.py, beside
// this file). Write every number without a leading 0: in C++ 0051 is octal, 41.

#include <concepts>
#include <string>
#include <vector>

// A ROW'S NUMBER IS A satellite_number (the author, 2026-09-16): "just make
// everything a satellite number, satellite numbers can hold any amount of
// digits". Write it as always -- 94, 1024, 34359738368 -- or, when it is longer
// than a C++ integer can hold, IN QUOTES: "99999999999999999999999999999". Either
// way it is kept here as its decimal digits and read into a satellite_number at
// start-up (satellite/arguments/arguments.cpp), so no digit is ever lost to a C++
// integer type on the way.
struct satellite_argument_number {
    std::string digits;
    template <std::integral Integer>
    satellite_argument_number(Integer value) : digits(std::to_string(value)) {}
    satellite_argument_number(const char *text) : digits(text) {}
};

// One row: the four pieces described inside return_arguments_vector(). A
// std::map holds a key and one value, so a row of four is a struct.
struct satellite_argument_row {
    std::string name;                  // 1)
    satellite_argument_number number;  // 2)
    bool flag;                         // 3)
    bool is_flag;                      // 4)
};

inline std::vector<satellite_argument_row> return_arguments_vector()
{
    std::vector<satellite_argument_row> arguments_vector;

    // arguments_vector is divided into 4 pieces:

    // 1) the name inside of the satellite interpreter of the property,
    // 2) the value of that argument
    // 3) if 4 is set to true, then the 3rd item is used instead of the number...
    // 4) whether it is a bool(true) or a number(false)

    // THE THREE FILES BECAME ONE (the author, 2026-09-16): "We skipped .satc and
    // .satb in place of something else, we are just saving the 16-bit bytecode as
    // .sate and skipping everything... there is only argument.sate=true/false".
    //
    // .satc held the program as numbers, .satb held combine's batch marks and
    // .sati held the strings as bits. The 16-bit bytecode is already all three:
    // it IS the numbered program (one code a word), its strings are codes inline
    // and counted, and the registry already reserves batch_start/batch_end/wait/
    // batch_size for the marks. So there is one file and one flag.
    arguments_vector.push_back({"arguments.sate", 0, true, true});

    // THE STEP (the author, 2026-09-16): "the magic is step, so it just jumps at
    // that step, preparing every 5 lines by default". A converting thread jumps
    // to its offset and builds that many lines, then jumps again.
    //
    // FIVE AND NOT ONE, and the reason is measured rather than chosen: one thread
    // per LINE is 5.6x SLOWER than one thread doing all of it (227 ms against 40
    // ms for 100,000 lines), because a line costs ~402 ns to tokenise and handing
    // it to a thread costs ~12,486 ns. A step is what makes the handoff worth
    // making. Raising it trades latency for throughput; 0 would mean no lookahead.
    arguments_vector.push_back({"arguments.magic", 5, false, false});
    arguments_vector.push_back({"arguments.startup_display", 0, true, true});
    arguments_vector.push_back({"arguments.version", 4, false, false});
    arguments_vector.push_back({"arguments.revision", 4, false, false});
    arguments_vector.push_back({"arguments.build", 115, false, false});
    arguments_vector.push_back({"arguments.object_bytes_max", 34359738368, false, false});
    arguments_vector.push_back({"arguments.threads_max", 1000000, false, false});
    arguments_vector.push_back({"arguments.threads_startup", 1024, false, false});
    arguments_vector.push_back({"arguments.file_size_max_bytes", 549755813888, false, false}); // 512 gigabyte file_size maximum

    // INFINITY'S DIGITS (the author, 2026-09-16): an infinity carries "a single
    // satellite float for going up or down" -- infinityx2, infinityx0.5 -- held to
    // arguments.infinity digits and "displayed as a rounded thing... we round to
    // 32 digits", "both digits configurable". Read by nothing yet: MILESTONES M11.
    arguments_vector.push_back({"arguments.infinity", 4096, false, false});
    arguments_vector.push_back({"arguments.infinity_display", 32, false, false});

    return(arguments_vector);
}