#pragma once
// satellite/satellite_test/speed_twins.hpp -- satellite.test.speed()'s C++: for each speed program,
// the same work as a C++ function that answers the line the program prints (test_programs.hpp's
// `twin`). Compiled into satl with -O2 like the rest of it, and timed in the same run.
//
// Each is `std::string speed_<name>(long long passes)`, named for its program,
// programs/speed/speed_<name>.satl, and given the passes that program was run with. Written in two
// groups, 2026-10-06, each proved against its programs line for line at many passes:

#include "speed_objects_twins.hpp"   // the author's own six loops, capsules, objects, containers, strings
#include "speed_values_twins.hpp"    // loops, statements, numbers of every size, the other value types
