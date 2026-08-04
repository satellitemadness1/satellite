// The two public entry points of the embedded compiler.
//
//   run()       a bare C++ body, for `satellite jit "cout << 1;"`
//   run_unit()  a complete translation unit, for satellite.cxx blocks
//
// Compile and run are timed SEPARATELY throughout, and it matters that they
// are.  An embedded JIT does not make code run faster -- it emits the machine
// code clang always would.  What it removes is the cost AROUND compiling: no
// fork, no .o and .so on disk, no linker, no dlopen.  One combined number
// would hide exactly the thing worth knowing.
#include "jit_internal.hpp"

#include "../capture_out.hpp"

#include <clang/Basic/Version.h>

#include <memory>
#include <sstream>

namespace satellite {
namespace jit {

bool available() { return true; }

std::string version() { return clang::getClangFullVersion(); }

namespace {

// Whatever the code prints goes to this process's stdout, which for a GUI is a
// log file nobody is looking at.  See src/capture_out.hpp for why this works at
// the descriptor level rather than by swapping a streambuf.
struct Capture {
    std::unique_ptr<::satellite::detail::CaptureStdout> c;
    explicit Capture(std::string* want) {
        if (want) c = std::make_unique<::satellite::detail::CaptureStdout>();
    }
    void finish(std::string* out) { if (c && out) *out = c->stop(); }
};

}  // namespace

// ---------------------------------------------------------------------------
Container run(const std::string& body, std::string* err,
              std::string* out_printed, Timing* timing) {
    if (err) err->clear();
    const Clock::time_point t_compile = Clock::now();

    // A preamble, so that `jit cout << "hi";` is a complete program.  Typing
    // #include lines by hand at a console prompt is not something a person
    // should have to do -- the same reasoning as the cxx bridge's prologue,
    // and the same header set for consistency.
    //
    // A body that starts with its own #include lines still works: they are
    // lifted out and placed above this preamble, because a #include cannot
    // appear inside a function body.
    std::string includes, rest;
    {
        std::istringstream in(body);
        std::string        ln;
        bool               still_leading = true;
        while (std::getline(in, ln)) {
            size_t a = ln.find_first_not_of(" \t");
            const bool is_inc =
                a != std::string::npos && ln.compare(a, 8, "#include") == 0;
            if (still_leading && is_inc) { includes += ln + "\n"; continue; }
            if (a != std::string::npos) still_leading = false;
            rest += ln + "\n";
        }
    }

    const std::string source =
        includes +
        "#include <cstdio>\n#include <string>\n#include <vector>\n"
        "#include <iostream>\n#include <exception>\nusing namespace std;\n"
        "extern \"C\" long satellite_block() {\n" + rest + "\n"
        // A body that only prints has no return statement, and falling off the
        // end of a non-void function is undefined behaviour rather than a
        // helpful diagnostic.  This makes the return optional.
        "return 0;\n}\n";

    std::unique_ptr<llvm::orc::LLJIT> jit = compile(source, err);
    if (!jit) return Container::nil();

    auto sym = jit->lookup("satellite_block");
    if (!sym) {
        if (err) *err = llvm::toString(sym.takeError());
        return Container::nil();
    }
    if (timing) timing->compile_ms = ms_since(t_compile);

    Capture cap(out_printed);
    // The LLJIT owns the code, so the call has to happen before it goes out of
    // scope -- taking the pointer out and calling it later is a use-after-free.
    const Clock::time_point t_run = Clock::now();
    const long rv = sym->toPtr<long (*)()>()();
    if (timing) timing->run_ms = ms_since(t_run);
    cap.finish(out_printed);

    return Container::integer((int64_t)rv);
}

// ---------------------------------------------------------------------------
Container run_unit(const std::string& unit, const std::string& entry,
                   const Container* args, int argc, std::string* err,
                   std::string* out_printed, Timing* timing) {
    if (err) err->clear();
    const Clock::time_point t_compile = Clock::now();

    std::unique_ptr<llvm::orc::LLJIT> jit = compile(unit, err);
    if (!jit) return Container::nil();

    auto sym = jit->lookup(entry);
    if (!sym) {
        if (err) *err = "the compiled unit has no `" + entry + "` symbol: " +
                        llvm::toString(sym.takeError());
        return Container::nil();
    }
    if (timing) timing->compile_ms = ms_since(t_compile);

    // The SAME signature the g++ bridge's modules have, because it is the same
    // generated source.  That is the point: the two engines are interchangeable
    // and a block cannot tell which one ran it.
    using BlockFn = Container (*)(const Container*, int);
    BlockFn fn = sym->toPtr<BlockFn>();

    Capture cap(out_printed);
    const Clock::time_point t_run = Clock::now();
    Container result = fn(args, argc);
    if (timing) timing->run_ms = ms_since(t_run);
    cap.finish(out_printed);

    return result;
}

}  // namespace jit
}  // namespace satellite
