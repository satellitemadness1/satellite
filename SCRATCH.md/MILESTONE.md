# Work with no milestone — the ledger, closed

**This file is scratch and is meant to be deleted.** It existed because PLAN.md §8
is supposed to be the whole of the work, and an audit on **2026-08-27** found that
it was not. **On 2026-08-28 every row in it was given a milestone**, and what
follows is the record of where each one went — kept rather than deleted because a
ledger that is thrown away the moment it balances leaves nobody able to check the
balancing.

The rule this file enforced, from PLAN §8's own opening: *"Each milestone is a thing
that **works and can be demonstrated.** No milestone is 'the parser is half done.'"*
A path with a number and no milestone is the other failure — a thing that is
half-*specified* and will be finished by accident.

**Its closing rule was that a row which gets a milestone is *moved* into §8, not
copied — a job in two places is a job that gets done twice or not at all.** That is
what the tables below record: every row is moved, and this file no longer holds any
work.

---

## 0. What the audit found, and what closing it cost

*Third audit, 2026-08-27, and the first one that actually counted.* The two before
it swept **top-level namespaces** and reported **50**, then **41**, then **about
29**. Its own §5 said why that number could not be trusted:

> **The 165 numbered paths under milestoned namespaces were not checked
> individually.**

That sweep was then run — **all 218 rows of WORD_NUMBERS.md §2.2, one at a time,
against PLAN §8** — and the answer was not 29:

| verdict | paths | |
|---|---:|---|
| **named** — a milestone names the path or its number | **61** | 28% |
| **implied** — only a parent is named, never the path | **35**, corrected to 40 | 18% |
| **uncovered** — no milestone reaches it at all | **122** | **56%** |

**Nothing had been descoped between audits.** The earlier numbers were the answer to
a different question — *"which top-level namespaces does §8 never mention?"* — and
that question cannot see a path hiding under a parent §8 happens to name. That is
the finding this whole file exists for, and it is the one worth carrying forward:
**the dangerous half is not the unscheduled namespace, it is the path under a parent
some milestone mentions.**

Four paths were added on 2026-08-28 after the sweep ran — `satellite.window.console`
`1 24 2` and its `new` `1 24 2 1`, which M11.A named, and
`satellite.variable.thread.start()` `1 6 13 1` and `.join()` `1 6 13 2`, which M12
did not — and `satellite.help`'s three moved out to M8.5 the same day. **The figure
this pass started from was 121 uncovered of 222.**

### 0.1 Where the 121 went

| namespace | uncovered | now | |
|---|---:|---|---|
| `satellite.system` | 30 | **M19**, less `.delete` `1 22 1` to **M18** and `threshold` `1 22 5`–`1 22 6` to **M10** | 27 / 1 / 2 |
| `satellite.random` | 16 | **M16** | 13 numbers on 16 rows |
| `satellite.library.*` | 11 | **M19** (the `arguments` subtree and `library.main`), **M14** (`library.system` and its four dials) | 9 / 2 |
| `satellite.network` | 8 | **M23** | |
| `satellite.variable.file` | 8 | **M18** | |
| `satellite.console` | 8 | **M17** | `1 5 2`–`1 5 9` |
| `satellite.directory` | 6 | **M18** | |
| `satellite.variable` leaves | 7 | **M16** (`time`, `date`, `duration`), **M15** (`expression`, `variant`), **M23** (`network`), **M12** (`capsule`) | 3 / 2 / 1 / 1 |
| `satellite.file` | 5 | **M18** | |
| `satellite.time` | 4 | **M16** | |
| `satellite.bool` | 3 | **M9**, which had said outright it was not M9 | |
| `satellite.thread` | 2 (+2) | **M12** — with `start()` and `join()`, four | |
| `satellite.window` | 2 | **M13** | |
| `satellite.analyze` | 1 | **M21** | |
| `satellite.include(spaceship)` | 1 | **M21** | |
| **total** | **121** | | |

**Eleven new milestones, M14 through M24**, and **ten of the 121 needed no milestone
at all** — a clause in M9, M12 or M13 covered them, which is the M3/M4 and M10 fix
for the fourth and fifth time.

### 0.2 The four drafts, corrected and moved

`SCRATCH.md/MILESTONE_DRAFTS.md` held four milestones drafted 2026-08-27 to cover
the largest blocks. **Every one of them failed a lens and none of the corrections
had been applied**, which is why they were there and not in §8. They are now in §8
with the corrections applied, and the drafts file records which:

| draft | now | the correction that mattered most |
|---|---|---|
| the machine, its facts and its ceiling | **M14** + **M19** | it claimed the `system_facts/` readers for a milestone after M10, and its own lens found all three consumed at M6.5, M7 and M8.A. **Split at the seam**: the readers are M14, the language surface is M19 |
| persistence — files and directories | **M18** | its done-when could run only once, because the only removal verb in the language is `satellite.system.delete` `1 22 1` and the draft argued that path depended on *it*. **`1 22 1` moved into the milestone** |
| the console's other half | **M17** | its "does not spin" clause needed `satellite.time.sleep(n)` `1 9 3`, which it filed in "Later" and which was in nothing. **M16 now precedes it and owns `1 9 3`** |
| the clock and the dice | **M16** | it slotted itself behind M9.5 to *"report that the dice cannot use the float"* — a type confusion, since `1.5` is a `satellite.variable.number`. **It has no float dependency and is not behind M9.5** |

### 0.3 The ordering problem, resolved

§0.4 of the old ledger recorded that four `satellite.variable.number` methods cannot
finish at M9 — `power` `1 6 4 10`, `modulus` `1 6 4 12`, `truncate` `1 6 4 13` and
`sqrt` `1 6 4 14`, all of which need rounding — and concluded: *"either those four
move to M9.5 or M9.5 moves ahead of M9."* **The four moved.** Moving four methods is
smaller than reordering two milestones, and M6.5 and M9.5 both now say which owns
what.

---

## 1. What is left, and it is not paths

**PLAN §8.2 is the count and it is 222 of 222.** What this file used to hold and no
longer does is *unscheduled work*. What replaces it, and what a future audit should
read instead of re-running the per-path sweep:

- **Eleven numbers are reserved and unbuilt on purpose**, each listed by the
  milestone that owns it: `1 7 1`, `1 7 2`, `1 7 3`, `1 7 6`, `1 7 9`, `1 7 12`,
  `1 9 2`, `1 6 7`, `1 6 8` (M16, which counts them as nine of its twenty); `1 6 9`
  (M15); `1 22 2` (M19). **Reserved is a decision; uncovered was an accident**, and
  the two must not be counted together.
- **Numbers that do not exist yet and are owed**, all of them WORD_NUMBERS' to mint:
  `.ok()` / `.path()` / `.error()` and a handle `clear` (M18); `.swap.used(unit)`,
  `.this.used(unit)`, `.environment(name)` (M19); call shapes for
  `satellite.analyze` `1 16`, `1 8 2`, `1 8 3` and `1 18 1`–`1 18 3`; a seeded
  `satellite.random` shape, for which `1 7 13` is free (M16 declines it, M20 needs
  it). **When they are assigned they arrive as new rows under nodes §8.2 calls
  finished** — the exact shape of the failure that produced the 122 — so they are
  enumerated in §8.2 as well as here.
- **Ten milestones carry something only the author can clear.** M9.5, M14, M15, M16,
  M18, M19, M20, M22, M23, M24 — six under a heading that says *Blocker*, four inside
  an open list standing in front of a demonstration. A milestone with a blocker in
  its done-when is *scheduled*; it is not *startable*.
- **`parallel_for` is in no numbering, no document and no milestone.** §4.5.1.2's
  decision that the pool starts at startup rests on it. M12 names the gap and M14
  repeats it; **neither builds it**, and this is the one piece of work in this file
  that ended the pass still homeless.

## 2. What this pass did not check

Said plainly so nobody reads §8.2 as more than it is.

- **Only PLAN §8 counts as a milestone**, which is the rule and not a limitation.
  But it means a path DESIGN specifies in full still reads as covered only when §8
  says so — and now that §8 says so for all 222, **the reverse risk arrives**: a
  path §8 names and no document specifies. M23's nine are the large case, and its
  own entry says the first job is writing the DESIGN section that does not exist.
- **Named is not sized.** `satellite.help` was 3 paths and nearly free once M2
  landed; `satellite.network` is 9 paths and a protocol stack. They count the same
  in §8.2's table. **The count is a coverage figure, not an estimate**, and that was
  true of this file's 122 and is true of §8.2's 222.
- **The three aliases are counted as their own rows**, because §2.2 lists them:
  `random.fast.range`, `.normal.range`, `.ultra.range` share numbers with the call
  shapes above them (§2.3). A by-number count is 219, not 222.
- **`src/` was not audited**, because there is little of it: M1 and M2's nine files
  plus `words.def`, and `tests/words_test`.
- **§8's new entries have had HALF a lens of their own, and the half they had
  found six things.** Four were planned — every number against §2.2, ownership and
  double-claims, every citation and v1 line count, internal consistency — and the
  first two ran. **The citation lens and the consistency lens did not**, so nothing
  has checked the quotations, the DESIGN and QUAD section numbers, or the v1 file
  and line claims in M14–M24 against their sources.

  What the two that ran caught, all now fixed: `1 5 5` and `1 6 8` attributed to
  the wrong milestone in M15; M18 asserting that M7's entry said something it did
  not; M19 calling itself the largest milestone in the same sentence that gives
  M10's larger number; M19 saying *two* of `satellite.system`'s thirty were not
  its when its own next four lines name three; and §8.2 announcing *nineteen*
  reserved numbers over a list of eleven. **Five of the six are counts or
  attributions that read fine and do not survive being added up** — which is
  exactly what the four drafts were wrong about, and the reason this bullet says
  what has not been checked rather than that the work was checked.

**The condition for deleting this file was that every row has a milestone in PLAN
§8. That condition is met.** The reason it is still here is the one M8.5 makes:
*"help walking the trie would advertise all 121 of these to a user as though they
worked"* — so `satellite.help` printing only what `handlers[path_id]` answers to is
the live version of this file, and **the claim in §8.2 is checkable by machine from
M8.5 onward.** Delete this file then, against a run of `satl` rather than against a
reading of §8.

---

*Companions: [PLAN.md](../PLAN.md) §8 is the list this file was the complement of,
and §8.2 is the count that closed it. [WORD_NUMBERS.md](../WORD_NUMBERS.md) §2.2 is
the 222 paths it was counted against.
[MILESTONE_DRAFTS.md](MILESTONE_DRAFTS.md) holds the four drafts and the lens
findings that corrected them. [QUAD.md](../QUAD.md) §4 asked for a milestone that
now exists and is M20.*
