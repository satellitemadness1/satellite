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
