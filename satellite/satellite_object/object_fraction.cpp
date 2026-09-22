// satellite/satellite_object/object_fraction.cpp -- what a fraction answers when it
// meets something (object_fraction.hpp says why this is its own file).
//
// NOT BUILT YET: every function refuses by name. Nothing makes a fraction until
// bytecode/fraction_values.cpp reads one, so none of these is reached today.

#include "object_fraction.hpp"

namespace satellite004 {
namespace {

signed long long int not_yet(std::string &why)
{
    why = "satellite.variable.fraction is numbered and not built yet";
    return not_built_yet;
}

} // namespace

signed long long int fraction_operation(char, const satelliteObject &, const satelliteObject &, satelliteObject &,
                                     std::string &why)
{
    return not_yet(why);
}

signed long long int fraction_compare(const satelliteObject &, const satelliteObject &, int &, std::string &why)
{
    return not_yet(why);
}

bool fraction_same(const satellite_fraction &, const satellite_fraction &)
{
    return false;
}

signed long long int fraction_to_string(const satellite_fraction &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int fraction_to_number(const satellite_fraction &, satellite_number &, std::string &why)
{
    return not_yet(why);
}

signed long long int fraction_to_binary(const satellite_fraction &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int fraction_to_hexadecimal(const satellite_fraction &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

} // namespace satellite004
