// satellite.cxx { ... } -- inline C++ inside satellite source.
//
//     satellite.cxx
//     {
//         #include <string>
//         std::string greeting = "hello"
//         greeting += " from C++"
//         satellite.return(greeting)
//     }
//
// The block is never lexed as satellite -- the lexer counts braces and hands
// the raw text through.  This bridge wraps it in a function, compiles it to a
// shared library, caches it by content hash, and calls it.
//
// Conversion is the interesting part, and C++ does it for us: `satellite.return`
// becomes a call to to_container(), and OVERLOAD RESOLUTION picks the right
// conversion for whatever type the expression has.  Satellite never has to
// understand C++ types; a type with no overload is a normal compile error
// naming that type.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "satellite/container.hpp"

namespace satellite {
namespace cxx {

// Bumped whenever the Container layout or the module contract changes.  A
// module built against an older ABI is refused rather than loaded, because
// loading it would corrupt memory silently.
constexpr int ABI = 1;

// ---------------------------------------------------------------------------
// C++ value -> satellite value.  Overload resolution does the type matching.
// ---------------------------------------------------------------------------
inline Container to_container(const Container& c) { return c; }
inline Container to_container(bool v)             { return Container::boolean(v); }
inline Container to_container(int v)              { return Container::integer(v); }
inline Container to_container(long v)             { return Container::integer((int64_t)v); }
inline Container to_container(long long v)        { return Container::integer((int64_t)v); }
inline Container to_container(unsigned v)         { return Container::integer((int64_t)v); }
inline Container to_container(unsigned long v)    { return Container::integer((int64_t)v); }
inline Container to_container(unsigned long long v){ return Container::integer((int64_t)v); }
inline Container to_container(float v)            { return Container::real((double)v); }
inline Container to_container(double v)           { return Container::real(v); }
inline Container to_container(char v)             { return Container::str(std::string(1, v)); }
inline Container to_container(const char* v)      { return Container::str(v ? v : ""); }
inline Container to_container(const std::string& v) { return Container::str(v); }

// any vector becomes a satellite list, element by element
template <typename T>
inline Container to_container(const std::vector<T>& v) {
    Container out = Container::list();
    auto& items = out.as_list()->items;
    items.reserve(v.size());
    for (const auto& x : v) items.push_back(to_container(x));
    return out;
}

// ---------------------------------------------------------------------------
// satellite value -> C++ value, for reading arguments
// ---------------------------------------------------------------------------
inline void from_container(const Container& c, int64_t* out) {
    *out = c.type == Type::Int  ? c.i
         : c.type == Type::Real ? (int64_t)c.d
         : c.type == Type::Bool ? (c.b ? 1 : 0)
         : 0;
}
inline void from_container(const Container& c, double* out) {
    *out = c.type == Type::Real ? c.d
         : c.type == Type::Int  ? (double)c.i
         : 0.0;
}
inline void from_container(const Container& c, bool* out)        { *out = c.truthy(); }
inline void from_container(const Container& c, std::string* out) { *out = c.to_string(); }

// ---------------------------------------------------------------------------
// The bridge
// ---------------------------------------------------------------------------
struct CxxConfig {
    std::string compiler;      // default "g++"
    std::string include_dir;   // where satellite/*.hpp lives
    std::string lib;           // libsatellite_core.so to link against
    std::string cache_dir;     // compiled blocks land here, keyed by hash
    std::string flags;         // default "-O2 -std=c++20"
};

CxxConfig default_config();

// The generated translation unit for a block -- exposed so it can be tested
// without invoking a compiler.
std::string generate(const std::string& block_source);

// Where a block's `satellite.return(...)` expression is, if it has one.
// Returns an empty string when the block returns nothing.
std::string return_expression(const std::string& block_source);

class Bridge {
public:
    explicit Bridge(CxxConfig cfg = default_config());
    ~Bridge();
    Bridge(const Bridge&) = delete;
    Bridge& operator=(const Bridge&) = delete;

    // Compile if needed, load, call.  Returns nil and fills `err` on failure.
    Container run(const std::string& block_source, std::string* err = nullptr);

    // Compile only.  `so_path` receives the cached library path.
    bool build(const std::string& block_source, std::string* so_path,
               std::string* err = nullptr);

    size_t compiles() const { return compiles_; }   // blocks actually compiled
    size_t cache_hits() const { return hits_; }     // blocks reused from cache

private:
    CxxConfig cfg_;
    size_t compiles_ = 0;
    size_t hits_     = 0;
    std::vector<void*> handles_;                    // kept open for the process
};

}  // namespace cxx
}  // namespace satellite
