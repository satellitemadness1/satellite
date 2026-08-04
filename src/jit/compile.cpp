// Source text -> machine code, without leaving the process.
//
// clang parses from an in-memory file, emits LLVM IR, and ORC links it into
// executable memory.  No fork, no .o, no linker, no dlopen.
#include "jit_internal.hpp"

#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Driver/CreateInvocationFromArgs.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/VirtualFileSystem.h>
#include <llvm/Support/raw_ostream.h>

#include <mutex>
#include <vector>

namespace satellite {
namespace jit {

static std::once_flag g_init;

void init_llvm() {
    std::call_once(g_init, [] {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        // libstdc++ headers contain inline asm, and parsing it needs the asm
        // PARSER, not just the printer.  Without this the JIT aborts the whole
        // process with "Inline asm not supported by this streamer" the first
        // time a real standard-library header is included -- which is why
        // hello-world is a better smoke test than `return 6*7;`.
        llvm::InitializeNativeTargetAsmParser();
    });
}

std::unique_ptr<llvm::orc::LLJIT> compile(const std::string& source,
                                          std::string* err) {
    init_llvm();

    // The in-memory layer sits ON TOP of the real filesystem, so /virtual/in.cpp
    // is served from memory while #include <vector> still resolves normally.
    // Once the header blob is mounted here too, the real FS drops out entirely
    // and the binary stops needing anything on disk at all.
    auto mem = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
    auto overlay = llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(
        llvm::vfs::getRealFileSystem());
    overlay->pushOverlay(mem);
    mem->addFile("/virtual/in.cpp", 0,
                 llvm::MemoryBuffer::getMemBufferCopy(source));

    // Diagnostics go into a string rather than to stderr: this is a library,
    // and the caller (a GUI text view, a CLI) decides where errors are shown.
    std::string              diag_text;
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
    std::vector<const char*> argv = {
        "clang++", "-xc++", "-std=c++20", "-O2",
        "-resource-dir", SATELLITE_CLANG_RESOURCE_DIR,
        // satellite's own headers, so a JIT'd block can speak Container.
        "-I", SATELLITE_INCLUDE_DIR,
        // The block entry point is `extern "C" Container satellite_cxx_block`,
        // and clang warns that a C-linkage function returning a C++ type is
        // not callable from C.  Nothing here ever claimed it was: extern "C"
        // is doing one job, giving the symbol an unmangled name that ORC and
        // dlsym can look up.  Both sides of the call are C++ built for the same
        // platform ABI, so the return is well defined.
        //
        // Left ON for the fork bridge, which only ever shows diagnostics when a
        // compile FAILS.  Here the warning surfaced as a bare "1 warning
        // generated." on stderr -- clang writes that summary straight to
        // llvm::errs(), so it escapes the diagnostic printer this code installs.
        "-Wno-return-type-c-linkage",
        "/virtual/in.cpp"};

    clang::CreateInvocationOptions cio;
    cio.Diags = diags;
    cio.VFS   = overlay;
    std::shared_ptr<clang::CompilerInvocation> inv =
        clang::createInvocation(argv, cio);
    if (!inv) {
        if (err) *err = diag_text.empty() ? "could not build the compiler "
                                            "invocation" : diag_text;
        return nullptr;
    }

    // A SECOND diagnostics engine, built from the invocation's own options.
    //
    // The first one had to exist before createInvocation, because building the
    // invocation can itself produce diagnostics -- but it was made from a
    // default-constructed DiagnosticOptions, which knows nothing about the -W
    // flags in argv.  Installing that engine on the CompilerInstance threw away
    // everything the driver had just worked out, so no warning flag had any
    // effect: -Wno-return-type-c-linkage was parsed, accepted, and ignored.
    //
    // Same output string, so diagnostics still land in one place for the caller.
    auto* printer2 = new clang::TextDiagnosticPrinter(diag_os,
                                                      inv->getDiagnosticOpts());
    auto  diags2   = clang::CompilerInstance::createDiagnostics(
        *overlay, inv->getDiagnosticOpts(), printer2, /*ShouldOwnClient=*/true);

    // The invocation goes to the CONSTRUCTOR -- there is no setter.
    auto ci = std::make_unique<clang::CompilerInstance>(std::move(inv));
    ci->setVirtualFileSystem(overlay);
    ci->setDiagnostics(diags2.get());
    ci->createFileManager();
    ci->createSourceManager();

    auto ctx = std::make_unique<llvm::LLVMContext>();
    clang::EmitLLVMOnlyAction action(ctx.get());
    if (!ci->ExecuteAction(action)) {
        if (err) *err = diag_text.empty() ? "compilation failed" : diag_text;
        return nullptr;
    }

    std::unique_ptr<llvm::Module> mod = action.takeModule();
    if (!mod) {
        if (err) *err = diag_text.empty() ? "no module produced" : diag_text;
        return nullptr;
    }

    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) {
        if (err) *err = llvm::toString(jit.takeError());
        return nullptr;
    }

    // Let the JIT'd code reach every symbol this process already has --
    // Container::str, the dispatch tables, to_container, the C++ runtime.  That
    // is what makes a block able to speak satellite's own value model without
    // anything being written to a shared library first.
    //
    // Registered explicitly rather than relied upon: a statically linked host
    // exports through .dynsym only with -rdynamic, and being explicit here
    // means the single-file build does not silently lose the ability to call
    // home.
    auto& jd = (*jit)->getMainJITDylib();
    if (auto gen = llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(
            (*jit)->getDataLayout().getGlobalPrefix())) {
        jd.addGenerator(std::move(*gen));
    } else {
        if (err) *err = llvm::toString(gen.takeError());
        return nullptr;
    }

    if (auto e = (*jit)->addIRModule(
            llvm::orc::ThreadSafeModule(std::move(mod), std::move(ctx)))) {
        if (err) *err = llvm::toString(std::move(e));
        return nullptr;
    }
    return std::move(*jit);
}

}  // namespace jit
}  // namespace satellite
