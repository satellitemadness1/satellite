# Milestone drafts — written, verified, NOT YET IN PLAN §8

**This file is scratch and is meant to be deleted.** Four milestones were drafted
on **2026-08-27** to cover the largest blocks of the 122 paths
[MILESTONE.md](MILESTONE.md) §0 found unscheduled. Each was grounded against the
documents, drafted in §8's voice, and then attacked by two adversarial lenses — one
checking every number against WORD_NUMBERS.md §2.2, one checking whether an existing
milestone already owned the work.

**Every lens reported problems, and none of the corrections has been applied.**
That is why these are here and not in PLAN.md: a draft with a known error in it is
worse in §8 than out of it, because §8 is read as settled. **Delete this file when
all four have been corrected and moved into §8** — moved, not copied.

The drafts are the agents' words. The corrections under each are the lenses'. Where
they disagree the lens read the table and the draft did not, so the lens is right
until someone checks.

---

## The machine, its facts and its ceiling

### The draft

**M10.5 — the machine, its facts and its ceiling.** *(Proposed 2026-08-27.)* The
`satellite.system` namespace, the `arguments` object DESIGN §7.7 specifies, and the
memory watchdog PLAN §4.5.2 describes. **39 numbered paths**, which makes it the
largest single milestone in this list, and 28 of them are the `satellite.system`
that §8's closing paragraph names as reached by nothing.

- **`satellite.system` `1 22 (0)`** and 27 below it: `.delete` `1 22 1`,
  `.environment` `1 22 2`, `.home` `1 22 3`, and all of `.memory` `1 22 4 (0)` —
  `.bit` `1 22 4 1` and `.frequency` `1 22 4 2` off SMBIOS type 17, `.main()`
  `1 22 4 3`, `.swap` `1 22 4 4 (0)` with `1 22 4 4 1`–`1 22 4 4 6`, `.this`
  `1 22 4 5 (0)` with `1 22 4 5 1`–`1 22 4 5 5`, and `.free()` `.total()` `.used()`
  with their `(unit)` forms at `1 22 4 6`–`1 22 4 12`.
- **`satellite.library.main.arguments` `1 14 1 1 (0)`** and its seven — `.machine`
  `1 14 1 1 1 (0)`, `.machine.cores` `1 14 1 1 1 1`, `.machine.cpu` `1 14 1 1 1 2`,
  `.machine.threads` `1 14 1 1 1 3`, `.memory` `1 14 1 1 2 (0)`, `.memory.total`
  `1 14 1 1 2 1`, `.username` `1 14 1 1 3` — under `satellite.library.main`
  `1 14 1 (0)`, which §8.1 discusses at M2 and which nothing builds.
- **`satellite.container.arguments` `1 4 3`**, the type name an error message uses so
  it does not send the reader to the list's method table, and
  **`satellite.library.system.min_free_mb` `1 14 2 3`**, the watchdog's threshold.

**`satellite.system.threshold()` `1 22 5` and `(n)` `1 22 6` are not this
milestone's.** They are spelled under `system` because that is where a knob belongs
beside `max_depth`, but they set how loose a search may be over the ten-level ladder,
and the search power cannot ship without its dial: **they go to M10.** v1 says it in
its own comment — *"it is not a system FACT: uname and getpwuid answer what the
machine is, and this sets how the search behaves."* That is why the count above is 28
and not 30.

**Three of the four `satellite.library.system` dials are owned already, and none of
the three milestones says so.** `max_depth` `1 14 2 2` is M7's, because M7 bounds
recursion and DESIGN §7.5 is its whole specification; `division_digits` `1 14 2 1` is
M6.5's, because `Number` reads it (PLAN §6.1, open question 1); `float_digits`
`1 14 2 4` is M9.5's, and DESIGN §13 says so outright while also *redefining* the
path. Only `min_free_mb` was unowned. This milestone therefore appends one child to a
node M6.5 has already had to build, and it must not touch the other three. **Whether
`division_digits` and `float_digits` are the same dial is still open**, and that
decides whether the node has four children or three.

**M2 owns the name, this milestone owns the behaviour.** All 39 are in `words.def`
and come out of `satl --words` years before any of them answers anything, so a sweep
that reads the dump as coverage will read `1 22 4 3` as done. Saying it in both places
is what stops the next audit making that mistake — as is the fact that
`satellite.library` `1 14` parses at M4 as one of DESIGN §6.1's eleven segment-1
words, so `satellite.library.system.min_free_mb = 8192` **parses three milestones
before it means anything.**

**Most of the port is not in `system_facts/`.** That directory is the fact readers —
`memory_facts.cpp` (236) ports nearly whole and is `1 22 4 1` through `1 22 4 12` plus
the ceiling; `host_facts.cpp` (44) and `stack_facts.cpp` (52) port whole. The
*language* surface is in the evaluator: `modules_system.cpp` (321) is the actual
`satellite.system.*` dispatch, the unit table and every error message, and at 321 it
is over §3's ceiling, so it splits; `helpers_limits.cpp` (121) is the dials;
`methods_containers.cpp:118-199`, `subscripts.cpp`, `helpers.cpp:174-190`,
`value_arguments.hpp` and `value_printer.hpp:114-135` are the arguments object. About
570 of ~800 portable lines sit outside the directory a survey would look in. Two
things do not come across: the flat 33-entry list, which §7.7 redesigns, and
`SAT_PATH`'s four-segment limit, which could not hold `.memory.swap.used()` at all —
WORD_NUMBERS §1.4 removed the limit, so that is a fix rather than a port. Of
`interrupt.cpp` only `run_emergency_exit_hook()` is needed here; **Ctrl-C stays
M11's.**

**It needs no float, and that is worth saying because everything else is waiting on
one.** Every unit is `b`/`kb`/`mb`/`gb`/`tb`, default `mb`, every divisor a power of
1024, and a decimal division by 2ⁿ terminates exactly. M9.5's undecided rounding rule
does not reach this milestone.

**`.delete` `1 22 1` is here for want of anywhere else, and that should be said
rather than hidden.** It is `unlink` or `rmdir` over one path — `lstat` and never
`stat`, `false` is never an error, an open handle is left open — and it sits under
`system` for a different reason than `.home` does: not because it is a machine fact,
but because a caller holding a path very often does not know which of the two things
it holds. It is filesystem work and it groups with `satellite.file` and
`satellite.directory`, which have no milestone. It comes across as its own commit, or
it gets built badly inside a memory milestone.

**M8 has to say what it hands over.** DESIGN §3's hello world declares its parameter
as `arguments` — one of §7.7's six spellings — so M8 gives `satellite.main` something
under that name two milestones before this one can make it real, and §7.7 puts the
recognition at resolve, which is M6. Hello world never reads it, so M8 can hand over a
plain `list<string>` and stay byte-identical; but **M8 must write down that this is
what it hands over**, or a program written in between gets an error for
`arguments.username` that no document predicts. This is the M3/M4 failure caught
before it happens instead of after.

**Why here.** The `arguments` half has a hard floor at M10: the object *is* a
`list<string>` to the type system, `.names()` returns one, and `args[i]` falls through
to the list's own bounds and error text. The `satellite.system` half has no such
floor — it needs M2, M5, M6.5 and M7 and nothing else, and could land before M8. If
this is ever split into two milestones, **that is where the seam is**; a single slot
after M12 would hold the machine facts behind the REPL, threads and windows for no
reason anyone has given.

**Open, and none of these is small:**

- **The 33.** DESIGN §7.7 says each of v1's flat entries needs placing under a parent
  or dropping. v1 took a third option and shipped it: the entry names get **no
  registry ids at all**, the bare selector lowers to `.get(name)`, and *"spending a
  permanent registry id on `kernel_release` would be the registry recording a fact
  about somebody's machine."* §1.2's freeze is forever, so this is 33 permanent
  choices and the largest irreversible decision in the milestone.
- **Two call shapes v1 accepts have no number.** The unit block is reached for every
  swap and `this` form, so v1 answers `.swap.used(unit)` and `.this.used(unit)`; §2.2
  has `1 22 4 4 3` and `1 22 4 5 3` written *without* the parens their siblings carry,
  which is the table recording a sweep's arity rather than the code. v1's own
  acceptance program writes `.this.used("kb")` at `example/full_test.satl:613`. Two
  numbers have to be assigned in WORD_NUMBERS.md, which is the only place that can.
- **`satellite.container.arguments` `1 4 3` has no children and the object answers
  ten selectors** — `.length()`, `.count()`, `.names()`, `.to_string()`, `.lines()`,
  `.has(k)`, `.get(k)`, `.first()`, `.last()`, `.contains(x)`. That is M10 and the
  twenty-nine container methods again, one level down, in a namespace nobody has
  looked at.
- **`MEMORY_MAX`'s unit and default** (§4.5.4). The watchdog cannot be written without
  it: 61.9 GiB and 64.9 GB are the same memory.
- **Does the file seed the namespace?** §4.5.3 proposes it and ends *"Not yet
  decided."* It is the difference between one authority and a reconciliation rule.
- **Is a setting allowed to differ from a fact?** §4.5.4 and DESIGN §7.7 both say
  `THREAD_COUNT` and `arguments.machine.threads` must not disagree, and neither says
  which way it lands. The answer decides whether this builds one reader or two.
- **DESIGN.md never says `satellite.system`.** The grep is empty. 28 of these paths
  are specified only by WORD_NUMBERS §2.2's table and v1's source comments, so
  demonstrating them *is* the specification, which inverts the order everything else
  in this plan follows. v1 recorded the identical gap about its own DESIGN and shipped
  anyway.
- **The seventh spelling and `arguments[0]`.** A parameter named `argv` gets a plain
  list with no properties, silently, and DESIGN §9 says that silence is wrong; this is
  where the language either says so or does not. `arguments[0]` v1 already answered —
  `.length()` and numeric `[i]` cover the command line and nothing else, so index 0 is
  the program name — and the author only has to confirm that answer is kept.
- **The arguments object is a startup cost.** v1 builds all 33 eagerly: `uname`,
  `/etc/os-release`, `getpwuid`, `gethostname`, `getcwd`, two `readlink`s, `sysconf` —
  a dozen syscalls and a file parse against satl's own measured 0.01 ms share (§4.3).
  §7.7's redesign is the chance to make it lazy, and §9 says measure it here.

Done when: one program prints `arguments.memory.total`,
`satellite.system.memory.total("mb")` and `satellite_string`'s live code 98 and
**asserts all three are the same number**, and the same for threads across
`arguments.machine.threads` and code 97 — DESIGN §7.7's *"they must not be allowed to
disagree"* turned into something that can fail; when `argz.machine.threads` answers 24
on this machine, which is the one line that proves §2.3's one-node-six-spellings
rather than five words that happen to be registered; when
`satellite.console.display(arguments)` prints all of it (§7.7); and when **the
watchdog fires** — `min_free_mb` retuned above what the machine has, exit status 2,
the plain-words line on stderr, and **the terminal still usable afterwards**, because
`_exit(2)` runs no destructor and the hook is the only thing between it and a terminal
left in raw mode. A watchdog that never fires is indistinguishable from no watchdog,
so the demonstration is the one that kills the process, and §9 means the *installed*
binary in a real pty rather than a simulation.

`example/full_test.satl:605-613` is the regression floor and ports almost verbatim:
five assertions over `.home`, `.total`, `.free`, `.main` and `.this.used("kb")` — and
that last line is also the proof that the two missing `(unit)` numbers are a real
problem and not a hypothetical one.

### What the lenses found — UNAPPLIED

**PROBLEMS.** Nine number problems, one of them the kind that survives an audit because the arithmetic covers for it.

**The serious one.** The `satellite.system` bullet glosses `1 22 4 6`–`1 22 4 12` as "`.free()` `.total()` `.used()` with their `(unit)` forms" — six things over a seven-wide range. `1 22 4 9` is `satellite.system.memory.main(unit)`, and the bullet never names it. The headline count of 28 is right, but it is right because the range is seven wide, not because the sentence describes 28 paths; the sentence describes 27. That is the failure mode this project keeps hitting, one level down: a number owned and not said.

**Two counts that do not add.** "About 570 of ~800 portable lines" sums only five of the seven files it names outside `system_facts/` (563), and 570 + the draft's own 236+44+52 = 902, not 800. And "the largest single milestone" beats M10 by exactly one path, unstated — M10 is 38 with the two `threshold` numbers this draft moves there, and 39 if `satellite.container` `1 4 (0)` counts.

**What is right, and worth saying because I went looking.** Every one of the 39 paths exists in §2.2 and every number is attached to the right path. The count reconciles: 28 system + 8 arguments subtree + `satellite.library.main` `1 14 1 (0)` + `satellite.container.arguments` `1 4 3` + `min_free_mb` `1 14 2 3` = 39, and SCRATCH.md/MILESTONE.md §0.2 independently sizes the arguments subtree at 8. The 28-not-30 subtraction is correct against §8's closing 30. The two missing `(unit)` numbers are real and are exactly two: v1's `swap_form` and `this_form` both fall through the same unit block accepting 0 or 1 arguments, so `.swap.used(unit)` and `.this.used(unit)` are accepted and unnumbered, and WORD_SURFACE.md:237-243 shows why — it swept `swap.used` as arity 0 and `this.used` as "unknown". The ten arguments selectors are exactly the ten in `methods_containers.cpp:118-199`. `example/full_test.satl:605-613` is five `show()` calls over `.home`, `.total`, `.free`, `.main` and `.this.used("kb")` at 613. 33 `add(args, …)` call sites confirm §7.7's 33. Both v1 quotes are verbatim (`search_apply.cpp:93`, `design/08-d…:214`). Line counts 236/44/52/321/121 all check. 24 threads checks. The DESIGN grep really is empty. No alias is treated as its own path — the six `arg` spellings are handled as one node throughout, and `1 22 5`/`1 22 6` are correctly excluded rather than double-counted.

1. **Claim:** "and `.free()` `.total()` `.used()` with their `(unit)` forms at `1 22 4 6`–`1 22 4 12`."
   **Wrong because:** The range is seven numbers wide and the gloss names six things. WORD_NUMBERS.md §2.2 fills it as free() `1 22 4 6`, total() `1 22 4 7`, used() `1 22 4 8`, **`satellite.system.memory.main(unit)` `1 22 4 9`**, free(unit) `1 22 4 10`, total(unit) `1 22 4 11`, used(unit) `1 22 4 12`. `1 22 4 9` is not a unit form of free/total/used — it is the unit shape of `.main()`, which the bullet introduces eight words earlier at `1 22 4 3` and then never mentions again. This is the one error that hides itself: the bullet's total of 28 is right ONLY because the range is seven wide, so the arithmetic checks out while the sentence describes 27 paths. A reader building what the sentence says builds 27 and reads "28" back at himself. §1.2 is why the two `.main` shapes are six slots apart, and that is exactly the kind of gap this document exists to name.
   **Fix:** Split the range: "`.free()` `.total()` `.used()` at `1 22 4 6`–`1 22 4 8`, `.main`'s unit shape at `1 22 4 9` — six slots from `.main()` `1 22 4 3`, which is §1.2 doing what it promises — and `.free(unit)` `.total(unit)` `.used(unit)` at `1 22 4 10`–`1 22 4 12`." Then the 28 is the sum of what is written rather than of what is left out.

2. **Claim:** "About 570 of ~800 portable lines sit outside the directory a survey would look in."
   **Wrong because:** Neither number reconciles with the file sizes the same paragraph supplies. 570 is the sum of only five of the seven things it names outside `system_facts/`: modules_system.cpp 321 + helpers_limits.cpp 121 + methods_containers.cpp:118-199 (82) + helpers.cpp:174-190 (17) + value_printer.hpp:114-135 (22) = 563. It drops `subscripts.cpp` (277 lines) and `value_arguments.hpp` (56 lines) — both named in the same sentence as the arguments object. Adding value_arguments.hpp whole gives 619; adding subscripts.cpp whole gives 896 (its Arguments block is only ~115-155, ~41 lines, but the draft gives no range). The denominator fails against the draft's own inside-directory figures too: 236 + 44 + 52 = 332, so 570 + 332 = 902, not ~800, under every reading of "nearly whole".
   **Fix:** Give `subscripts.cpp` and `value_arguments.hpp` line ranges the way the other five have them, then state both figures as the sum of the parts rather than as an estimate. The claim the paragraph is actually making survives every reading — 563 to 896 outside against 332 inside — so it costs nothing to make the numbers add.

3. **Claim:** "`satellite_string`'s live code 98 and **asserts all three are the same number**, and the same for threads across `arguments.machine.threads` and code 97"
   **Wrong because:** 97 and 98 are correct against the source — `old_versions/first_satellite/src/satellite_string/satellite_string.hpp:17-19` and `:42-44` give 97 threads, 98 mem_total_mb, 99 mem_used_mb — and PLAN §6.1 agrees verbatim. But DESIGN §7.7, which this same sentence quotes for *"they must not be allowed to disagree"*, reads the other way: "`arguments.machine.threads`, `arguments.machine.cores` and `arguments.memory.total` ... the same numbers again as codes 97, 98 and 99", which positionally makes 98 `cores` and 99 `memory.total`. There is no live code for cores at all — the table goes 97 threads straight to 98 mem_total — so §7.7's "three surfaces, one set of facts" is only two surfaces for `arguments.machine.cores`. The draft silently picks the right number out of a two-document conflict it is citing, and a reader who checks the Done-when against §7.7 will read code 98 as a core count and think the assertion compares memory against cores.
   **Fix:** Keep 98 and say why: DESIGN §7.7's triple is the one that is wrong, because v1's header and PLAN §6.1 both give 97/98/99 as threads/mem_total_mb/mem_used_mb. Add to the Open list that `arguments.machine.cores` has no live code, so §7.7's third surface does not exist for it — which is a finding of exactly the shape this milestone is looking for.

4. **Claim:** "**Whether `division_digits` and `float_digits` are the same dial is still open**, and that decides whether the node has four children or three."
   **Wrong because:** It cannot decide that, and saying it can is the numbering read backwards. §1.2 is "Never renumber. Never reuse. Always append", and it states outright that removing a child "leaves a hole rather than shifting its neighbours down". `float_digits` is already `1 14 2 4` in §2.2. If the two turn out to be one dial the outcome is a §2.3 alias (one node, two spellings, and §2.3 says an alias "does not take a second number", so `1 14 2 4` retires) or a hole at 4 — never a node with three children. `satellite.library.system` keeps four numbered slots forever; what is open is how many of them answer.
   **Fix:** "…and that decides whether the fourth slot answers or becomes §1.2's first hole." Then say which of §2.3's two shapes it would take — an alias and a hole are different work, and this would be the numbering's first hole either way, which is worth recording.

5. **Claim:** "**39 numbered paths**, which makes it the largest single milestone in this list"
   **Wrong because:** The margin is one path and the comparison is never shown. M10 as PLAN §8 writes it is `satellite.container.map` `1 4 1 (0)` + its nine + `satellite.container.list` `1 4 2 (0)` + its twenty-five = 36, plus the two threshold numbers `1 22 5` and `1 22 6` this draft hands it = 38. Add `satellite.container` `1 4 (0)` — which SCRATCH.md/MILESTONE.md §0.1 counts inside the container namespace's 39 and does not list among its 7 uncovered — and M10 is 39 and the two tie. The superlative also holds only by excluding M2, which this draft's own "M2 owns the name" paragraph says covers all 39 and which seeds all 218 rows of §2.2.
   **Fix:** Show the arithmetic instead of asserting the rank: "39, against M10's 38 once `threshold` moves there" — and say the comparison is over behaviour, not over `words.def`, since M2 names every one of the 218.

6. **Claim:** "`satellite.library` `1 14` parses at M4 as one of DESIGN §6.1's eleven segment-1 words"
   **Wrong because:** §2.2 writes that row `1 14 (0)`, not `1 14`. The draft writes the `(0)` for every sibling it cites — `satellite.system` `1 22 (0)`, `satellite.library.main` `1 14 1 (0)`, `.arguments` `1 14 1 1 (0)`, `.machine` `1 14 1 1 1 (0)`, `.memory` `1 14 1 1 2 (0)`, `.swap` `1 22 4 4 (0)`, `.this` `1 22 4 5 (0)` — and drops it only here. The two spellings are not interchangeable in that table: §2.2 carries genuinely bare rows where they exist (`satellite.return` `1 15`, `satellite.help` `1 19`, `satellite.returns` `1 21`, `satellite.analyze` `1 16`), and there is no bare `1 14` row. The rest of the draft's claims about `library` are sound — DESIGN §6.1's table does list `library` among the eleven.
   **Fix:** Write `satellite.library` `1 14 (0)`.

7. **Claim:** "`SAT_PATH`'s four-segment limit, which could not hold `.memory.swap.used()` at all — WORD_NUMBERS §1.4 removed the limit"
   **Wrong because:** The v1 fact is right — `src/bytecode_format/format_tables.hpp:26` is `SAT_PATH(ident, s1, s2, s3, s4, arity)`, four segments, and `satellite.system.memory.swap.used` `1 22 4 4 3` is five — but the citation is not. §1.4 is "One `uint32_t` carries the whole path" and argues about integer *width*: "a path of four segments is not a special case that needs a wider one." WORD_NUMBERS.md credits the refusal to the other section in its own words at §4: the arguments subtree is "six numbers deep, and still the clearest argument in the language for **§1.3** refusing a segment limit."
   **Fix:** Cite §1.3 for refusing the segment limit, and §1.4 only for why depth costs nothing at runtime if both points are wanted.

8. **Claim:** "`modules_system.cpp` (321) is the actual `satellite.system.*` dispatch, the unit table and every error message" — read against "28 of them are the `satellite.system`"
   **Wrong because:** 27 of the 28 have a v1 implementation to port; `satellite.system.environment` `1 22 2` does not. `SCRATCH.md/WORD_SURFACE.md:237` sources it to `v1docs` alone — not `registry`, not `evaluator`, not `programs`, which is what every other `satellite.system` row in that table carries — and it appears nowhere in `modules_system.cpp` or any other evaluator file. The porting paragraph accounts for every other number in the milestone and carries this one along as though it were a port, so the only path in the 28 that is new work is the one the paragraph does not name.
   **Fix:** Name it: `1 22 2` is the one `satellite.system` number with no v1 code behind it, so it is new work and needs a decision of its own (whether an environment answer is the whole block, one named variable, or a map) — and that decision belongs in the Open list, not in the port.

9. **Claim:** "`memory_facts.cpp` (236) ports nearly whole and is `1 22 4 1` through `1 22 4 12` plus the ceiling; `host_facts.cpp` (44) and `stack_facts.cpp` (52) port whole."
   **Wrong because:** `1 22 4 5` is `.this`, and `.this` is not memory_facts'. v1 answers the whole `1 22 4 5 (0)` family — available, free, used — through `thread_stack_bytes()` in `stack_facts.cpp`, via the `this_form` branch of `modules_system.cpp`; the sentence credits that file separately in its own next clause. The cited range hands one of memory's twelve children, and its five grandchildren, to the wrong source file.
   **Fix:** "…is `1 22 4 1`–`1 22 4 4` and `1 22 4 6`–`1 22 4 12` plus the ceiling; `1 22 4 5` and its five are `stack_facts.cpp`, which is why that file ports whole rather than as a fragment."

**PROBLEMS.** The draft is unusually careful about the M3/M4 trap in the paths direction — its M2/M8 hand-over notes, its threshold()→M10 carve-out and its dial audit are correct and well-sourced. But it repeats the same failure in the FILES direction, where nobody has looked. It claims stack_facts.cpp and helpers_limits.cpp, both of which an earlier milestone (M7, M6.5) cannot exist without, and the second of which is literally the two dials the draft says in the same paragraph it must not touch. Its done-when leans on satellite_string live codes 97/98/99 that PLAN §6.1 ports with satellite_string — earlier than M10.5 — and on a DESIGN §7.7 sentence that v1's own header contradicts. Its slot argument ("could land before M8") fails §8's own demonstration rule, since every clause of its done-when prints. The two clauses that matter most — the watchdog and the terminal-still-usable check — demonstrate the v1 policy PLAN §4.5.2 explicitly says to replace, against a hook whose only registrar is M11's. Ownership of the 39 paths is broadly sound; ownership of the ~800 lines that implement them is not.

1. **Claim:** "`host_facts.cpp` (44) and `stack_facts.cpp` (52) port whole" — listed as this milestone's port.
   **Wrong because:** stack_facts.cpp is M7's and cannot wait for M10.5. It provides `stack_limit_bytes()`, and DESIGN §7.5 says the recursion ceiling is "derived from `RLIMIT_STACK` rather than fixed" while PLAN M7 says "Recursion depth is bounded here." In v1 the chain is explicit: helpers_limits.cpp:70 `max_max_depth()` calls `stack_limit_bytes()`, and register_file/reg_stack.hpp:41 sizes the stack from `max_max_depth()` at startup. M7 must therefore already have stack_facts.cpp built. This is the M3/M4 failure exactly — a milestone owning work and not saying so — moved from paths to files, which is the one place this project has never audited.
   **Fix:** Give stack_facts.cpp to M7 by name in M7's own bullet ("the ceiling is derived from RLIMIT_STACK, so `system_facts/stack_facts.cpp` ports here"), and have M10.5 say it inherits it and adds only `.this.*`'s consumer of `thread_stack_bytes()`. Then re-check host_facts.cpp the same way: `hardware_threads()` is what §4.5.1's lazy pool falls back to, and that pool's first tenant is M8's printer thread.

2. **Claim:** "`helpers_limits.cpp` (121) is the dials" — counted in this milestone's ~800 portable lines.
   **Wrong because:** This contradicts the draft two paragraphs earlier. helpers_limits.cpp is 121 lines and is *entirely* `max_depth` and `division_digits` — the two dials the draft has just assigned to M7 and M6.5 and said "it must not touch." It holds DEFAULT_MAX_DEPTH, max_max_depth(), read_max_depth() and read_division_digits() and nothing else. Worse, `min_free_mb` — the one dial this milestone actually owns — is not in that file at all: it is read inline inside `start_memory_watchdog()` at memory_facts.cpp:206. So the draft claims 121 lines belonging to two earlier milestones and claims none for the dial it does own.
   **Fix:** Strike helpers_limits.cpp from this milestone's inventory and name it in M6.5/M7 instead. Say plainly that `min_free_mb` needs no new file — it is eight lines inside the watchdog loop — and recompute the "570 of ~800" figure without it.

3. **Claim:** Done-when: "one program prints `arguments.memory.total`, `satellite.system.memory.total("mb")` and `satellite_string`'s live code 98 and asserts all three are the same number."
   **Wrong because:** The live-code half is already ported before this milestone and the draft never says by whom. PLAN §6.1: "`satellite_string` needs `system_facts/`, because its code table is not only characters: codes 95–100 are live values resolved at decode time, and 97, 98 and 99 are `threads`, `mem_total_mb` and `mem_used_mb`." M7 owns `Str`, so whoever ports satellite_string needs memory_facts.cpp and host_facts.cpp at M7 at the latest — and SCRATCH.md/MILESTONE.md §3 records that port as having no milestone at all. The draft's central three-way assertion is a check across a milestone that is not scheduled and a fact reader it claims three milestones too late.
   **Fix:** Add a line saying which milestone makes codes 95–100 live (M7, with `Str`), that it needs `mem_total_mb()` / `mem_used_mb()` / `hardware_threads()` at that point, and that M10.5 inherits the readers rather than introducing them. Or move the fact readers wholesale to M7 and leave M10.5 the language surface — which is the seam the draft is already half-arguing for.

4. **Claim:** "the memory watchdog PLAN §4.5.2 describes" … done-when: "`min_free_mb` retuned above what the machine has, exit status 2."
   **Wrong because:** That is v1's policy, and §4.5.2's decision is to change it. §4.5.2: "v1 watches the *machine's* available memory against a floor (`min_free_mb`, default 4096); MEMORY_MAX bounds *satl's own* use. Both are defensible and they are not the same guarantee — the second is the stronger promise and the easier one to explain, and `process_memory_bytes()` is already the call that implements it. **Keep the thread, keep the exit path, change what it compares**, and ideally keep both checks." `process_memory_bytes()` appears nowhere in the draft, and the done-when fires the watchdog through the check §4.5.2 says to replace. The draft also lists "MEMORY_MAX's unit and default" as open and says "The watchdog cannot be written without it" — then gives a demonstration that never needs it. It cannot be both a blocker and absent from the done-when.
   **Fix:** Own both checks explicitly: `process_memory_bytes()` against MEMORY_MAX, and `mem_available_mb()` against `min_free_mb`. Make the done-when fire the MEMORY_MAX one, since that is the new guarantee and the one nothing has ever demonstrated, and keep the min_free_mb firing as the ported regression.

5. **Claim:** Open: "Does the file seed the namespace? §4.5.3 proposes it and ends 'Not yet decided.'" and "MEMORY_MAX's unit and default (§4.5.4)."
   **Wrong because:** The draft depends on `satellite_config.ini` and leaves it unowned, and the file cannot be this milestone's anyway. MILESTONE.md §3 lists it as its own un-milestoned row, separate from the watchdog row this draft closes; §4.5.4 says "The installer (§5) is the natural author" and the installer has no milestone either. The ordering half is the real problem: §4.5.1's lazy pool is "sized from `THREAD_COUNT`" and its tenants are "the console's printer thread (DESIGN §10.1), parse-time interning, and `satellite.variable.thread` at M12" — the first two are M8 and M2, both *before* M10.5. So the file has to exist before this milestone, and this milestone's ceiling has no source for its number until it does.
   **Fix:** Say which milestone writes and reads `satellite_config.ini` — M2 or M8 by the THREAD_COUNT dependency, or the installer at M11 — and have M10.5 declare it a dependency rather than an open question. §4.5.3's seeding question is then answered by whoever builds the reader, not here.

6. **Claim:** "The `satellite.system` half has no such floor — it needs M2, M5, M6.5 and M7 and nothing else, and could land before M8."
   **Wrong because:** It needs M8, by §8's own rule. Every clause of the draft's own done-when prints: "one program prints…", "`satellite.console.display(arguments)` prints all of it", and the regression floor at example/full_test.satl:605-613 is five `show(...)` lines that end in `satellite.console.display`. `satellite.console.display` is `1 5 1` and M8 builds the console. Before M8 there is nothing to demonstrate with, and §8's rule is "a thing that works and can be demonstrated." The floor reaches M9 as well: those lines use `>`, `.length()` and `.to_string()`.
   **Fix:** Correct the dependency list to M2, M5, M6.5, M7, M8 and M9, and drop "could land before M8." If the seam argument is kept, the honest version is that the `satellite.system` half could land at M9, immediately after control flow — still earlier than M10, and it still makes the point.

7. **Claim:** "Of `interrupt.cpp` only `run_emergency_exit_hook()` is needed here; **Ctrl-C stays M11's**" … done-when: "**the terminal still usable afterwards**, because `_exit(2)` runs no destructor and the hook is the only thing between it and a terminal left in raw mode."
   **Wrong because:** At M10.5 the hook is never registered, so that clause is a test that cannot fail. `run_emergency_exit_hook()` is a no-op until `set_emergency_exit_hook()` is called, and its only caller in the whole of v1 is console_input/raw_mode.cpp:38. v1's own comment at the exit site (memory_facts.cpp:213) says "a terminal **the line editor** put into raw mode" — the line editor being the REPL's, which the draft has just left with M11. Running a program file at M10.5, nothing puts the terminal in raw mode and nothing registers a hook, so the terminal survives whether or not the hook exists. §9's "verify through the real code path" is exactly what this clause fails.
   **Fix:** Either bring raw_mode.cpp's registration here with `run_emergency_exit_hook()` — and say so, and say what M11 inherits — or move the terminal clause to M11 and replace it here with something that can fail: exit status is 2, the plain-words line is on stderr, and the process is gone within a second of the threshold being crossed.

8. **Claim:** The three-surface assertion, cited as "DESIGN §7.7's *'they must not be allowed to disagree'* turned into something that can fail," using code 98 for memory and 97 for threads.
   **Wrong because:** Two permanent documents disagree about code 98 and the draft picks a side without saying so. DESIGN §7.7: "`arguments.machine.threads`, `arguments.machine.cores` and `arguments.memory.total` are … the same numbers again as codes 97, 98 and 99." PLAN §6.1: "97, 98 and 99 are `threads`, `mem_total_mb` and `mem_used_mb`." v1 settles it — satellite_string.hpp:42-44 is `SAT_THREADS = 97, SAT_MEM_TOTAL_MB = 98, SAT_MEM_USED_MB = 99`. There is no live code for cores at all, so §7.7's three-surface rule has only two surfaces for `.machine.cores`. The draft is right against v1 and wrong against the sentence it quotes as its authority, and it never flags the contradiction — in a milestone whose whole point is that these must not disagree.
   **Fix:** Add it to the open list and name it as a DESIGN fix: §7.7's mapping is wrong, `.cores` has no live code, and the milestone must either assign one or restate the rule as two surfaces for cores and three for threads and memory. Then the done-when's "98" is a decision rather than a coincidence.

9. **Claim:** The `arguments` object port — `value_arguments.hpp`, `value_printer.hpp:114-135`, `methods_containers.cpp:118-199`, `subscripts.cpp`, `helpers.cpp:174-190`.
   **Wrong because:** The list is right and it is missing the consequence: the object is a new alternative on M7's `Value`, and neither milestone says so. v1's value_variant.hpp is explicit — "ArgsRef is index 10 and was appended… appending ArgsRef and then ResultRef leaves `sizeof(Value)` at 40 and the static_assert in value_layout.hpp keeps holding" — and "TWO SWITCHES IN THE WHOLE TREE read a raw index — `module_of()` in types.cpp and `help_for()` in help.cpp — and both gained a `case`." DESIGN §8.2 budgets `Value` at 40 bytes, PLAN §6.1 calls `sizeof` "the one number that could make this port not fit," and M7 is where the static_assert lands. So M10.5 amends a type M7 froze, and touches `help_for()`, whose namespace (`satellite.help` `1 19`, `1 19 0`, `1 19 1`) has no milestone at all.
   **Fix:** Say in M7 that the variant is append-only and that `ArgsRef` and `ResultRef` are two known future appends that must stay inside 40 bytes. Say in M10.5 that it appends one alternative, must re-run M7's static_assert, and that `help_for()`'s arm is either taken here or left for whoever gets `satellite.help`.

10. **Claim:** "§7.7 puts the recognition at resolve, which is M6."
   **Wrong because:** DESIGN §7.7 does not say resolve, and the alias table the recognition needs is claimed by M3 in words that deny it. WORD_NUMBERS §2.3 has three alias rows and one is "`arg` `args` `argz` `argument` `arguments` `argumentz` | one node, six spellings" — while PLAN M3 says "`hexadecimal` is the language's **one** alias for `hex` (WORD_NUMBERS §2.3), which the lexer's spelling table has to know." §2.3 holds six more aliases than M3 counts. So the table the draft's headline demonstration (`argz.machine.threads` answering 24) rests on is asserted by the draft to be M6's, half-claimed by M3's sentence, contradicted by M3's own word "one", and owned by nobody. The draft catches M8's unsaid hand-over and misses this one, on the very path it is claiming.
   **Fix:** Fix M3's sentence — §2.3 has three alias rows, not one — then say where the six spellings live: the spelling table (M2/M3) knows they are one node, resolve (M6) is where a parameter name matching one of them becomes the special variable. Add the M6 note the way the draft added the M8 one.

11. **Claim:** "`max_depth` `1 14 2 2` is M7's, because M7 bounds recursion and DESIGN §7.5 is its whole specification."
   **Wrong because:** §7.5 is not its whole specification — `max_depth` has a second consumer and it is the search. In v1 the same dial bounds the search walk: search_walk.cpp:65, search_apply.cpp:66-71 with its own error text "max_depth (N)", methods_containers.cpp:50, subscripts.cpp:64. By the draft's own argument for moving `threshold()` to M10 — "the search power cannot ship without its dial" — the search's other dial is M10's too. M7 builds it and M10 reuses it, which is fine, but with "whole specification" written down neither milestone will ever say so, and that is precisely the set-up for a fourth occurrence of this project's recurring failure.
   **Fix:** Replace "whole specification" with the truth: M7 builds `max_depth` for recursion (DESIGN §7.5) and M10's search walk is its second consumer with its own depth error, so M10 must say it reads the dial M7 built rather than inventing one.

12. **Claim:** "That directory is the fact readers — `memory_facts.cpp` (236)… `host_facts.cpp` (44) and `stack_facts.cpp` (52)… About 570 of ~800 portable lines sit outside the directory a survey would look in."
   **Wrong because:** The inventory omits system_facts/system.cpp — 263 lines, the largest file in the directory, and the one that actually builds the arguments object. `arguments_for()` is at system.cpp:112 (declared system.hpp:141, called from session.cpp:161); arguments_facts.cpp (113) holds only the one-fact-each readers it is built from. The draft accounts for the flat 33-entry list and never for its assembler. system.cpp also carries `library_path()` and the `-DSATELLITE_LIB_DIR`/VERSION_DEFS build coupling — "system.o is the one object the Makefile compiles with -DSATELLITE_LIB_DIR and VERSION_DEFS" — which is `satellite.include`'s territory in "Later", so the file splits across milestones. The headline "570 of ~800 outside" is computed against an inventory missing 263 lines inside.
   **Fix:** Add system.cpp to the inventory, say which half comes across (`arguments_for()`) and which does not (`library_path()`, to whoever gets `satellite.include`), note the Makefile -D coupling as work the build has to reproduce, and recompute the inside/outside split.

13. **Claim:** "**`.delete` `1 22 1` is here for want of anywhere else**… It is `unlink` or `rmdir` over one path."
   **Wrong because:** v1's `.delete` takes two argument shapes, not one, and the second depends on a namespace no milestone builds. modules_system.cpp:239-262 accepts a `satellite.variable.file` handle as well as a string — "An open satellite.variable.file answers with the path it was opened on, so a program that already holds a handle does not have to write `.delete(f.path())`" — and its nil message reads "a satellite.variable.file is nil until `satellite.file.open` gives it one." `satellite.variable.file` is in "Later" and `satellite.file` `1 8` is one of MILESTONE.md §0.1's five uncovered. The draft's "one path" silently drops half the accepted surface and leaves an error message pointing at a feature the language will not have.
   **Fix:** Say the handle arm is deferred with `satellite.variable.file`, say what the nil and wrong-type messages say in the meantime, and say the arm gets added when `satellite.file` gets a milestone — so the next reader does not port it here and then have nothing to open a file with.

14. **Claim:** The 28 are enumerated as `.delete`, `.environment`, `.home`, and all of `.memory`; "Two call shapes v1 accepts have no number."
   **Wrong because:** Three enumeration defects, and one is the same defect the draft is flagging. (a) `.environment` `1 22 2` is the only one of the 28 with no v1 implementation and no design text — WORD_SURFACE.md:237 sources it as "v1docs", and system.cpp:253 says "environment in general is `satellite.system.environment(name)`'s job", i.e. v1 never built it — yet it takes an argument and §2.2 has no `(name)` shape, which is exactly the missing-arity problem the draft raises for `.swap.used(unit)` and `.this.used(unit)` and then does not raise here. (b) `.main(unit)` `1 22 4 9` is inside the draft's `1 22 4 6`–`1 22 4 12` range but never named while its sibling `.main()` is — and it is in the regression floor the draft cites, at full_test.satl:609 `satellite.system.memory.main("kb")`. (c) "the largest single milestone in this list" is at best a tie: M10 as written owns `1 4 (0)`, map + nine, list + twenty-five = 37, and the draft hands it `1 22 5` and `1 22 6`, making 39.
   **Fix:** Add `.environment(name)` to the missing-arity list — three numbers to assign, not two — and say it is a build rather than a port. Name `.main(unit)` `1 22 4 9` in the enumeration. Change "largest single milestone" to "tied with M10 at 39", or drop the superlative.

15. **Claim:** Done-when demonstrates memory total, threads, `display(arguments)` and the watchdog; "§9 means the *installed* binary in a real pty rather than a simulation."
   **Wrong because:** Two of the 28 paths cannot be demonstrated at all as an ordinary user and the done-when never touches them. `.bit` `1 22 4 1` and `.frequency` `1 22 4 2` come from SMBIOS type 17 via `/sys/firmware/dmi/entries/*/raw`, and v1's own comment at memory_facts.cpp:138 on the fopen failure path reads "root-only, which is the usual answer" — so both answer 0 for a normal user. Under §9's "verify through the real code path", a done-when that never asks them lets two paths ship answering 0 with nothing that fails.
   **Fix:** Either add a clause that says what `.bit` and `.frequency` answer when the DMI table is unreadable and demonstrates that answer (0 is a truthful answer, not an error), or say outright that these two are demonstrated under sudo and that the ordinary-user path is the 0 case, so the next reader does not take a 0 for a broken port.

---

## Persistence — files and directories

### The draft

**M10.5 — persistence: files and directories.** *(New 2026-08-27.)* **Nineteen
numbered paths, and no milestone reaches one of them today.** They are
`SCRATCH.md/MILESTONE.md` §0.1's three consecutive rows — `satellite.variable.file`
8 of 8, `satellite.directory` 6 of 6, `satellite.file` 5 of 5 — the largest
contiguous block of that audit's 122, and the two `file` rows are there because
§0.0 corrected the reading that had "Later" covering them: Later names
`satellite.variable.file` and nothing else.

- the module face — `satellite.file` `1 8 (0)`, `.new(path)` `1 8 1`, `.open`
  `1 8 2`, `.clear` `1 8 3`, `.new(path, mode)` `1 8 4`, which is
  WORD_NUMBERS §1.3's own worked example of the variadic split
- the handle — `satellite.variable.file` `1 6 2 (0)`, `new` `1 6 2 1`, `open`
  `1 6 2 2`, `read_line` `1 6 2 3`, `write_line(s)` `1 6 2 4`, `read_all`
  `1 6 2 5`, `close` `1 6 2 6`, `exists` `1 6 2 7`
- the directory — `satellite.directory` `1 18 (0)`, `.change` `1 18 1`,
  `.current` `1 18 2`, `.exists` `1 18 3`, `.list()` `1 18 4`, `.list(d)`
  `1 18 5`

**It writes no new syntax, and a plan for it that contains a parse rule is wrong.**
M4 owns DESIGN §6.1's eleven segment-1 words, `variable` among them, so
`satellite.variable.file f = satellite.file.open(p, "read")` already parses as a
declaration on `path[1] == "variable"`; §6.1's last row parses `satellite.file.*`
and `satellite.directory.*` as modules, needing a `(`; and §6.2's postfix loop
gives `f.read_line()` and `satellite.file.new(p).close()` for nothing. That is the
same shape as M4 owning the eleven words and not saying so, one milestone further
on — so this one says so.

**What it does not own.** M2 registers all nineteen nodes and their numbers, since
its seed is the whole first-satellite word surface; this milestone re-registers
nothing and M2 owns the number rather than the behaviour. M5 owns every message
here, the mode-word suggestion included — that is "did you mean" over the trie
level that failed, not a port of v1's bespoke `file_mode_error()`. M7 owns
`Value`'s handle alternative, because DESIGN §8's table makes a file a **reference
type** and two variables holding one file share one descriptor, and M7 owns the one
dispatch table of DESIGN §6.4 whose qualification-2 receiver-binding tag is the
only reason `1 8 1` and `1 6 2 1` can coexist. M10 owns the list that `1 18 4` and
`1 18 5` return — a `satellite.container.list<satellite.variable.string>`, sorted
by M10's `sort()` `1 4 2 3` and not by a second sort here.

**SATC.md §5's atomic write is a different mechanism**, not an early version of
this one: tmp, `fsync`, rename is M4.5 doing the interpreter's own C++ file I/O,
and a reader who has just landed M4.5 could reasonably think the ground was
already taken. **`satellite.system.delete` `1 22 1` is not this milestone either**,
and it points the other way — v1 argues in twenty lines that `unlink` acts on a
*name*, so one verb covers a file and an empty directory and belongs under neither
— and its body accepts an open handle as well as a string, so `1 22 1` **depends on
this milestone**. Whoever schedules `satellite.system`'s thirty paths needs that.

**After M10 and before M11, and two of the nineteen decide it.** `.list()` and
`.list(d)` return M10's list and there is no faking it: a directory listing that is
not a list is not the thing. The other seventeen need only M9's strings — a path is
a string, a mode word is a string, a line is a string, and a `.sky` record is
`split(separator)` `1 6 1 10` and `to_number` `1 6 1 13`. Before M11 because
QUAD.md §4's missing milestone wants *a piece of QUAD running* and the REPL is on
nobody's path to that, and because M11 pulls in GTK4 + VTE and a fourth binary,
which is the largest single thing left in this list. **Nothing here is a float, so
it is not behind M9.5** and may overtake it if the rounding rule stalls; saying so
is what stops it inheriting a blocker that is not its own. The seam, if persistence
is ever wanted sooner: the thirteen paths under `1 8` and `1 6 2` have no container
dependency and could run at M9. Against — splitting orphans `satellite.directory` a
second time, which is the precise failure MILESTONE.md exists to record.

**The port is a rewrite, and what survives it is the arguments.** About 850 lines
under `old_versions/first_satellite/src/`: `modules_file.cpp` (304),
`methods_file.cpp` (324), `modules_directory.cpp` (221), plus `FileHandle` in
`satellite_value/value_types.hpp` and the six `format.def` rows, which are the
arity evidence WORD_NUMBERS §4 says has to be read out of the v1 evaluator rather
than guessed. Two of the three files are already over §3's 300-line ceiling, which
§3 applies from the first commit; every arm is the `full == "satellite.file.open"`
string compare §7 throws away wholesale; every failure is `fail()`-then-return-
`nullptr`, which DESIGN §9.1 throws away. What ports is the comment culture §6 says
to keep — the `O_RDWR|O_APPEND` paragraph, the rewind paragraph, the `O_EXCL`
paragraph, the `.` and `..` paragraph, the sort-as-`SatString`s paragraph. The
`std::atomic` fd comes across with it, because §8's reference semantics are what
make two handles race; **M12 is when that first gets exercised, not when it gets
written.** `helpers_listing.cpp` and the two `helpers_file_facts` files **do not
port** — 418 lines of columnar `ls`-style rendering for the REPL echo, with no
number in §2.2 and nothing to do with `.list`, which returns plain sorted names. A
reader sweeping v1 for "file" finds that half first.

**Open, and the first one stands between this milestone and its demonstration:**

- **The failed-open contract has no numbers.** v1's whole answer to DESIGN §9 for
  files is that a failed open is a *value* — the handle comes back holding `errno`
  and the caller asks `.ok()`. `.ok()`, `.path()` and `.error()` have **no rows in
  WORD_NUMBERS §2.2**; `SCRATCH.md/WORD_SURFACE.md`'s v1 sweep missed all three. The
  specification exists and the spelling does not, and only the author assigns them.
- **Which spelling constructs a file** (SESSION §5.7 item 3). DESIGN §13's settled
  `satellite.thread.new` entry already calls the two-part shape established *"exactly
  as `satellite.file.new` and `satellite.time.new` already do"*, which reads as:
  `1 8 1` and `1 8 4` are what a program writes, `1 6 2 1` is the dispatch-table row
  §6.4 qualification 2 describes. Confirm that reading — `1 6 2 1`'s entire origin is
  §6.4's example sentence `my_file.new()`, and v1 has no `new` handle method at all.
  `open` is **not** a second collision: `1 8 2` opens a path, `1 6 2 2` reopens a
  handle that was closed or whose open failed, which is §4.4's deduplication doing
  its job.
- **`1 8 2` and `1 8 3` carry no call shape** while every neighbour does, and §1.3
  makes the shape part of the number. v1 gives `open` an arity of exactly 2 —
  `SAT_PATH(P_FILE_OPEN, 1, 25, 31, 0, 2)` — so the row wants to read
  `satellite.file.open(path, mode)`, and only the author may write it into §2.2.
  `clear` has no v1 module form to read at all: v1 has `satellite.file.new`, `.open`
  and `.delete` and no `.clear`, because its clear is a handle method (`ftruncate`
  then `lseek`). Decide whether `1 8 3` is `clear(path)`, the module face of the
  handle method, or a row that should never have left the handle.
- **`read_line` `1 6 2 3` wants the one thing v1 refused.** v1's `.read()` seeks to
  0 on every call, and the recorded reason is that with no `.seek()` in the
  language, *read from wherever the offset happens to be* is a question no satellite
  program can pose. Does `read_line` advance a per-handle cursor, and what does
  `read_all` `1 6 2 5` do to that cursor when both are called on one handle?
- **`write_line(s)` `1 6 2 4` overrules an argument that is written down.** v1's
  `.write(s)` deliberately appends no newline: otherwise a file with no trailing
  newline, or a line assembled from several writes, both become unwritable, and
  §5.4 gave the language a real `\n` so `f.write("x\n")` already says what it means.
  Either `write_line` is a second verb beside a byte-exact write, or that argument
  loses. Say which, because DESIGN §5.5 refuses `<<` for good and this is the only
  surface a port of QUAD's `.sky` writer can use.
- **`exists` `1 6 2 7` sits on the type node**, so §6.4 desugars it to a call whose
  first argument binds a receiver — and *does this file exist* is asked before a
  handle exists. Either it asks about an open handle's own path, which is a narrow
  question, or it wants to be a module path under `1 8` the way
  `satellite.directory.exists` `1 18 3` is.
- **`.list` is the one failure in its module that is an error and not a value**,
  because the empty list already means an empty directory and spending it on *there
  was no directory* makes the two indistinguishable. Still defensible under DESIGN
  §9's reporter — but re-affirm it, rather than inherit it by porting.

**Done when two programs run under `satl --run`.** The first is a round trip over
all nineteen paths printing one `PASS`: create with `1 8 1` and confirm a second
`new` on the same path comes back as a value that is not ok rather than clobbering
the file (DESIGN §1.1 — *never silently truncate*), write N lines, `close` and read
the status, reopen with `1 8 2` and `read_line` until nothing with the lines
identical and in order, `read_all` and assert it equals them joined, **open a path
that does not exist, print the reason, and keep running** — DESIGN §9's ordinary
case, and the one assertion that cannot be faked — ask for a mode word that is not
one of `read` / `write` / `append` / `read_append` and get M5's error naming the
four, then `1 18 2`, `1 18 4` with the new file in it, `1 18 1` into a subdirectory
and a `1 18 1` onto a plain file that returns false without dying, `1 18 3` true
then false, `1 18 5`, and `1 8 3` leaving the file at zero bytes.

The second is the milestone QUAD.md §4 asks for and this list does not have: **a
piece of QUAD, running.** QUAD's corpus reader — `quad_main.cpp:184-197` — written
in satellite by hand against DESIGN.md: `read_line` until nothing, skip empty
lines, a line starting `"# "` opens a document (`starts_with` `1 6 1 6`,
`substring` `1 6 1 5`), count lines and documents, print the two counts, and they
must match the C++ QUAD's on the same file. It needs no float and no `pow`, so it
does not inherit M9.5's blocker. `Sky::save` / `Sky::load` is the fuller version
and belongs after M9.5, because it round-trips 164 doubles at six significant
digits. **`satellite.directory` is in this milestone on the language's own account
and not QUAD's** — QUAD's includes are `<fstream>` and nothing else, no
`<filesystem>` and no `opendir`, and QUAD.md §2's table credits it with the handle
alone.

### What the lenses found — UNAPPLIED

**PROBLEMS.** Every one of the nineteen path/number pairs the draft cites is exact against WORD_NUMBERS.md §2.2, as are all its outside citations: `sort()` `1 4 2 3`, `split(separator)` `1 6 1 10`, `to_number` `1 6 1 13`, `starts_with` `1 6 1 6`, `satellite.system.delete` `1 22 1`, `satellite.directory.exists` `1 18 3`, `satellite.thread.new` `1 23 1`. The block counts are right too: 8+6+5 = nineteen, `1 8` + `1 6 2` = thirteen, the remaining seventeen, `satellite.system`'s thirty (rows 396-425), DESIGN §6.1's eleven segment-1 words, the six file/directory `format.def` rows, 304/324/221 ≈ 850, 274+112+32 = 418, and `SAT_PATH(P_FILE_OPEN, 1, 25, 31, 0, 2)` verbatim. No alias is treated as its own path, and nothing invents a number.

Nine problems, all in the counts and provenance *around* the numbers rather than the numbers themselves. Three are load-bearing:

1. **The MILESTONE.md sentence is wrong three ways in one breath.** The three §0.1 rows are not consecutive (5th, 8th, 10th of seventeen); nineteen is nowhere near "the largest contiguous block" of the 122 (`satellite.system` alone is 30, and the real top-three run is 57); and "Later names `satellite.variable.file` and nothing else" contradicts the eight-entry list §0.0 and PLAN.md:898 both print in full. The §0.0 attribution is also crossed: §0.0 corrected `satellite.file` **and `satellite.time`**, not the two file rows, and `satellite.variable.file` was never in the wrong category.

2. **"v1 has `satellite.file.new`, `.open` and `.delete`" is false and self-contradicting.** v1's `satellite.file` face is `new` and `open` only; delete is `P_SYS_DELETE` under `satellite.system`, and `modules_system.cpp:204` names `satellite.file.delete` only as the surface it *rejects* — the same argument the draft endorses two paragraphs earlier. It would also make the "six format.def rows" seven.

3. **M2's stated reason misses five of the nineteen.** WORD_SURFACE.md §3 has only two paths under `satellite.variable.file`; `1 6 2 3`–`1 6 2 7` came from QUAD §3's settling ("five on the file" among the 71 that took §2.2 from 144 to 215), not from the v1 surface. The conclusion holds, the premise does not.

The rest: the mode-word "did you mean" has no trie level to walk (four string literals, one number `1 8 2`, and §1.5's fold deliberately not applied); the `1 8 2`/`1 8 3` shape gap is unraised for `1 18 1`–`1 18 3`, whose v1 arities are equally readable; and `substring` `1 6 1 5` is cited for a one-argument call §2.2 does not have.

Files: /home/madness/code/cxx/satellite/WORD_NUMBERS.md (§1.3, §2.2, §2.3, §4), /home/madness/code/cxx/satellite/SCRATCH.md/MILESTONE.md (§0.0, §0.1), /home/madness/code/cxx/satellite/SCRATCH.md/WORD_SURFACE.md (§1, §3), /home/madness/code/cxx/satellite/PLAN.md:898, /home/madness/code/cxx/satellite/old_versions/first_satellite/src/bytecode_format/format.def:521-565, /home/madness/code/cxx/satellite/old_versions/first_satellite/src/evaluator/modules_system.cpp:199-216.

1. **Claim:** "They are SCRATCH.md/MILESTONE.md §0.1's three consecutive rows — satellite.variable.file 8 of 8, satellite.directory 6 of 6, satellite.file 5 of 5"
   **Wrong because:** They are not consecutive. In /home/madness/code/cxx/satellite/SCRATCH.md/MILESTONE.md §0.1 the table runs system 30, random 16, library.* 11, network 8, variable.file 8, console 8, container 7, directory 6, variable-leaves 6, file 5, time 4, ... The three cited rows are the 5th, 8th and 10th, separated by `satellite.console` (8 of 10), `satellite.container` (7 of 39) and `satellite.variable` leaves (6 of 12). Their individual counts (8/8, 6/6, 5/5) are all correct against §0.1.
   **Fix:** Write "three of §0.1's rows" and drop "consecutive". If adjacency is wanted for rhetorical effect, the honest statement is that they are the 5th, 8th and 10th of seventeen.

2. **Claim:** "the largest contiguous block of that audit's 122"
   **Wrong because:** False on both halves. (a) The rows are not contiguous, per the finding above. (b) 19 is not the largest anything: §0.1's first row alone, `satellite.system`, is 30 of the 122, and the genuinely-contiguous top three rows are 30+16+11 = 57. Nineteen is a little over a seventh of the 122.
   **Fix:** Delete the superlative. "Nineteen of MILESTONE.md's 122 uncovered paths, in three of §0.1's rows" says everything true here.

3. **Claim:** "Later names `satellite.variable.file` and nothing else."
   **Wrong because:** MILESTONE.md §0.0 quotes the Later list in full and it has eight entries: "`satellite.variable.file`, `.time`, `.date`; `satellite.random.*`; `satellite.variable.variant`; spacesuits; `satellite.include` of other files; Satellite Orbit and the wire format." PLAN.md:898 is identical. §0.0's actual sentence in the bullet the draft is paraphrasing is "Later names `satellite.variable.file` and `satellite.variable.time`" — the draft drops the second half, which is the half that carries §0.0's point (two different nodes, `variable.time` vs `time`).
   **Fix:** "Of these nineteen, Later reaches only `satellite.variable.file`'s eight — and Later is not a milestone."

4. **Claim:** "the two `file` rows are there because §0.0 corrected the reading that had 'Later' covering them"
   **Wrong because:** Misattributed. §0.0's second bullet corrected exactly two rows: `satellite.file` `1 8` (5) and `satellite.time` `1 9` (4) — one file row and one *time* row, and `satellite.time` is not in this milestone. `satellite.variable.file` was never in the wrong category: §0.1 lists it as "Later only, which is not a milestone", which is a different reason for being uncovered (Later is not a milestone) from §0.0's reason (a milestone-adjacent list named a different node). The draft merges two unrelated reasons into one.
   **Fix:** Split them: `satellite.file`'s five are uncovered because §0.0 corrected the `satellite.file` / `satellite.variable.file` node confusion (paired there with `satellite.time`, which this milestone does not take); `satellite.variable.file`'s eight are uncovered because the only thing naming them is "Later", which §8 itself says is not a milestone.

5. **Claim:** "v1 has `satellite.file.new`, `.open` and `.delete` and no `.clear`"
   **Wrong because:** v1 has no `satellite.file.delete`. `old_versions/first_satellite/src/bytecode_format/format.def` carries exactly two `satellite.file` rows — `SAT_PATH(P_FILE_OPEN, 1, 25, 31, 0, 2)` and `SAT_PATH(P_FILE_NEW, 1, 25, 100, 0, SAT_VARIADIC)` — and `modules_file.cpp` dispatches only `full == "satellite.file.open"` (line 204) and `full == "satellite.file.new"` (line 259). Delete is `SAT_PATH(P_SYS_DELETE, 1, 88, 99, 0, 1)`, and `modules_system.cpp:204` names `satellite.file.delete` only as the split surface it rejects — "satellite.file.delete and satellite.directory.delete would each be a true half of the answer" — which is the same twenty-line argument the draft cites approvingly two paragraphs earlier. The draft therefore contradicts itself, and the error also threatens its own "six format.def rows" count: a seventh row would exist if this were true. (The `.clear` half is right: `satellite.file.clear` `1 8 3` is a hand-written v2 row, and v1's clear is the handle method at `methods_file.cpp:213`.)
   **Fix:** "v1 has `satellite.file.new` and `.open` and no `.clear` — its clear is a handle method (`ftruncate` then `lseek`), and its delete is `satellite.system.delete`, which the paragraph above already puts outside this milestone."

6. **Claim:** "M2 registers all nineteen nodes and their numbers, since its seed is the whole first-satellite word surface"
   **Wrong because:** The stated reason does not reach five of the nineteen. SCRATCH.md/WORD_SURFACE.md §3 records `satellite.variable.file` as "2 to append, next free is `1 6 2 1`" and lists only `.new` and `.open`. `read_line` `1 6 2 3`, `write_line(s)` `1 6 2 4`, `read_all` `1 6 2 5`, `close` `1 6 2 6` and `exists` `1 6 2 7` are not in the v1 sweep at all — they came from settling QUAD.md §3 on 2026-08-27, which QUAD §4 counts as "five on the file" among the 71 that took §2.2 from 144 entries to 215. PLAN M2 says the same: "The seed is wide" is followed by "It got wider on 2026-08-27." The conclusion (M2 registers all nineteen) is right; the premise is not.
   **Fix:** Use PLAN M2's own rule instead: `words.def` "is a transcription of WORD_NUMBERS.md and nothing else", so M2 registers all nineteen because all nineteen are in §2.2 — five of them added by QUAD's settling rather than found in v1.

7. **Claim:** "M5 owns every message here, the mode-word suggestion included — that is 'did you mean' over the trie level that failed"
   **Wrong because:** There is no trie level for a mode word to fail at. `read` / `write` / `append` / `read_append` are user-owned string literals in argument position; §2.2 gives them no rows, and `satellite.file.open` is one number, `1 8 2`, for all four. "Did you mean over the trie level that failed" (PLAN M5) walks numbered children of a node — the mechanism that exists for `satellite.consle.display`, not for a bad second argument. Where a literal option *has* been given numbers, §2.2 shows it: `sort_down()` `1 4 2 5` is WORD_NUMBERS §1.5's fold. No such fold rows exist under `1 8`, even though §1.5's own worked example of the fold is `satellite.file.open("filename", "read_append")`.
   **Fix:** Say M5 owns the message and its shape, but that the four candidate words come from the handler's own table (v1's `kFileModes`), not from the trie — and either raise §1.5's fold for `1 8 2` as an open item or state that it is deliberately not folded.

8. **Claim:** "`1 8 2` and `1 8 3` carry no call shape while every neighbour does" — presented as the milestone's one shape gap
   **Wrong because:** True inside `1 8`, but three more of the nineteen have the identical defect and the draft raises none of them: `satellite.directory.change` `1 18 1`, `.current` `1 18 2` and `.exists` `1 18 3` are module paths in this same milestone written with no call shape, while their siblings `.list()` `1 18 4` and `.list(d)` `1 18 5` carry one. And v1 records their arities just as legibly as `open`'s: `SAT_PATH(P_DIR_CHANGE, 1, 32, 34, 0, 1)`, `SAT_PATH(P_DIR_EXISTS, 1, 32, 35, 0, 1)`, `SAT_PATH(P_DIR_CURRENT, 1, 32, 33, 0, 0)`. Eleven of the nineteen rows carry no shape in total. A milestone that raises the arity question for two rows and silently ports the other three is how §1.3's shape-is-the-number rule gets decided by accident.
   **Fix:** Extend the open item: "`1 8 2`, `1 8 3` and `1 18 1`–`1 18 3` carry no call shape. v1 gives arities of 2, —, 1, 0 and 1 respectively; only the author may write them into §2.2."

9. **Claim:** "a line starting `"# "` opens a document (`starts_with` `1 6 1 6`, `substring` `1 6 1 5`)"
   **Wrong because:** §2.2 writes the row as `satellite.variable.string.substring(start, end)` `1 6 1 5` — a two-argument shape — and §1.3 makes the count part of the number. The line being ported is `ln.substr(2)` at quad_main.cpp:191, which is start-only; §2.2 has no one-argument `substring`. (`starts_with(x)` `1 6 1 6` matches exactly.)
   **Fix:** Write the row as §2.2 does — `substring(start, end)` `1 6 1 5` — and spell the demonstration call with both arguments, or add the missing one-argument shape to the open list so the author, not the port, decides it.

**PROBLEMS.** Not clean — ownership and ordering both have real defects, and the largest is this project's own trap running in reverse. The numbers check out: all nineteen match WORD_NUMBERS §2.2 exactly (`1 8`+4, `1 6 2`+7, `1 18`+5), the v1 evidence is real (304/324/221 = 849 lines; 274+112+32 = 418; `SAT_PATH(P_FILE_OPEN, 1, 25, 31, 0, 2)`; `satellite.system.delete` really does accept a FilePtr at modules_system.cpp:239), and the §1.3/§6.4/§13/§4.4/SATC §5 citations are faithful. No existing milestone owns any of the nineteen — M2, M4, M4.5, M5, M7, M10 and M12 are each disclaimed, and as far as I can find, correctly. The failures: (1) it claims QUAD.md §4's missing milestone with a program QUAD §4 does not ask for, which would close MILESTONE.md §4 and orphan `Sky::decay`+`Rack::draw`; (2) its done-when cannot run twice, because nothing in the nineteen removes a file or makes a directory and the removal verb is one the draft says depends on M10.5 — a circular dependency inside the demonstration; (3) the mode-word error is not M5's, because a string literal is not a trie level and `open` has no folded rows, and the fold itself is unowned; (4) it claims `satellite.variable.file` "8 of 8" while leaving `.ok`/`.path`/`.error` — its own failure contract — owned by nobody; (5) it never strikes `satellite.variable.file` from "Later" or closes MILESTONE.md §1's row, and misreports §0.1 three times in the sentence that would have caught it; (6) it legislates M7's `Value` and dispatch table from inside M10.5 rather than amending M7. The slot itself is defensible — after M10 for `.list`, before M11 for the GTK4+VTE weight — but dependencies (1) through (3) have to be fixed before that slot means anything.

1. **Claim:** "The second is the milestone QUAD.md §4 asks for and this list does not have: a piece of QUAD, running" — satisfied by QUAD's corpus reader from quad_main.cpp:184-197.
   **Wrong because:** QUAD.md §4 does not ask for that. Its words are: "one mechanism out of `mind.hpp`, chosen because it exercises floats, containers, sorting and persistence at once." QUAD.md §5, MILESTONE.md §4 and SESSION.md §5.7 item 6 all independently name the candidate — `Sky::decay` plus `Rack::draw`. The corpus reader is in quad_main.cpp, not mind.hpp; it is that loop with `mind.perceive` and `mind.tick` deleted; and the draft itself boasts it touches no float, no `pow`, no sort and (bar two counts) no container. It fills none of the four things QUAD names. This is the recurring trap running the other way: a milestone claiming a slot it does not fill. If M10.5 lands saying this, MILESTONE.md §4 is struck as done under the ledger's own "moved, not copied" rule, and `Sky::decay`+`Rack::draw` — the one thing three documents agree is the honest next step — is owned by nothing. The draft compounds it by pushing `Sky::save`/`Sky::load` to "after M9.5" while naming no milestone, creating a fresh unowned item in the act of closing the ledger.
   **Fix:** Delete the claim. Keep the corpus reader as an ordinary second demonstration of `read_line` over a real file, and state explicitly that QUAD.md §4's milestone is still unfilled and still wants `Sky::decay`+`Rack::draw` after M9.5 and M10 — the same sentence M10.5 already uses to point forward at `satellite.system.delete`, instead of claiming backward.

2. **Claim:** "Done when two programs run under `satl --run`" — a round trip that creates with `1 8 1` and "confirm[s] a second `new` on the same path comes back as a value that is not ok rather than clobbering the file", plus "`1 18 1` into a subdirectory".
   **Wrong because:** The demonstration runs once. `new` is O_EXCL (v1 format.def, word 100: "`new` says make one that is not there"), so on the second run of the program the FIRST `1 8 1` is the one that comes back not-ok and the round trip fails. Nothing in the nineteen paths can clean up: `1 8 3` clear "empties a file that goes on existing" (format.def word 101), and the only removal verb in the language is `satellite.system.delete` `1 22 1` — which this draft argues at length "**depends on this milestone**". That is a circular dependency inside the demonstration: M10.5's done-when needs a verb the draft says cannot be built until M10.5 exists. The same defect one node over: §2.2 gives `satellite.directory` exactly change / current / exists / list() / list(d) — there is no way to create a directory — so "`1 18 1` into a subdirectory" needs a subdirectory the program can neither make nor remove. A done-when that passes only on a hand-prepared tree and cannot restore it is not "a thing that WORKS and CAN BE DEMONSTRATED"; it is a thing that works once.
   **Fix:** Either (a) state the pre-condition and the manual cleanup as part of the milestone and replace the second-`new` assertion with an idempotent one, or (b) pull `satellite.system.delete` `1 22 1` into this milestone — defensible, since v1's body already accepts a handle and this milestone is the only thing that gives it one — and stop asserting the dependency from outside. Either way, note that `satellite.directory` has no create verb: that is a real hole under a node the draft claims "6 of 6".

3. **Claim:** "M5 owns every message here, the mode-word suggestion included — that is 'did you mean' over the trie level that failed, not a port of v1's bespoke `file_mode_error()`", with the done-when asking for "M5's error naming the four".
   **Wrong because:** There is no trie level. DESIGN §4.6 defines the mechanism as edit distance over one node's *children* — `satellite.consle.display` works because `console` is a child of `satellite`. `"reed"` is a string literal in argument position; `read` / `write` / `append` / `read_append` are children of nothing. §2.2 gives `satellite.file.open` exactly one row, `1 8 2`, with no folded option rows — even though WORD_NUMBERS §1.5's own worked example of the literal-option fold is `satellite.file.open("filename", "read_append")`, and the fold gave `sort_down` a row of its own at `1 4 2 5`. So the modes are either a runtime string test whose message this milestone owns (contradicting "M5 owns every message here" and reinstating exactly what v1's `file_mode_error()` at modules_file.cpp:222 did), or they need numbers that do not exist and only the author may assign. Worse, the fold itself is unowned: MILESTONE.md §1 files it as "M6 or M7 — nothing says whether resolve (M6) or closure compilation (M7) owns it." The done-when leans on machinery no milestone has.
   **Fix:** Say which. If open's modes fold, the folded rows are the author's to assign, and this milestone should say it is blocked on them and that it closes MILESTONE.md §1's literal-option-fold row for this call. If they do not fold, admit the bad-mode message is a runtime check this milestone owns — using M5's reporter rather than M5's suggester — and rewrite the done-when to assert a code and a caret rather than "M5's error naming the four".

4. **Claim:** "the handle — `satellite.variable.file` `1 6 2 (0)` … `exists` `1 6 2 7`", presented as the whole of a node the headline counts as 8 of 8, with `.ok()` / `.path()` / `.error()` filed only as an open numbering question.
   **Wrong because:** This is the trap one level down, spotted by the draft and then left standing. v1's handle surface is ok, path, error, read, write, clear, close, open (methods_file.cpp:61-65, 265, 300; format.def word 101 `my_file.clear()`), and the draft's entire failure contract — its "one assertion that cannot be faked", and the second-`new`-is-not-ok test — runs through `.ok()` and `.error()`. The draft correctly reports they have no rows in §2.2 and that WORD_SURFACE.md missed them, then never says who BUILDS them once numbered. The moment the author assigns `1 6 2 8`+ they read as new uncovered rows under a node PLAN now calls finished — the precise shape of M3/M4's eleven words and M10's twenty-nine methods. v1's handle `clear` has the same problem: the draft debates `1 8 3` as a module row and never says whether the handle keeps a `clear` of its own.
   **Fix:** Say that this milestone owns `.ok`, `.path`, `.error` and any handle `clear` as behaviour and is blocked only on their numbers; change the headline from "Nineteen numbered paths" to nineteen plus the three-or-four the failure contract requires, so the count is what the milestone builds rather than what happens to be numbered today.

5. **Claim:** "They are `SCRATCH.md/MILESTONE.md` §0.1's three consecutive rows … the largest contiguous block of that audit's 122, and the two `file` rows are there because §0.0 corrected the reading that had 'Later' covering them: Later names `satellite.variable.file` and nothing else."
   **Wrong because:** Three misreadings of the ledger it is closing, and one hides a real ownership defect. (a) Not consecutive: §0.1's rows run system 30, random 16, library 11, network 8, variable.file 8, console 8, container 7, directory 6, variable-leaves 6, file 5 — the three claimed are rows 5, 8 and 10 of 17. (b) Not the largest block: `satellite.system` is 30 paths in one row against these 19. (c) §0.0's second bullet corrected `satellite.file` `1 8` and `satellite.time` `1 9` — not the two `file` rows; `satellite.variable.file` is still filed in §0.1 as "Later only", and PLAN §8's Later list does name it, in full: `satellite.variable.file`, `.time`, `.date`; `satellite.random.*`; `satellite.variable.variant`; spacesuits; `satellite.include` of other files; Orbit and the wire format. So the draft never does the one thing the ledger demands: MILESTONE.md's opening rule is "Each row that gets one should be *moved* into §8, not copied — a job in two places is a job that gets done twice or not at all." As drafted, `satellite.variable.file` is named by M10.5 and still named by "Later", and MILESTONE.md §1's row for `1 6 2 3`–`1 6 2 7` is left open.
   **Fix:** Say the milestone strikes `satellite.variable.file` from PLAN §8's "Later" list — leaving it `.time`, `.date`, random, variant, spacesuits, include and Orbit — and that it closes MILESTONE.md §1's five-methods row and §0.1's three rows. Then fix or drop the three factual claims; they buy nothing and they are the first thing a future audit will re-check.

6. **Claim:** "M7 owns `Value`'s handle alternative … and M7 owns the one dispatch table of DESIGN §6.4 whose qualification-2 receiver-binding tag is the only reason `1 8 1` and `1 6 2 1` can coexist."
   **Wrong because:** M7's text says, in full on this point: "`Value` (40 bytes, the static_assert comes too) and `Str`. … Module calls dispatch through `handlers[path_id]`, and the inline caches of §2.4 land here too." It names no handle alternative and no receiver-binding tag. So the draft asserts from inside M10.5 that a milestone three slots earlier owns work that milestone does not say it owns — the failure this project has caught three times, recorded in the one place M7's reader will never look. The fix that worked for M10 was to put the clause in M10, not to describe M10 from elsewhere. WORD_NUMBERS §4 makes the stakes explicit: `1 8 1` against `1 6 2 1` is "the kind of thing that gets decided by accident at M10", and accidental settlement is exactly what happens when the constraint lives only in a later milestone's prose.
   **Fix:** Propose the amendments to M7 in the same breath: M7's `Value` carries the reference-type handle alternative DESIGN §8's table requires (file and thread both), and M7's dispatch table carries §6.4 qualification 2's receiver-binding tag. Then M10.5 can cite M7 instead of legislating for it.

7. **Claim:** "`clear` has no v1 module form to read at all: v1 has `satellite.file.new`, `.open` and `.delete` and no `.clear`."
   **Wrong because:** v1 has no `satellite.file.delete` either — and the draft says so itself two paragraphs earlier, summarising v1's twenty-line argument that `unlink` acts on a name so the verb "belongs under neither" and lives at `satellite.system.delete`. Checked: the only occurrence of the string `satellite.file.delete` anywhere in the v1 tree is inside modules_system.cpp:204's comment arguing it must not exist; `satellite.file`'s registry rows are P_FILE_OPEN and P_FILE_NEW and nothing else. Since this sentence is the whole evidence base for the `1 8 3` question, an inflated v1 module surface points at the wrong answer: the real v1 file module is two verbs, and `clear` was a handle method beside a two-verb module — which strengthens the "a row that should never have left the handle" reading rather than weakening it.
   **Fix:** Correct to: v1's `satellite.file` module is `new` and `open` only; `clear` is a handle method (`ftruncate` then `lseek`), and `delete` is `satellite.system.delete` by v1's own argument. Then re-put the `1 8 3` question against that.

8. **Claim:** "Nothing here is a float, so it is not behind M9.5 and may overtake it if the rounding rule stalls", and "helpers_listing.cpp and the two `helpers_file_facts` files do not port — 418 lines of columnar `ls`-style rendering for the REPL echo, with no number in §2.2 and nothing to do with `.list`".
   **Wrong because:** Two smaller slips, one per lens. (a) The overtaking claim does not hold at the slot chosen: M10.5 sits after M10, and M10 sits after M9.5 in §8's order, so overtaking M9.5 requires M10 to overtake it too — which the draft neither says nor has standing to say. As written the milestone does inherit M9.5's blocker, through M10, which is the exact thing the paragraph exists to prevent. (b) v1's `list_lines` is not only the REPL echo: methods_containers.cpp:88 makes it a list method, `.lines()`, and v1's own comment at session.cpp:88 says the rendering exists because "the commonest list anyone types at this prompt is `satellite.directory.list()`". Ruling it out settles a question belonging to M10 (an unnumbered list method) and M11 (the echo), from a milestone that is elsewhere scrupulous about not touching either.
   **Fix:** For (a): place the seam explicitly — "M10.5 is behind M9.5 only through M10; if the rounding rule stalls, the thirteen paths under `1 8` and `1 6 2` can run at M9" — or drop the overtaking sentence. For (b): say the 418 lines are not this milestone's to port and that `.lines()` is an unnumbered v1 list method M10 or M11 must decide on, rather than deciding it here.

---

## The console's other half — the reader thread and the terminal's facts

### The draft

**M9.25 — the console's other half.** *(New milestone, 2026-08-27.
`SCRATCH.md/MILESTONE.md` §1 proposed it "between M8 and M11", which is four
milestones of latitude.)* **Eight paths — `1 5 2` through `1 5 9`, every child of
`console` except `display` `1 5 1`:**

- `satellite.console.input()` `1 5 2`, `input(prompt)` `1 5 3` and
  `input(prompt, target)` `1 5 4` — ask, and wait, in three shapes. WORD_NUMBERS
  §1.3 is why that is three numbers and not one number with an arity check: a
  user-owned argument cannot extend anything, so the count takes its own slot and
  **the arity is the identity**, settled at M4's parse before an argument is
  evaluated. `1 5 4` writes a place and returns nil — the only out parameter in
  the language, and deliberately not the start of a general facility.
- `satellite.console.typed()` `1 5 5` — a line or nothing, immediately. The one
  genuinely new mechanism here; v1 has no non-blocking input of any kind.
- `satellite.console.width` `1 5 6` and `.height` `1 5 7` — property-shaped in the
  table because they are facts, asked fresh rather than sampled. v1 has no
  `height` at all: `ws_row` appears zero times in it.
- `satellite.console.clear()` `1 5 8` and `.home()` `1 5 9` — call-shaped because
  they are actions, and **both go through the printer's queue**. v1 emits them as
  one escape written straight to the fd (`Renderer::clear_screen`), which is a
  frame shredded between two queued lines.

**The reader thread is DESIGN §10.1's printer with the polarity named.** The
printer's producer is the program and its consumer is a dedicated thread; the
reader is that reversed — the dedicated thread blocks on stdin and pushes whole
lines in, and the program asks and is answered at once. The invariant that
survives both directions, and the reason no satellite program ever learns what a
terminal mode is: **the program's own thread never blocks on the terminal.** §1.1
applied to input. QUAD.md §3.4 is the requirements document — it is what retires
QUAD's `VMIN=0` poll loop, and it bends on echo so that no raw mode is needed.

**Why not M8.5.** `typed()`'s whole claim is that a program keeps running while
nobody has typed, and a program with no loop cannot show that:
`satellite.statement.while` `1 13 3` runs at M9, and the line both `input()` and
`typed()` return is a `satellite.variable.string`, also M9. It goes immediately
after M9 and ahead of M9.5 because it needs nothing from the float, and **M9.5
cannot land until the rounding rule is chosen** — sequencing a milestone with no
blockers of its own behind somebody else's blocker buys nothing. QUAD.md §4
already listed it among the three things "M9 and M10 carry" that were in neither;
the other two have since been placed and this was the last. (Renumbering the float
to M9.75 does the same job; the decimal awkwardness belongs to the `.5` convention
and not to this.)

**M2 owns all eight numbers already and this milestone registers nothing.** They
are paths rooted at `satellite` (WORD_NUMBERS §1.5), so `words.def` carries them,
the header's static_asserts check them and `satl --words` dumps them before any of
them does anything. What lands here is behaviour, dispatched through
`handlers[path_id]`.

**M8 owns the output half of `1 5 3` and does not say so.** M8 reads "Console with
its printer thread", which sounds like output only — but §6's kept-list is "the
Console with its own printer thread, **and the `drain()` barrier before reading
input**", and DESIGN §10.1 justifies the flush by "a prompt written with no
trailing newline". The un-newlined `emit()` and `drain()` are M8's and both exist
*for input*. M8's line should say it; this milestone consumes them and must not
rebuild them. Same shape as M3/M4 and M10, caught before it landed instead of
after.

**This is not a line editor, and M11's prompt is not `satellite.console.input`.**
M11's prompt is a second, unrelated reader: v1's `console_input/`, 1449 lines of
raw mode, key decoding, history and a wrap-aware renderer, whose own header states
that no satellite program can reach anything in it — while `satellite.console.input`
runs through `std::getline` in the evaluator. Two readers, one language; only 14 of
those 1449 lines come here, and they are `terminal_columns()` and the clear.

**M11 also reads as owning all of Ctrl-C and cannot.** DESIGN §10.2 gives it two
meanings, and raw mode turns ISIG off, so the prompt's Ctrl-C arrives as byte
`0x03` and never reaches a handler. M11 owns that half. The SIGINT half belongs to
the first milestone that runs an interruptible program, and **that is this one** —
which means this milestone also takes `install_interrupt_handler()`, 249 lines in
v1 (`system_facts/interrupt.hpp` + `.cpp`), installed without `SA_RESTART`, that
PLAN §6 keeps and marks *hard-won; do not rediscover* and that no milestone has
ever named. `eof()` is the entire discrimination between a closed stdin and an
interrupted read, so `1 5 2`–`1 5 4` cannot report truthfully without it.

**`width` and `height` are not `arguments.machine.*`,** which the ledger wondered
about. DESIGN §7.7's test is three surfaces and one set of facts: `.threads`,
`.cores` and `.memory.total` live there because they are also
`THREAD_COUNT`/`CORE_COUNT`/`MEMORY_MAX` and codes 97/98/99, all out of
`system_facts`, and "must not be allowed to disagree". Terminal size has no second
surface, no config counterpart, and **changes during a run**, which a
startup-sampled object cannot express. `clear()` and `home()` could not go there
under any reading — they are not facts, and `arguments.clear()` would collide head
on with the container selector at `1 4 1 6` / `1 4 2 11`. WORD_NUMBERS §1.2 closed
it anyway: never renumber, never reuse.

Roughly 150 lines port close to unchanged — `evaluator/modules.cpp` 62–204, where
the comments are the specification and should come across with the code, plus
`terminal_columns()`'s ioctl with its 80-column fallback. `typed()` and the reader
thread port nothing. `console_output/console.cpp` is not ported here and **is the
file to read before writing the reader**, because every hazard it documents —
swapping the vector out under the lock rather than erasing the front, two condition
variables so a waiting drain and a sleeping thread are never woken for each other's
reason — reappears reversed.

Open, and each of these shapes the milestone rather than decorates it:

- **One reader of stdin, or two?** A reader thread parked in `read()` and an
  `input()` calling `getline` on the walking thread are two consumers racing one
  fd. Routing `input()` through the same queue is almost certainly right and it
  changes code §6 marks *do not rediscover*, so it is a decision and not a detail.
- **Ctrl-C across a thread boundary.** §10.2's mechanism was designed for a read on
  the walking thread. Move the read and the signal lands on an arbitrary thread,
  EINTR surfaces where there is no error to report, and the
  interrupted-versus-EOF answer has to travel back through the queue.
- **When the thread starts and how it ever stops.** §4.3 fixes satl's own startup
  share at 0.01 ms and §4.5.1 measured 24 threads at 0.5–1.5 ms, so an eager start
  is a visible regression against the floor every milestone is measured on; and a
  thread blocked in `read()` cannot be joined at exit the way `~Console()` joins
  the printer. Lazy plus a self-pipe wakeup, or detach and leak, differ in whether
  `satl` exits cleanly. Related: §4.5.1 names three tenants of the lazy pool and
  this is a fourth it does not mention.
- **What "nothing" is, and how a program tests for it.** `1 5 5` is "a line or
  nothing" and `satellite.variable.file.read_line` `1 6 2 3` is "one line, or
  nothing at end" — the identical shape, so it is decided once for both.
  `satellite.variable.variant` is in "Later" and nil is DESIGN §6.4 q3's *declared
  variable holding nothing*.
- **How the registry declares that a parameter is a place.** §7 already condemns
  v1's route to `1 5 4` — flatten the path, compare it against a string literal,
  before evaluating arguments — as the same hack as the `100ms` case six lines
  away. The replacement is a declaration on the word, the way `display` will
  *declare* that it accepts a pace argument. §6.4 q2's receiver-binding tag is the
  nearest precedent and nothing yet extends it to a non-receiver parameter.
- **Does `clear()` imply `home()`?** v1 emits both as one sequence. If `1 5 8`
  homes, `1 5 9` is only ever useful alone; if it does not, QUAD's frame draw is
  two calls where `view.hpp` had one. Small, and it is a promise.
- **What the reader does during M11's prompt**, which puts the terminal in raw mode
  and thinks it owns stdin. Whether the reader parks or the prompt uses it is
  decided by whichever milestone lands second, and saying so now is cheaper than
  finding it at M11.

Done when: one program, run under `pty.fork()` — assert on the screen, not on the
bytes — proves all eight paths, and each clause is a separate assertion.

1. **A counter keeps rising while a line is half-typed.** Send `/q` a byte at a
   time; the number advances across every pause. QUAD.md §3.4 requirement (1),
   with `VMIN=0` retired, and the one claim the milestone exists to make.
2. **And it does not spin.** CPU near idle between keystrokes, because the blocking
   happens on a thread that waits — "strictly better than the poll loop the obvious
   alternative produces" (DESIGN §10.1).
3. **`typed()` tells an empty line from no line.** Return on an empty line is a
   line; nobody typing is nothing. Two answers, not one empty string.
4. **The prompt appears before the cursor waits.** `1 5 3` prints with no trailing
   newline and with the queue drained; a prompt arriving after the program has
   already blocked is the failure `drain()` exists to prevent and is observable
   only here.
5. **`1 5 4` writes a place and yields nil**, assigning its result is an error, and
   a non-variable second argument fails *before* the prompt prints — so nobody
   types an answer that is then thrown away.
6. **Ctrl-C is not end of input.** At the prompt it reports `interrupted`;
   `satl demo.satl < /dev/null` reports reaching the end of input and exits instead
   of looping. Getting those two backwards is the regression §6 says not to
   rediscover.
7. **`width` and `height` are live.** Resize the pty between two frames and both
   numbers change without a restart.
8. **`clear()` and `home()` never race the printer.** Alternating `home()` with
   `display()` in a tight loop never shows an escape landing between two queued
   lines — proof both went through the queue rather than the fd.

The demonstration wants `satellite.time.sleep(n)` `1 9 3`, which is in "Later" and
belongs to no milestone: a busy-spin loop proves clause 1 and makes clause 2
untestable. **Clauses 1, 2 and 8 cannot be demonstrated by any earlier milestone,
and cannot be demonstrated by the eight paths one at a time** — which is §8's own
argument that this is one milestone rather than a bullet added to two.

### What the lenses found — UNAPPLIED

**PROBLEMS.** PROBLEMS — seven errors, none of them a mistyped digit.

Every number the draft transcribes from WORD_NUMBERS.md §2.2 is written exactly as §2.2 writes it, parens and all: `1 5 1`–`1 5 9` for `display` / `input()` / `input(prompt)` / `input(prompt, target)` / `typed()` / `width` / `height` / `clear()` / `home()`, plus `satellite.statement.while` `1 13 3`, `satellite.variable.file.read_line` `1 6 2 3`, `satellite.container.map.clear` `1 4 1 6`, `satellite.container.list.clear` `1 4 2 11` and `satellite.time.sleep(n)` `1 9 3`. The counts that matter hold: `1 5 2` through `1 5 9` is eight; §2.2 has no tenth child of `console`, so "every child except `display`" is exact; the ledger's own "8 of 10" for `satellite.console` (10 rows = the `1 5 (0)` node plus nine children) agrees; and console's children are dense 1–9, so the M2 static_assert claim survives. The draft never quotes the 218-row / 215-distinct totals, so there is nothing to get the wrong way round there, and it treats no alias as its own path.

The failures are all in what a correct number is attached to. The worst is `satellite.time.sleep(n)` `1 9 3` being placed "in Later" — Later names `satellite.variable.time` `1 6 3`, a different node, and MILESTONE §0.0 exists specifically to correct that confusion; the draft has copied it back in from MILESTONE §1's stale row. Second is the `arguments.clear()` collision, which cites the map's `1 4 1 6` for a receiver DESIGN §7.7 declares a list, calls two spelling-sharing nodes "the container selector," and describes as a numbering collision something §1's per-parent rule makes impossible. Third, `.memory.total` is filed under `arguments.machine.*` when §2.2 puts `memory` `1 14 1 1 2` beside `machine` `1 14 1 1 1`, not under it.

The remaining four are counts: "four milestones" between M8 and M11 (there are three — M9, M9.5, M10); `modules.cpp` 62–204, which over-runs the input code's end at 187 and pulls seventeen lines of M8's `display(text, end=)` into this milestone's port — the exact ownership blur the M8 paragraph is written to prevent; "six lines away" for two v1 hacks 44 lines apart; and "two condition variables" where `console.hpp` declares three.

On the trap in both directions: the M8 finding is sound and checks out verbatim (PLAN §6 keeps "the Console with its own printer thread, and the `drain()` barrier before reading input"; DESIGN §10.1 justifies the flush by "a prompt written with no trailing newline"; the v1 comment at modules.cpp:65 says the drain is the whole reason the function exists). The reverse direction is clean — QUAD §4's other two items have both since been placed (float at M9.5; PLAN M10 now says outright "The sort primitive is part of this milestone," `1 4 2 3`–`1 4 2 7`), so "this was the last" holds. Verified as accurate: 1449 lines in `console_input/`, 249 in `system_facts/interrupt.{hpp,cpp}`, the 14 lines of `terminal_columns()` + `clear_screen()`, zero `ws_row`, VMIN=1 (so no non-blocking input in v1), ISIG off giving byte 0x03, `install_interrupt_handler()` without SA_RESTART, and both DESIGN §10.1 quotations. One softer point outside the numbers lens: "the first milestone that runs an interruptible program, and that is this one" is contestable, since `satellite.statement.while` `1 13 3` gives a program that loops one milestone earlier at M9.

1. **Claim:** "The demonstration wants `satellite.time.sleep(n)` `1 9 3`, which is in "Later" and belongs to no milestone."
   **Wrong because:** The digits are right (WORD_NUMBERS §2.2: `satellite.time.sleep(n)` `1 9 3`) but the node is not in Later. PLAN §8's Later list is `satellite.variable.file`, `.time`, `.date`; … — the `.time` there is `satellite.variable.time` `1 6 3 (0)`, a different node from `satellite.time` `1 9 (0)`. This is the exact error SCRATCH.md/MILESTONE.md §0.0 was written to correct: "`satellite.file` `1 8` (5) and `satellite.time` `1 9` (4) were filed as 'covered only by Later.' Later names `satellite.variable.file` and `satellite.variable.time` — different nodes." §0.1 then files `satellite.time`'s four paths under "nothing — same mistake, same shape." The draft has inherited the wording from MILESTONE §1's own sleep row ("'Later' has `.time`"), which §0.0 had already overturned. DESIGN §4.4 and WORD_NUMBERS §2.3 name this hazard directly: two nodes sharing one spelling.
   **Fix:** Write: "`satellite.time.sleep(n)` `1 9 3`, which is reached by nothing — not §8, not Later, not prose. Later names `satellite.variable.time` `1 6 3`, a different node (MILESTONE §0.0), so `satellite.time`'s paths `1 9 1`–`1 9 3` are uncovered outright." That strengthens the point rather than weakening it: a dependency in Later is merely unordered, a dependency in nothing is unscheduled.

2. **Claim:** "`arguments.clear()` would collide head on with the container selector at `1 4 1 6` / `1 4 2 11`."
   **Wrong because:** Three errors in one clause. (a) `1 4 1 6` is `satellite.container.map.clear` and `1 4 2 11` is `satellite.container.list.clear` — not one selector with two numbers but two distinct nodes that share a spelling, which is precisely DESIGN §4.4's arrangement that WORD_NUMBERS §2.3 sets against aliasing ("deduplication is many-nodes-one-string, aliasing is one-node-many-strings"). §2.2 in fact carries four `clear`s: `1 4 1 6`, `1 4 2 11`, `satellite.variable.string.clear` `1 6 1 15`, `satellite.file.clear` `1 8 3`. (b) Only one is reachable through `arguments`: DESIGN §7.7 declares it `satellite.container.list<satellite.variable.string>`, so §1.5's selector rule resolves `arguments.clear()` through the list's type to `1 4 2 11` alone; the map's `1 4 1 6` is unreachable from a list receiver. (c) Numbers cannot "collide head on" at all — WORD_NUMBERS §1 is explicit that the lists are per-parent, so a number under `arguments` and one under `container.list` "have nothing to do with each other. That is the scheme working." The collision is over the spelling `clear`, never over a number.
   **Fix:** "…and `arguments.clear()` would already mean something: `arguments` is a `satellite.container.list` (DESIGN §7.7), so §1.5 resolves the bare `clear` through the receiver's type to `satellite.container.list.clear` `1 4 2 11`. That is a spelling collision, not a numbering one — §1's per-parent rule means the numbers could never clash." Drop `1 4 1 6` entirely.

3. **Claim:** "DESIGN §7.7's test is three surfaces and one set of facts: `.threads`, `.cores` and `.memory.total` live there" — "there" being `arguments.machine.*`, the paragraph's subject.
   **Wrong because:** `.memory.total` does not live under `machine`. §2.2 numbers `satellite.library.main.arguments.machine` at `1 14 1 1 1` and `satellite.library.main.arguments.memory` at `1 14 1 1 2` — siblings under `arguments`, not parent and child. So `.memory.total` is `1 14 1 1 2 1`, outside the `arguments.machine.*` subtree the paragraph is arguing against, and DESIGN §7.7 spells it `arguments.memory.total` in both its tree and its "Three surfaces" paragraph. (The 97/98/99 mapping itself is right: threads/cores/memory.total against THREAD_COUNT/CORE_COUNT/MEMORY_MAX, in that order.)
   **Fix:** Either widen the subject — "are not `arguments.*`" — or name the two subtrees: "`arguments.machine.threads` `1 14 1 1 1 3`, `.machine.cores` `1 14 1 1 1 1` and `arguments.memory.total` `1 14 1 1 2 1` live under the `arguments` object because they are also THREAD_COUNT/CORE_COUNT/MEMORY_MAX and codes 97/98/99."

4. **Claim:** "Roughly 150 lines port close to unchanged — `evaluator/modules.cpp` 62–204, where the comments are the specification and should come across with the code."
   **Wrong because:** The input code ends at line 187. Lines 189–198 are `named_arg_misuse()`, whose entire body is the error message for `satellite.console.display(text, end="")`, and 200–204 are the comment header for `display_with_end()` — whose function body starts at 205, so the range also severs a function from its own comment. Both belong to `display` `1 5 1`, which is to say to M8, the milestone this draft has just finished arguing owns the un-newlined write. Pulling seventeen of M8's lines into this milestone's port is the same ownership blur the M8 paragraph exists to prevent. The real range 62–187 is 126 lines, not ~150.
   **Fix:** "Roughly 125 lines port close to unchanged — `evaluator/modules.cpp` 62–187 (`read_input_line`, `console_input`, `console_input_into`), where the comments are the specification. Lines 189 onward are `display`'s and stay with M8."

5. **Claim:** "`SCRATCH.md/MILESTONE.md` §1 proposed it "between M8 and M11", which is four milestones of latitude."
   **Wrong because:** PLAN §8 has three milestones between M8 and M11: M9, M9.5, M10. "Four" is only reachable by counting insertion gaps rather than milestones, which is not what the sentence says.
   **Fix:** "…which is three milestones of latitude" — or, if gaps were meant, "which leaves four places to put it."

6. **Claim:** "§7 already condemns v1's route to `1 5 4` … as the same hack as the `100ms` case six lines away."
   **Wrong because:** In `old_versions/first_satellite/src/evaluator/expr_call.cpp` the `100ms` special case occupies lines 44–62 and the `input(prompt, target)` match begins at line 106 — 44 lines apart, not six. Nothing six lines from the `1 5 4` block is the pace form either: the nearest neighbour is `display(x, end=…)` at line 118, two lines below. The condemnation itself is sound — PLAN §7's first bullet, "the joined path built per call," covers `flatten_path` + `join_path(path) == "satellite.console.input"` at expr_call.cpp:113–114 — but §7 never names `1 5 4`, so "already condemns" claims more specificity than the text has.
   **Fix:** "§7's first bullet — 'the joined path built per call' — already condemns v1's route to `1 5 4` (`expr_call.cpp:110–116`: flatten the path, compare it against a string literal, before evaluating arguments), the same hack as the `100ms` case forty lines above it at `expr_call.cpp:44–62`."

7. **Claim:** "every hazard it documents — swapping the vector out under the lock rather than erasing the front, two condition variables so a waiting drain and a sleeping thread are never woken for each other's reason — reappears reversed."
   **Wrong because:** `console_output/console.hpp` declares three condition variables: `pace_woken_` (line 184), `arrived_` (191) and `emptied_` (196). The pair the header's comment describes is `emptied_` against `arrived_` — "Separate from arrived_ so that a waiting drain and a sleeping printer are never woken for each other's reason" — while `pace_woken_` is a third, deliberately guarded by its own `pace_mutex_` so a printer waiting out a pause never holds the queue lock. A claim to enumerate "every hazard it documents" that stops at two undercounts by one, and "a sleeping thread" blurs the header's "sleeping printer," which is the point of the separation.
   **Fix:** "…three condition variables — `arrived_`, `emptied_` and `pace_woken_` on a mutex of its own — so that a waiting drain and a sleeping printer are never woken for each other's reason."

**PROBLEMS.** PROBLEMS — six errors, none of them in the path arithmetic. The console subtree itself checks out: WORD_NUMBERS.md §2.2 has exactly `1 5 (0)` and `1 5 1`–`1 5 9`, so claiming `1 5 2`–`1 5 9` alongside M8's `display` leaves no child of `console` unowned, and nothing in PLAN §8 currently reaches width, height, clear or home. Every v1 figure I checked is right: `ws_row` is zero hits, `system_facts/interrupt.hpp`+`.cpp` is 249 lines, `console_input/` is 1449 source lines, `console_input/raw_mode.cpp:65` confirms ISIG-off turns Ctrl-C into byte `0x03`, and `evaluator/expr_call.cpp:114` confirms the `join_path(path) == "satellite.console.input"` hack PLAN §7 condemns. The errors are in ownership and ordering. (1) The done-when requires `satellite.time.sleep` `1 9 3`, which SCRATCH.md/MILESTONE.md §0.0 shows is scheduled nowhere — "Later" names `satellite.variable.time`, a different node — and the draft concedes clause 2 is untestable without it. (2) "The first milestone that runs an interruptible program ... is this one" is false by the draft's own argument that `while` `1 13 3` runs at M9; v1's handler lets "the walk stop itself at the next statement," whose first consumer is M9, one slot earlier — an owner landing after its consumer. (3) "M8 owns the output half of `1 5 3`" splits one numbered path across two milestones, the very failure the paragraph exists to prevent; M8 owns `display`'s un-newlined form (under `1 5 1`) and `drain()`, neither of which is `1 5 3`. (4) Clause 3 asserts nil-versus-empty while open question 4 asks what nil is; `nil` has no numbered path anywhere and `satellite.variable.variant` is in Later, so the clause is not assertable. (5) The port range `modules.cpp` 62–204 overshoots into M8's `named_arg_misuse` and `display_with_end`; `console_input_into` ends at 186. (6) The decimal M9.25 is coherent only if MILESTONE.md §0.4 resolves one way — it also proposes moving M9.5 ahead of M9, which the draft never addresses. Findings 1 and 2 are the ones that would land wrong in PLAN.md as written. One smaller note not listed as an error: v1's `terminal_columns()` is called by `console_input/render.cpp:61` and `Renderer::clear_screen` by `console_input/line_reader.cpp:146`, so M11's prompt consumes the two helpers this milestone builds — that is the right direction for the ordering, but M11's line should say it consumes them rather than leaving a future reader to reimplement 14 lines.

1. **Claim:** Done-when clause 2 ("And it does not spin. CPU near idle between keystrokes") is a demonstrable clause of this milestone. The draft admits it needs `satellite.time.sleep(n)` `1 9 3` and leaves it there: "which is in 'Later' and belongs to no milestone: a busy-spin loop proves clause 1 and makes clause 2 untestable."
   **Wrong because:** This is the exact ordering bug the lens asks about — a milestone that cannot be demonstrated until something later. WORD_NUMBERS.md §2.2 puts `satellite.time.sleep(n)` at `1 9 3` under `satellite.time` `1 9`, and PLAN §8's "Later, in no fixed order" names `satellite.variable.time`, a DIFFERENT node — SCRATCH.md/MILESTONE.md §0.0 draws exactly that distinction and files all four `satellite.time` rows as reached by "nothing". So `1 9 3` is scheduled nowhere at all, and the milestone's own text concedes the clause is untestable without it. A done-when with a clause its author says cannot be tested is not a demonstration; PLAN §8's rule is violated on the milestone's own admission. M9.5 handles the same situation honestly — "Done when the four operations run and — the blocker — the rounding rule is chosen" — by putting the blocker IN the done-when. This draft puts its blocker in a footnote after it.
   **Fix:** Either (a) claim `satellite.time.sleep(n)` `1 9 3` in this milestone explicitly, and in the same breath say what happens to its three siblings `satellite.time` `1 9 (0)`, `.now` `1 9 1` and `.new` `1 9 2` — otherwise naming `satellite.time` and taking one child is the same trap one level down that this milestone was written to avoid; or (b) delete clause 2 and demote "it does not spin" to a note, leaving seven clauses. Do not leave it as a demonstration that depends on an unscheduled path.

2. **Claim:** "The SIGINT half belongs to the first milestone that runs an interruptible program, and **that is this one** — which means this milestone also takes `install_interrupt_handler()`."
   **Wrong because:** False by the draft's own dependency argument, and it inverts the ordering. The draft places itself after M9 because "`satellite.statement.while` `1 13 3` runs at M9" — so M9 is the first milestone with a loop, and therefore the first that can run a program long enough to be stopped at a statement boundary (M8 already runs a program with statement boundaries at all). v1's `system_facts/interrupt.hpp:70-73` describes the mechanism as "the FIRST SIGINT sets the flag and lets the walk stop itself at the next statement, which is what makes an interrupted program report the line it was on" — an evaluator-walk feature whose first real consumer is M9's `while`, one slot EARLIER than this milestone. An owner landing after its first consumer is a bug in the ordering. Separately, the draft calls the handler something "no milestone has ever named" two sentences after correctly noting that PLAN §8's M11 line reads "The prompt, Ctrl-C, the exit words" — so Ctrl-C is already named, and unless M11's line is amended, two milestones own it.
   **Fix:** Drop the "first interruptible program" justification, which points at M9, and keep only the argument genuinely specific to these paths: `eof()` is the discrimination between a closed stdin and a read interrupted by EINTR (PLAN §6, DESIGN §10.2, `modules.cpp:76-99`), so `1 5 2`–`1 5 4` cannot report truthfully without it. Then say which half lands where — the statement-boundary flag and escalation belong wherever the interruptible walk first exists — and prescribe the M11 edit the way the draft prescribes the M8 edit: M11's line must read "the prompt's Ctrl-C (the `0x03` byte, ISIG off)" rather than bare "Ctrl-C".

3. **Claim:** "**M8 owns the output half of `1 5 3`** and does not say so... M8's line should say it; this milestone consumes them and must not rebuild them."
   **Wrong because:** This splits one numbered path across two milestones, which is the failure the draft itself cites as fatal ("Two milestones owning one path is as bad as none owning it"). `1 5 3` is `satellite.console.input(prompt)` — one call shape, one identity, per WORD_NUMBERS §1.3 where "the arity is the identity." It has no "output half." What M8 actually owns are two differently-named things: the un-newlined form of `display` (which lives under `1 5 1`) and the `drain()` barrier (a mechanism with no number at all, kept by PLAN §6 as "the Console with its own printer thread, and the `drain()` barrier before reading input"). If M8's line is amended to say it owns "the output half of `1 5 3`", a future per-path audit finds `1 5 3` named twice — a fourth instance of this project's recurring failure, created by the paragraph written to prevent it. And the un-newlined `display` has no number to be owned by anything yet: WORD_NUMBERS §2.2 gives `display` exactly one row, `1 5 1`, with no arity variants, and §4 lists "The variadic split is mechanical but not small" as still open.
   **Fix:** Reword to: "M8 owns `display`'s un-newlined form and the `drain()` barrier, and its line should say so. This milestone owns all of `1 5 3` and consumes both." Neither M8 nor this milestone should be described as owning a fraction of a number.

4. **Claim:** Done-when clause 3: "**`typed()` tells an empty line from no line.** Return on an empty line is a line; nobody typing is nothing. Two answers, not one empty string" — while open question 4 asks "**What "nothing" is, and how a program tests for it.**"
   **Wrong because:** The same undecided mechanism is both a listed open question and an assertion in the demonstration. There is today no way for a satellite program to make that distinction: `nil` has zero occurrences in WORD_NUMBERS.md and exactly two in DESIGN.md (§6.4 q3 and §8.2), neither of which is a test a program can write; there is no numbered path for it anywhere in §2.2; and `satellite.variable.variant`, which the draft names as the relevant machinery, sits in PLAN §8's "Later, in no fixed order." A pty test asserting on the screen cannot separate the two outcomes either — printing nil and printing "" look the same on a screen unless the program branches on the difference first. So clause 3 requires a decision plus a new numbered path this milestone would have to own and does not name, or it is not assertable.
   **Fix:** Give the milestone the mechanism or give up the clause. If it takes it, name the path it appends (WORD_NUMBERS §1.2 forbids renumbering or reuse, so it must be an append) and state that the decision binds `satellite.variable.file.read_line` `1 6 2 3` in Later. If it does not, move clause 3's blocker into the done-when the way M9.5 does — "done when ... and the nothing-versus-empty answer is chosen" — instead of leaving a demonstration resting on an open question.

5. **Claim:** "Roughly 150 lines port close to unchanged — `evaluator/modules.cpp` 62–204, where the comments are the specification and should come across with the code."
   **Wrong because:** The cited range runs past the input code into M8's, contradicting the draft's own instruction not to rebuild what M8 owns. In `/home/madness/code/cxx/satellite/old_versions/first_satellite/src/evaluator/modules.cpp`, `console_input_into` ends at line 186. Line 188 begins `named_arg_misuse` — whose message text is literally about `satellite.console.display(text, end="")` — and lines 200–204 are the comment header of `display_with_end`. Both are M8's un-newlined display, the exact code the draft says "this milestone consumes them and must not rebuild them." 62–204 is also 143 lines, not "roughly 150"; the input-only region is about 130.
   **Fix:** Cite `evaluator/modules.cpp` 62–186 (`read_input_line`, `console_input`, `console_input_into`), roughly 130 lines. Note separately that 188–226 (`named_arg_misuse` and `display_with_end`) is M8's, and that `set_display_pace` at 227–233 is the `100ms` case PLAN §7 throws away.

6. **Claim:** "It goes immediately after M9 and ahead of M9.5 ... (Renumbering the float to M9.75 does the same job; the decimal awkwardness belongs to the `.5` convention and not to this.)"
   **Wrong because:** The number M9.25 is coherent only under one resolution of an ordering question the ledger leaves open, and the draft never mentions it. SCRATCH.md/MILESTONE.md §0.4 — "One ordering problem, which is not a coverage gap" — records that four `satellite.variable.number` methods (`power` `1 6 4 10`, `sqrt` `1 6 4 14`, `modulus` `1 6 4 12`, `truncate` `1 6 4 13`) cannot finish at M9 because rounding is M9.5's blocker, and concludes: "Either those four move to M9.5 or **M9.5 moves ahead of M9**." If M9.5 moves ahead of M9, M9.25 sits numerically before a milestone it must follow and has to be renamed on the spot. The draft argues at length past M9.5's blocker but engages only with "M9.5 cannot land until the rounding rule is chosen," never with §0.4's proposal to reorder the two milestones this one is wedged between.
   **Fix:** State the dependency as a relation rather than a decimal — "after M9 (`while` `1 13 3` and `satellite.variable.string`), independent of M9.5" — and add one sentence resolving it against MILESTONE.md §0.4: if M9.5 moves ahead of M9, this milestone still lands after M9 and takes whatever number that implies.

---

## The clock and the dice — the two sources of nondeterminism

### The draft

**M9.75 — the clock and the dice, the two sources of nondeterminism.** *(New on
2026-08-27. It lands here and not after M10 because nothing in it needs a list or a
map, and putting it past the containers would imply a dependency that does not
exist. It needs M9, because a sleep outside a loop demonstrates nothing. It sits
directly after M9.5 on purpose: that milestone establishes the float, and this one
has to report that the dice cannot use it.)*

Twenty numbers, named here because a milestone that names only `satellite.random`
leaves twelve children owned by nothing — which is this document's recurring failure:

- `satellite.random` `1 7 (0)`, and `1 7 1` through `1 7 12`.
- `satellite.time` `1 9 (0)`, `.now` `1 9 1`, `.new` `1 9 2`, `.sleep(n)` `1 9 3`.
- `satellite.variable.time` `1 6 3 (0)`, `.date` `1 6 7`, `.duration` `1 6 8`.

**`satellite.random` is thirteen numbers written on sixteen rows.** `.range` is a
second spelling of the two-argument shape, not a fourth segment and not a path
(WORD_NUMBERS §2.3): `fast.range(min, max)` *is* `1 7 5`, `normal.range` *is*
`1 7 8`, `ultra.range` *is* `1 7 11`. **Those three are the only duplicate numbers in
the language** — 218 rows, 215 distinct — and they are the whole of the difference
between the 218 this section quotes and the 215 DESIGN §12 and WORD_NUMBERS §4 do.
That reconciliation is currently written down only in `SCRATCH.md/MILESTONE.md` §5,
which is scratch and is going to be deleted; it lives here now.

**This moves `satellite.random.*` and `.time`, `.date` out of "Later, in no fixed
order," and "Later" is not a milestone** — which is how sixteen rows of working,
ported, tested code came to be sitting in it. Two of this milestone's paths were not
even there: `satellite.time` `1 9` is a different node from `satellite.variable.time`,
and `satellite.variable.duration` `1 6 8` is in no list at all. The sentence below
"Later" calling `satellite.random` "16 more" is counting rows, and M2's acceptance
test already names `1 7 2` outright, so the number uncovered was never 16.

**DESIGN §11 is stale about how many shapes there are, and says so nowhere.** It says
"two shapes on each," which was true of the first satellite — six random rows in its
registry, and a header that reads "the public surface is two shapes per tier." §2.2
gives each tier four slots. §11 is a day and a commit older than the numbering
(`095eb49`, against `99fa6d9` and `cc3f813`), and neither numbering commit touched it.
**Building four shapes out of a section that says two is the M3/M4 failure run
backwards**, so correcting §11 is part of this milestone. Everything in it is still
true and stays: the tier table, uniform over [0, 10⁴⁰), inclusive at both ends, the
one-in-ten paragraph, and the whole of §11.1. What replaces "two shapes on each" is a
pointer to WORD_NUMBERS §2.2 and a line saying which numbers are specified — plus a
date, which §11 carries neither of and is how it went stale without looking wrong:

- `1 7 4`, `1 7 7`, `1 7 10` — `<tier>(digits)`. Specified, built, tested.
- `1 7 5`, `1 7 8`, `1 7 11` — `<tier>(min, max)`, spelled `.range` in v1. The same.
- `1 7 6`, `1 7 9`, `1 7 12` — `<tier>(min, max, step)`. Specified in no document.
- `1 7 1`, `1 7 2`, `1 7 3` — open below, and it decides whether the call surface is
  nine shapes or twelve.

**No tier is secure and none of them is described that way** — not in the header, not
in `satellite.help`, not in an error message, not here. DESIGN §11.1. PCG makes no
cryptographic claim and its state is recoverable from its output; `ultra` is slower
and better distributed and that is all it is. The first satellite records why the
refusal is worded that hard — the word "secure" in a language's documentation is
load-bearing, because someone will key something on it — and names the answer not
taken, `getrandom(2)`, 256 bits, about a microsecond, no new dependency. Both travel
with this milestone; otherwise a later reader re-derives them.

**The bignum half of the dice is M6.5's and this milestone does not claim it.**
`satellite_number/random.cpp` — 121 lines: the limb-aligned uniform draw in base 10⁹,
the rejection sampler that exists because a bare `%` would skew 2:1, and
`MAX_RANDOM_DIGITS = 100000` — is one of the ten files M6.5 ports, and §6.1's open
question 3 says as much. **M6.5 owns it and does not say so**, which is the fourth
instance of this failure after M3/M4's eleven words and M10's twenty-nine methods, and
the fix is one clause there rather than a claim here. What this milestone ports is the
other half, `random_numbers/random.{hpp,cpp}` — 260 lines, the tiers, the spun seed,
the fold and the watchdog — plus the dispatch, which is a **rewrite and not a copy**:
v1's `modules_random.cpp` is a string compare on the tier and on `"range"`, and §7
throws exactly that away. What survives it is the argument checking and the ten error
texts, reached through `handlers[path_id]` and reported through M5.

**The three alias rows break M2 as M2 is currently written, and that is a finding
about M2.** Its `words.def` is a tree where a node's position among its parent's
children *is* its number, and there is no tree entry that yields `1 7 5` for a node
named `range` under `fast` — under that rule it comes out `1 7 1 1`, the superseded
plan still visible in `SCRATCH.md/WORD_SURFACE.md`. So an alias cannot be a tree row
at all; it is a two-segment rewrite performed in the spelling table during the walk.
And M2's static_assert as specified — "dense from 1 with no holes and no duplicates" —
fires on `1 7 5`, `1 7 8` and `1 7 11`. QUAD.md §4 already states the check correctly,
as four properties with "no alias pointing at nothing" as the fourth, and M2 adopts
that wording. **Only these rows force either change**, which is why neither was
visible from M2. Two smaller consequences: WORD_NUMBERS §1.4 still illustrates its
argument with `satellite.random.fast.range`, "which is four numbers and nothing else"
— the same fossil, contradicting §2.2 from inside the same commit, and settled against
§1.4 by the file's own rule that prose may never be the only place a number lives; and
M3's "`hexadecimal` is the language's one alias" is one word wrong, because §2.3 lists
three families and one of them is these.

**The vendored PCG is the project's first third-party dependency and LAYOUT.md has no
row for one.** Three headers, 3138 lines, Apache-2.0 inside an MIT tree, header-only
so `ldd` does not change. It is reached through `-isystem` for a load-bearing reason
rather than a stylistic one: `pcg_extras.hpp` warns under this project's `-Wall
-Wextra`, and §4's silent from-scratch rebuild depends on that warning not being ours.
This milestone writes LAYOUT.md's first vendored row, and the Apache notice travels
with it.

**The tier windows are a design choice and stand; the watchdog is a measurement and
does not.** §9 says measure on this machine and do not quote. The first satellite's
figures are the Xeon E5-2670 v3's: about 153,600 draws per millisecond, `ultra`
reaching roughly 249M draws at 2000 ms and 310M at 3000 ms, against a cap drawn from
[500M, 600M] — a margin of about 1.6–1.9×. **On a machine 1.7× faster `ultra` reaches
the cap, and the watchdog stops being a failsafe and becomes part of the mechanism**,
silently shortening the spin. Re-measure before claiming the tiers behave as
documented; if the margin has closed, the per-tier caps v1 recorded and declined are
this milestone's work.

**What this milestone produces for QUAD is a correction, not a mechanism.** QUAD.md
§2 files `random` under "already decided in satellite, and fine." It is not.
`quad_core.hpp`'s entire randomness is one primitive — a uniform `double` in [0, 1)
from a seeded `mt19937` — and `Rack::draw` is a roulette wheel over `std::pow`
weights. Three independent reasons `satellite.random` cannot write it: **no float
draw exists in any of the thirteen numbers**, and v1 refuses fractional bounds
deliberately, because there is no uniform draw over the reals between 1 and 100;
**there is no seed**, every call spins on kernel entropy and reseeds from the fold,
so QUAD's determinism invariant is not expressible; and **the cheapest tier throws
draws away for 50–100 ms before it answers, against a 90 ms tick** that draws from
the rack once and calls the rng nineteen further times. These are different features
that share a namespace. It is the failure QUAD.md §5 predicted in as many words — the
gap between "the language has floats" and "this expression is writable" — and writing
it down is worth more than a fourth tier. `Sky::decay` needs no randomness at all, so
QUAD's candidate mechanism splits cleanly: decay is M9.5 plus M10, and `draw` is
neither this milestone nor yet anyone's. The shape of an answer is a fourth thing
under `satellite.random` that takes a seed and does not spin — `1 7 13` is free — and
**this milestone does not assign it**; that is the numbering's to mint or refuse.

Open questions, and the milestone is not finishable around them:

- **One clock or two.** DESIGN §13 says `satellite.variable.time`, `.date` and
  `satellite.time.now()` must agree on one clock and one epoch, and that a monotonic
  clock and a wall clock cannot both be the one. **The first satellite answered both
  halves and the answers do not compete**: `system_clock` and the Unix epoch for the
  value, because steady_clock's epoch is unspecified and the value has to mean
  something outside this process; `steady_clock` for the timer — the spin deadline and
  the sleep — because `high_resolution_clock` is the same type as `system_clock` here,
  `is_steady` is false, both tick at 1 ns with 21 ns granularity so there is no
  precision to trade away, and NTP can step `system_clock` backwards underneath a
  running deadline. Only one of the two is ever a satellite *value*, so "one clock,
  one epoch" is a rule about the type and not about the implementation. That reading
  measured the author's stated leaning and rejected it, so it is put to the author
  rather than taken here.
- **What `1 7 1`, `1 7 2` and `1 7 3` name** — the tier node, or a zero-argument call.
  The author's note, DESIGN §4's example and M2's own acceptance test all write
  `satellite.random.fast` bare; the parentheses appear first in the §2.2 rewrite,
  whose preamble promised that every number was preserved unchanged — a promise about
  numbers and not about what they name. Nothing says what range a bare draw would be
  uniform over, and v1 tests that source as an arity error and the bare tier as a path
  that is not a value. Only the numbering's authority can settle it.
- **The step forms** `1 7 6`, `1 7 9` and `1 7 12`. Assigned by rule, specified in no
  document and built nowhere, and they collide with the one promise §11 makes:
  `.range` is inclusive at both ends, and `(min, max, step)` reaches `max` only when
  `max - min` is a multiple of `step`. Whether `(1, 10, 3)` can answer 10 is exactly
  what §13 warns gets settled by accident.
- **What `satellite.time.new` `1 9 2` takes.** Its only specification anywhere is nine
  words in a deleted note — "set the arguments for a point in time" — which reads as a
  component constructor and contradicts v1's rule that an instant is read off the
  clock and never written down as a literal. It cannot be designed apart from
  `satellite.variable.date` `1 6 7`, since components are what a date is made of. And
  **DESIGN §13 cites it as an established precedent** — "exactly as
  `satellite.file.new` and `satellite.time.new` already do" — when it has never
  existed; the argument still holds, on `file.new` alone.
- **The unit of `satellite.time.sleep(n)`.** QUAD sleeps 90 ms. The one unit the first
  satellite spells is `100ms`, a lexed literal converted at parse time, and §7 throws
  away the special case that consumed it but not the literal. A bare number makes the
  unit invisible at the call site, which is the readability failure DESIGN §1.1 exists
  to prevent.
- **`satellite.variable.date` `1 6 7` has a number and nothing else** — no row in
  DESIGN §8's types table, no representation, no constructor, no method, no v1 code.
- **`satellite.variable.duration` `1 6 8` is a number minted from a refusal.** The
  sweep sourced it from v1's documents, and the v1 source that uses the phrase says
  there is no such type and gives four reasons — a duration value would have to answer
  `.plus`, `.size`, its type name and its identity as a map key first — while DESIGN
  §12 still defers durations today. It cannot simply be struck either: §1.2 says a
  removed child leaves a hole, and M2's assert demands none. **It stays numbered and
  unbuilt, and saying so is the work.**
- **The two time methods the first satellite ships are unnumbered** — `.minus(t)` and
  `.nanoseconds()`. `satellite.variable.time` `1 6 3 (0)` has zero children while its
  three hand-written siblings have 16, 7 and 14. WORD_NUMBERS has to number them and
  this milestone must not; until it does, a program can obtain an instant and do
  nothing whatever with one.

Done when this runs inside `satellite.main` and every clause below holds:

```satellite
satellite.variable.number big = satellite.random.ultra(40)
satellite.console.display(big)

satellite.variable.number die = satellite.random.fast.range(1, 6)
satellite.console.display(die)

satellite.variable.number i = 0
satellite.statement.while (i < 10)
{
    satellite.console.display(satellite.time.now())
    satellite.time.sleep(90)
    i = i + 1
}
```

1. `ultra(40)` is uniform over [0, 10⁴⁰), and a hundred runs give **about ten answers
   of 39 digits or fewer**. §11 says the leading-zero fact is said in the header and
   in the test, so the test asserts the short answers *happen*; one that merely
   tolerates them is not the test §11 asked for.
2. `.range(1, 100)` answers both 1 and 100, `.range(7, 7)` is 7, and
   `.range(-100, -90)` lands inside itself. The first satellite's assertions, ported.
3. All three windows are honoured, timed against `steady_clock` and asserted **on the
   floor only** — a ceiling is a machine's to miss under load, and a test that fails
   when the build box is busy is a test nobody trusts.
4. Draws per millisecond and `ultra`'s real draw count at 3000 ms are measured **on
   this machine, with the date, beside the watchdog cap they justify**, and the margin
   is stated. §9, and nothing above is quoted into this tree without being re-run.
5. Ten refusals produce ten messages through M5: a non-number digit count, a
   fractional one, a negative one, one past 100000, fractional bounds, a backwards
   range, wrong arity on each of two shapes, an unknown tier, an unknown tail segment.
   All ten are already written in v1 and port as text, not only as behaviour.
6. `satellite.time.now()` twice in succession returns two different values — the
   one-line proof that int64 nanoseconds is the representation and a `double` is not,
   since 61 bits of epoch against a 53-bit mantissa is 198 ns of resolution.
7. The loop paces at 90 ms, which is QUAD's main loop shape and the whole of what
   QUAD.md §4 asked `sleep` for. **This is not behind M12**: QUAD spawns no threads,
   and `sleep_for` is its only use of `<thread>`.
8. No tier is described as secure, anywhere.

**Nine of the twenty numbers stay reserved and unbuilt, and are listed rather than
left to a later audit**: `1 7 1`, `1 7 2`, `1 7 3`, `1 7 6`, `1 7 9`, `1 7 12`,
`1 9 2`, `1 6 7` and `1 6 8`. A milestone that quietly leaves numbers behind it is how
this document came to have a 122-path ledger.

### What the lenses found — UNAPPLIED

**PROBLEMS.** PROBLEMS — six, none of them in the place the numbering was most likely to break.

The two hardest checks both pass. `satellite.random` really is 13 distinct numbers on 16 rows: I counted §2.2 mechanically — 218 rows, exactly three duplicated numbers (`1 7 5`, `1 7 8`, `1 7 11`), 215 distinct — so "those three are the only duplicate numbers in the language" is exactly true, and the 218/215 attribution is the right way round (PLAN.md:952 quotes 218; DESIGN.md:1299 and WORD_NUMBERS.md:477 quote 215; SCRATCH.md/MILESTONE.md:226 is genuinely the only reconciliation, and it is in §5). Every one of the twenty numbers exists on the path claimed and is written as §2.2 writes it; the four-slots-per-tier split (`1 7 1`/`1 7 4`/`1 7 5`/`1 7 6` and its two rotations) is correct; twelve children is correct; `1 7 13` is free; nine reserved plus eleven built is twenty. The alias is never treated as its own path. `1 6 3 (0)` having zero children against siblings of 16, 7 and 14 checks out (`1 6 1 1`-`16`, `1 6 2 1`-`7`, `1 6 4 1`-`14`).

Supporting facts also hold: §11's body is byte-identical since 095eb49 and neither 99fa6d9 nor cc3f813 touched it; both the alias rows and §1.4's "four numbers and nothing else" fossil entered in the same commit, 99fa6d9; SCRATCH.md/WORD_SURFACE.md:132 does show the superseded `1 7 1 1`; PLAN.md:753 does say "the language's one alias" against §2.3's three; QUAD.md:283-284 does state the check as four properties ending "no alias pointing at nothing"; and v1's line counts (121, 260, 3138), the 153,600/249M/310M/[500M,600M] figures, MAX_RANDOM_DIGITS = 100000, the 2:1 skew, the steady_clock/system_clock split and the getrandom(2) note are all quoted accurately.

The six errors are all counts or attributions around the edges: one source cited for the opposite of what it says (the author's note writes `fast(99)` = `1 7 4`, not a bare tier), one bullet whose leading-dot notation hangs `1 6 7` and `1 6 8` off `1 6 3` in violation of §1's per-parent rule, "sixteen rows of working code" where nine is right, "two paths" where five is right, "ten error texts" where nine is right, and a QUAD §4/§2 citation swap. None of them changes a number; all of them are the kind of figure that gets quoted back at the milestone later as if it were one.

1. **Claim:** Open question 2: "The author's note, DESIGN §4's example and M2's own acceptance test all write `satellite.random.fast` bare; the parentheses appear first in the §2.2 rewrite."
   **Wrong because:** The author's note is evidence for the opposite, and M2's test names a different tier. /home/madness/code/cxx/satellite/SCRATCH.md/FIRST_NOTE.md:133-135 writes `satellite.random.fast(99)`, `satellite.random.normal(99)`, `satellite.random.ultra(99)` — the DIGIT shape, which §2.2 numbers `1 7 4`, `1 7 7`, `1 7 10`, not `1 7 1`-`1 7 3`. There is no bare tier anywhere in the note. Only two sources write a bare tier: DESIGN.md:213-215, which does write all three (`satellite.random.fast 1 7 1`, `.normal 1 7 2`, `.ultra 1 7 3`), and PLAN.md:697, which writes `satellite.random.normal` — not `.fast`. This matters because the open question is which of `1 7 1`, `1 7 2`, `1 7 3` names what, and the draft cites three sources for the bare reading when one of them argues against it.
   **Fix:** Rewrite as: "DESIGN §4's example writes all three tiers bare — `satellite.random.fast` `1 7 1`, `.normal` `1 7 2`, `.ultra` `1 7 3` (DESIGN.md:213-215) — and M2's acceptance test writes `satellite.random.normal` bare at `1 7 2`. The author's note writes the digit shape only, `satellite.random.fast(99)`, which is `1 7 4`, so it settles nothing about `1 7 1`. The parentheses appear first in the §2.2 rewrite."

2. **Claim:** "- `satellite.variable.time` `1 6 3 (0)`, `.date` `1 6 7`, `.duration` `1 6 8`."
   **Wrong because:** The leading-dot convention is set one line earlier by "`satellite.time` `1 9 (0)`, `.now` `1 9 1`", where the dot means a CHILD of the bullet's head. Read the same way, this bullet asserts `satellite.variable.time.date` = `1 6 7` and `satellite.variable.time.duration` = `1 6 8`. §2.2 has neither: `1 6 7` is `satellite.variable.date` and `1 6 8` is `satellite.variable.duration`, both SIBLINGS of `satellite.variable.time` under `satellite.variable`. Worse, the reading is impossible under §1's per-parent rule — a child of `1 6 3` is `1 6 3 n`, never `1 6 7` — and the draft's own last open question says `1 6 3 (0)` "has zero children."
   **Fix:** Write the paths in full, as §2.2 does: "- `satellite.variable.time` `1 6 3 (0)`, `satellite.variable.date` `1 6 7`, `satellite.variable.duration` `1 6 8` — three siblings under `satellite.variable`, not a subtree."

3. **Claim:** "...which is how sixteen rows of working, ported, tested code came to be sitting in it."
   **Wrong because:** Nine of `satellite.random`'s sixteen rows are code v1 built and tested; seven are not. v1's registry has exactly six random paths — old_versions/first_satellite/src/bytecode_format/format.def:248 says so in as many words ("Five words buy six paths") and lines 595-600 list them: `fast/normal/ultra(digits)` and `.fast/.normal/.ultra.range`. Those six map onto nine §2.2 rows (`1 7 4`, `1 7 5` + its alias row, `1 7 7`, `1 7 8` + alias, `1 7 10`, `1 7 11` + alias) carrying six distinct numbers. The other seven rows are `1 7 (0)` plus the six numbers the draft's own closing paragraph reserves as unbuilt — `1 7 1`, `1 7 2`, `1 7 3`, `1 7 6`, `1 7 9`, `1 7 12` — of which the draft itself says the step forms are "specified in no document" and built nowhere.
   **Fix:** "...which is how nine rows — six of the thirteen numbers, and exactly the six paths v1's registry carries — came to be sitting in it, alongside seven rows nothing has ever built."

4. **Claim:** "Two of this milestone's paths were not even there: `satellite.time` `1 9` is a different node from `satellite.variable.time`, and `satellite.variable.duration` `1 6 8` is in no list at all."
   **Wrong because:** "Two" counts nodes while every other count in the draft — and in the ledger it is correcting — counts paths. `satellite.time` `1 9 (0)` is four numbers, not one: `1 9 (0)`, `1 9 1`, `1 9 2`, `1 9 3`, all four of which the draft's own opening list claims. SCRATCH.md/MILESTONE.md:57-60 counts it that way explicitly — "`satellite.file` `1 8` (5) and `satellite.time` `1 9` (4)... Nine more paths with nothing anywhere." With `1 6 8` that is five of the milestone's twenty numbers absent from "Later", not two.
   **Fix:** "Five of this milestone's twenty numbers were not even there: all four under `satellite.time` `1 9 (0)`, a different node from `satellite.variable.time`, and `satellite.variable.duration` `1 6 8`, which is in no list at all."

5. **Claim:** "What survives it is the argument checking and the ten error texts" / acceptance clause 5: "Ten refusals produce ten messages through M5... All ten are already written in v1 and port as text, not only as behaviour."
   **Wrong because:** old_versions/first_satellite/src/evaluator/modules_random.cpp has nine `fail()` sites, and they do not map one-to-one onto the draft's ten conditions. Non-number and fractional digit counts share ONE text ("wants a whole number of digits, got …", line 52); negative and past-100000 share ONE text ("draws between 0 and 100000 digits, not …", line 57). The two "unknown" cases — unknown tier and unknown tail segment — produce no text in that file at all: line 40's `random_tier` guard falls through to `return std::nullopt` at line 117, and the message comes from the single generic `fail(span, "no such module function: " + full)` at modules.cpp:58. Meanwhile three v1 texts the draft's list omits do exist: "could not draw N digits", ".range wants two satellite.variable.number arguments", and ".range spans more than 100000 digits".
   **Fix:** "What survives it is the argument checking and v1's nine failure texts in `modules_random.cpp`, plus the shared generic 'no such module function' that today answers an unknown tier and an unknown tail segment alike. Nine texts cover ten refusals because two pairs share a message; whether M5 splits them is this milestone's to decide."

6. **Claim:** "The loop paces at 90 ms, which is QUAD's main loop shape and the whole of what QUAD.md §4 asked `sleep` for."
   **Wrong because:** QUAD.md §4 does not ask for `sleep`; it only lists `satellite.time.sleep` at line 282 among the 71 numbers settling §3 added. The ask — and the number — is in §2, QUAD.md:80-82: "The only use of `<thread>` in all 3029 lines is `std::this_thread::sleep_for` at `quad_main.cpp:260`... What QUAD needs is a **sleep** — `satellite.time.sleep(n)` at `1 9 3`." That is also the only place in QUAD.md that writes the number `1 9 3`, so the citation points away from the sentence that carries it.
   **Fix:** Cite §2: "...and the whole of what QUAD.md §2 asked `sleep` for, where `satellite.time.sleep(n)` `1 9 3` is named against `quad_main.cpp:260`." (Separately: 90 ms appears nowhere in QUAD.md, so under PLAN §9 it needs a source beside it or re-measuring.)

**PROBLEMS.** PROBLEMS. The draft is right about the big structural things and wrong in eight checkable places, three of them load-bearing.

What it gets right, and I checked each: no existing milestone owns any of its twenty numbers — M9 says \"`.bool`, `.number`, `.string` and their methods\" and reaches neither `1 6 3` nor `1 6 7`/`1 6 8`; M10's containers and M12's `satellite.variable.thread` are clear of it; the three parallel drafts (satellite.system + arguments + library.system; file/directory/variable.file; console `1 5 2`–`1 5 9`) are untouched, and it only *uses* M8's `1 5 1` and M9's `1 13 3`. The arithmetic holds: WORD_NUMBERS §2.2 is 218 rows with exactly 3 ALIAS rows, so 215 distinct, and satellite.random is 13 numbers on 16 rows. DESIGN §11 really does say \"Two shapes on each\" against §2.2's four slots, and 095eb49 (2026-08-26) predates 99fa6d9 and cc3f813 (2026-08-27) with neither touching §11. The v1 figures check out against the source: 260 lines in random_numbers, 121 in satellite_number/random.cpp, ~153,600 draws/ms, ultra at ~249M/2000ms and ~310M/3000ms against a [500M, 600M] cap, PCG at 3 headers / 3138 lines and Apache-2.0 only (v1's own plan saying \"MIT/Apache-2.0\" is what is wrong), and LAYOUT.md with no vendored row. On your sharpest question — it does **not** take Sky::decay + Rack::draw as its done-when, it declines them, and the refusal is correct: sky.hpp:176's decay uses no rng at all and quad_core.hpp:56's `persistence` is `0.15 + 0.85·alt²` with no fractional pow, while rack.hpp:61's draw needs a float draw and a seed that satellite.random has neither of. Declining it is what lets this milestone land alone.

The three that matter: (1) it lists `1 7 2` as reserved-and-unbuilt on one page and already-covered-by-M2 on another, and its done-when walks through `1 7 1`–`1 7 3` whichever way its own open question resolves; (2) the M9.5 slot argument is a type confusion — DESIGN §8.1 makes `satellite.variable.number` an exact decimal, so v1's `fast(1.5)` and `.range(1.5, 2)` refusals need no float, nothing here depends on M9.5, and the draft's own \"do not imply a dependency that does not exist\" rule condemns its own placement behind M9.5's undecided rounding rule; (3) two of clause 5's ten error texts are the evaluator's generic \"no such module function\" and become M5's did-you-mean, with the real count being 8 refusals over 6 texts. Also: the ported seed path can SIGABRT and does not use kernel entropy, per v1's own plans/pcg_k16384_spec.md, which the draft never mentions.

Two things outside the draft's scope, found while checking it: SCRATCH.md/MILESTONE.md §0.1's table sums to 121, not the 122 it reports; and PLAN §8's \"Later\" note still says the float \"is now M9\" when it is M9.5, matching DESIGN §13's stale \"M9 owns this.\"

1. **Claim:** "M2's acceptance test already names `1 7 2` outright, so the number uncovered was never 16" — and, later, "Nine of the twenty numbers stay reserved and unbuilt … `1 7 1`, `1 7 2`, `1 7 3` …"
   **Wrong because:** The same document counts `1 7 2` as already covered (to knock down the ledger's 16) and as one of nine numbers this milestone deliberately leaves unbuilt. Worse, neither answer to its own open question survives contact with its done-when. The program runs `satellite.random.ultra(40)` and `satellite.random.fast.range(1, 6)`; the walk goes satellite → random → ultra before arity selects `1 7 10`, so something under `1 7` must carry the tier's identity. If `1 7 1`–`1 7 3` name the tier nodes, this milestone builds them and they are not reserved. If they name the zero-argument call, then v1 already refuses that shape as an arity error (/home/madness/code/cxx/satellite/old_versions/first_satellite/satellite_system/tests/random_test/random_test_surface.cpp:28, `satellite.random.fast()` → "takes 1 argument, got 0"), so this milestone must implement that refusal too — clause 5 lists it — and there is then no number left for the node the spelling `ultra` lands on. Either way the reserved list is wrong, and WORD_NUMBERS §4 already flags the identical shape as unsettled ("Call shapes sit at two different depths, and `words.def` can only encode one" — `include()` is `1 1 0`, a child; `input()` is `1 5 2`, a sibling).
   **Fix:** The open question is a precondition of the number list, not a companion to it. Settle whether `1 7 1`–`1 7 3` are tier nodes or zero-argument shapes — against WORD_NUMBERS §4's language-owned/user-owned rule — before writing either the "twenty numbers" list or the "nine reserved" list, and stop counting `1 7 2` in both columns.

2. **Claim:** "It sits directly after M9.5 on purpose: that milestone establishes the float, and this one has to report that the dice cannot use it."
   **Wrong because:** The dependency does not exist, and the sentence rests on a type confusion. DESIGN §8.1 makes `satellite.variable.number` an exact arbitrary-precision **decimal** — "a bignum significand times a power of ten" — so `1.5` is a number, not a float. Every refusal the draft attributes to the float is a `Number` test in v1: random_test_surface.cpp:38 runs `satellite.random.fast(1.5)` and :45 runs `.range(1.5, 2)`, both landing on `to_integer` / `is_integer` in evaluator/modules_random.cpp. Nothing in the milestone — program or clause 5 — needs `satellite.variable.float`. The draft refuses a post-M10 slot with "putting it past the containers would imply a dependency that does not exist," then does exactly that to M9.5 — and M9.5 is the one milestone in §8 that "cannot land until the rounding rule is chosen," an undecided design question this milestone has no stake in and would now inherit.
   **Fix:** Slot it after M9 (M9.25), where its real dependencies are: M6.5 for the bignum, M8 for `display` and `satellite.main`, M9 for `while`. Delete the M9.5-adjacency sentence or restate it correctly — the dice refuse a fractional **`satellite.variable.number`**, which is DESIGN §8.1's exact decimal and exists at M6.5.

3. **Claim:** Clause 5: "Ten refusals produce ten messages through M5: … an unknown tier, an unknown tail segment. All ten are already written in v1 and port as text, not only as behaviour."
   **Wrong because:** Two of the ten are not random's messages at all, and the count is wrong. random_test_surface.cpp:52-56 shows `satellite.random.quick(4)` and `satellite.random.fast.middle(1, 2)` both answering the evaluator's generic "no such module function" from evaluator/modules.cpp:58 — v1's random arm returns `std::nullopt` ("not mine, keep looking") for an unknown tier, exactly as random_numbers/random.hpp:39 documents. PLAN §7 throws that string dispatch away and DESIGN §4.6 replaces the message with M5's "no `quick` under `random` — did you mean…" over the failing trie level, so those two refusals belong to M2/M5/M6 and cannot port as text. The rest is also miscounted: evaluator/modules_random.cpp has 9 `fail()` sites, and the eight in-scope conditions collapse onto 6 texts — `fast("x")` and `fast(1.5)` share "wants a whole number of digits", `fast(0 - 1)` and `fast(100001)` share "draws between 0 and" — while two real texts go unlisted: ".range wants two satellite.variable.number arguments" and the "could not draw" / "spans more than … digits" failures.
   **Fix:** Rewrite clause 5 as eight refusals through six ported texts: drop the unknown tier and unknown tail segment (name them as M5's did-you-mean, owned elsewhere) and add the non-number-bounds and could-not-draw texts.

4. **Claim:** "an alias cannot be a tree row at all … And M2's static_assert as specified — 'dense from 1 with no holes and no duplicates' — fires on `1 7 5`, `1 7 8` and `1 7 11`. QUAD.md §4 already states the check correctly … and M2 adopts that wording. Only these rows force either change."
   **Wrong because:** The two halves cancel. Once the alias is out of the tree — which the draft's own first sentence establishes — `1 7`'s children are 1 through 12, dense, no holes, no duplicates, and M2's assert passes untouched. Adopting QUAD §4's wording does not fix the firing either, because QUAD's four properties still include "no duplicates"; its fourth property is an *addition* (a check over the spelling table), not a correction to the existing one. And "only these rows force either change" is false for the first change: `satellite.variable.hexadecimal` forces it identically — a tree row for it under `variable` would number `1 6 17`, not `1 6 11` — as do WORD_NUMBERS §2.3's six `arg`/`args`/`argument`… spellings under one node. M3 already names `hexadecimal`, so that half was visible from M3 long before this draft.
   **Fix:** State one finding, not two: an alias is a spelling-table entry rather than a `words.def` row, which `hexadecimal` and the six `arguments` spellings already required and which M3 half-knows. Then add QUAD §4's fourth property as a new check, and drop both the claim that the existing assert fires and the claim that only these rows force it.

5. **Claim:** "The two time methods the first satellite ships are unnumbered — `.minus(t)` and `.nanoseconds()`."
   **Wrong because:** This is the draft's own recurring trap one level down, inside a node it names. v1's time value carries more than two: evaluator/methods_scalars.cpp:159 handles `to_string` on a `Time` before `minus` and `nanoseconds`, and evaluator/help.cpp:160 advertises the surface as "`.minus(t)` -> nanoseconds  `.nanoseconds()`  `.to_string()`" with `.size()` on the next line. So `satellite.variable.time` `1 6 3` has at least three and probably four children to number, not two — and the paragraph whose whole point is "a program can obtain an instant and do nothing whatever with one" undercounts the very thing it is counting. It also silently falsifies WORD_NUMBERS §4's closing claim that "Nothing found by the sweep is unnumbered."
   **Fix:** Enumerate the time surface from methods_scalars.cpp and help.cpp rather than from memory — `.minus(t)`, `.nanoseconds()`, `.to_string()`, and settle whether `.size()` is generic or the type's — and record the contradiction where WORD_NUMBERS §4 is.

6. **Claim:** "What this milestone ports is the other half, `random_numbers/random.{hpp,cpp}` — 260 lines, the tiers, the spun seed, the fold and the watchdog" / "every call spins on kernel entropy and reseeds from the fold"
   **Wrong because:** v1's own measurement retired the kernel-entropy claim and found a crash in the code being ported. old_versions/first_satellite/plans/pcg_k16384_spec.md records that under strace the built `./satl` issues exactly one `getrandom` in the whole process — glibc's 8-byte startup draw — and the 65568 seed bytes come from RDRAND via libstdc++'s default token, never touching the kernel; it names random.hpp:10 and random.cpp:67/86 as the three places that say otherwise. Worse, that path **throws**: libstdc++ retries RDRAND 100 times then raises `std::runtime_error`, and there is no `catch` anywhere under `src/`, so the failure mode of a language builtin is SIGABRT. The recommended fix is `getrandom(2)` flags 0 in a loop — 12.4× faster, errno instead of an exception, `ldd` stays at six. The draft cites the *other* getrandom record (the 256-bit "answer not taken" for a hypothetical secure tier) and misses this one, so a straight 260-line port ports the abort and re-tells the retired sentence.
   **Fix:** Add the seeding blocker to the port clause: the kernel-entropy sentence is false as built and the seed path can abort. Make the from-scratch seeding function return `bool` through the existing `random_digits`/`random_range` bool ladder, and either adopt `getrandom(2)` here or record the refusal. This is a done-when, not a footnote.

7. **Claim:** "which is how sixteen rows of working, ported, tested code came to be sitting in it" and "`satellite.variable.duration` `1 6 8` is in no list at all"
   **Wrong because:** Both overstate in the direction that makes the finding look bigger — the failure this tree's last three commits were about. Of `satellite.random`'s 16 rows only 9 (6 distinct numbers, two shapes per tier) are built in v1; the draft itself says `1 7 6`, `1 7 9`, `1 7 12` are "specified in no document" and built nowhere, and that `1 7 1`–`1 7 3` are open. "Ported" is also the wrong tense — none of it is in this tree yet. And `1 6 8` is in two lists: SCRATCH.md/MILESTONE.md §0.1 counts it among the six uncovered `satellite.variable` leaves and §0.2 names it with its number, while DESIGN §12 carries "**Durations** — `time` is an absolute instant only" in the deferral list.
   **Fix:** "Nine rows, six numbers, working and tested in v1 and not yet ported." And say what is true of `1 6 8`: it is in MILESTONE.md and in DESIGN §12's deferrals, and in no milestone.

8. **Claim:** "`.range` is a second spelling of the two-argument shape … it is a two-segment rewrite performed in the spelling table during the walk" — presented as a finding about M2 and a one-word correction to M3.
   **Wrong because:** The draft says which milestones the alias *breaks* and never says which one *builds* it, while its own done-when clause 2 is entirely `.range`: "`.range(1, 100)` answers both 1 and 100, `.range(7, 7)` is 7, and `.range(-100, -90)` lands inside itself." The mechanism has no home. DESIGN §4.4's spelling table is node→string (deduplication, "two nodes sharing one piece of text"); WORD_NUMBERS §2.3 says the opposite direction must also be held and nothing implements it; M3 owns the lexer's spelling table and M2 owns `words.def`. A milestone cannot make a mechanism nobody builds load-bearing in its own acceptance test.
   **Fix:** Name the owner of the one-node-many-spellings rewrite — M2 if it lives beside `words.def`, M3 if it lives in the lexer's spelling table that already has to know `hexadecimal` — in the same paragraph that claims `.range` for clause 2.

### What the grounding pass found that outranks the draft

These are not drafting errors. They are findings about the **documents**, and
several are corrections to permanent files.

- THE 218-VS-215 DISCREPANCY IS ENTIRELY THIS MILESTONE'S THREE ROWS. I extracted and sorted all 218 number cells from §2.2; the duplicate list is exactly `1 7 5`, `1 7 8`, `1 7 11`. Every count disagreement across four documents — PLAN says 218, DESIGN §12 says 215, WORD_NUMBERS §4 says 215, MILESTONE says 218 — traces to the three .range aliases and to nothing else in the language. The single place that explains it is a scratch file scheduled for deletion.

- MILESTONE.md §0.1's UNCOVERED COLUMN SUMS TO 121, NOT 122. 30+16+11+8+8+8+7+6+6+5+4+3+3+2+2+1+1. The headline, the total row, PLAN §8, and SESSION §5.7 all say 122. The missing path is satellite.variable.time 1 6 3 — the 'satellite.variable leaves' row says '6 | 12' when variable has thirteen leaf children, and the one it omits is mine. Restore it and the column reaches 122 and the headline is vindicated. This is the ledger being right in prose and wrong in its own table, which is the failure mode it was written to catch.

- WORD_NUMBERS.md CONTRADICTS ITSELF ABOUT THIS MILESTONE'S PATHS, AND BOTH SIDES SHIPPED IN THE SAME COMMIT. §1.4: 'see satellite.random.fast.range, which is four numbers and nothing else.' §2.2 and §2.3: it is `1 7 5`, three numbers, an alias. Both were written in 99fa6d9. The four-number reading is a fossil of the plan still visible at WORD_SURFACE.md:132 ('next free is 1 7 1 1'), which really is four numbers — the alias decision superseded it and §1.4's illustration was not updated. The file's own rule settles it in §2.2's favour, and §1.4 needs a different example.

- THE PARENTHESES ON 1 7 1 / 1 7 2 / 1 7 3 ARE THE REWRITE'S, NOT THE AUTHOR'S. SCRATCH.md/WORD_NUMBERS_ORIGINAL.md:27-29 writes `satellite.random.fast  1 7 1` bare. So do DESIGN.md:213 and PLAN.md:694. The rewrite promised only that 'Every number below is preserved unchanged' — a promise about numbers, and the parentheses changed what three of them NAME. And v1 tests the parenthesised source as a specific error: random_test_surface.cpp:29, 'takes 1 argument, got 0'.

- v1 ANSWERED DESIGN §13's CLOCK QUESTION TWICE, ON BOTH SIDES, WITH MEASUREMENTS — AND MEASURED THE AUTHOR'S OWN STATED LEANING AND REJECTED IT. random.cpp:22-28: high_resolution_clock is the same type as system_clock here, is_steady is false, both tick at 1 ns with 21 ns granularity, 'so there is no precision to trade away', and system_clock can be stepped backwards by NTP under a running deadline. Meanwhile modules_file.cpp:176 chooses system_clock for time.now precisely because steady_clock's epoch is unspecified. The resolution of §13's 'cannot both be the one' is that only one of the two is ever a satellite VALUE; the other is a timer nobody names.

- satellite.variable.duration 1 6 8 IS A NUMBER MINTED FROM A DOCUMENTED REFUSAL. The sweep sourced it from 'v1docs'; the v1 source that uses the phrase says 'there is no satellite.variable.duration' and gives four reasons (ast_expr.hpp:67). DESIGN §12 still defers durations today. And WORD_NUMBERS §1.2's never-renumber rule means it cannot now be withdrawn without leaving a hole that PLAN M2's density static_assert rejects.

- satellite.time.new 1 9 2 HAS NEVER EXISTED, AND DESIGN §13 CITES IT AS AN ESTABLISHED PRECEDENT. §13 settles satellite.thread.new by appeal to 'exactly as satellite.file.new and satellite.time.new already do'. satellite.file.new is real in v1; satellite.time.new is not, and v1's stated rule is the opposite of it — an instant 'is read off the clock, never written down as a literal'. The argument still holds on file.new alone, but half its evidence is imaginary. Its only specification anywhere is nine words in a deleted note.

- QUAD's ENTIRE RANDOMNESS IS ONE PRIMITIVE — A UNIFORM DOUBLE IN [0,1) FROM A SEEDED MT19937 — AND satellite.random HAS NO FLOAT DRAW, NO SEED, AND A 50 ms FLOOR PER CALL AGAINST A 90 ms TICK. QUAD.md §2 files this under 'Already decided in satellite, and fine'. It is the exact failure QUAD.md §5 predicts: 'the gap between "the language has floats" and "this expression is writable" is where languages actually fail.' Nobody had opened quad_core.hpp:78 with satellite.random in hand until now.

- Sky::decay USES NO RANDOMNESS AT ALL. sky.hpp contains zero uses of rng; decay at sky.hpp:176-188 is float arithmetic over a live list. QUAD.md §5's candidate mechanism therefore splits cleanly down the middle — decay is M9.5 plus M10, draw is the half that needs a dice this milestone cannot supply.

- M6.5 ALREADY OWNS THE UNIFORM DRAW AND DOES NOT SAY SO. satellite_number/random.cpp is one of the ten files in its port. This is the fourth occurrence of the pattern the brief named, after M3/M4's eleven words and M10's twenty-nine methods, and it is found the same way: read the inventory the milestone points at rather than the milestone's own sentence. SCRATCH.md/PORTING.md:31 has the row; M6.5 does not repeat it.

- THE ONE-IN-TEN LEADING-ZERO FACT IS THE BEST-WRITTEN THING IN THE AREA AND IT SURVIVED THREE COPIES INTACT — v1's design/18, v1's random.hpp:50-53, and DESIGN §11 — each saying it must appear in the header AND the test. It is the model for how the rest of this milestone's surprises should be recorded, and it is the reason I trust the tier documentation more than the tier numbering.

- M2's STATIC_ASSERT AS SPECIFIED WOULD FAIL ON THIS AREA. PLAN.md:679-681 asks for children 'dense from 1 with no holes and no duplicates'; §2.2 has three duplicate numbers, all here. QUAD.md §4 states the check correctly as four properties including 'no alias pointing at nothing'. PLAN M2 has to adopt QUAD's wording, and only this milestone's rows force it.

- THE VENDORED PCG IS THE PROJECT'S ONLY THIRD-PARTY DEPENDENCY AND HAS NO HOME IN LAYOUT.md. Three headers, 3138 lines, Apache-2.0 inside an MIT (Expat) tree, header-only so ldd is unchanged, and reached through -isystem for a load-bearing reason (pcg_extras.hpp:223 warns under -Wall -Wextra and the silent-rebuild property depends on that warning not being ours). LAYOUT.md's file tables have no row for a vendored directory; this milestone creates the first one, and the Apache notice has to travel with it.

### Blockers — decisions only the author can make

- THE CLOCK, AND IT BLOCKS EVERY ONE OF THE SEVEN CLOCK-SIDE NUMBERS. DESIGN §13: satellite.variable.time, .date and satellite.time.now() 'must agree on **one clock and one epoch** before any of them is built.' The document already names the hard half — 'a high-resolution monotonic clock and a wall-clock date are not the same clock and cannot both be the one' — and records the author's leaning toward a high-precision clock. TWO THINGS THE PROJECT DOES NOT SEEM TO KNOW IT ALREADY HAS. First, v1 answered it: system_clock and the Unix epoch, with the reason written at modules_file.cpp:176-180 (steady_clock's epoch is unspecified; the cost is that a clock adjustment can move it; the benefit is that the value means something outside this process). Second, v1 also measured the author's leaning and rejected it, for the other job: random.cpp:22-28 says high_resolution_clock is the same type as system_clock on this machine, is_steady is false, both tick at 1 ns with 21 ns granularity 'so there is no precision to trade away', and system_clock can be stepped backwards by NTP under a running deadline. SO THE ANSWER TO §13's DILEMMA IS THAT THE MILESTONE NEEDS BOTH CLOCKS AND THEY DO NOT COMPETE: system_clock is the VALUE (time.now, an absolute instant), steady_clock is the TIMER (the random spin deadline, and sleep). Only one of them is ever a satellite value, so 'one clock, one epoch' is a rule about the type and not about the implementation. That reading has to be put to the author — it is the shape of an answer, not an answer I can take.

- WHAT 1 7 1, 1 7 2 AND 1 7 3 NAME. Three of the thirteen numbers, and the project holds two incompatible readings: the tier NODE (the author's original note, DESIGN §4's example, PLAN M2's test — all three write it bare) or a ZERO-ARGUMENT CALL (WORD_NUMBERS §2.2's parentheses). Under the second reading nothing anywhere says what range a bare draw is uniform over, and v1 tests that exact source as an arity error. Under the first reading the tier is a path that is not a value, which v1 also tests. Only the numbering's authority can settle it, and the milestone cannot be written until it is: it decides whether the surface is nine call shapes or twelve.

- THE STEP FORMS, 1 7 6 / 1 7 9 / 1 7 12. Three numbers, marked 'assigned', with no specification in any document and no implementation in v1. And the semantics are not obvious, because they collide with the one promise DESIGN §11 makes: .range is 'inclusive at both ends', but (min, max, step) can only reach max when (max-min) is a multiple of step. Is (1, 10, 3) able to answer 10, or does it answer from {1,4,7}? Undecided, and it is exactly the kind of thing DESIGN §13 warns 'will be settled by accident'.

- WHAT satellite.time.new 1 9 2 TAKES. Its only specification in the project is a parenthesis in a deleted scratch file: 'set the arguments for a point in time'. That reads as a component constructor and it contradicts v1's explicit rule that an instant 'is read off the clock, never written down as a literal'. It also cannot be designed independently of satellite.variable.date 1 6 7, since components are what a date is made of. DESIGN §13 leans on time.new as an established precedent ('exactly as satellite.file.new and satellite.time.new already do') when in fact it has never existed.

- satellite.variable.duration 1 6 8 IS A NUMBER MINTED FROM A REFUSAL. WORD_SURFACE.md:157 sourced it from 'v1docs' — and the v1 source that mentions it says it does not exist and gives the reason (ast_expr.hpp:67-70: a duration value 'would first have to answer .plus, .size, its type name and its identity as a map key before it earned a slot in the Value variant'). DESIGN §12 still defers durations today. AND IT CANNOT SIMPLY BE STRUCK: WORD_NUMBERS §1.2 says removing a child 'leaves a hole rather than shifting its neighbours down', while PLAN M2's static_assert demands children 'dense from 1 with no holes'. So 1 6 8 must stay numbered whether or not anything is ever built behind it, and the milestone's job is to say that out loud rather than to build a type two documents refuse.

- satellite.variable.date 1 6 7 HAS A NUMBER AND NOTHING ELSE. No row in DESIGN §8's types table, no representation, no constructor, no methods, no v1 code, no sentence longer than a list item. It is unbuildable as specified and the milestone should either carry the specification work explicitly or leave the number reserved and say so.

- THE TWO TIME METHODS v1 SHIPS ARE UNNUMBERED. .minus(t) and .nanoseconds() (help.cpp:45, methods_scalars.cpp:166 and :183). satellite.variable.time 1 6 3 (0) has zero children in §2.2 while its three hand-written siblings have 16, 7 and 14. I must not invent 1 6 3 1 and 1 6 3 2; WORD_NUMBERS has to. Until it does, a program can obtain an instant and cannot do anything with one — which also means the milestone cannot measure its own tier floors in satellite.

- THE UNIT OF satellite.time.sleep(n). Milliseconds, nanoseconds, or a duration? QUAD's call is sleep_for(milliseconds(90)). v1's one spelled unit is `100ms`, converted to nanoseconds at parse time — a literal PLAN §7 throws away. If sleep takes a bare number the unit is invisible at the call site, which is precisely the readability failure DESIGN §1.1 exists to prevent.

- THE MEASURED CONSTANTS ARE ANOTHER MACHINE'S, AND ONE OF THEM IS LOAD-BEARING. Under PLAN §9 the tier windows are a design choice and stand, but the figures that make the WATCHDOG a failsafe rather than a mechanism are measurements from the Xeon E5-2670 v3: throughput ~153,600 draws/ms, ultra drawing ~249,000,000 at 2000 ms and ~310,000,000 at 3000 ms, against a cap drawn from [500,000,000, 600,000,000] — 'about 1.6-1.9x above ultra's worst case'. IF THIS MACHINE IS ~1.7x FASTER, ULTRA REACHES THE CAP AND THE WATCHDOG FIRES IN ORDINARY OPERATION, which silently shortens the spin and turns a failsafe into part of the mechanism. That is a re-measurement this milestone must do before it can claim the tiers behave as documented, and PLAN §9's 'verify through the real code path' says the way to find out is to run it, not to reason about it. v1 also records the refinement not taken (per-tier caps, ~50M for fast, ~150M for normal) which is the fix if the margin has gone.
