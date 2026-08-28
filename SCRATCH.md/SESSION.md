# Session state — 2026-08-28

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

**M1 landed 2026-08-26. M11.A landed 2026-08-27** — out of order, because the
author asked for the window and it needed nothing that has not been built.
**M2 LANDED 2026-08-28** — see §5.17, which is the section to read first.
**M3, the lexer, is next.**

M2 as PLAN §8 defined it was: `src/satellite_words/words.def`, the trie, the
spelling interner, `PathId`, a digest over `words.def`, `satl --words` as its
consumer, and a test proving paths walk to their numbers. **All of it is built**,
plus the tree's first `tests/` directory and `make test`, which FORMAT/CXX.md §6
said did not exist. The blocker was never code — it was that the numbering had to
be settled first, and settling it took a whole session and changed the plan in six
places (§3).

**What IS done: the numbering itself.** WORD_NUMBERS.md §2.2 now holds **222 rows
carrying 219 distinct numbers** — 144 after the first pass, then the 71 that settling
QUAD.md §3 added the same day (§5.6), then four assigned 2026-08-28 (§2.4). Nothing
found by the sweep is unnumbered. **Re-verified mechanically on 2026-08-28**, and the
three-versus-219 gap is exactly the three declared `.range` aliases — `1 7 5`,
`1 7 8`, `1 7 11` — and nothing else, which is what makes M2's check writable:

    duplicate numbers that are not declared aliases      0
    parents with holes in their child list               0
    paths whose parent number is absent                  0
    aliases pointing at a number that does not exist     0

So `words.def` is now a transcription job against a table that is known to be
internally consistent, rather than a design job.

**The build is clean.** `make` produces all four binaries, `make test` passes, and
`satl --words` prints the whole numbering. `src/` and `tests/` are **25 C++ files,
3,148 lines**, plus `words.def` at 542. The largest is
`tests/words_test/authority.cpp` at 261; nothing is over 300. `words_walk.hpp`
reached **305** before being split by subject into it and `words_spellings.hpp`,
and that 305 is what prompted the author to soften PLAN §3 to **"try to build for
300 lines"** on 2026-08-28 — mid-split, with *"we can modularize later."* **M2 is the first session
since M1 to write interpreter code.**

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
prints a line. Threading the *fixed table* at startup is a guaranteed loss — and M2 removed
even that possibility, since the table is now `constexpr` and nothing runs
before `main()`. *(It is 254 nodes, not the ~150 this paragraph guessed.)* Threading the *user's source lines* is a different question with a different
answer, because that work scales with the program and the table does not.

The number that decides it is the **crossover**: how many satellite-rooted source
lines a program needs before 24 threads beat 1, counting thread creation.

**MEASURED 2026-08-28 and the answer is in PLAN §4.5.1.** ~2,650 lines against 24
freshly created threads; **~170** against 24 from a pool that is already warm. It
could not have been taken before M2 — the thing it measures is `words::walk()` over
a user's source, and that did not exist. *(An earlier benchmark was written and
killed: it was competing for the same 24 cores as a workflow's own measurement
agents, which made both sets of numbers wrong. This one ran at load 0.55.)*

**AND THE 170 IS A SECOND-BATCH NUMBER**, which the first write-up of this did not
say and which is the whole point. Creating the 24 parked threads costs **~590 µs**,
so a **cold** pool crosses at ~2,650 too — a cold pool *is* 24 fresh threads plus a
cheap wake. **A program that threads exactly once gets nothing from a pool**, and
`satl program.satl` is exactly that program.

**So this section's requirement is half right, and the fix is not "use a pool".**
One line per thread pays; creating the threads to do it does not; and the pool earns
its keep by being **shared across a run** — PLAN §4.5.1's "three tenants amortise a
cost none of them could justify alone" — rather than by making one parse cheaper.
A one-shot parse should stay single-threaded. See §4.

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

### 3.5 ~~DESIGN §4.1 is now stale~~ — FIXED, verified 2026-08-28

**§4.1 now reads `1 5`, `1 6`, `1 7` and matches WORD_NUMBERS.md**, and §4's opening
carries the authority line this entry asked for. Kept as a record of what was wrong.
*(A different §4.1 defect was found on 2026-08-28 and is in §5.14: its "walk the
program" order says `console` before `variable`, and a literal walk of §3 gives the
reverse. The numbers are frozen and right; the sentence is the imprecise part, and
§4.1 now says so.)* The original entry:

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

**Nothing is running as of 2026-08-28.** No workflow, no background task, no
uncommitted build. The tree has **uncommitted document changes in nine files** —
see §5.14 — and `git status` is the list.

The entry below is from the 2026-08-27 session and is kept only for its script path.

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

**MEASURED 2026-08-28 — §3.2's crossover is answered and is in PLAN §4.5.1.**
It could not have been taken earlier: the thing it measures is `words::walk()`
over a user's source, and that did not exist until M2. Run against the real code
path with the machine quiet, best of 15: **24 fresh threads break even at ~2,650
satellite-rooted lines** (creating them costs ~690 µs flat), **24 from a warm pool
break even at ~170** (waking them costs ~47 µs). Below ~170 the walk must stay
single-threaded — at 80 lines the pooled arm is 0.48×, i.e. twice as slow as just
doing the work.

**So the author's instruction splits in half.** *"Hand them each a line that
begins with satellite"* is right; *"create that many threads"* to do it is a loss
for anything under ~2,650 lines. The pool PLAN §4.5.1 already proposed is what
makes the request pay, and it is now a requirement rather than a nicety.

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
2. **~~Call shapes sit at two depths.~~ — RESOLVED 2026-08-28 by M2, and it needed
   no new decision.** WORD_NUMBERS §1.3's definition of `(0)`, written the same day,
   had already settled it: `include()` `1 1 0` is include's **bare call shape**, the
   same marker `satellite.container` `1 4 (0)` carries, and not a child at an odd
   depth. So the rule is one rule seen twice — **a word reached bare holds its shapes
   as children; a word only ever called does not exist apart from them, so its shapes
   are siblings.** `input` has no number anywhere in §2.2, which is why its three
   shapes sit beside `display`. `words.def`'s header carries the argument and
   `match_shape()` is the rule in ten lines. **No number moved.**
3. **`satellite.file` `1 8` and `satellite.variable.file` `1 6 2` both carry `new`.**
   The five new file methods went on the type node per DESIGN §6.4. Nothing says
   which a program should write.
4. **~~`satellite.window.console` has no number~~ — ANSWERED 2026-08-28.** It is
   `1 24 2` and its `new(title, width, height)` is `1 24 2 1`, a child of
   `satellite.window` and **not** the same thing as `satellite.window.new` `1 24 1`.
   The author delegated this one assignment and the two thread selectors below;
   WORD_NUMBERS.md §2.2 carries them and records who assigned them and why.
   **`(0)` was defined 2026-08-28** and is no longer open: §1.3 now says it is the
   same `0` it already defined, so `satellite.container` `1 4 (0)` means the bare
   call shape is `1 4 0`. Not a third concept — a reading aid on a node reached both
   bare and as a parent.
5. **`satellite.variable.thread.start()` `1 6 13 1` and `.join()` `1 6 13 2` now
   exist** *(assigned 2026-08-28)*, so `example/thread_test.satl` names only real
   paths. **M12 still owns neither**, and the deferred call `satellite.variable.capsule`
   `1 6 16` that `satellite.thread.new(f(x))` needs is owned by nothing at all.
   [THREADS.md](THREADS.md) is the brief.
6. **`satellite.thread.new` has one number for two shapes.** Argued in WORD_NUMBERS.md
   §4: its own arity is always 1. `1 23 2` is free if that is decided the other way.
7. **~~M2's `static_assert` cannot be written as PLAN specifies it.~~ — RESOLVED
   2026-08-28.** "No duplicates" was false of §2.2 by design — the three `.range`
   aliases. **PLAN M2 now carries the four properties in full** (no holes; no
   duplicates *among non-aliases*; no orphans; no alias pointing at a number that
   does not exist), which is QUAD.md §4's form. The part that was not obvious and is
   now written down: **property 2 is a `words.def` requirement before it is an assert
   requirement** — an alias must be *declarable*, or the check cannot tell a
   deliberate duplicate from a typo. **This no longer blocks M2.**
8. **121 of the 222 numbered paths reach no milestone** — 54% — and
   [MILESTONE.md](MILESTONE.md) is the ledger. *(Was 124; `satellite.help`'s three
   moved out to **PLAN M8.5** on 2026-08-28, the first row ever moved. §5.14.)* *(Corrected 2026-08-27: the figure was
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

9. **The honest next step is unchanged** and is now named: write `Sky::decay` plus
   `Rack::draw` in satellite by hand against DESIGN.md. Between them they touch
   floats, the map, a weighted pick, and the one `pow` that has no exact answer.
   **Still not done, and it is the largest thing left that needs no decision from
   the author** — it is writing, not choosing. QUAD.md §5 names it as the milestone
   PLAN §8 does not have.

10. **The threading question is CLOSED as of 2026-08-28** — see §4. The crossover is
    measured and PLAN §4.5.1 carries it. What remains is not a measurement but a
    build: the lazy shared pool, which now has a number justifying it and still has
    no milestone (`MILESTONE.md` §3).

11. **`satellite_config.ini` still does not exist**, and §5 question 2 —
    `MEMORY_MAX`'s unit and default — is the author's and is unanswered. PLAN §4.5.4
    holds it.

### 5.8 What landed on 2026-08-27, second half

**`satl-term` is built and runs** — `src/programs/window.cpp` (209),
`terminal.cpp` (232), `terminal.hpp` (35), and `make_support/047-window.mk`. One
file reached 305 lines, so it is split by subject; PLAN §3's ceiling applies from
the first commit. `satl` resolves **6** shared objects and `satl-term` **79**,
measured here.

**M11 became M11.A and M11.B**, and that was the author's call: M11.A is the
window, M11.B is the prompt, and **M11.B's rule is that the window does not
close.** Until there is a prompt the child lives milliseconds, so M11.A closes on
a clean exit and holds on failure. `on_child_exited` marks its clean-exit arm as
M11.A's so M11.B deletes it deliberately.

**The window is the same one the language hands out.** The author's words:

> *satl-term is the `satellite.variable.window my_console =
> satellite.window.console.new("window_title", 800x600)` that takes 2 numbers as
> arguments, and a satellite string as an argument*

So the title and size are arguments, reached as `--title` and `--size 800x600`.

**`satellite.window.console` and `satellite.window.console.new` ARE NOT
NUMBERED.** §2.2 has `satellite.window` `1 24` and `satellite.window.new`
`1 24 1` and nothing else under that node. **This is a question only the author
can answer** and it belongs in §5.7's list: is the console window a *child* of
`1 24`, or is `1 24 1` already what it describes? Nothing was assigned — the
author owns the numbering.

**The binaries carry a desktop icon as `gio` metadata**, not as anything in this
tree: `metadata::custom-icon` points at
`~/.local/share/icons/hicolor/256x256/apps/org.satellite.terminal.png`. It was on
`satl` and `satl-term`; `satl.haswell` was given it on 2026-08-27 at the author's
request. **`satl-cpu-level` still has none.** The metadata is keyed by path and
**survives `make clean` and a rebuild** — checked, not assumed — and it is
outside git, so it is not restored by a clone.

**The `.desktop` entry is now installable and is not installed.** It was held back
for want of a binary and that reason is gone; adding it to
`install_support/060-install-tree.sh` is one line and was deliberately not taken,
because it changes what an install puts on someone's system.

### 5.9 Four milestone drafts exist and none is in PLAN §8

[MILESTONE_DRAFTS.md](MILESTONE_DRAFTS.md) holds them, with the adversarial
findings under each. **They are not in §8 because every lens found real errors**
and none is fixed. The one that shows the shape of the rest: a draft glossed
`1 22 4 6`–`1 22 4 12` as "`.free()` `.total()` `.used()` with their `(unit)`
forms" — six things across a seven-wide range, missing that `1 22 4 9` is
`satellite.system.memory.main(unit)`.

Two findings from that pass are corrections to §5.7 itself and matter more than
the drafts:

- **`satellite.system.threshold()` `1 22 5` and `(n)` `1 22 6` are M10's, not a
  machine milestone's.** They set how loose a search may be, and the search power
  cannot ship without its dial. So `satellite.system` is **28** unscheduled paths
  and not 30.
- **Three of the four `satellite.library.system` dials are already owned and no
  milestone says so** — `max_depth` `1 14 2 2` is M7's, `division_digits`
  `1 14 2 1` is M6.5's, `float_digits` `1 14 2 4` is M9.5's. Only `min_free_mb`
  `1 14 2 3` was unowned. **That is the M3/M4 pattern for the fourth time.**

### 5.10 What the fourth milestone's pass found, which outranks the draft

The `satellite.random` + `satellite.time` pass returned last and found more about the
**documents** than about the milestone. In rough order of how much they cost:

- **M2's `static_assert`, as PLAN specifies it, fails on the numbering it checks.**
  §2.2 holds three duplicate numbers and they are deliberate — the `.range` aliases.
  Now recorded in PLAN M2 itself, and it **blocks M2**, which is the next milestone.
- **Every 218-vs-215 disagreement in every document is those same three rows.** All
  218 number cells were extracted and sorted; the duplicates are exactly `1 7 5`,
  `1 7 8`, `1 7 11` and nothing else. That question is now closed.
- **M6.5 already owns the uniform draw and does not say so** —
  `satellite_number/random.cpp` is one of the ten files in its port. **Fourth
  occurrence of the M3/M4 pattern**, found the same way: by reading the inventory a
  milestone claims rather than its sentence.
- **WORD_NUMBERS.md drifted from the author's own note.** `satellite.random.fast`
  `1 7 1`, `.normal` `1 7 2` and `.ultra` `1 7 3` are written **bare** in
  WORD_NUMBERS_ORIGINAL.md, in DESIGN §11 and in PLAN M2's test, and **with
  parentheses** in §2.2. The rewrite promised to preserve every *number*; the parens
  are a change of meaning it did not promise, and v1 tests `satellite.random.fast()`
  as an *error*. §1.4 also still calls `fast.range` "four numbers" while §2.2 and
  §2.3 make it an alias at `1 7 5` — both shipped in the same commit.
- **`satellite.variable.duration` `1 6 8` is a number minted from a refusal.** The
  sweep sourced the word from v1's documents; the v1 source that uses it says *"there
  is no satellite.variable.duration"* and gives four reasons, and DESIGN §12 still
  defers durations today.
- **`satellite.variable.date` `1 6 7` has a number and nothing else** — no row in
  DESIGN §8's types table, no representation, no constructor, no methods, no v1 code.
- **`satellite.time.new` `1 9 2` has never existed, and DESIGN §13 cites it as
  established precedent** when settling `satellite.thread.new`. Its only description
  anywhere is a parenthesis in a deleted scratch file.
- **The two time methods v1 ships are unnumbered** — `.minus(t)` and
  `.nanoseconds()`. `satellite.variable.time` has zero children while its three
  hand-written siblings have 16, 7 and 14. **Not invented here; WORD_NUMBERS has to.**
- **MILESTONE.md §0.1's column summed to 121 against its own headline of 122.** The
  headline was right; the row was understated. Corrected in that file.

### 5.10.5 What was settled about the numbering itself on 2026-08-28

**`(0)` is defined**, as §1.3's `0`. Forty-odd rows used it and nothing said what it
meant. A trailing `0` is written only where a program can write the bare form, which
is why `satellite.variable.binary` `1 6 5` has none.

**§1.4's illustration was false and is corrected.** It cited
`satellite.random.fast.range` as *"four numbers and nothing else"*; §2.3 makes it an
**alias at `1 7 5`**, three numbers. Both shipped in the same commit. The true
example makes the point better: the deepest path is six segments —
`satellite.library.main.arguments.machine.cores` `1 14 1 1 1 1` — and it interns to
the same four bytes as `satellite.main` `1 3`.

**Fixed-length paths were considered and DECLINED**, and the reasons are in §1.4 so
nobody re-opens it without them. It buys the runtime nothing, because the `uint32_t`
is an interned id and not a packed path; **§3 makes fixed-width packing impossible
anyway**, since user capsules take unbounded numbers at parse time while the widest
language child list is 25; and six is today's maximum rather than a bound.

**Four numbers were assigned by delegation** — `1 6 13 1`, `1 6 13 2`, `1 24 2`,
`1 24 2 1` — and WORD_NUMBERS §2.4 records that they were, by whom, and what rule
decided each. **That delegation was for those four only.**

### 5.11 The second half of 2026-08-27 — what was built, not just decided

**`satl-term` (M11.A) and `satellite.random` both landed ahead of their milestones.**
Five commits, all on `main`, all with the tree clean afterwards.

**M11 became M11.A and M11.B**, the author's split. M11.B's rule is that **the window
does not close**; M11.A closes on a clean exit and holds on failure, and
`on_child_exited` names its clean-exit arm as M11.A's so M11.B deletes it on purpose.

**DESIGN §3's hello world lost its parameter, and that was a bug fix.** Declaring
`satellite.container.list<satellite.variable.string> arguments` made M8 depend on M10
for the container and M9 for the string — a milestone needing two that come after it.
`satellite.main` is `1 3 (0)` and always was. **WORD_NUMBERS.md §1.1 must keep the old
program**: the parameter is what fixes `container` `1 4` and `variable` `1 6`, and
walking the new one puts `console` on `1 4`. Nothing renumbers, but the example there
is now the historical walk. **The sentence saying so inside WORD_NUMBERS is unwritten
and is the author's.**

**DESIGN §11's windows changed** to fast 50–300 ms, normal 500–600 ms, ultra
2000–3000 ms, all random within range, and **the throwaway is whole numbers of the
size being asked for** with **at least one always** — a `do/while`, so a draw that
outlasts its window still costs exactly one. Built in `src/satellite_random/`,
measured on this machine, every tier inside its window.

**A 512-bit PCG was asked for and refused with a reason.** `uint_x4` hard-codes the
word width at `pcg_uint128.hpp:531, 534, 543, 558`, so composing it to 64-bit words
compiles and multiplies **wrong**. `pcg/README.md` has the full cost of doing it
properly and the argument that width belongs to the sampler, not the generator.

**The tree is now MIT and Apache-2.0 and says so.** pcg-cpp 0.98 is Apache-2.0 only.
`nm` says no built binary contains any of it, and LICENSE states that with its own
expiry condition written in.

### 5.12 Two things left dangling on purpose

1. ~~**`example/` is untracked and two permanent documents cite it.**~~ **RESOLVED —
   commit `6209c83`, "example/ is four acceptance programs, and PLAN already cited
   them."** The four programs are in the repository and `git clean -fd` no longer
   removes M8's acceptance program. The original entry, for the record: PLAN M8 says
   *"`example/hello_world.satl` is the done-when"* and DESIGN §3 calls it *"this
   file's copy of it"*. **A fresh clone gets a milestone whose acceptance program is
   not in the repository.** The four programs are the author's, one of them
   (`super_advanced.satl`) still being written, so nothing was committed. Either they
   go in or those two citations have to soften — and the first is much better,
   because an acceptance program that lives outside the repo is not an acceptance
   program.
2. **`SCRATCH.md/THREADS.md`** holds the grounded thread surface. `start` and `join`
   are two unnumbered paths on a type with zero children, `.detach()` is argued as
   **refused rather than deferred**, and the one genuine race is a started thread that
   `main` never joins. Nothing there is numbered and nothing may be.

### 5.13 2026-08-28 — hello world's parameter came back, and §5.11 above is now history

**The author reversed the 2026-08-27 removal.** `satellite.main` declares
`satellite.container.list<satellite.variable.string> arguments` again, in
`example/hello_world.satl`, DESIGN §3 and README. **§5.11's paragraph above records
the removal as "a bug fix" and it should be read as a record of what was believed on
2026-08-27, not as current.**

**The removal misread the authority.** It rested on WORD_NUMBERS §2.2 writing
`satellite.main` as `1 3 (0)` with `(0)` read as *zero arguments, nothing there*.
§5.10.5 above records what `(0)` was actually defined to mean the next day — **a
node reached both bare and as a parent** — which is a fact about reaching
`satellite.main`, not about declaring it. The two entries were written a day apart
and the second unmakes the first's argument. THREADS.md finding 205 had already
caught DESIGN defining `(0)` twice and differently; this is what that cost.

**Three further facts the removal did not weigh.** Hello world was the only one of
the four acceptance programs without the parameter — `advanced`, `thread_test` and
`super_advanced` all declare it. SATC.md §1.1's specimen listing already compiles
the parameterised signature as `1.2 1.3(1.4.2<1.6.1> arguments)` and never stopped.
And WORD_NUMBERS §1.1 is the current program again rather than "the historical
walk", so **the sentence §5.11 reserved for the author — why §1.1's hello world
differs from DESIGN §3's — is moot: they no longer differ.**

**What it cost, and the author settled it the same day.** Hello world never reads
`arguments`, so no `string` is ever constructed and M9 is not a dependency. What was
left is **one empty `satellite.container.list` bound to the slot**, and **the author
ruled that list M10's and moved M8 after M10** — an empty list is still a list, and
the milestone that constructs one has built the type. See §5.15.

**`//` comments are specified as of today, in DESIGN §5.6.** Every program in
`example/` used them and no section defined them — LAYOUT.md had been carrying the
gap as a note on the hello world row. §5.6 now says `//` runs to end of line, is
discarded in the lexer, and that there is no block comment.

**One thing found and not touched: WORD_NUMBERS §1.1's prose does not match its own
program.** It says the walk goes "`main`, then `container`, then `console`, then
`variable`", but `satellite.container.list<satellite.variable.string>` meets
`variable` before the next line meets `console`. A literal walk gives `variable` 5
and `console` 6; the table gives `console` 5 and `variable` 6. **The numbers are
frozen and correct — it is the sentence describing them that is wrong** — and this
mattered less while §1.1 was annotated as historical. It is load-bearing again now.
**WORD_NUMBERS.md is the author's and was not edited.**

### 5.14 2026-08-28, second half — M2 unblocked, and the first ledger row moved

**START HERE AFTER A CLEAR.** Nothing is running. Nine files carry uncommitted
document changes and no code changed. `git status` is the list; **nothing has been
committed this session**, and the author has not been asked for one.

**M2 is unblocked and is the next action.** §5.7 item 7 was the blocker on the
milestone in progress and PLAN §8's M2 now carries the four-property `static_assert`
— no holes; no duplicates **among non-aliases**; no orphans; no alias pointing at a
number that does not exist. The thing worth not re-deriving: **property 2 is a
`words.def` requirement before it is an assert requirement.** An alias has to be
declarable in the file, or the check cannot tell a deliberate duplicate from a typo,
and the three real aliases would then train whoever hits it to loosen the assert.
WORD_NUMBERS §2.3's model is the one the syntax must express — **one node with a
second spelling, not a second node** — and it carries §7.7's six spellings of
`arguments` too.

**Verified mechanically on 2026-08-28, so M2 can be written against it:**
WORD_NUMBERS §2.2 is **222 rows, 219 distinct numbers, and exactly three duplicates
— `1 7 5`, `1 7 8`, `1 7 11`, all three the declared `.range` aliases.** PLAN M2's
claim that they are the only duplicates in the language is true as of today.

**`satellite.help` is PLAN M8.5, and it is the first row ever moved out of the
ledger.** Three paths — `1 19`, `1 19 0`, `1 19 1` — written into §8 rather than
into [MILESTONE_DRAFTS.md](MILESTONE_DRAFTS.md), because MILESTONE.md's opening rule
is that a row which gets a milestone is *moved* into §8, and the four drafts that
stopped short of §8 are why the rule is worth obeying. Running figure: **121 of 222.**

**The milestone found a real defect in DESIGN §4.6, now corrected.** §4.6 said *"help
cannot drift from what exists — the trie **is** what exists."* **The trie is what is
*numbered*, not what is *built*.** After M2 it holds every path in `words.def`, so a
walk of it would advertise the 121 unscheduled paths to a user as working — worse
than v1, and DESIGN §1.1's *never behind their back* broken outright. The fix needs
no new machinery: **print a node when `handlers[path_id]` is non-null**, so the table
that decides whether a call runs decides whether help mentions it. Two consequences
worth keeping: v1's `help_for()` switch on `value.index()` is **deleted rather than
extended** — a value's type is a node and its methods are its children — which closes
the handoff M10.5's draft leaves open; and `satellite.help` becomes a live version of
MILESTONE.md, which is the condition under which *"delete this file when every row
has a milestone"* can be checked rather than believed.

**Grounded against v1's source, not memory.**
`old_versions/first_satellite/src/evaluator/help.cpp` is 221 lines and
`help_topics.cpp` another 315, all string literals, and it had already drifted:
`help_for_module()` answers for six modules and returns empty for every other name,
while its own comment says *"an empty answer means the name is not a module."*
`satellite.analyze` is a module, advertised in the same file's overview, and help
denies it exists.

**Two stale documents corrected.** PLAN §1 said *"five C++ files… the largest C++
file is 137 lines"*, true of M1 alone; `src/` is **ten files, 1,153 lines**, largest
`terminal.cpp` at 236. And `example/` is **tracked** as of commit `6209c83` — both
SESSION §5.12 and THREADS' operational note said otherwise.

**One new finding, and the author asked for it to be fixed rather than flagged.**
WORD_NUMBERS §1.1's prose said the walk goes *"`main`, then `container`, then
`console`, then `variable`"*, but §3's program meets `variable` inside the parameter
type **before** the next line reaches `console`. A literal walk gives `variable` 5 and
`console` 6; §2.1's table has the reverse. **The numbers are frozen and right — the
sentence was the imprecise part**, and the file already contained its own answer:
**§2.1 says *1 to 15 were written by hand***, and §3's last line proves it, because
`satellite.return` is met there and is **15**, not 7. §1.1 and DESIGN §4.1 both now
say that the walk is why the order is this one and not another, and is the rule that
decides every number from 16 on — **not a procedure that regenerates 1 through 15.**
No number moved. *(This is the one edit made to WORD_NUMBERS.md, and it was asked
for.)*

**Nothing was left open from this half.** PLAN M8's ordering question was put to the
author and answered the same day — §5.15.

### 5.15 2026-08-28 — the empty list is M10's, and M8 now runs after M10

**The author's decision, asked for and given before the clear.** Hello world's
restored parameter needs one empty `satellite.container.list` and nothing else.
**That list is M10's**, and **M8 moves after M10.** PLAN §8's build order is now
M7, M9, M9.5, M10, **M8, M8.5**, M11.A — and M8 keeps its name.

**Why the name did not change.** Seven documents cite "M8" — DESIGN §3, LAYOUT,
MILESTONE.md, THREADS.md, MILESTONE_DRAFTS.md among them — and renumbering to make
the list read in sequence would rewrite all of them to say something the numbers
never promised. **§8's opening now states that the numbers are assignment order and
the list is build order**, which is WORD_NUMBERS §1.2's own rule applied to the plan
instead of the language: never renumber, and record the order where it can be read.

**The argument written into M8 and M10, so it is not re-litigated:** an empty
`satellite.container.list` is still a `satellite.container.list`, and a milestone
that constructs one has built the type. M8 owning a private empty-list shape that
M10 later replaces is the thing that looks free and is found later as two
implementations of one type.

**ONE CONSEQUENCE WAS OPEN FOR ABOUT AN HOUR AND IS NOW CLOSED — see §5.16.** M8
builds *"Console with its printer thread"*, so moving all of M8 after M10 would have
left **M9 and M10 with nothing to print through**, against PLAN §8's own opening rule
that a milestone must be demonstrable. **The author split M8 the same session.**

Propagated to DESIGN §3, LAYOUT's hello world row, PLAN M8 / M8.5 / M10 / §8
opening, and MILESTONE_DRAFTS' M8 handover note.

### 5.16 2026-08-28 — M8 is split, and the build order is settled

**The author's second decision of the session, and it closes the consequence §5.15
opened.** M8 was one milestone; the empty list ruling would have moved all of it
after M10 and left M9 and M10 with no console. **Only the parameter ever needed a
list**, so the split falls exactly there:

    M8.A   the console, satellite.main, satellite.return    between M7 and M9
    M8.B   hello world — DESIGN §3, and the parameter       after M10
    M8.5   satellite.help                                   after M8.B

**Build order in PLAN §8 is now** M1, M2, M3, M4, M4.5, M5, M6, M6.5, M7, **M8.A**,
M9, M9.5, M10, **M8.B**, M8.5, M11.A, M11.B, M12, M13. Naming follows the author's
own M11.A/M11.B precedent.

**M8.A is the milestone at which satellite executes anything at all**, which is why
everything from M9 on depends on it. **Its done-when cannot be DESIGN §3** — §3's
hello world declares the parameter and the parameter is M8.B's — so what runs there
is the **bare `satellite.main()` form**, still legal under §6's grammar and kept by
DESIGN §3 on purpose. **`example/` holds no bare-main program**, so M8.A has no
acceptance file; writing one is the author's, and until then its done-when is prose,
which §8's opening calls the weaker kind. **That is the one thing this split leaves
open.**

**A disambiguation rule is written into §8's opening, because ~30 citations of "M8"
exist across the tree.** A bare "M8" written before 2026-08-28 means **M8.A**, the
console — MILESTONE.md's console rows, MILESTONE_DRAFTS' console draft, QUAD §4's
*"nothing before M8"*. The exceptions name **hello world or DESIGN §3 as a
done-when** and mean **M8.B**. Corrected in DESIGN, LAYOUT, QUAD, PLAN, MILESTONE.md,
THREADS.md and the drafts' M8 handover note. **Deliberately not corrected: the dated
adversarial findings in MILESTONE_DRAFTS.md**, which are verbatim records of what a
review said on a day — rewriting them would falsify the record, and §8's rule covers
reading them.

**One thing noticed and left alone.** M8.5's real floor is **M8.A**, not M8.B — help
needs a console and a `main` to run inside, not the parameter — so it could land
right after M8.A and give M9, M9.5 and M10 a live account of themselves while they
are being built. It is left where the split put it because moving it is a second
decision; PLAN M8.5 says so, and says it moves without consequence.

### 5.17 2026-08-28 — M2 LANDED

**START HERE AFTER A CLEAR.** Nothing is running. `make` builds four binaries
clean, `make test` passes, and nothing is committed — `git status` is the list.

**What is on disk that was not before:**

    src/satellite_words/   words.def (542 lines) + 9 headers + dump.cpp
    tests/words_test/      5 files, the tree's FIRST test
    make_support/065-tests.mk

**The encoding, which is the part not to re-derive.** `words.def` is
`SAT_NODE(parent, ident, text, kind)` and **the number is a POSITION, not a
column** — computed at compile time in one forward pass, which is why a parent
must be declared before its children and why that is asserted. `kind` is
`SAT_NUMBERED` or `SAT_BARE`; a bare row is WORD_NUMBERS §1.3's `(0)`, takes
position 0 and **does not advance its parent's counter**. `text` is §2.2's path
column with the parent's path removed, character for character, and a text
beginning with `(` joins with no dot — which is what lets `path_text()` reproduce
the authority's path column exactly, checked for all 219 non-alias rows.

**254 nodes, and that count was in no document.** 216 numbered + 38 bare. §2.2's
219 does not include the bare shape a `(0)` marker names, because the marker rides
on the row of the node it belongs to.

**Three of PLAN M2's four properties needed no assert**, which is the encoding
paying off: no holes and no duplicates are unrepresentable when a number is a
position, and an alias naming an identifier makes property 4 a compiler error.
Only *no orphans / no cycles* was left. `words_invariants.hpp` says so out loud
and adds eight more the encoding needs.

**THE MUTATION TEST IS THE THING TO REMEMBER.** Deleting one row from `words.def`
— `satellite.console.typed()` `1 5 5` — **compiles clean, every static_assert
passing**, because the file stays internally consistent; it just silently
renumbers every later sibling. `tests/words_test` caught it and named the missing
row plus the four it shifted. That is why the test **opens WORD_NUMBERS.md at run
time** and walks all 222 paths rather than the two PLAN names.

**Measured, not quoted.** M1 `satl` 1.60 ms, M2 `satl` 1.61 ms, bare
`int main(){return 0;}` 1.59 ms, noise floor ~0.02 ms — best of seven runs of 200
invocations, all binaries on the same filesystem. M2 costs nothing at startup
because the tables are `constexpr` rodata. *(The first attempt put the bare binary
in `/tmp` and reported satl as faster than an empty main. A result that cannot be
true is a setup difference, not a finding.)*

**Decisions the author made this session:**

- **the test reads WORD_NUMBERS.md and checks all 222 rows** rather than PLAN's two
- **a real allocator, not a stub** (PLAN §8.1's open question), **and M4 is marked
  as the milestone that starts calling it**
- **the digest is over the numbering, not over `words.def`'s bytes** (SATC §6)
- **PLAN §3's line rule is now "try to build for 300 lines"** — the author softened
  it mid-session after a header came out at 305. It is a target, not a ceiling.
  FORMAT/CXX.md §1 and §10 match. *"We can modularize later."*

**What moved in the permanent documents:** PLAN §1 (M2 landed, the measurement),
§3 (the softened rule), §8 (M2 landed + what it decided, M3 next, M4 owns the
allocator's first call, §8.1's question answered); FORMAT/CXX.md §1, §5, §6, §7,
§8, §9 (all ten blockers answered), §10; SATC.md §6; LAYOUT.md (the module, the
new `tests/` section, 065-tests.mk, and two stale claims fixed — `.start()` and
`.join()` were numbered on 2026-08-28 and LAYOUT still said they were not).

**WORD_NUMBERS.md was not touched.** It is the author's and it is the authority;
M2's job was to transcribe it, and the test is the proof that it did.

**What M2 did NOT do**, said plainly so nobody reads this as more than it is:
nothing executes, there are no handlers, `handlers[path_id]` appears in this
module only in comments, and `satl --words` ends by saying so. The 121 unscheduled
paths of `SCRATCH.md/MILESTONE.md` are all numbered now and none is built — which
is exactly the gap DESIGN §4.6 was corrected for, and M8.5's job.

### 5.18 2026-08-28 — what the adversarial review of M2 found

**Fifteen findings confirmed out of twenty-nine raised**, six lenses over the M2
implementation with each finding independently attacked before it was believed.
**All fifteen are fixed.** Recorded because the four code defects are the kind that
come back.

**Three real bugs in the walk and the allocator, all reachable:**

1. **An empty path segment matched every `(`-row.** The 38 bare rows and the 6
   argument rows have an EMPTY spelling by design, so an empty `word` compared
   equal to all of them and the walk handed back the bare shape.
   `satl --words satellite.console.` answered `1 5 0` and **exited 0**;
   `satellite.include.(satellite)` answered `1 1 1`. Fixed with a guard in
   `match_shape()`, and four regression checks in `walking.cpp`.
2. **`Words::find` never consulted the aliases**, so
   `intern(satellite.library.main, "args")` allocated a **user** number for one of
   DESIGN §7.7's six spellings of `arguments` — while `walk()` answered `1 14 1 1`
   for the same spelling under the same parent, at the same moment. Two numbers for
   one word, and a parser would have called the half that was wrong. The comment
   above the function claimed the opposite.
3. **`find("")` returned the parent's bare shape** as a language word, which
   `define()` had always refused and `intern()` reached `find()` before ever getting
   to that refusal. Same root cause as (1).

**One real build defect.** The test binary had no `.cxxflags-stamp` prerequisite,
so **`make OPT=-O0 test` and `make CXX=g++ test` re-ran the binary the previous
build left** — printing `ok` from a clang -O3 build while the command line said
g++ -O0. Most of `words_test` is `static_assert`s, so "does the registry compile
under this compiler at this -O" IS the test, and that is exactly what was skipped.

**The test was half-blind, and the half it could not see is the everyday edit.**
`section_authority()` walked the authority INTO the code, which only ever catches a
row `words.def` is **missing**. A row it has and the authority does not takes no
number from anybody: appending `satellite.container.set` at `1 4 5` — which
WORD_NUMBERS §4 says is deliberately free — compiled clean and the suite printed
`ok`, with the digest silently moved and every cached `.satc` invalidated. **The
converse is now checked**, derived from the authority so no number gets a second
home: `kNodeCount == rows - aliases + markers` and `kAliasCount` from §2.2 plus
§2.3. Verified by mutation in an isolated copy.

**Six comment and count errors, in a tree whose whole culture is that a comment
states a finding.** `if`/`for`/`while`/`else` called segment-1 words when
`statement` is the one of the eleven and they are its children; "the map's
twenty-nine methods" when the map has nine and ten rows sit between; `dump.cpp`'s
"measured" longest path at 48 characters when it is **50** and a different row;
"nine spellings over four nodes" when it is five; `060-compile.mk` and FORMAT §5
still calling the haswell pattern rule unreached in the milestone that reached it;
`040-sources.mk` still saying "there is no test target in this tree" in the
milestone that added one. And **065-tests.mk claimed an include-order constraint it
does not have** — the real one, 030 before 065, was stated nowhere.

**One pre-existing document error, found by counting:** the container methods are
**34**, not 29. `MILESTONE.md` §0.3 counted the list at twenty when §2.2 gives it
twenty-five, and PLAN M10 inherited the 29 into a sentence naming nine and
twenty-five two lines above. **§0.3's "35 implied" is 40.** Corrected in both.

**One process failure that is mine and worth not repeating.** A review agent ran
mutation probes against the **live working tree** rather than a copy, and for a few
seconds `words.def` carried two rows it should not have. Nothing was lost —
`WORD_NUMBERS.md` is byte-identical to HEAD and `words.def` is back at 254 nodes,
both verified — but the author saw it and asked. **A review that mutates files must
be given its own copy of the tree**, and the sibling agents also collided in a
shared scratch directory. Neither was instructed; both should have been.

### 5.19 2026-08-28 — WHAT IS ACTUALLY LEFT, after M2 and the crossover

**Read this one first if you are picking the work up cold.** Everything above is
history; this is the state. **Six commits landed 2026-08-28**, in order:

    d9ff549  M2 lands -- the registry, the trie, the interner, --words, the test
    bb0af1c  the crossover measured; the bare-alias fix; ledger closes
    c48014d  four more ledger rows struck; §5.19 written
    c368a29  correction: the 170 is a second-batch number, a cold pool crosses
             where spawning does
    04b4329  eager warming beats lazy; a size trigger proposed
    8b0ad56  the author's decision: the pool starts at startup, ALWAYS --
             which supersedes both of the two above it

**The last three are one argument that changed answer twice**, so read only
`8b0ad56` and PLAN §4.5.1.2 as current; the two before it are kept because each
was measured and each was wrong for a reason worth not repeating.

**Nothing is running. The build is clean, `make test` passes, four binaries.**

#### Closed today, and not to be re-derived

| was open | now |
|---|---|
| M2 itself — the trie, the interner, `PathId`, the digest, `--words`, the test | **built, committed, tested against all 222 rows of the authority** |
| FORMAT/CXX.md §9's ten blockers | **all ten answered**, each recorded in §9 beside what settled it |
| "call shapes sit at two depths" | **one rule seen twice**; §1.3's `(0)` had already settled it and no number moved |
| SATC §6 — what the digest is over | **the numbering, not the file's bytes** |
| PLAN §8.1 — real allocator or stub | **real**, and M4 is marked as its first caller |
| PLAN §4.5.1's crossover | **measured**: ~2,650 lines against fresh threads **or a cold pool**; **~170** only against a pool something else already warmed |
| where the thread pool is built | **decided by the author: at startup, always** (PLAN §4.5.1.2). Both "lazily, on first threaded work" and the size trigger are superseded |
| `WORD_SURFACE.md`'s delete-when | **met and verified** by walking all 95 paths; the file is deletable |
| MILESTONE.md's binary/`.hex` row | **struck** — PLAN M3 owns them and says so |
| PORTING.md item 3 (`satellite.random`'s directory) | **settled by the tree** — `src/satellite_random/` exists |
| PORTING.md item 1 (`Number`'s library dependency) | **half dissolved** — the registry exists now; what is missing is anywhere to keep values |
| the container methods count | **34, not 29**, in PLAN M10 and MILESTONE.md §0.3, whose "35 implied" is 40 |

#### Open, and ONLY THE AUTHOR CAN ANSWER

1. **The float's rounding rule — truncate, half-up, or half-even.** M9.5 cannot
   land without it and QUAD §3.1 calls the float the whole remaining gap. **This is
   the single most blocking open question in the project.**
2. **Is `division_digits` `1 14 2 1` the same dial as `float_digits` `1 14 2 4`?**
3. **`MEMORY_MAX`'s unit and default** — 61.9 GiB and 64.9 GB are the same memory.
   Whole machine or a fraction, and does a smaller machine win? PLAN §4.5.4.
4. **`satellite.file.new` `1 8 1` vs `satellite.variable.file.new` `1 6 2 1`** —
   which does a program write? Two spellings for one construction.
5. **`satellite.thread.new`, one number for two shapes.** `1 23 2` is free if it
   goes the other way.
6. **The seventh spelling of `arguments`** — `argv` gets a plain list silently, and
   DESIGN §9 says that silence is wrong.
7. **What is `arguments[0]`** — program name, current directory, or first argument?
8. **Scheduling.** 121 of 222 paths reach no milestone (`MILESTONE.md`), and the
   **thread pool now has a measured justification, a decided shape and still no
   milestone** — which after today is the sharpest gap in §8.

9. **`parallel_for` has to be designed, numbered and scheduled.** PLAN §4.5.1.2
   rests on it — *"we almost have to assume the user will call parallel_for"* — and
   it appears in no numbering, no milestone and no document. It needs a call shape
   (WORD_NUMBERS §1.3), a number under whichever parent owns it, and a milestone.
   **This is the newest and largest open item in the project.**

10. **`satellite_config.ini` moved onto the critical path.** It was a later nicety;
    §4.5.1.2 makes `THREAD_COUNT` something satl reads before it builds the pool,
    which is before it does anything else. Its unit-and-default question (§5 q2)
    now blocks startup rather than a setting.

#### Open, and MINE — no decision needed, only work

- **The threading conclusion is settled and is in PLAN §4.5.1.1.** It took three
  measurements in one day and each changed the answer, so the short version:
  **lazy-on-first-use is dominated** — warming from startup beats it at every size,
  because the main thread never waits for the pool and kicking the build off costs
  it ~20 µs rather than ~590. **But warming is not free either**: it runs at
  0.42×–0.60× of single-threaded between 100 and 1,000 lines, because creating 23
  threads contends with the main thread's own page faults. And **file I/O hides
  nothing** — reading hello world takes 4.4 µs against ~600 µs to warm.

  **AND THEN THE AUTHOR DECIDED IT, and the size rule is gone.** PLAN §4.5.1.2:
  *"satl almost must start the THREAD_COUNT in satellite_config.ini because we
  almost have to assume the user will call parallel_for... MOST satellite code will
  require 24, so start them now."* **The pool is built at startup, always.**

  That is right and my size rule was wrong, because every measurement I took was
  against **parse-time interning** and the pool's real tenant is the **running
  program** — a ten-line program can run a million-iteration parallel loop, so
  source size predicts parse cost and not parallelism. In absolute terms the cost
  to a program that never threads is **+21 µs at 100 lines and +47 µs at 500**,
  against 1,750 µs of process startup; a program that does thread saves ~590 µs of
  blocked execution at its first parallel call. **PLAN §4.5.1's "lazily, on first
  real threaded work" is superseded, and so is §4.5.1.1's size trigger.**

  Two things this carries: the main thread must **not** build the pool itself
  (spawn one, let it build 23 — 20 µs instead of 590), and `THREAD_COUNT` has to be
  readable first, which means `satellite_config.ini` — **which still does not
  exist** and is now on the critical path for startup rather than a later nicety.

  **Two facts flagged to the author and not blocking:** `parallel_for` exists in no
  numbering, no milestone and no document — the whole parallelism surface is
  `satellite.thread.new` and `.start()`/`.join()` — so the decision assumes a
  construct that still has to be designed, numbered and built. And **QUAD spawns
  zero threads**, so the one program the language exists to express would never
  touch the pool.

  **The `.satc` write is a different job and is unaffected**: SATC §5 keeps it on
  its own thread always, which is one thread hiding disk latency, not
  twenty-four splitting work.

- **Write `Sky::decay` and `Rack::draw` in satellite by hand** against DESIGN.md.
  QUAD §5 calls it the smallest thing that would prove the language works, and it
  is the largest remaining item that needs nothing from the author. **It will stop
  at the rounding rule** — that is the point of writing it, and it is worth doing
  before M9.5 rather than after.
- Everything from M3 on. **M3, the lexer, is next**, and it owns
  `satellite.variable.binary` and `.hex`, whose literals the lexer decides.

## 6. Jobs the user has asked for that are not started

- **Convert `plans/madness/first_note.txt` into the permanent documents, then
  delete it.** Its content is mostly already in DESIGN and PLAN; what is *not*
  captured anywhere else is noted in [FIRST_NOTE.md](FIRST_NOTE.md).
- **Port `satellite_number` and `satellite_string` from the first satellite** into
  `src/satellite_number/` and `src/satellite_string/`, which exist and are empty.
  See [PORTING.md](PORTING.md).
- **Rewrite `WORD_NUMBERS.md` in prose** — the user asked that their own notes be
  replaced by my writing, with their numbers preserved exactly.
