// Shared internals of the satellite.cxx bridge.
//
// src/cxx.cpp used to be one file holding the whole bridge.  Running C++ is a
// large enough part of what satellite does that it now has its own directory:
//
//     cxx_internal.hpp   this file -- helpers shared by the four below
//     config.cpp         CxxConfig, the SATELLITE_CXX_* settings, describe()
//     generate.cpp       block text -> a compilable translation unit
//     bridge.cpp         compile, cache, dlopen, call
//     args.cpp           the satellite.cxx( ... ) header
//
// In src/ rather than include/, for the usual reason: everything under
// include/ is preprocessed by every satellite.cxx block at runtime.
#pragma once

#include "satellite/cxx.hpp"

#include <cstdio>
#include <string>

#include <sys/stat.h>
#include <sys/types.h>

namespace satellite {
namespace cxx {
namespace detail {

inline std::string trim(const std::string& s) {
    size_t b = s.find_first_not_of(" \t\r\n");
    if (b == std::string::npos) return "";
    size_t e = s.find_last_not_of(" \t\r\n");
    return s.substr(b, e - b + 1);
}

inline bool file_exists(const std::string& p) {
    struct stat st;
    return ::stat(p.c_str(), &st) == 0;
}

inline void make_dirs(const std::string& path) {
    std::string acc;
    for (size_t k = 0; k < path.size(); ++k) {
        acc += path[k];
        if (path[k] == '/' && acc.size() > 1) ::mkdir(acc.c_str(), 0700);
    }
    ::mkdir(path.c_str(), 0700);
}

// FNV-1a.  Not a cryptographic hash and does not need to be: this names a
// cache entry, and the only thing at stake in a collision is a wrong module
// for a block the user wrote themselves on their own machine.
inline std::string hash_hex(const std::string& s) {
    uint64_t h = 1469598103934665603ull;
    for (unsigned char c : s) { h ^= c; h *= 1099511628211ull; }
    char buf[17];
    std::snprintf(buf, sizeof buf, "%016llx", (unsigned long long)h);
    return buf;
}

}  // namespace detail
}  // namespace cxx
}  // namespace satellite
