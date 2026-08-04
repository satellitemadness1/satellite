// Shared internals of the embedded compiler.
//
// src/jit.cpp was one file; it is now src/jit/ for the same reason ops/ and
// cxx/ are directories -- it grew past the point where one file was honest
// about how many separate jobs it was doing:
//
//     jit_internal.hpp   this file -- LLVM setup and the compile seam
//     compile.cpp        source text -> a loaded, executable LLJIT
//     run.cpp            the two public entry points
//     unavailable.cpp    the whole API, for a build with no static LLVM
//
// This header pulls in LLVM's own headers, so only files compiled with the
// LLVM include path and -fno-rtti may include it.  CMake sets both on the
// files in this directory and nowhere else, which is the same containment that
// keeps include/satellite/jit.hpp free of LLVM types.
#pragma once

#include "satellite/jit.hpp"

#include <llvm/ExecutionEngine/Orc/LLJIT.h>

#include <chrono>
#include <memory>
#include <string>

namespace satellite {
namespace jit {

using Clock = std::chrono::steady_clock;

inline double ms_since(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

// LLVM's target registry is process-global and not safe to initialize twice
// from two threads.
void init_llvm();

// Compile a complete translation unit and hand back a JIT that has it loaded.
// Returns nullptr with *err set to clang's own diagnostics on failure.
//
// The caller keeps the LLJIT alive for as long as it intends to call into the
// code: the JIT owns the executable memory, so letting it go and calling a
// function pointer taken out of it earlier is a use-after-free.
std::unique_ptr<llvm::orc::LLJIT> compile(const std::string& source,
                                          std::string* err);

}  // namespace jit
}  // namespace satellite
