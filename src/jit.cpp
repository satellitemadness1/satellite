// The embedded C++ compiler -- clang and LLVM inside the satellite process.
//
// Two builds come out of this one file:
//
//   SATELLITE_HAVE_CLANG defined   -- the real thing
//   not defined                    -- a stub that reports itself unavailable
//
// The stub is what lets the ordinary cmake build keep working on a machine
// with no static LLVM archives.  A missing compiler is a capability the caller
// asks about, not a build error.
#include "satellite/jit.hpp"

#ifdef SATELLITE_HAVE_CLANG

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/Basic/Version.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Driver/CreateInvocationFromArgs.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include "capture_out.hpp"

#include <chrono>
#include <cstdio>
#include <memory>
#include <mutex>
#include <sstream>

namespace satellite {
namespace jit {

bool available() { return true; }

std::string version() { return clang::getClangFullVersion(); }

// LLVM's target registry is process-global and not safe to initialize twice
// from two threads.  once_flag rather than a bool, because satellite is heading
// for 200 workers and a racing bool would be a real bug rather than a
// theoretical one.
static std::once_flag g_init;

static void init_llvm() {
    std::call_once(g_init, [] {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        // libstdc++ headers contain inline asm, and parsing it needs the asm
        // PARSER, not just the printer.  Without this the JIT aborts the whole
        // process with "no asm parser for this target" the first time a real
        // standard-library header is included.
        llvm::InitializeNativeTargetAsmParser();
    });
}

using Clock = std::chrono::steady_clock;
static double ms_since(Clock::time_point t) {
    return std::chrono::duration<double, std::milli>(Clock::now() - t).count();
}

Container run(const std::string& body, std::string* err,
              std::string* out_printed, Timing* timing) {
    init_llvm();
    const Clock::time_point t_compile = Clock::now();
    if (err) err->clear();

    // A preamble, so that `jit std::cout << "hi";` is a complete program.
    // Typing #include lines by hand at a console prompt is not something a
    // person should have to do -- the same reasoning as the cxx bridge's
    // prologue in src/cxx.cpp, and the same header set for consistency.
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
        "#include <cstdio>\n"
        "#include <string>\n"
        "#include <vector>\n"
        "#include <iostream>\n"
        "#include <exception>\n"
        "using namespace std;\n"
        "extern \"C\" long satellite_block() {\n" + rest + "\n"
        // A body that only prints has no return statement, and falling off the
        // end of a non-void function is undefined behaviour rather than a
        // helpful diagnostic.  This makes the return optional.
        "return 0;\n}\n";

    // The in-memory layer sits ON TOP of the real filesystem, so /virtual/in.cpp
    // is served from memory while #include <vector> still resolves normally.
    // Once the header blob is mounted here too, the real FS drops out entirely.
    auto mem = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
    auto overlay = llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(
        llvm::vfs::getRealFileSystem());
    overlay->pushOverlay(mem);
    mem->addFile("/virtual/in.cpp", 0,
                 llvm::MemoryBuffer::getMemBufferCopy(source));

    // Diagnostics go into a string rather than to stderr: this is a library,
    // and the caller (a GUI text view, a CLI) decides where errors are shown.
    std::string          diag_text;
    llvm::raw_string_ostream diag_os(diag_text);

    // Diagnostics have to exist before the invocation, because building the
    // invocation is itself something that can produce diagnostics.
    auto  diag_opts = std::make_shared<clang::DiagnosticOptions>();
    auto* printer   = new clang::TextDiagnosticPrinter(diag_os, *diag_opts);
    auto  diags     = clang::CompilerInstance::createDiagnostics(
        *overlay, *diag_opts, printer, /*ShouldOwnClient=*/true);

    // Build the invocation through the DRIVER, not CreateFromArgs.
    //
    // CreateFromArgs takes -cc1 arguments and assumes someone has already
    // worked out where the headers live.  Nobody has: this process is not the
    // clang binary, so there is no install directory to infer them from, and
    // the first #include fails with "'cstdio' file not found".
    //
    // createInvocation runs the real driver, which detects the GCC install and
    // the libstdc++ version actually present.  That matters here -- the g++ on
    // this machine is a from-source GCC 17 under ~/source-opt, so any path list
    // written by hand would be wrong.
    //
    // -resource-dir has to be explicit for the same reason: clang normally
    // finds its builtin headers relative to its own executable, and that
    // executable is satellite.
    std::vector<const char*> argv = {"clang++", "-xc++", "-std=c++20", "-O2",
                                     "-resource-dir", SATELLITE_CLANG_RESOURCE_DIR,
                                     "/virtual/in.cpp"};
    clang::CreateInvocationOptions cio;
    cio.Diags = diags;
    cio.VFS   = overlay;
    std::shared_ptr<clang::CompilerInvocation> inv =
        clang::createInvocation(argv, cio);
    if (!inv) {
        if (err) *err = diag_text.empty() ? "could not build the compiler "
                                            "invocation" : diag_text;
        return Container::nil();
    }

    // The invocation goes to the CONSTRUCTOR -- there is no setter.
    auto ci = std::make_unique<clang::CompilerInstance>(std::move(inv));
    ci->setVirtualFileSystem(overlay);
    ci->setDiagnostics(diags.get());
    ci->createFileManager();
    ci->createSourceManager();

    auto ctx = std::make_unique<llvm::LLVMContext>();
    clang::EmitLLVMOnlyAction action(ctx.get());
    if (!ci->ExecuteAction(action)) {
        if (err) *err = diag_text.empty() ? "compilation failed" : diag_text;
        return Container::nil();
    }

    std::unique_ptr<llvm::Module> mod = action.takeModule();
    if (!mod) {
        if (err) *err = diag_text.empty() ? "no module produced" : diag_text;
        return Container::nil();
    }

    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) {
        if (err) *err = "could not create the JIT";
        return Container::nil();
    }
    if (auto e = (*jit)->addIRModule(llvm::orc::ThreadSafeModule(
            std::move(mod), std::move(ctx)))) {
        if (err) *err = llvm::toString(std::move(e));
        return Container::nil();
    }
    auto sym = (*jit)->lookup("satellite_block");
    if (!sym) {
        if (err) *err = llvm::toString(sym.takeError());
        return Container::nil();
    }

    if (timing) timing->compile_ms = ms_since(t_compile);

    // Whatever the JIT'd code prints goes to the process's stdout, which for a
    // GUI is a log file nobody is looking at.  Redirect fd 1 across the call so
    // `std::cout << "hello"` lands in the console window instead.  The same
    // helper does this for the g++/dlopen bridge -- see src/capture_out.hpp for
    // why it works at the descriptor level.
    std::unique_ptr<detail::CaptureStdout> cap;
    if (out_printed) cap = std::make_unique<detail::CaptureStdout>();

    // The LLJIT owns the code, so the call has to happen before it goes out of
    // scope -- taking the pointer out and calling it later is a use-after-free.
    const Clock::time_point t_run = Clock::now();
    const long rv = sym->toPtr<long (*)()>()();
    if (timing) timing->run_ms = ms_since(t_run);

    if (cap) *out_printed = cap->stop();

    return Container::integer((int64_t)rv);
}

}  // namespace jit
}  // namespace satellite

#else   // ---------------------------------------------------------------

namespace satellite {
namespace jit {

bool        available() { return false; }
std::string version()   { return ""; }

Container run(const std::string&, std::string* err, std::string*, Timing*) {
    if (err)
        *err = "this build has no embedded compiler -- link with clang and "
               "LLVM to get one (see docs/SINGLE_FILE.md)";
    return Container::nil();
}

}  // namespace jit
}  // namespace satellite

#endif
