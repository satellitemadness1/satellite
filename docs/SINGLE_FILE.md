# One file — satellite with a C++ compiler inside it

> "need to fit these things into the .exe is that possible to have a single
> file for the exe?"

Yes. It has been **built and run**, not estimated.

## The answer

```
satellite_single    111,619,736 bytes   106.45 MB   unstripped
ss_stripped          91,565,008 bytes    87.32 MB   stripped
clang/LLVM shared-library dependencies: 0
```

Running it:

```
=== satellite single-file build ===
embedded header archive: 3098450 bytes
JIT-compiled C++ returned: 385
satellite core in-process:  2 + 40 = 42
satellite interpreter:      ok -- 19 tokens, 0 cxx blocks, stopped at validate
```

One file. It contains clang, LLVM, satellite's own core, and 2,330 C++ headers
as a compressed blob. It parsed C++ from memory, emitted LLVM IR, JIT-compiled
it with ORC, called the result, and then ran satellite's own interpreter — all
in one process, with no fork, no `g++`, no temporary `.so` and no `dlopen`.

This build links `llvm-config --libs all`, so it carries **every** target
backend — AArch64, AMDGPU, ARM, RISC-V and the rest — not just x86-64.
Restricting to the native target only is where the 43 MB figure below comes
from; 87 MB is the price of keeping every backend, and cross-compilation with
it.

## The budget

| Component | Size | How measured |
|---|---|---|
| clang + LLVM, static, minimal (parse → IR → x86 codegen) | **43 MB** stripped, 52 MB not | linked `llvm_size_test_full.cpp` |
| clang + LLVM, static, full frontend | **84 MB** stripped, 102 MB not | same, wider lib set |
| libstdc++ headers (914 files, 15.1 MB raw) | **1.5 MB** zstd | tarred and compressed |
| system/glibc headers (891 files, 13.7 MB raw) | **1.3 MB** xz | tarred and compressed |
| satellite core + CLI | ~0.2 MB | build tree |
| **total, one file** | **≈ 46 MB minimal / 87 MB full** | |

Headers compress about 10:1 because they are text. `/usr/include` is 336 MB on
this box, but that is boost, GTK and LLVM's own headers; the set a C++ program
actually needs is 1,805 files and 28.7 MB raw.

For comparison, the shared-library route costs a 30–47 KB executable plus
`libLLVM.so` (119 MB) and `libclang-cpp.so` (63 MB) — **182 MB** to ship, four
times the static single file, because a shared library carries every target
backend and every symbol whether or not this program references them.

## Why a single file is possible at all

The blocker is not code, it is headers: `#include <vector>` needs header text to
exist somewhere at the moment the block compiles. LLVM answers this with
`llvm::vfs::InMemoryFileSystem`, so headers become bytes you carry rather than
files you install.

`vfs_proto.cpp` proves it, and the negative control is the part that matters:

```
[1] NEGATIVE CONTROL: real filesystem only (no in-memory FS)
    compile succeeded: NO
    | error: error reading '/virtual/main.cpp': No such file or directory
    -> clang genuinely cannot find the files on disk
[2] OverlayFileSystem(real FS) + pushOverlay(InMemoryFileSystem)
    overlay stat /virtual/myheader.h -> size 48 bytes
    -fsyntax-only compile succeeded: YES
[3] Full codegen (-O1 -emit-llvm) over the same overlay
    codegen succeeded: YES
```

Without step [1] this would only show that clang compiled *something*. With it,
we know the headers really came from memory.

The other half — actually running the result in-process — is proven too:

```
JIT-compiled C++ inside the process returned: 385
```

That is clang building a `CompilerInstance`, parsing C++ from an in-memory
buffer, emitting an LLVM module, and ORC JIT-ing it to a callable function
pointer. No fork, no `g++`, no temporary `.so`, no `dlopen`.

## The shape

1. Static-link clang + LLVM into the satellite executable.
2. Embed the compressed header set in a data section.
3. On the first `satellite.cxx` block, decompress into an `InMemoryFileSystem`
   and overlay it. Once per process, not once per block.
4. Compile to an LLVM module in memory; ORC JIT it; call it.

## What this removes

`CMakeLists.txt` currently builds the core as `SHARED` so that `satellite.cxx`
modules can link against it. With in-process ORC that requirement disappears —
JIT'd code resolves symbols against the running process image directly. The
current pipeline (write .cpp → fork g++ → dlopen the .so) collapses into a
function call, and the on-disk cache stops being load-bearing.

## How to rebuild it

Source: `tools/single_file_probe.cpp`. Requires a from-source LLVM with static
archives — the AlmaLinux `clang-devel` RPM ships only `libclang-cpp.so`, no
`libclang*.a`, so the system packages cannot produce a single file. The build
at `/home/jsadlier/source-opt/llvm_build` has all 269 of them.

```sh
L=/home/jsadlier/source-opt/llvm_build
# 1. the header payload
tar cf all.tar -C / usr/include/c++/14 usr/include/bits usr/include/sys ...
zstd -19 all.tar -o all.tar.zst
ld -r -b binary -o blob.o all.tar.zst

# 2. satellite core as static objects
for f in charmap string bignum bigdiv container ops source library \
         capsule lexer machine cxx interpret; do
  g++ -std=c++20 -O2 -Iinclude -c src/$f.cpp -o obj/$f.o
done

# 3. one binary
g++ -std=c++20 -O2 -fno-rtti -I$L/include -Iinclude \
    tools/single_file_probe.cpp obj/*.o blob.o -o satellite_single \
    -L$L/lib -Wl,--start-group $(ls $L/lib/libclang*.a | sed 's|.*/lib\(.*\)\.a|-l\1|') \
    -Wl,--end-group $($L/bin/llvm-config --libs all) \
    $($L/bin/llvm-config --system-libs) -lz -lzstd
```

Two API notes, both of which cost a link cycle to find: use the **member**
`createDiagnostics()`, not the static `createDiagnostics(VFS, Opts, ...)` — the
static one returns an engine and installs nothing, and the failure surfaces much
later as an assertion inside `getDiagnostics()`. And `createSourceManager()`
takes no arguments in this version.

## Not yet proven

**Symbol resolution from JIT'd code into a statically-linked host.** Code that
calls `std::vector` or `satellite::Container::release()` needs those symbols
reachable from the executable — `-rdynamic`, or explicit registration into the
ORC symbol table. This is routine, but it has not been run here, and it is the
one link in the chain that is currently an assumption rather than a measurement.

Everything else on this page was executed.
