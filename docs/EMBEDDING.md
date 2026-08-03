# Baking the C++ toolchain into the satellite binary

Assessment produced by a 12-agent measurement workflow on this machine
(AlmaLinux 10, GCC 17.0.0 experimental, Clang/LLVM 21.1.8 from RPM plus a
source build of LLVM 24.0.0git at `/home/jsadlier/source-opt/llvm_build`).

Every number below was measured, not estimated. Claims marked **Proven** had a
prototype built and run; the confidence table at the end says exactly which is
which.

## Verdict

Yes, technically â and the header side is far cheaper than anyone guessed. But the cost is not in the headers. It is in the compiler.

The headers you need are **3.40 MB compressed** (33.05 MB raw), which is a non-issue. The embedded Clang that consumes them is **87.55 MB** and â the load-bearing finding â **cannot be built from any package available on this machine.** AlmaLinux 10 ships zero `libclang*.a`; `dnf list --available clang-static` returns "No matching Packages to list." A self-contained `satellite` requires building LLVM from source as a prerequisite of building satellite. That is a different project than the one this question sounds like.

Recommendation: **do it, but stage it.** Ship raw compressed headers over a VFS first (small, proven, useful on its own), and treat "embed Clang" as a separate funded decision with a source-built-LLVM bootstrap attached.

## The real numbers

Current `libsatellite_core.so` is **152,176 bytes**. Everything below is a delta against that.

### Compiler

| Approach | Bytes | Self-contained? |
|---|---:|---|
| Shared: `libLLVM.so.21.1` + `libclang-cpp.so.21.1` | 191,029,184 (191.0 MB) | **No** â requires LLVM 21 on the user's machine |
| Static, all targets, stripped | 87,550,464 (87.55 MB) | Yes |
| Static, reduced backend set, stripped | 44,171,704 (44.17 MB) | Yes, *unvalidated* |

**Corrected number:** the working estimate of "~130 MB" was wrong in both directions. It appears to be `libLLVM.so` alone (124,868,888 B), forgetting that Clang's frontend is **not** inside libLLVM â `libclang-cpp.so.21.1` is another 66,160,296 B, so shared is 191.0 MB, not 130. Static is *better* than the estimate at 87.55 MB, because the linker garbage-collects 351 MB of `libLLVM*.a` archives down to what is actually reachable. Both figures are exact `ls -l`, not estimates.

The 44.17 MB reduced link was produced and measured but **I did not verify it emits working x86-64 object code.** Treat 87.55 MB as the number to plan against; treat 44.17 MB as an optimization to attempt later, not a budget.

### Headers, raw

The "batteries-included" set â every header a user could plausibly `#include` from a `satellite.cxx` block:

| Component | Raw bytes | Files |
|---|---:|---:|
| libstdc++ (GCC 17.0.0, the active tree) | 14,756,823 | 874 |
| Clang resource headers (intrinsics, stdarg, etc.) | 8,792,734 | 317 |
| Kernel headers (`linux/`, `asm/`) | 7,073,192 | 1,034 |
| glibc C headers | 2,431,594 | 502 |
| **Total** | **33,054,343 (33.05 MB)** | 2,727 |

The "179 MB of headers" figure that started this investigation is an artifact of measuring `/usr/include` as a whole. `/usr/include` is 159,291,820 B, but it is dominated by packages irrelevant to a C++ escape hatch: llvm 33.4 MB, clang 30.5 MB, vulkan 18.9 MB, rocm/hip ~15 MB, plus a second nested libstdc++ (`/usr/include/c++/14`, 13.3 MB) that is not even on the active `g++` search path. Actual glibc-owned C headers are 2.31 MB.

You may not even need the full tree. The transitive closure for the *real* satellite `cxx.cpp` case (`satellite/cxx.hpp` + `<string>` + `<vector>` + `<exception>`) is **204 files / 2,534,691 bytes**. The union of four representative test programs is **289 files / 4,490,480 bytes**. Independently reproduced: `<string>` alone is 152 files / 1,956,496 B; `<vector>+<string>+<algorithm>` is 167 files / 2,706,682 B; a generous 7-header program is 277 files / 4,398,850 B. Byte totals cross-checked two ways (`stat` sum vs `du -cb`), symlinks resolved through `realpath` with no change in counts.

### Headers, compressed (zstd -19)

| Component | zstd -19 | Ratio |
|---|---:|---:|
| libstdc++ full tree | 1,555,147 | 9.94x |
| Clang resource headers | 589,341 | ~14.9x |
| Kernel headers | 844,168 | 8.38x |
| glibc headers | 415,729 | 7.32x |
| **Batteries-included total** | **3,404,385 (3.40 MB)** | **9.71x** |
| *(alternative)* 289-file union closure only | 534,320 | 8.83x |

zstd -19 is the right codec and the ranking is decisive. On the full libstdc++ tree, xz -9 is 1,494,844 B â only 3.9% smaller than zstd â but decompresses **4.5x slower** (106.69 ms vs 23.60 ms; 141.6 vs 715.2 MiB/s). gzip -9 is both 46% larger *and* slower (2,272,414 B, 166.0 MiB/s) â dominated on both axes, no remaining niche. All three round-tripped to identical SHA-256. Decompressing the 289-file union at runtime is below timer resolution (<10 ms wall including process spawn).

### Bottom line

```
static Clang (all targets)   87,550,464
batteries-included headers    3,404,385   (zstd -19, decompressed to VFS at runtime)
satellite core                  152,176
                             -----------
                             91,107,025 bytes = 91.1 MB
```

With the reduced backend link, if it validates: **47.7 MB**. The headers are 3.7% of the total either way. **Stop optimizing headers; the entire question is Clang.**

## Is the VFS mechanism proven?

**Proven.** A prototype was built and *ran* â `vfs_proto`, exit code 0, against system LLVM/Clang 21.1.8 (`libclang-cpp.so.21.1` + `libLLVM.so.21.1`, confirmed via `ldd`). This is not a design sketch.

It proves the crux in four escalating steps:

1. **Negative control.** Compiling `/virtual/main.cpp` over a bare `vfs::getRealFileSystem()` fails with `error reading '/virtual/main.cpp': No such file or directory`, and `/virtual` verifiably does not exist on disk. Rules out the whole test being files that happened to exist.
2. **The mechanism.** `vfs::OverlayFileSystem(getRealFileSystem())` + `pushOverlay(InMemoryFileSystem)`, wired in via `CompilerInstance::createFileManager(Overlay)`. `-fsyntax-only` succeeds against an in-memory header.
3. **Full codegen, not just parsing.** Same overlay, `-O1 -emit-llvm`. The emitted IR for `compute()` (defined as `magic() + 1`, where the in-memory header says `return 42`) ends in `ret i32 43`. That constant cannot exist unless Sema and CodeGen actually consumed the in-memory header. This is the load-bearing evidence â not the exit status.
4. **HeaderSearch works through the VFS.** A second in-memory TU resolves both `#include "../myheader.h"` (quoted, relative to includer) and `#include <myheader.h>` (angle, via `-I/virtual`, a directory existing only in memory). IR contains `ret i32 142`. You are **not** restricted to absolute-path includes; Clang's normal header search machinery operates over the VFS.

Independently, a full stdlib served entirely from `InMemoryFileSystem` with `-nostdinc++ -isystem /vstd` compiles, and a self-contained blob built with `-Xclang -fmodules-embed-all-files` compiles from a pure in-memory FS with **strace-confirmed zero opens** under any header directory.

**What is not proven:** the prototype ran against *shared* LLVM 21. Nobody has run the same prototype against a *statically linked* Clang. The VFS API is the same, but static-link initialization order and target registration are a different code path.

## The static-vs-shared problem â read this before planning anything

This is the item that changes the project's size class.

- `llvm-static` **is** installed: 154 archives, 351,267,882 bytes of `libLLVM*.a`.
- `clang-static` **does not exist in any configured repo**.
- `libclang*.a` count in `/usr/lib64/llvm21/lib64`: **zero**. The 156 `.a` entries there are symlinks into `/usr/lib64/`; all resolve, none are Clang.

This combination is actively deceptive: half the static story is present, so a naive static link gets far enough to look plausible before dying on the Clang half. The 87.55 MB static binary was produced **only** against a pre-existing source build at `/home/jsadlier/source-opt/llvm_build` (LLVM 24.0.0git, 50 `libclang*.a`, 269 archives / 512 MB total). It exists on this machine because someone built it by hand.

Consequences, stated plainly:

- "Embed Clang" means **"build LLVM from source first."** Not a link-flag change. A multi-hour build (24 cores here; unmeasured on this machine) and a hard new bootstrap dependency for every satellite contributor and every CI runner.
- The shared-library path is a non-answer. 191 MB of `.so` that must already be present on the user's machine is strictly worse than the status quo of "must have g++ installed."
- Four LLVM installations exist on this box (RPM 21, source-built 24, a clang resource dir at LLVM 24, plus GCC 17 at the literal path `/~/source-opt/gcc`) and the toolchain picks them up in surprising order. `clang` first on `PATH` is the source build reporting 24.0.0git, not `/usr/bin/clang-21`. Any build script must pin absolute paths. Note also that `/~` is a **real directory named `~` at filesystem root**, not a shell tilde â it must be quoted in every command that touches the GCC 17 tree.

## Raw headers vs PCH: ship raw headers

Not close. Two independent experiments at different scales and toolchains agree in direction.

| | Raw headers | PCH | PCH penalty |
|---|---:|---:|---|
| string/vector/algorithm (172 files) | 2,565,177 B | 5,991,384 B | 2.34x larger |
| ...compressed (xz -9e) | 289,512 B | 2,508,680 B | **8.7x larger** |
| 27 common headers (398 files) | 7,166,808 B | 24,221,144 B | 3.4x larger |
| ...compressed (xz -9) | 818,240 B | 10,396,588 B | **12.7x larger** |
| union closure (319 files) | 5,027,840 B tar | 12,124,064 B | 2.4x larger |
| ...compressed (zstd) | 565,776 B | 5,870,474 B | **10.4x larger** |

The gap *widens* under compression, and that is structural, not incidental: header text is highly redundant and compresses ~9.3x; a PCH is a dense binary hash-table/offset format and compresses only ~2.4x. Since the payload ships compressed, the compressed column is the one that counts. A PCH costs you **10x the shipped bytes.**

PCH's advantage is real but it is speed, not size: `-fsyntax-only` 511 ms â 141 ms (3.6x), full `-c` 571 ms â 199 ms (2.9x), kitchen-sink `-c` 2,414 ms â 793 ms (3.0x). A Clang header module (`.pcm`) is better still â 159 ms at kitchen-sink scale, 15x faster than raw headers â because a header unit deserializes only the decls actually named, while `-include-pch` materializes the whole TU context.

Two further findings that kill PCH-as-payload:

1. **A default PCH/PCM is not self-contained.** It stores absolute paths and re-opens 9 of the original headers at load. Move the headers away and it dies with `malformed or corrupted precompiled file`. Against a pure `InMemoryFileSystem` it fails with `could not find file ... referenced by AST file`, and setting `PreprocessorOptions::DisablePCHOrModuleValidation = DisableValidationForModuleKind::PCH` does **not** lift the requirement â the AST records and validates every original header's path, size, and mtime.
2. Making it self-contained requires `-Xclang -fmodules-embed-all-files`, which works (verified: 13,418,476 B blob, strace-confirmed zero header-directory opens) but by definition re-embeds the header text *inside* the PCH â you now ship the headers twice, badly compressed.

**Decision: raw headers in the shipped payload.** If startup latency later proves to be the user-visible complaint, generate a PCH or `.pcm` *on the user's machine at first run*, cache it next to the existing `~/.satellite/cache`, and keep it out of the binary. You get the 3xâ15x speedup without the 10x shipped-size penalty.

## Implementation plan

Sized honestly. Phases 1â3 are independently useful and can ship without ever touching LLVM.

**Phase 1 â Embed and mount the headers (small; ~1â2 days).**
Build-time: tar the four header sets (`--sort=name --owner=0 --group=0 --numeric-owner --mtime='UTC 2020-01-01'` for reproducibility), zstd -19, embed as a byte array. Runtime: decompress into memory (<10 ms measured), populate a `clang::vfs::InMemoryFileSystem` at canonical paths. Payload: 3,404,385 B. Start with the 289-file union (534,320 B) if you want to prove the pipeline before committing the full tree. Note that entries need plausible mtimes/sizes even for raw-header use â Clang stats them.

**Phase 2 â Route the existing compiler through the VFS (small; ~2â3 days).**
`src/cxx.cpp` currently shells out (`compiler = "g++"`, `-O2 -std=c++20`, dlopen). Replace the subprocess with an in-process `CompilerInstance` linked against *shared* LLVM 21 first â that is exactly the configuration `vfs_proto` already proved. Pass `-nostdinc -nostdinc++` plus `-isystem` for each embedded root, so nothing on the user's disk is consulted. Validate with strace: zero opens outside your virtual roots. This phase is where correctness gets settled, cheaply, before the static-link problem is anywhere near the critical path.

**Phase 3 â Decide the linking strategy (medium; unmeasured, see risks).**
Compiling to an object file is proven. Turning that object into something callable is **not measured at all**. Two candidate paths, and you must pick one before Phase 4 because it changes what you statically link:
- *ORC JIT* â no external linker, no ELF `.so`, no crt files, no `dlopen`. Almost certainly the right answer for a self-contained binary. Adds ORC/JITLink to the static link.
- *Embed LLD as a library* â produces a real `.so`, preserving the current dlopen model and the existing cache design, but requires crt startup objects and `libstdc++.so.6`/`libc.so.6` on the target.
Prototype both against shared LLVM. Budget a week; this is the phase most likely to surprise.

**Phase 4 â Source-build LLVM and link statically (large; the real cost).**
Pin an LLVM release. Script the build (`LLVM_ENABLE_PROJECTS=clang`, `LLVM_TARGETS_TO_BUILD=X86`, `LLVM_ENABLE_ASSERTIONS=OFF`, Release). Wire it into CMake as an external prerequisite with a clear error when absent. Re-run every Phase 2/3 test against the static link â this is the untested transition. Then attempt the reduced-backend link and confirm whether 44.17 MB actually holds up. Multi-day at minimum, and it permanently changes satellite's build story from "cmake -S . -B build" to "build LLVM first."

**Phase 5 â Fall back gracefully.**
Keep the `g++` subprocess path alive behind `CxxConfig`. If the embedded toolchain is not compiled in, `satellite.cxx` should still work exactly as it does today. This is cheap and it de-risks all of the above.

## What could still go wrong

**Unmeasured â linking.** The single biggest gap. Every experiment stopped at "produces an object file" or "produces IR with the right constant in it." Nothing measured `.o` â callable code. If the answer turns out to be "embed LLD," you also inherit crt startup objects and a dependency on the user's `libstdc++.so.6`.

**Runtime/header version skew.** You would bake GCC 17.0.0 *experimental* libstdc++ headers and then link the compiled code against whatever `libstdc++.so.6` is on the user's machine. Headers from a compiler newer than the runtime will reference `GLIBCXX_3.4.34`-class symbol versions that an older runtime does not export â a link or load failure on the user's machine that never reproduces on yours. Not measured. Mitigations: bake a *released* GCC's headers rather than trunk; or switch to **libc++**, which pairs naturally with an embedded Clang and can be statically linked so there is no external runtime at all. libc++ was not measured on this machine.

**Licensing.** libstdc++ headers are GPLv3-with-Runtime-Library-Exception; glibc headers are LGPL; LLVM is Apache-2.0-with-LLVM-exception. Embedding and redistributing them inside a binary has obligations. This is unresolved and I am not the right source for the answer, but it is a real gating item and libc++ makes it materially simpler.

**"Batteries-included" is still finite.** 33.05 MB covers libstdc++ + glibc + kernel + Clang builtins. It does not cover Boost, Eigen, TBB, or anything else a user might reasonably reach for from a C++ escape hatch. Decide now whether `satellite.cxx` promises "the standard library, always" or "whatever the user has." The former is achievable; the latter needs a disk fallback anyway.

**Static-link initialization.** Target registration, command-line option registration, and static-initializer ordering behave differently in a statically linked Clang than in the shared build the prototype used. Expect to spend real time here.

**Build-story blast radius.** Requiring a source-built LLVM affects every contributor, every CI runner, and every packaging target. This is the cost people underestimate â not the 87 MB.

**Environment hazards on this machine specifically.** Four LLVM installs with surprising `PATH` precedence; two libstdc++ trees where the *inactive* one (`/usr/include/c++/14`) is nested inside `/usr/include` and silently double-counts in naive measurements; and the literal `/~` directory that breaks any unquoted shell command. Every build script must use pinned absolute paths and quote `/~`.

## Confidence

| Claim | Status |
|---|---|
| VFS-served headers compile and codegen correctly | **Proven** â binary ran, `ret i32 43` in emitted IR |
| Header search (quoted + angle) works over VFS | **Proven** â `ret i32 142` |
| Header payload is 3.40 MB compressed | **Measured**, independently reproduced |
| Closure sizes (152/167/204/277/289 files) | **Measured**, reproduced byte-for-byte, dep lists diff-identical |
| Closure is genuinely self-sufficient | **Proven** â materialized in isolation, compiled `-nostdinc`, binary ran and returned the correct answer (27) |
| Raw headers beat PCH by ~10x compressed | **Measured** at two scales, two toolchains |
| Static Clang = 87.55 MB | **Measured** (`ls -l` on a real stripped binary) |
| `clang-static` unavailable from packages | **Verified** â `dnf` returns no packages; zero `libclang*.a` |
| Reduced static link = 44.17 MB | Measured size; **codegen correctness unverified** |
| Static-linked Clang works with the VFS | **Untested** |
| Object â callable code | **Not measured at all** |
| Header/runtime ABI skew on foreign machines | **Not measured** |
| LLVM source build wall-clock | **Not measured** |
