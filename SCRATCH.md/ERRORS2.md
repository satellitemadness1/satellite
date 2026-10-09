# ERRORS2 — what is still wrong in satellite 004

Written 2026-09-25, late, for **the session after `/clear`**. Read this whole file first. It
assumes you remember nothing about tonight.

The first list, `SCRATCH.md/NEW_ERROR_LIST.md`, is **answered**. The author ruled on every
question in it (its status table says where each ruling landed) and every ruling is built.
This file holds what is still open.

---

## Where things stand

- **The repo:** `/home/madness/code/cxx/satellite`. Branch `milestones-install-and-no-console-handover`.
  Every commit also goes to GitHub's main through `utility/push_to_main.sh`.
- **The last commit before this file:** `1a913c0` (a number + text that spells a number adds).
  Then a documentation commit with this file.
- **Tests:** `./check.sh` passes **934 of 934** (about 70 s). `sh satellite_enterprise/check_install.sh`
  passes 20 of 20.
- **The website's examples:** all 318 pass locally
  (`/usr/bin/python3 ~/.config/satellite-foundation/tools/check_examples.py ~/.config/satellite-foundation/site/*.wp.md`).
- **The author's 400 programs** in `~/code/satl`: same exit codes as before tonight, except
  `quad/quad_main.satl`, which now names its own real bug (line 6 never closes its string).
- **The installed satl** (`~/.satl/satl`) is the tree's latest build. Every bare `make` installs.
- **Next, in his words:** "do M16 and M21". M16 is the rest of the string methods; M21 moves the
  string libraries off `satellite_string32`. See `MILESTONES.md`.

### Rules to keep (each one cost something to learn)

- **One `make` at a time.** Check `pgrep -c make` in its own call first. **Never edit sources
  while a make runs.** Tonight a make read `token_codes.hpp` while it was being regenerated
  and failed.
- **Run satl with `SATL_NO_WINDOW=1`,** and send output to a file, never `/dev/null`. Otherwise
  a console window opens on his desktop.
- **Never run a window program here.** Unsetting `DISPLAY` still reaches his desktop. Also set
  `XDG_RUNTIME_DIR` to an empty 0700 folder, and unset `XAUTHORITY` and `GDK_BACKEND`.
- **Any agent that runs satl gets `HOME=<its own folder>`.** Tonight a reviewer agent ran
  `check.sh`'s fixtures under the real HOME and turned his `access` setting off. He had it put back.
- **Stage by name.** Another Claude session may share this checkout. Read `git status` before any
  `git add`.
- **`python3` here is PyPy:** always `with open(...)` when writing. The site tools need
  `/usr/bin/python3`.
- **Commits** are in the house voice: a lowercase sentence, then prose, then the co-author line.
  Push every commit with `utility/push_to_main.sh`.

### Tools built tonight, in the repo

- **`utility/sweep_corpus.sh <satl> <out>`:** his 400 programs, one at a time and safely. Compare
  two runs' `results.tsv`.
- **`utility/check_help_examples.py build/satl`:** every example in `satellite.help`. 8 failures
  are expected; its header lists them.
- **In `~/.config/satellite-foundation/tools/`** (the site, not in git):
  - `refresh_outputs.py`: rewrites output boxes from what satl prints.
  - `pull_taxonomy.py`: copies his categories and tags into the page sources.
  - `publish.sh` now runs `pull_taxonomy.py` before uploading.

---

## Part 1 — needs the author

### 1. The website is not published, because his server stopped answering this machine

Since about 20:30 on 2026-09-25, 162.35.187.225 answers nothing from here: not the website, not
the feedback port, not ping. GitHub loads fine. He was asked to check from his phone, which
separates "the site is down" from "this machine is blocked".

**What is waiting:**
- Every page matches satl (the string joins, S103's new wording, recursion, exercise 10).
- A 20px spacer between each code box and its output box (`tools/blocks.py`).
- `publish.sh` pulls his categories and tags first. He added them by hand on WordPress
  (2026-09-24, 21:13–21:40 GMT) and wants them kept. He also said "you can add more tags":
  do that after the pull, once his vocabulary is visible.

**Three new posts are waiting too**, and they're in `publish.sh`'s list:
- `post-build-an-interpreter-in-cpp`: a whole interpreter in about 200 lines of C++. Its five
  code boxes were joined and compiled tonight, and run.
- `post-recursion-a-million-deep`
- `post-a-syntax-tree-of-objects`: spacesuit nodes evaluating `1 + 2 * 3`.

All 321 of the site's examples pass. He asked for these after asking what satellite needs
before a language can be written in it; that answer is `SCRATCH.md/LANGUAGE_IN_SATELLITE.md`.

**When the server answers:** run `~/.config/satellite-foundation/publish.sh` by its full path.
Never upload a few pages with `upload.py` alone: it re-dates those posts to the top of the blog
and turns their links to other pages into bare links to the site.

### 2. A bool added to a string -- RULED AND BUILT

His ruling: a bool joins as true or false with a space between. `"flag:" + satellite.bool.true`
is `flag: true`, and no second space is added where the text already has one. A colour or an
infinity added to a string is still refused, with "write .string"; he hasn't been asked about
those.

### 3. A number + text that does NOT spell a number joins. Confirm.

His words were "we need 4 + "2" to return the number 6". For `4 + "abc"` I followed 003:
it joins, giving `4abc`, and so does `4 + " 2"`, giving `4 2`. The other choice is to refuse
it. It's one branch in the same function.

### 4. His `satellite.include` help page is out of date

`satellite.help/satellite.include/help_text.txt` is his page, and it's his to edit. It says:
- "absolute path": a leading slash is RELATIVE, by his own ruling of 2026-09-16;
- "relative to a changing current_working_directory": an include is relative to the INCLUDING
  FILE's folder.

It doesn't mention that `satellite.include(./x)` and `(../x)` work unquoted since tonight.

### 5. Blocks at the prompt -- BUILT, at his word

A block is gathered until its braces close. The prompt writes the `{`, and for a spacesuit its
constructor, protected and public sections. What a block declares is kept for the session;
an if, while or for runs. A statement over several lines is gathered too. Terminal-checked in
`satellite/satl/check_session.py`. Still open around it:
- **A level is 4 spaces, not the tab he wrote.** Every satellite program is indented with 4
  spaces. One constant in session.cpp changes it.
- **A block written whole on ONE line, `{ statement }`, is refused**, in a file as at the
  prompt ("followed by something that is not a method call"). A statement can't have a `}`
  after it on its line. His "accept anything valid regardless of lines" suggests it should
  work; the fix is the same kind as tonight's same-line `{` (move a trailing `}` to the next
  line).
- **Re-declaring a spacesuit that objects were made from:** the objects keep the old layout
  (each holds its own).

### 6. Choices made tonight that he may overrule

- **`.center()` into a pipe or a file** prints the text unchanged, since there's no width.
- **A byte-order mark** at a file's very start is dropped, not refused.
- **A shebang** (`#!/usr/bin/env satl`) as a first line is allowed.
- **A line starting with `#` or `/*`** at the top is refused with "satellite's comments begin with //".
- **The config notice** is numbered S016. He offered SC01 "or any free one".
- **upper/lower** use the C library's Unicode tables. `ß` stays `ß`, since it would become two
  letters, SS.
- **Main-thread stack:** the main thread is taken as 64 MiB deep before moving to fresh stack
  segments (`machine/stack_segments.hpp` says why).

---

## Part 1b — found by the author's own testing, 2026-09-26 (to fix)

He tested the installed satl (2119b0f) in another window while the site was published:

- **A. `m.has("a")` is refused on a map**, though `satellite.help(satellite.container.map)`
  names `.has(k)` -- the page says ".has(k) is .contains(k)", meaning it is SPELLED contains,
  and he read it (reasonably) as a method a map has. Map's numbered words are `.set(k, v)`
  `.get(k)` `.has(k)` (1 4 1 1-3). Fix: accept `.has`, `.get`, `.set` on a map (or index) as
  second spellings of `.contains(k)`, `m[k]` and `m[k] = v` -- or, if he would rather not,
  reword the page so it cannot be read as offering them. His to choose; accepting them is the
  reversible, friendlier default.
- **B. A bare `satellite.variable.string empty` cannot be displayed** -- a string declared with
  no value holds nothing, and display refuses nothing. A container declared bare starts empty
  (program_walk.cpp `empty_container_for`); a string declared bare could start as "" the same
  way. Whether a bare number starts as 0 (C++ would say no, Python has no bare declaration) is
  his to rule; the string is the case he met.

- **C. A name declared inside an if block is still readable after the block** (`y` declared in
  the if, read after its `}`). No ruling on block scope is written anywhere (searched the docs):
  a for loop's own number is erased when the loop ends (M20.A), but a body's VariableTable is one
  per capsule, so a block's names live until the capsule ends. C++ and 003-style braces say a
  block's names end at its `}`; Python says they live on. **His to rule** before it is changed --
  he was checking whether the leak is intended.
- **D. `.set(k, v)` and `.get(k)` are refused on a map the same way as `.has`** -- confirms A:
  all three of map's numbered words.
- **E. S501's explanation calls every position "the file's lines"**: a list or map read past its
  end (`age["max"]`, `l[9]`) prints "a line was read by its number and the file has no line with
  that number". The S-code's paragraph was written for files. Say it for what was read (a list's
  item, a map's key). Same family: S301's paragraph says "this operator has no scenario for the
  two kinds it was given" under a list `.append` or a `[ ] =` write that does not fit.

**A, B, D and E are FIXED (5b8cdf3); C is his to rule. Everything his testers found after these, and
the status of each, is in SCRATCH.md/ERRORS3.md.**

## Part 2 — can be fixed without him (not done yet)

### 7. A type refusal at run time is one line, not a report

```
satellite.variable.number n = "abc"
[satellite] satl(run): n was declared satellite.variable.number, and it holds a string (machine_code: 27 types_do_not_meet)
```

Every other refusal is the full report, with its S-code, the line and a caret. These come from
`report_error(...)` in `satellite/bytecode/program_walk.cpp`: search for "was declared". The
fix is `raise_at(...)` with the statement's position. It touches several call sites, and
check.sh rows read those sentences with `grep -c`, so keep each sentence whole.

### 8. A wrong-type literal is refused only when its line runs

Given `takes("text")` for a number parameter, or `satellite.variable.number n = "abc"`, the
lines above run and print first. The checker (`program_check.cpp`) knows each parameter's and
each name's declared type, so it could refuse a literal of the wrong kind before anything runs.
This is new checker work.

### 9. 003's spellings get a sentence about something else

`satellite.machine.cores()` gets "machine has no satellite.variable line declaring it". It
should say that 004 spells it `arguments.machine.cores`. The same goes for 003's other spellings;
the old word table is in `old_versions/second_satellite/`.

### 10. `n.power(2)` is refused under the token's name

It says "n.power_of is not built": `power_of` is the registry's name, and he wrote `power`.
The refusal should use what was written.

### 11. A report inside a statement that spans lines

A report names the statement's first line, even when the mistake is on a later line, and its
caret sits at column 0. The joined statement's tokens all sit on the first line (that is how
line numbers stay exact everywhere else), and `column_of` (`machine/source_position.hpp`)
re-lexes only the first physical line. To fix it, keep a map from the joined line's offsets
back to (physical line, column), built in `join_statements_across_lines`.

### 12. `check.sh` doesn't run the help examples

Before tonight's fixes, the string topic's own example was refused under the return rule and
nothing noticed. Add a row that runs `utility/check_help_examples.py` and expects exactly the
eight known failures, or mark those eight in the topics so the tool skips them.

### 13. Deep nesting inside ONE statement still uses the C++ stack

`display(display(display(...)))` 30,000 deep recurses through `evaluate()` and can crash. His
ruling of 2026-09-16 ("leave it") still covers this. Capsule calls are fixed; the same
stack-segment move (`machine/stack_segments.hpp`) could guard `evaluate()` if he ever wants it.

### 14. `==` on two windows

The help writers suspected it's refused. It has never been tested: that needs the headless
compositor (memory: gui-testing-on-this-machine).

### 15. The prompt's pty check is flaky under load

`satellite/prompt/check_prompt.py`, run inside `check.sh`, sometimes fails a different check.
It's harness timing, not the prompt; see `ERROR.md`.

### 16. `ERROR.md`'s older entries, not re-checked tonight

- The prototype runner: #4, #5, #6, #9–#12, #17.
- The test harnesses: #22–#24.
- satellite_number's internals: #28–#32.
- The toolchain: #33 and #34.

Nobody writing a program sees these. Each has its own entry in `ERROR.md`.

---

## Part 3 — his own programs

`~/code/satl/quad/quad_main.satl` line 6 never closes its string. satl now says exactly that.
His file, his fix.

366 of his 400 programs stop at exit 11: they are included files with no `satellite.main`, or
003 programs. That's expected, not an error.
