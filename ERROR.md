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

1. *(fixed by 2026-09-25 -- moved to Fixed)*
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
13. *(fixed by 2026-09-25 -- moved to Fixed)*
14. *(fixed by 2026-09-25 -- moved to Fixed)*
15. *(fixed 2026-09-25 -- moved to Fixed)*
16. *(fixed 2026-09-25, the author's ruling "refuse an escape that is unknown" -- moved to Fixed)*
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
    come within ×1.05 of it. satellite_string (string_race, 2026-09-17, best of six
    cold runs): ASCII and all-16-bit text within the noise of plain C++; text with
    emoji decodes ×1.12 and encodes ×1.83 of the fastest plain C++ loop (it was ×2.65
    until to_utf8's wide path stopped building a string for every run).
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

**16 -- fixed 2026-09-25 on the author's ruling ("refuse an escape that is unknown").** An
escape that is not one of the six (\" \\ \n \t \r \') is refused before anything runs,
naming it and the six; `\\` is how a backslash is written. 003's value escapes (\home,
\user) are refused with the rest.

**1, 13, 14 and 15 -- checked 2026-09-25, the error sweep (SCRATCH.md/NEW_ERROR_LIST.md):**

1. **The order of include, main and return is never checked.** Each shape measured on
   BUILD 0072: a `satellite.return(satellite)` on line 1 no longer drops the program (it
   runs); a display above main is refused, "this line is outside every capsule" (the
   sweep); a missing } is "satellite.capsule X is never closed" and an extra one "this }
   closes nothing" (the sweep); `satellite.capsule satellite.main(` with no `)` is refused
   by the capsule header's own check (fixed earlier, by M-scopes).
13. **A byte-order mark or CR-only line endings.** The BOM already ran; \r\n and a lone
    \r end a line now (satl_file.cpp's load_satl, the sweep) -- a file saved on Windows
    was refused at its first capsule.
14. **Whitespace inside the brackets.** `display( 42 )` and `include( satellite )` run
    (fixed earlier; measured).
15. **An unterminated string followed by a `//` comment.** Any string with no closing
    quote is refused before anything runs, "a string on this line has no closing \""
    (capsule_scopes.cpp, the sweep).

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

## A character with no code, outside a string, is accepted without a word — FIXED 2026-09-25

**RULED AND BUILT 2026-09-25.** The author: *"Let's not accept characters that have no
meaning"*. Every such character outside a string or a comment is refused by name before
anything runs (capsule_scopes.cpp, first_thing_with_no_meaning), with a hint for the
common ones -- a no-break space, a curly quote, `#`. A byte-order mark at a file's very
start is dropped as the file's encoding (satl_file.cpp). What the entry said while open:

**The author's call.** `é` on a line of its own runs and exits 0, and so does a
no-break space used as indentation. The lexer writes `error_token` for the character
and nothing ever reports one: the check steps over it, as the run does. That silence
is what turned the payload readers above into wrong answers instead of refusals. Two
readings, neither decided: a no-break space (and the other Unicode spaces) is
whitespace; or any character with no code is refused by name before anything runs,
e.g. `U+00A0 has no meaning in a program` (13). Found by the payload sweep, 2026-09-17.

## Only seven names lex as methods, and the rest say "no capsule named x" — OPEN

`s.size()` on a string does not run `satellite.variable.string.size` (`1 6 1 1`).
It fails **in the checker** with `no capsule named size` (13), which names
something the person did not write and points nowhere near the problem.

**Why.** `bytecode_registry.cpp`'s `method_code_of()` knows exactly seven
spellings — `find`, `replace`, `to_string`, `to_number`, `to_binary`, `to_hex`,
`add`. Anything else after a `.` lexes as `name_token`, so `s.size()` reads as a
call to a capsule called `size`, and the checker says so truthfully about a
program nobody wrote.

**What makes it worth an entry rather than a milestone.** The 24 string methods
are all numbered in `words.tsv` and 20 of them have a built library under
`satellite-numbers/` — they are *there*, and there is no spelling that reaches
them. So this is not "not built yet" (14), which the language has a good word
for; it is a built feature behind a lexer that has never heard of it, reported as
a different mistake entirely.

**Two parts, and only the first is a bug.** The spelling is a milestone. The
MESSAGE is the bug: a `.name()` that is not a known method should say that the
receiver's type has no method of that name, and should not invent a capsule.

Found 2026-09-18, writing a test program for the switch hierarchy — I wrote
`s.upper()` and `s.size()` in turn and believed the interpreter both times.

## 004 does NOT have 003's unbounded console queue — CHECKED 2026-09-18, fine

Recorded so nobody measures it twice. 003's `satellite_console/console.cpp` kept
an unbounded `std::vector<std::string>` between the program and the terminal; a
print loop through satl-term grew the interpreter **125 MB a second, to 5.9 GB in
48**, because `display` never waited for the printer (fixed the same night, 003
revision 09, `2560f1d`).

**004 cannot have that bug**, and structurally rather than by luck:
`satellite-numbers/satellite.console.display/` writes to `std::cout` directly —
no queue, no printer thread — so a slow reader applies backpressure through the
pipe, which is the operating system doing for free what 003 had to be taught.

**Measured, not assumed:** the same shape of program (an unending print loop into
a deliberately slow reader) held **13 MB flat for 20 seconds**.

## The prompt's pty check is FLAKY — a different check fails each run — OPEN

Found 2026-09-19 while proving an unrelated change (the revision/build reset,
`64d1ae5`). `check.sh` line 1161 runs `satellite/prompt/check_prompt.py` under a
real pty; it reports 36–37 checks. **It does not give the same answer twice.**

    check.sh          1 failure:  "Ctrl-D on a line with text deletes forward"
    run alone, once:  37 ok, 0 failures
    run alone, again: 1 failure:  "a line exactly as wide as the screen: its
                                   answer on the next row, with no blank row between"

A *different* check failing each time, with one clean run in between, is timing in
the harness rather than a defect in the prompt. The two named checks share no
PROMPT code path — one is forward-delete, the other is wrap geometry — and they run
on separate `Terminal` instances (`check_prompt.py:192`, width 40, and `:283`, width
20). **What they DO share is the harness**: `Terminal.type` (:148),
`Terminal.wait_for`/`pump` (:140, :126) and `Screen.feed` (:86), which is exactly
where the timing is suspected. Nothing in that session touched `satellite/prompt/`.

**Why it matters more than one red line.** check.sh is what the author trusts to
say 343/343, and a suite that goes red for no reason trains everyone to ignore it.
It also means "check.sh passed" is not evidence on its own — a run must be
repeated before a failure is believed, and before a PASS is believed either.

**Where to look:** `check_prompt.py` drives a pty (`pty.fork()`, :117) and asserts
on the screen rather than the bytes. It does **not** sleep between writes —
`type()`'s `gap` defaults to 0.0 and exactly one call passes one (`:251`, the 800 KB
paste, which is neither failing check). The timing it actually depends on is
`wait_for`'s 0.05 s poll (`:145`) and the single trailing `pump(0.1)` at the end of
`type` (`:157`): a check whose screen is still mid-redraw when that last pump returns
will pass or fail by load.

Note the two failures do **not** share a shape, which was an earlier guess here and
was wrong. `:296` compares positioned rows (`rows_from(...)[:2] == [...]`); `:215` is
a bare substring search over the whole screen (`t.wait_for('[b]')` →
`Screen.shows`, :111), which is position- and order-insensitive. The common suspect
is the wait/pump timing, not positional comparison. Not yet reproduced under
deliberate load.

## A program may have to be run from its own folder — OPEN, not reproduced

The author, 2026-09-18: *"everything except for running files as programs.... you
have to be in the folder to run the program for some reason"*, and *"this file
could not be read"*.

**Recorded because it was seen, and marked because it was not reproduced.** It was
reported against 003 during the console work and never pinned down; nothing in
either tree locks a directory, and the `.satellite_build.lock` in the repository
root is unrelated. The likeliest shape is relative-path resolution — 004 resolves
a program's includes against the file's own folder (the author's rule, 2026-09-16:
*"the files directory becomes the cwd for each file"*) — but the report was about
the PROGRAM, not an include, so that is a guess and is written as one.

**What would settle it in one minute:** run the same program by absolute path from
three different working directories and compare. Nobody has.

**CHECKED 2026-09-25 (the error sweep): NOT REPRODUCED IN 004.** One program with an
include (`parts/ship`) and a `satellite.file.open("note.txt")` beside it, run by absolute
path from /, /tmp, $HOME and its own parts/ folder: all four printed the same and exited
0 -- includes AND relative file names resolve against the program's own folder. If it
comes back it is 003's (the report was made against 003).
