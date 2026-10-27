# TIME_PLAN — satellite.time.now, arithmetic on times, convert, and timing a capsule

Written 2026-10-26 from the author's messages and answers that day. Built on REVISION 11 BUILD 0017. Each milestone
starts with a snapshot in `~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-26-before-time-N`.

---

## 1. The author's design

His words (2026-10-26): *"satellite.variable.time second = satellite.time.now("seconds") and you can type
"nanoseconds" or "milliseconds" or "s" or "ns" or "ms" but if nothing is given just default to nanoseconds"*.

His answers the same day:
- `now()` counts from **1970**, the computer's clock. He confirmed the same for 003 on 2026-09-04: the value `now`
  answers is the clock, and `sleep` waits on the monotonic clock.
- **Keep every nanosecond.** The unit only changes how a time is shown, as it does for every time today:
  `now("s")` shows as `1793036700.123456789s`.
- **`+` and `-` on times**, and **`*` and `/` by a number**.
- **Convert:** *".satellite.time.convert("seconds") or "ns" or "nanoseconds" or "ms" or "milliseconds" or "s" will
  convert that time to another time"*.
- **Timing a capsule:** *"capsule_name.satellite.time() should run a capsule and place a time object into
  (time_object_name) and it should create that time object as well"*:

  ```
  satellite.variable.number result = divide_by_2.satellite.time(the_clock)
  satellite.console.display(the_clock)
  ```

  This displays how long `divide_by_2` took to run. *"If no name is given in time() then it just displays the result
  for you"*, in his block:

  ```
  ---------------------------- (as wide as the prompt)
  CAPSULE: capsule_name_here
  TIME: 0 ns
  -----------------------------
  ```

- **A capsule's own arguments go after the clock:** `divide_by_2.satellite.time(the_clock, 10)`.

What stays unbuilt: `satellite.time()` (1 9 0) and `satellite.time.new` (1 9 2).

---

## 2. The unit, read in one place

`now` and `convert` take the same six spellings, and nothing given means nanoseconds:

| written | the unit |
|---|---|
| `"seconds"` or `"s"` | second |
| `"milliseconds"` or `"ms"` | millisecond |
| `"nanoseconds"` or `"ns"` | nanosecond |
| nothing | nanosecond |

- They are written exactly, in lower case.
- A literal that is not one of the six is refused **before anything runs**, with S301 (exit 27). That is the code
  `satellite.file.open`'s `"append"` gets. For example:
  `satellite.time.now takes a unit -- "seconds" or "s", "milliseconds" or "ms", "nanoseconds" or "ns" -- and was given "minutes"`.
- A unit held in a string name is judged when its line runs, in the same sentence.
- Anything that is not a string is refused the same way: `now(5)`, `now(1s)`.

There is one reader for both words, in `satellite/bytecode/time_calls.cpp`, beside `satellite_time::Unit`.

---

## 3. TIME-1 — `satellite.time.now(unit)` and `.satellite.time.convert(unit)`

### now

```
satellite.variable.time second = satellite.time.now("seconds")
satellite.console.display(second)                      // 1793036700.123456789s
satellite.console.display(satellite.time.now())        // 1793036700123456789ns
satellite.console.display(satellite.time.now("ms"))    // 1793036700123.456789ms
```

- It is the time since 1970-01-01 00:00:00 UTC, read from `CLOCK_REALTIME` fresh at every call, as 003's
  `now_nanoseconds()` read `system_clock`.
- It is held exactly, as every time is: whole nanoseconds in a satellite_number.
- It answers a time, so it goes wherever a time goes: a time name, a display, a sleep, a comparison.
- **Refused before anything runs:**
  - Two or more arguments, with S110 (exit 13) and the count sentence `time_word_refused` already writes.
  - A unit that is not one of the six (Section 2).
  - `satellite.time.now` with no brackets. Today it is S210 "has no library built for it yet", and it becomes S110:
    `satellite.time.now is called with brackets -- satellite.time.now() or satellite.time.now("s")`. check.sh's M4 row
    for `now` changes to match. Its `new` row stays as it is.
- Word: `satellite.time.now` keeps row **1 9 1**. Its spelling has no brackets, so `now`, `now()` and `now(x)` all
  lex to that one code (program_check.cpp:2063). `time_word_refused` judges the count, and 0 or 1 is right.

### convert

```
satellite.variable.time waited = 1500ms
satellite.console.display(waited.satellite.time.convert("s"))    // 1.5s
waited = waited.satellite.time.convert("ns")
satellite.console.display(waited)                                // 1500000000ns
```

- It answers the same length, shown in the unit asked for. Nothing given means nanoseconds.
- It answers a new time and leaves the name as it was, as a list's `.satellite.reverse()` answers a reversed list and
  leaves the list alone. To change the name, give it back: `waited = waited.satellite.time.convert("ns")`.
- `1s == 1s.satellite.time.convert("ms")` is true.
- It is a two-part word, laid out like the window's `window.section`:
  - **Rows:** `satellite.variable.time.time`, a part that runs nothing, takes **1 6 3 2**, the next free row under the
    time. `satellite.variable.time.time.convert(unit)` takes **1 6 3 2 1**.
  - **REGISTRY.satellite:** `time.convert` becomes one `[METHOD]` token, and `time` becomes a part. Both go in the
    free codes from `0000101110100000`. On a time, `.satellite.time` written alone is refused as a part, as
    `a_word_in_parts` refuses a window's parts (window_shapes.cpp:208).
  - **`time` is the part only after `.satellite.`** (the fresh read, 2026-10-26). After a plain period it stays a
    name, as it was through BUILD 0017. So a section, file display or text input named "time", `n.time`,
    `satellite.arguments.time` and a field or capsule named `time` all behave as before. Neither TIME-1 nor TIME-3
    needs more: both write it after `.satellite.`.
  - On anything that is not a time, `.satellite.time.convert` is refused with the sentence `method_on_a_name` gives
    an unknown method.

### Help and rows

- `satellite.help/satellite.time/help_text.txt`: now, its six spellings and an example. The line "satellite.time.now
  and satellite.time.new are not built yet" becomes "satellite.time.new is not built yet".
- `satellite.help/satellite.variable.time/help_text.txt`: convert.
- check.sh, block `# TIME-1 (2026-10-26)`:
  - now() is within 5 seconds of `date +%s%N`;
  - each of the six spellings shows its unit;
  - two nows in a row are in order;
  - every refusal above, before anything runs;
  - convert in each unit, and convert compared with `==`;
  - the author's own line, `satellite.variable.time second = satellite.time.now("seconds")`.

---

## 4. TIME-2 — `+ - * /` on times

| written | answer | shown in |
|---|---|---|
| `time + time`, `time - time` | a time | the finer unit of the two: `1s - 250ms` is `750ms`, `1s + 1s` is `2s` |
| `time * number`, `number * time` | a time | the time's unit: `2 * 1.5s` is `3s`, `250ms * 4` is `1000ms` |
| `time / number` | a time | the time's unit: `1s / 4` is `0.25s`, `1s / 3` is `0.333333333s` |

- A number here is a whole number or a float, both exact.
- A digit finer than a nanosecond is cut toward zero, as the time already does (`0.5ns` is `0ns`).
- A time keeps its sign: `250ms - 1s` is `-750ms`.
- **Refused when the line runs**, with S301 (exit 27), where today's "no scenario for that pair" stands:
  - `time + number` and `time - number`. A bare number names no unit, so the sentence says "write 5s, 5ms or 5ns",
    as `sleep(5)`'s does.
  - `time * time`, `time / time` and `number / time`.
  - `%` on a time.
  - A time with a percentage, hex, binary, fraction, string or bool.
- `time / 0` is S401 `division_by_zero` (exit 22), the code a number divided by 0 gets.
- `t += 1s` stays refused, as every compound assignment is today ("write t = t + ...").
- **Where:** the walker's add, subtract, multiply and divide (expression.cpp:432, 586, 618, 641) pass a pair holding a
  time to one `time_operation`. It goes in a file of its own, `satellite/satellite_object/object_time.cpp`, the way
  `percentage_operation` answers percentages (object_percentage.cpp:52-125), chosen through `answered_by_its_own_file`
  (expression.cpp:194).
- **Help:** satellite.variable.time loses "+ - * / on times are not built yet" and gains the table, with
  `finished - started` as its example:

  ```
  satellite.variable.time started = satellite.time.now()
  work()
  satellite.variable.time finished = satellite.time.now()
  satellite.console.display(finished - started)        // 1250ns
  ```

- check.sh, block `# TIME-2 (2026-10-26)`: every row of the table, both signs, cut toward zero, every refusal, and /0.

---

## 5. TIME-3 — timing a capsule: `name.satellite.time(clock, arguments)`

```
satellite.variable.number half = divide_by_2.satellite.time(the_clock, 10)
satellite.console.display(half)          // 5
satellite.console.display(the_clock)     // 1250ns

satellite.variable.number again = divide_by_2.satellite.time(10)
```

The second line shows the block (with no clock):

```
--------------------------------------------------------------------------------
CAPSULE: divide_by_2
TIME: 1250 ns
--------------------------------------------------------------------------------
```

**What it does.** It runs the capsule and answers what the capsule hands back. It measures how long the run took.

**What is timed.**
- The run is timed on the monotonic clock, `CLOCK_MONOTONIC`, the one `sleep` waits on, which NTP never moves.
- The clock starts once the arguments are worked out and stops when the capsule ends, so working out the arguments is
  not counted.
- The time is exact, in nanoseconds.

**The clock and the block.** The count of what is written inside time() is set against the capsule's parameters:
- **One more than the parameters:** the first is the clock.
  - The clock must be a plain name.
  - **A new name** is declared by this line as a `satellite.variable.time`, in the block the line is in, from the line
    after it (`the_names_a_line_makes`, program_check.cpp:4815). It holds 0ns until the capsule ends.
  - **A time name** already declared is given the new time.
  - **A name declared as anything else** is refused before anything runs.
- **As many as the parameters:** there is no clock. The block is shown when the capsule ends.
  - The rules are as wide as the console, the figure `satellite.console.width` answers, asked fresh. They are 80 into a
    pipe or a file.
  - The block goes wherever `satellite.console.display` writes.
  - `TIME:` is the nanoseconds, with a space before `ns`, as he wrote it.
- **Any other count** is refused before anything runs, with the sentence `given_what_it_takes` writes.

**Where it can be written:** wherever a call of that capsule can be.
- It can be the value in an assignment, the argument of a display, or a line of its own when the capsule hands back
  nothing.
- When the capsule hands back nothing, using it as a value is S240, as a call is.

**When the capsule is refused while it runs,** the line is refused as the call would be, and the clock is not given a
time.

**In this milestone, a capsule named by its plain name** is the only kind that can be timed: the program's own
capsules.
- An included file's capsule, `shapes.area.satellite.time(...)`, is refused before anything runs, with a sentence
  that says so.
- So is an object's capsule, `s.area.satellite.time(...)`.
- So is a timer handed to `satellite.thread.new`.

**The word:**
- `satellite.variable.capsule.time(clock, arguments)` takes **1 6 15 1**, the next free row under the capsule.
- REGISTRY.satellite's `time` token, made a part in TIME-1, also becomes the capsule's method. What it means depends
  on what is on its left: a capsule name runs the timer, and a time is the part before `.convert`.
- Today `divide_by_2.satellite.time(x)` lexes as a name followed by the word `satellite.time` (1 9). It is refused as
  undeclared at expression.cpp:1885, and that is where the walker's timer goes in.

**The walker's path:**
- The capsule is run as `call_capsule_for_its_answer` runs one (capsule_calls.cpp:111 → `run_capsule_for`,
  program_walk.cpp:2755).
- The clock is read on each side of that run.
- The clock's name is declared as window_methods.cpp:259 declares a window's name.

**Help:** `satellite.help/satellite.capsule/help_text.txt` gets the timer, both forms and the block.

**check.sh**, block `# TIME-3 (2026-10-26)`:
- a clock made new, and a clock given again;
- the capsule's answer kept;
- arguments after the clock;
- the block, at width 80 into a pipe;
- a capsule that sleeps 50ms timed at 50ms or more;
- every refusal above, before anything runs.

---

## 6. How each milestone is built

The author's lean gate (2026-10-21):
1. Take the snapshot.
2. Build it in the real tree.
3. Add the milestone's own check.sh rows, and keep the help examples running (check_help_examples.py runs every
   block, check.sh:2189).
4. Run the full check.sh under a short TMPDIR. Only the machine's 5 may fail.
5. Have ONE fresh reader read the change.
6. Make the final fix, then run the full check.sh again.
7. Install with `MAKE=true sh satellite_enterprise/install.sh`.

Committing is a separate question to the author.

New words go at the end of `words/words_004.tsv`. Then:
1. Run `words/make_words.py`, then `satellite/bytecode/make_word_codes.py`.
2. Add new tokens to REGISTRY.satellite, then run `make_token_codes.py`.
3. Check that check.sh:5517-5537 still finds the generated headers byte-identical.

---

## 7. For the author

These are readings, to overrule with a word:
1. `+` and `-` show the finer unit of the two (`1s - 250ms` is `750ms`, not `0.75s`).
2. "As wide as the prompt" is read as the console's width (`satellite.console.width`), and 80 into a pipe.
3. `convert` answers a new time and leaves the name as it was.
4. The word numbers: convert 1 6 3 2 and 1 6 3 2 1, the capsule timer 1 6 15 1. These are the next free rows.
5. A clock or no clock is decided by the count against the capsule's parameters, so `divide_by_2.satellite.time(10)`
   shows the block and `divide_by_2.satellite.time(c, 10)` fills `c`.
6. `time` is a word only after `.satellite.`. A widget can still be named "time", unlike "window" or "section".
   Whether method words should be freed as widget names in general is still his open question G of 2026-10-10.

Open, for after TIME-3: timing an included file's capsule (`shapes.area`) or an object's capsule (`s.area`).
