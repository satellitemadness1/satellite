# view_forge runs — 2026-08-24

Written so this survives a `/clear`. All of it is in the working tree, built
clean with clang 24 at -O2.

## The headline

`/home/madness/code/satl/view_forge` (40 .satl files, 13,251 lines) RUNS END TO
END. `echo CHINA | satl view_forge_main.satl` → **0 errors, 582,810 lines of
output, 501 objects loaded, 501 forged, 43 types.** It died on line 3 before.

`plans/missing.txt` was the gap list. Every one of its ten entries is now
CLOSED except #9. Nine were closed by changing the LANGUAGE, not the program.

## What changed in the language

1. **`satellite.include` takes a quoted path** — `satellite.include("object/forge_object.satl")`.
   `src/spaceship_loader/loader.{cpp,hpp}`: a new `Target::Path` kind and
   `path_candidates()`. Resolved RELATIVE TO THE INCLUDING SPACESHIP (not the
   cwd), so running the program from another directory cannot change what it
   means. `..` works; absolute works; the `.satl` extension may be left off;
   include-once still holds across two spellings of one file because the key is
   the canonical path. The parser needed NO change — it already parsed a string
   there; only `classify()` refused it. §1 is not weakened: a bare word is still
   a name the user owns, and a string literal was never a name.

2. **List literals** — `{1, 2, 3}`, `{}`, nested, names and expressions as
   elements. 2,774 sites in view_forge. New `ListLit` AST node appended to
   `ExprBase` (ast.hpp/cpp, syntax_parser/expr.cpp, environment/walk.cpp,
   evaluator/expr.cpp). Left-to-right evaluation, built-then-frozen like every
   other list. The ELEMENT TYPE is deliberately not part of the literal.

3. **A redeclaration rebinds instead of erroring** (`src/environment/scopes.cpp`).
   341 + 43 sites. **THE FRESH SLOT IS THE CORRECTNESS ARGUMENT** — the obvious
   implementation (reuse the old slot) would have been silently wrong, because
   §14 makes a spacesuit a reference type, so 501 objects built through one name
   would have read back as 501 copies of the last one with no error anywhere.
   Verified: the 501 forged objects carry 498 distinct names. Tested in
   env_test (slot_count == 2, and the read sees the newer binding).

4. **`satellite.statement.else()`** — an empty pair of parens accepted and
   dropped. `else(x)` is still an error. 24 sites.

5. **Bare `TRUE` / `FALSE`** — accepted, but ONLY after every scope the user
   owns has been asked and said no (`environment/names.cpp` →
   `evaluator/slots.cpp`). So §1 survives: a variable, field, capsule or
   spacesuit named TRUE still wins, and there is a test for exactly that.
   `satellite.bool.true` remains canonical. Doing this in the LEXER is the
   obvious implementation and is what would have broken §1.

6. **Named arguments as grammar, `end=` as the only one understood** — new
   `NamedArg` AST node, same pattern as `DurationLit`: parses anywhere, accepted
   in one place. `satellite.console.display("x", end="")` prints without a
   newline; `end` takes any string. Anywhere else, one message names the whole
   rule instead of a parse error about a missing `)`.

7. **`satellite.console.input`** — three shapes: `input()`, `input(prompt)`
   returning the line, and `input(prompt, target)` writing into a variable.
   The two-argument form is THE ONLY OUT PARAMETER IN THE LANGUAGE and is not
   the start of a general facility. `read_input_line()` calls
   `Console::drain()` first — without it the prompt sits in the printer
   thread's queue and the read blocks on an apparently empty terminal. EOF is
   loud, because "empty line" and "nobody there" are different answers.

8. **`l[i] = x`** (`evaluator/mutators.cpp::assign_index`). Copy-and-replace
   through `update_through_slot`, so it inherits the existing write protocol
   and lock rather than inventing a second one. Same index rules as the read
   side: negative counts from the end, out of range is an error and never a
   grow. A map still refuses and still says `.set(key, value)`.

10. **`\'` accepted as a redundant spelling of `'`** (satellite_string.cpp,
    plus `SAT_APOSTROPHE` and its static_assert). This was printing a stray
    backslash 501 times in view_forge's real output — silent, no error. Now 0.

## Still open

- **missing.txt #9: `string + bool` is still refused.** `"flag=" + b` →
  `+ does not apply to flag= and true`. 0 sites today; string+number works.
  The asymmetry is untouched. THIS IS THE ONLY ONE OF THE TEN STILL OPEN.

## Closed since this file was written (2026-08-24, later the same day)

- **`make test` is GREEN.** Seventeen binaries, ThreadSanitizer included. The
  five eval_test failures were one cause -- a list echoes with `.lines()` now,
  one element per line, and those five still spelled `[a, b]`. The behaviour was
  confirmed intended ("one per line, always"), so the expectations were
  rewritten and no code changed. Two facts recorded in the test while doing it:
  an EMPTY list still prints `[]`, because zero lines cannot be told from a
  broken echo; and a MAP still echoes as one value, so `.keys()` prints as a
  listing and the map does not. Reading every file in a directory to get its
  line count is accepted as the cost of a listing.

- **TSAN works under clang, through a new `TSAN_CXX`.** The earlier note below
  was right that a full LLVM rebuild was not needed for GCC's libtsan, but it
  understated the problem: `make test` ABORTED before running anything, because
  the clang at `$(LLVM_BIN)` has no compiler-rt at all. The Makefile now probes
  by LINKING an empty `main` with `-fsanitize=thread` and uses `$(CXX)` if that
  works, falling back to `/usr/bin/clang++` (21.1.8). Do NOT probe with
  `-print-file-name=libtsan.so`: clang answers with GCC's 38-byte linker script
  because it searches GCC's directories, so the lookup says yes for the one
  compiler that cannot do it. When a clang built WITH
  `-DLLVM_ENABLE_RUNTIMES="compiler-rt;..."` is installed at `$(LLVM_BIN)`,
  `TSAN_CXX` resolves back to it on its own with no edit. compiler-rt belongs in
  `LLVM_ENABLE_RUNTIMES`, not `LLVM_ENABLE_PROJECTS` -- in PROJECTS it is built
  by the host compiler rather than the just-built clang.

- **The design is no longer one file.** `DESIGN.md` is an 80-line index and the
  nineteen sections live under `design/`, one section per file, each file
  numbered by its section -- `§14` is `design/14-spacesuits.md`. Section bodies
  moved byte for byte; nothing was renumbered, so all twenty `§N` citations
  under `src/`, the man page, `install.sh`, `debian/` and `plans/` still resolve.
  The thirty-five LINE citations under `plans/` (nearly all in
  `pcg_k16384_spec.md`) do not, and cannot be repaired.

- **Everything above is COMMITTED.** Five commits, working tree clean, 20
  commits ahead of `origin/main` and still never pushed. The nine language
  additions could not be their own commit: the `src/` move renames the same
  files they edit, so splitting them would have made the first commit delete
  `loader.cpp` without adding `src/spaceship_loader/loader.cpp`, and that commit
  would not build.

- **lld is available and unused.** `~/opt/clang-24/bin/ld.lld` works;
  `LDFLAGS` is empty by design so the build links with GNU ld 2.46 from
  `~/opt/binutils-246`. Measured on satl's link: 143 ms with GNU ld, 76 ms with
  `-fuse-ld=lld`. Not adopted -- `LDFLAGS` is deliberately the environment's to
  set (Makefile:111).

## The other programs

- `data_creator/creator_001` — RUNS CLEAN now (it was blocked only on the
  quoted include path).
- `line_creator` — still 4 errors, and all four are **typos in that program**,
  not language gaps: a stray `)` at line 326, and `"WHITE_VICTOR"})` with an
  extra `}` before the `)` at 375, 470 and 500. One character each.
- `print_test/print_test.satl` (in the satellite repo) — needs bare
  `while (...)` (the language wants `satellite.statement.while`) and
  `satellite.variable.thread` / `satellite.thread.new()`, which do not exist.

## Change to view_forge itself (requested)

`view_forge_main.satl` forged exactly ONE object — whichever matched the typed
target. Added a pass at the end of `satellite.main` that walks `object_list`
and hands EVERY object through `local_view_forge.call_forge_object()`, in the
program's own narrating style. That is what forges all 501. Backup of the
original: scratchpad/view_forge_main.satl.bak.
