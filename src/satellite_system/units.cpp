// The unit table. See satellite_system/units.hpp for why it is a string and
// why every divisor is a power of two.

#include "satellite_system/units.hpp"

#include <string>

namespace satellite::system {

unsigned long long unit_divisor(std::string_view unit)
{
    // FOLDED HERE RATHER THAN AT EVERY CALL SITE, and lower-cased by hand
    // rather than through <cctype>: std::tolower is locale-dependent and takes
    // an int, and a table of five ASCII words needs neither. The Turkish
    // dotless i is the standard example of a locale making an ASCII fold wrong,
    // and "KB" must mean kilobytes on every machine satellite runs on.
    std::string folded;
    folded.reserve(unit.size());
    for (const char c : unit)
        folded += (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c;

    // 1024 AND NOT 1000. The exactness argument in the header depends on it,
    // and so does agreeing with every other tool on the machine about what a
    // gigabyte of RAM is.
    if (folded == "b")
        return 1ULL;
    if (folded == "kb")
        return 1024ULL;
    if (folded == "mb")
        return 1024ULL * 1024ULL;
    if (folded == "gb")
        return 1024ULL * 1024ULL * 1024ULL;
    if (folded == "tb")
        return 1024ULL * 1024ULL * 1024ULL * 1024ULL;
    return 0;
}

} // namespace satellite::system
