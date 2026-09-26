# FAST PRINTING — the plan to make satellite print as fast as it can

Written 2026-09-26, for the author and for **the session after `/clear`**. Read this whole file first.
It replaces the open design in `SCRATCH.md/DISPLAY_THREADS.md`. That file still holds the first
attempt: why it was pulled out, and every measurement it made. `SCRATCH.md/RUNNING_PROGRAMS.md`
holds the running-programs half.

**Steps 1 to 4 are built.** 1 and 2 are installed and pushed (builds 0109 `ab6e7d7`, 0111 `fbf0788`);
3 and 4 went in together as build 0113 (0112 was the same before the review's fixes, never kept).
**Steps 5 and 6 are next.** The measurements below are real.
The code they came from is in `SCRATCH.md/FAST_PRINTING/`.

| best of five (step 2: of three) | 0108 (read in the step-1 race) | 0109, step 1 | 0111, step 2 | 0112, steps 3+4 (0113's own times: the line below) |
|---|---|---|---|---|
| 1,000,000 lines | 2.34 s | 1.07 s | 1.06 s (0109 read 1.12 in that race) | 1.07 s (0111 read 1.02) |
| 1,000,000 numbers | 2.10 s | 0.83 s | 0.83 s | 0.85 s (0111 read 0.81) |
| 200 × 1 MiB | 0.63 s | 0.22 s | 0.23 s (noise) | 0.30 s (0111 read 0.22) |

Output byte-identical in every race. `./check.sh` 1028 of 1028 after steps 1 and 2; 1035 of 1035 after
3 and 4, seven rows of them new. 0113 against 0111: lines 1.04 s / 1.00, numbers 0.86 / 0.80, big 0.22-0.31 / 0.22.

**STEPS 3 AND 4 CAME OUT SLOWER, AND HE KEPT THEM.** Reported the moment they were timed, with the
numbers above and three options -- hand over 8 KB at a time instead of each display, fix the big
strings, or revert. His answer: *"let's just keep it for now, and someday we can do the whole 3.5 ns
thing that beats compiled C++, the mini interpreter, so it's no big deal either way, we can always
build a mini satl for printing someday"*, then *"keep step 3"*. Why it is slower, measured:
- **200 × 1 MiB, +36%: my choice.** A slot keeps its value until the interpreter reuses it 1,024
  displays later, so the interpreter frees it on its own thread. For 1 MiB strings that held every one
  alive: peak memory 26 MB -> 440 MB, page faults 7k -> 107k.
- **Short lines and numbers, +5%: the hand-off costs what it saves.** After steps 1 and 2 a display's
  own work past its statement is ~500 instructions (callgrind on 0111), and handing it over costs about
  that.

After step 1 he said: *"we are building a fast-enough-plan anyways, we gave up on racing compiled C++
with our interpreter"*. **The rule below still holds** (a slower step stops), but no C++ race is owed.

And his last words before the `/clear`, on 0113's times: *"still going fast besides the 1 MB strings, but
0.22s is still fast for 200 1 mb strings, it's still running fast, and we can retune it when we're done"*.
So **retuning waits until steps 5 and 6 are built.** The list is under START HERE.

---

## START HERE AFTER `/clear`

**Where things stand (2026-09-26):**
- **The repo:** `/home/madness/code/cxx/satellite`, branch `milestones-install-and-no-console-handover`.
  Every commit also goes to GitHub's main through `utility/push_to_main.sh`. **The repo is PUBLIC:**
  before a push that adds files, scan them for the values in `~/.config/satellite-foundation` without
  printing them (the old scanner, secret_scan.py, was in session 8164ac05's scratchpad on tmpfs and
  may be gone; a grep of the new files for those values does the same).
- **Installed:** `~/.satl/satl` is build 0113, commit `c8fca60` (steps 3 and 4). This file's own last
  update is the commit after it. `git log --oneline -5` shows both.
- **`./check.sh`: 1035 of 1035.**
- **Nothing is half-built.** `git status --short` should show only `?? .claude/`; anything else is not
  this session's.

**Also owed, from earlier on 2026-09-26:** a fresh review of the M16 + errors merge (`8923ad3`), skipped
then for tokens (memory note everything-on-github-for-cloud). It's his to schedule, and it isn't dropped.

**What is next, in order:**
1. **Step 5, straight into VTE** (below). It can start at once. The risk to measure first: satl's own
   console window gets text by TWO roads today, and step 5 adds a third:
   - the pty, which carries the prompt's line editor (`satellite/prompt/render.cpp` writes fd 1
     directly), the terminal's own echo of typed keys, and every program output today
   - a direct `vte_terminal_feed` (step 5's new road)

   Output fed straight in can overtake bytes still in the pty, or fall behind them. So a flush must
   wait until VTE has been FED, not just until the bytes were handed to the window's thread. The flush
   is `display_drain()` in `printing_satellite.cpp`, reached through `std::cout.flush()` (display_stream.cpp's
   `sync`). The one before every prompt is `session.cpp` line 625. And anything that still writes the
   pty must come after the feed.
   - **The console's pieces:**
     - `satellite/satellite_variable_window/window_console.cpp`: `a_terminal_to_type_in` makes the
       VteTerminal and its pty; `open_the_slave_of`
     - `console_launch.cpp`: the pty's slave becomes fd 1
     - `window_desk.hpp`: the window's own thread, the only one that may touch VTE
   - **Do not hand pieces over with `window_desk.hpp`'s `on_the_desk()`.** It WAITS for the GTK
     thread, which is the opposite of once a frame. The non-waiting shape is the race's: an eventfd
     watched with `g_unix_fd_add` (`on_wake` in `vte_print_race.cpp`).
   - **What the race says, read right:** rows A and B BOTH feed VTE directly; C writes the pty.
     - **Step 5 builds A's shape:** the interpreter's side makes the line, and the window's thread
       feeds it. B is the bypass (SECRET.md's), not this step.
     - **As raced, A took 17,355 ns a print end to end against C's 3,738:** the direct feed was SLOWER.
       The race feeds on every wake, with no once-a-frame mode, so the 60 Hz explanation is untested.
     - **Step 5 counts only if A, fed once a frame, beats C end to end.** Add that mode to the race and
       measure it BEFORE building step 5 into satl.
   - **How to time step 5 in satl:** HOW TO MEASURE's loop only proves the file and pipe path did not
     get slower. The step itself must be timed in satl's own console on the headless mutter: `satl
     --console prog.satl`, prove-console.sh's `loud` shape, with WAYLAND_DISPLAY set to its socket, 0113
     against the new build. No script for that exists yet: write it before building, or tell him.
2. **Step 6, a running program's output** (below). **satl starts no programs today** (no posix_spawn in
   satellite/; the only program start is prompt_run.cpp's fork+execv of satl itself), so step 6 IS
   building `RUNNING_PROGRAMS.md`, with its output joining before the display thread. **Ask him first:**
   its open questions 1-4, 6 and 7 are his and unanswered -- the satellite spelling of running a
   program, whether satl waits for it, keyboard input for it, and Ctrl-C. Question 5 is answered by his
   drawing: program output goes between the printing satellite and the display thread. **Why it matters, his words before the `/clear`:**
   *"this is a pre-requisite for running a pre-version of quad ai that another ai cooked up, I dunno, we
   do need to run std::system, or a command like it anyways"* -- running programs (the fast std::system
   on posix_spawn, `RUNNING_PROGRAMS.md`) stands between satellite and QUAD AI's first run. That
   pre-version is mostly another AI's: *"it's only like... 15% built by me"* -- so its choices are not
   his rulings unless he says so.
3. **Then the retune he asked for**, once 5 and 6 are in:
   - the 1 MiB strings: a slot keeps its value until reused, so 200 x 1 MiB peaks at 440 MB. The
     printing satellite could let go of a big value as soon as it is written. It is a cross-thread free,
     which costs ~100 ns and does not matter at 1 MiB.
   - the +4-8% on short lines and numbers. The option he did not take: the interpreter makes its own line, as 0111
     did, and hands over whole 8 KB pieces, so no hand-off is paid per display.
   - Someday, his: the mini satl for printing (SECRET.md, *"the whole 3.5 ns thing"*).
4. **Then the errors.** His words before the `/clear`: *"there's still... 200 errors to fix or something
   like that, when this is done"*.
   - The list is `SCRATCH.md/ERRORS4/README.md`: one page per error, 85 pages merged from 178 suspected
     (127 confirmed new), found 2026-09-26 on build 0099. Older lists: `SCRATCH.md/ERRORS2.md`
     and `SCRATCH.md/ERRORS3.md`.
   - **None is re-checked since build 0099.** Today's builds 0109-0113 changed call_word, string
     literals and all of display, so run each page's own program on today's satl before fixing it.
     Some may be gone; page 083 (`083-display-thread-nothing-s210.md`) is about display and threads.

**How to build and check, the rules that cost something to learn:**
- **Keep the build to beat first:** `cp -a build/satl build/satellite-numbers <scratch>/before/`. satl
  loads its word libraries from beside itself.
- **Build without installing:** `make INSTALL_AFTER_BUILD=no`, in the background (about a minute:
  PGO + ThinLTO + BOLT). A bare `make` installs into `~/.satl`. **One make at a time.**
- **Race old against new:** the loop in HOW TO MEASURE, with output compared byte for byte.
  **Slower: stop and tell him** with the numbers. His answer this time was "keep step 3"; the next may not
  be.
- **Then:** `./check.sh` (about 1.5 min, in the background); a fresh reader for any concurrency code (this
  time it found eleven real defects); commit; `utility/push_to_main.sh`;
  `SATELLITE_JUST_BUILT=yes sh satellite_enterprise/install.sh`.
- **callgrind** needs `--max-threads=1200`, because of the 1024 start-up threads.
- **GUI tests with a window** go on a headless mutter with the REAL `XDG_RUNTIME_DIR`, its own
  `--wayland-display=<name>`, and satl run under `env -u DISPLAY -u DBUS_SESSION_BUS_ADDRESS
  WAYLAND_DISPLAY=<name>` inside `dbus-run-session` (prove-console.sh lines 200-213; run_race.sh).
- **A run that must open NO window** uses the memory note's recipe instead: an empty 700
  `XDG_RUNTIME_DIR`, `-u XAUTHORITY -u GDK_BACKEND`. Anything else can reach his desktop.
- **run_race.sh needs `/run/user/1000/satl-window-*`** (the console's fonts and keyboard data, on tmpfs,
  gone after a reboot). If it is gone, run `prove-console.sh`, which makes it. Never make it by opening
  the console on his desktop.

**Where the printing satellite lives:**
- `satellite/display/printing_satellite.hpp`: the design and his words.
- `printing_satellite.cpp`: the ring, his buffer and limit, the pieces, the display thread, the sleeps,
  and the flush.
- `display_stream.cpp`: std::cout's 8 KB.
- `bytecode/expression.cpp`'s `display_plainly`: what the interpreter keeps and what it hands over.
- `console_calls.cpp`'s `display_with_options`: a styled line, made on the interpreter's thread.
- `program_walk.cpp`: search `display_overran`; the S840 stop between statements.
- `structured-library.cpp`: `main` starts the printing satellite; `run_satl` reports an overrun found
  after the last statement.
- `satl/session.cpp`: the overrun and Ctrl-C let-go between prompt lines.
- `arguments/arguments.cpp`: the `display.buffer` row.

---

## His words, in order (this session)

After the first attempt was pulled out, he gave the design again:

> *"satl>buffer>converter>display"*, *"that is how we are building it, regardless of how fast or slow it
> comes out, because we need that buffer to be built to relieve the interpreter of having to convert and
> having to display the strings"*, *"arguments.display.buffer(131072) is the default, but the user can
> configure it to anything if they want, and we load this value so we don't have to keep getting it from
> arguments, it's loaded as an unsigned long long int"*

Then:

> *"remember we are grabbing the string AFTER satl has converted it into its final satellite_string
> form, we don't need to do anything with floats"*

> *"The entire satellite interpreter is used to render different color properties and add numbers and
> everything, there is no final satellite_string object to hand to a buffer at all"*, *"the only
> stipulation is that the satl interpreter must be able to run a posix_spawn process"*

> *"if creating an additional, mini-interpreter to handle all the printing is what solves that, then
> let's just build something that does that"*, *"My original satl>buffer>convert>display becomes
> satl>mini_satl_with_builtin_buffer>display_thread_here and then the program output is mixed in, in
> between mini satl and the display thread"*, *"we have our own libvte window ... libvte is the very
> fastest option and we're crippling the entire libvte purpose as we have it currently"*

And the ruling this plan follows:

> *"Let's just do your 5 step thing, and wire it into libvte as fast as we can get it to go into
> libvte"*, *"write up the plan to go as fast as we can using a mixture of whatever is reasonable"*,
> *"i'm going to write SECRET.md for another time"*

**SECRET.md is his**, for another time. It is the bypass: the printing satellite works out the
parentheses itself. Nothing here closes the door on it. Steps 1 and 3 are what it would stand on too.

---

## What was measured today — the facts this plan stands on

**Build 0108 is the build to beat** -- read in the callgrind session, the one the 1.27 µs comes from
(the top table's 0108 column was read in the step-1 race). Output to a file, best of five:

| | 1,000,000 lines | 1,000,000 numbers | 200 × 1 MiB |
|---|---|---|---|
| with display | 2.27 s | 1.96 s | 0.61 s |
| the same loop, no display | 1.00 s | 0.76 s | — |

So a display costs about 1.27 µs a line.

**Where that 1.27 µs goes** comes from callgrind on 0108: 100,000 displays of a 37-character literal,
against 100,000 assignments of it (`SCRATCH.md/FAST_PRINTING/callgrind_display.satl`). A display runs
about **11,600 more instructions** than an assignment:

| per display | instructions |
|---|---|
| `is_window_word`: `word_at` (`bytecode/window_calls.cpp:162`) walks the whole window word table, and for every row looks its code up again with `word::code_of` | **~8,600** |
| `is_file_word`, `is_container_word`, `is_infinity_word`, `is_info_word` and the `word::code_of` calls inside them | ~1,900 |
| converting to UTF-8 and writing it out (`write_utf8`, `text_utf8`, `std::cout`) | a few hundred, **~5%** |

`word::code_of` (`bytecode/word_codes.hpp:1436`) is a binary search over the ~364 words, run at run time
on every call. **So about 90% of a display is satl asking whether `display` is a window, file or
container word, and about 5% is the printing.** Every thread design before this one aimed at the 5%.

String literals are also rebuilt on every use: `text_at` turns the bytecode's codes into UTF-8, then
`of_utf8` turns that back into a `satellite_string`. That's ~1,500 instructions a literal, in every
program, display or not.

**His VTE race** (`SCRATCH.md/FAST_PRINTING/vte_print_race.cpp`):
- 1,000 prints of `500000 + 12349.474 * 457474 + " hello, world!"` into a real VTE window, with satl's
  own vendored GTK 4.24 and VTE, on a headless mutter
- every row printed `5650063268.676 hello, world!`, as satl does
- per print, median of 7 (best in brackets)

| | main thread | window thread | end to end (VTE has every line) |
|---|---|---|---|
| **A** the five steps: main works it out through `satelliteObject::multiply`/`add`, hands the finished string over | 1,082 ns (995) | 141 ns | 17,355 ns |
| **B** the bypass: main hands over "one more line", the window's thread does all of it | 43 ns (39) | 953 ns | 1,613 ns |
| **C** compiled C++: `double` and `std::cout` into the window's pty | 1,558 ns (1,478) | — | 3,738 ns |

- **Handing a job to another thread cost 43 ns** (B's main thread), with the receiver keeping up.
- **C's main thread waits on the window:** `std::cout`'s `write()` into the pty waits for VTE to read.
- **A's 17 µs end to end looks like one 60 Hz frame** (16.7 ms for the 1,000). Its lines were fed to
  VTE in many small pieces, and the last ones waited for the next frame. This is not proved yet. Step 5
  feeds once a frame, which proves or disproves it.
- **A feed is not a pty:** nothing turns `\n` into `\r\n`, so the feeding side must.

**From the first attempt** (`DISPLAY_THREADS.md`, still true):
- moving a `satellite_string` costs 11–19 ns at any size
- copying one costs about what converting it does
- a string made on one thread and let go on another costs ~100 ns more (glibc's per-thread caches)
- waking a sleeping thread costs ~420 ns
- three other busy cores slow this Xeon's interpreter core ~7% (turbo)

---

## THE RULE FOR EVERY STEP

**Time it against the step before, the moment it runs.** Use the three programs in
`SCRATCH.md/DISPLAY_THREADS/` (`bench_lines`, `bench_numbers`, `bench_big`), alternating old and new,
best of five, output to a file, `SATL_NO_WINDOW=1`. For steps 1 and 2, also run callgrind for the
instructions per display.

**Slower is a failed step.** Stop and tell him, with the numbers. Don't start a round of fixes. He
ignores my timings: give him the command, and lead with right/wrong counts.

`./check.sh` must pass after every step. Commit and push after every step.

---

## THE PLAN, IN ORDER

### Step 1 — the word checks become one read (the ~90%) — DONE, build 0109, `ab6e7d7`

**What changes:**
- **DISPLAY NEVER ASKS ABOUT WINDOWS AT ALL** (the author, 2026-09-26: *"make sure that your plan
  bypasses that whole thing about checking satellite.window code, we need to bypass those 8,000 lines
  when we are printing at least"*).
  - Today, once a display's arguments and `.center()` are read, it runs the whole family chain before
    reaching its own code at `expression.cpp:1825`:
    - `is_file_word` (`:1770`)
    - `is_info_word` (`:1774`)
    - `is_infinity_word` (`:1777`)
    - `is_window_word` twice (`:1784`, `:1790`)
    - `is_container_word` (`:1793`)
    - `is_console_word` (`:1806`)
  - Instead, **`is_display_word(code)`, one compare, is the first test after the arguments**, and a
    display jumps straight to its own path. From step 3 on, that path is the hand-off to the printing
    satellite. Not one family check runs for a display.
  - Only `is_thread_word` and `is_access_word` stay ahead of it, because they read their arguments
    their own way. With `code_of` made `constexpr` (below), each is a compare against a constant.
- **`word::code_of` becomes `constexpr`.** Its tables already are. Every `code == word::code_of(1, 8, 1)`
  in `is_file_word`, `is_info_word`, `is_infinity_word`, `is_container_word`, `is_access_word`,
  `is_thread_word` and `call_word`'s own compares then folds to a constant.
  - `word_codes.hpp` is generated by `satellite/bytecode/make_word_codes.py`, and check.sh compares it byte for byte, so the
    change goes in the generator.
- **For every other word, the window words get a table built at compile time.** It's indexed by
  `code - kFirst`: about 450 bytes, one per word. `is_window_word` becomes one read, not a walk. Every
  word that isn't a display still passes the family checks, and each one becomes a read or a constant
  compare.
- **`call_word` asks `is_window_word` twice today.** Once is enough.
- **`const Value argument = arguments.front()` becomes a move.** `arguments` is `call_word`'s own. This
  removes a second full copy of every displayed value (found in the first attempt's review).

**Why:** ~10,500 of the ~11,600 instructions a display costs. And it speeds up every word call in the
language, not only display.

**Expected:** by instruction count, most of the 1.27 µs. Callgrind and the three bench programs say
how much.

### Step 2 — string literals built straight from their codes — DONE, build 0111, `fbf0788`

**What changes:** `text_at` builds the `satellite_string` directly from the bytecode's codes, in one
pass. Today it goes codes → UTF-8 → `from_utf8` → codes.

**Why:** ~1,500 instructions a literal, paid by every program. It's independent of display, and cheap
to do while in `expression.cpp` for step 1.

### Step 3 — the printing satellite (the five steps) — BUILT, build 0113

**As built** (`satellite/display/printing_satellite.hpp` and `.cpp`), and where it differs from the plan
below -- my choices, his to overrule:
- **A styled display** (`foreground=`, `end=`, `.center()`, the console's colours) is made on the
  interpreter's thread and handed over as its bytes: its refusals need its text there anyway.
- **A container's text** (list, index, window, thread) is made on the interpreter's thread, so a list
  holding a file is refused on its line. Everything else is moved in as its value.
- **Freeing: the slot keeps the value** until the interpreter reuses it (see the +36% above).
- **The overrun report** points at the statement the walker was on when it stopped the program --
  in a display loop, the loop's closing `}`, not the display line.
- **At the prompt,** an overrun stops that line and the prompt comes back; the next line starts clean
  (`display_overrun_let_go` in session.cpp).
- **Ctrl-C at the prompt lets go of what the stopped line left waiting,** as a terminal lets go of its
  output -- otherwise up to 131,072 lines scroll on, and the second press ends the whole session.

**ONE FRESH READER WENT THROUGH IT BEFORE THE COMMIT** (a one-agent workflow, read-only). Eleven
findings, all fixed before 0112's successor was built:
1. a styled line (colours, `end=`, `.center()`) overtook std::cout's bytes before it -- `clear()` then a
   coloured display cleared the screen after the line. Shown on 0112 (`Hello` came out before `ESC[2J`);
   now `display_line_bytes`, and a check.sh row.
2. an overrun nobody reported (a program thread's, at the prompt) left the printing satellite dropping
   everything for the rest of the session -- the prompt's next line now lets go as a report would.
3. a flush after an overrun returned before up to 256 KiB already on their way, so the S840 report could
   land among them and exit could cut a line (or a colour code) in half -- it now waits while the screen
   is still taking text, and gives up after 1 s with nothing written (a fifo nobody reads).
4. SIGTTOU was still blocked on the display thread (its mask was added to, from a thread with every
   signal blocked) -- now set outright.
5. Ctrl-C at the prompt left up to 131,072 lines to scroll -- see above.
6. after an overrun the count of finished jobs could stop short and hang the next flush -- jobs let go
   now travel through the pieces (a piece may carry no bytes), so they settle in order.
7. two threads flushing at once could lose a wake-up -- the sleepers are counted, not flagged.
8. out of memory on the printing satellite was permanent and could leave half a line in a piece -- the
   half line is taken back out, and it is said once.
9. one failed allocation in the std::cout stream would have left it pointing into memory it gave away.
10. a membarrier that failed after registering fenced one side only -- both sides fence from then on.
11. one huge line kept a huge buffer for the rest of the run -- given back past 256 KiB.

Checked and found fine: the slot hand-off, the pieces, the asymmetric barrier between all three threads,
the flush in normal running, several threads displaying, fork, satl's own console, the S840 check.

One thread, started off `main()` at satl's start-up. It has an explicit small stack, because satl
widens its own stack limit to gigabytes (`stack_share.hpp`).

**The main satl, for a plain display. Steps 1–5:**
1. Works out the parentheses: every read, `+`, capsule call, method and error, on this line, in the
   program's order.
2. Works out the options (`foreground=`, `background=`, `end=` and the rest) and notes `.center()`.
3. Takes the console colours in effect now, because the next line may change them.
4. Runs the checks that must stop the program on this line:
   - display takes one value
   - the value can become text. A file or a spacesuit object can't, and neither can a list holding
     one (`satellite_object.cpp:725`). For a list this is a walk of the item kinds, making no text.
5. **Moves the job into the printing satellite, and goes on to the next line.**

**The job:** the value (moved), the options, the colours, centred or not, and the end of line.
- A list or index is shared copy-on-write (`satellite_list.hpp`), so a change on the next line makes
  the program its own copy.
- The value from step 1 already belongs to main, even for `display(s)`, so nothing is copied for the
  hand-off.

**The hand-off, from the measurements:**
- **No system call per display.** The printing satellite keeps looking for work for ~50 µs after its
  last job before it sleeps, so a print loop never pays the 420 ns wake. A hand-off wakes it only
  when it is asleep.
- **A lock only once a program has started a thread,** under `console_lock.hpp`'s rule.
- **Freeing moved values: measure both ways in a standalone race before touching satl.** Either the
  printing satellite lets them go (+100 ns measured), or it hands the emptied values back for main to
  let go in batches. Pick the faster.
- **Only one extra busy thread while output flows,** not three: the Xeon's turbo counts busy cores.

**The printing satellite, per job:**
- the value becomes text: digits, a float, a fraction, `{1, "two"}`, `true`
- styling: colours and bold (`styled_line`)
- centring, with the console width read at print time
- `to_utf8`
- the `\033` rule (`for_the_screen`: colours kept when they reach a terminal, taken out when not)
- the end of line

It hands UTF-8 pieces to the display thread. That hand-off is bounded to a few pieces, so when the
screen is slow, jobs wait in the printing satellite, where they are counted.

**HIS BUFFER AND HIS LIMIT live in the printing satellite:**
- **The count:** `unsigned long long held`, +1 when a job arrives, −1 when its text leaves for the
  display thread. `if (held > limit)` is the check, his own spelling.
  - He asked whether a `std::vector` keeps its size. It does: it holds a begin and an end pointer, so
    `size()` is one subtraction and a divide by a constant, as cheap as his counter. His separate
    counter is used as he asked.
- **The limit is `arguments.display.buffer`, default 131,072:**
  - a row in `satellite_config.hpp`
  - one machine may set `display.buffer = N` in `~/.satl/config.ini`; add it to S016's list of rows
    satl reads
  - read once at start-up into an `unsigned long long`
  - a program cannot change it: it is a row satl holds, and writing one is already refused (check it)
- **The crash:**
  - The printing satellite sets the overrun and `program_quit()`. Every walker stops at its next
    statement: the check that `satellite.return(satellite)` uses, so it adds nothing per statement.
  - The one that sees it first prints the critical report. satl exits with the machine code, and
    satl's own console holds on "stopped on machine code N".
  - The waiting jobs are let go, not printed.
  - Proposed: **S840 `DISPLAY_STRING_BUFFER_OVERRUN`, machine code 65.** Both are free: the first
    attempt used them and was never committed. The name is his from the first design: *"report display
    string buffer overrun"*.
  - His message, drafted from his words, **his to reword:**

    > more than 131,072 displays were waiting for the console. It is satellite's philosophy that
    > programming is not meant to overrun the console with messages faster than it can print, and this
    > program has overrun the 131,072 item buffer. Rework it so that it displays less -- or raise
    > arguments.display.buffer (display.buffer = ... in ~/.satl/config.ini).

### Step 4 — one door for everything satl prints — BUILT with step 3, build 0113

Step 3 could not keep the order without it, so it went in with it (`satellite/display/display_stream.cpp`).
One difference: **the listing's progress line flushes std::cout before it starts** instead of going
through the door -- it writes from its own thread, and a flush first keeps the order the same way.

**What changes:** `std::cout` gets a new stream buffer. Its 8 KB goes to the display thread as **one
piece**, not a hand-off per line. That covers satl's own messages, the prompt, and every library still
writing `std::cout`. The libraries share satl's libstdc++ and its `std::cout` (`048-link.mk`).

**Order:**
- Before a display job is handed over, anything waiting in `std::cout`'s 8 KB goes first.
- **A flush waits** until everything before it is written: the prompt, input, reports, the end of the
  run. `std::cerr` flushes `std::cout` before every report, so reports stay after the lines they
  explain. After an overrun, a flush does not wait.

**The places that write the screen directly:**
- the listing's progress line (`listing_progress.cpp`) goes through the door
- the two signal handlers that `_exit` stay as they are

**Failure cases:**
- A forked child (`prompt_run.cpp`) has no printing satellite, so it writes directly
  (`pthread_atfork`).
- A refused write (`| head`): the display thread remembers it. The next display on main answers
  `display_error` (S820), as today, and the prompt's `std::cout.clear()` resets it.

**Signals:**
- The printing satellite and the display thread block every signal, so Ctrl-C lands on the
  interpreter.
- SIGTTOU stays open on the display thread, so a background satl still stops at the terminal under
  `stty tostop`.

### Step 5 — straight into VTE, in satl's own window

**What changes:** when satl runs in its own console window (the in-process VTE, `window_console.cpp`),
the display thread stops writing the pty. It hands its bytes to the window's thread, which calls
`vte_terminal_feed` **once a frame**, from a tick callback. `\n` becomes `\r\n` on the way.

**Why:**
- The race: B (the bypass) had every line in VTE in 1.6 µs a print, against 3.7 µs for `std::cout`
  through the pty. But A -- step 5's own shape -- took 17.4 µs, SLOWER than the pty.
- Feeding once a frame is the fix to prove for A's 17 µs, and step 5 counts only if it does (START HERE).
- Only output skips the pty. Typed keys still come in through it.

**Everywhere else** (VS Code's terminal, gnome-terminal, a pipe, a file): the display thread writes
fd 1 in big pieces, never a line at a time.

### Step 6 — a running program's output joins before the display thread

**What changes:** satl starts no programs today, so this step builds `RUNNING_PROGRAMS.md` itself. Each
program satl then starts (posix_spawn) gets one pool thread to watch it:
- `poll()` on its output pipe, its error pipe and its pidfd
- whole lines cut out, with the `\033` rule applied to each
- the lines handed to the display thread, between the printing satellite and the screen, as he drew it
- when the pidfd says the program ended: the last partial line, then the exit code

The watcher is one of the start-up threads (`startup_threads.hpp`'s `submit()`).
**RUNNING_PROGRAMS.md's open questions 1-4, 6 and 7 still apply; 5 is answered by this design.**

---

## THE PICTURE

```
main satl ──moves the finished value + options──> PRINTING SATELLITE ──UTF-8 pieces──> DISPLAY THREAD ──> fd 1
                                                  (his buffer: held,                  (big writes)   (terminal,
std::cout's 8 KB (satl's own text) ────────────────────────────────────────────────>     ^            pipe, file)
                                                   limit, crash;                            |
                                                   text, colours,          program output: |            or VTE in
                                                   centring, UTF-8,        whole lines,  ──┘            satl's own
                                                   \033, end of line)      \033 rule                    window:
                                                                                                        fed once
                                                                                                        a frame
```

---

## OPEN — HIS TO RULE

1. **An overrun at the prompt:** stop the line and bring the prompt back (my pick, as for every other
   critical error at the prompt, and BUILT that way in 0113), or end satl.
2. **A slow reader** (`| less`, Ctrl-S, a slow window) holding 131,072 jobs crashes too. That is his
   ruling, and it stands unless he changes it. The first attempt showed it is reachable.
3. **What counts as one item:** a display job, one 8 KB piece of satl's own text, and one line of a
   program's output -- as built. **A byte cap as well?** His limit counts strings, not bytes: 131,072
   waiting strings of 1 MiB would run out of memory long before the count is reached. The first design
   capped bytes at 512 MB. His to rule.
4. **S840 / machine code 65 / the message:** above; his to reword.
5. **SECRET.md:** his, for another time.
6. **RUNNING_PROGRAMS.md 1-4, 6, 7:** the spelling, whether satl waits, keyboard input, Ctrl-C (5 is
   answered by his drawing: step 6).
7. **Someday, his:** the mini satl for printing (*"the whole 3.5 ns thing that beats compiled C++"*), and
   the option he did not take today -- the interpreter makes its own line, as 0111 did, and hands
   over whole 8 KB pieces, so nothing is paid a display.

---

## HOW TO MEASURE

```bash
# S is YOUR SCRATCHPAD (the session's, never /tmp). The times file is emptied first: a second race
# appended to the first mixes old times into the new best-of-five.
S=<scratchpad>; mkdir -p $S/before; : > $S/times
# 1. Keep the build to beat -- satl finds its word libraries BESIDE ITSELF, so copy both. BEFORE building.
cp -a build/satl build/satellite-numbers $S/before/
# 2. Alternate old and new, best of five, output to a file:
for p in lines numbers big; do
  for t in 1 2 3 4 5; do
    /usr/bin/time -f "$p before %e s" env SATL_NO_WINDOW=1 $S/before/satl SCRATCH.md/DISPLAY_THREADS/bench_$p.satl > $S/$p.before 2>>$S/times
    /usr/bin/time -f "$p after  %e s" env SATL_NO_WINDOW=1 build/satl SCRATCH.md/DISPLAY_THREADS/bench_$p.satl > $S/$p.after 2>>$S/times
  done
  cmp $S/$p.before $S/$p.after && echo "$p identical"
done
grep -E 'before|after' $S/times | sort
# 3. Instructions per display (about a minute each). --max-threads because of the 1024 start-up
#    threads: valgrind's default of 500 crashes on satl.
valgrind --tool=callgrind --max-threads=1200 --callgrind-out-file=$S/cg.out \
    build/satl SCRATCH.md/FAST_PRINTING/callgrind_display.satl > /dev/null
callgrind_annotate $S/cg.out | head -40
# 4. His VTE race, on its own headless mutter (nothing reaches the desktop):
sh SCRATCH.md/FAST_PRINTING/build_race.sh && sh SCRATCH.md/FAST_PRINTING/run_race.sh
```
