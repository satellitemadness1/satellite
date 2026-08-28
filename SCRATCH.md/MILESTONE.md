# Work with no milestone — the ledger

**This file is scratch and is meant to be deleted.** It exists because PLAN.md §8 is
supposed to be the whole of the work, and an audit on **2026-08-27** found that it is
not. Everything below is real, specified somewhere, and belongs to no milestone.

**Delete this file when every row has a milestone in PLAN §8.** Each row that gets
one should be *moved* into §8, not copied — a job in two places is a job that gets
done twice or not at all.

The rule this file enforces, from PLAN §8's own opening: *"Each milestone is a thing
that **works and can be demonstrated.** No milestone is 'the parser is half done.'"*
A path with a number and no milestone is the other failure — a thing that is
half-*specified* and will be finished by accident.

---

## 0. The headline

*Third audit, 2026-08-27, and the first one that actually counted.* The two before it
swept **top-level namespaces** and reported **50**, then **41**, then **about 29**. Its
own §5 said why that number could not be trusted:

> **The 165 numbered paths under milestoned namespaces were not checked
> individually.**

That sweep has now been done — **all 218 rows of WORD_NUMBERS.md §2.2, one at a time,
against PLAN §8.** The answer is not 29.

| verdict | paths | |
|---|---:|---|
| **named** — a milestone names the path or its number | **61** | 28% |
| **implied** — only a parent is named, never the path | **35** | 16% |
| **uncovered** — no milestone reaches it at all | **122** | **56%** |

**122 of 218, not 29.** More than half the language is unscheduled, and another 35
paths are covered only by a sentence about their parent. **96 covered, 122 not.**

Nothing was descoped between audits. The earlier numbers were the answer to a
different question — *"which top-level namespaces does §8 never mention?"* — and that
question cannot see a path hiding under a parent §8 happens to name. §0.3 found three
of those by hand and stopped; there were far more than three.

### 0.0 Two premises the last audit had backwards

Both moved paths **out** of the worst category on a reading of "Later" that the text
does not support. `grep -n 'satellite\.system' PLAN.md` returns **one line**, and it is
the audit paragraph saying the namespace is unscheduled.

- **`satellite.system`'s 30 paths were removed from §0.1** on the grounds that
  *"'Later' now names them."* **It does not.** The Later list is, in full:
  `satellite.variable.file`, `.time`, `.date`; `satellite.random.*`;
  `satellite.variable.variant`; spacesuits; `satellite.include` of other files;
  Satellite Orbit and the wire format. `satellite.system` is not in it. Those 30 are
  **nothing anywhere**, which is where §0.1 had them.
- **`satellite.file` `1 8` (5) and `satellite.time` `1 9` (4) were filed as
  "covered only by Later."** Later names `satellite.variable.file` and
  `satellite.variable.time` — **different nodes**, exactly the distinction §0.2 drew
  correctly for `satellite.bool` and then missed here. Nine more paths with nothing
  anywhere.

That is **39 paths** in the wrong category, all in the direction of looking better.

### 0.1 Where the 122 are

| namespace | uncovered | of | what reaches it |
|---|---:|---:|---|
| `satellite.system` | **30** | 30 | nothing. Not §8, not Later, not prose |
| `satellite.random` | **16** | 16 | Later only, which is not a milestone |
| `satellite.library.*` | **11** | 15 | the whole `arguments` subtree — see §0.2 |
| `satellite.network` | **8** | 8 | nothing |
| `satellite.variable.file` | **8** | 8 | Later only |
| `satellite.console` | **8** | 10 | M8 builds the *printer* — see §0.2 |
| `satellite.container` | **7** | 39 | sort ×5, `.arguments`, `.result` |
| `satellite.directory` | **6** | 6 | nothing |
| `satellite.variable` leaves | **7** | 16 | `time`, `date`, `duration`, `expression`, `network`, `variant`, `capsule` |
| `satellite.file` | **5** | 5 | nothing — a different node from `variable.file` |
| `satellite.time` | **4** | 4 | nothing — same mistake, same shape |
| `satellite.bool` | **3** | 3 | M9 says outright it is not M9 |
| `satellite.help` | **3** | 3 | nothing. DESIGN §4.6 makes it a trie walk, nearly free |
| `satellite.thread` | **2** | 2 | M12 names only `satellite.variable.thread` |
| `satellite.window` | **2** | 2 | M13 says "windows" in prose |
| `satellite.analyze` | **1** | 1 | nothing |
| `satellite.include(spaceship)` | **1** | 5 | loading another file is Later |
| **total** | **122** | **218** | |

*(Four paths added 2026-08-28, after this sweep ran: `satellite.window.console`
`1 24 2` and its `new` `1 24 2 1`, which **M11.A names**, and
`satellite.variable.thread.start()` `1 6 13 1` and `.join()` `1 6 13 2`, which
**M12 does not** — it names only `satellite.variable.thread`. So the current
figure is **124 of 222**, and the two new uncovered ones are the same M12 gap
`example/thread_test.satl` already demonstrates.)*

`satellite.system` alone is a quarter of it, and `system` + `random` is **46** — more
than the last audit's whole figure.

*(Corrected 2026-08-27, second pass: this column used to sum to **121** against a
headline of 122, and the headline was the right one. The `satellite.variable`
leaves row said "6 of 12" — `satellite.variable` has **16** leaves, `1 6 1`
through `1 6 16`, and the leaf it omitted was `satellite.variable.time` `1 6 3`,
which "Later" names and no milestone does. A row that does not sum to its own
total is the cheapest error in this file to find and it survived two readings.)*

### 0.2 The new gaps — found only by going path by path

These are the ones no namespace count could reach. Each sits under a parent some
milestone names, so the parent looks covered. This is §0.3's category, finished.

| what | number | hides under | the gap |
|---|---|---|---|
| **the whole `arguments` subtree** — `.machine.cores` / `.cpu` / `.threads`, `.memory.total`, `.username` | `1 14 1 1`–`1 14 1 1 3` | `library`, which M4 parses | **8 paths.** DESIGN §7.7 specifies it, PLAN §4.5.3 argues about it, no milestone builds it. The largest single find of this audit |
| **`satellite.console.input`, all three shapes** | `1 5 2`–`1 5 4` | `console`, which M8 names | M8 is *"Console with its printer thread."* Reading a line is not the printer. The last audit caught `typed()` and missed blocking input entirely |
| **`satellite.variable.duration`, `.expression`** | `1 6 8`, `1 6 9` | `variable`, which M9 names | not in §8, not in Later, not in prose. §5 listed them as "unaccounted for" and never counted them |
| **`satellite.library.main` and `.system`** | `1 14 1`, `1 14 2` | `library` | §8.1 discusses their *numbering* at M2 and nothing builds them |
| **the 29 container methods** | `1 4 1 1`–`1 4 2 25` | M10 names both types | not counted as uncovered here, but see §0.3 — M9 says *"and their methods"* and M10 does not |

### 0.3 The 35 implied — covered by a parent's sentence and never by name

Not counted in the 122, because a fair reader would say the milestone means to include
them. But **M9 writes `satellite.variable.bool`, `.number`, `.string` and their
methods** — the author knows to say "and their methods" when they mean it, and M10
does not say it:

- **29 container methods.** M10 is *"containers and the search power."* Nine map
  methods and twenty list methods, none named. The five sort paths are already known
  not to be M10's, which proves the wording does not automatically reach a child.
- **`satellite.variable.window` `1 6 15`** — M13 names the `.so`, not the type.
- **`division_digits`, `max_depth`, `float_digits`** — M9.5 asks whether
  `division_digits` is the same dial as the float precision; asking is not owning.
- **`satellite` `1` itself**, the runtime singleton, and
  `satellite.include(satellite)` `1 1 1` — DESIGN §3's hello world uses both and M8
  runs DESIGN §3.

**Adding one clause to M10 — "and their methods", the words M9 already uses — moves 29
paths from implied to named.** That is this audit's cheapest fix and the direct
descendant of the last one, where naming DESIGN §6.1's table moved eight.

### 0.4 One ordering problem, which is not a coverage gap

M9 owns `satellite.variable.number`'s fourteen methods. Four of them cannot be
finished at M9: `power` `1 6 4 10`, `sqrt` `1 6 4 14` (*"irrational in general, so it
rounds"*), `modulus` `1 6 4 12` (cites DESIGN §8.6, the float spec) and `truncate`
`1 6 4 13` (*"on a float this is just its left half"*). **Rounding is M9.5's blocker
and M9 comes first.** Either those four move to M9.5 or M9.5 moves ahead of M9.

## 1. Added on 2026-08-27 and given no milestone

These came out of settling QUAD.md §3. All are in WORD_NUMBERS.md §2.2 and none is in
PLAN §8.

| what | number | where it probably belongs | why it is not obvious |
|---|---|---|---|
| `satellite.console.typed()` and the **reader thread** | `1 5 5` | new, between M8 and M11 | M8 builds the console's *printer* thread; M11 builds the REPL. The **input** thread (DESIGN §10.1, the polarity flipped again) is neither, and QUAD needs it before either |
| `satellite.console.width` / `.height` / `.clear()` / `.home()` | `1 5 6`–`1 5 9` | with the above | terminal facts, not console I/O. Might belong with `arguments.machine.*` instead |
| `satellite.variable.capsule` — the deferred call | `1 6 16` | **M12 at the latest, and probably earlier** | M12 says only `satellite.variable.thread`. But `satellite.thread.new(f(x))` cannot work without the packaging semantics, and DESIGN §12/§13 now lean on it to keep §2 shut. If it slips, so does the argument that sorting did not need first-class capsules |
| `satellite.time.sleep(n)` | `1 9 3` | "Later" has `.time` | QUAD's main loop cannot run without it, so it is not "later" in any sense QUAD.md §4 recognises |
| `satellite.library.system.float_digits` | `1 14 2 4` | with the float | §2 below |
| the **sort primitive** — `sort_down(key)` and friends | `1 4 2 3`–`1 4 2 7` | M10, presumably | M10 says "containers and the search power" and never says **sort**. The search power is the v1 comparator/walker ladder, which is a different thing |
| the **literal-option fold** (WORD_NUMBERS §1.5) | — | M6 or M7 | it is a resolve-time decision that changes which `PathId` a call site interns. Nothing says whether resolve (M6) or closure compilation (M7) owns it |
| `satellite.variable.file`'s five methods | `1 6 2 3`–`1 6 2 7` | "Later" has `satellite.variable.file` | QUAD's `.sky` persistence needs them, and QUAD.md §4 puts QUAD's needs at M10 |

## 2. ~~`satellite.variable.float`~~ — SETTLED 2026-08-27, it is M9.5

**Was:** PLAN §8 filed it under *"Later, in no fixed order"* while QUAD.md §3.1 called
it *"the whole remaining gap"* and DESIGN §13 called it *"on the critical path."* Two
permanent documents in direct contradiction about whether the acceptance test's one
remaining blocker was urgent.

**Now: its own milestone, M9.5**, with `satellite.variable.number` pulled out of M7
into **M6.5** beside it. Both were bullets inside larger milestones and both are types
with specifications of their own. They are apart rather than merged because M7's
`Value` contains a `Number` — so number must precede M7 — while the float waits on a
rule nobody has chosen.

**M6.5 builds the sign both types share** (an explicit `positive` bool defaulting to
true, DESIGN §8.1), so M9.5 inherits it instead of defining a second one. That shared
sign is the reason they can be two milestones instead of one large one.

**M9.5 cannot land until the rounding rule is decided** — truncate, half-up, or
half-even — which is the forcing function "Later, in no fixed order" removed.

It also carries work that is not just "build a type":

- the **rounding rule**, which DESIGN §13 now argues is part of the type rather than a
  setting on it, because `pow` at a fractional exponent has no exact value
- the **default precision**, and whether
  `satellite.library.system.division_digits` `1 14 2 1` is already the same dial
- **DESIGN §8.1's amendment**, which is written but describes a type that does not
  exist

## 3. Specified in PLAN itself, and never scheduled

Pre-existing — these were already un-milestoned before 2026-08-27.

| what | specified in | status |
|---|---|---|
| `satellite_config.ini` — `THREAD_COUNT`, `CORE_COUNT`, `MEMORY_MAX` | PLAN §4.5 | *"Specified here; not built."* No milestone. The file does not exist and nothing reads or writes it |
| the **lazy thread pool**, sized from `THREAD_COUNT`, shared by the console printer, parse-time interning and M12 | PLAN §4.5.1 | no milestone. M12 is the closest and says nothing about a pool |
| the **memory ceiling** and the watchdog that enforces it | PLAN §4.5.2 | no milestone. v1's `memory_facts.cpp` is most of the machinery |
| the **crossover measurement** — how many satellite-rooted source lines before 24 threads beat 1 | PLAN §4.5.1, SESSION.md §3.2 | *"That is not yet measured."* It decides whether M2's interning is threaded at all, so it blocks a decision **inside M2** |
| porting `satellite_number` and `satellite_string` | PLAN §6.1, SCRATCH.md/PORTING.md | surveyed in detail, four open questions listed, **no milestone**. M7 needs `Number` and does not say the port happens there |
| **`satellite.variable.binary` and `.hex`** — DESIGN §8.5 says these are real types with literals and that the width is part of the value | DESIGN §8.5 | numbered `1 6 5` and `1 6 11`. No milestone. The *lexer* has to know about them, so M3 may already own them without saying so |
| **spacesuits** — `satellite.spacesuit`, `.protected`, `.public` | DESIGN §13 (decided), §6 grammar | "Later". A whole feature, with a grammar rule already written, in the unordered pile |
| **Satellite Orbit and the wire format** | PLAN §8 "Later" | fine where it is, but WORD_NUMBERS §3 and SATC.md §3 both impose a constraint on it (never persist a user PathId), and nothing will check that until it exists |

## 4. The milestone QUAD.md asks for and PLAN does not have

QUAD.md §4, unchanged since the first draft and still true:

> **it suggests a milestone that does not exist yet:** a milestone whose done-when
> condition is *a piece of QUAD, running.*

QUAD.md §5 now names the candidate: **`Sky::decay` plus `Rack::draw`** — between them
they touch floats, the map, a weighted pick, and the one `pow` that has no exact
answer. That is the smallest thing that would prove the language works, and it has no
number in §8.

## 5. What this audit did *not* check

Said plainly so nobody reads this file as complete. **The per-path sweep this section
used to call for has now been done** — all 218 rows, §0 is its result. What is left:

- **Only PLAN §8 counts as a milestone.** That is the rule, not a limitation: §8's own
  opening says a milestone is *"a thing that works and can be demonstrated."* But it
  means a path DESIGN specifies in full and §8 never mentions reads here as
  **uncovered** even though the design work is done. `satellite.system`'s 30 are the
  large case — WORD_SURFACE.md swept them out of v1, so most already have an
  implementation to port. **Uncovered means unscheduled, not unknown.**
- **Coverage was judged from §8's words, not from what a milestone would have to build
  anyway.** M8 cannot run DESIGN §3 without the `satellite` singleton, so the singleton
  is real work M8 must do; it is filed as *implied* because M8 does not say so. Every
  one of §0.3's 35 is that shape, and the M3/M4 lesson is that this is the dangerous
  half, not the safe one.
- **The 122 were not sized.** `satellite.help` is 3 paths and DESIGN §4.6 makes it a
  walk of the trie — nearly free once M2 lands. `satellite.network` is 8 paths and a
  protocol stack. They count the same in §0.1's table and are not the same work. **The
  count is a coverage figure, not an estimate.**
- **The three aliases were counted as their own rows**, because §2.2 lists them:
  `random.fast.range`, `.normal.range`, `.ultra.range` share numbers with the call
  shapes above them (§2.3). A by-number count would be 215, not 218.
- **`src/` was not audited**, because there is almost none of it: five files, 459
  lines, all M1.

---

*Companions: [PLAN.md](../PLAN.md) §8 is the list this file is the complement of.
[WORD_NUMBERS.md](../WORD_NUMBERS.md) §2.2 is the 218 paths it was counted against.
[QUAD.md](../QUAD.md) §4 is the milestone that does not exist.
[SESSION.md](SESSION.md) §5.7 is what is open for other reasons.*
