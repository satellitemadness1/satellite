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
//
// Type::COUNT is part of that contract even though no module ever names it:
// the dispatch tables are declared [TCOUNT][TCOUNT] and add/sub/mul/div/equals/
// less are inline in container.hpp, so a module compiles the ROW STRIDE into
// itself.  Growing Type therefore invalidates every module already built --
// which is why 2 replaces 1 here, for Type::Capsule.  Refusing is only half
// the answer; see Bridge::build for why the stride is in the cache key too.
constexpr int ABI = 2;

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
// Arguments:  satellite.cxx(greeting = "hello", n = 42) { ... }
//
// Each argument becomes a NAMED, TYPED C++ local inside the block -- `greeting`
// is a std::string and `n` is an int64_t, no unpacking to write.  The type comes
// from the value, through the same from_container overload set that reads them,
// so satellite never has to describe a C++ type and C++ never has to describe a
// satellite one.
//
// Unnamed arguments are positional and get arg0, arg1, ...:
//
//     satellite.cxx("hello", 42) { std::cout << arg0 << arg1; }
//
// The raw `const Container* args` and `int argc` are in scope either way, which
// is the escape hatch for a type with no from_container overload.
// ---------------------------------------------------------------------------
struct CxxArg {
    std::string name;      // the C++ variable the block sees
    Container   value;
};

// Parse a block header's text (what Token::args holds -- no outer parentheses)
// into arguments.  Returns false with *err set on a malformed list.
//
// Version 001 accepts LITERALS only: strings, integers, reals, true/false, nil.
// A bare identifier is refused with a message saying why rather than guessed at,
// because there is no virtual machine yet to evaluate a variable and silently
// passing the name as a string would be a wrong answer instead of an error.
bool parse_args(const std::string& header, std::vector<CxxArg>* out,
                std::string* err);

// The C++ type a satellite value binds to inside a block.
const char* cxx_type_for(Type t);

// ---------------------------------------------------------------------------
// The bridge
// ---------------------------------------------------------------------------

// Compiling and running are timed apart because they are different costs that
// move for different reasons: compile_ms is ~0 on a cache hit and ~1s on a
// miss, while run_ms is native C++ either way.  One combined number would hide
// which of the two a program is actually paying.
struct Timing {
    double compile_ms = 0;   // g++ (0 when the block came from the cache)
    double load_ms    = 0;   // dlopen and symbol lookup
    double run_ms     = 0;   // the block's own code
    bool   cached     = false;
    double total_ms() const { return compile_ms + load_ms + run_ms; }
};

// Everything tunable about compiling a satellite.cxx block.
//
// Running C++ is a large part of what satellite does, so the knobs are real
// ones rather than a token gesture -- optimisation level, target architecture,
// which compiler, whether the cache is used at all.  Every field can be set
// from the environment (see the SATELLITE_CXX_* names below), because version
// 001 has no parser and therefore no way to write settings in satellite itself.
//
// ANY field that changes the generated machine code is part of the cache key.
// It has to be: the cache is content-addressed, so without that, raising
// opt_level from 2 to 3 would serve back the -O2 module that is already on disk
// and the setting would appear to do nothing.
struct CxxConfig {
    // --- where things live --------------------------------------------------
    std::string compiler;      // SATELLITE_CXX_COMPILER   default "g++"
    std::string include_dir;   // where satellite/*.hpp lives
    std::string lib;           // libsatellite_core.so to link against
    std::string cache_dir;     // SATELLITE_CXX_CACHE_DIR

    // --- how a block is compiled --------------------------------------------
    std::string std_version = "c++20";  // SATELLITE_CXX_STD
    int         opt_level   = 2;        // SATELLITE_CXX_OPT        -O0 .. -O3
    bool        native_arch = false;    // SATELLITE_CXX_NATIVE     -march=native
    bool        fast_math   = false;    // SATELLITE_CXX_FAST_MATH  -ffast-math
    std::string extra_flags;            // SATELLITE_CXX_FLAGS      appended verbatim
    std::string link_flags;             // SATELLITE_CXX_LINK_FLAGS -lm, -fopenmp, ...

    // --- behaviour ----------------------------------------------------------
    bool cache_enabled = true;   // SATELLITE_CXX_CACHE=0 recompiles every time
    bool keep_sources  = true;   // SATELLITE_CXX_KEEP_SOURCES=0 deletes the .cpp
    bool verbose       = false;  // SATELLITE_CXX_VERBOSE=1 echoes the command

    // The compile flags these settings add up to, in a stable order -- stable
    // because this string goes into the cache key, and a set that reordered
    // itself between runs would miss the cache every time.
    std::string compile_flags() const;

    // Human-readable dump: every setting, its value, and the variable that
    // overrides it.  Backs `satellite cxx-config`.
    std::string describe() const;
};

// Defaults, with every SATELLITE_CXX_* environment variable applied on top.
CxxConfig default_config();

// The generated translation unit for a block -- exposed so it can be tested
// without invoking a compiler.
//
// The argument NAMES and TYPES appear in this text; their VALUES do not, since
// those arrive at call time.  That is what makes the content hash the right
// cache key: changing `n = 42` to `n = 43` reuses the module, while changing it
// to `n = "forty-two"` recompiles, because the declared type moved.
std::string generate(const std::string& block_source,
                     const std::vector<CxxArg>& args);
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
    //
    // `out_printed` collects whatever the block wrote to stdout.  A GUI has no
    // terminal, so a block whose only effect is std::cout would look broken
    // rather than unsupported; passing null leaves stdout alone, which is what
    // a command-line caller wants.
    Container run(const std::string& block_source,
                  const std::vector<CxxArg>& args,
                  std::string* err = nullptr,
                  std::string* out_printed = nullptr,
                  Timing* timing = nullptr);

    // No-argument blocks, which is every block written before headers existed.
    Container run(const std::string& block_source, std::string* err = nullptr);

    // Compile only.  `so_path` receives the cached library path.
    bool build(const std::string& block_source, const std::vector<CxxArg>& args,
               std::string* so_path, std::string* err = nullptr);
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
