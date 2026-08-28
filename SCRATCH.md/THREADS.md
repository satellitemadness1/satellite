# The thread surface — grounded 2026-08-27, decided by nobody yet

**This file is scratch and is meant to be deleted.** `example/thread_test.satl`
writes `my_thread.start()` and `my_thread.join()`. **Both are numbered as of
2026-08-28** — `1 6 13 1` and `1 6 13 2`, assigned by delegation and recorded in
WORD_NUMBERS.md §2.2 — so the brief below is out of date in exactly one way: where it
says these paths do not exist, they now do. **Nothing else in it is settled.** When it
was written:
`satellite.variable.thread` `1 6 13` has **zero children numbered**, and the whole
thread surface is three rows. This is the brief that came back from grounding it
against DESIGN, the first satellite's source, and QUAD.

**Nothing here is numbered and nothing may be.** The author owns the numbering.
Delete this file when the questions in it have answers and the paths have numbers.

---

# The thread surface after `thread_test.satl`

## 1. What exists

| path | number | children |
|---|---|---|
| `satellite.variable.thread` | `1 6 13` | none — and the row carries no `(0)` |
| `satellite.thread` | `1 23 (0)` | one |
| `satellite.thread.new` | `1 23 1` | — |

That is the whole thread surface: a type nothing can be done to, a namespace, and a
constructor. The fourth number the program touches is `satellite.variable.capsule`
`1 6 16`, the type of the packaged call (DESIGN §8's table, WORD_NUMBERS §2.2).

One notation point, because it changes what "zero children" costs. Every sibling
under `variable` that carries selectors is written with the marker — `1 6 1 (0)`,
`1 6 2 (0)`, `1 6 3 (0)`, `1 6 4 (0)` — and DESIGN §7.7 says the `(0)` is what marks
a node that is both a value and a parent. `1 6 13` has no marker, so **the row
currently notates a leaf, and giving the thread type methods edits an existing row
rather than appending under it.**

---

## 2. The decisions only you can make

**Q. What happens to a started thread that main never joins?** Nothing in DESIGN,
PLAN, QUAD or WORD_NUMBERS says — I grepped for it and the tree's three existing
answers are all for satl's *own* threads and disagree with each other (`~Console()`
joins, the memory watchdog detaches, SATC.md §"the run does not wait for the write").
Three options. **Wait at main's return**: costs a pause the program did not write,
and there is no timeout to bound it because durations are deferred (DESIGN §12). But
it is the only answer under which `satl --run`'s output is complete. **Refuse in
plain words**: costs a §9 error code and a per-handle flag, and it arrives *after*
the work has run, which is late. **Exit and drop the work**: costs nothing to build
and is the one answer §1.1 already forbids — it is losing output behind the user's
back, and it makes the output nondeterministic.

§1.1's tie-breaker points at **wait**: *do absolutely everything for the user, and
pay for it in performance rather than in their attention.* Waiting is paid in
performance; refusing is paid in attention. **That is my inference from §1.1, not
the document's word — no section states it**, and the first satellite's unbuilt
`plans/threading_plan.md` reached the same answer independently ("detaching would
trade a visible pause for six nondeterministic crashes"), which is corroboration and
still not a decision.

**Q. What does `.start()` start — an OS thread or a queue slot?** DESIGN §8 gives the
type exactly two words, "handle | reference type", and never says what the handle
refers to. PLAN §4.5.1 makes M12 the third tenant of a pool **created lazily and
sized from `THREAD_COUNT`** (24 here), shared with the console printer and parse-time
interning. **A bounded pool plus an explicit `.start()` means `start` can mean
"enqueue"** — and then a thread that joins another still queued behind it deadlocks,
and a program can start more threads than the pool holds. Options: one OS thread per
`.start()` (costs ~8 MB of address space each, which is exactly what PLAN §4.5.2's
memory watchdog kills the process over, skipping the Console and its queued output);
or pool slots (costs the deadlock above unless `join` can steal work or the pool can
grow). Neither is written down.

**Q. Does a program construct through the module and operate through the type?**
Your program has already answered — `satellite.thread.new(...)` then
`my_thread.start()` — but no document records it. This is the exact ambiguity
WORD_NUMBERS §4 holds open for the file (`satellite.file` `1 8` and
`satellite.variable.file` `1 6 2` both carry a `new`, and "two spellings for one
construction is the kind of thing that gets decided by accident at M10"). Options:
write the rule once for both handle types now (costs one paragraph), or **let the
thread be decided by accident one milestone after the file is.**

---

## 3. The surface a program needs, as paths with no numbers

| path | side | what it does | why that side |
|---|---|---|---|
| `satellite.variable.thread.start` | TYPE | runs the packaged call | DESIGN §6.4's dispatch key is (type node, method name) with the receiver written out as the first argument; and it mutates the handle, so §6.4 requires a receiver that names a storage slot — which is why `satellite.thread.new(f(x)).start()` is already rejected |
| `satellite.variable.thread.join` | TYPE | the point where the work must be finished | same key; the receiver names *which* thread, and nothing but a receiver can |
| `satellite.variable.thread.result` | TYPE | the capsule's returned value | **required only if Q5 below goes to "barrier"** — otherwise it is a second spelling for one act, which is the trap WORD_NUMBERS §4 already names |

That is **two paths, or three**, and all of them are unnumbered.

Everything else I could find a case for is ceremony, and saying why is the point:

- `.finished` / `.running` — ceremony. Polling is a busy wait, and `join` is the
  language's answer to waiting. §1.1 pays in performance, not in the user's
  attention, and a poll loop is attention.
- `.detach()` — **refused, not deferred.** A detached thread is the language agreeing
  in advance to lose the result *and* the error; §1.1's "not doing things behind
  their back" is exactly this verb. v1's one detach is the memory watchdog, which
  returns nothing to anybody and ends in `_exit(2)`.
- `satellite.thread.join_all()` — MODULE, because it names no receiver. Ceremony
  until a program can hold a list of threads, and dead entirely if Q1 answers "wait".
- `.stop()` / `.cancel()` — deferred. Cancellation needs a safe point, and DESIGN
  §10.2's "stops it at its next statement boundary" was written before a second
  statement stream existed. That is plumbing to build, not a path to number.

Nothing more is needed on the MODULE side: `satellite.thread.new` `1 23 1` is the
whole of it, which is what makes the split in §2 worth writing down.

**None of these numbers is mine.** WORD_NUMBERS §1.2 — never renumber, never reuse,
always append — makes the position the decision, and §1.1's rule (walk a real program
from the top) would meet `start` before `join` in `thread_test.satl`. That is what
the rule outputs about *order*, stated as the rule's output; it is not an assignment,
and I have written nothing into any table.

---

## 4. What your shape settles, and what it leaves open

Settled, and it is more than it looks: **`new` packages, `.start()` runs, and
`.join()` waits — so *when* a thread begins and *where* the program waits are the
program's policy, not the runtime's.** That is the second half of §1.1 applied
(do all the plumbing and none of the policy), and a runtime that started at `new()`
and joined at a moment of its own choosing would be taking both decisions invisibly.
It also settles that the handle must be bound to a name, since DESIGN §6.4 rejects a
mutating method on a temporary.

Open, each with what the options cost. **RACE** marks the ones where the answer
changes a program's output run to run; those are urgent, the rest can wait.

1. **A thread that is never started, at exit.** Silently drop it (costs: the language
   discarded work the program asked for — §1.1) or refuse in plain words (costs: a
   §9 code and a state bit per handle). Deterministic either way — not a race.
2. **A started thread that is never joined, at main's return.** RACE — the output
   differs between runs; `HELLO, WORLD!` appears sometimes. Options and costs are in
   §2 above; §1.1 reads as *wait*, and that reading is an inference.
3. **`join` twice.** Refuse the second (costs: a program joining from two places
   must track state itself) or return the same answer again (costs: the handle
   retains the result after the thread is gone — which is a field it needs anyway if
   `join` returns anything). Deterministic. It becomes a RACE only under the one
   option nobody should take: a second `join` that returns something *different*.
4. **`start` twice.** Refuse (costs: a code and a state bit) or run again — and
   running again is a RACE by construction, because one handle then names two runs
   and `join` cannot say which one it waited for. **Refusing is the only answer that
   leaves `join` meaning anything.**
5. **Does `join` return the capsule's value?** *Barrier* (yields `satellite`, i.e.
   success): costs the language computing a value and throwing it away, because
   `satellite.returns(TYPE)` `1 21` is decided (DESIGN §13) and the grammar packages
   such a capsule into `thread.new` today — and it forces the third path in §3.
   *Accessor*: costs a field that does not exist — `satellite.variable.capsule`'s
   representation is `(capsule number, argument values)` (DESIGN §8), which records
   what goes in and has nowhere for what comes out. Your own program is ambiguous
   here: `capsule_test` declares no returns clause so it defaults to `satellite`,
   while its body writes `satellite.return()` `1 15 0` — *nothing there*. DESIGN §6.4
   already names that pair as two non-dispatchable states needing different messages.
   Same question, same place: **an error inside a spawned capsule has only one place
   to surface**, since §9.1 rules out throwing and DESIGN §7.2's frame is
   single-threaded on purpose.

---

## 5. What this does to M12

M12 reads in full: *"M12 — threads. `satellite.variable.thread`. The arena makes the
walk atomic-free; the Console already keeps output lines atomic."* Four things follow.

- **It names the type and not the constructor.** `satellite.thread` `1 23 (0)` and
  `satellite.thread.new` `1 23 1` — the only way in the language to make a thread —
  are reached by no milestone at all (SCRATCH.md/MILESTONE.md scores `satellite.thread`
  2-of-2 uncovered, "M12 names only `satellite.variable.thread`"). **The milestone
  builds a type with nothing you can do to it.**
- **It has no "Done when",** against PLAN §8's own opening rule that each milestone
  is a thing that works and can be demonstrated. M6.5, M9.5, M11.A and M11.B all
  carry one.
- **The obvious Done when is `example/thread_test.satl`, and it cannot be, as
  written.** Its signature declares `satellite.container.list<satellite.variable.string>
  arguments`, which pulls in the whole `arguments` subtree — MILESTONE.md calls that
  the largest single find of its audit, and no milestone builds it. *(Updated
  2026-08-28.)* This bullet used to add that the same parameter had been "removed from
  hello world on 2026-08-27 because it made the program unreachable at the milestone
  that owns it." **That removal was itself reversed on 2026-08-28** — it misread
  WORD_NUMBERS §1.3's `(0)` — so hello world declares the parameter again and the
  precedent this bullet leaned on is gone. The narrower point survives and still holds
  here: **`thread_test.satl` never reads `arguments`**, so the parameter is cost with
  no purchase. The difference from hello world is what the two programs need to make
  the parameter real — hello world can be handed an empty list, and this one cannot,
  because M12's own acceptance depends on a subtree nobody builds.
- **`satellite.variable.capsule` `1 6 16` should not be inside M12.** No milestone
  owns it (MILESTONE.md: "M12 at the latest, and probably earlier"), and QUAD needs
  it — `rack.hpp:22`'s stored `std::function`, eleven post sites — while **QUAD spawns
  zero threads** (QUAD.md §2, verified against the source). Left where it is, the
  acceptance program waits on a milestone it has no use for.

One operational note, since it bears on the done-when: ~~`example/` is untracked —
`git status --porcelain example/` returns `?? example/`~~ — **no longer true as of
commit `6209c83`.** The four programs are tracked, so the specification program for
all of the above, and `hello_world.satl` which PLAN M8.B names as its done-when, now
survive a `git clean -fd`. **The done-when problem this note framed is unchanged**:
`thread_test.satl` being in the repository does not make M12 able to run it.


---

## What the two adversarial lenses said

**PROBLEMS.** The cardinal-sin check passes: the brief assigns no number to any new path. Its §3 table has an empty number column, it says outright \"all of them are unnumbered\" and \"I have written nothing into any table\", and its one order claim (§1.1's walk meets `start` before `join`) is correctly labelled as the rule's output about order rather than as an assignment. Every existing number it cites for a bare path checks out against §2.2: `1 6 13`, `1 23 (0)`, `1 23 1`, `1 6 16`, `1 15 0`, `1 8`, `1 6 2`, `1 4 2 21`-adjacent claims, plus DESIGN §8's two-word type row, §6.4's dispatch key and temporary-receiver rejection, §7.7's marker definition, §10.2, §12, PLAN §4.5.1's three pool tenants, M12's text verbatim, the four milestones carrying \"Done when\", MILESTONE.md's three quotes, QUAD's zero threads, v1's one join and one detach, and `example/` being untracked.\n\nTwelve problems, in order of what they cost. The worst is that §1's bolded \"notation point\" is unsound: it lists `1 6 3` among rows that carry selectors when `satellite.variable.time` has none, and §2.2 contradicts the inference in both directions — `1 1`, `1 15` and `1 19` are parents with no `(0)`, while six rows carry `(0)` with no children, and DESIGN defines the marker two different ways in §3 and §7.7. The conclusion it draws from that — that adding methods \"edits an existing row rather than appending under it\" — inverts §1.2, since appending under a node with zero children is the cheapest case the rule has. One number sits next to a shape §2.2 does not carry: `satellite.returns(TYPE)` `1 21`, against §1.3's rule that arguments are part of the number. Two completeness claims are wrong — the surface is not three rows (`satellite.library.main.arguments.machine.threads` `1 14 1 1 1 3` is the same number PLAN §4.5.1 sizes the pool from), and the module side is not settled (§4 holds `thread.new`'s two shapes open by name, over exactly this program's call). §3 labels selectors as \"paths\", the one confusion §1.5 exists to prevent. The rest are citation and cost errors: Q3's \"one paragraph\" hides a ruling on the already-numbered `1 6 2 1`; `join` and `result` reuse existing spellings unremarked; \"the fourth number the program touches\" is not fourth and is not in the program; bare \"§1.1\" means two different documents; the 8 MB-versus-watchdog claim measures the wrong thing; and §6.4's two non-dispatchable states are about receivers, not return values.

1. **Claim:** "Every sibling under `variable` that carries selectors is written with the marker — `1 6 1 (0)`, `1 6 2 (0)`, `1 6 3 (0)`, `1 6 4 (0)` ... `1 6 13` has no marker, so **the row currently notates a leaf**."
   **Wrong because:** Two things are wrong. (a) `satellite.variable.time` `1 6 3 (0)` carries zero selectors in §2.2 — it is listed as a row that "carries selectors" and it has no children at all. (b) The inference itself fails against §2.2 in both directions: three rows are parents with NO marker — `satellite.include` `1 1` (children `1 1 0`/`1 1 1`/`1 1 2`), `satellite.return` `1 15` (children `1 15 0`/`1 15 1`/`1 15 2`), `satellite.help` `1 19` (children `1 19 0`/`1 19 1`) — and six rows carry the marker with NO children: `1 2`, `1 3`, `1 6 3`, `1 10`, `1 11`, `1 12`. Absence of `(0)` on `1 6 13` therefore notates nothing; it is uniform across the whole batch `1 6 5`–`1 6 16` assigned on 2026-08-27, none of which carries it, including `network`, `variant`, `window` and `capsule`, which will all need children. DESIGN also defines the marker twice and differently — §7.7 as "a node that is both a value and a parent", §3 as "§1.3's `(0)` means zero arguments, nothing there" — and the brief cites only §7.7 without noting the conflict.
   **Fix:** Delete the "notation point" paragraph, or rewrite it as a finding about the notation rather than about thread: `(0)` is applied inconsistently in §2.2 (six childless rows carry it, three parents lack it) and is defined two ways in DESIGN §3 and §7.7. Nothing about `1 6 13`'s row can be read off it. If the paragraph survives at all, drop `1 6 3` from the list of siblings that carry selectors — it carries none.

2. **Claim:** "giving the thread type methods **edits an existing row rather than appending under it**."
   **Wrong because:** This inverts §1.2. Adding `start`/`join` under `satellite.variable.thread` is a pure append — they become the first children of a node with zero children, and no existing number moves, is reused, or is renumbered. The only "edit" is possibly adding a `(0)` marker to the parent row, which carries no number and changes no program's meaning. The sentence frames the ordinary, cheapest case under §1.2 as a violation of it, and it is bolded, so it is the sentence most likely to be acted on.
   **Fix:** State the opposite: `1 6 13` has zero children, so methods on the thread type are the cleanest append §1.2 allows — next free number under the parent, nothing existing touched. Anything about the parent row's marker is a notation question, not a numbering one.

3. **Claim:** "`satellite.returns(TYPE)` `1 21` is decided (DESIGN §13)".
   **Wrong because:** §2.2's row is `satellite.returns` `1 21` — the bare path, with no call shape. §1.3 is explicit that "a number does not identify a path. It identifies a **call shape**", and that a language-owned argument (a TYPE is `satellite.variable.*`, language-owned) *extends* the path, the way `include(satellite)` is `1 1 1` and not `1 1`. So `satellite.returns(TYPE)` is a longer sequence than `1 21`, whatever it turns out to be, and §2.2 does not carry it. PLAN §8's M4 keeps the two separate for exactly this reason — it writes "`returns` at `1 21`" and cites DESIGN §13's syntax in a different sentence. This is the one place in the brief where a number sits next to a shape the authority does not number; transcribed into a table it would be a wrong row.
   **Fix:** Write `satellite.returns` `1 21` when citing the number and `satellite.returns(TYPE)` with no number when citing the syntax, exactly as PLAN M4 does. Do not name what the shape's number would be.

4. **Claim:** "That is the whole thread surface: a type nothing can be done to, a namespace, and a constructor."
   **Wrong because:** §2.2 numbers a fourth thread-related path the brief never mentions: `satellite.library.main.arguments.machine.threads` `1 14 1 1 1 3`. DESIGN §7.7 ("Three surfaces, one set of facts") says it is the *same number* as `THREAD_COUNT`, which is the number PLAN §4.5.1 sizes the lazy pool from — the pool the brief's own Q2 spends a paragraph on. A program deciding how many threads to start, or asked not to start more than the pool holds, reads that path, and it is already numbered. Declaring the surface complete at three rows drops the one thread fact the language already hands programs.
   **Fix:** Add `satellite.library.main.arguments.machine.threads` `1 14 1 1 1 3` to §1's table as an existing, already-numbered part of the thread surface, and cite it in Q2 — whichever way `.start()` is answered, the program can already read the machine's thread count, and DESIGN §7.7 forbids that number and the pool's size from disagreeing.

5. **Claim:** "Nothing more is needed on the MODULE side: `satellite.thread.new` `1 23 1` is the whole of it."
   **Wrong because:** WORD_NUMBERS §4 "Still to be decided" holds this open under its own heading: "**`satellite.thread.new` has one number for two shapes.** `thread.new(f())` and `thread.new(f(x))` both hand `new` exactly one thing... `1 23 2` is free if this is decided the other way." The specification program is `satellite.thread.new(capsule_test(arg_test_str))` — precisely the `f(x)` shape §4 names — so the brief is written about the program that puts the open question under pressure and reports the module side as settled without mentioning it. It also matters for the brief's own §3: it parks `satellite.thread.join_all()` on the module side, and §4 has already flagged the next slot under `satellite.thread` as contested.
   **Fix:** Say the module side is complete **only if** §4's open item resolves toward one number for both shapes, quote §4's reasoning (splitting on the capsule's arity does not stop at two), and note that `thread_test.satl` is the first real program to exercise the `f(x)` shape — which is the evidence that item was waiting for. Do not state which way it should go, and do not repeat `1 23 2` as if it were allocated.

6. **Claim:** §3's heading and table present `satellite.variable.thread.start` / `.join` / `.result` as "the surface a program needs, as **paths** with no numbers", in a table whose first column is headed `path`.
   **Wrong because:** §1.5 draws exactly this line and says confusing it "is how a `.satc` ends up naming a handler (SATC.md §7 forbids exactly that)". Its table lists `satellite.thread.new` as a path and `sort`, `append`, `get`, `substring` as selectors; the brief's own `side: TYPE` column and its §6.4 reasoning are right, but the words "path" and the `path` column header put all three on the wrong side of §1.5. The distinction is not cosmetic: a path becomes a number in the `.satc`, a selector stays bare, so mislabelling changes whether the thing is written into a file at all. DESIGN §2 sharpens it — `satellite.variable.*` is a type namespace and a path in it is never a value expression, so `satellite.variable.thread.start` is a spelling no program may ever write; the program writes `my_thread.start()`.
   **Fix:** Retitle §3 "the selectors a program needs" and head the column `selector, under`, keeping the dotted spelling as §2.2's house style for selector rows. Add one line: these are numbered under `1 6 13` for dispatch only and stay bare in a `.satc`, while `satellite.thread.new` `1 23 1` is the one path here that becomes a number.

7. **Claim:** Q3 offers "write the rule once for both handle types now (costs one paragraph)".
   **Wrong because:** It is not one paragraph, because the file's two spellings are not symmetric with the thread's. `satellite.variable.file.new` `1 6 2 1` is already numbered and is the *first* child of `1 6 2`, with `1 6 2 2`–`1 6 2 7` after it; `satellite.file.new(path)` `1 8 1` is the module form. `satellite.variable.thread` has no `new` at all. So "construct through the module" written once for both leaves `1 6 2 1` as a numbered row nothing should write — and §1.2 forbids reusing it, while PLAN §8's M2 static_assert requires "every parent's children are dense from 1 with no holes and no duplicates", which is the check that would have to absorb the removal. The cost of the paragraph is a decision about an existing numbered child, not a paragraph.
   **Fix:** State the asymmetry in Q3: whichever way the rule goes, the thread costs nothing (no children yet) and the file costs a ruling on `satellite.variable.file.new` `1 6 2 1`, which §1.2 will not let anyone reuse and M2's density assert will not let anyone quietly drop. That is the real reason §4 says it "gets decided by accident at M10".

8. **Claim:** The proposed `.join` and `.result` are presented with no check against existing spellings.
   **Wrong because:** Both words are already language-owned elsewhere in §2.2: `satellite.container.list.join(separator)` `1 4 2 21` (concatenate a list with a separator — the opposite of waiting) and `satellite.container.result` `1 4 4` ("Satellite Orbit's answer"). Neither is a numbering collision — §1's lists are per-parent — but §2.3 and DESIGN §4.4 make the interner responsible for many-nodes-one-string, so each reuse is a row the spelling table has to hold, and `join` meaning "concatenate" on one type and "wait" on another cuts against DESIGN §1.1's "the path segments are the documentation". A brief that adjudicates `.detach()` and `.finished` on naming grounds should say it looked.
   **Fix:** Add a line to §3 noting that `join` and `result` already exist as language words at `1 4 2 21` and `1 4 4`, that per-parent numbering means no collision, and that §2.3/DESIGN §4.4 already accommodate the shared spelling — then say whether the meaning clash on `join` is acceptable. Leave the naming decision to the author.

9. **Claim:** "The fourth number the program touches is `satellite.variable.capsule` `1 6 16`."
   **Wrong because:** `1 6 16` is nowhere in `thread_test.satl`'s text — it is the inferred type of `thread.new`'s argument — and it is not fourth by any walk of the program. Walking from the top, as §1.1 requires, the program meets `satellite.include(satellite)` `1 1 1`, `satellite.capsule` `1 2`, `satellite.main` `1 3`, `satellite.container.list` `1 4 2`, `satellite.variable.string` `1 6 1`, `satellite.variable.thread` `1 6 13`, `satellite.thread.new` `1 23 1`, `satellite.console.display` `1 5 1`, `satellite.return(satellite)` `1 15 1` and `satellite.return()` `1 15 0`. In a document whose §1.1 makes order-of-first-appearance *be* the numbering, an ordinal attached to a walk of a program is the one phrase that must not be loose.
   **Fix:** Write "a fourth number is involved but never spelled in the program: `satellite.variable.capsule` `1 6 16`, the type of the packaged call." Drop the ordinal, or give the actual walk.

10. **Claim:** Bare "§1.1" is used throughout §2, §3 and §4 for DESIGN §1.1's tie-breaker ("do absolutely everything for the user"), while §4's "§1.1's rule (walk a real program from the top)" means WORD_NUMBERS §1.1.
   **Wrong because:** The two documents' §1.1 are different rules — DESIGN §1.1 is "What the rule is for" (the tie-breaker, "not doing things behind their back"); WORD_NUMBERS §1.1 is "The order is the order words first appear". The brief cites both as bare "§1.1" in the same document, and elsewhere it is careful to write "WORD_NUMBERS §1.2", "WORD_NUMBERS §4", "DESIGN §6.4" — so the bare form reads as the numbering file. Six of the seven uses are DESIGN's; one is WORD_NUMBERS'. In a brief whose subject is WORD_NUMBERS, a reader lands on the wrong rule.
   **Fix:** Prefix every one: DESIGN §1.1 for the tie-breaker and the behind-their-back rule; WORD_NUMBERS §1.1 for the order-of-first-appearance walk.

11. **Claim:** "one OS thread per `.start()` (costs ~8 MB of address space each, which is exactly what PLAN §4.5.2's memory watchdog kills the process over, skipping the Console and its queued output)".
   **Wrong because:** Neither watchdog measures address space. v1's `start_memory_watchdog()` compares the *machine's* `mem_available_mb()` against `min_free_mb`, and `process_memory_bytes()` reads `/proc/self/statm`'s second field — resident pages, as its own comment says ("the second field is the resident page count") — which PLAN §4.5.2 proposes to keep and repoint at MEMORY_MAX. A default 8 MB stack is reserved virtual address space, committed lazily; an unused one contributes a few kilobytes of RSS and nothing to MemAvailable. So the cost quoted is real but is not the thing either watchdog fires on. The second half of the sentence is right and worth keeping: `_exit(2)` runs no destructor, so `~Console()`'s `printer_.join()` (`console.cpp:26`) never runs and queued output is lost.
   **Fix:** Say the cost is virtual address space plus whatever each thread actually touches, and that neither watchdog counts the reservation — v1 watches MemAvailable, and PLAN §4.5.2's replacement watches RSS. If the point is that many live threads can reach the threshold, say so through touched memory.

12. **Claim:** "DESIGN §6.4 already names that pair as two non-dispatchable states needing different messages", applied to a capsule that defaults to `satellite.returns(satellite)` while its body writes `satellite.return()`.
   **Wrong because:** §6.4's third qualification is about a *receiver* that cannot be dispatched on: "`nil` has no module, and there are **two** non-dispatchable states needing different messages: an undeclared variable, and a declared variable holding nothing." That is a variable-versus-nil distinction at a method call site. The brief's pair is a declared return type versus an empty return statement inside a spawned capsule — a different pair in a different place. The underlying observation about `capsule_test` is good and worth keeping; the citation does not support it.
   **Fix:** Keep the observation and drop the §6.4 attribution, or cite it as an analogy — "the same shape as §6.4's two non-dispatchable states, one level up" — rather than as something §6.4 names.

**PROBLEMS.** The brief's citations of the four main documents are accurate almost everywhere — M12's text, the three thread rows, DESIGN §1.1/§8/§13, PLAN §4.5.1, QUAD's zero threads, MILESTONE.md's scores, and the untracked example/ are all verbatim-correct, and flagging the join-at-exit recommendation as an inference rather than the document's word is exactly right. But the semantics have four load-bearing errors and three omissions that change the answers. Lens 2 is unanswered: the brief never checks DESIGN §13's appeal, and the appeal fails — v1 implements satellite.file.new as O_CREAT|O_EXCL returning an OPEN handle, so "new" means make-and-activate for files and make-inert for threads, while satellite.time.new has never existed in any implementation. Lens 4 has a bigger answer than the brief gives: the first satellite's threading_plan.md §0(b) found that the deferred call's argument is evaluated at the call site, which makes thread_test.satl print the correct output whether or not the thread ever runs — the specification program cannot fail, which is a stronger reason it cannot be M12's done-when than the arguments parameter. Two arguments rest on the wrong mechanism (§6.4's storage-slot rule does not reach a reference type; the memory watchdog reads machine-wide MemAvailable and resident pages, not address space), and one option is costed against the wrong type (the join result belongs on the handle, not on satellite.variable.capsule). The surface is not too big — the cut list is the best-argued part of the brief — but it is too small in one place the brief itself identifies and then abandons: a spawned capsule's error has nowhere to go, and the language already has the answer-shape in v1's failed-open-is-a-value contract. No number is assigned anywhere below.

1. **Claim:** The brief never checks DESIGN §13's appeal — "the established two-part shape ... exactly as satellite.file.new and satellite.time.new already do" — and so never notices that the language now has two meanings for "new".
   **Wrong because:** The appeal fails on both halves. satellite.file.new is fully implemented in v1 and it is not inert: old_versions/first_satellite/src/evaluator/modules_file.cpp:296 opens with `(chosen->flags & ~O_TRUNC) | O_CREAT | O_EXCL`, methods_file.cpp:252 says "satellite.file.open and satellite.file.new open", and format.def:338 states the contract — "open says give me a handle on what is there, new says make one that is not there and give me a handle on that". So file.new performs the effect and hands back a LIVE handle, while thread.new performs nothing and hands back an inert one. And satellite.time.new has never existed: there is no P_TIME_NEW in format.def (only P_TIME_NOW at 1,24,30) and no implementation anywhere; its sole specification in the project is a nine-word parenthesis in SCRATCH.md/WORD_NUMBERS_ORIGINAL.md:33, "set the arguments for a point in time". Half of §13's evidence is imaginary and the surviving half is a counter-example. The brief also misses the positive precedent that does support the author's shape: satellite.variable.file already carries new (1 6 2 1), open (1 6 2 2) and close (1 6 2 6) on the TYPE side — construct, activate, finish — which is structurally thread's new / .start() / .join().
   **Fix:** Add a section saying the language now has two meanings for one word, with the evidence above, and put the choice to the author: either file.new's effect is what needs re-reading, or "new" is accepted as meaning "make the handle" with the effect varying by type and DESIGN §13's sentence is rewritten to claim only the two-part SHAPE, which is all it can support. Cite satellite.variable.file's new/open/close as the precedent that does hold for an inert-then-activate thread, and strike satellite.time.new from the appeal entirely.

2. **Claim:** §4's list of what is open marks four questions RACE and treats thread_test.satl as a program whose output "differs between runs; HELLO, WORLD! appears sometimes".
   **Wrong because:** There is a worse failure and the brief does not have it. old_versions/first_satellite/plans/threading_plan.md §0(b): "satellite.thread.new(worker(1,2)) runs worker today, at the call site" — eval_call reduces every argument (expr.cpp:353-355) before the module path is flattened, confirmed at runtime by two readers, and the plan's own warning is that "getting this wrong produces a feature that passes any test which only checks the final answer and is a complete no-op concurrently." DESIGN §13's "it parses today with no change to §6" is a claim about PARSING; the packaging is an evaluation-order exception that nothing has specified. Applied to thread_test.satl: if capsule_test(arg_test_str) is reduced at the new() site, the program prints HELLO, WORLD! on the main thread, .start() runs an already-spent package, .join() returns instantly, and the output is byte-identical to a correct run. The specification program cannot distinguish a working M12 from a completely broken one.
   **Fix:** Add this as the first entry in §4, ahead of the four RACE items, and carry it into §5: the reason thread_test.satl cannot be M12's done-when is not only that its signature pulls in later milestones, it is that the program has no failing form. Say what a done-when would need instead — an observable only a genuinely concurrent run can produce — and name the packaging exception (thread.new's argument is not reduced at the call site) as something DESIGN §6 has to state, since §13 currently implies no change is needed.

3. **Claim:** §3's "why that side" column argues .start() and .join() are TYPE-side because "it mutates the handle, so §6.4 requires a receiver that names a storage slot — which is why satellite.thread.new(f(x)).start() is already rejected", and §4 lists as SETTLED that "the handle must be bound to a name, since DESIGN §6.4 rejects a mutating method on a temporary".
   **Wrong because:** §6.4's rule is "A mutating method needs a receiver that names a storage slot. foo().append(x) is rejected — there is nowhere to write back." The operative reason is the write-back, and a thread has somewhere: DESIGN §8 lists satellite.variable.thread as "handle | reference type", so mutating the referent is not mutating the slot. v1 examined this exact question and decided the other way — threading_plan.md:230: "is_mutator: leave it alone. start/join are not container mutations. This is a decision, not an omission: ... a thread mutates an OS resource behind a shared_ptr and has nowhere to write back, so call_mutator would report a storage error." So §6.4 does not reject satellite.thread.new(f(x)).start(), and the brief's one item under "Settled" that it derives rather than reads off the program is not settled. The (type node, method name) half of the dispatch argument is correct and survives; only the mutation half fails.
   **Fix:** Drop the storage-slot clause from both places. Keep the dispatch-key argument, which stands on its own. Then move "must the handle be bound to a name?" into §4 as an open question with its real cost: an unbound thread.new(f(x)).start() is legal under §6.4 as written but unjoinable, which under Q1's "wait" answer means an anonymous thread the program can never wait for and never name — an argument for refusing it, but one that has to be made rather than borrowed from §6.4.

4. **Claim:** §2 Q2 costs one OS thread per .start() at "~8 MB of address space each, which is exactly what PLAN §4.5.2's memory watchdog kills the process over, skipping the Console and its queued output".
   **Wrong because:** Wrong on the mechanism, twice. v1's watchdog does not measure the process at all — memory_facts.cpp:215 reads mem_available_mb(), i.e. machine-wide MemAvailable from /proc/meminfo, against min_free_mb. And PLAN §4.5.2's proposed replacement policy bounds satl's own use through process_memory_bytes(), which the same file's comment describes as "/proc/self/statm's second field is the RESIDENT page count". An untouched pthread stack is a virtual reservation: it is not resident, and it does not reduce MemAvailable. Neither watcher sees it. The 8 MB figure does exist in the tree but means something else — threading_plan.md hazard A6, "Leaks ~8 MB of stack per cycle", is about a LEAKED thread whose stack stays live, not about the cost of starting one. (The rest of that sentence is right: _exit(2) does skip ~Console, per memory_facts.cpp:219-230.)
   **Fix:** Replace the cost line with what actually bounds it: threads-per-process limits, vm.max_map_count and RLIMIT_AS, plus the resident cost of stacks a program actually touches — and note that neither the existing watchdog nor the proposed MEMORY_MAX would catch a thread-count blowup, which is itself worth telling the author, since it means "one OS thread per start" has no guard rail in the design at all.

5. **Claim:** §4.5 costs the accessor answer to "does join return the capsule's value?" as "a field that does not exist — satellite.variable.capsule's representation is (capsule number, argument values) (DESIGN §8), which records what goes in and has nowhere for what comes out."
   **Wrong because:** The result would not live on the capsule value. DESIGN §8 lists satellite.variable.capsule and satellite.variable.thread as two separate types, and the thread handle is the one that outlives the run — it is given exactly two words, "handle | reference type", and no representation at all. A type with no specified representation cannot be short a field. v1 had already specified the mechanism: threading_plan.md:300, ".join() — arity(0). Returns the capsule's value", with the worker storing `h.result = r` on the handle under done_lock (:288), and :291 records the one subtlety — join must capture call_capsule's return value, never the worker Evaluator's returned() afterwards, which is the restored top-level value. So the accessor answer costs a field on an undesigned type, which is roughly free; the barrier answer costs throwing a computed value away. The brief's costing inverts which option is cheap.
   **Fix:** Re-cost Q5: accessor costs one field on satellite.variable.thread, a type that has no representation yet; barrier costs discarding a value that satellite.returns(TYPE) makes it legal to declare, and forces a third path. Cite the v1 mechanism and its capture-the-right-value warning, since that is the part that gets built wrong silently.

6. **Claim:** §4.5's closing line names the real problem — "an error inside a spawned capsule has only one place to surface" — and then §3 cuts every candidate path, leaving a surface of two-or-three with no route for a failure.
   **Wrong because:** This is the one place the surface is genuinely too small, and the language already has the answer-shape. v1's contract for a handle-based failure is that the failure is a VALUE, not an error: modules_directory.cpp:74, "Like a failed satellite.file.open, this is a VALUE and not an error", and SCRATCH.md/MILESTONE_DRAFTS.md:375 states it for files — "the handle comes back holding errno and the caller asks .ok()". threading_plan.md:301 proposes exactly this for threads: "Consider .ok() and .error() in FileHandle's shape — a joined thread that failed should be inspectable without the join itself being fatal." The brief's cut of status methods rules these out by accident, because its stated reason is "polling is a busy wait" — and .ok()/.error() AFTER a join are not polling, they are reads of a settled value. The gap is also sharper than the brief says: §9.1's replacement for throwing is "a sticky MACHINE flag checked at statement boundaries", which is one flag against two statement streams.
   **Fix:** Split the cut list in two. Keep the refusal of .finished/.running as pre-join polling. Add satellite.variable.thread.ok and satellite.variable.thread.error as UNNUMBERED candidates on the TYPE side, argued from the failed-open-is-a-value precedent, and say plainly that without them a spawned capsule that fails has no route to the program at all — §9.1 rules out throwing and §7.2's frame is single-threaded on purpose. Also note that the sticky machine flag is per-machine and there are now two, which is a decision M12 forces whether or not these paths exist.

7. **Claim:** §2 recommends "wait" at main's return, and §3 refuses .detach() outright — "refused, not deferred". The two are argued separately and never against each other.
   **Wrong because:** They interlock, and together they close the language. If exit waits and detach is refused, there is no way to express a background worker at all, and DESIGN §13 makes for(;;) the infinite loop — so a capsule containing one, started on a thread, turns satellite.return(satellite) into a hang with no bound (durations are deferred, §12, so there is no timeout) and no diagnostic. DESIGN §10.2 defines Ctrl-C as stopping the program "at its next statement boundary", and a main parked inside a join is not at a statement boundary — the brief has this fact but files it under the .stop()/.cancel() deferral in §3, where it is decorative, instead of under Q1, where it is decisive. The §1.1 argument also needs qualifying: "pay for it in performance rather than in their attention" is a tie-breaker about finite costs, and an unbounded wait is a liveness change, not a performance cost. And the tree's one precedent for waiting at exit, ~Console()'s printer_.join() at console.cpp:26, joins a thread THE LANGUAGE WROTE and can prove terminates; joining a user's capsule is not the same guarantee. Separately, the brief undersells its own corroboration: threading_plan.md §1.5 is not one quoted sentence but a designed mandatory join barrier, including the ordering rule join-then-drain, because "drain() waits only until the queue is empty; it has no concept of a writer that has not queued its line yet."
   **Fix:** Argue the other side in Q1 as the lens asks: state the for(;;) case, the absent timeout, the Ctrl-C contract that does not reach a joining main, and the ~Console counter-reading. Then either recommend wait AND keep one escape hatch (which means .detach() moves from "refused" to "the price of waiting"), or recommend the pair and say outright that a fire-and-forget worker is refused by construction — but do not recommend both halves as if they were independent. Bring threading_plan.md §1.5's join-before-drain ordering forward, since it is the mechanism the "wait" recommendation actually needs.

8. **Claim:** §5: thread_test.satl's arguments parameter "pulls in M9, M10, and the whole arguments subtree ... the parameter costs three dependencies and buys nothing."
   **Wrong because:** M9 is not a cost of the parameter. The body declares satellite.variable.string arg_test_str = "HELLO, WORLD!" and capsule_test takes a satellite.variable.string, and PLAN M9 is where ".bool, .number, .string and their methods" land. Strip the parameter and M9 is still required. The parameter costs M10 (container.list) and the arguments subtree — two, not three. The argument is still correct and still worth making; the count is wrong, and a wrong count in the paragraph telling the author to change their own program is the kind of thing that gets the paragraph dismissed.
   **Fix:** Say two dependencies, name them (M10 for satellite.container.list, plus DESIGN §7.7's arguments subtree that MILESTONE.md:104 records as owned by no milestone), and note that M9 is needed by the body regardless — which is separately useful, because it means M12 as written depends on M9 no matter what the signature says. Worth adding: DESIGN §3 calls the parameterised main "the shape without a number", so the specification program's entry point is a call shape §2.2 does not carry.

9. **Claim:** §1's notation point: "Every sibling under variable that carries selectors is written with the marker — 1 6 1 (0), 1 6 2 (0), 1 6 3 (0), 1 6 4 (0) ... 1 6 13 has no marker, so the row currently notates a leaf, and giving the thread type methods edits an existing row rather than appending under it."
   **Wrong because:** The absence carries almost no information. Every row from 1 6 5 through 1 6 16 lacks the marker — twelve consecutive rows, all marked "assigned" in the same 2026-08-27 batch — including satellite.variable.bool, whose methods PLAN M9 explicitly promises (".bool, .number, .string and their methods"), satellite.variable.float, which DESIGN §13 decided in full and M9.5 builds, satellite.variable.window, which M13 builds, and satellite.variable.capsule, which DESIGN §8 gives a two-field representation. Thread is not notated as a leaf; it is in a batch that was not annotated. The supporting list is also wrong about one row: satellite.variable.time 1 6 3 (0) carries the marker and has zero children in §2.2, so the marker does not track "carries selectors" in either direction. And the cost framing is inflated: adding (0) changes no number, so §1.2's freeze is untouched — satellite.variable.file 1 6 2 (0) took five new children on 2026-08-27 with no incident, and satellite.thread 1 23 (0) already carries the marker with one child.
   **Fix:** Cut the paragraph to one sentence: the row will need a (0) when it acquires children, which is an annotation and not a renumbering. If the point is kept at all, state it as an observation about the whole 1 6 5–1 6 16 batch rather than about thread, since that is a real and larger finding — twelve type rows whose parenthood is unnotated, several of them owned by milestones.

10. **Claim:** §4.5: "capsule_test declares no returns clause so it defaults to satellite, while its body writes satellite.return() 1 15 0 — nothing there. DESIGN §6.4 already names that pair as two non-dispatchable states needing different messages."
   **Wrong because:** §6.4's qualification 3 is about a different pair entirely: "nil has no module, and there are two non-dispatchable states needing different messages: an undeclared variable, and a declared variable holding nothing." Those are variable-lookup states at a method call site, not a declared-return-type against a returned-nothing. §6.4 does not name the pair the brief attributes to it. The underlying observation is real and worth keeping — DESIGN §13 makes satellite.returns(TYPE) optional and defaulting to the satellite type, so capsule_test promises satellite and returns 1 15 0, which is a contradiction inside the author's own specification program and directly determines what .join() could hand back.
   **Fix:** Keep the observation, drop the citation. State it as its own finding: the specification program's capsule declares a default return of satellite and returns nothing, and Q5 cannot be answered until that is resolved. Cite DESIGN §13's "defaulting to the satellite type" for the default, and nothing for the pair, because nothing says it yet.

11. **Claim:** §5's "Four things follow" about M12 — it names the type not the constructor, it has no Done when, thread_test.satl cannot be the done-when, and satellite.variable.capsule should not be inside it.
   **Wrong because:** All four are correct and verified (M12's two sentences are quoted verbatim; MILESTONE.md:81 does score satellite.thread 2-of-2 uncovered; MILESTONE.md:148 does say "M12 at the latest, and probably earlier"; M6.5/M9.5/M11.A/M11.B do carry the only four "Done when" lines, though M8 carries one in another form — "example/hello_world.satl is the done-when"). But the section that exists to say what M12 owns omits the two largest things it does not. First: no milestone owns the thread pool. MILESTONE.md:191 — "the lazy thread pool, sized from THREAD_COUNT, shared by the console printer, parse-time interning and M12 | PLAN §4.5.1 | no milestone. M12 is the closest and says nothing about a pool." The brief builds its entire Q2 on that pool and then leaves it out of the list. Second: M12's second clause, "the Console already keeps output lines atomic", is asserted as a discharged dependency and is only half true — line-atomicity is the unit of queueing, not writer lifetime. threading_plan.md hazard A9 is a worker writing to a destroyed Console, and console.hpp:44-47's own contract is that "the writer must be finished first". thread_test.satl's entire observable output travels exactly that path from a worker.
   **Fix:** Add both to §5 as a fifth and sixth item. The pool one matters because it decides Q2 and is currently nobody's work. The Console one matters because M12's one-sentence justification is the thing most likely to be read as "that part is handled" — say that line-atomic is not lifetime-safe, and that the join-before-drain ordering is the missing half.
