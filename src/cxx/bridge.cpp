// Compile, cache, load, call.
//
// A block is content-addressed: the generated translation unit is hashed, and
// the hash names the .so.  Same text, same binary, no compiler run.  Forking
// g++ costs about a second, so the cache is the difference between a program
// that starts instantly on the second run and one that never feels finished.
#include "cxx_internal.hpp"

#include "../capture_out.hpp"

#include <cstdio>
#include <chrono>
#include <dlfcn.h>
#include <fstream>
#include <memory>
#include <sstream>
#include <unistd.h>

namespace satellite {
namespace cxx {

using Clock = std::chrono::steady_clock;
static double ms_since(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

// ---------------------------------------------------------------------------
Bridge::Bridge(CxxConfig cfg) : cfg_(std::move(cfg)) {
    if (!cfg_.cache_dir.empty()) detail::make_dirs(cfg_.cache_dir);
}

Bridge::~Bridge() {
    // Deliberately NOT dlclose'd: static destructors in a module can run while
    // satellite values still point into it.  Modules live for the process.
    handles_.clear();
}

bool Bridge::build(const std::string& block_source, std::string* so_path,
                   std::string* err) {
    return build(block_source, {}, so_path, err);
}

bool Bridge::build(const std::string& block_source,
                   const std::vector<CxxArg>& args, std::string* so_path,
                   std::string* err) {
    // THE SETTINGS ARE PART OF THE SOURCE, and therefore part of the cache key.
    //
    // The key is a hash of the text below.  The compile flags do not appear
    // anywhere in the generated code, so without stamping them here, raising
    // opt_level from 2 to 3 would hash identically to the -O2 module already on
    // disk and be served straight back -- the setting would look like it did
    // nothing, and the reason would be invisible.  The compiler name matters
    // for the same reason: g++ and clang++ produce different binaries from
    // identical input.
    //
    // Written into the file rather than mixed into the hash on the side, so
    // that a kept .cpp records exactly how it was built.
    std::ostringstream stamped;
    stamped << "// built with: " << cfg_.compiler << ' '
            << cfg_.compile_flags() << '\n'
            << generate(block_source, args);
    const std::string unit = stamped.str();

    std::string key = detail::hash_hex(unit);
    std::string so  = cfg_.cache_dir + "/cxx_" + key + ".so";

    if (cfg_.cache_enabled && detail::file_exists(so)) {   // same input, same binary
        ++hits_;
        if (so_path) *so_path = so;
        return true;
    }

    std::string cpp = cfg_.cache_dir + "/cxx_" + key + ".cpp";
    { std::ofstream f(cpp); if (!f) { if (err) *err = "cannot write " + cpp; return false; } f << unit; }

    std::ostringstream cmd;
    cmd << cfg_.compiler << ' ' << cfg_.compile_flags() << " -shared -fPIC"
        << " -I" << cfg_.include_dir
        << ' '   << cpp;
    if (!cfg_.lib.empty()) {
        size_t slash = cfg_.lib.find_last_of('/');
        std::string dir = slash == std::string::npos ? "." : cfg_.lib.substr(0, slash);
        cmd << ' ' << cfg_.lib << " -Wl,-rpath," << dir;
    }
    if (!cfg_.link_flags.empty()) cmd << ' ' << cfg_.link_flags;
    cmd << " -o " << so << " 2>&1";

    if (cfg_.verbose) std::fprintf(stderr, "satellite.cxx: %s\n", cmd.str().c_str());

    FILE* p = popen(cmd.str().c_str(), "r");
    if (!p) { if (err) *err = "cannot start the compiler"; return false; }
    std::string diag;
    char buf[512];
    while (std::fgets(buf, sizeof buf, p)) diag += buf;
    int rc = pclose(p);

    if (rc != 0) {
        ::unlink(so.c_str());                       // never cache a failure
        // The .cpp stays on disk when the compile FAILS regardless of the
        // keep_sources setting: the diagnostic refers to line numbers in it,
        // and deleting it would leave an error message pointing at nothing.
        if (err) *err = diag.empty() ? "compilation failed" : diag;
        return false;
    }
    if (!cfg_.keep_sources) ::unlink(cpp.c_str());

    ++compiles_;
    if (so_path) *so_path = so;
    return true;
}

// ---------------------------------------------------------------------------
Container Bridge::run(const std::string& block_source, std::string* err) {
    return run(block_source, {}, err);
}

Container Bridge::run(const std::string& block_source,
                      const std::vector<CxxArg>& args, std::string* err,
                      std::string* out_printed, Timing* timing) {
    if (out_printed) out_printed->clear();

    const size_t hits_before = hits_;
    const Clock::time_point t_compile = Clock::now();
    std::string so;
    if (!build(block_source, args, &so, err)) return Container::nil();
    if (timing) {
        timing->cached     = (hits_ > hits_before);
        timing->compile_ms = ms_since(t_compile);
    }

    const Clock::time_point t_load = Clock::now();
    void* h = ::dlopen(so.c_str(), RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        if (err) *err = std::string("dlopen: ") + dlerror();
        return Container::nil();
    }

    auto abi = (int (*)())::dlsym(h, "satellite_cxx_abi");
    if (!abi) {
        if (err) *err = "module has no satellite_cxx_abi symbol";
        return Container::nil();
    }
    if (abi() != ABI) {
        if (err) *err = "ABI mismatch: module " + std::to_string(abi()) +
                        ", host " + std::to_string(ABI) + " -- refusing to load";
        return Container::nil();
    }

    using BlockFn = Container (*)(const Container*, int);
    auto fn = (BlockFn)::dlsym(h, "satellite_cxx_block");
    if (!fn) {
        if (err) *err = "module has no satellite_cxx_block symbol";
        return Container::nil();
    }
    handles_.push_back(h);
    if (timing) timing->load_ms = ms_since(t_load);

    // The module takes a flat array, so the values are lifted out of the
    // CxxArgs.  The names went into the generated source at compile time and
    // are not needed again -- what crosses the ABI boundary is 16 bytes per
    // argument and nothing else.
    std::vector<Container> values;
    values.reserve(args.size());
    for (const auto& a : args) values.push_back(a.value);

    Container result;
    {
        // Constructed before the timer starts and destroyed after it stops:
        // the pipe setup is satellite's cost, not the block's.
        std::unique_ptr<::satellite::detail::CaptureStdout> cap;
        if (out_printed) cap = std::make_unique<::satellite::detail::CaptureStdout>();

        const Clock::time_point t_run = Clock::now();
        result = fn(values.empty() ? nullptr : values.data(), (int)values.size());
        if (timing) timing->run_ms = ms_since(t_run);

        if (cap) *out_printed = cap->stop();
    }
    return result;
}

}  // namespace cxx
}  // namespace satellite
