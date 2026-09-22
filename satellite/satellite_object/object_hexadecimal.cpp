// satellite/satellite_object/object_hexadecimal.cpp -- what a hex answers when it
// meets something (object_hexadecimal.hpp says why this is its own file).
//
// NOT BUILT YET: every function refuses by name. Nothing makes a hex until
// bytecode/hexadecimal_values.cpp reads one, so none of these is reached today.

#include "object_hexadecimal.hpp"

namespace satellite004 {
namespace {

signed long long int not_yet(std::string &why)
{
    why = "satellite.variable.hex is numbered and not built yet";
    return not_built_yet;
}

} // namespace

signed long long int hexadecimal_operation(char, const satelliteObject &, const satelliteObject &, satelliteObject &,
                                     std::string &why)
{
    return not_yet(why);
}

signed long long int hexadecimal_compare(const satelliteObject &, const satelliteObject &, int &, std::string &why)
{
    return not_yet(why);
}

bool hexadecimal_same(const satellite_hexadecimal_number &, const satellite_hexadecimal_number &)
{
    return false;
}

signed long long int hexadecimal_to_string(const satellite_hexadecimal_number &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int hexadecimal_to_number(const satellite_hexadecimal_number &, satellite_number &, std::string &why)
{
    return not_yet(why);
}

signed long long int hexadecimal_to_binary(const satellite_hexadecimal_number &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int hexadecimal_to_hexadecimal(const satellite_hexadecimal_number &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

} // namespace satellite004
