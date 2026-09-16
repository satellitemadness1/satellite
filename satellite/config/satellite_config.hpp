#pragma once
// satellite_config.hpp -- the author's configuration. The interpreter compiles this
// file and calls return_arguments_vector() once at start-up
// (satellite/arguments/arguments.cpp), so it needs <string> and <vector>.
//
// make raises arguments.build by one on every build (build_number.py, beside
// this file). Write every number without a leading 0: in C++ 0051 is octal, 41.

#include <string>
#include <vector>

// One row: the four pieces described inside return_arguments_vector(). A
// std::map holds a key and one value, so a row of four is a struct.
struct satellite_argument_row {
    std::string name;             // 1)
    signed long long int number;  // 2)
    bool flag;                    // 3)
    bool is_flag;                 // 4)
};

inline std::vector<satellite_argument_row> return_arguments_vector()
{
    std::vector<satellite_argument_row> arguments_vector;

    // arguments_vector is divided into 4 pieces:

    // 1) the name inside of the satellite interpreter of the property,
    // 2) the value of that argument
    // 3) if 4 is set to true, then the 3rd item is used instead of the number...
    // 4) whether it is a bool(true) or a number(false)

    arguments_vector.push_back({"arguments.satc", 0, true, true});
    arguments_vector.push_back({"arguments.satb", 0, true, true});
    arguments_vector.push_back({"arguments.sati", 0, true, true});
    arguments_vector.push_back({"arguments.startup_display", 0, true, true});
    arguments_vector.push_back({"arguments.version", 4, false, false});
    arguments_vector.push_back({"arguments.revision", 4, false, false});
    arguments_vector.push_back({"arguments.build", 61, false, false});
    arguments_vector.push_back({"arguments.object_bytes_max", 34359738368, false, false});
    arguments_vector.push_back({"arguments.threads_max", 1000000, false, false});
    arguments_vector.push_back({"arguments.threads_startup", 256, false, false});
    arguments_vector.push_back({"arguments.file_size_max_bytes", 549755813888, false, false}); // 512 gigabyte file_size maximum

    return(arguments_vector);
}