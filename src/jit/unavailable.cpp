// The embedded compiler, for a build that does not have one.
//
// A missing compiler is a capability the caller asks about, not a build error.
// This is what keeps the ordinary cmake build working on a machine with no
// static LLVM archives -- everything links, and available() answers false.
#include "satellite/jit.hpp"

namespace satellite {
namespace jit {

bool        available() { return false; }
std::string version()   { return ""; }

static const char* kWhy =
    "this build has no embedded compiler -- link with clang and LLVM to get "
    "one (see docs/SINGLE_FILE.md)";

Container run(const std::string&, std::string* err, std::string*, Timing*) {
    if (err) *err = kWhy;
    return Container::nil();
}

Container run_unit(const std::string&, const std::string&, const Container*,
                   int, std::string* err, std::string*, Timing*) {
    if (err) *err = kWhy;
    return Container::nil();
}

}  // namespace jit
}  // namespace satellite
