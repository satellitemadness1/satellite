// satellite/satellite_object/object_color.cpp -- what a color answers when it
// meets something (object_color.hpp says why this is its own file).
//
// NOT BUILT YET: every function refuses by name. Nothing makes a color until
// bytecode/color_values.cpp reads one, so none of these is reached today.

#include "object_color.hpp"

namespace satellite004 {
namespace {

signed long long int not_yet(std::string &why)
{
    why = "satellite.variable.color is numbered and not built yet";
    return not_built_yet;
}

} // namespace

signed long long int color_operation(char, const satelliteObject &, const satelliteObject &, satelliteObject &,
                                     std::string &why)
{
    return not_yet(why);
}

signed long long int color_compare(const satelliteObject &, const satelliteObject &, int &, std::string &why)
{
    return not_yet(why);
}

bool color_same(const satellite_color &, const satellite_color &)
{
    return false;
}

signed long long int color_to_string(const satellite_color &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int color_to_number(const satellite_color &, satellite_number &, std::string &why)
{
    return not_yet(why);
}

signed long long int color_to_binary(const satellite_color &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

signed long long int color_to_hexadecimal(const satellite_color &, satellite_string &, std::string &why)
{
    return not_yet(why);
}

} // namespace satellite004
