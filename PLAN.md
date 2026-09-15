# satellite-004 — PLAN.md

Started 2026-09-14; reordered the same night around the author's three files:
`.satc`, `.satb` and `.sati`. DESIGN.md holds the standards; this file is the
order of work. One milestone at a time. **For each milestone the author writes
the pseudocode, Claude writes the C++**, and the milestone lands when its
entries are cleared.

## M0.5 — port the build and install support *(the very next milestone)*

(author, 2026-09-15) On 2026-09-15 satellite 004 moved to the top of the
repository, and satellite 003 revision 07 went, unchanged, into
`old_versions/second_satellite/`. 003's build and install machinery stays there,
and 004 needs its own:

- **`make_support/`** — 003's Makefile is an index over numbered fragments (010
  the tree and compiler, 020 the version, 040 sources, 048 static, 065 tests,
  and so on). Port the ones 004 needs, one by one, so 004's `Makefile` becomes the
  same kind of index. **`satellite_debian` uses make_support too** (its own
  `make_support/010-tree.mk` and `020-inherited.mk` inherit from it), so the port
  has to keep what satellite_debian relies on working.
- **`satellite_enterprise/`** — copied to the top on 2026-09-15, icons and
  artwork included, byte for byte; its installer (`install.sh` and
  `install_support/`) still expects 003's layout and binaries, and has to learn
  004's: `build/satellite-004` and the `build/satellite-numbers/` folder of
  libraries, which must be installed together (DESIGN §3.4).
- **Version:** 020-version.mk's `SATELLITE_VERSION` / `SATELLITE_REVISION`
  become 004 / 02, read from `version.hpp`'s one place.
- **Not** satellite_debian or distribute yet — they stay in second_satellite until
  004 has something to package.

**Done when** `make` in the top folder builds 004 through the ported fragments;
`make test` runs 004's checks (check.sh, the string checks); the installer puts
`satellite-004` and its libraries into a prefix and the installed copy passes
check.sh; and satellite_debian's build still finds what it inherits.

---

## How a milestone is done

(author) "Instead of testing our programs, we will define entries with each of
the milestones." Each milestone lists its **entries**: every input that could
break it, including DESIGN §9's hostile names, bytes and machine code. A run of
the list writes an `[entry]` to `satellite.log` for anything handled wrongly.
**A milestone is done when that run writes no entries**, the build has no
warnings, and its speed race (where it has one) is within **×1.05 of compiled
C++**.

---

## The shape of a run (author, 2026-09-14)

```
source .satl ──24 threads──▶ .satc ──combine──▶ .satb ──▶ .sati
   numbers for every word    batch marks: wait here,   strings as bits,
                             run these in parallel     32 bits per character
```

- **If there is no `.satc`, make it. If there is no `.satb`, make it.** The
  main thread runs "as fast as it can" meanwhile.
- **Once a `.satb` exists, the 256 start-up threads stay dormant** until the
  program reaches a batch the `.satb` marks.
- **`arguments.satc` and `arguments.satb` choose when the files are built**
  (M0).

---

## What exists now

The prototype (2026-09-14):
- `structured-library.cpp`, `arguments`, `machine_state`, `satl_file`
- the number index in `satellite-numbers/call_number.satellite.cpp`
- one library, `satellite.console.display`
- `check.sh` (16 checks) and `race.sh`

It runs `satellite.console.display` with a string, a whole number or a bool.
The review proved 17 defects (DESIGN §11). Its speed is in DESIGN §12:
`display` about **119 times faster than 003 06's**, and ×1.002 of C++ with
`write`/`put`.

**003 already writes `.satc` files and never reads them while running.**
`satl --satc` writes `~/.satl/cache/<name>.<hash>.satc`, but a traced ordinary
`satl` run touched a `.satc` or the cache folder 0 times (measured). 004 puts
the file into the run.

---

## M0 — `satellite_config.hpp` and the `arguments` values

The file is in the satellite-004 folder, written by the author. **Every value is
quoted text, turned into a `satellite_number` at start-up, and used as a
ceiling** (DESIGN §1). The author's values:

| name | value | meaning |
|---|---|---|
| `object_bytes_max` | `"34359738368"` | 32 GB |
| `threads_max` | `"1000000"` | the target: one million threads running |
| `threads_startup` | `"256"` | the pool started first, then recalled |
| `file_size_max_bytes` | the same as `object_bytes_max` | |
| `max_memory_bytes` | `"61847529062"` | 64 GB less 10% |
| `satc` | `1` | build the `.satc` before running |
| `satb` | `1` | build the `.satb` before running |

They are also readable from a program through the special `arguments`
variable. (author) `arguments.satc` and `arguments.satb` choose when each file
is built:
- **`1`:** build it before anything runs (the default).
- **`0` (false):** build it while the program runs.
- **A special value:** never build it, so satellite-004 runs as a plain
  interpreter.

**Decision D0.1:** which value means "never". The author first said `3`, then
"we need a special value for this."

**Done when** the Makefile and the interpreter both read the file; each value
shows under `--debug`; and a value of a million nines is accepted and capped by
what the machine really allows.

**Entries:**
- a value that is not a number, a negative value, an empty value
- a million nines
- `satc` set to `0`, `1` and the "never" value

## M1 — the `.satc`, built by 24 threads *(the first thing to build)*

- **The word table: BUILT 2026-09-14** (`words/make_words.py`,
  `words/satellite_words.hpp`). It holds 364 words numbered first-available
  (DESIGN §3.2), generated from 003's registry so no number can drift. Six
  numbers are held inline, and deeper commands go in `longer`.
- **Strings as `char32_t`: BUILT 2026-09-14** (`strings/satellite_string.*`),
  ahead of M3. 30,055 cases agree with Python's strict UTF-8 decoder.
- **24 threads convert the program.** One job per file (the main `.satl` and
  every spaceship), each file cut into large pieces so every recall carries
  real work. A recall costs about 12.5 µs, so one thread per line would be
  about 400 times slower (DESIGN §6).
- **The format is 003's SATC.md, ported:** `#1.5.1("Hello, World!")`, with a
  header recording the word-list version and the source file's size and time,
  so a stale `.satc` is rebuilt and never believed.
- **Full `satellite.` paths are numbered from the text alone.** A method
  written on a variable (`my_list.append`) needs that variable's declared type
  first, which is why 003's `.satc` leaves those words un-numbered until the
  names are resolved. 004 does the same.
- **The prototype's defects** in the code that carries over — the loader, exit
  statuses, SIGPIPE, the library path — are fixed here (DESIGN §11).

**Race:** time to a finished `.satc` for a 10-line file, a 100,000-line file
and 100 spaceships, against converting on one thread and against 003's
`satl --satc`.

**Decision D1.1:** with `satc = 0`, the program starts before the `.satc` is
finished. A mistake on line 50 is then found after lines 1–49 have run. 003
checks everything first.

**Entries:**
- a stale `.satc` (the source changed)
- a `.satc` from another word-list version
- a truncated or hand-edited `.satc`
- a `.satc` that is a directory, or unwritable
- 100 spaceships
- DESIGN §9's list

## M2 — the `.satb`: satellite combine

(author) "It's just a .satc file with marks." Combine studies the numbered
program and marks:
- **where the main thread must wait:** input, random, time, file writes, and
  anything depending on the line before;
- **where commands can run in parallel,** such as independent loop iterations
  split into batches; each batch runs its calls in order and writes its output
  into its own buffer, and the buffers are printed in order (DESIGN §6);
- **how big a batch must be to pay for a recall,** at least several hundred
  commands (measured break-even about 450).

Combine is conservative: what it cannot prove independent stays in order.
- **Spacesuits are references:** two names can be one object.
- **Capsules called inside a loop** need a summary of what they read and write.
- **`polymorph` voids those summaries** for the object it changes.
- **An error inside a batch** is reported for the earliest failing iteration,
  exactly as one thread would report it.

**Done when** every program in the entries gives byte-identical output with
`satb = 1` and with the "never" value.

**Entries:**
- a loop writing a shared total
- a loop through two names for one spacesuit
- a loop calling `console.input`, and one calling `random`
- an error in iteration 700,001 of 1,000,000
- a `polymorph` inside a loop

## M3 — the `.sati`: strings as bits

(author) Every string in the program is turned into its bits, **32 bits per
character**, so the numbered file carries no text at all. One program,
`bits_to_cxx_str.cpp`, turns a bit sequence back into the C++ string a library
takes. This is the step before satellite someday compiles to bytecode in one
pass.
- **32 bits a character is UTF-32,** which C++ already has as `char32_t` and
  `std::u32string`. Every character has the same width, but plain English text
  becomes 4 times larger.
- **To stay within ×1.05,** each string is turned back once, when the `.sati`
  is loaded, never on every library call.

**Race:** 10,000,000 `display` calls of a string that came from a `.sati`,
against `std::cout`.

**Decision D3.1:** does `satellite.variable.string` itself become 32 bits a
character everywhere (the author: "we are going to have to rebuild
satellite_strings into sequences of bits"), or only inside the file?

**Entries:**
- an empty string
- a string with `"`, `\` and a newline
- every Unicode plane
- an invalid UTF-8 byte in the source
- a 100 MB string literal

## M4 — `satellite_number`

DESIGN §4: limbs laid side by side, the sign as a bool, and the digit count and
byte size each as a `satellite_number`. Small numbers stay inline; the largest
digit count comes from M0.

**Race:** a million `i = i + 1` against a C++ `long long`. 003 06 spends 283 ns
a line on this loop.

**Entries:**
- 0, −0, and the largest `unsigned long long int` + 1
- 27 nines, and the maximum number of digits
- hostile text as a number

## M5 — names and `satellite.log`

DESIGN §7:
- the name vectors, the `a–z A–Z 0–9 _` rule, and `write_entry`, with a mutex
  and its path from config;
- a clash in any vector is an entry.

**Entries:**
- `!@#$%^&*()` and `poly.#@($*&$&$` as names
- machine-code bytes as a name
- one name declared as both an object and a class

## M6 — the parser, ported from 003 06 with fast paths

003's parser and resolver come across one part at a time, rewritten around the
number table and likely scenarios: variables, expressions (DESIGN §5), `if` /
`for` / `while`, capsules and calls. Every walker keeps its own stack.

This is also where selectors written on a variable get their numbers in the
`.satc` (M1).

**Race:** a 1,000,000-iteration loop that displays, against the same loop in
C++.

## M7 — the runtime that runs a `.satb`

- **The main thread runs the numbered program at full speed.**
- **The 256 pool threads** stay dormant until a marked batch is reached.
- **A `.satb` still being built** (`satb = 0`) is used only for the parts it
  has finished; **the main thread never waits for combine.**
- **`satellite.thread.new` takes its thread from the pool.**

**Race:** a splittable 1,000,000-iteration loop on the pool against the same
loop in C++ on 24 threads. Also, start 1,000,000 threads doing math (the
author's target; 32.7 GB measured).

## M8 — user-defined classes (spacesuits)

As 003 06 has them: fields, capsules, `satellite.protected`, `satellite.public`
and `satellite.constructor`. Every library is reachable from their capsules,
and a spacesuit inside a spacesuit takes the slower path.

## M9 — polymorph

Both spellings, `object_name.polymorph(args)` and `satellite.polymorph(args)`.
The area comes first. A class declared inside is reached as
`satellite.library.poly.class_name`, `poly.class_name`, or plain `class_name`
when that name is free.

**Decisions:**
- **D9.1** What "re-included into the individual capsules" means.
- **D9.2** What `args` are passed to.
- **D9.3** A class declared twice.

## M10 — `satellite.cxx() { C++ }`

An ordinary C++ file inside the block, compiled into a numbered library. This
is also the test for commands deeper than six numbers. POLYMORPH/M6 holds the
open questions.

## M11 — `satellite.infinity`

`satellite.variable.infinity x = satellite.infinity.new()`, in
`satellite/infinity.cpp`.

**Decision D11.1:** the arithmetic and comparison rules, before any code.

## M12 — finding more batches while the program runs *(one of the last milestones)*

(author) "As the interpreter runs, it continues to look for spots where it can
unmark things, where it can turn code into batches … we monitor int.int.int that
is being run, what names of variables are being called."

- **The pool is split:** some threads look for new batches while the rest run
  them. (author) "64 of them to search … and 196 of them to run". **64 + 196
  is 260**, so a 256-thread pool splits **64 + 192**.
- **Watching is cheap or it fails the bar.** Names are recorded at loop
  boundaries or by sampling, never on every command.
- **Seeing is not proving.** 1,000 iterations that never touched a shared
  variable say nothing about iteration 1,001: a branch that has not run yet
  may. So a batch found while running is either proven by combine's reading of
  the code (M2), or guarded: the first shared write drops that loop back to
  running in order, with its output held until then (the guess-then-check
  approach).
- **A new batch plan starts at the next time a loop begins,** never partway
  through a batch.

**The parallel group in the numbered file.** The author sketched two forms:

```
#1.1.1(args).#1.1.1(args)          -- two commands in parallel, inline

parallel_start:                    -- a group that ends at the first empty line
#1.1.1(args)
#1.1.1(args)
#1.1.1(args)
```

**Decision D12.1, with three problems to weigh:**
- **An empty line cannot end the group in a `.sati`,** which is written "with
  the spacing removed".
- **The inline `.` already means "method on"** (`my_list.append`) and sits inside
  every number (`#1.5.1`), so `a(x).#1.6.1.1()` could be read two ways.
- **One cheap command per line costs about 12,500 ns** to hand to a thread
  against about 28 ns to run, so three such commands in parallel are about
  400× slower. Every line in a group must be a big unit: a range of loop
  iterations, a heavy capsule call, a file read.

Recommended: one form with a count, so no spacing, dot or end marker is
needed, and every line is a batch:

```
#parallel 3
#1.1.1(args)
#1.1.1(args)
#1.1.1(args)
```

**Done when** a program whose batches are only discoverable while running (a
loop behind a condition combine cannot resolve) speeds up after its first
entries; the guarded fallback keeps output byte-identical when iteration
1,000,001 suddenly writes a shared variable; and watching costs less than the
×1.05 bar on a program with no batches at all.

---

## Not in this plan yet

`satellite.access` (ACCESS_PLAN.md), the network (003's M27), the prompt and
satl-term, and the single compile to bytecode that the author expects to
replace `.satc`, `.satb` and `.sati` someday.
