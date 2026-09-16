# satellite-004 — ERROR.md

Every KNOWN error in satellite 004 (revision 02, and revision 04 from 2026-09-15). An error is
something that gives a wrong answer, a crash, a hang, or a misleading message.
Choices made on purpose are in DESIGN.md instead, and are not listed here.

Each entry says where it is and how it was found. **When one is fixed, move it to
the bottom section with the commit or date that fixed it.** Never delete one.

---

## 1. The prototype runner — reproduced by review 2026-09-14, all OPEN

A three-lens review attacked the first prototype, and a second agent reproduced
each finding (DESIGN §11). PLAN M1 fixes them. Files (under `satellite/` since
2026-09-15): `satellite/satl/satl_file.cpp`, `satellite/structured-library.cpp`,
`satellite-numbers/call_number.satellite.cpp`, `satellite/race/race.cpp`,
`satellite/race/race.sh`.

1. **The order of include, main and return is never checked.** A
   `satellite.return(satellite)` on line 1 drops the whole program and exits 0;
   a display above `satellite.main(` still runs; unbalanced or backwards braces
   are accepted; `satellite.capsule satellite.main(` with no `)` counts as main.
2. **A directory given as the .satl file** loads as an empty program and reports
   10 (missing include); read errors are never checked, and an unreadable file
   says "missing" (8) with no reason.
3. *(fixed 2026-09-15 — moved to Fixed)*
4. **A refused write names the wrong line** — thousands of bytes after the line
   that first failed, because std::cout buffers ~4 KB. It must say "at or before".
5. **In --debug, a refused state line is never reported** on the early-return
   paths (display_machine_state's code is thrown away).
6. **An empty numbers folder** is "defined" (6), and the program then fails as 13
   instead of vector_loading_error (5).
7. **Exit codes are cut to 8 bits:** a machine code of 256 or -256 exits 0
   (success) after printing an error; 4294967298 exits 2.
8. **Libraries load from the CURRENT directory** when `/proc/self/exe` cannot be
   read (e.g. a path longer than PATH_MAX): another folder's code runs, and under
   ASan dlopen hung.
9. **race.sh prints a passing ratio when a run fails** (a -1 ns time counts as the
   fastest), and race.cpp loads `build/satellite-numbers` relative to the cwd.
10. **Every `*.so` directory entry is loaded as a library** — a directory, a
    dangling symlink, a half-written file — and any one of them stops start-up.
11. **No ABI check at load:** a library built with `-D_GLIBCXX_USE_CXX11_ABI=0`
    loads and writes 82 KB of heap memory to stdout.
12. **A library writing with printf/puts prints out of order**, after later
    interpreter output, because main sets `sync_with_stdio(false)`.
13. **A UTF-8 byte-order mark or CR-only line endings** give a misleading 10
    ("no line says satellite.include(satellite)").
14. **Whitespace inside the brackets is rejected:** `display( 42 )` and
    `include( satellite )` give 13 / 10 / 11 / 12.
15. **An unterminated string followed by a `//` comment** gives 13 (with the
    comment in the message) where the same line without it gives 4.
16. **Unknown escapes are accepted:** `"a\qb"` displays `aqb`, `"\0"` displays `0`.
17. **A library's file name is never checked against the numbers it describes:**
    `7.7.so` can register itself as `satellite.console.display 1 5 1`.

## 2. `satellite_config.hpp` — all fixed

*(18–21 moved to Fixed on 2026-09-15.)*

## 3. Test harnesses — OPEN (they can pass or fail for the wrong reason)

22. **`strings/test_string_methods.cpp` splits text arguments on `|`**, so an
    argument containing `|` becomes two arguments.
23. **`strings/check_string_methods.py` writes 003 programs without escaping**
    the receiver or arguments, so a case containing `"` or `\` produces a broken
    003 program and a false mismatch.
24. **`words/make_words.py` needs 003's built `satl` in `old_versions/second_satellite/`**; without it the
    word table cannot be regenerated (it says so and stops).

## 4. Found in satellite 003 (`old_versions/second_satellite/`), for reference

Not 004's errors, but 004 ports this code and must not port these:

25. A NUL byte in a file's text cuts `errors::render` output short (printed with
    fputs). — MILESTONES/M25.md §7.1
26. At the prompt, a kept include is resolved again after
    `satellite.directory.change`, so a kept `ship` can become another file.
27. S0734's caret sits on `satellite`, not on the argument.

## 5. satellite_number and satellite_string — OPEN (built 2026-09-15, not yet reviewed)

Found by the two builders themselves; no adversarial reviewer has run yet.

28. **`satellite_number x = -1;` compiles and holds 18,446,744,073,709,551,615.**
    The one-argument constructor takes an `unsigned long long int` and is not
    `explicit`, so a signed literal converts silently under `-Wall -Wextra`.
    `from_signed(-1)` is right. Make the constructor explicit, or add a signed one.
29. **`to_text` is quadratic:** 7.7 s for a 1,000,000-digit number (`from_text`
    1.2 s, `digits()` 0.8 s). Multiply and divide are schoolbook, with no
    Karatsuba or divide-and-conquer.
30. **Running out of memory throws `std::bad_alloc`, not a machine code,** and
    leaves a number valid but changed (the basic guarantee). No out-of-memory
    code is on the list.
31. **The speed bar is missed in places.** `i = i + 1` is 0.92 ns (clang) and
    1.00 ns (g++) against 0.61 / 0.33 ns for C++ that refuses to wrap — ×1.49 and
    ×3.0; the author's plain C++ loop folds to one multiplication, so no loop can
    come within ×1.05 of it. satellite_string: decoding ASCII ×1.022, encoding
    ×1.068, the wide path ×1.19–×1.24 of plain C++.
32. **`satellite_number.hpp` exposes GCC/clang-only features** to every file that
    includes it: `[[gnu::always_inline]]` and `unsigned __int128`.

## 6. The toolchain on this machine — OPEN

33. **g++ 17.0.0 (experimental) miscompiles at -O2:** a reserve + nested
    push_back loop followed by `std::vector<char> out(text.size())` read
    `text.size()` as 0. Found while building satellite_string; the code was
    written differently to avoid it.
34. **g++ here is configured `--with-arch=native`,** so it builds for this
    machine's AVX2 by default while clang builds for baseline x86-64. A g++-built
    satellite would die with an illegal instruction on an older CPU, which
    matters for the distribute package.

---

## Fixed

*(move entries here with the commit that fixed them)*

3. **A closed pipe killed the process** (`| head -1`): SIGPIPE, exit 141, nothing
   on stderr. Fix: ignore SIGPIPE so it becomes display_error (2).
   Fixed 2026-09-15 in the build-number commit: `main` ignores SIGPIPE before
   writing anything. The review found it had got worse: the start-up block made
   satl die before any program ran when stderr was a pipe nobody read. check.sh
   now tests that case.

**18–21, `satellite_config.hpp`:** all four fixed 2026-09-15 in the build-number commit. The author rewrote the file
as `return_arguments_vector()`, rows of {name, number, flag, is_flag}. Claude
made it compile (the includes, a row struct, a return type), and
`satellite/arguments/arguments.cpp` loads every row.

18. **`satellite_config.hpp`: nothing could read the values** (local variables of a
    function that returned nothing).
19. **`satellite_config.hpp` did not compile if included** (no `<vector>`, no
    `#pragma once`).
20. **`threads_max = "1'000'000"` used digit separators.** It is the number row
    `1000000` now.
21. **`arguments_satc = "arguments.satc=true"` held a whole assignment as text.**
    `arguments.satc` is a bool row now. PLAN M0's 1 / 0 / "never" (D0.1) is still
    open.

## The walker recurses on the C++ stack, and 200,000 deep segfaults

**Found 2026-09-16 by the author asking "your building another tree walking ast
aren't you?"** — and measured rather than argued:

```
depth  25000  exit 0
depth  30000  Segmentation fault (139)
```

`satellite.console.display(` nested, in a program that is otherwise correct
satellite. **The ceiling is between 25,000 and 30,000**, not the 200,000 this
entry first said — narrowed 2026-09-16 by bisecting.

**AND THE STACK IS NOT A PORTION OF RAM, which is the whole point.** `ulimit -s`
is 8192 KB — **8 MB, a fixed reservation**, independent of this machine's 61 GB.
The program dies at ~27,000 deep with **48 GB sitting unused**: about 280 bytes
of C++ stack per level. A stack the walker owned would be a heap vector bounded
by those 48 GB at roughly 32 bytes a frame — over a billion levels instead of
27,000, on the same machine. That is four orders of magnitude, and it is the
difference between a bound and a limit.

**It is not an AST, and that part of the design holds.** Nothing is allocated per
node, there is no `Node`, no pointer, no tree: the program stays one flat
`std::vector<std::bitset<16>>` and a call is a POSITION in it.

**But the walk is recursive, and the C++ call stack is what holds the nesting.**
`evaluate()` → `call_word()` → `evaluate()` in
`satellite/bytecode/program_walk.cpp`, and `run_body()` recursing into a capsule
has the same shape, so a deep chain of capsule calls dies the same way.

**THIS BREAKS A STANDING RULE, in the author's own words: the language has no
limits, a crash is not a limit, and a bound is never the fix — THE WALKER KEEPS
ITS OWN STACK.** Raising the thread stack size would be exactly the bound the
rule refuses.

**The fix is the rule:** an explicit stack the walker owns — frames of
`{row, position, the argument being built}` pushed and popped in a loop, with no
C++ recursion anywhere in `evaluate`, `call_word` or `run_body`. Then depth is
bounded by memory, which is the only bound satellite accepts (DESIGN §7.5).

Until then the depth that works is large but real, and it is a defect rather
than a limit.
