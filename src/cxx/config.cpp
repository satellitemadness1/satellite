// CxxConfig -- the tunable settings for compiling satellite.cxx blocks.
//
// Version 001 has no parser, so there is no way to write settings in satellite
// itself yet.  The environment is the channel that works today and keeps
// working afterwards: SATELLITE_CXX_OPT=3 in a shell profile, or in front of a
// single run, needs nothing from the language.
//
// When the parser lands these become satellite.cxx.settings.* and this file
// becomes the place that reads them; nothing above it has to change, because
// everything already goes through CxxConfig.
#include "cxx_internal.hpp"

#include <cctype>
#include <cstdlib>
#include <sstream>

// Set by CMake to the source tree, or to the bundle's own directories in a
// redistributable build.  Empty defaults keep this file compiling on its own.
#ifndef SATELLITE_INCLUDE_DIR
#define SATELLITE_INCLUDE_DIR ""
#endif
#ifndef SATELLITE_CORE_LIB
#define SATELLITE_CORE_LIB ""
#endif

namespace satellite {
namespace cxx {

namespace {

const char* env(const char* name) {
    const char* v = std::getenv(name);
    return (v && *v) ? v : nullptr;
}

// "1", "true", "yes", "on" are true; "0", "false", "no", "off" are false.
// Anything else leaves the default alone rather than guessing -- a typo in a
// shell profile should not silently turn the cache off.
void env_bool(const char* name, bool* out) {
    const char* v = env(name);
    if (!v) return;
    std::string s;
    for (const char* p = v; *p; ++p) s += (char)std::tolower((unsigned char)*p);
    if (s == "1" || s == "true" || s == "yes" || s == "on")   *out = true;
    else if (s == "0" || s == "false" || s == "no" || s == "off") *out = false;
}

void env_str(const char* name, std::string* out) {
    if (const char* v = env(name)) *out = v;
}

}  // namespace

// ---------------------------------------------------------------------------
CxxConfig default_config() {
    CxxConfig c;
    c.compiler    = "g++";
    c.include_dir = SATELLITE_INCLUDE_DIR;
    c.lib         = SATELLITE_CORE_LIB;

    const char* home = std::getenv("HOME");
    const char* tmp  = std::getenv("TMPDIR");
    c.cache_dir = home ? std::string(home) + "/.satellite/cache"
                : tmp  ? std::string(tmp)  + "/satellite-cache"
                       : "/tmp/satellite-cache";

    env_str("SATELLITE_CXX_COMPILER",   &c.compiler);
    env_str("SATELLITE_CXX_CACHE_DIR",  &c.cache_dir);
    env_str("SATELLITE_CXX_STD",        &c.std_version);
    env_str("SATELLITE_CXX_FLAGS",      &c.extra_flags);
    env_str("SATELLITE_CXX_LINK_FLAGS", &c.link_flags);

    if (const char* v = env("SATELLITE_CXX_OPT")) {
        // -Ofast and -Os are deliberately not reachable through a plain
        // integer.  Anyone who wants them can say so in SATELLITE_CXX_FLAGS,
        // where it is visible in describe() rather than hidden behind a number.
        int n = std::atoi(v);
        c.opt_level = n < 0 ? 0 : (n > 3 ? 3 : n);
    }

    if (const char* v = env("SATELLITE_CXX_ENGINE")) {
        std::string s;
        for (const char* p = v; *p; ++p) s += (char)std::tolower((unsigned char)*p);
        if      (s == "jit")  c.engine = Engine::Jit;
        else if (s == "fork") c.engine = Engine::Fork;
        else if (s == "auto") c.engine = Engine::Auto;
    }

    env_bool("SATELLITE_CXX_NATIVE",       &c.native_arch);
    env_bool("SATELLITE_CXX_FAST_MATH",    &c.fast_math);
    env_bool("SATELLITE_CXX_CACHE",        &c.cache_enabled);
    env_bool("SATELLITE_CXX_KEEP_SOURCES", &c.keep_sources);
    env_bool("SATELLITE_CXX_VERBOSE",      &c.verbose);
    return c;
}

// ---------------------------------------------------------------------------
// The order here is fixed and must stay fixed: this string is part of the
// cache key, so a set that reordered itself between runs would hash
// differently every time and never hit the cache.
std::string CxxConfig::compile_flags() const {
    std::ostringstream f;
    f << "-O" << opt_level << " -std=" << std_version;
    // -march=native bakes in whatever this CPU supports, so a cached module
    // stops being portable to another machine.  That is fine -- the cache is
    // per-user under $HOME and never shipped -- but it is why the flag is in
    // the key: copying $HOME to a different CPU would otherwise load a module
    // full of instructions it cannot execute.
    if (native_arch) f << " -march=native";
    // -ffast-math permits reassociation and assumes no NaNs, so it changes
    // ANSWERS, not just speed.  Off by default for that reason.
    if (fast_math)   f << " -ffast-math";
    if (!extra_flags.empty()) f << ' ' << extra_flags;
    return f.str();
}

std::string CxxConfig::describe() const {
    std::ostringstream o;
    auto row = [&](const char* name, const std::string& value, const char* var) {
        o << "  " << name;
        for (size_t k = std::string(name).size(); k < 14; ++k) o << ' ';
        o << value;
        for (size_t k = value.size(); k < 34; ++k) o << ' ';
        o << var << '\n';
    };
    auto yn = [](bool b) { return std::string(b ? "on" : "off"); };

    o << "satellite.cxx settings\n\n";
    o << "  setting       value                             environment variable\n";
    o << "  ------------  --------------------------------  --------------------\n";
    const char* eng = engine == Engine::Jit  ? "jit"
                    : engine == Engine::Fork ? "fork"
                    : jit_engine_installed() ? "auto (jit)" : "auto (fork)";
    row("engine",      eng,                         "SATELLITE_CXX_ENGINE");
    row("compiler",    compiler,                    "SATELLITE_CXX_COMPILER");
    row("std",         std_version,                 "SATELLITE_CXX_STD");
    row("opt",         "-O" + std::to_string(opt_level), "SATELLITE_CXX_OPT");
    row("native",      yn(native_arch),             "SATELLITE_CXX_NATIVE");
    row("fast-math",   yn(fast_math),               "SATELLITE_CXX_FAST_MATH");
    row("flags",       extra_flags.empty() ? "(none)" : extra_flags,
                                                    "SATELLITE_CXX_FLAGS");
    row("link-flags",  link_flags.empty() ? "(none)" : link_flags,
                                                    "SATELLITE_CXX_LINK_FLAGS");
    row("cache",       yn(cache_enabled),           "SATELLITE_CXX_CACHE");
    row("cache-dir",   cache_dir,                   "SATELLITE_CXX_CACHE_DIR");
    row("keep-sources", yn(keep_sources),           "SATELLITE_CXX_KEEP_SOURCES");
    row("verbose",     yn(verbose),                 "SATELLITE_CXX_VERBOSE");

    o << "\n  effective compile flags:  " << compile_flags() << '\n';
    o << "  include dir:              " << include_dir << '\n';
    o << "  core library:             " << lib << '\n';
    o << "\nEvery setting above that changes the generated code is part of the\n"
         "cache key, so changing one recompiles rather than serving a stale\n"
         "module.  Blocks compiled under the old settings stay on disk and are\n"
         "reused if you change back.\n";
    return o.str();
}

}  // namespace cxx
}  // namespace satellite
