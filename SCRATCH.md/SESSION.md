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

**What IS done: the numbering itself.** WORD_NUMBERS.md §2.2 holds **144 numbered
entries** and nothing found by the sweep is unnumbered. It validates the way M2's
`static_assert`s will — checked mechanically on 2026-08-27:

    duplicate numbers that are not declared aliases      0
    parents with holes in their child list               0
    paths whose parent number is absent                  0

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

3. **Is `satellite.number.shift_left` a slip for `satellite.variable.number.shift_left`?**
   (§3.4.)

4. **`satellite.returns` still has no number** — DESIGN §6.1 lists it as a
   segment-1 form and DESIGN §13 marks the return-type syntax decided.

5. ~~In what order do the nine remaining top-level namespaces go?~~ **Done
   2026-08-27.** The author delegated the assignment; 16–24 went to `analyze`,
   `bool`, `directory`, `help`, `network`, `returns`, `system`, `thread`, `window`
   alphabetically, and everything below them followed by the next-lowest-free rule.
   **WORD_NUMBERS.md §2.2 now holds 144 numbered entries and nothing in the sweep
   is unnumbered.**

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

Stated explicitly at the end of the session, before the clear, so it is not lost:

> *we need to be able to express quad_infinity*

**[QUAD.md](../QUAD.md) is that document** and it is the first thing to read after
this one. `/home/madness/code/cxx/quad_infinity/` is 3029 lines of C++ that run a
model of a mind, and expressing it in satellite is the literal purpose of the
language — general-purpose everywhere else, but that program is the acceptance
test.

QUAD.md §3 is the gap list. Four things satellite has not settled that QUAD needs:
**floats** (DESIGN §13 open), **a set container** (not numbered), **sorting** — which
is blocked on DESIGN §12's deferral of capsules-as-values — and **a console that can
address a screen**. QUAD.md §5 says what the honest next step is: take one mechanism
out of `mind.hpp` and try to write it in satellite by hand against DESIGN.md, which
will find gaps this list does not have.

The author expects this to be a long planning conversation and said so.

## 6. Jobs the user has asked for that are not started

- **Convert `plans/madness/first_note.txt` into the permanent documents, then
  delete it.** Its content is mostly already in DESIGN and PLAN; what is *not*
  captured anywhere else is noted in [FIRST_NOTE.md](FIRST_NOTE.md).
- **Port `satellite_number` and `satellite_string` from the first satellite** into
  `src/satellite_number/` and `src/satellite_string/`, which exist and are empty.
  See [PORTING.md](PORTING.md).
- **Rewrite `WORD_NUMBERS.md` in prose** — the user asked that their own notes be
  replaced by my writing, with their numbers preserved exactly.
