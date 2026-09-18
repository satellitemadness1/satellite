# satellite-004 — SATELLITE_ERROR

The error system: what an S-code is, what a report looks like, which pieces are
built, and what each unbuilt piece costs.

Written 2026-09-18 from the author's briefs, which are kept whole in Parts 1 and 7.

**PART 7 IS THE FULL REPORT** — thirteen sections holding everything the interpreter
knows, four of which need nothing built. **PART 10 IS THE FEATURE REGISTER** — one
binary and `satl --rebuild`, measured, and the reason the whole system is free when
it is off. **PART 11 IS WHAT WE ARE NOT DOING.**

**THIS IS NOT `ERROR.md`.** That file is the list of *bugs in 004* — things that
give a wrong answer, and where they are. This one is the *machinery a failure is
reported through*. A bug goes in ERROR.md; the shape it is told in goes here.

---

# Part 1 — the brief, as the author wrote it

> and don't run the interpreter without /home_dir/.satl/config.ini say "cannot
> find /home/username/.satl/config.ini" please reinstall satellite, or create a
> config.ini there... and well, let's build default config values so the
> interpreter still runs without the config.ini file, we'll set some default
> config values, and then our critical error report will look like this
>
> ```
> ------------------------------------------------------------------------ (80 chars)
> SATELLITE CRITICAL ERROR REPORT
> ------------------------------------------------------------------------
> S0721: BASIC_ERROR_NAME
> FULL_ERROR_DESCRIPTION_HERE
>
> directory: path
> syntax: code here
>                      /\
>                    however you had 003
>
> ------------------------------------------------------------------------
> ```
>
> and that ends the critical error report we will give them, it'll look like it
> came out of some space kinda novel or something!

> and then before the last ------- line, add anything else you want to add that
> we can, we could have super huge error reports, in fact, let's build a
> SATELLITE_ERROR.md whenever your done doing what your doing, and we'll build an
> entire error system...

---

# Part 2 — what is built, as of 2026-09-18

- **The renderer is built and running** —
  [satellite/machine/critical_report.hpp](satellite/machine/critical_report.hpp).
  Header-only, so a numbered library can raise a report too (each library is
  compiled from exactly one `.cpp`, so anything shared has to be inline).
- **`CriticalReport` carries** `code`, `name`, `description`, `directory`,
  `syntax`, `caret_at`, `caret_note`, and `notes` — the author's "anything else"
  room, kept last.
- **One caller exists**: `S0721 CONFIG_FILE_MISSING`, in
  `structured-library.cpp`, raised when `$HOME/.satl/config.ini` is absent.
- **Everything else still reports the old way** — `report_error()` in
  `machine/machine_state.cpp`, one line to stderr:
  `[satellite] satl(run): ... (machine_code: 34 setting_is_not_a_flag)`.

## The two numbering schemes, and they are not rivals

| | what it is | where it lives | range |
|---|---|---|---|
| **machine code** | what a function *answers*; becomes the exit status | `machine/machine_codes.hpp` | 0–254, 130 is Ctrl-C |
| **S-code** | what a *person* is shown in a report | this file, Part 4 | S0001– |

A machine code is the interpreter talking to itself and to the shell. An S-code
is the interpreter talking to a person. **They are many-to-many on purpose**:
`satl_line_not_understood` (13) is a hundred different things a person needs a
hundred different sentences for, and one S-code may be reachable from more than
one machine code. A report carries both — the S-code in its header, the machine
code in the exit status.

**003'S S-CODES ARE NOT PORTED WHOLESALE.** 003 numbered its own (`S0714`,
`S0723`, `S1001`) against a different set of refusals. Where a 004 refusal *is*
003's refusal, it takes 003's number so a person moving between them reads one
number; where it is new, it takes the next free one in its block. The author's
sketch used `S0721`, which is why `CONFIG_FILE_MISSING` is S0721.

---

# Part 3 — the `syntax:` row, and what it costs

The report has a `syntax:` row and a caret. **Nothing fills them in yet**, and
this section is why that is a small job rather than a large one — it was measured
against the tree on 2026-09-18 rather than guessed.

- **The file name is already there.** `BytecodeFilenames` is a
  `std::vector<std::string>` kept row for row with the registry
  (`bytecode_registry.hpp`), and **one row is one FILE** — not one line. So
  `filenames[which_row]` is the file, free.
- **The line number is a count, and it is exact.** `line_end_token` (`0x0100`) is
  put at the end of every tokenised line, and `add_file_to_bytecode_registry`
  splits the source on newlines and tokenises **every** line — blank lines and
  comment lines included. So the number of `line_end_token`s before a position
  `at` is that position's 0-based source line, with no drift.
- **The line's TEXT is not kept, and should not be.** The registry holds codes,
  and the header says why: *"a row stays nothing but codes and a name never has
  to be spelled in tokens to be carried."* **Re-read the file at report time.**
  An error is rare and the path is already failing, so one `open`+`getline` costs
  nothing — and it shows the person's *actual* text, not a reconstruction from
  tokens that would quietly differ in spacing from what they wrote.
- **The caret column is the one real piece of work.** A token's position in the
  stream is not a column in the line. Either the lexer records a byte offset per
  token, or the reporter re-tokenises the one line it re-read and counts. The
  second costs nothing at run time and adds no field to the hot path, and is the
  recommendation.

---

# Part 4 — the S-code register

Blocks, so a number never has to move. **Add at the end of a block; never
renumber.** Same rule as the word table, for the same reason.

| block | what it is for |
|---|---|
| **S00xx** | starting up: config, libraries, the machine |
| **S01xx** | the command line |
| **S02xx** | reading a file: it is not there, not readable, not satellite |
| **S03xx** | the lexer: a character or a token that cannot be read |
| **S04xx** | the checker: shapes refused before anything runs |
| **S05xx** | names: declared twice, never declared, out of scope |
| **S06xx** | types: two kinds that do not meet |
| **S07xx** | settings and arguments |
| **S08xx** | numbers: division by zero, an answer that is not whole |
| **S09xx** | strings, binary, percentage |
| **S10xx** | input and the console |
| **S11xx** | threads and memory |
| **S12xx** | directories and files a program opens |

## Assigned

| S-code | name | machine code | where |
|---|---|---|---|
| **S0721** | `CONFIG_FILE_MISSING` | *(notice — the run carries on)* | `structured-library.cpp` |

## Owed — the refusals that exist and have no S-code yet

Each of these already answers a machine code and already prints the one-line
form. They are the first candidates for the report, in the order a person is
likeliest to meet them:

| machine code | what a person did | block |
|---|---|---|
| 34 `setting_is_not_a_flag` | `arguments.access = 5` | S07xx |
| 35 `word_takes_no_assignment` | `satellite.console.display = ...` | S07xx |
| 25 `name_not_declared` | used a name with no `satellite.variable` line | S05xx |
| 26 `name_declared_twice` | two declarations of one name in one capsule | S05xx |
| 27 `types_do_not_meet` | `"a" - "b"`, or a number given to a string | S06xx |
| 22 `division_by_zero` | a divisor of 0 | S08xx |
| 24 `answer_is_not_whole` | `2 ^ -1` | S08xx |
| 14 `not_built_yet` | a word that is numbered and has no library | S04xx |
| 13 `satl_line_not_understood` | everything the checker has no shape for | S04xx |
| 32 `config_file_unwritable` | a setting could not be saved | S07xx |
| 33 `config_file_unreadable` | config.ini is there and unreadable | S07xx |

---

# Part 5 — the milestones

**E1–E6 make the report reachable.** E7–E12 move the refusals onto it.

## Phase 0 — the position

- **E1** — Record a byte offset per token, or re-tokenise one line at report time; pick the second unless the first is free.
- **E2** — `line_of(row, at)`: count `line_end_token` before `at`.
- **E3** — `source_line(file, n)`: re-read the file, answer that line's text.
- **E4** — `column_of(row, at)`: the caret's position inside that text.

## Phase 1 — the seam

- **E5** — `raise(CriticalReport)`: one call that renders, prints and answers the machine code.
- **E6** — A report knows its file and position from `ExpressionContext`, so a caller passes neither.

## Phase 2 — the refusals, one at a time

- **E7** — S07xx: the two setting refusals, 34 and 35. They are the newest and the smallest.
- **E8** — S05xx: `name_not_declared` and `name_declared_twice`.
- **E9** — S06xx: `types_do_not_meet`, which is the one a person meets most.
- **E10** — S08xx: `division_by_zero` and `answer_is_not_whole`.
- **E11** — S04xx: `not_built_yet` names the word and says which milestone owes it.
- **E12** — S03xx: the lexer's `error_token`, which already carries the character it could not read.

## Phase 3 — the room at the bottom

The author: *"we could have super huge error reports"*. `notes` is already there
and already prints; these are what to put in it.

- **E13** — The nearest spelling, when a word is misspelled.
- **E14** — The line above and the line below, when they explain the one that failed.
- **E15** — What 003 did with the same program, where 003 is installed and did something different.
- **E16** — `satl --help <S-code>`: the whole entry for one code.

---

# Part 7 — THE FULL REPORT: everything the interpreter knows

The author, 2026-09-18:

> what all can we pack into SATELLITE_ERROR.md do you think? We can display a
> full error report with every argument that arguments was at even if it wasn't
> active, and we need to build the interpreter so it has access to arguments no
> matter what, only the user doesn't have access to it if it's not defined as one
> of the names, and what else can we pack into the error report? I am imagining a
> HUGE list of values and information so that we shut the interpreter down and the
> user is left with the entire error report, like so they can see EVERY VARIABLE
> every STATEMENT, EVERYTHING the interpreter knows, and we can use this as a
> debug tool anyways, so it will facilitate development

And, in the same breath: *"we are feature creepin' hard"*. He is right, and this
is the creep that pays for itself — **every section below is a thing a person
building the interpreter wants to see anyway**, so the crash report and the
debugger are one piece of work rather than two.

## 7.1 — The one that is already true

**THE INTERPRETER ALREADY GATHERS EVERY ARGUMENT, UNCONDITIONALLY.**
`structured-library.cpp` calls `arguments.gather_config()` and
`arguments.gather(command_line)` at start-up on every run, before anything about
`arguments_active` is consulted. Run `satl --debug` on any program and thirty
rows print. So the author's requirement — *"every argument that arguments was at
even if it wasn't active"* — **costs nothing and needs nothing built**.

**WHICH INVERTS A1–A3 IN SATELLITE_ARGUMENTS.md, AND MAKES THEM CHEAPER.** The
first brief had `arguments_active` gate whether the interpreter *looks for*
arguments at all, for speed. The second makes it gate whether the **user** may
read them: *"only the user doesn't have access to it if it's not defined as one
of the names"*. That is a **visibility gate, not a gathering gate** — one test in
the expression reader, nowhere near the hot path, and the crash report always has
the data because the data was always collected.

## 7.2 — The inventory: what exists, and what each section costs

Thirteen sections. The cost column is the honest one — **free** means the
interpreter already holds it and the report only has to print it.

| § | section | where it comes from | cost |
|---|---|---|---|
| A | **identity** — version, revision, build, compiler, flags, OS, pid, started at, ran for | `version.hpp`, `arguments.*` | **free** |
| B | **arguments** — all ~30 rows, name, kind, value, and WHERE each came from (config / command line / machine) | `Arguments::all()` | **free** |
| C | **the failure** — S-code, machine code, name, description, file, line, column, the source line, the caret | the report itself + Part 3 | small |
| D | **the call stack** — capsule → capsule, innermost first | **needs a frame stack** | medium |
| E | **variables** — every name in every live frame: name, declared type, current value | `VariableTable` per frame | **free per frame**, needs D for all of them |
| F | **variables that are GONE** — every name the run ever held, last type, last value | **the last-known store** | see 7.3 |
| G | **statements** — the last N statements run, innermost frame first | **needs a ring** | see 7.4 |
| H | **the program** — every file loaded, which was an include and by whom, token count each | `BytecodeFilenames`, registry | **free** |
| I | **the words** — 28 of 367 have a library; whether the failing word had one, and which are owed | `FunctionTable` | **free** |
| J | **the machine** — memory total/free/used, swap, cores, threads, disk, page size | `arguments.cpp` readers | **free** (Phase C of ARGUMENTS) |
| K | **this interpreter** — resident memory, threads started/parked, how much of `threads_max` used | `/proc/self/statm`, `StartupThreads` | small |
| L | **the config** — config.ini's path, whether it was there, every key, and which settings took defaults | `config_file.hpp` | small |
| M | **what to do next** — nearest spelling, the milestone that owes the word, `satl --help <S-code>` | E13–E16 | medium |

**FOUR OF THE THIRTEEN NEED NOTHING BUILT** and are the whole of milestone R3.
That is the order: print what is already known first, so the report is useful
before any of the expensive sections exist.

## 7.3 — §F is the last-known store, and that is the good news

**THE CRASH REPORT'S BEST SECTION AND `satellite.access(object_name)` ARE THE
SAME MACHINERY.** SATELLITE_ARGUMENTS Part 4B describes a store keeping *the last
known name, type and value* of everything, so a person can ask about an object
after the program has stopped. That is **exactly** §F, and `arguments.access` —
already built, already lasting in config.ini — is **already its valve**.

So neither feature pays for it alone:

- `satellite.access()` gets its store,
- the report gets every variable including ones out of scope,
- and one `arguments.access = satellite.bool.false` turns off the cost of both.

**AND IT IS BOUNDED BY NAMES, NOT BY ASSIGNMENTS**, which is what makes it
affordable at all — "only the last" means one row per name, so a variable written
ten million times in a loop is ten million writes to one row, not ten million
rows. That bound is the whole reason the author's "only the last" is the right
design and not a limitation.

## 7.4 — §G is the one to be careful with

*"every STATEMENT"* read literally is unbounded: a loop running a billion times
is a billion statements. **A ring buffer of the last N**, not a log.

**THIS IS THE CONSOLE QUEUE AGAIN.** On 2026-09-17 an unbounded queue of
already-rendered output took 5.9 GB in 48 seconds and would have had the machine.
Anything appended once per statement is on the hottest path there is, so §G is
the section that needs its bound decided **before** it ships, and needs measuring
rather than arguing. N wants to be a config.ini key.

## 7.5 — Where a huge report goes

**NOT ALL OF IT TO THE TERMINAL.** A two-thousand-line dump into stderr scrolls
the thing a person needs off their screen, and if they were piping the program
it lands in the pipe. So:

- **stderr** gets the frame — the eighty-column report of Part 1, §A–§C — which
  is what a person reads to know what happened;
- **`~/.satl/reports/<when>-<S-code>.txt`** gets the whole thing, every section;
- and the last line on stderr says where it was written.

That also answers *"we shut the interpreter down and the user is left with the
entire error report"* better than printing it would: the file is still there
tomorrow.

## 7.6 — Three rules the report has to keep

1. **IT MUST NOT FAIL WHILE REPORTING A FAILURE.** The report runs at the worst
   moment there is, and the worst moment includes *out of memory* — which is the
   exact state the console queue put this machine in. A reporter that allocates
   to describe an allocation failure is a reporter that is not there when it is
   needed. **Its buffer is taken at start-up and never grown.**
2. **IT MUST NOT LEAK WHAT IT WAS NOT ASKED TO.** A report is made to be pasted
   into a bug entry. The environment holds tokens, keys and passwords, so
   `environ` is **never** dumped wholesale — named variables only, and a list
   that is written down rather than a sweep.
3. **IT MUST SAY WHAT IT COULD NOT READ.** A section that failed to gather prints
   as that, never as empty. An empty `variables:` and a variables section that
   could not be walked look identical, and one of them is a second bug.

---

# Part 8 — the full report's milestones

**R1–R6 make the report exist and print what is already known.** R7–R12 are the
sections that need building. R13–R18 are the debugger the author wants it to be.

## Phase R0 — the frame, and the four free sections

- **R1** — `ReportBuffer`: taken at start-up, fixed size, never grown. Rule 1 of 7.6.
- **R2** — `--report=short|full`, and a config.ini key for the default. Short is today's eighty-column frame.
- **R3** — §A identity, §B arguments, §H the program, §I the words. All four are printing what is already held.
- **R4** — Write the full report to `~/.satl/reports/<when>-<S-code>.txt`; stderr's last line says where.
- **R5** — §L the config: every key, its value, and which settings fell back to a default.
- **R6** — §J the machine and §K this interpreter. §J lands free once ARGUMENTS Phase C is built.

## Phase R1 — the frame stack

Today a capsule call makes `VariableTable theirs;` with no link to its caller
(`program_walk.cpp`), so **only the innermost body's variables can be reached**.
This phase is what makes §D and §E whole.

- **R7** — A `Frame`: the capsule's name, the file and line it was called from, and its `VariableTable`.
- **R8** — A stack of them, pushed and popped around a capsule call. **Measure the cost of the push before keeping it.**
- **R9** — §D prints the stack, innermost first, the way a person reads a traceback.
- **R10** — §E prints every frame's variables, with each value rendered by the same code `display` uses, so the report and the program agree about what a value looks like.

## Phase R2 — the two stores

- **R11** — §F, the last-known store: one row a NAME — last type, last value, last file and line that wrote it. **This is SATELLITE_ARGUMENTS' store, and `arguments.access` is already its valve.**
- **R12** — §G, the statement ring: the last N statements, N from config.ini. **Bound first, measured second, shipped third** (7.4).

## Phase R3 — the debugger it turns into

The author: *"we can use this as a debug tool anyways, so it will facilitate
development"*. Everything above is collected whether or not anything failed, so
these cost almost nothing more.

- **R13** — `satl --report <program.satl>`: run it, and write the full report at the end whether it failed or not.
- **R14** — `satellite.report()`: a program asks for its own report, mid-run, without stopping.
- **R15** — A SIGSEGV/SIGABRT handler that writes the report. **There is none today** — a crash gives nothing at all — and this is the case a person is most helpless in.
- **R16** — Ctrl-C writes one, so a hung program says where it was hanging.
- **R17** — `satl --report-diff <a> <b>`: two reports, what changed. Two runs that differ by one value is the commonest debugging question there is.
- **R18** — §M: the nearest spelling, and the milestone that owes an unbuilt word.

## Phase R4 — the report as a satellite value

- **R19** — The report as a `satellite.container.arguments`, so a program can read its own state.
- **R20** — `satellite.access(object_name)` answers out of §F's store. **This is the second brief's whole purpose, and by here it is one line.**

---

---

# Part 10 — THE FEATURE REGISTER: one binary, and `satl --rebuild`

The author, 2026-09-18:

> those are things that we actually have to build that will also slow down the
> interpreter slightly, we just have to hide all of them behind
> satellite.variable.bool's so it only slows the interpreter down by a nanosecond
> while it checks all of the values, we could technically hide all of these into
> something that we build -- a satellite.variable.binary where each value in the
> width is a feature, then we can just scan this single binary and it flips on or
> off all of the features, this is how we'll keep the interpreter really fast!

> so we do this... satl --rebuild is the command to rebuild the arguments string,
> so we have to set the arguments how we want them, then run satl --rebuild to
> rebuild the binary string that is stored inside of config.ini, it saves it and
> then it just loads that single value...

**THE IDEA IS RIGHT AND THE REASON IS NOT THE ONE IT LOOKS LIKE.** Measured
2026-09-18, clang 24, -O2, this machine, 2,000,000,000 statements, all eight
features OFF (the common case), two runs agreeing to the third digit:

| how the eight features are gated | ns per statement | over the floor |
|---|---|---|
| **no gates at all** (the floor) | **0.224** | — |
| eight separate `bool`s | 1.81 | +1.59 |
| **one bitmask, eight bit tests** | **1.84** | **+1.62** |
| one bitmask, **one** test per statement | 0.72 | +0.50 |
| **split loop, tested ONCE on entry** | **0.225** | **+0.001** |

Read the middle row twice. **A bitmask tested bit by bit is not faster than eight
separate bools** — 1.84 against 1.81, which is noise. It cannot be: either way
the processor runs eight compare-and-branches, and a branch that is always false
is already free to predict. Scanning one binary instead of eight bools does not
remove the eight tests; it only changes where the bits are stored.

**WHAT THE ONE BINARY ACTUALLY BUYS IS THE ROW BELOW IT.** Because all eight
features live in ONE value, one test can gate ALL of them:

```
    if (flags != 0) { ... the eight checks ... }
```

That is 0.72 ns against 1.84 — **two and a half times cheaper**, and it gets
cheaper the more features are added, because the number of tests stops growing
with the number of features. **That is the author's idea, and it is a good one.**

**AND THE LAST ROW IS THE ONE TO BUILD.** Hoist the test out of the loop
entirely: two walker loops, one plain and one instrumented, chosen ONCE.

```
    if (flags == 0) run_statements_plain(...);     // no check in the loop at all
    else            run_statements_watched(...);
```

**0.225 ns against the 0.224 ns floor — the features cost NOTHING when they are
off.** Not a nanosecond, not a branch: the checks are not in the compiled loop.

At a billion statements that is the difference between **1.6 seconds of pure
overhead and none**. For a language whose client reads corpora, that is the whole
argument.

## 10.1 — `satl --rebuild`, which is exactly the right shape

The author's command is what makes the split affordable, because it moves every
cost to a moment nobody is timing:

1. A person sets the features they want — settings, the way `arguments.access`
   already works.
2. **`satl --rebuild`** composes them into ONE binary and writes it to
   `config.ini` as a single value.
3. Start-up reads **that one value** — one parse, one integer, once per run.
4. The walker branches on it **once** and then runs a loop with no checks in it.

So the per-run cost of the whole debug system, with everything off, is: one line
read from config.ini, and one integer compare. **And the per-statement cost is
zero.**

**`--rebuild` IS ALLOWED TO BE SLOW, AND THAT IS THE POINT.** The author:
*"this way it remains fast, and the --rebuild takes a long time only, that is
what is slow"*. Nothing it does is on any path a person waits on twice, so it may
do work that would be unthinkable per run — validate every bit against the
setting it came from, walk every library and check the features it claims,
re-measure the machine, write a human-readable table of what it turned on. **A
step that runs once per machine has no budget**, and the only mistake available
is putting something in start-up that belongs here.

**AND IT IS THE SECOND COMMAND OF THAT SHAPE, WHICH MEANS IT IS A PATTERN.**
SATELLITE_ARGUMENTS Phase 8 has `satl --config`: a thread probe that takes 9.6
seconds and is *"unthinkable at every startup and nothing at all once per
machine"*. Same argument, same answer, and the two are siblings — `--config`
measures what the MACHINE can do, `--rebuild` composes what the PERSON asked for.
They write to the same config.ini and should agree about how: whether they are
one command with two halves, or two commands, is red note 13.

## 10.2 — The rules the register has to keep

1. **A BIT'S MEANING NEVER MOVES. APPEND ONLY.** This is the words.tsv rule
   again, and it bites harder here: a `config.ini` written by an older build
   holds a number, and if bit 5 stopped meaning "trace" and started meaning "dump
   memory", that old number silently turns on the wrong feature. A bit is
   retired by being **abandoned**, never reused.
2. **THE FAST PATH IS A `uint64_t`; THE SATELLITE VALUE IS A BINARY.** satellite
   has no limits and `satellite.variable.binary` is any width — but the walker's
   one compare has to be a register compare, not a bignum compare. So: 64 bits
   for the features the WALKER tests, and the satellite-visible binary may be
   wider for everything else. If 64 is ever not enough for hot features, a second
   word is one more compare, not a redesign.
3. **READ-ONLY AFTER START-UP, OR IT IS A DATA RACE.** `arguments.threads_startup`
   is 1,024 threads. A flag a running program can flip is a value 1,024 threads
   read while one writes. Either the register is fixed once at start-up — which
   is what `--rebuild` naturally gives — or every read is atomic and the split
   loop above is impossible.
4. **THE REPORT PRINTS THE REGISTER, SPELLED OUT.** A report gathered with half
   the features off is a report with holes in it, and a person reading it has no
   way to tell a section that was empty from a section that was never collected.
   §A of Part 7 carries the register as bits AND as names.
5. **`arguments.access` IS ALREADY A SETTING AND HAS TO BECOME BIT 0.** It is
   built, it lasts in config.ini, and it is the valve for the last-known store.
   Either `--rebuild` folds it into the register, or there are two mechanisms for
   one job — which is the thing R6 of SATELLITE_ARGUMENTS refused for arguments
   and should refuse here.

---

# Part 11 — WHAT WE ARE NOT DOING

The author asked: *"tell me about what we are not doing"*. Honestly, then —
fourteen things, with the ones that are nearly free marked, because those are the
ones being left on the table for no reason.

## 11.1 — A defect in this file's own plan

**THE `syntax:` ROW CAN LIE, AND PART 3 IS WHY.** Part 3 recommends re-reading
the source file at report time rather than keeping the text. That is right for
cost and it has a hole nobody has closed: **if the file changed since it was
loaded, the line printed under `syntax:` is not the line that ran.** A person
editing while a long program runs gets a caret pointing at the wrong code, and
nothing says so. The fix is small — hash or stat the file at load, check it at
report time, and say "the file changed since this ran" instead of printing a line
that is not true. **It is written here because it is my own plan's flaw, found
while answering this question rather than by somebody debugging at 3am.**

## 11.2 — The ones that are nearly free

Each of these is cheap *because* something already planned is being built, and
each costs nothing when its bit is off.

| what | why it is nearly free |
|---|---|
| **Per-word call counts** — how many times each of 367 words ran | The word's code IS an array index (`function_table.hpp`). One `++counts[code]` |
| **Coverage** — which statements never ran at all | One bit per statement position; the statement ring already walks there |
| **Watchpoints** — stop when a named variable changes | The last-known store (§F) already intercepts every write |
| **The reproduction command** — the exact line to run it again, with the cwd | Every piece is already in `Arguments` |
| **Timing per capsule** — where the time went | The frame stack (R7) is already push/pop; a clock read is one instruction |
| **`satellite.assert(x)`** — a program raises its own report | The report exists; this is a word and a library |

## 11.3 — The ones that are real work, and worth it

- **A STEP DEBUGGER.** Break at a line, step one statement, look. The walker
  already keeps a position and the frame stack gives the rest. This is the single
  biggest debugging win not in Parts 5 or 8, and it is *reachable* — most of its
  machinery is being built for the report anyway.
- **A POST-MORTEM PROMPT.** 004 already has `--repl`. Dropping into it **at the
  point of failure, with that frame's variables loaded**, turns every crash into
  an investigation instead of a report to read. The two pieces exist separately
  and have never been joined.
- **DETERMINISTIC REPLAY.** Record what came in — `console.input`, file reads,
  the random seed, the clock — so a failing run can be run again identically. For
  a language with 1,024 threads and a PCG generator this is the difference
  between a bug that is fixed and a bug that is "not reproducible".
- **A THREAD SECTION.** Part 7's §K counts threads. It does not say **what each
  one is doing**, who is waiting on whom, or which of them was the one that
  failed. With threads as the point of the language, a report with a thread
  *count* is half a report.
- **MEMORY ACCOUNTING — WHAT IS HOLDING IT.** Not how much this process uses
  (§K), but **which variables and which structures**. On 2026-09-17 a queue took
  5.9 GB in 48 seconds and finding it took a measurement rig; a report that named
  the biggest holder would have said it outright.
- **A STREAMING TRACE.** The statement ring (§G) keeps the last N for a crash. A
  trace writes **all** of them to a file as they happen, which is the other
  question — not "what was it doing when it died" but "what did it actually do".
  Different bit, different cost, same collection point.
- **WHAT KILLED IT.** If the OOM killer took the process, or a signal did, the
  report should say so rather than not existing. `/proc/self/status` and a
  handler; related to R15 and red note 7.
- **A STABLE, PARSEABLE FORM.** R17 proposes `--report-diff`. Diffing only works
  if the report has a fixed order and a machine-readable shape — decided now, or
  every tool built on it later fights the formatting.

---

# Part 12 — the milestones for Parts 10 and 11

**F1–F8 are the feature register.** They come first: every debug feature below
hides behind it, and building the features before the register means retro-fitting
each one.

## Phase F — the register

- **F1** — `FeatureRegister`: one `uint64_t`, one named bit per feature, **append-only**, with the rule written above it.
- **F2** — Read it from config.ini at start-up: one value, one parse.
- **F3** — `satl --rebuild`: compose the settings into the binary, write it, print what it turned on.
- **F4** — Fold `arguments.access` in as bit 0, so there is one mechanism and not two (rule 5).
- **F5** — **Split the walker**: `run_statements_plain` and `run_statements_watched`, chosen once. **This is the milestone the 0.225 ns depends on.**
- **F6** — Re-measure with a real program after F5. `experiments/energy/release.satl` is about a million statements a second and is the shape that would show any regression.
- **F7** — §A of the report prints the register as bits AND as names (rule 4).
- **F8** — The register is fixed after start-up; a program that tries to write one says so (rule 3).

## Phase G — the nearly-free ones (11.2)

- **G1** — Per-word call counts: `++counts[code]`, and a report section ordered by count.
- **G2** — Coverage: one bit per statement, and a section naming statements that never ran.
- **G3** — Watchpoints, off the last-known store's write path.
- **G4** — The reproduction command line, in the report.
- **G5** — Timing per capsule, off the frame stack.
- **G6** — `satellite.assert(x)`: a word, a library, and the report it raises.

## Phase H — the real work (11.3)

- **H1** — The file-changed check (11.1). **Small, and it closes a hole this file opened.**
- **H2** — What killed it: signal, OOM killer, exit status.
- **H3** — The thread section: what each thread was doing.
- **H4** — Memory accounting: which variables hold the most.
- **H5** — The streaming trace, its own bit.
- **H6** — A stable, parseable report form, and `--report-diff` on top of it.
- **H7** — Deterministic replay: record the inputs and the seed.
- **H8** — The step debugger: break, step, inspect.
- **H9** — The post-mortem prompt: `--repl` at the point of failure, that frame loaded.

---

# Part 13 — left to the author

Red notes. None of them blocks E1–E6, R1–R6 or F1–F5.

1. **Does a NOTICE use the same block as a failure?** S0721 is not fatal and
   prints the identical frame. A person may read two rules of dashes as "this run
   is over" — and that reading is wrong exactly once, on the commonest report
   there is. A `SATELLITE NOTICE` header on the same frame would fix it and would
   be a second shape to keep.
2. **Do both numbers show?** The report shows the S-code; the one-line form shows
   the machine code. A person who gets a report and then reads `echo $?` sees two
   unrelated numbers for one failure, and nothing on screen connects them.
3. **How many reports does one run print?** A refusal inside a loop can print the
   same 16-line report every turn. 003's warning log kept every warning for the
   whole run and drained at the end; the console queue's lesson from the same
   night is that anything per-iteration needs a bound before it is shipped, not
   after.
4. **Is `S` followed by four digits enough?** Twelve blocks of a hundred is 1,200
   codes. 003 reached S1001, so the shape is proven; the question is only whether
   a block ever needs more than a hundred.

## From the full-report brief, 2026-09-18

5. **What is N for the statement ring (§G, R12)?** *"every STATEMENT"* read
   literally is unbounded — a loop running a billion times is a billion
   statements. A ring of the last N is the answer, and N is a number only the
   author can pick, because it is a trade he owns: 64 is a page and tells you
   almost nothing about a loop; 100,000 is megabytes on the hottest path in the
   language. **This is the one number in Part 8 that has to be measured rather
   than chosen**, and it is the one most likely to hurt if it is guessed.
6. **Does the statement ring cost anything when nothing fails?** It is written
   once per statement whether or not there is ever an error, which is the
   definition of a hot path. `arguments.access = false` turns it off — but the
   default is ON, on this machine, on the author's instruction. **Measure before
   and after with a real program** (`experiments/energy/release.satl` is a
   million statements a second and is exactly the shape that would show it).
7. **Should a crash report be written by a SIGSEGV handler at all (R15)?** It is
   the case a person is most helpless in, and it is also the case where the
   interpreter's own structures may be the thing that is broken — walking a
   corrupt frame stack inside a segfault handler turns one crash into two and
   loses the report. 003's watchdog met the same question and answered `_exit`
   with no destructors, deliberately. The safe version writes only the sections
   that need no pointers chased (§A, §B, §C, §L) and says the rest were skipped.
8. **Where does `~/.satl/reports/` get cleaned up?** R4 writes a file per
   failure. A person debugging a loop makes hundreds in an afternoon, and nothing
   in Part 8 ever deletes one. Keep the last N, keep N days, or keep them all and
   say so — but it is a decision, and an unbounded folder is the same bug as an
   unbounded queue with a slower fuse.
9. **Which environment variables are named in §M?** 7.6's rule 2 forbids dumping
   `environ`, because a report is made to be pasted into a bug entry and the
   environment holds tokens and keys. So there is a written list: `HOME`, `PATH`,
   `PWD`, `SHELL`, `TERM`, `LANG`, `SATELLITE_*`. The author should say whether
   that list is right before it ships, because adding to it later is easy and
   taking something out of a report somebody already pasted is impossible.

## From the feature-register brief, 2026-09-18

10. **How many bits does the WALKER get?** Rule 2 of 10.2 puts the hot features
    in one `uint64_t` so the split test is a register compare, and leaves the
    satellite-visible value a full-width binary. Sixty-four hot features is a lot
    — Phases G and H together want about fifteen — but the number has to be said
    out loud once, because a 65th hot feature is a second compare in the branch
    that Part 10 measured at 0.001 ns, and nobody should discover that by adding
    one.
11. **Can a program turn a feature on mid-run?** Rule 3 says no, and says why:
    1,024 threads reading a value one thread writes is a data race, and the split
    walker is impossible if the answer can change inside the loop. But it forbids
    something genuinely useful — `satellite.trace_on()` around one suspect
    capsule. A middle answer exists: the register is fixed, and the SPLIT is
    re-chosen at a capsule boundary, where no loop is in flight. That is a real
    design and it is not free, so it is the author's.
12. **What does `--rebuild` do with a bit this build does not know?** A newer
    config.ini read by an older satl holds bits with no meaning here. Ignore them
    silently, refuse, or say so and carry on — and the same question upside down
    for an older file read by a newer satl, which is the ordinary case after
    every upgrade.
13. **Is `--rebuild` the same command as `--config`?** They are siblings: both
    run once per machine, both may be slow, both write config.ini. `--config`
    measures what the MACHINE can do (the 9.6-second thread probe,
    SATELLITE_ARGUMENTS Phase 8); `--rebuild` composes what the PERSON asked for.
    One command with two halves is fewer things to explain; two commands let a
    person recompose their features without re-probing threads for ten seconds.
14. **Which bit is `arguments.access`?** Rule 5 says it becomes bit 0, because it
    is already built, already lasting, and already the valve for the last-known
    store — and two mechanisms for one job is what R6 of SATELLITE_ARGUMENTS
    refused. But it ships TODAY as a `flag_setting` library, so folding it in is
    a change to something that works, and the author should say whether the
    setting stays the spelling with the register behind it, or is replaced.
