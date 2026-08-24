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
  The asymmetry is untouched.
- **`make test` has 5 pre-existing failures, NOT from this work.** All five are
  one root cause: the previous session made `.lines()` the default list echo,
  so eval_test's `[1, 2, 3]`-style expectations fail (the map one fails on
  `m.keys()`, which is a list). Confirmed pre-existing by mtime — helpers.cpp
  23:50 and session.cpp 23:05, versus 01:11+ for everything touched here.
  Decide whether `.lines()` really is the right default echo, then fix the
  expectations. Everything else PASSES, including loader_test, interp_test,
  spacesuit_test, env_test, parser_test, ast_test.
- **TSAN: you do not need to rebuild LLVM.** `~/opt/clang-24` was built without
  compiler-rt (`lib/clang/24/lib/` does not exist at all). GCC's libtsan 14.3.1
  IS installed, and `make CXX=g++ src/satellite_library/library_test_tsan`
  builds and **PASSES** — verified, including the new code paths. For clang's
  own TSAN you need compiler-rt only, not a full clang rebuild.
- **DESIGN.md now records all nine — DONE 2026-08-24.** New `## 19. Nine
  additions, and the program that asked for them` (19.1 include paths, 19.2 the
  list literal, 19.3 the rebind and the fresh-slot argument, 19.4 index
  assignment, 19.5 console.input and the only out parameter, 19.6 named
  arguments, 19.7 TRUE/FALSE without a reserved word, 19.8 else() and \',
  19.9 what did not change and what is still open). Cross-references added into
  the header status line, §3.4, §6, §7, §8 (generic element types), §8.4 and
  §16, so no earlier section now silently contradicts §19. Two of those are
  corrections rather than additions: §8.4's "would be the language's second and
  third reserved words" objection is answered rather than overruled, and §8's
  "check at insertion and literal construction" turned out to already cover the
  new literal with no new code (verified).
- **`plans/missing.txt` is now stale** — it describes a program that dies on
  line 3. Kept as written; this file is the update.

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
