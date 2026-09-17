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
2. *(fixed 2026-09-17, PLAN M0.5 — moved to Fixed)*
3. *(fixed 2026-09-15 — moved to Fixed)*
4. **A refused write names the wrong line** — thousands of bytes after the line
   that first failed, because std::cout buffers ~4 KB. It must say "at or before".
5. **In --debug, a refused state line is never reported** on the early-return
   paths (display_machine_state's code is thrown away).
6. **An empty numbers folder** is "defined" (6), and the program then fails as 13
   instead of vector_loading_error (5).
7. *(fixed 2026-09-17, PLAN M0.5 — moved to Fixed)*
8. *(fixed 2026-09-17, PLAN M0.5 — moved to Fixed)*
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

**2, 7 and 8 — fixed 2026-09-17 by PLAN M0.5**, each with a check in check.sh:

2. **A directory given as the .satl file** loaded as an empty program and reported
   10 (missing include), and an unreadable file said "missing" (8) with no reason.
   `load_program` now refuses anything that is not a regular file by name --
   "cannot run examples: it is a directory" -- and says why a file could not be
   read ("Permission denied"). A FIFO and /dev/zero are refused the same way
   instead of blocking or reading forever. Still 8, `missing_satl_file`.
7. **Exit codes were cut to 8 bits:** 256 exited 0. `machine/exit_status.hpp`:
   a code outside 1-254 exits 255 with the whole code on stderr, and 255 is
   never given to a code (D0.5.2). No program can stop on 256 yet, so
   `build/exit_status_cases` proves 0, 1, 255, 256, -1 and 4294967298.
8. **Libraries loaded from the CURRENT directory** when `/proc/self/exe` could
   not be read. `numbers_folder()` grows its buffer until the path fits, and a
   path the kernel will not give (a satl deeper than 4,096 bytes) is refused
   with 5 and the reason; check.sh plants a working set of libraries in the
   current folder to prove none is loaded.

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

**DECIDED, THE AUTHOR, 2026-09-16: LEAVE IT. ~27,000 IS ENOUGH.** In his words:
"we are never going to program 1,000 ever... so leave it". This entry stays open
as a KNOWN and ACCEPTED depth, not as work owed.

**What was weighed.** The depth that matters is not nested parentheses — nobody
writes 20,000 of those — it is a capsule that calls ITSELF, because `run_body()`
recurses too. A tree walk or a graph traversal can reach a few thousand levels,
and QUAD's sky is a graph. Against that: 27,000 is an order of magnitude past any
recursion a person writes, and the fix is a real rewrite of the walker.

**It also cannot be reached today.** satellite has no variables, no arithmetic
and no working `if`, so a terminating recursive capsule cannot be written at all.

> **THAT LAST PARAGRAPH STOPPED BEING TRUE ON 2026-09-16.** satellite now has
> variables (`satellite.variable.number n = 34587`), all six arithmetic
> operations wired to their tokens, comparisons, and
> `satellite.statement.while`. A capsule that calls itself with a decreasing
> number and stops at zero is now writable, so the depth is reachable by a
> program a person could actually write. The author's ruling above still stands
> — ~27,000 is accepted, not owed — but the reason it was unreachable is gone,
> and `satellite.statement.if` is the only piece still missing before a
> recursion can choose to stop. Re-read this entry when `if` lands.

**What would reopen it:** a real program that actually runs out — generated code
rather than written code is the likely source, since QUAD writes satellite. If
that ever happens the fix is known and is the rule itself: explicit frames of
`{row, position, the argument being built}` pushed and popped in a loop, no C++
recursion in `evaluate`, `call_word` or `run_body`. Raising `ulimit -s` is not the
fix; it only picks a different number.

## An operator with no meaning cuts an expression in half, silently — FIXED 2026-09-17

**Fixed:** `run_assignment` and `run_while` now refuse an expression that does not
reach the line's end (a trailing `//` comment counts as the end) with
`satl_line_not_understood` (13): `n = 1 & 2`, `while(n < 3 & 1)` and `n = 5 6` are all
refused. The same sweep found `n += 1` skipped without a word for every type; the
checker now refuses it by name before anything runs (`not_built_yet`, 14). Checked by
tests/unread_*.satl, compound_assign.satl and comment_after_value.satl, and by every
tracked program giving the same exit code and output before and after.

What the entry said while it was open:

**Found 2026-09-16 by five adversarial agents in their own worktrees, and it
survived a skeptic who set out to refute it** (reset to the right baseline,
rebuilt, confirmed `check.sh` was green first, then reproduced every case).

```
satellite.variable.number n = 1 & 2
satellite.console.display(n)              prints 1, exit 0, nothing on stderr
```

`evaluate_at` ends an expression on ANY code whose `precedence_of` is 0
(`expression.cpp`), and `&` `|` `&&` `<<` `!!` `~` all have no case — they are the
rows REGISTRY.satellite marks QUESTION, because §13 never decided what they mean.
So the expression stops there and **the left half is answered as though it were
the whole thing.** `n` is left holding 1.

**The half that is fixed, and why the other half was missed.** `call_word` now
refuses when an expression does not reach its own `)`, so
`display(1 & 2)` is caught. `run_assignment` and `run_while` call
`past_the_statement` unconditionally and check nothing — so a wrong value reaches
a VARIABLE and a LOOP BOUND, which are worse than a wrong printed line because
nothing shows. The three lines that fix `call_word` are the three lines both need.

**What makes it reachable rather than theoretical:** `token_codes.hpp` states the
intended behaviour in its own words — `bit_and_token ... & ends an expression and
is reported`. It ends the expression. It is not reported.

**Not reachable through a touching operator any more.** `(a + b)/2`, `6/3` and
`6/2` were part of the same finding and are now refused, because the author's
whitespace rule (2026-09-16) gave the touching slash its own token. That fix was
incidental to this one and does not cover `&` `|` `<<` `!!`.

## A string's last code, or a stray character's, was read as a word — FIXED 2026-09-17

**Fixed** in the commit that added this entry. A [COUNTED] token is followed by a
count and that many codes, and a character's own number can be any 16 bits, so a
walker that steps one code at a time lands inside a payload and reads it as
structure. The count review had found three such readers (2314 = long_count_token,
515 = `)`, 258 = error_token; fixed the commit before). A sweep of every walker
(workflow wf_3e492290-2c1: one lens reading each loop, one fuzzing about 75,000
generated programs through satl against Python, a skeptic per finding) found three
more:

- **`load_program`'s include scan.** U+1002 is 0x1002, `satellite.include`'s code.
  `"ဂ"("other")` -- or a stray `ဂ("other")` -- loaded other.satl, whose capsules then
  replaced the program's own; with no such file it was refused as a missing include (8).
- **`capsules_in`.** U+1006 is 0x1006, `satellite.capsule`'s code. A stray
  `ဆ satellite.main` on a line of its own made the capsule after it main: its output,
  exit 0. `"ဆ" greet` replaced the capsule the user wrote with a loop body.
- **The lexer's look-back for a method name** read the last CODE, not the last token.
  U+0704 is 0x0704, `method_token`, so `"܄"str` made `str` a method code the check
  never looks at: "before" printed, then 13.

Each fires as well for the character above U+FFFF whose low half is that code
(U+11002, U+11006, U+10704). The two scans now skip a payload whole, as every other
walker does, and the lexer remembers the last token it put. check.sh has each case
beside the neighbours one character either side that never collided.

## The check and the run disagreed about a stray code at a line's start — FIXED 2026-09-17

**Fixed** in the commit that added this entry. Found by the same sweep's skeptics. A
statement the check had no shape for -- one starting with a character that has no
code, a `)`, a string -- was skipped WHOLE by the check, while the run stepped over
that one code and ran the rest of the line. So a no-break space pasted as indentation
hid the declaration after it (a declared `n` was refused with 25, "no
satellite.variable line declaring it"), and `undeclared = 5` after one ran past the
check: "before" printed, then 25 at run time, breaking "nothing runs before a
refusal". The check now steps over the one code, as the run does. A comment line
still passes: its token steps to the line's end.

## A character with no code, outside a string, is accepted without a word — OPEN

**The author's call.** `é` on a line of its own runs and exits 0, and so does a
no-break space used as indentation. The lexer writes `error_token` for the character
and nothing ever reports one: the check steps over it, as the run does. That silence
is what turned the payload readers above into wrong answers instead of refusals. Two
readings, neither decided: a no-break space (and the other Unicode spaces) is
whitespace; or any character with no code is refused by name before anything runs,
e.g. `U+00A0 has no meaning in a program` (13). Found by the payload sweep, 2026-09-17.
