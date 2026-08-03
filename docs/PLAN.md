# Planned work

Two projects are mapped out but not started. Both have everything they need
already installed on this machine.

---

# 1. `satellite.statement.parallel_for` on oneTBB

**Status:** not started. `tbb-devel 2021.11.0` installed, `pkg-config --modversion tbb` → 2023.1.0.

## Syntax

```
satellite.statement.parallel_for(i, 0, 1000)
{
    my_list[i] = my_list[i] * 2
}
```

The body is **satellite code**, not C++ — unlike `satellite.cxx`, it gets lexed
and compiled normally, then executed once per index across TBB's workers.

## Why TBB is the right substrate

The threading benchmarks in the README landed on three findings, and TBB
answers all three without satellite having to:

- **Work-stealing, sized to the hardware.** You never choose a thread count;
  TBB does, and idle workers steal from busy ones. That sidesteps the whole
  "how many threads" question the benchmarks could not settle.
- **Grain size.** `tbb::blocked_range` batches iterations into chunks large
  enough to amortise scheduling. The measured rule — never dispatch work
  smaller than ~100 interpreter steps — is enforced by the library.
- **Composability.** Nested `parallel_for` does not oversubscribe; TBB flattens
  it onto the same worker pool.

## Steps

1. **Link TBB.** `find_package(TBB)` in CMake; `-ltbb`.
2. **`src/parallel.cpp`** — a thin wrapper so the rest of satellite never
   includes TBB headers directly:
   ```cpp
   void parallel_range(int64_t begin, int64_t end,
                       const std::function<void(int64_t,int64_t)>& body);
   ```
   Implemented with `tbb::parallel_for(tbb::blocked_range<int64_t>(...))`.
   A `SATELLITE_NO_TBB` fallback runs it serially, so TBB stays optional.
3. **Lexer** — recognise `satellite.statement.parallel_for(...)` followed by a
   brace block. The body is ordinary satellite tokens; only the *statement
   form* is new.
4. **Parser / compiler** — emit the body as its own callable unit with the loop
   variable as a parameter.
5. **VM** — run it under `parallel_range`. Needs the VM to exist first.

## The hard part: shared state

`satellite.library` makes every variable reachable from anywhere. A parallel
body that writes one is a data race, and races are the single worst failure
mode to hand a user — silent, nondeterministic, unreproducible.

The compiler already resolves `satellite.library.x.y` to slot indices, so it
can classify every variable a parallel body touches:

| access | action |
|---|---|
| loop-local | free, no synchronisation |
| read-only outer | free |
| written outer, disjoint index (`list[i]`) | free — prove non-overlap from the index expression |
| written outer, shared | **reject at compile time**, or require an explicit reduction |

Rejecting is better than locking. A lock makes a race into a slow correct
program the user never notices; a compile error makes them say what they meant.
Provide `satellite.statement.parallel_reduce` for the accumulate case.

## First milestone

`parallel_range` plus a benchmark against a serial loop, before any language
syntax. That proves the substrate and gives a number to point at.

---

# 2. Embedding Clang/LLVM for self-contained `satellite.cxx`

**Status:** not started. `clang-devel` / `llvm-devel` 21.1.8 installed,
`libclang-cpp.so` and `libLLVM-21.so` present.

Today `satellite.cxx` shells out to the system `g++`. That works, is ~60 lines,
costs nothing in binary size (`libsatellite_core.so` is 149 KB), and requires a
toolchain on the user's machine.

## What embedding buys

- **No external compiler needed** — satellite ships its own.
- **In-process compilation** — no `fork`/`exec`, no temp `.cpp`/`.so` files.
- **Faster** — plausibly ~1.7 s → a few hundred ms for a small block, since the
  process and the AST for common headers can be reused across blocks.
- **JIT via ORC** — emit straight to memory, skip the shared library entirely.

## The caveat that decides the design

**Embedding Clang does not remove the need for standard library headers.** To
compile `#include <string>` the compiler must read that header from somewhere.
A 100 MB self-contained binary that still requires libstdc++-devel installed
would defeat the whole point.

Cling and `clang-repl` solve this by mounting the headers in an in-memory
virtual filesystem. So genuinely self-contained is:

```
libclang + libLLVM (static)     ~100 MB
libstdc++ headers in a VFS       ~30 MB
                                ------
                                ~130 MB
```

That is the real number. It is achievable — it has been done before — but it is
the actual scope, not 100 MB.

## Two builds, one codebase

Worth supporting both rather than choosing:

| build | binary | needs installed | how |
|---|---|---|---|
| `SATELLITE_CXX=gpp` | 149 KB | g++ or clang++ | current, works today |
| `SATELLITE_CXX=llvm-shared` | ~200 KB | llvm + clang packages | link `libclang-cpp.so` |
| `SATELLITE_CXX=llvm-static` | ~130 MB | nothing | embed + header VFS |

`cxx::Bridge` already has the right shape for this: `run()` and `build()` are
the whole interface, so a second backend slots in behind them without touching
anything else.

## Steps

1. **Backend interface** — extract `Bridge`'s compile step behind a
   `CxxBackend` interface. The g++ path becomes `GppBackend`.
2. **`LlvmBackend`, shared first.** `clang::CompilerInstance` +
   `EmitLLVMOnlyAction` + ORC `LLJIT`. Link `libclang-cpp.so`, `libLLVM-21.so`.
   Measure against the g++ path — if it is not meaningfully faster, the whole
   project is only about removing the dependency.
3. **Header VFS.** `llvm::vfs::InMemoryFileSystem`, populated from headers
   embedded as generated arrays. Biggest single chunk of work.
4. **Static link.** `llvm-config --link-static`, then measure the real binary.

## Risks

- **LLVM's C++ API is not stable across major versions.** Code written against
  21 may need work on 22. Pin the version and expect maintenance.
- **Build complexity** goes from "cmake" to "cmake plus a 130 MB link step".
- **Licensing** is fine — Apache 2.0 with the LLVM exception.

## Recommendation on sequencing

Do this **after the VM exists**. The g++ path already delivers the feature, and
`satellite.cxx` is a developer convenience; the VM is what makes satellite a
language rather than a value type with a lexer. Nothing about embedding Clang
gets harder by waiting, and the backend interface in step 1 is cheap to add now.

---

# Current state, for whoever picks this up

Built and tested: `satellite_container`, `satellite_string` with the charmap,
`BigInt`, the lexer, `satellite.machine` (uinput), `satellite.cxx` (g++), the
interpreter seam, and an optional GTK4 console (`satellite-gui`, built only
when `pkg-config` finds gtk4). 335 checks passing across five test binaries.

Not built: parser, compiler, VM, scheduler, `Type::Handle`. The GTK console
exists as a front end, but it can only drive what the seam can do -- it lexes,
validates and runs `satellite.cxx` blocks, and executes no satellite code,
because there is nothing behind the seam to execute it with.

The next piece on the critical path is the **parser**, then a single-pass
compiler to bytecode, then a register VM. Both projects above want the VM to
exist first.
