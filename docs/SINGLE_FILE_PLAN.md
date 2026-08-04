# One file, nothing installed

The goal: `satellite` is a single executable. A person downloads it, runs
`satellite run hello.satl`, and a `satellite.cxx { }` block compiles and runs
C++ — on a machine with **no compiler, no C++ headers, and no satellite
install**.

This document is the plan to get there. It is written to be picked up cold in a
new session.

---

## Where we already are

Done and verified:

- **No g++.** `env PATH=/nonexistent satellite run examples/cxx_args.satl`
  prints `hello, world!` three times and returns `7.5`. Blocks compile through
  embedded clang and run through ORC. No fork, no `.so`, no `dlopen`.
- **JIT'd code can call back into the host.** `Container::str`, `to_container`,
  the dispatch tables — all resolve against the running process, registered
  explicitly with `DynamicLibrarySearchGenerator::GetForCurrentProcess`.
- **One binary, two faces.** No arguments opens the GTK4 console; arguments are
  a CLI.
- **The compiler is swappable and tunable.** `SATELLITE_CXX_ENGINE=auto|fork|jit`
  plus eleven other settings, all in the cache key.

## What is still on disk

Run `ldd build/satellite`. Everything below has to go, or be shown to be
already present everywhere.

| Dependency | Why it is there | Fix |
|---|---|---|
| C++ standard headers | a block does `#include <iostream>` and clang reads it off the filesystem | embed the blob (step 1) |
| `libsatellite_core.so` | the library is built `SHARED` and found by rpath **into the build tree** | link it statically (step 2) |
| `libstdc++.so.6` | resolved through `LD_LIBRARY_PATH` to `/~/source-opt/gcc/opt/lib64` — a directory literally named `~`, from a bad install prefix | `-static-libstdc++ -static-libgcc` (step 2) |
| GTK4 and ~50 more | the console | decide (step 3) |
| glibc | every Linux has one | leave dynamic |

The `/~/` path is worth staring at: the shipped binary currently will not start
on any machine but this one, headers or no headers.

---

## The decision that is already made

**libstdc++, not libc++.** Not a limitation, and it costs the end user nothing
— it only says which headers get embedded.

The reason is ABI. `cxx.hpp` passes `std::string` between a block and
`satellite_core`:

```cpp
inline void from_container(const Container& c, std::string* out);
inline Container to_container(const std::string& v);
```

libstdc++ spells that type `std::__cxx11::basic_string`; libc++ spells it
`std::__1::basic_string`. Different layouts, same name, no diagnostic — the
block would write one shape and the host would read another.

Measured on this machine: `libLLVMSupport.a` has **234** `__cxx11` symbols and
**zero** `_ZNSt3__1`. Every LLVM and clang archive satellite links is
libstdc++. `libc++.a` exists here, but using it would mean rebuilding LLVM,
clang and satellite against it for no gain.

---

## Step 1 — embed the headers

**Status: written, not yet wired in.**

Already committed:

- `tools/pack_headers.cpp` — packs a list of files into one blob
- `cmake/HeaderBlob.cmake` — probe → dependency scan → pack → `ld -r -b binary`
- `cmake/DepsToList.cmake` — turns `clang -M` output into a path list

The measurement that makes this easy: `/usr/include` is **351 MB**, but the
reachable closure of 46 common standard headers is **423 files, 7.3 MB**. Embed
what is reachable, not what is installed.

Remaining work:

1. Call `satellite_build_header_blob()` from `CMakeLists.txt`, add
   `pack_headers` as a host tool target, and add the resulting `.o` to the
   `satellite` executable.
2. In `src/jit/compile.cpp`, replace the plain `InMemoryFileSystem` with one
   built **once** from the blob and cached. Building it per compile would re-add
   423 files on every block.

   ```cpp
   extern "C" const char _binary_headers_blob_start[];
   extern "C" const char _binary_headers_blob_end[];
   ```

   Walk the records and `addFile` each with
   `MemoryBuffer::getMemBuffer(StringRef, name, /*RequiresNullTerminator=*/true)`.
   The blob already stores a trailing NUL per file precisely so this is
   zero-copy — no 7 MB memcpy at startup.

3. **Mount at the original absolute paths.** clang's driver computes its own
   include search list by probing for a GCC install; if the files answer at the
   paths it already expects, that list keeps working and there are no flags to
   keep in sync. The VFS answers instead of the disk.

4. Prove it. Keeping the real filesystem underneath means we cannot tell
   whether the blob is doing anything, so add a memory-only mode
   (`SATELLITE_CXX_SEALED=1`) that drops the real FS from the overlay. The test
   is the **negative control**: memory-only must succeed, and a build with the
   blob removed must fail. Both, or the test proves nothing.

## Step 2 — cut the shared libraries

1. `satellite_core` → `STATIC`. It is `SHARED` today only so forked g++ modules
   could link against it; the JIT path resolves symbols from the process
   instead, so this only matters when `SATELLITE_CXX_ENGINE=fork`. Keep
   building the `.so` **as well** for that path.
2. Link the executable with `-static-libstdc++ -static-libgcc`.
3. Add `-rdynamic` (`ENABLE_EXPORTS`). This is load-bearing and easy to miss:
   once `satellite_core` is inside the executable rather than in a `.so`, its
   symbols reach `.dynsym` **only** with `-rdynamic`, and without them the JIT
   silently loses the ability to call home. There is a test for this already —
   the `Container::str` round trip through JIT'd code.
4. Re-verify with `ldd`: nothing but glibc, and GTK if the console is in.

## Step 3 — the console question

GTK4 pulls ~50 shared libraries. It cannot be statically linked in any sane
way. Three options, and this one is a judgement call rather than a technical
finding:

- **Two binaries** — `satellite` (CLI, ships anywhere) and `satellite-console`
  (needs GTK). Cleanest, and the CLI is what "single file" is really about.
- **`dlopen` GTK at runtime** — one binary that grows a console when GTK is
  present. Keeps the promise literally true. More machinery.
- **Accept the dependency** — most desktop Linux has GTK4; a server does not.

Recommendation: **two binaries.** The pitch is "download one file and run
C++" — that file is the CLI, and nothing about it needs a window.

## Step 4 — prove it somewhere else

A claim that the binary is self-contained is only worth what it was tested on.

- `docker run --rm -v ...:/s alpine:latest /s/satellite run /s/hello.satl`
  — different libc, so this will fail; that is fine and worth knowing.
- A minimal `debian:slim` container with no `build-essential` is the real
  target: glibc present, no compiler, no C++ headers.
- Add it to `.ship/001/verify.sh`.

---

## Order, and why

1. **Step 1** first — it is the actual feature, and it is testable on its own.
2. **Step 2** second — mechanical, but `-rdynamic` can break the JIT silently,
   so it wants the step 1 tests already passing underneath it.
3. **Step 3** is a decision, not a blocker; the CLI can ship while it is open.
4. **Step 4** is what turns "should work" into "does work".

## What not to do

- Do not embed all of `/usr/include`. 351 MB, and almost none of it reachable.
- Do not switch to libc++ without rebuilding LLVM and clang to match. See above.
- Do not drop the fork/g++ engine. It is the fallback for a build with no static
  LLVM, and its on-disk cache is genuinely faster on a warm run: **0.1 ms**
  against the JIT's ~900 ms, because JIT'd code dies with the process.
- Do not claim zero setup in the README until step 4 passes. The README is
  currently honest; a promise the binary does not keep is worse than no README.
