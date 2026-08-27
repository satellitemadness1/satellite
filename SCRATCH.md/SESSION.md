# Session state — 2026-08-27

**This file is scratch and is meant to be deleted.** It exists so that a `/clear`
costs nothing. Everything in here is either (a) waiting to be moved into a
permanent document, or (b) a question only the user can answer. When both lists
are empty, delete this file.

Permanent documents, for reference: [DESIGN.md](../DESIGN.md) is what the language
is, [PLAN.md](../PLAN.md) is how it gets built, [LAYOUT.md](../LAYOUT.md) is every
file in the tree, [WORD_NUMBERS.md](../WORD_NUMBERS.md) is the numbering and is the
authority over every number in the language.

---

## 1. Where the work is

**M1 landed 2026-08-26.** **M2 is in progress** and has not committed any code.

M2 as PLAN §8 defines it is: `src/satellite_words/words.def`, the trie, the
spelling interner, `PathId`, a digest over `words.def`, `satl --words` as its
consumer, and a test proving paths walk to their numbers. **No C++ is written yet.**
The blocker was never code — it was that the numbering had to be settled first, and
settling it took the whole session and changed the plan in six places (§3).

**What IS done: the numbering itself.** WORD_NUMBERS.md §2.2 holds **215 numbered
entries** — 144 after the first pass, plus the 71 that settling QUAD.md §3 added
later the same day (§5.6). Nothing found by the sweep is unnumbered. It validates
the way M2's `static_assert`s will — checked mechanically on 2026-08-27, and again
after the 71 landed:

    duplicate numbers that are not declared aliases      0
    parents with holes in their child list               0
    paths whose parent number is absent                  0
    aliases pointing at a number that does not exist     0

So `words.def` is now a transcription job against a table that is known to be
internally consistent, rather than a design job.

**The build is clean.** `make` produces `satl`, `satl.haswell` and `satl-cpu-level`
and `satl --version` runs. Nothing in this session touched `src/`.

---

## 2. What the user decided this session

Recorded because these are decisions, not observations, and a decision that only
exists in a transcript did not happen.

**The seed is wide.** `words.def` gets the whole first-satellite word surface, not
just what M8–M10 needs. This closes the question PLAN §8 marks as *"Open, and it
must be settled before `words.def` is written."* PLAN §8 must be edited to say so.

**The numbering rule is ORDER OF FIRST APPEARANCE.** This corrected a wrong
assumption of mine that had reached the point of being asked about. The rule is not
"group the namespaces sensibly and number the groups" — it is: walk a real program
top to bottom and *"chang[e] the number only when you MUST."* Their example:

    satellite.include           1 1
    satellite.capsule           1 2
    satellite.main              1 3
    satellite.console.display   1 4 1
    satellite.container.list    1 5 1
    satellite.variable.string   1 6 1
    satellite.variable.number   1 6 2

**The user owns the numbering personally.** They wrote `WORD_NUMBERS.md` by hand
and asked me to stop while they did. My job is to transcribe it into `words.def`
and to hand them the list of what is still unnumbered — not to assign numbers.

**User-defined names get numbers too, allocated dynamically.** In their words:

> *all user defined capsules and all user defined spacesuits need to grab the next
> available number; the language needs to keep and always have ready the next
> available number*

This is the largest change to M2 and §3.1 below is about it.

---

## 3. What is not yet in a permanent document

Each of these is real, decided or measured, and currently lives nowhere but here.

### 3.1 The numbering is partly dynamic, and M2 did not know that

`WORD_NUMBERS.md` now carries:

    satellite.library.main   1 14 1
    satellite.library.x      1 14 ?   <- x is a user capsule name

with the user's note that this *"needs to be built-in to grab the next available
number, as x is just a capsule name."*

PLAN §8's M2 describes a **static** registry: a `.def` file, compile-time
`static_assert`s, frozen ids. That is still right for the language's own words. But
a user's capsules and spacesuits are numbered **at parse time**, as they are met,
under the node that owns them. So M2 needs a second thing beside the frozen table:

- a **live child counter per node**, so any node can be asked for its next free
  child number, and
- the rule that the language's own children are numbered first and frozen, and
  user children are allocated after them and last only as long as the program does.

Consequences that need thinking about before the code is written, none of them
settled:

- The frozen `static_assert`s cannot cover dynamically allocated children, so the
  header's guarantees apply to the language's words only. That needs saying out
  loud in the header, or it will read as a promise it does not keep.
- Two programs can give the same capsule name two different numbers. That is fine
  for dispatch but means **a PathId is not stable across runs** for user names, and
  anything that persists a PathId (Satellite Orbit, the wire format) must know it.
- DESIGN §4.3's *never renumber, never reuse, always append* was written about
  frozen language words. For user names it cannot hold across runs, so §4.3 needs a
  sentence separating the two cases.

### 3.2 The threading and memory requirement

The user's words:

> *this part of the language needs to be threaded -- converting all of the
> satellite.something.something into integers needs to start from the number in
> satellite_config.ini and you need to have a... THREAD_COUNT=24 CORE_COUNT=12
> MEMORY_MAX=65??? whatever is on this machine, and the language needs to not use
> more than the MEMORY_MAX, so you need to know how much memory is being used by
> the interpreter*

and, clarifying where the threading goes:

> *we don't have a satellite_config.ini that you can grab the thread_count from to
> create that many threads and then hand them each a line that begins with
> satelllite, or the fastest possible way to do it, whatever that may be*

So: **one source line per thread**, over the user's program, sized from a config
file that does not exist yet and that I am to create.

**This machine, verified 2026-08-27** — `lscpu` and `/proc/meminfo`:

| | |
|---|---|
| CPU | Intel Xeon E5-2670 v3 @ 2.30GHz, 1 socket |
| hardware threads | 24 |
| physical cores | 12 |
| MemTotal | 64946428 kB = 61.9 GiB = 64.9 GB |

The user's guesses of 24 and 12 were exactly right.

**The conflict, which they have been told about and have not yet ruled on.** PLAN
§4.3 measured startup at 1.75 ms total with satl's own share at **0.01 ms**.
Spawning 24 threads costs roughly 0.5–1.5 ms — 50 to 150 times satl's entire
current startup cost — and it would be paid by every program including one that
prints a line. Threading the *fixed 150-word table* at startup is a guaranteed
loss. Threading the *user's source lines* is a different question with a different
answer, because that work scales with the program and the table does not.

The number that decides it is the **crossover**: how many satellite-rooted source
lines a program needs before 24 threads beat 1, counting thread creation. That
number is not yet measured. A benchmark was written and then killed — it was
competing for the same 24 cores as the design workflow's own measurement agents,
which made both sets of numbers wrong. See §4.

### 3.3 The four segment-1 words that were missing, now fixed

DESIGN §6.1 holds an authoritative table of the eleven segment-1 words that get
their own parse rule. Four of them had no number when this session started;
the user has since numbered all four — `statement` 13, `library` 14, `return` 15,
with `returns` still absent. Worth keeping because it is how the gap was found:
**§6.1's table is the checklist**, and a word missing from the numbering but
present in that table is a word the parser cannot reach.

### 3.4 Three findings about the documents themselves

- **`satellite.bool` is a top-level namespace, separate from the type
  `satellite.variable.bool`.** DESIGN §6.1 cites `satellite.bool.true` as a module
  constant and v1 implemented both. Two different `bool`s under two different
  parents. Legal, and worth numbering deliberately rather than discovering.
- **`satellite.number.shift_left` (DESIGN §5.5) was a slip and is fixed.** It
  implied a top-level `number` namespace when every other number operation is
  under `satellite.variable.number`. Confirmed by the user 2026-08-27; §5.5 now
  reads `satellite.variable.number.shift_left(n)`.
- **`satellite.control.return` (DESIGN §6.1) is not a real path** and must never be
  numbered. It is a hypothetical, there to show a parse collision. Likewise
  `satellite.consle.display` in §4.6 is a deliberate misspelling.

### 3.5 DESIGN §4.1 is now stale

§4.1's worked examples say `satellite.console` = `1 1`, `satellite.variable` =
`1 2`, `satellite.random` = `1 5`. Under `WORD_NUMBERS.md` those are `1 5`, `1 6`
and `1 7`. **The examples must be rewritten from WORD_NUMBERS.md**, and §4 should
say in a line that WORD_NUMBERS.md is the authority and §4 only explains it — which
is the project's own rule that prose may explain a number but never be the only
place it lives.

`satellite.random.normal = 1 5 2` is repeated in README-adjacent prose and in my
own saved memory; both are corrected.

---

## 4. In flight right now

**Workflow `wf_c906ee56-f2d`** — designs `satellite_config.ini`, the thread pool and
the MEMORY_MAX ceiling. Four phases: measure on this machine, three independent
designs, an adversarial attack on each, then one synthesis. It was launched before
the user's *"one line per thread"* clarification, so **its framing of where the
threading goes is narrower than what was finally asked for** — read its measurements
as sound and its design conclusions as needing that correction applied.

    script  ~/.claude/projects/-home-madness-code-cxx-satellite/
            5f56c04b-1617-4890-9065-0e86bf62d942/workflows/scripts/
            satellite-threads-and-memory-wf_c906ee56-f2d.js
    result  .../tasks/w0m5712po.output

If that result is lost, the script is on disk and can be re-run.

**Not measured, still needed:** the crossover point in §3.2. The killed benchmark
source is at `line_intern.cpp` in this session's scratchpad; it works but must be
run when nothing else is using the cores, with fewer repetitions at the large
sizes.

---

## 5. Questions only the user can answer

1. ~~What does `(0)` mean?~~ **Settled 2026-08-27.** `0` is a real number meaning
   nothing in that position, and a number identifies a **call shape**, not a path.
   A language-owned argument extends the path with its own number
   (`include(satellite)` = `1 1 1`); user values cannot, so each distinct argument
   count takes its own slot. `?` was the other mark and means something else
   entirely — a number allocated when a name is first met (§3). WORD_NUMBERS.md
   §1.3 is the rule.

   **One inconsistency survives and is worth a human eye.** The author's own
   examples place call shapes at two different depths: `include()`/`include(satellite)`
   are children *of* `include` (`1 1 0`, `1 1 1`), while `input()`/`input(prompt)`
   are siblings *of* `input` under `console` (`1 5 2`, `1 5 3`). Both are written
   down and both were confirmed. The table follows each as given, so it is faithful
   but not uniform, and `words.def` will have to encode one shape or the other.

2. **`MEMORY_MAX` in what unit, and what is the default?** 61.9 GiB and 64.9 GB are
   the same memory. And should the shipped default be the whole machine, or a
   fraction of it, and should a machine with less than the file says win?

3. ~~Is `satellite.number.shift_left` a slip for
   `satellite.variable.number.shift_left`?~~ **Confirmed a slip.** DESIGN §5.5 is
   corrected, and `satellite.variable.number` now holds ten more operations at
   `1 6 4 2` through `1 6 4 11`, `shift_right` among them.

4. ~~`satellite.returns` still has no number.~~ **Numbered `1 21`.**

5. ~~In what order do the nine remaining top-level namespaces go?~~ **Done
   2026-08-27.** The author delegated the assignment; 16–24 went to `analyze`,
   `bool`, `directory`, `help`, `network`, `returns`, `system`, `thread`, `window`
   alphabetically, and everything below them followed by the next-lowest-free rule.
   That pass ended at 144 entries; **§2.2 now holds 215** (§5.6) and nothing in the
   sweep is unnumbered.

6. **The float, and it is the only one on the critical path.** §5.7 has it, along
   with the three that came out of the QUAD reading.

---

### 3.6 `.satc` — specified, not built

The author asked for a cached, human-readable form of a numbered program, written
on its own thread and checked for before the conversion runs. It is specified in
**[SATC.md](../SATC.md)** and scheduled as **PLAN M4.5**, after the parser exists
to produce something to serialise.

Two things it changed elsewhere, both now fixed rather than left contradictory:

- PLAN §2.3 listed `no serializable format` among what closure compilation
  deliberately lacks. Corrected: what is never serialised is the **closure tree**;
  `.satc` serialises the layer above it.
- PLAN §2.1 said "bytecode was ruled out by the brief" flat. Corrected to say what
  was ruled out is the *machinery* — the instruction stream, the decode loop, the
  compile step — while the part that earns its keep, a small integer per operation,
  is exactly what DESIGN §4 is.

**The constraint it puts on M2:** `words.def` needs a digest, so a `.satc` can name
the numbering it was written against. Added to M2's list in PLAN §8.

**The constraint it puts on the numbering:** user-owned names must be written into
a `.satc` as names, never as numbers, because their numbers are allocated per run
(WORD_NUMBERS.md §3). SATC.md §3.

## 5.5 THE NEXT THING THE AUTHOR WILL SAY

Stated explicitly at the end of the previous session, before the clear:

> *we need to be able to express quad_infinity*

**[QUAD.md](../QUAD.md) is that document.** It was rewritten on 2026-08-27 after
reading QUAD's source rather than inferring from its includes, and it now opens with
the rule the author set: **both sides bend, and QUAD bends more.** satellite is the
thing being built, so the burden of proof sits on satellite changing, and a feature
enters the language only when QUAD's *meaning* cannot survive without it.

**Four of the six holes closed.**

| was | now |
|---|---|
| sorting needs capsules as values | all seven comparators are one shape — one primitive. DESIGN §12's deferral **survives** and §2 stays shut |
| a set and a deque | membership tests and bounded ring buffers — map + `remove_first` + `truncate`. `1 4 5` left free |
| a console that can address a screen | `view.hpp` never moves a cursor. The gap was **non-blocking input**, answered by a reader thread (DESIGN §10.1) |
| map keys unchecked | every key is a number or a string. §6.5 costs QUAD nothing |

**`satellite.variable.float` is the one that grew teeth**, and it is now the whole
remaining gap. Three findings from the source, all in QUAD.md §3.1 and DESIGN §13:
`pow` at a fractional exponent has no exact decimal value at any length, so a
rounding rule is part of the type; `activation *= keep` every tick grows digits
without bound; and `.sky` already round-trips the whole mind through six significant
digits and works. DESIGN §8.1 now says its no-`double` argument does not reach the
float.

**One correction to QUAD.md §2 worth keeping:** QUAD spawns **zero threads**. The
only `<thread>` use in 3029 lines is `sleep_for`. `satellite.time.sleep` is `1 9 3`.

### 5.6 What landed in the documents on 2026-08-27

**71 numbers, taking WORD_NUMBERS.md §2.2 from 144 entries to 215.** Validated
mechanically the way M2's `static_assert`s will: 0 duplicates, 0 parents with holes,
0 paths whose parent is absent, 0 aliases pointing at nothing.

The map had **no** children before this and now has nine; the string had none and now
has sixteen. Also: 23 more on the list, 10 on the number, 5 on the file, 5 on the
console, `satellite.variable.capsule` `1 6 16`, `satellite.time.sleep` `1 9 3`,
`satellite.library.system.float_digits` `1 14 2 4`.

**Two new rules were written down**, and both came out of the author's questions
rather than from planning:

- **WORD_NUMBERS.md §1.5 — paths against selectors.** A path is rooted at `satellite`
  and becomes a number in a `.satc`; a selector is a bare word after a receiver and
  stays bare, because PLAN M4.5 writes the file from the parse tree and DESIGN §6.3
  keeps the parser resolution-free, so the selector's identity is *unknowable* at that
  point. Most of the 71 are selectors. They are numbered for dispatch only.
- **A literal option folds into the number.** `my_list.sort("down")` can intern to a
  different `PathId` than `sort("up")` — same readable surface per DESIGN §1.1, no
  runtime test. Resolve-time only; SATC.md §3 keeps the literal in the file.

**SATC.md gained four sections** — §3.1, §3.2, §5.1, §5.2 — covering the two
populations, why the pipeline forces the split rather than taste, the five-step
transformation order, and the one ordering the writer must preserve: **source
order**, because user names are numbered as they are first met and a cache hit has to
re-allocate them identically. That also answered §6's open question about whether the
number column is diffable.

### 5.7 Still open after this session

1. **The float — representation settled, rounding rule not.** The author decided
   2026-08-27 that a float is a **`satellite.variable.bool` and two
   `satellite_number`s** — a sign held once, then one number per side of the decimal
   point, both halves magnitudes that never carry a sign. Left exact and unbounded,
   right bounded, and the right half's length is the precision. No negative zero.
   **DESIGN §8.6** is the specification: the invariants, `normalize`, the four
   operations, modulus, power, and the three classes that say where rounding bites.
   The sign is an explicit `positive` bool defaulting to **true**, and DESIGN §8.1
   now gives `satellite.variable.number` the same one, so the two types cannot
   disagree about what negative means. Both are now milestones of their own —
   **M6.5** for the number (before M7, whose `Value` holds one, and it is where the
   shared sign is built) and **M9.5** for the float. It is **M9** and cannot land until the
   **rounding rule** — truncate, half-up, or half-even — is chosen, because
   `pow` at a fractional exponent is irrational and no pair of exact numbers
   represents it. Also still open: whether
   `satellite.library.system.division_digits` `1 14 2 1` is the same knob as
   `float_digits` `1 14 2 4` under a narrower name.
2. **Call shapes sit at two depths.** `include()` is a *child* of `include` at
   `1 1 0`; `input()` is a *sibling* of `display` at `1 5 2`. WORD_NUMBERS.md §4 now
   proposes a reading under which both are correct — language-owned arguments extend
   downward, user-owned ones take a sibling slot — but it needs confirming before
   `words.def` encodes one.
3. **`satellite.file` `1 8` and `satellite.variable.file` `1 6 2` both carry `new`.**
   The five new file methods went on the type node per DESIGN §6.4. Nothing says
   which a program should write.
4. **`satellite.thread.new` has one number for two shapes.** Argued in WORD_NUMBERS.md
   §4: its own arity is always 1. `1 23 2` is free if that is decided the other way.
5. **122 of the 218 numbered paths reach no milestone** — 56% — and
   [MILESTONE.md](MILESTONE.md) is the ledger. *(Corrected 2026-08-27: the figure was
   50, then 41, then 29, and all three were namespace sweeps. §5 of that file admitted
   the 165 paths under milestoned namespaces had never been checked one at a time.
   They have been now.)* Another 35 are covered only by a sentence about their parent,
   which is the half that hides.

   **`satellite.system` is 30 of the 122 and `satellite.random` is 16** — between them
   more than the whole of the old figure. Neither is in "Later"; `satellite.system`
   appears in PLAN.md on exactly one line, the audit paragraph saying it is
   unscheduled.

   **The pattern to keep watching for** is a milestone that owns work and does not say
   so. It has now been caught three times — M3/M4 and DESIGN §6.1's eleven words, then
   M10 and the twenty-nine container methods. Each time it looked like an unscheduled
   namespace and was not.

6. **The honest next step is unchanged** and is now named: write `Sky::decay` plus
   `Rack::draw` in satellite by hand against DESIGN.md. Between them they touch
   floats, the map, a weighted pick, and the one `pow` that has no exact answer.

## 6. Jobs the user has asked for that are not started

- **Convert `plans/madness/first_note.txt` into the permanent documents, then
  delete it.** Its content is mostly already in DESIGN and PLAN; what is *not*
  captured anywhere else is noted in [FIRST_NOTE.md](FIRST_NOTE.md).
- **Port `satellite_number` and `satellite_string` from the first satellite** into
  `src/satellite_number/` and `src/satellite_string/`, which exist and are empty.
  See [PORTING.md](PORTING.md).
- **Rewrite `WORD_NUMBERS.md` in prose** — the user asked that their own notes be
  replaced by my writing, with their numbers preserved exactly.
