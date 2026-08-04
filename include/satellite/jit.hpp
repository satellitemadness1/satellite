// The embedded C++ compiler.
//
// satellite.cxx blocks currently take the long road: write a .cpp, fork g++,
// dlopen the .so.  This is the short road -- clang and LLVM linked into the
// process, compiling from memory and JIT-ing straight to a function pointer.
// No fork, no temporary file, no dlopen.
//
// Deliberately no LLVM types in this header.  Everything under include/ is
// preprocessed by every satellite.cxx block at runtime, and clang's headers
// cost ~195,000 lines where satellite's own cost ~44,000.  The GUI and the CLI
// get to call the JIT without paying for its headers.
//
// A build without an embedded clang still compiles and links against this
// header; available() simply returns false.  That is what keeps the ordinary
// cmake build working on a machine with no LLVM static archives.
#pragma once

#include <string>

#include "satellite/container.hpp"

namespace satellite {
namespace jit {

// Was this binary linked with an embedded clang?  Everything below is a no-op
// returning an error when this is false.
bool available();

// "clang version 24.0.0git", or empty when there is no embedded compiler.
std::string version();

// Compile a C++ function BODY and run it.  The body is wrapped in
//
//     extern "C" long satellite_block() { <body> }
//
// so `return 6*7;` is a complete program.  Returns the value as an Int
// Container, or nil with *err set.
//
// The body is compiled fresh each call.  There is no caching here on purpose --
// the on-disk cache exists because forking g++ costs ~1s; an in-process JIT
// does not need one until measurement says it does.
// Anything the body printed to stdout is written to *out_printed, so a console
// can show it.  A GUI has no terminal, and code that prints into the void looks
// broken rather than unsupported.
//
// The two halves are timed SEPARATELY and it matters that they are.  An
// embedded JIT does not make code run faster -- it emits the same machine code
// clang always would.  What it removes is the cost around compiling: no fork,
// no .o and .so on disk, no linker, no dlopen.  Reporting one combined number
// would hide exactly the thing worth knowing.
struct Timing {
    double compile_ms = 0;   // parse, sema, IR, optimize, codegen
    double run_ms     = 0;   // the JIT'd code actually executing
    double total_ms() const { return compile_ms + run_ms; }
};

Container run(const std::string& body, std::string* err,
              std::string* out_printed = nullptr, Timing* timing = nullptr);

// Compile a COMPLETE translation unit -- what satellite::cxx::generate()
// produces -- and call its entry point, which must have the signature
//
//     extern "C" Container <entry>(const Container* args, int argc)
//
// This is what frees satellite.cxx blocks from g++.  Identical generated
// source and identical Container ABI; the only difference is that clang runs
// in this process instead of being forked, and the code is linked by ORC
// instead of being written to a .so and dlopen'd back.
//
// The JIT'd code calls straight into this process for Container::str,
// to_container, the dispatch tables and everything else -- ORC resolves those
// against the host, so nothing has to be exported to a shared library first.
Container run_unit(const std::string& unit, const std::string& entry,
                   const Container* args, int argc, std::string* err,
                   std::string* out_printed = nullptr, Timing* timing = nullptr);

}  // namespace jit
}  // namespace satellite
