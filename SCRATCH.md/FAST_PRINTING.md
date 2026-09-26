# FAST PRINTING — the plan to make satellite print as fast as it can

Written 2026-09-26, for the author and for **the session after `/clear`**. Read this whole file first.
It replaces the open design in `SCRATCH.md/DISPLAY_THREADS.md`. That file still holds the first
attempt: why it was pulled out, and every measurement it made. `SCRATCH.md/RUNNING_PROGRAMS.md`
holds the running-programs half.

**Steps 1 and 2 are built, installed and pushed** (builds 0109 `ab6e7d7` and 0111 `fbf0788`). **Step 3 is
next.** The measurements below are real. The code they came from is in `SCRATCH.md/FAST_PRINTING/`.

| best of five (step 2: of three) | 0108 | 0109, step 1 | 0111, step 2 |
|---|---|---|---|
| 1,000,000 lines | 2.34 s | 1.07 s | 1.06 s (0109 read 1.12 in that race) |
| 1,000,000 numbers | 2.10 s | 0.83 s | 0.83 s |
| 200 × 1 MiB | 0.63 s | 0.22 s | 0.23 s (noise) |

Output byte-identical in every race. `./check.sh` 1028 of 1028 after each step.

After step 1 he said: *"we are building a fast-enough-plan anyways, we gave up on racing compiled C++
with our interpreter"*. **The rule below still holds** (a slower step stops), but no C++ race is owed.

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

**Build 0108 is the build to beat.** Output to a file, best of five:

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
  - `word_codes.hpp` is generated by `make_words.py`, and check.sh compares it byte for byte, so the
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

### Step 3 — the printing satellite (the five steps)

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

### Step 4 — one door for everything satl prints

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
- The race: B had every line in VTE in 1.6 µs a print, against 3.7 µs for `std::cout` through the pty.
- Feeding once a frame is also the fix to prove for A's 17 µs.
- Only output skips the pty. Typed keys still come in through it.

**Everywhere else** (VS Code's terminal, gnome-terminal, a pipe, a file): the display thread writes
fd 1 in big pieces, never a line at a time.

### Step 6 — a running program's output joins before the display thread

**What changes:** each program satl starts (posix_spawn, `RUNNING_PROGRAMS.md`) gets one pool thread
to watch it:
- `poll()` on its output pipe, its error pipe and its pidfd
- whole lines cut out, with the `\033` rule applied to each
- the lines handed to the display thread, between the printing satellite and the screen, as he drew it
- when the pidfd says the program ended: the last partial line, then the exit code

The watcher is one of the start-up threads (`startup_threads.hpp`'s `submit()`).
**RUNNING_PROGRAMS.md's open questions 1–7 still apply.**

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
   critical error at the prompt), or end satl.
2. **A slow reader** (`| less`, Ctrl-S, a slow window) holding 131,072 jobs crashes too. That is his
   ruling, and it stands unless he changes it. The first attempt showed it is reachable.
3. **What counts as one item:** a display job, one 8 KB piece of satl's own text, and one line of a
   program's output.
4. **S840 / machine code 65 / the message:** above; his to reword.
5. **SECRET.md:** his, for another time.
6. **RUNNING_PROGRAMS.md 1–7:** the spelling, whether satl waits, keyboard input, Ctrl-C.

---

## HOW TO MEASURE

```bash
# 1. Keep the build to beat -- satl finds its word libraries BESIDE ITSELF, so copy both.
mkdir -p /tmp/before && cp -a build/satl build/satellite-numbers /tmp/before/
# 2. Alternate old and new, best of five, output to a file:
for p in lines numbers big; do
  for t in 1 2 3 4 5; do
    /usr/bin/time -f "$p before %e s" env SATL_NO_WINDOW=1 /tmp/before/satl SCRATCH.md/DISPLAY_THREADS/bench_$p.satl > /tmp/$p.before 2>>/tmp/times
    /usr/bin/time -f "$p after  %e s" env SATL_NO_WINDOW=1 build/satl SCRATCH.md/DISPLAY_THREADS/bench_$p.satl > /tmp/$p.after 2>>/tmp/times
  done
  cmp /tmp/$p.before /tmp/$p.after && echo "$p identical"
done
grep -E 'before|after' /tmp/times | sort
# 3. Instructions per display (about a minute each). --max-threads because of the 1024 start-up
#    threads: valgrind's default of 500 crashes on satl.
valgrind --tool=callgrind --max-threads=1200 --callgrind-out-file=/tmp/cg.out \
    build/satl SCRATCH.md/FAST_PRINTING/callgrind_display.satl > /dev/null
callgrind_annotate /tmp/cg.out | head -40
# 4. His VTE race, on its own headless mutter (nothing reaches the desktop):
sh SCRATCH.md/FAST_PRINTING/build_race.sh && sh SCRATCH.md/FAST_PRINTING/run_race.sh
```
