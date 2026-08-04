# Working on satellite

## Start here

**`docs/SINGLE_FILE_PLAN.md`** is the current work. It is written to be picked up
cold: what is done, what is left, the order, and a "what not to do" section so
settled questions are not reopened.

## Build and test

```sh
cmake -S . -B build && cmake --build build -j8
cd build && ctest            # 13 suites, 1343 checks
```

Every suite prints `N checks, M failures` on its last line. `M` must be 0.

## Conventions

- **~400 lines per C++ file.** At the limit, the file gets a *directory* and is
  split into 3-5 files — not trimmed. See `src/ops/`, `src/cxx/`, `src/jit/`.
  Each has a private `<name>_internal.hpp` for shared helpers and declarations.
- **Binary size and compile time are not constraints.** Runtime speed is the
  only target. The binary is ~107 MB on purpose.
- **Comments explain why, not what** — especially the non-obvious call, the
  thing that looks wrong until you know the reason, and the bug that a
  particular line is there to prevent.
- **Anything under `include/` is preprocessed by every satellite.cxx block at
  runtime.** Keep it lean, and keep LLVM types out of it. Implementation-only
  headers live in `src/` (`big_limbs.hpp`, `capture_out.hpp`, `*_internal.hpp`).
- **Measure before claiming.** A benchmark on this machine has already
  overturned a recommendation made from first principles.

## Version 001 is preliminary

There is **no parser and no virtual machine**. A `.satl` file is lexed and
validated, and its `satellite.cxx { }` blocks execute — that path is complete
end to end. Ordinary satellite statements do not run, and no output should ever
imply that they did.

`satellite.include(satellite)` is **checked for, not implemented**.

## Two compilers

`SATELLITE_CXX_ENGINE=auto|fork|jit`, and `satellite cxx-config` prints every
setting with the variable that overrides it.

- **jit** — clang linked into the process, straight to executable memory. Needs
  nothing installed. No cache: ~900 ms every run.
- **fork** — writes a `.cpp`, runs g++, `dlopen`s the `.so`. Cached by content
  hash: ~1700 ms cold, **0.1 ms** warm.

Both compile *identical generated source* and use the same Container ABI, so a
block cannot tell which ran it. Keep it that way.

Any setting that changes generated code **must** be in the cache key — the cache
is content-addressed, so a setting left out of it silently does nothing on every
run after the first.
