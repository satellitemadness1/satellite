#pragma once
// satellite/satellite_test/speed_objects_twins.hpp -- the C++ twins of satellite.test.speed()'s objects,
// capsules, containers and strings sections (group speed_objects). speed_<name>(passes) does the same work
// as speed_<name>.satl run with that many passes, and returns the one line that program displays, without
// its newline. Any passes of 0 or more gives the satl program's line (proved at 0, 1, 2, 3, 7 and each
// default); none throws.

#include <string>

namespace satellite004 {

// the author's own speed test (official_test/speed_test/official_speed_test.satl), one loop each
std::string speed_his_new_object(long long passes);
std::string speed_his_set_number_list(long long passes);
std::string speed_his_append_number(long long passes);
std::string speed_his_set_string(long long passes);
std::string speed_his_set_string_list(long long passes);
std::string speed_his_append_string(long long passes);

// capsules, objects, containers and strings
std::string speed_capsule_calls(long long passes);
std::string speed_object_fields(long long passes);
std::string speed_list_ops(long long passes);
std::string speed_map_ops(long long passes);
std::string speed_string_build(long long passes);
std::string speed_string_methods(long long passes);
std::string speed_string_list(long long passes);
std::string speed_nested_containers(long long passes);

} // namespace satellite004
