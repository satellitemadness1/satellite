#pragma once
// satellite/satellite_variable_color/satellite_color.hpp -- `satellite.variable.color`
// `1 6 19`, second spelling `satellite.variable.colour` (words/aliases.tsv). Arm 16
// of satelliteObject.
//
// (the author, 2026-09-22) "satellite.variable.color my_color = 000000 which is
// just a hexadecimal number of a mandatory width, 6 digits"; earlier the same day:
// "satellite.variable.color my_color = x000000 or just 000000 without the x ...
// and then we'll add transparency in two ways, my_color.transparency(0 - 99) and
// my_color = x000000, 0-99 for transparency".
//
// A MACHINE INTEGER, AND THAT IS NOT A LIMIT. Six hex digits are 24 bits, always:
// the width is the type, the author's "mandatory width". Nothing a person can
// write makes a colour wider, so a satellite_number would only be slower.

namespace satellite004 {

struct satellite_color {
    unsigned int rgb = 0;                // the six digits: 0xRRGGBB
    unsigned int transparency = 0;       // 0 to 99 (the author's range)
};

} // namespace satellite004
