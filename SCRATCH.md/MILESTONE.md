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

**50 of the 215 numbered paths in WORD_NUMBERS.md §2.2 belong to a top-level
namespace that PLAN §8 never mentions — not in a milestone, not in "Later", not in
prose.** Counted mechanically 2026-08-27.

| namespace | numbered paths | said about it anywhere in §8 |
|---|---:|---|
| `satellite.system` | 30 | nothing |
| `satellite.network` | 8 | nothing |
| `satellite.directory` | 6 | nothing |
| `satellite.bool` | 3 | nothing — and this is **not** `satellite.variable.bool`, which M9 has. It is the module-constant namespace `satellite.bool.true` / `.false` (DESIGN §6.1) |
| `satellite.analyze` | 1 | nothing |
| `satellite.help` | 1 | nothing — DESIGN §4.6 says help "becomes a walk of the trie", which makes it nearly free once M2 lands, and nobody has scheduled it |
| `satellite.returns` | 1 | nothing — DESIGN §13 records the return-type syntax as **decided** and DESIGN §6.1 lists it among the eleven segment-1 words with their own parse rule, so **M3 or M4 has to parse it** whether or not anyone scheduled it |

`satellite.returns` is the sharpest of these: it is a *parse rule*, so a milestone
already owns it and does not say so. The other six are runtime namespaces that could
sit late without hurting anything — but "could sit late" is a decision, and right now
it is an omission.

---

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

## 2. ~~`satellite.variable.float`~~ — SETTLED 2026-08-27, it is M9

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

Said plainly so nobody reads this file as complete:

- **Only PLAN §8 was audited.** A thing mentioned in DESIGN or SATC with no milestone
  and no §8 mention would be caught; a thing mentioned nowhere at all would not.
- **The 165 numbered paths under milestoned namespaces were not checked individually.**
  `satellite.variable` is named by M9, which does not mean every one of its fifteen
  children is scheduled — M9 names `bool`, `number` and `string` and leaves `binary`,
  `date`, `duration`, `expression`, `float`, `hex`, `network`, `thread`, `variant`,
  `window` and `capsule` unaccounted for. §1 and §3 above catch several of those by
  other routes; a full per-path sweep has not been done.
- **`src/` was not audited**, because there is almost none of it: five files, 459
  lines, all M1.

---

*Companions: [PLAN.md](../PLAN.md) §8 is the list this file is the complement of.
[WORD_NUMBERS.md](../WORD_NUMBERS.md) §2.2 is the 215 paths it was counted against.
[QUAD.md](../QUAD.md) §4 is the milestone that does not exist.
[SESSION.md](SESSION.md) §5.7 is what is open for other reasons.*
