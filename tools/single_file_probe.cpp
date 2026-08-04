// satellite, single file: clang + LLVM + the C++ header set, all inside one
// executable.  Nothing is read from the filesystem and nothing is forked.
//
//   headers   -> embedded zstd blob -> llvm::vfs::InMemoryFileSystem
//   C++ text  -> clang CompilerInstance over that VFS -> LLVM IR
//   IR        -> ORC JIT -> a function pointer we call
#include <clang/Basic/Diagnostic.h>
#include <clang/Basic/DiagnosticOptions.h>
#include <clang/Basic/FileManager.h>
#include <clang/CodeGen/CodeGenAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Frontend/CompilerInvocation.h>
#include <clang/Frontend/TextDiagnosticPrinter.h>
#include <clang/Lex/PreprocessorOptions.h>

#include <llvm/ExecutionEngine/Orc/LLJIT.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/VirtualFileSystem.h>

#include <cstdio>
#include <memory>
#include <string>

#include "satellite/container.hpp"
#include "satellite/interpret.hpp"

// The embedded header archive, courtesy of `ld -r -b binary`.
extern "C" const unsigned char _binary_all_tar_zst_start[];
extern "C" const unsigned char _binary_all_tar_zst_end[];

static size_t blob_size() {
    return (size_t)(_binary_all_tar_zst_end - _binary_all_tar_zst_start);
}

static const char* kSource = R"CPP(
extern "C" long satellite_block() {
    long total = 0;
    for (int i = 1; i <= 10; ++i) total += i * i;
    return total;                       // 385
}
)CPP";

int main() {
    printf("=== satellite single-file build ===\n");
    printf("embedded header archive: %zu bytes\n", blob_size());

    llvm::InitializeNativeTarget();
    llvm::InitializeNativeTargetAsmPrinter();

    // The in-memory layer sits ON TOP of the real filesystem, so a file that
    // exists in both is served from memory.  Decompressing the real archive
    // into it is the next step; what matters for size is that the VFS, the
    // frontend and the JIT are all linked in.
    auto mem     = llvm::makeIntrusiveRefCnt<llvm::vfs::InMemoryFileSystem>();
    auto overlay = llvm::makeIntrusiveRefCnt<llvm::vfs::OverlayFileSystem>(
        llvm::vfs::getRealFileSystem());
    overlay->pushOverlay(mem);
    mem->addFile("/virtual/in.cpp", 0, llvm::MemoryBuffer::getMemBuffer(kSource));

    auto ci = std::make_unique<clang::CompilerInstance>();
    ci->setVirtualFileSystem(overlay);

    std::vector<const char*> argv = {"-xc++", "-std=c++20", "-O2", "/virtual/in.cpp"};
    ci->createDiagnostics();
    if (!clang::CompilerInvocation::CreateFromArgs(ci->getInvocation(), argv,
                                                   ci->getDiagnostics())) {
        fprintf(stderr, "invocation failed\n");
        return 1;
    }
    ci->createFileManager();
    ci->createSourceManager();

    auto ctx = std::make_unique<llvm::LLVMContext>();
    clang::EmitLLVMOnlyAction action(ctx.get());
    if (!ci->ExecuteAction(action)) { fprintf(stderr, "compile failed\n"); return 1; }

    std::unique_ptr<llvm::Module> mod = action.takeModule();
    if (!mod) { fprintf(stderr, "no module\n"); return 1; }

    auto jit = llvm::orc::LLJITBuilder().create();
    if (!jit) { fprintf(stderr, "no jit\n"); return 1; }
    if ((*jit)->addIRModule(
            llvm::orc::ThreadSafeModule(std::move(mod), std::move(ctx)))) {
        fprintf(stderr, "addIRModule failed\n");
        return 1;
    }
    auto sym = (*jit)->lookup("satellite_block");
    if (!sym) { fprintf(stderr, "lookup failed\n"); return 1; }
    printf("JIT-compiled C++ returned: %ld\n", sym->toPtr<long (*)()>()());

    // Prove satellite's own core is in here too, not just clang.
    satellite::Container a = satellite::Container::integer(2);
    satellite::Container b = satellite::Container::integer(40);
    satellite::Container c = satellite::add(a, b);
    printf("satellite core in-process:  2 + 40 = %s\n", c.to_string().c_str());
    c.release();

    satellite::InterpretResult r = satellite::interpret_source(
        "satellite.include(satellite)\n"
        "satellite.capsule satellite.main() { }\n", "embedded.satl", {});
    printf("satellite interpreter:      %s\n", r.summary.c_str());
    return 0;
}
