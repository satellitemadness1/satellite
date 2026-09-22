// satellite/satellite_object/object_float.cpp -- what a float answers when it
// meets something (object_float.hpp says why this is its own file).
//
// NOT BUILT YET: every function refuses by name. Nothing makes a float until
// bytecode/float_values.cpp reads one, so none of these is reached today.

#include "object_float.hpp"

namespace satellite004 {
namespace {

signed long long int not_yet(std::string &why)
{
    why = "satellite.variable.float is numbered and not built yet";
    return not_built_yet;
}

} // namespace

signed long long int float_operation(char, const satelliteObject &, const satelliteObject &, satelliteObject &,
                                     std::string &why)
{
    return not_yet(why);
}

signed long long int float_compare(const satelliteObject &, const satelliteObject &, int &, std::string &why)
{
    return not_yet(why);
}

bool float_same(const satellite_float &, const satellite_float &)
{
    return false;
}

signed long long int float_to_string(const satellite_float &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int float_to_number(const satellite_float &, satellite_number &, std::string &why)
{
    return not_yet(why);
}

signed long long int float_to_binary(const satellite_float &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int float_to_hexadecimal(const satellite_float &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

} // namespace satellite004
