# satellite — the plan

**This file is permanent**, and it is the second satellite's plan. It says how the
language gets built: the architecture, the build, the install, the milestones, and
what is carried over from the first satellite against what is deliberately left
behind. [DESIGN.md](DESIGN.md) is the other half and says what the language *is*.

It supersedes `PLAN_ONE.md`, which was written on 2026-08-25 before any of the
second satellite existed and was always meant to be thrown away. Everything in it
worth keeping is here. **`PLAN_ONE.md` may be deleted as soon as nothing cites it**
— it is not a reference, it is a draft that has been superseded.

The first satellite lives at `old_versions/first_satellite/`, is not going anywhere,
and is the source this is pulled from. It works and it is fast.

---

## 0. What this is all for

[QUAD.md](QUAD.md) names the program satellite has to be able to express:
`/home/madness/code/cxx/quad_infinity/`, 3029 lines of C++ that run a model of a
mind. It is a general-purpose language and everything below is meant generally, but
that program is the acceptance test.

**Both sides bend, and QUAD bends more** — QUAD.md §0 is that rule, and it is what
keeps the language from being shaped by one program's C++ conveniences. A feature
enters satellite only when QUAD's *meaning* cannot survive without it.

*(2026-08-27.)* QUAD.md §3 used to list four unsettled things — floats, a set,
sorting, and a console that could address a screen. **Reading the source closed
three of them and one other**: sorting needs one primitive rather than comparators,
so DESIGN §12's deferral of *a bare name can be a value* survives and §2 stays shut;
the sets and deques are membership tests and bounded ring buffers; the display never
moves a cursor and the real gap was non-blocking input; and QUAD's map keys all fall
inside §6.5's restriction. **`satellite.variable.float` is the one that grew teeth**
and is the whole remaining gap (QUAD.md §3.1, DESIGN §13).

None of it touches M2 through M10 as *code* — but settling it added **71 numbers** to
WORD_NUMBERS.md, taking §2.2 from 144 entries to 215 and eventually 222, and **M2
transcribed all of them on 2026-08-28.** The rest lands on M11 and M16, and QUAD.md §4 proposes a milestone
that does not exist yet: **one mechanism out of `mind.hpp`, running.**

## 1. Where things stand

**Milestone 1 landed 2026-08-26, M1.5 on 2026-08-27, M2 on 2026-08-28, M3 on
2026-08-29, M4, M4.5, M5 and M6 all on 2026-08-30, M7 and M8 on 2026-08-31,
M8.5 and M9 on 2026-09-01, M10 on 2026-09-02, M11 and M12 both on
2026-09-03, M13, M14 and M15 all on 2026-09-04 — M13 the day its blockers
cleared, which no earlier milestone can say, and M14 behind it the same
sitting, the day the author took its six open answers — M16 and M17 both
on 2026-09-06, M18 and M19 both on 2026-09-08, M19.5 across 2026-09-08 and
2026-09-09, and M19.6 on 2026-09-09.** **M20 LANDED 2026-09-11** (`17ea781`), six commits in
one day — the largest single milestone this list has had by paths. §8's entry
carries every decision as it was argued and `MILESTONES/M20.md` is the review:
the arguments object and its ten selectors, `satellite.system`, a word for
reading a live code, and a watchdog floor a running program can move.

**M21 LANDED THE SAME DAY** — `MILESTONES/M21.md`, two commits — and it is the
only milestone in this list whose content is a *program* rather than a
mechanism: **`Sky::decay` and `Rack::draw`, two pieces of QUAD, written in
satellite by hand and refereed against the C++.** QUAD.md §4 had asked for it
since its first draft. `Sky::decay` matched an independent exact-decimal
referee on **40 of 40** activations where the C++ double matched on 6 of 40;
all four of `Rack::draw`'s annealing exponents are exact in satellite and none
in the double, and all twenty of its wheel choices are identical. Its two
blockers both turned out to be numbers and the author cleared both:
`satellite.random.seeded` `1 7 13`–`1 7 16`, a tier that does not spin and the
only fractional draw in the language, and
`satellite.variable.float.to_string()` `1 6 10 1`, without which `Sky::save`
could not be written at all. **And it produced the list QUAD.md §5 predicted it
would** — six things the language still cannot say, four of them still open.

**THE NARRATIVE BELOW STOPS AT M17 AND THE LIST ABOVE DOES NOT.** Everything
from M18 on is in §8 and in `MILESTONES/`, and this section was not extended
with it — which is worth saying here rather than leaving a reader to notice:
`SCRATCH.md/SESSION.md` sends people to this section for *where things stand*,
so a §1 that quietly ended at M17 was pointing them at a frontier four
milestones behind the tree. **satellite.help exists (M18), files and
directories are open and read (M19), binary and hexadecimal are values
(M19.5), `satellite.system` answers the machine (M20), and two pieces of QUAD
run and are held to the C++'s own numbers (M21).**
*(M1.5 is the window, and it
was called M11.A and counted as unlanded until the 2026-08-30 renumber found it
had been finished for three days — §8's opening carries the whole mapping.)* There is a `satl` that says what it is, says how a
file will be run, refuses to pretend about the parts that do not exist, **holds
the whole numbering and can be asked about it** — `satl --words` — reads a file
into tokens — `satl --tokens` — **parses one and prints it back** — `satl
--unparse`, the first command in this tree that answers *in satellite* — and now
**caches one to the disk with its words as numbers and reads it back**:
`satl --satc`, which is the first command that leaves anything behind it. **And
since M6 it holds itself to a machine** — `satl --limits` says how many threads
it may use, how much memory it may take, and where each of those answers came
from, and a thread pool and a memory watchdog start on every run. **M7 gave
every name a slot and every path a number** — `satl --resolve` — and **M8 gave
the language a number type**: `satl --number 0.1 + 0.2` answers `0.3`, and
`satl --number 1 / 3` answers thirty-four digits and says it rounded them. **And since M8.5 no walker in the tree has a depth of its own** — the parser, the
resolver, the printer and the `.satc` writer each keep their stack on the heap, so
100,000 nested brackets answer where 19,000 used to segfault. **And since M9
there is an interpreter**: the arena AST compiles to a closure tree, the machine
that walks it keeps its stack on the heap, and `satl --call
example/frames.satl factorial 10` answers 3628800 — a capsule 1,000,000 frames
deep answers too, at the 8 MiB a login shell hands out, and a runaway one is
refused in words about recursion with exit 4. `satl --compile` prints the closure
tree and says which parts of DESIGN §6's grammar do not run yet, naming the
milestone for each. **And since M10 satellite runs**: `satl file.satl` starts a
program at `satellite.main()`, prints through `satellite.console.display` — a
console with a printer thread of its own, joined in four steps at the end of the
run — and ends at `satellite.return(satellite)`. That is the milestone at which
this language executes anything at all, and everything from M11 on depends on it
for the reason every milestone after M1 depends on there being a binary: without
it there is nothing to print through.

**Its acceptance files are `example/console.satl` and `example/bare_main.satl`,
written by the author on 2026-09-02**, and they are the first two programs in
this tree that run. `console.satl` prints `hello!` and returns the runtime;
`bare_main.satl` prints nothing and returns the runtime, which is not a lesser
copy of the first — a program that prints nothing still takes the shutdown path,
and a barrier that waited on a printer nobody started would hang every one of
them.

**What M10 will NOT run is any program written before it, and that is the
boundary rather than a shortfall.** All five of the older ones declare
`satellite.main`'s parameter, which DESIGN §3 settles as an empty
`satellite.container.list` and therefore **M16's**; each is answered with a caret
under the word `arguments`, the milestone that will bind it, and exit 3. Hello
world is M17.

**And since M11 the language has scalars and control flow.** `if`, `else`,
`while` and `for` run — their machinery landed with M9's machine, and what M11
added inside them is the statement boundary Ctrl-C lands on: v1's SIGINT
handler ported whole (no `SA_RESTART`, §6's hard-won note), a fourth `Ending`
that is neither a wrong program nor a machine limit, exit 130 with S0730's
caret under the statement that did not run. `satellite.bool.true` and
`.false` answer as DESIGN §6.1's module constants; the string's sixteen
methods and the number's fifteen sit behind `handlers[path_id]` with the
receiver as argument 0 (`power`, `truncate` and `sqrt` refuse naming M15's
rounding rule — and answer since M15 landed — `split` naming M16's list); `append` and `clear` write back to
the slot the method was called on, which is DESIGN §6.4's storage-slot rule
enforced by the op. **Its acceptance file is `example/scalars.satl`** —
fourteen lines, every statement form, a method from each family — and
[MILESTONES/M11.md](MILESTONES/M11.md) carries the decisions (positions count
from 0; where another language hands back a sentinel, satellite refuses, and
the argument is that a refusal can loosen at M12 while a `-1` is forever) and
the two findings worth reading before touching words.def: the paren
annotations count two ways between the string and number families, and the
selector fold now tries both counts.

**And since M12 "nothing" is a thing a program can ask about.** The blocker
only the author could take — *is "nothing" a state every type has, or a value
only a `variant` can hold?* — was delegated whole on the day it landed and
answered: **a state every type has**, with `satellite.variable.variant` as the
type whose vocabulary can name it. DESIGN §8.7 is the language's statement,
[MILESTONES/M12.md](MILESTONES/M12.md) §2 the argument. The variant's
representation is the `Value` itself — no arm, no handle, §8.2's forty bytes
untouched — and its four methods are the numbering's first appended CHILDREN,
`holding` `1 6 14 1`, `holds(x)` `1 6 14 2`, `held` `1 6 14 3` and `clear`
`1 6 14 4`, assigned by the walk of **its acceptance file
`example/variant.satl`** — nine lines, one box, three states, a different true
answer at each. `satellite.variable.expression` `1 6 9` stays numbered and
unbuilt, and saying so was the work. And every method refusal in the language
now names its method: the machine's `text_of` answered a call's `(` token
until M12's done-when demanded the refusal be BY NAME, so M11's sentences
healed with it.

**And since M17 DESIGN §3 runs, which is the program this language was described
with.** `satl example/hello_world.satl` prints `Hello, World!` and exits 0 —
the console is M10's, the empty `satellite.container.list` its parameter binds
to is M16's, and what was left for this milestone was **saying what the
parameter hands over, and the two of its own four paths that had never
answered.** `satellite.include()` `1 1 0` did not parse, against a WORD_NUMBERS
§1.3 that uses that exact form to teach what a trailing zero is; and
`satellite.include(cargo)` — the bare identifier, which is the spelling both
DESIGN §3 and WORD_NUMBERS §2.2 use for a spaceship — answered **S0511,
"nothing called `cargo` is in scope here"**, while the same include written
with a string or a path answered correctly with M25. **Neither was a hard
problem and both were invisible**, because the only program anybody ran through
them was hello world, which writes the third spelling. `example/hello_world.satl`
is the done-when, and **DESIGN §3 is now held to it by a test** rather than by
the intention to keep a copy — [MILESTONES/M17.md](MILESTONES/M17.md) §2 is
that argument and §3 is what building it found.

*(§1's running account below stops at M12, and this paragraph is the first
since. M13, M14, M15 and M16 landed without one — the notes in
[MILESTONES/](MILESTONES/) are what they have instead, and the table there is
the index. Said rather than backfilled: four milestones of narrative written
after the fact by somebody who did not build them would read like a record and
be a reconstruction, and this file's own recount paragraphs already argue that
a stale figure is worth more dated than quietly replaced.)*

What exists: the `Makefile` as an index over eleven fragments under
`make_support/`, **223 C++ files totalling 35,948 lines** plus `words.def` at
558 and `errors.def` at 702, **ten** test suites under `tests/`, and
`satellite_enterprise/`, the Enterprise Linux installer and the artwork.
*(Recounted 2026-09-03 again, at M12, over `src/` and `tests/` together — 150
files and 24,888 lines of it is `src/`. The two new files are
`satellite_scalars/variant_methods.cpp` and `tests/eval_test/variant.cpp`; the
figures at M11 were 221 files and 35,501 lines, with `src/` at 149 and 24,651,
`words.def` at 551 and `errors.def` at 688.)*
*(Recounted 2026-09-03, at M11, over `src/` and `tests/` together — 149 files
and 24,651 lines of it is `src/`. The ten new files are one whole module,
`satellite_scalars/`, plus `system_facts/interrupt.*`,
`evaluator/operations_dispatch.cpp` and two new sections of `tests/eval_test/`.
The figures at M10 were 211 files and 33,622 lines, with `src/` at 141 and
23,210, and `errors.def` at 630.)*
*(Recounted 2026-09-02, at M10, over `src/` and `tests/` together — 141 files
and 23,210 lines of it is `src/`. The twelve new files are one whole module,
`satellite_console/`, plus `programs/run_command.*`, `programs/built_program.*`
and the four of `tests/console_test/`. The figures at M9 were 199 files and
32,121 lines, with `src/` at 133 and 22,328.)*
*(Recounted 2026-09-01, at M9, over `src/` and `tests/` together — 133 files and
22,328 lines of it is `src/`. The thirty new files are two whole modules:
`satellite_value/` and `evaluator/`, plus `system_facts/user_facts.cpp`,
`programs/evaluate_commands.*` and the eight of `tests/eval_test/`. The figures
at M8.5 were 169 files and 27,403 lines, with `src/` at 111 and 18,766; at M8,
164 and 25,830 with `src/` at 108 and 17,592.)*

*(The figures here read "120 C++ files totalling 18,031 lines … six test suites"
until this recount and were taken at M6, so they had already missed M7. Left
recorded rather than quietly replaced, for the reason the paragraph below gives.)*

*(The figures here read "eighty-two C++ files totalling 12,384 lines … four test
suites" until this recount, and they were taken at M4.5 — so they had already
missed M5 as well as M6. The count immediately before M6 was **98 files and
15,106 lines**, which is the number this paragraph should have carried. Left
recorded rather than quietly replaced, for the same reason the 2026-08-28 figures
below are: a count in prose goes stale the day after it is taken, and what makes
one worth keeping is that it is dated.)*

**The largest C++ file is `src/machine_limits/config.cpp` at 487**, then
`src/machine_limits/limits.hpp` at 438, `src/parser/parser_statements.cpp` at
416, `src/parser/parser_expressions.cpp` at 396,
`src/parser/parser_declarations.cpp` at 377 and
`src/name_resolver/names.cpp` at 367. *(Recounted at M10. The list did not
change hands: `machine_limits` has been the widest module in the tree since M6
and the two files at the top grew again. **M10's own widest file is
`src/programs/run_command.cpp` at 208** and its next is
`src/satellite_console/console.cpp` at 195, both well under §3's target, which
is what a milestone that adds a module rather than a pass looks like.)* *(Recounted at M8.5. The three parser
files grew because a machine says out loud what a recursion said by where a call
sat — in CODE they are 296, 252 and 245, which is at the target; §3's rule is
about the shape a file comes out in. `unparse.cpp` left this list by being split
three ways, which MILESTONES/M8.5.md §3.6 argues.)* *(The two
`machine_limits` files at the top grew again in M8's follow-up pass — 435 to 470
and 383 to 399 — which is where `division_digits` got the range that let its
silent clamp be deleted.)* *(Recounted at M8. `main.cpp` is no longer on this
list at all — it is 310, down from the 427 the paragraph below is about, because
M7 split three more arms out of it on the same seam. The two `machine_limits`
files at the top are where M8 added `division_digits()`, and the module has been
the widest in the tree since M6.)*

**M8's own widest file is `src/satellite_number/bignum_number.hpp` at 328**,
which is twenty-eight lines over §3's target and is named here rather than left
to a future audit — 306 when the milestone landed, and the follow-up pass added
the two shift declarations and the note where the division ceiling used to be. It is a header whose comment mass IS the milestone — DESIGN §8.1's
sign, what it cost, what it paid for and the `static_assert` that holds the
layout — and §3's rule is a target to write toward rather than a ceiling that
fails a build. The other nine ported files are between 48 and 301. *(This paragraph named `unparse.cpp` as
the largest at M4 and that was wrong on the day it was written — `main.cpp` was
380 then, and a count that skips the one file everybody edits is the count most
likely to go stale. M4.5 took sixty lines off it by giving `--satc` a file of its
own, which is why it was 348 rather than 440.)* MILESTONES/M4.md §6 names
`unparse.cpp`'s seam and says why it was not taken, which is the answer M3 gave
for `lexer.cpp` and M2 for `authority.cpp` before it.

**`main.cpp` went from 348 to 427 at M6 and is now 127 over the target, which is
the widest this tree has run.** M6 split `programs/limits_command.cpp` out of it
on the seam M4.5 used for `--satc` and M5 for `--check` — the flags whose work is
a module's — and that took the growth from +79 to +49 rather than removing it,
because what remains is the two arms themselves and the `only_prints_and_exits`
list they joined. MILESTONES/M6.md §6 names the seam that WOULD take it back
under 300 and says why M6 did not take it: it is a reshaping of M2 through M5's
arms and not of M6's.
[LAYOUT.md](LAYOUT.md) lists all of it.

*(The figures this paragraph carried on 2026-08-28 — twenty-five files, 3,148
lines, `words.def` at 542, `authority.cpp` at 261, `words_invariants.hpp` at 244
— were **already wrong when M2 was reviewed**, and M2.md §6 item 4 recorded the
drift rather than fixing it. Four of the five were stale by between 4 and 38.
A count in prose goes stale the day after it is taken; what makes this one worth
keeping is that it is dated, so a reader can tell how far to trust it.)*

*(Recounted 2026-08-28. This paragraph said "five C++ files… the largest C++ file is
137 lines", which was true of M1 alone and stopped being true the next day.)* M1's
five are `src/system_facts/version.hpp` (76), `src/programs/opening.hpp` (45) and
`.cpp` (68), `src/programs/main.cpp` (137) and `src/programs/cpu_level.cpp` (133).
**Five more landed ahead of their milestones on 2026-08-27** —
`src/programs/terminal.{hpp,cpp}` (35, 236) and `window.cpp` (209), which are
`satl-term` and M1.5, and `src/satellite_random/random.{hpp,cpp}` (106, 108),
which is `satellite.random` and has no milestone at all. **The largest C++ file is
`terminal.cpp` at 236 lines**, inside the 300-line rule and the number to watch.

Two things came out different from the **first satellite**, and both were right:
`--help` exits 0 rather than 2 (a request correctly made is not an error), and that
satellite's claim of "119 shared objects" turned out to be **78 on this machine** and
is not repeated anywhere. Neither was a change against `PLAN_ONE.md`, which had
already called both.

**M2 cost nothing measurable at startup**, which §4.3's floor exists to check.
Measured on this machine 2026-08-28, best of seven runs of 200 invocations, all
three binaries on the same filesystem: a bare `int main(){return 0;}` at
**1.59 ms**, `satl` built from M1's sources at **1.60 ms**, this `satl` at
**1.61 ms**, and a repeat of the bare binary at 1.58 ms — so the noise floor is
about 0.02 ms and the M1-to-M2 difference is inside it. That is what the encoding
buys: the tables are `constexpr` and land in rodata, so nothing runs before
`main()` and a program that never asks about a word never pays for one.

*(The first attempt at this measurement put the bare binary in `/tmp` and the
others in the tree, and reported `satl` as **faster** than an empty `main` — a
result that cannot be true and was a different filesystem rather than a finding.
Recorded because §9's rule is to measure on this machine, and a measurement whose
setup differs between arms is not one.)*

**Next: milestone 10**, the console and the first program that runs (§8) — the
printer thread, `satellite.main`, `satellite.return` and `satellite.console
.display`. **Everything it dispatches through landed at M9**: `Value`, the
closure tree, `handlers[path_id]` and the inline caches, so what M10 adds is the
first rows in a table that is built and empty.

*(M9 landed 2026-09-01 and [MILESTONES/M9.md](MILESTONES/M9.md) is the review.
Its precondition was M8.5 — §2.6 puts the explicit control stack between the
arena and closure compilation, so M9 emitted onto a stack that already existed.
That held, and what it did not predict is that the COMPILER would be a fifth walk
needing the same treatment: M9.md §4.1 has it, and the general form, which is
that no walk over user-controlled depth may use the C++ stack from its first
commit.)*

**M6 landed 2026-08-30** and it answered §4.5.4's three open questions rather
than leaving them, because a milestone whose done-when needs an answer cannot
land without giving one. It lands **before resolve** because three later
milestones read something it builds and none of them said so until 2026-08-28: M8's
`Number` reads `division_digits`, M9 derives its recursion ceiling from
`RLIMIT_STACK` and its `Str` needs three live machine facts at decode time, and
M10's printer thread is the pool's first tenant. *(The third reason is wrong and
was wrong the day it was written — corrected 2026-09-02, §4.5.1. M6 still lands
before resolve on the first two, which are the two that were checked against code
rather than against a sentence.)*

*(That milestone was called **M14** until 2026-08-30, and it is the reason the
numbers were put back into build order that day: it had been the next thing to
build since 2026-08-28 while the last finished milestone was M5, and a plan whose
next step is nine numbers past its last finished one needs a decoder to read.
§8's opening has the argument and the old-to-new table.)*

### 1.1 The finding this whole plan hangs off

Read the first satellite end to end and one thing shapes everything else:

**It already built the number table, and the evaluator never read it.**

`src/bytecode_format/format.def` holds **107 word entries** (ids running to 110, so
three are spent and unused) and **29 paths**, with frozen ids and static_asserts
enforcing density, ascension and uniqueness. It is careful work. *(Counted
2026-08-26; `PLAN_ONE.md` said 108 and 30.)*

Meanwhile the actual dispatch for `satellite.console.display(x)`, the most-called
thing in the language, is: flatten the member chain into a `vector<string>`,
heap-allocate a joined string **on every module call including the arms that never
read it**, then try seven arms in a **load-bearing order** — the file says so, "the
arms are not disjoint" — each doing `full == "satellite…"` or
`path[0] == "satellite" && path[1] == "…"` string comparisons, with `module_console`
tried **sixth** and the free function `search_threshold_call` seventh.

So the fastest identity the language had was compiled into a header, asserted over,
documented at length, and then the hot path compared strings.

Frames tell the other half of the story. Resolve turns local names into integer slot
indices statically, once, before anything runs — and it is the fastest part of the
interpreter, measured at 7.1× (DESIGN §7.2).

**Names got numbers and got fast. Paths got numbers and stayed strings.** The whole
thesis of the second satellite is: finish the job the numbering started. DESIGN §4
is that design.

---

## 2. Architecture

### 2.1 The options considered

Bytecode was ruled out by the brief — meaning the *machinery*: an instruction
stream, a decode loop, a compile step. **The part of a bytecode that earns its
keep is kept**, and DESIGN §4 is where it went: every language-owned operation has
a small integer, so dispatch is an array index. What remains:

| | approach | verdict |
|---|---|---|
| 1 | naive tree-walk (`variant` visit, `shared_ptr` children) | what v1 is — the baseline we are beating |
| 2 | **closure compilation** — walk once, emit a tree of callables with every static decision baked in | **adopt** |
| 3 | **flattened arena AST** — nodes contiguous, children by `uint32_t` index | **adopt** |
| 4 | **self-specializing nodes / inline caches** | **adopt, on call sites** |
| 5 | explicit control stack (CEK) — pausable, resumable | **defer** — §2.5 |
| 6 | graph reduction | no — for lazy functional languages |
| — | copy-and-patch (pre-compiled machine-code stencils) | out of scope — it is a compiler |

### 2.2 The arena, first

The first satellite's tree is `shared_ptr<const Expr>` with `sizeof(Expr) == 96`,
guarded by a static_assert. That means:

- a **cache miss per child**, since children are wherever the allocator put them
- **96 bytes per node**, most of it paid by every node to fit the widest alternative
- a refcount on every node that **buys nothing**: the tree is immutable, lives as
  long as the program, and nothing ever frees a node early

*(Corrected 2026-08-26.)* This list used to claim an atomic refcount touch per node
visit and cross-thread contention from it. Neither is true of the first satellite:
`eval` takes `const Expr &` and `exec` takes `const Stmt &`, and every descent
dereferences rather than copying the handle, so the walk touches no refcount at all.
The layout costs are real; the atomic ones were not, and a plan that beats a
strawman cannot tell you whether it won.

An arena of PODs indexed by `uint32_t` fixes all three. Multi-threaded walking
becomes **atomic-free, not merely safe** — which matters because
`satellite.variable.thread` is on the roadmap (DESIGN §10.5).

It also kills a documented data race. `Name::slot` is `mutable int` on a
`shared_ptr<const Expr>`, and the comment holding the race off reads: *"resolve()
must finish, on one thread, before any evaluation begins."* With an arena and a
separate resolved side-table indexed by node id, that race is **structurally
impossible** rather than documented.

### 2.3 Closure compilation, and why it is not bytecode

**One thing here *is* the bytecode, and it is not this.** DESIGN §4's numbering does
the job an opcode table does — every language-owned operation gets a small integer
so dispatch is `handlers[path_id]`, one array index. What satellite never builds is
the *rest* of a bytecode VM: the linear instruction stream, the decode loop, the
serialised format, and the compile step a user waits for. The numbers exist before
any program does, because they belong to the namespace rather than to a program.

With that said, closure compilation:

- no linear instruction stream
- no opcode decode loop
- **no serialised form of the closure tree** — it is built in memory every run
- **no compile step the user ever runs or waits for**

*(Corrected 2026-08-27.)* This list used to say "no serializable format" flat, and
that is no longer true: [SATC.md](SATC.md) specifies `.satc`, a written-down form
of a program with its language-owned words replaced by their numbers. The
distinction the line was reaching for is real and now stated precisely — what is
never serialised is the **closure tree**, because writing that down would freeze an
implementation. `.satc` serialises the layer above it, which is just the source
with DESIGN §4's dictionary already applied, and it is a cache: deleting every one
of them costs a walk and nothing else.

The closure build is the same pass as resolve, measured in microseconds, and it
happens between "parsed" and "running" exactly the way resolve already does. From
outside, `satl file.satl` is as interpreted as it ever was.

What it buys: every decision that *can* be made before execution *is*. Which frame
slot. Which PathId. Which handler. How many arguments, already checked. At runtime a
node is one indirect call with no tag test and no re-resolution. 2–5× over a naive
walk is the usual figure — **to be measured here, not quoted.**

**BUILT AT M9, AND THE 2–5× IS STILL NOT MEASURED — WHICH IS WORTH SAYING RATHER
THAN LETTING IT LOOK LIKE IT WAS.** What M9 measured is §2.5's figure, the cost of
the explicit control stack, because that is the one a decision was waiting on.
This one is a different comparison: a closure tree against a naive walk over the
AST, and there is no naive walk in this tree to compare against — M9 never built
one, because building a second evaluator to be slower is only worth it when the
number decides something. Nothing is waiting on this one. It stays quoted and
unmeasured, and is marked so, which is §9's rule applied to a figure that has not
earned its measurement rather than to one that has.

**One thing §2.3 predicted exactly.** "An op is one indirect call with no tag
test" is what `evaluator/closure.hpp` keeps: an `Op` is a function pointer and
four payload words in 24 bytes, which is `ast.hpp`'s `Node` with the kind
replaced by the address it would have jumped to. MILESTONES/M9.md §3.1.

### 2.4 Inline caches

A `Call` node caches the resolved PathId and handler pointer on first execution,
behind a guard. This is what permanently retires the seven-arm chain of §1.1: the
second execution of a call site does no lookup at all.

**BUILT AT M9, WITH ONE THING SHARPENED.** The CELL is allocated when the op is
emitted rather than on first execution, and it lives in a side table indexed by
call site. That keeps the op arena immutable and shareable, which is what DESIGN
§10.5's threads need: one arena, one cache vector per run. Filling it is still
first execution and the guard is still a guard.

### 2.5 The explicit control stack, un-deferred

**THIS SECTION WAS "Why the explicit control stack is deferred, not dropped" AND
THE AUTHOR REVERSED IT ON 2026-08-31.** The old text is kept below the line,
because the argument it makes is still the argument — what changed is the
conclusion, and one of the two reasons for it turned out to be about a defect
rather than about a cost.

**The rule is that the language has no depth limit** (DESIGN §7.5, rewritten the
same day), and a walker that recurses on the C++ stack cannot keep it. Not
"cannot keep it cheaply" — cannot keep it at all: the depth at which it dies is
`ulimit -s` divided by a frame, which is the shell's decision and nobody's
design.

**What the deferral was buying was not what it was spending.** The old text
defers the CEK machine and then says to *bound the recursion depth from M9 onward
so deep recursion produces a clean error rather than a segfault* — a bound
INSTEAD of a stack, which reads as the cheap 90%. It is not, for two reasons this
project found out by measuring on 2026-08-31:

1. **The segfault it promises to prevent is already here, in four passes that
   have nothing to do with M9.** `satl --unparse` dies at 19,000 nested brackets,
   `--satc` at 20,000, and the parser at 32,000; forty thousand nested blocks kill
   every command in the tree. A bound at M9 would have fixed none of them, because
   none of them is the evaluator. `SCRATCH.md/NO_LIMITS.md` has the table.
2. **A bound is not a cheaper version of a stack, it is a different product.** It
   converts a crash into a refusal, which is worth doing and is not what was
   asked for. DESIGN §1.1's tie-breaker is *do absolutely everything for the
   user*; refusing a program because the machine's stack is 8 MiB is not doing
   everything, it is doing 8 MiB.

~~**The cost figure still has to be measured before it is quoted again**~~
**— MEASURED AT M8.5 FOR THE STATIC PASSES AND AT M9 FOR THE EVALUATOR, WHICH IS
THE HALF THIS SECTION WAS ACTUALLY ARGUING ABOUT.** §2.5's own sentence was why:
*the figure usually quoted is 2–3×, and that one is borrowed rather than
measured, which by §9's own rule means it decides nothing here until this project
measures it.*

**IT IS A RANGE AND NOT A NUMBER, AND WHAT DECIDES IT IS HOW MUCH WORK SITS
BETWEEN TWO PUSHES.** Measured 2026-09-01 with two evaluators over the same
closure arena, the same values, the same arithmetic and the same dispatch, so the
only difference is where the intermediate state lives:

| workload | control stack | C++ stack | |
|---|---:|---:|---|
| a tight arithmetic loop, no calls | 107.0 ms | 30.0 ms | **3.5×** |
| long expression chains | 107.3 ms | 36.5 ms | **2.9×** |
| 200,000 calls that each return | 120.1 ms | 45.6 ms | **2.6×** |
| **a program shaped like a program** | 343.1 ms | 258.3 ms | **1.3×** |
| 20,000 frames deep | **answers** | **SEGFAULT** | — |

So 2–3× was a fair thing to have borrowed and it is wrong at both ends. **The
last row is why it is paid everywhere rather than only where it is needed**: the
recursive arm dies between 15,000 and 20,000 frames at the default `ulimit -s`,
and where a program will need depth is not knowable before it runs. A hybrid that
recursed until depth N would be two evaluators that must agree forever, with N a
number satl invented — which §4.5.4 refuses about `MEMORY_MAX` and
`SCRATCH.md/NO_LIMITS.md` §4.1.1 refuses about the 8 MiB, in the same words.
MILESTONES/M9.md §6 has the method and the fifth of it that came off when it was
looked at.

**And the static passes are not the hard part.** §2.5 called the CEK machine
"genuinely hard to write" and that is true of the EVALUATOR — it has to pause
mid-call and resume. A resolver, a printer and a `.satc` writer walk a finished
tree for effect, and an explicit worklist over an arena of PODs is the shape
`satellite_cache/paths.cpp` already uses for exactly one chain. Those four come
first and M9 inherits a tree that cannot crash under it.

---

*The original section, 2026-08-27 to 2026-08-31, kept because its first paragraph
is now a list of things the fix DELIVERS rather than things a deferred machine
would have bought:*

> A CEK machine would make execution **pausable and resumable**, which is what you
> want for green threads, generators, a stepping debugger, Ctrl-C at an arbitrary
> point, and driving the interpreter from a GTK idle callback with no second thread.
>
> It also costs against direct recursion — the figure usually quoted for it is 2–3×,
> and **that one is borrowed rather than measured**, which by §9's own rule means it
> decides nothing here until this project measures it. It is genuinely hard to write.
> So: not now. But **bound the recursion depth from M9 onward** so deep recursion produces a
> clean `capsule call too deep` error rather than a segfault. DESIGN §7.5 has the
> numbers, and the first satellite's `system_facts/stack_facts.cpp` is half the
> machinery already.

### 2.6 Order of adoption

**Arena first** — it is a data-layout decision and everything else rides on it. Then
closure compilation. Then inline caches. Doing them in the other order means doing
the arena twice.

**And the explicit control stack now sits between the arena and closure
compilation** *(2026-08-31, when §2.5 was un-deferred; all three adopted by
2026-09-01, the arena at M4, the stack at M8.5 and closure compilation with the
caches at M9)*, which is the same argument one adoption later. The four static passes — resolve, the unparser, the
`.satc` writer and the parser — walk the arena and must stop using the C++ stack
to do it; closure compilation then emits onto a stack that already exists rather
than growing one afterwards. **Doing them in the other order means doing the
evaluator twice**, which is the sentence above with a different noun in it.

The arena is what makes this cheap and it is worth saying why: a node is a
24-byte POD indexed by `uint32_t`, so a walker's own stack is a
`std::vector<uint32_t>` and not a stack of visitor objects. That is a property
§2.2 bought for cache locality and is being spent on something else.

---

## 3. The line rule

**Try to build for 300 lines.**

*(Softened from "no C++ file exceeds 300 lines" on 2026-08-28, by the author, and
the word that changed is `try`.)* It is a target to write toward, not a ceiling
that fails a build — the number is tighter than the first satellite's 325 and it
is aimed at the shape a file comes out in, which is a thing a person judges.

The first satellite arrived at its ceiling late, by splitting a 2208-line
`eval.cpp` and two 700-line functions *after* they had been written — and the
splits are visible in the result, with headers named `eval_internal.hpp` existing
to hold what an anonymous namespace used to, and comments explaining that "the
bodies below are UNCHANGED; they moved."

**Splitting a file after the fact preserves its shape. Writing to a target changes
the shape.** So the target is what you aim at from a file's first commit rather
than something applied at the commit where somebody notices — and that is the
whole of what it is for. **A file that goes over is not wrong; a file that got
long because nobody was aiming is.** Split by SUBJECT when you split at all: a
seam chosen to satisfy an arithmetic is a seam in the wrong place, which is the
failure the first satellite's `eval_internal.hpp` records.

Consequences to plan for rather than discover:

- **An umbrella header is a legitimate answer.** One door that includes its parts in
  an order that compiles, so every consumer's include line stays the same.
- **A dispatch table is a legitimate answer and a chain of `if` arms is not.** This
  is the same change §1.1 demands for other reasons — `handlers[path_id]` does not
  grow with the number of paths, so the file holding it does not either.
- **`words.def` is the one file that may exceed 300 lines.** It is data, not code,
  and splitting a numbering whose meaning is registration order is the one split
  that could silently change what a program means (DESIGN §4.3).

Markdown is not C++. This file and DESIGN.md are not bound by the rule.

The same argument produces the same shape one level up: **the `Makefile` is an index
over `make_support/`, and `satellite_enterprise/install.sh` is an index over
`install_support/`.** Both start that way rather than arriving there.

---

## 4. The build

### 4.1 Two version numbers, moving at different rates

`SATELLITE_VERSION` (003) is the **language**; it changes rarely and deliberately.
`SATELLITE_REVISION` (01) is **this build of it** and goes up as work lands. 001 was
what the first satellite carried while its design was being written, 002 is what it
became, and 003 is the second satellite — the number moved because the language is
being rebuilt, not because this build is newer.

They arrive as `-D` on the four compile recipes that need them — `main.o` and
`opening.o` in each of the two variants — and are **deliberately not in `CXXFLAGS`**, because `CXXFLAGS` is what `.cxxflags-stamp` records and a build stamp
that changes every second would recompile the whole tree on every invocation,
permanently silencing the one check that exists to catch a real flag change.

`satl --version` prints **both the path make invoked and the `__VERSION__` the
compiler reported**, because they are different facts. This project has already been
bitten once by the difference: `LLVM_BIN` pointed at a directory that no longer
existed, `c++` answered instead, and every figure attributed to clang was GCC's with
nothing anywhere saying so.

It caught a second, milder case on 2026-08-26: `CXX=clang++` is exported in this
machine's environment, so `origin CXX` is `environment` rather than `default` and
`010-compiler.mk`'s wildcard never fires. `__VERSION__` proved it was the same
compiler anyway. **That is what printing both fields is for.**

### 4.2 Two microarchitecture builds, and the program that chooses

*Landed 2026-08-26.* `make` produces **three binaries on x86-64 and one everywhere
else**:

| binary | built with | what it is |
|---|---|---|
| `satl` | no `-march` | the baseline build; runs on any x86-64 |
| `satl.haswell` | `-march=x86-64-v3 -mtune=haswell` | for Haswell (2013) and newer |
| `satl-cpu-level` | no `-march`, deliberately | prints `haswell` or `baseline` |

`satellite_enterprise/install.sh` runs `satl-cpu-level` and installs the answer as
`$HOME/.satl/satl`. The machine gets the fastest build it can actually execute, and
neither build has to be a compromise for the other.

**`-march=x86-64-v3` and not `-march=haswell`, and that is the correctness argument
for the whole mechanism.** `-march=haswell` additionally licenses FSGSBASE, PCLMUL, RDRND and
XSAVEOPT on both of this project's compilers, plus INVPCID on clang 24 and HLE on
gcc 17. *(Measured 2026-08-26 by diffing each compiler's own enabled-feature set;
AES is in neither, and needs an explicit `-maes`.)* Every real Haswell has them, so the flag is not *wrong* — but the check
on the other side, `__builtin_cpu_supports("x86-64-v3")` in
`src/programs/cpu_level.cpp`, does not cover them. The set the compiler may emit
from would then be a strict superset of the set that was verified, and **that gap is
where a SIGILL on somebody else's machine comes from.** `-march=x86-64-v3` closes it
by construction. *Check what you compiled for; compile for what you check.*

`-mtune=haswell` is separate, because tuning is not licensing: it reorders and
schedules for Haswell and emits no instruction `-march` did not already allow, so a
Zen 4 running this build runs correct code that was merely scheduled for a different
pipeline. It is the right guess, since Haswell is the oldest machine that can run it.

**The detector is compiled at the baseline and must stay there.** It is the one
binary that runs before anything is known about the machine, so an `-march` that let
the compiler emit a single AVX2 instruction *there* would turn "you get the portable
build" into a SIGILL on the machine that needed to be told that.

**The build asks the compiler what it targets, not `uname` what the machine is** —
`$(CXX) -dumpmachine`. The two answers differ under every cross build, and the flags
go to the compiler, so the compiler decides. On aarch64 and ppc64le — both of which
Enterprise Linux ships — `-march=x86-64-v3` is not a flag the compiler will take, and
there is nothing for a second build to be second to; those machines build one `satl`.
An unrecognised or empty triple takes the one-build branch too, because one build is
the answer that is never wrong and two builds on a machine we could not identify is a
guess.

**The installed binary is its own record.** `SATELLITE_BUILD_FLAGS` is baked in per
variant, so `satl --version` prints which of the two it is for the rest of its life.
The install therefore writes **no manifest**, and cannot have one that disagrees with
the file it describes.

Measured on this machine, 2026-08-26 — Intel Xeon E5-2670 v3, which is Haswell-EP:

- `satl-cpu-level` prints `haswell`, and `satl-cpu-level --explain` prints the three
  levels it read — `x86-64-v2 yes  x86-64-v3 yes  x86-64-v4 no` — so this box gets
  the haswell build.
- The flag reaches the compiler: a vectorizable test loop compiles to **17 `ymm` /
  `vfmadd` / `vmovups` instructions with `-march=x86-64-v3` and 0 without.**
- `satl` contains **no v3 instruction at all**; `satl.haswell` contains **21** — 16
  `vmovups` on 256-bit `%ymm0` and 5 `vzeroupper` — and every one of them is inside
  `satellite::version_text`, where the compiler vectorised a string copy. That is
  the honest state: the flag reaches the code and changes it, on the one function
  M1 gave it to work with, and there is still no speed claim to make, because
  nothing measurable runs there. The payoff arrives with DESIGN §8.1's base-10⁹
  loops.

Building the variants before there is anything to speed up is the same decision as
measuring startup at M1 (§4.3): put the guardrail in before there is a language to
regress it.

Where it lives: `make_support/045-microarchitecture.mk` (the flags, the triple test
and the object lists), `src/programs/cpu_level.cpp` (the detector), and a second
stamp file `.cxxflags-stamp-haswell` — two stamps rather than one, because a shared
stamp would rebuild the baseline objects whenever the haswell flags changed, and
because a single file's contents could only ever describe one of the two variants.

### 4.3 The startup budget

Measured at M1, with `satl` doing nothing, so that every later milestone has a floor
to be compared against and a regression has somewhere to be attributed. Best of five
runs of 200 invocations:

```
bare int main(){return 0;}          1.74 ms
satl (M1, opening information)      1.75 ms
satl --version                      1.75 ms
```

So satl's own share of starting up is about **0.01 ms**, and `ldd satl` lists **6
shared objects.**

The numbers live in `make_support/040-sources.mk`, beside the decision they justify
— not in a commit message, where nobody looks for them again.

**Re-measured at M6 on 2026-08-31, and the 0.01 ms above stopped being true.** Both
sides static, which is M3's correction to this method: `satl --version` is **1.172 ms**
against an empty static program's 0.552 ms, so satl's own share is **0.620 ms** — 34×
what M1 took and M3 reproduced, and twice as long as starting a program that does
nothing. Two thirds of it is `facts::physical_cores()`, which reads two sysfs files per
CPU on every run to fill in a row only `satl --limits` ever prints; most of the rest is
the two threads §4.5.1.2 starts. The table, the attribution and the method are in
`make_support/040-sources.mk`. **M4, M4.5 and M5 landed with no re-measurement at all**,
so this one covers four milestones — which is the case for §9's rule rather than an
argument against it: the floor did its job the first time it was used in anger.

**Fixed the same day, and the floor is 0.724 ms.** `satl --version` opens **no files
at all** now — fifty before it — because `satellite_config.ini` can say
`CORE_COUNT=arguments.machine.cores` (§4.5.4) and a setting nobody asks for is never
resolved. satl's own share is **0.168 ms**, of which 0.14 is the pool builder and the
watchdog: §4.5.1.2's decision costing what it decided to spend, and nothing else.

**`make startup` takes this measurement, as of 2026-08-31, and until then nothing
did.** §9's rule below has asked for it every milestone since this plan was written
and had no target behind it, which is exactly why the paragraph above covers four
milestones instead of one. The harness is `make_support/067-startup.mk`,
`startup.rows` — the commands and what each cost last time — and `startup.sh`.

**What building it found, and it sharpens this section's own method.** M3 corrected
§4.3 to *"both sides static"* after timing a static satl against a dynamic empty
program made satl look 0.9 ms **faster** than doing nothing. That instruction is
right and it is not the underlying fact. Measured while the harness was written:

| | floor | `satl --version` | satl's own share |
|---|---|---|---|
| dynamic | 1.457 ms | 1.633 ms | **0.176 ms** |
| static | 0.568 ms | 0.755 ms | **0.187 ms** |
| M6's record, static | 0.556 ms | 0.724 ms | **0.168 ms** |

The two absolute columns are 0.9 ms apart. **The share is not** — 0.176 against 0.187,
and against a figure taken on a different afternoon. The dynamic loader is a constant
this build pays *twice*, once in the floor and once in satl, so subtracting the floor
removes it. So the rule is **both sides linked the same way**, and then the share is
the number a milestone is answerable for; `067-startup.mk` links its floor through the
same variables `050-build.mk` links `satl` through, which makes that structural rather
than an instruction somebody has to remember. The harness reproduces the M6 table row
for row when run at `STATIC=full`.

**This binary links no GUI, and that is measured rather than tidy-minded.** The first
satellite's `satl-term` resolves 79 shared objects and maps 78 on this machine
against `satl`'s 6, and the dynamic linker loads every one before `main()` on every
run. The first satellite measured that at **25.9 ms with the GTK link against 2.5 ms
without**, of which 23.4 ms is the linker and the interpreter's own share of hello
world is 0.3 ms. Rendering was never the problem.

### 4.4 The window needs `dlopen`, not the binary split

The first satellite fixed the startup cost by making `satl` and `satl-term` two
binaries. That works for a terminal host, which never interpreted anything anyway.

**It does not work for `satellite.window.new()`**, which runs inside a user program,
which runs in `satl`. You cannot split that out.

The answer is to **`dlopen` a `libsatellite_window.so` the first time a window path
executes.** Startup stays at this tree's own floor — 1.75 ms (§4.3) — for every
program that never opens a window, and the 23 ms is paid only by programs that do.

This is a port rather than an invention, and the first satellite deserves the
credit: its `design/11-build-order.md` already schedules "a `dlopen`ed shim" as its
own M9, with the done-when condition that `satellite.include(satellite.window)`
opens a window *and* `ldd satl` still lists six objects. It was planned there and
never built; it gets built here.

`satl-term` stays, and keeps its name. `satl` keeps its name.

---

## 4.5 The machine limits, and the file that holds them

*(Asked for 2026-08-27. Specified here; **built at M6 on 2026-08-30** —
[MILESTONES/M6.md](MILESTONES/M6.md) is the review, and §4.5.3 and §4.5.4 below
are answered rather than open.)*

satellite reads a **`satellite_config.ini`** and holds itself to what it says:

```ini
THREAD_COUNT=24     # hardware threads to use
CORE_COUNT=12       # physical cores
MEMORY_MAX=...      # satl must not exceed this
```

**This machine, measured 2026-08-27** — `lscpu` and `/proc/meminfo`: Intel Xeon
E5-2670 v3, 1 socket, **24 hardware threads, 12 physical cores**, MemTotal
64946428 kB = **61.9 GiB**.

### 4.5.1 What gets threaded, and what must not

The request was that converting `satellite.something.something` into integers be
threaded — *"create that many threads and then hand them each a line that begins
with satellite, or the fastest possible way to do it, whatever that may be."*

**The line-per-thread part is right and the startup part is not**, and §4.3 is why.
satl starts in 1.75 ms of which its **own share is 0.01 ms**. Spawning 24 threads
costs **690 µs measured** — seventy times satl's entire current startup — and the
fixed word table is 254 nodes, which a single thread walks in microseconds.
**Threading the table at startup is a guaranteed loss**, and M2 settled it a second
way: the table is `constexpr` and lands in rodata, so there is no startup work left
to thread.

Threading the walk over a *user's source* is a different question, because that
work scales with the program and the table does not. The number that decides it is
the **crossover**: how many satellite-rooted source lines a program needs before 24
threads beat 1, counting thread creation. **Measured 2026-08-28, below.**

So the shape to build toward: a pool that is **created lazily, on first real
threaded work**, sized from `THREAD_COUNT`, and shared by everything that needs
threads — the console's printer thread (DESIGN §10.1), parse-time interning, and
`satellite.variable.thread` at M23. One pool with three tenants amortises a cost
that none of them could justify alone, and a program that never threads never pays.

**NEITHER OF THE CONSOLE'S THREADS IS A TENANT, AND THE RULE THAT SAYS SO IS ONE
LINE: THIS POOL TAKES WORK THAT FINISHES.** *(Corrected 2026-09-02, on the
author's word — **the plan was always that the printer would create its own
thread.** The sentence above is left standing because four other places in this
document and one in LAYOUT.md were written from it, and a reader who met one of
those needs to find the correction rather than a silence.)* `run_over(units,
body)` is a range, a split and a join: it returns when the last chunk lands. **The
console's two threads exist precisely because they do not finish.** The printer
waits on a queue for the life of the run; DESIGN §10.1's reader *"blocks on
stdin"*, and that one is not a mis-count but a deadlock — a worker sitting in
`read()` never comes back for a chunk and `run_over()` waits for it forever. A
thread that outlives every batch cannot be lent by something that counts what it
has lent out: `wanted()` and `parked()` would go on counting it and `satl
--limits` would go on reporting it, which is §4.5.2's whole job done wrong.

**THE READER WAS NEVER AN OPEN QUESTION, AND DESIGN SETTLED IT FIRST.** §8's M14
entry has said *"the dedicated thread blocks on stdin"* since it was written, and
the invariant under both directions is §10.1's *"the program's own thread never
blocks on the terminal."* So `satellite.console.typed()` `1 5 5` at M14 and the
prompt at M22 inherit a decision rather than taking one. **The sentence corrected
here cites `(DESIGN §10.1)` as its authority and DESIGN §10.1 is where the
contradiction was**, which is the part worth keeping: this was not a fact nobody
had established, it was a fact established in the document the claim pointed at.
**The test to apply to the next candidate is not "does it want a thread" but
"does the work end".**

**So the pool has two named tenants and not three**, and neither has been built.
The honest list is **parse-time interning**, which is real work this tree does on
one thread today and which belongs to no milestone, and
**`satellite.variable.thread` at M23**.

***AND M23 IS NOT A TENANT, WHICH THE PARAGRAPH ABOVE IS THE REASON FOR AND DID
NOT NOTICE.*** *(Corrected 2026-09-12, when M23 was built.)* The rule stated here
is **"this pool takes work that finishes"**, and it was arrived at by ruling out
the console's two threads, which do not finish. A satellite thread's capsule DOES
finish, so it passes — and the test turns out to be necessary and not sufficient.
**The half it is missing is *and does the caller wait for it*.** `run_over()` is
a range, a split and a join: it returns when the last chunk lands, and
`my_thread.start()` has to return before the work starts. The two are opposites.

**And the deeper objection is the one that settles it.** A pool has `THREAD_COUNT`
workers. A program starting `THREAD_COUNT + 1` threads that join each other would
wait forever — which is a ceiling on the language, arrived at by accident, of
exactly the kind SCRATCH.md/NO_LIMITS.md refuses. So M23 makes a fresh
`std::thread` per `start()` (about 28 us created-run-joined, measured
2026-09-12) and **this list is down to one unbuilt tenant**: parse-time interning,
plus whatever `parallel_for` becomes. **The test to apply to the next candidate
is now two questions and not one**: does the work end, AND does the caller wait
for it. `satellite_thread/thread_handle.hpp` carries the same correction beside
the code that declines the pool. `parallel_for` would be the third and it
is in no numbering, no document and no milestone; §4.5.1.2 is the decision that
rests on it and says so out loud. `satellite.include` of a second file is **M25**
and is the first thing that collects the ~170 figure below rather than the ~2,650
one, which is a different claim from being a tenant.

**The one real question in this neighbourhood is M27's**, and it is downstream of
the invariant rather than of the pool: *"what `receive` `1 20 5` blocks on, and on
whose thread"* asks whether §10.1's rule generalises past the terminal to a
socket. That one is open and is listed where it belongs.

**Measured 2026-08-28, and the pool is not an optimisation — it is the thing that
makes the request worth honouring at all.** M2 built the walk, so the crossover
this section had been holding open since 2026-08-27 could finally be taken against
the real code path rather than a stand-in: `words::walk()` over real paths of the
real language, on this machine, 24 hardware threads, best of 15 runs, load average
under 1 before starting.

| satellite-rooted lines | 1 thread | 24 fresh threads | 24 from a pool |
|---:|---:|---:|---:|
| 150 | 41 µs | 695 µs (0.06×) | 48 µs (0.85×) |
| **180** | 51 µs | 692 µs (0.07×) | **48 µs (1.07×)** |
| 300 | 90 µs | 684 µs (0.13×) | 47 µs (1.90×) |
| 2,500 | 676 µs | 717 µs (0.94×) | 129 µs (5.25×) |
| **2,800** | 778 µs | **709 µs (1.10×)** | 125 µs (6.21×) |
| 50,000 | 13,264 µs | 1,688 µs (7.9×) | 1,124 µs (11.8×) |

**Two crossovers, and they are fifteen times apart:**

- **Spawning 24 fresh threads breaks even at about 2,650 lines.** Creating them
  costs **~690 µs** flat — which is the 0.5–1.5 ms this section estimated,
  measured — and that cost does not move with the size of the program, so the
  whole of the left-hand column is spent paying it back.
- **Waking 24 threads that already exist costs ~47 µs and breaks even at about
  170 lines.**

**THE 170 IS A SECOND-BATCH NUMBER, AND SAYING SO IS THE WHOLE OF WHAT THIS
MEASUREMENT IS WORTH.** *(Corrected 2026-08-28, hours after it was first written
here.)* The pooled column above was timed with the pool **already built**, because
that is what "waking threads that already exist" means. **Creating those parked
threads costs ~590 µs** — measured separately, best of five — which is the spawn
cost again, near enough. So a pool's FIRST batch pays it:

| | 2,500 lines | 2,800 lines |
|---|---:|---:|
| 1 thread | 676 µs | 778 µs |
| pool, **cold** — create + wake + work | 712 µs | 708 µs |
| pool, **warm** — wake + work | 129 µs | 125 µs |

**A cold pool crosses at ~2,650 lines, which is the spawn figure**, and it could
not be otherwise: a cold pool *is* 24 fresh threads plus a cheap wake.

**So the pool does nothing for a program that threads exactly once**, and that is
the common case this section was reasoning about — `satl program.satl` parses one
file and exits, and parse-time interning is likely to be the FIRST threaded work in
the run, so it pays the creation itself and crosses at ~2,650 like everything else.

**What the pool actually buys is the second tenant onward**, which is the argument
this section already made from first principles and can now put a number on:
*"one pool with three tenants amortises a cost that none of them could justify
alone."* The tenants that collect the ~170 figure are the ones that are not first —
`satellite.include` of another file at M25, M22's prompt parsing repeatedly, and
M23. *(This list opened with "the console's printer thread if it started earlier"
until 2026-09-02; see the correction above. Removing it takes the figure's
collectors from four to three and moves the first of them from M10 to M25, which
is fifteen milestones later and is the real cost of the correction.)* **The lazy
pool is right and the reason is amortisation across a run, not a cheaper parse.**

### 4.5.1.1 Warming the pool at startup — measured, and it beats the lazy rule

**The author's question, 2026-08-28: if satl is started we must assume a `.satl`
file is coming, so should the pool not be warming from the moment satl starts?**
It is the right question and it makes §4.5.1's *"created lazily, on first real
threaded work"* the wrong rule. Three arms, same corpus, same machine, best of
seven, load average 0.02:

| satellite-rooted lines | single | lazy — build at the parse | **eager** — warm from startup |
|---:|---:|---:|---:|
| 100 | **27 µs** | 672 µs (0.04×) | 48 µs (0.57×) |
| 1,000 | **309 µs** | 742 µs (0.42×) | 491 µs (0.63×) |
| 2,000 | **602 µs** | 716 µs (0.84×) | 622 µs (0.97×) |
| 2,500 | 733 µs | 745 µs (0.98×) | **641 µs (1.14×)** |
| 4,000 | 1,113 µs | 715 µs (1.56×) | **675 µs (1.65×)** |
| 50,000 | 13,745 µs | 1,882 µs (7.3×) | **1,825 µs (7.5×)** |

*Eager is: spawn ONE thread at startup, which builds the other 23; the main thread
starts walking immediately and the pool takes the remainder once it exists.*

**Three findings, and the first two say the author is right.**

- **Lazy is dominated. Eager beats it at every size measured**, because the main
  thread never waits for the pool. Kicking the build off costs the main thread
  **~20 µs**, not the ~590 µs it costs to build the pool yourself.
- **The crossover moves in, from ~2,650 to ~2,300 lines.** By the time the pool is
  ready — ~600 µs — a single thread has already walked ~2,150 lines, so anything
  smaller finishes before the pool exists. That is not a flaw in the scheme; it is
  the ceiling on what any scheme can do.
- **But warming is NOT free, and this is the part the argument gets wrong.**
  Eager runs at **0.42×–0.60× of single-threaded between 100 and 1,000 lines** —
  reproduced twice. Creating 23 threads is `mmap` and `clone`, which contends with
  the main thread's own allocation and page faults on the process's memory locks
  while it walks. **The main thread is slowed by warming a pool it will never use.**

**And file I/O hides none of it.** Reading `example/hello_world.satl` takes
**4.4 µs** and reading a 35 KB file takes **8.1 µs** — measured. There is no
startup window to hide 600 µs of thread creation behind. The only thing long
enough to hide it behind is the walk itself.

### 4.5.1.2 The pool starts at startup, always — the author's decision

**Decided by the author 2026-08-28, and it supersedes both §4.5.1's "lazily, on
first real threaded work" and the size rule §4.5.1.1 first proposed:**

> *satl almost must start the THREAD_COUNT in satellite_config.ini because we
> almost have to assume the user will call parallel_for, so that requires a warm
> pool no matter what; in other words, MOST satellite code will require 24, so
> start them now.*

**This changes the question, and the measurements above were answering the wrong
one.** Everything in §4.5.1.1 measures the pool against *parse-time interning* —
27 µs of work for a small program, which nothing can usefully thread. The pool's
real tenant is the **running program**, and a program's parallelism has nothing to
do with the size of its source: ten lines can run a million-iteration loop. **A
size trigger predicts parse cost and is therefore the wrong trigger**, and it is
also the kind of hidden threshold DESIGN §1.1 refuses — *do absolutely everything
for the user, and pay for it in performance rather than in their attention.*

**The arithmetic supports it once the base is stated in absolute terms rather than
as a ratio.** §4.5.1.1's frightening "0.42×–0.60×" is a ratio on a very small base:

| | eager costs a program that never threads |
|---|---:|
| 100 satellite-rooted lines | **+21 µs** |
| 500 | **+47 µs** |
| 1,000 | +182 µs |
| against satl's whole process startup (§4.3) | 1,750 µs |

So a program that never threads pays **1–3% of startup**, and a program that does
avoids **~590 µs of blocked execution** at its first parallel call. The trade is
worth taking well before "most" — one program in four is enough.

**Two implementation requirements this decision carries**, both measured above:

- **The main thread must not build the pool itself.** Spawn one thread and let it
  build the other 23: that is ~20 µs on the main thread instead of ~590 µs.
- **`THREAD_COUNT` has to be readable before the pool is built**, and
  `satellite_config.ini` does not exist yet (§4.5.4). Until it does, the count
  falls back to what the OS reports — 24 on this machine, and **not** cores × 2,
  which is this CPU's SMT ratio and not a rule.

  *(M6 built both halves on 2026-08-30. The file is read first and the fallback
  is `src/system_facts/host_facts.cpp`'s `hardware_threads()`, which asks
  **`sched_getaffinity`** rather than `hardware_concurrency` — a third reading of
  this number that §4.5.4's setting-versus-fact split did not name: not what the
  machine has, and not what satl may use, but what the OS will let this process
  run on. It matters because the pool starts on every run, so under `taskset -c
  0-3` the old reader would have spawned 24 threads onto 4 CPUs. `satl --limits`
  prints the setting and the machine's own answer side by side.)*

**The ~20 µs is right about the main thread and is not what a run costs, measured
2026-08-31 (§4.3).** `strace` confirms the shape this section asked for — two `clone3`
calls on a `satl --version`, not twenty-four, because the process is gone long before
the builder has made the other 23. The process still pays for the two that exist:
detached threads that park and never run cost **87 µs for the first and 28 µs for each
one after it**, wall clock per invocation, because the kernel tears them down before
the parent's `wait()` returns. So the pool builder and the watchdog together are
**0.14 ms** against a whole pre-M6 startup of 0.624 ms — **22%, not 1–3%**, and the
1–3% row above is a ratio against §4.3's *dynamic* 1,750 µs, which M3 had already
retired as not comparable to a static binary. **The decision stands and its
arithmetic does not:** one thread in four still beats ~590 µs of blocked execution,
and the honest base is 0.55 ms rather than 1.75. `make_support/040-sources.mk`
carries the thread-count table this comes from.

**And since 2026-08-31 it is the whole of what M6 costs a run**, which is the number
this section should be judged on: 0.14 ms of the 0.168 ms satl adds to starting up is
these two threads. Everything else M6 put on the startup path was `physical_cores()`
being read for a row nobody had asked for, and §4.5.4 is where that stopped.

**Two facts the author should have in view, neither of which blocks the decision:**

- **`parallel_for` does not exist.** It is in no numbering, no milestone and no
  document; the language's whole parallelism surface today is
  `satellite.thread.new` `1 23 1` and `satellite.variable.thread.start()` /
  `.join()` `1 6 13 1`–`1 6 13 2`. The decision above assumes a construct that has
  to be designed, numbered and built, and that work is not scheduled anywhere.
- **QUAD spawns zero threads.** The only `<thread>` use in its 3,029 lines is
  `sleep_for` (QUAD.md §2, checked against the source). The one program this
  language exists to express would never touch the pool — which does not make the
  decision wrong, but does mean *"most satellite code will require 24"* is a
  statement about where the language is going rather than about what it runs today.

**The `.satc` write is untouched by all of this and stays threaded always.**
SATC.md §5 puts the write on its own thread so the run never waits for it — that
is **one** thread hiding disk latency, not twenty-four splitting work, and it is
worth its ~25 µs at any program size. *"Thread the conversion no matter what"* is
right about the write and wrong about the walk, and they are different jobs.

**And below ~170 lines the walk must stay single-threaded**, which is a rule the
pool's owner has to enforce rather than a suggestion: at 80 lines the pooled arm is
still 0.48× — it takes twice as long as doing the work — and hello world is one
line. The speedup tops out near 12× on 24 hardware threads, which is 12 physical
cores answering, exactly as §4.5's table says the machine is built.

*(The measurement that matters is the pooled one, and it is the one this section
proposed before it had a number. The benchmark is
`SCRATCH.md`-adjacent scratch and is not in the tree; what is in the tree is the
result, here, beside the decision it justifies.)*

### 4.5.2 Knowing what satl is using

**The first satellite already built most of this**, in
`old_versions/first_satellite/src/system_facts/memory_facts.cpp`:

- `process_memory_bytes()` reads `/proc/self/statm` field 2 × page size, fresh
  every call, never cached — its own comment says *"the question is what this
  program is using NOW."*
- `start_memory_watchdog()` runs a detached thread that wakes once a second, reads
  a threshold, compares, and exits through a hook that restores the terminal first.

**Its policy is the opposite way round.** v1 watches the *machine's* available
memory against a floor (`min_free_mb`, default 4096); MEMORY_MAX bounds *satl's
own* use. Both are defensible and they are not the same guarantee — the second is
the stronger promise and the easier one to explain, and `process_memory_bytes()` is
already the call that implements it. **Keep the thread, keep the exit path, change
what it compares**, and ideally keep both checks.

### 4.5.3 A file is not the only place a setting lives

v1's watchdog does not read a file. It reads
`satellite.library.system.min_free_mb` — the language's own namespace, which a
running program can read and retune — and `Number` does the same for
`division_digits`. Both are numbered: `1 14 2 3` and `1 14 2 1`.

The two answer different questions and are not in conflict: **the file is where a
machine's settings live before a program starts**, and is what an installer writes
and a person edits; **`satellite.library.system.*` is how a running program reads
and changes them.** The obvious arrangement is that the file seeds the namespace at
startup and the namespace is what everything reads afterwards — one authority at
runtime, one place to edit at rest.

**Decided at M6, 2026-08-30: it is that arrangement, and what this section was
missing is that the file holds TWO KINDS OF LINE.** The file is read once, at
startup, its values land in the dials, and nothing reads the file again.

    THREAD_COUNT   CORE_COUNT   MEMORY_MAX        machine settings, SHOUTED
    division_digits  max_depth  min_free_mb  float_digits     dials, lower case

**The upper-case three are in no numbering at all** — no `satellite.` path names
them, and a running program cannot read or retune them, because what they
configure is the process rather than the language. The lower-case four *are*
`satellite.library.system.*`, `1 14 2 1` to `1 14 2 4`, and they are the half
this section was written about. The case difference is the rule: one line of the
file tells you which kind you are looking at. `src/machine_limits/config.cpp`
takes the four dial names **out of `words.def`** rather than writing them a
second time, so the spelling in the file is the spelling in the language by
construction.

**And only `min_free_mb` has a MEANING at M6**, which is the constraint §8's M6
puts on it: the other three are stored, printed by `satl --limits` beside the
milestone that will read them, and interpreted by nothing. A default for
`division_digits` would have been M8 deciding what a division does.

### 4.5.4 Answered at M6, 2026-08-30, and the second question again on 2026-08-31

*(This section was three open questions and PLAN §8's M6 called them its three
blockers. Each is restated as it stood, with what was decided.
[MILESTONES/M6.md](MILESTONES/M6.md) §3 carries the arguments.)*

- **What unit is `MEMORY_MAX` in, and what is the default?** 61.9 GiB and 64.9 GB
  are the same memory. Should the shipped default be the whole machine or a
  fraction, and does a machine with less than the file claims win?

  **The unit is written in the file and a bare number is REFUSED**, with a code
  and a caret — because the question as posed has no answer a reader of the file
  could check. Nine units: `B`, `KiB`, `MiB`, `GiB` and `TiB` are powers of 1024,
  `KB`, `MB`, `GB` and `TB` are powers of 1000. Whole numbers of units only;
  `61.9GiB` is refused with its own sentence naming the fix, because a ceiling
  rounded to a whole byte behind the user's back is one nobody can predict.
  **The default is the WHOLE MACHINE**, because a fraction is a number satl
  would have invented about a program it has never seen — 50% is generous for a
  parser and absurd for the thing QUAD.md exists to run. **And the machine
  wins**: a file asking for more than `MemTotal` is clamped to it, and
  `satl --limits` prints that row as coming from *"the machine, over the file"*,
  because clamping quietly is the thing DESIGN §1.1 refuses.

- **A setting is not a fact.** `arguments.machine.threads` (DESIGN §7.7) asks what
  the machine *has*; `THREAD_COUNT` says what satl may *use*. If they are ever
  allowed to differ they need two names, and a program asking the first must never
  get the second.

  **They may differ, they already have two names, and M20 builds ONE reader.**
  `THREAD_COUNT` is not in the numbering, so no program can reach it and no rule
  is needed to stop one; `1 14 1 1 1 3` answers `facts::hardware_threads()` and
  nothing else. `satl --limits` prints both side by side, which is the visible
  form of the difference — and it is where **not cores × 2** stops being a rule
  in a document: 24 and 12 are read from two different places and neither is
  derived from the other. The same answer settles `satellite_string`'s code 97
  (SCRATCH.md/PORTING.md §3): it is the fact. §4.5.1.2 above records the third
  reading this question did not have a name for.

  **AND THE FILE MAY NOW NAME THE FACT, WHICH IS THE HALF THIS ANSWER MISSED,
  added 2026-08-31.** `CORE_COUNT=arguments.machine.cores` says *"whatever this
  machine has"*, in the spelling a program uses for the same fact — one path per
  setting, exactly DESIGN §7.7's pairing, and all six spellings of `arguments`
  because config.cpp walks the real trie. The other two are
  `arguments.machine.threads` and `arguments.memory.total`. **The dials do not
  take one and the line is the same line**: a machine setting is about the
  machine and has a fact behind it, and there is no fact called
  `division_digits` for a file to name.

  **What it cost NOT to have was 0.42 ms on every run of satl** (§4.3). Without
  a way for the file to say "the machine", `limits.cpp` had to fill all three
  settings in from the machine *before* opening the file and let the file
  overwrite what it named — "machine first, then file" — so
  `facts::physical_cores()` read two sysfs files per CPU on every invocation to
  produce a number only `satl --limits` prints. The order was a workaround for a
  missing spelling; the spelling exists now and the order is gone with it.
  **A setting is resolved when something asks for its value and not before**,
  which is the shape this question should have had from the start: three ways to
  ask, one place that knows, and nobody asking until they need it.

- **The file does not exist yet.** Nothing reads it and nothing writes it. The
  installer (§5) is the natural author.

  **M6 built the reader and did NOT make the installer write one**, and the
  split is deliberate. satl works with no file at all — every value then comes
  from the machine, and `satl --limits` says so — so an install that writes
  nothing is a complete install. What the installer would be taking on is
  ownership of a file the user then EDITS, which §5.1's `rmdir`-never-`rm -rf`
  rule and `--uninstall` both have to answer for, and that is §5's decision
  rather than the reader's. `example/satellite_config.ini` is the documented
  format in the meantime, and `satl --limits <file>` checks one before it is
  installed.

---

## 5. Installing

`satellite_enterprise/` is the Enterprise Linux installer — written and tested on
AlmaLinux 10.2, aimed at the RHEL family, and it **says so and carries on** if it
finds itself elsewhere, because refusing on the strength of a name in
`/etc/os-release` would be a policy dressed up as a check.

`install.sh` is an index over eight fragments in `install_support/`, POSIX sh, and
**needs no root ever** — everything it writes is under `$HOME`. That is a real
simplification over the first satellite's installer, which had to compile as the
human and copy as root; the fixed per-user root is what buys it.

**The root is `$HOME/.satl` and it is not a prefix.** The binary is
`$HOME/.satl/satl` rather than `$HOME/.satl/bin/satl`: a prefix layout exists so
many packages can share `bin/`, `lib/` and `share/`, and nothing shares this
directory. One name to remember, one directory to delete. `share/` underneath it
keeps the XDG-relative paths so that publishing to the desktop is a link with the
same relative path on both sides.

Twenty files: the chosen `satl`, the `.satl` mime packet, and nine icon sizes in
both `apps` and `mimetypes`.

### 5.1 The rules it inherits

- **Every command that changes anything goes through `run()`**, which is what makes
  `--dry-run` a complete and honest transcript rather than an approximation. Purely
  read-only helpers — reading `/etc/os-release`, sorting the install list — run
  directly and deliberately, because a transcript of them is noise, and because a
  `--dry-run` that could not read the machine could not tell you what it would do.
- **Nothing edits `.profile`, `.bashrc` or any other file the user owns**, and
  nothing tells the user to export a variable. A child process cannot change its
  parent's environment, so an installer that "exports PATH" exports it into a shell
  that exits one line later; editing a login file is a permanent change made by a
  program somebody ran once, and `--uninstall` cannot reliably undo it.
- **`rmdir`, never `rm -rf`.** An uninstall removes files by name and directories
  only when they are empty, so anything the script did not install survives and is
  *reported* rather than discovered later.
- **The install tree is declared exactly once**, in `060-install-tree.sh`, and read
  by both the install and the uninstall. Two lists is how an install tree rots. When
  the Makefile grows an `install` target, that function is what should be **deleted**
  rather than duplicated.

### 5.2 What it will not do on its own

`--link` (a symlink in `~/.local/bin`) and `--desktop` (the icons and file type in
`~/.local/share`) are **off by default**, and the reason is measured rather than
principled.

This machine already has `~/.local/bin/satl` — 968536 bytes, the **first**
satellite's interpreter — plus `satl-term`, the nine hicolor sizes, an `index.theme`
and the mime packet, all installed there by that satellite's own installer. There is
a third install at `/usr/local/bin/satl`, root-owned, and because `/usr/local/bin`
precedes `~/.local/bin` on this PATH, **`satl` resolves to the first satellite** for
anything that searches PATH. *(Not in this user's interactive shell, where `.bashrc`
aliases `satl` to this tree's build and an alias beats PATH — so a terminal test and
a script test answer differently. Noted 2026-08-26.)*

Defaulting those flags on would have replaced a working interpreter with an M1 build
that cannot interpret anything, on the first plain `./install.sh`. So they are
opt-in, and even when asked for the script **refuses every path it does not already
own** — verified: all twenty refused, nothing overwritten. It creates only symlinks,
which is what makes ownership unambiguous when another install owns real files at
the same paths.

That is DESIGN §1.1's rule applied to an installer: do everything for the user, and
never anything behind their back.

### 5.3 The artwork

`satellite_enterprise/icons/` is the installable tree, whose layout mirrors the
install destination exactly, so installing is a copy and not a translation.
`satellite_enterprise/icon_artwork/` is the source work, including the `.xcf`.
Everything was copied byte-for-byte from the first satellite and verified with
`cmp` — 43 files, no re-encoding.

Two names must not drift: `org.satellite.terminal` (the app, matching the
GApplication id and the `.desktop` filename) and `application-x-satellite` (the mime
icon, matching the type with `/` replaced by `-`).

**The SVG and the PNGs must never both be installed.** The icon theme spec lets
either satisfy a lookup, so shipping both makes which one a shell draws
unpredictable. The artwork is a photograph, which has no scalable form, so the PNGs
are the ones that ship and `org.satellite.terminal.svg` travels in the tree
uninstalled. The `.desktop` entry was likewise held back until a binary existed for
it to launch — a launcher for a missing program is a menu entry that does nothing.
**M1.5 built that binary on 2026-08-27, and the entry was named in
`060-install-tree.sh` on 2026-08-28**, which is the one declaration of what gets
installed. It is still conditional on the binary: `047-window.mk` drops `satl-term`
from `all` when gtk4 and vte are missing, and the installer then installs neither the
window nor its launcher and says so, because the reason for holding the entry back
was never the date — it was the binary.

**The installer installs three programs and the build makes four files.** `satl`,
`satl-cpu-level` and `satl-term` go in; `satl.haswell` does not, because it and
`satl` are one program compiled twice and §4.2's argument is that the installed
binary is its own record — which stops being true the moment two interpreters sit in
the root and something has to say which one runs. **`satl-term` must land beside
`satl`**: `src/programs/terminal.cpp` reads `/proc/self/exe` and spawns the `satl`
next to itself rather than the one PATH finds, so the fixed root is a requirement
here and not only a tidiness.

`satellite_enterprise/icons/application-x-satellite.xml` carries several findings in
its own comments —
why `generic-icon` points at our own icon, why `--` is banned inside its comments,
why the magic offset window is 4096. **Read them before changing anything about
icons or the mime type.** Each was found the hard way.

---

## 6. What we keep from the first satellite

Ported, adapted, or taken as-is:

- **The syntax and the generating rule.** It is coherent and it is the identity.
  DESIGN §1–§6.
- **Resolve and frames.** Names to integer frame slots, statically, before anything
  runs. The best engineering in the first satellite, and measured. DESIGN §7.
- **Frames as isolation, the library as the atomic global.** The `SLOT_GLOBAL` /
  `SLOT_CAPSULE` / `SLOT_METHOD` / `SLOT_SUIT` / `SLOT_FIELD` sentinel scheme.
- **Exact-decimal `Number` on base-10⁹ limbs, with `double` constructors deleted.**
  Refusing binary floats at the C++ type level is a genuinely good call and it is
  the foundation `satellite.variable.float` needs. DESIGN §8.1.
- **The Console with its own printer thread**, and the `drain()` barrier before
  reading input. DESIGN §10.1.
- **The SIGINT handling** — installed without `SA_RESTART`, using `eof()` to tell a
  real closed stdin from an interrupted read. Hard-won; do not rediscover.
- **The search power** — the comparator/walker split, deliberately free of the
  Evaluator class and *callable from anything*. The ten-level ladder is a general
  power that applies to any value the language has, present or future. Port it close
  to unchanged.
- **The X-macro registry mechanism**, ids frozen, append-only, static_asserts in the
  header so every consumer inherits them.
- **`format.def`'s rule**, verbatim: prose may explain a number, it may never be the
  only place the number lives.
- **Spans on every node.**
- **PCG, and the fast / normal / ultra tiers.** DESIGN §11.
- **The two-binary split and the startup measurement discipline.**
- **The comment culture.** Comments that state a number and where it came from,
  rather than an intent. Rare and valuable. Keep writing them.

---

### 6.1 `satellite_number` and `satellite_string` come across close to unchanged

*(Surveyed 2026-08-27. `satellite_string`'s character half **landed at M3 on
2026-08-29**; `satellite_number` **landed whole at M8 on 2026-08-31**, and its
four open questions are answered at the end of this section.)* What fills them is
the first satellite's, and it very nearly ports as-is:

| | files | lines | largest |
|---|---:|---:|---:|
| `satellite_number` | 10 | 1509 | `limbs.cpp` at 284 |
| `satellite_string` | 2 | 249 | `satellite_string.cpp` at 155 |

**Every file is already under §3's 300-line ceiling**, which is worth noticing
given §3's argument that a ceiling applied after the fact preserves a file's shape
rather than changing it. Here the shape already fits.

`satellite_number` is internally closed — every include is a sibling or the
standard library — so it ports alone. `satellite_string` needs `system_facts/`,
because its code table is not only characters: **codes 95–100 are live values**
resolved at decode time, and 97, 98 and 99 are `threads`, `mem_total_mb` and
`mem_used_mb` — the same facts §4.5 and DESIGN §7.7 reach for by two other routes.

**That dependency is what split the port in two, and M3 took the half that has
none.** *(2026-08-29.)* DESIGN §5's first sentence makes the alphabet the lexer's
dependency, so waiting for M9 was not available; dragging M6's three fact readers
in four milestones early was the other way to pay for it, and it was refused. What
landed is the character table, both encoders and a `decode()` whose six live codes
emit `<threads>` and its siblings. **Nothing in the lexer can reach one**, which is
what makes the stub safe rather than merely cheap: `encode_raw` maps every source
byte to a letter, a digit, a punctuation code or the raw area, so no code in 95..100
can occur in a program's text at all. They are reachable only through `encode()`'s
backslash names inside a string literal body — a **value**, which does not exist
until M9 builds one. **Finishing it is replacing six lines with six calls**, and
`lexer_test`'s check that `"\threads"` decodes to `<threads>` is what fails when it
has not happened.

One thing the port gained rather than carried: **the escape table's ordering rule is
now a `static_assert`.** §5.4 requires longest-first matching so a short name cannot
shadow a longer one, and v1 held that by hand — `"t"` sits four rows below
`"threads"` and nothing but care kept it there. An alphabetised edit would have made
`\threads` lex as a tab followed by `hreads`, silently. FORMAT/CXX.md §1's third
rule is the one that says a fact like that may not live only in prose.

Four things to settle before copying, and `SCRATCH.md/PORTING.md` has the detail.
**All four are answered as of 2026-08-31, and three of them came out differently
from what this list expected.**

1. ~~**Does `Number` keep its reach into `satellite.library`?**~~ **It does, and
   the alternative this row offered is not what happened.** The row proposed *"a
   compile-time default now and the lookup restored later"* because the fear was
   dragging the library registry in years early. M6 built the registry AND the
   storage — `machine_limits/limits.hpp`'s `Dial` — and left `division_digits`
   deliberately unset, saying *"the milestone that will read it decides what no
   answer means."* So M8 reads the real dial through `limits::division_digits()`
   and there is no compile-time default to restore later. **The 34 lives beside
   the dial and not in `Number`**, which is the seam turned the other way up from
   v1: `Number::divide` takes a count and never invents one.
2. **Does the code table stay 16-bit?** *(Settled at M3 with the character half,
   and the LIVE half landed at M9.)* Yes, ported as-is. Nothing in this design
   contradicted the header's argument. **What the live half found is that the six
   calls do not go in this module** — the lexer decodes every token's text and
   `--unparse` prints it back, so a live `decode()` here would rewrite a
   program's source. PLAN §8's M9 entry has it.
3. ~~**Where does `satellite.random` live?**~~ **Settled by the tree rather than
   by a decision, and the port then had to give something back.**
   `src/satellite_random/` landed at M2 ahead of any milestone that calls it. So
   v1's `random.cpp` came across into `satellite_number/` — it is bignum work —
   but v1's `Bits32` and `MAX_RANDOM_DIGITS` did **not**, because this tree
   already has both. **That makes the "internally closed, so it ports alone"
   sentence above wrong by one include**, and the trade was taken on FORMAT §1's
   second rule: one `Bits32` in the tree beats two that agree until somebody edits
   one. `satellite_random/random.hpp` names no PCG type, which is what makes the
   include cost nothing.
4. **`sizeof(Number)` on arrival.** **Checked first, and it fits — but only in one
   layout, which is the part this row did not anticipate.** Measured 2026-08-31
   before a line was written: v1's shape is 32 bytes, an explicit `bool positive_`
   laid flat beside `int exp_` is still 32, and cutting the magnitude out as its
   own type is 40 — because a nested struct's tail padding is not reusable by the
   type holding it, which would put a `Value` at 48. **So the one number that
   could have made this port not fit instead decided its shape.**

**And the sign was the fifth thing, which this list did not have and DESIGN §8.1
added on 2026-08-27.** It is the only part of the port that is not a port: v1
packed the sign into the significand, §8.1 requires an explicit `positive` bool
with the magnitude carrying none, and every operation that decides a sign was
rewritten around it. `MILESTONES/M8.md` §3 is what it cost and what it paid for,
and the short version is that the module came out **shorter** than v1's — three
blocks of code existed only so that `-LLONG_MIN` had somewhere to land.

---

## 7. What we throw away

- **String-keyed dispatch.** All of it: the joined path built per call, the ordered
  arms, the `full == "…"` ladders. This is the change everything else hangs off
  (§1.1).
- **`shared_ptr<const Expr>` for the tree.** §2.2.
- **`mutable int slot` on `Name`.** §2.2.
- **`EvalError{std::string, Span}`** and the 199 `fail()` call sites that each
  compose their own message string. DESIGN §9.
- **"Record the error and return `nullptr`."** The decision not to throw is kept and
  is measured; the 199 sites where a missed null check is a segfault are not. DESIGN
  §9.1.
- **Switching on raw `std::variant` indices.** The code documents this as a landmine
  — "APPEND ONLY", "silently renumber every alternative after it". Make adding an
  alternative a compile error.
- **The `bytecode_format` module *as named*.** The ids are right; the framing is
  wrong. It is not a bytecode format, it is the language's word-and-path registry,
  and it belongs in the hot path rather than in a serialiser. New home:
  `src/satellite_words/`.
- **The fixed four-segment `SAT_PATH`.** No word limit — the trie has no depth limit
  (DESIGN §4.5).
- **The `100ms` special case** in `eval_call`, which matches on the argument
  *expression* before evaluating arguments and hardcodes a string comparison against
  `satellite.console.display`. With a real registry, `display` **declares** that it
  accepts a pace argument.
- **Housekeeping.** No `.o` files committed beside sources. No tarball blobs in the
  tree. No scratch test directories at the root. Build output lives out of source.

---

## 8. Milestones

Each milestone is a thing that **works and can be demonstrated.** No milestone is
"the parser is half done."

**The numbers are positions and the list is build order, and since 2026-08-30 they
agree.** Read the list top to bottom; a milestone's number is where it sits. The
milestone after M5 is M6, and on the day that stops being true this section is
wrong rather than subtle.

**This reverses the rule that stood from 2026-08-28 to 2026-08-30**, which was
*the numbers are assignment order, the numbers are names and not positions, never
renumber and never reuse.* That is WORD_NUMBERS §1.2 applied to the plan instead
of to the language, and **it is right about the language and wrong about this
file.** A `PathId` is written into every `.satc` on disk, so renumbering one
changes what a cached program means without touching the program — SATC §2's
digest exists for precisely that failure. **Nothing reads a milestone number but a
person**, and the only question a person asks a milestone list is *what is next.*
Under the old rule the answer was M14, the last finished milestone was M5, and
getting from one to the other took a decoder.

**What the reversal costs is that every number from M6 to M24 now names different
work, and here is the whole of it.** No work moved and no milestone changed what
it is; the labels changed. This table is the key to every document written before
2026-08-30 and it is the reason it is in the plan rather than in a commit message:

    old     new   what it is
    -----   ---   -------------------------------------------------
    M11.A   M1.5  the window                    LANDED 2026-08-27
    M14     M6    the machine limits — file, pool, ceiling
    M6      M7    resolve
    M6.5    M8    satellite.variable.number
    M7      M9    the value model and closure compilation
    M8.A    M10   the console, and the first program that runs
    M9      M11   scalars and control flow
    M15     M12   the variant, and what "nothing" is
    M16     M13   the clock and the dice
    M17     M14   the console's other half
    M9.5    M15   satellite.variable.float
    M10     M16   containers and the search power
    M8.B    M17   hello world
    M8.5    M18   satellite.help
    M18     M19   persistence — files and directories
            M19.5 binary and hexadecimal   LANDED 2026-09-08 / 09
            M19.6 the `.satc` after resolve, and the option token
                                           LANDED 2026-09-09
    M19     M20   the machine's facts in the language
    M20     M21   a piece of QUAD, running
    M11.B   M22   the prompt
    M12     M23   threads
    M13     M24   windows
    M21     M25   another file — satellite.include and satellite.analyze
    M22     M26   spacesuits
    M23     M27   the network
    M24     M28   Satellite Orbit and the wire format

**M1 through M5 do not move, and M4.5 keeps its decimal**, because each of them
has a note in `MILESTONES/` and a commit that landed it. A number that has been
written into the history is a fact and not a label, which is the one place the
rule this pass reversed still holds.

**The decimals and the letters are gone, and the renumber is what dissolved
them.** M6.5, M8.5 and M9.5 were interpolations from before 2026-08-28, and M8.A /
M8.B and M11.A / M11.B were splits: two halves of one milestone that turned out to
build far apart. **A letter was the old rule's way of saying "these are one thing
in two places", and it stopped being true the moment the halves separated** — the
console and hello world build seven positions apart, and M10 and M17 say that where
M8.A and M8.B hid it. The one surviving decimal is M1.5, and it is the shape M4.5
already is: a milestone that landed between two others.

**M1.5 is the finding this pass turned up, and it is the same finding as M11 and
M16's.** The window landed on 2026-08-27 in `9f7f71d` — the GTK4 + VTE binary, the
`.desktop` entry, `050-build.mk`'s "four binaries and two", and `satl --repl`
answering that the prompt is not built yet, which is the whole of its done-when —
and **it was still sitting twenty-third in the build order with no note in
`MILESTONES/`.** The 2026-08-30 pass that made every milestone name its commit
caught M11 and M16 being mis-labelled as landed and did not catch this one being
mis-labelled as unlanded, which is the same error with its sign flipped. It landed
between M1 and M2 and it is now named for where it landed.

**Three places keep the old numbers on purpose, and the table above is the key to
all three.** `SCRATCH.md/` is dated records — MILESTONE_DRAFTS.md alone holds 464
of them — and rewriting a review to agree with a decision taken after it would
falsify the record. [PLAN_ONE.md](PLAN_ONE.md) is the superseded draft and carries
its **own** M1–M13 in its §7, which were never these numbers and must not be read
as them; that file already says why it is recorded rather than edited.
`prototype/M3` through `prototype/M11` are directory names on nine drafts, and a
draft is dated by construction. **A bare "M8" in text written before 2026-08-28
means the console and is M10 here**; a bare "M8" that names hello world or DESIGN
§3 as its done-when is M17; a bare "M11" predates the 2026-08-27 split and means
M1.5 and M22 together.

**Eleven milestones were added on 2026-08-28, and they are M6, M12, M13, M14, M19,
M20, M21, M25, M26, M27 and M28.** `SCRATCH.md/MILESTONE.md` had counted **121 of
WORD_NUMBERS.md §2.2's 222 numbered paths reached by no milestone at all** — more
than half the language — and four milestones drafted on 2026-08-27 to cover the
largest blocks had been sitting in `SCRATCH.md/MILESTONE_DRAFTS.md` ever since,
because a lens found real errors in every one of them and *a draft with a known
error in it is worse in §8 than out of it.* Those four are corrected and are here.
The rest of the 121 are covered two ways: by seven further milestones, and by
**naming clauses added to milestones that already owned the work and did not say
so** — this document's oldest recurring failure, and its cheapest fix, applied for
the fourth and fifth time.

**"Later, in no fixed order" is gone, and emptying it was most of what that pass
did.** Every one of its eight entries now has a milestone that names it, which is
the only thing that was ever wrong with it: a pile is not an order, and this
section's own closing sentence had been calling it *"not a milestone"* while
`satellite.random`'s sixteen rows of working, tested v1 code sat in it.

**What that pass did not do is make the language smaller or the work smaller.**
Naming a path is not building it. Several of the milestones below are mostly a list
of decisions only the author can take, and they say so in their own done-when the
way M15 does; **§8.2 counts them** rather than letting a future audit rediscover
them. The claim it makes is narrower than "everything is scheduled" and is the one
the ledger asked for: *every numbered path is named by exactly one milestone, and
every milestone that cannot start says what it is waiting for.*

**M1 — `satl` exists and says how to use it.** *Landed 2026-08-26.* §1.

**M1.5 — the window. LANDED 2026-08-27** (`9f7f71d`), and
[MILESTONES/M1.5.md](MILESTONES/M1.5.md) is the review. *(Split on 2026-08-27 from
a milestone then called M11 and built the same day — the window landed ahead of
the prompt it will host. It was called M11.A and sat twenty-third in this list
until the 2026-08-30 renumber, which is the pass that noticed it had landed; it is
numbered for when that happened.)* The GTK4 + VTE binary, and the `.desktop` entry
joins the install here (§5.3), because the entry names a binary and now there is
one.

`satl-term` is a **fourth binary and not a fifth**: it links the window and nothing
of the runtime, and spawns the installed `satl` into its PTY, so it never interprets
and has nothing for `-march` to act on. It is built at the baseline like
`satl-cpu-level`, and it gets the haswell interpreter for free by spawning whichever
`satl` the installer chose. §4.2's table, `050-build.mk`'s "three binaries on x86-64
and one everywhere else", and LAYOUT.md's build-output table all become four and two
at this milestone. Measured on this machine 2026-08-27: **`satl` resolves 6 shared
objects and `satl-term` 79**, which is the split of §4.4 in one line.

**It is the same window the language hands out**, and that is why it is a milestone
rather than a build artefact. A program asks for one with

    satellite.variable.window my_console =
        satellite.window.console.new("window_title", 800, 600)

— a string and two numbers — so the title and the size are **arguments** in
`satl-term` too, reached as `--title` and `--size 800x600`. The binary is the first
caller of that signature and must not be a special case of it; M24's `dlopen`'d
library calls the same three values in. **`satellite.window.console` is `1 24 2` and its
`new(title, width, height)` is `1 24 2 1`** *(assigned 2026-08-28)*, so this
milestone's paths are real. It is a **different node from `satellite.window.new`
`1 24 1`**, which is still M24's and still reached by nothing that names it.

Done when — **and every clause of it was met on the day it landed**: `satl-term`
opens, spawns the `satl` beside it, and renders what it prints, which today is
`satl --repl` saying the prompt is not built yet.

**`satl` now hands itself over to this binary when it was started with no
console**, which is DESIGN §10.4 and was built 2026-08-28. It is here rather than
in a milestone of its own because it is one function and it only means anything
once this binary exists: opened from a file manager or a desktop menu, `satl` has
nowhere to print, and a program that runs correctly and shows nothing is
§1.1's failure exactly. **The test is a controlling terminal and not
`isatty(stdout)`** — the obvious version breaks every pipeline — and the recursion
terminates because this binary spawns its child on a pty. `--no-window` and
`SATL_NO_WINDOW=1` turn it off; `src/programs/window_handover.cpp` names all six
conditions under which it declines.

**M2 — the namespace trie and the path interner.** *Landed 2026-08-28.*
- `src/satellite_words/words.def`, written as a **tree**: each entry names its
  parent, and its position among that parent's children *is* its number. It is a
  transcription of [WORD_NUMBERS.md](WORD_NUMBERS.md) and nothing else — that file
  is the authority and this one is the copy a compiler can check.
- the trie, the spelling interner, and `PathId`.
- `words.hpp` as the consumer, with the static_asserts **in the header** so every
  future consumer inherits them. What they check, given per-parent numbering: every
  parent's children are dense from 1 with no holes and no duplicates, every named
  parent exists, and no node is its own ancestor.

  **As first written that check failed, and it failed on the numbering it was
  checking.** *(Found 2026-08-27, resolved 2026-08-28.)* WORD_NUMBERS.md §2.2 holds
  **three duplicate numbers** and they are deliberate: `satellite.random.fast.range(min,
  max)` is `1 7 5`, `.normal.range` is `1 7 8` and `.ultra.range` is `1 7 11`, each an
  **alias** of the call shape above it (§2.3). They are the *only* duplicates in the
  language — every one of the 218-versus-215 disagreements across these documents is
  these three rows and nothing else.

  **So "no duplicates" is false as stated, and the check is these four properties
  instead**, which is the form QUAD.md §4 states and the form that survives:

  1. **No holes.** Every parent's children are dense from 1.
  2. **No duplicates *among non-aliases*.** Two entries may share a number only when
     one is declared an alias of the other.
  3. **No orphans.** Every named parent exists, and no node is its own ancestor.
  4. **No alias points at a number that does not exist.**

  **The load-bearing part is property 2's escape clause, and it is a `words.def`
  requirement before it is an assert requirement.** An alias has to be *declarable*
  in the file, or the check cannot tell a deliberate duplicate from a typo — and a
  check that cannot tell those apart is worth nothing, because the three real
  aliases would train whoever hits it to loosen the assert. §2.3's model is what the
  syntax must express: **an alias is one node with a second spelling, not a second
  node.** That also settles the count — 218 rows, 215 numbers — and the same
  mechanism carries §7.7's six spellings of `arguments`, which are the same shape at
  a different scale.

  **This no longer blocks M2.** Write the four properties, give `words.def` an alias
  form, and the transcription can start.
- **a digest over `words.def`**, so a `.satc` can name the numbering it was written
  against and a changed numbering stops every stale cache being read on the same
  instant. SATC.md §2 is why; M4.5 is where it gets used.
- **a live child counter on every node**, so any node can be asked for the next
  number free under it. See §8.1 below — this is not optional and it is what the
  milestone did not originally know about.
- `satl --words` dumps the tree with each node's number — **the registry gets a
  consumer in the same milestone it gets written**, which is the one thing the first
  satellite did not do. It shipped three commits where the registry had zero
  consumers, which is how two sections assigned kind 4 to different things and
  neither noticed.
- a test proving `satellite.console.display` walks to `1 5 1`,
  `satellite.random.normal` to `1 7 2`, and that both intern to stable `PathId`s.
  The numbers come from WORD_NUMBERS.md and the test is how we know the
  transcription did not drift.

**What landed, and the four things the milestone decided on its way.**
*(2026-08-28.)* `src/satellite_words/` is nine files plus `words.def`;
`tests/words_test/` is the tree's first test; `satl --words` is the consumer.

- **`words.def` holds 254 nodes and 9 aliases**, and 254 is a count no document
  had. §2.2's 219 numbers do not include the bare shape a `(0)` marker names —
  the marker rides on the row of the node it belongs to, so it is a number that
  section carries without counting. 216 numbered rows + 38 bare shapes = 254.
- **A number is a position and is not a column.** The `// 1 5 1` ending each row
  is a comment nothing reads. That closes FORMAT/CXX.md §9's first question and
  it is what makes the next point necessary.
- **Only one of this milestone's four properties is a `static_assert`, and that
  is the encoding paying off rather than three being forgotten.** No holes and no
  duplicates are BY CONSTRUCTION — a number is a position, so the file cannot
  express either. No alias pointing at a number that does not exist is BY THE
  COMPILER, because an alias names an identifier. **Only "no orphans, no node is
  its own ancestor" needed asserting.** `words_invariants.hpp` says which is
  which, and adds eight more the encoding needs and §8 could not have known to
  ask for.
- **The transcription is the one thing no assert can reach, so the test reads
  the authority.** A row left out of `words.def` does not leave a hole; it
  silently renumbers every sibling after it, and *both files stay internally
  consistent.* `tests/words_test` therefore opens WORD_NUMBERS.md, parses §2.2
  and walks all 227 paths. **Verified by mutation on 2026-08-28**: deleting one
  row — `satellite.console.typed()` `1 5 5` — compiles clean with every assert
  passing, and the test names the missing row and the four siblings it shifted.

**The call-shape question is answered and the two depths are one rule.**
WORD_NUMBERS §4 asked for confirmation *before* `words.def` encoded it, and §1.3's
definition of `(0)` on 2026-08-28 had already settled it: **a word reached bare
holds its shapes as children (`include()` is `1 1 0`); a word only ever called
does not exist apart from them, so its shapes are siblings (`input()` is
`1 5 2`).** `words.def`'s header carries the argument and `match_shape()` is the
rule in ten lines.

**M2 is not threaded, and that is a decision rather than an omission.** §4.5.1
already argues that threading the fixed table at startup is a guaranteed loss, and
M2 settled it twice over: the tables are `constexpr` and land in rodata, so there
is no startup work left to thread at all.

**And M2 is what finally made §4.5.1's crossover measurable**, because the walk it
is a crossover *of* did not exist until this milestone. Taken 2026-08-28 and
written into §4.5.1: **~2,650 satellite-rooted lines** before 24 freshly created
threads beat one, and **~170** before 24 threads from a warm pool do. That is the
number this plan had been holding open since 2026-08-27, and it says the
line-per-thread request only pays against the pool §4.5.1 already proposed.

**The seed is wide** — the whole first-satellite word surface, not just what M10–M16
needs. *(Settled 2026-08-27.)* This closes what this section used to hold open.
`SCRATCH.md/WORD_SURFACE.md` is the inventory it is seeded from: 111 real paths
found by sweeping the v1 registry, the v1 evaluator, every v1 `.satl` program, the
v1 design documents and this tree's own two, with each source swept once and then
attacked by a second reader looking for what the first missed.

**It got wider on 2026-08-27**, when settling QUAD.md §3 added 71 numbers and took
WORD_NUMBERS.md §2.2 from 144 entries to 215. Most of them are **selectors rather
than paths** — the map's nine methods and the string's sixteen, where both had none
at all — and WORD_NUMBERS.md §1.5 is the distinction: a path is numbered in a `.satc`
and a selector is numbered only for dispatch, because PLAN M4.5 writes the file from
the parse tree and a selector's identity is not known until M7's resolve. Both kinds
go in `words.def`; only one kind ever appears in a cached program.

Nothing executes. This is the spine.

### 8.1 The numbering is partly dynamic, and that changes M2

A user's capsules and spacesuits get numbers too — **the next one free under the
node that owns them, allocated when the name is first met** (WORD_NUMBERS.md §3,
DESIGN §4.3). `satellite.library.main` is `1 14 1` because the language put it
there; a user's `x` is `1 14` followed by whatever is free.

So M2 builds two things that look alike and are not:

| | the language's words | the user's names |
|---|---|---|
| numbered | ahead of time, in `words.def` | at parse time, as met |
| frozen | forever, across every program | for one run |
| checked by | `static_assert` in the header | nothing a compiler can see |
| a `PathId` is | stable and quotable | valid inside one run only |

Three things follow, and each is a way to get this wrong:

- **The header's static_asserts cover the frozen half only.** That has to be said
  in the header itself, or it reads as a guarantee about the whole trie when it is
  a guarantee about part of it.
- **Anything that persists a PathId must record the name instead.** Satellite Orbit
  and the wire format are the two that will want to, and both are far enough out
  that the rule needs writing down now rather than remembering later.
- **`satellite.library.<name>` means the library registry is reachable at M2**, at
  least as a counter, years before the milestone that builds it. ~~Decide whether M2
  owns a real allocator or a stub, and say which in the code.~~ **Decided
  2026-08-28: a real allocator.** The counter was never optional — this section
  requires it — and once it exists, refusing to hand out the number it is holding
  buys nothing and leaves a second thing to build later.
  `src/satellite_words/words_runtime.hpp` is the record, as this asked.
  **Its caller arrives at M4**, which now says so.

**M3 — the lexer.** **Landed 2026-08-29.** Tokens, spans, the reservation rule. A
known word carries its **spelling** out of the lexer — DESIGN §4.4's interner id,
not a `PathId`; a user-owned bare word carries its text. DESIGN §5. *(This sentence
said "node identity" until 2026-08-30, which is the wording the paragraph below is
about; DESIGN §5.6 now carries the correction and the account.)* `src/lexical_analyzer/`, and `satl --tokens <file>` is the consumer
it gained in its own milestone. [MILESTONES/M3.md](MILESTONES/M3.md) is the review.

**"Node identity" turned out to be a SPELLING and not a path, and the distinction is
invisible to the compiler.** *(2026-08-29.)* `words_spellings.hpp` says
`using SpellingId = PathId`, so the two are the same 32 bits and a lexer that hands
the parser the wrong one compiles clean and dispatches on a number that means
something else. DESIGN §4.4 settles which it is — the interner is *"deduplication,
not identity"*, because `list` under `container` and `list` under `directory` are two
nodes sharing one string — and §4.5 says a `PathId` comes from a **walk**, which
happens once a whole path has been read and is therefore M4's. A lexer sees
`display` and cannot know whose it is. **A first draft of the lexer got this wrong
and built**; what catches it is a word the language spells twice, and `console` is
the one the test uses.

**It also pulls `satellite_string` forward from M9**, and that is recorded here
because an unsaid hand-over is the failure this document names most often. DESIGN
§5's opening sentence — *"the lexer walks `SatString`, so the code table in
`satellite_string.hpp` **is** the language's alphabet"* — makes the port M3's
dependency, and §6.1 has the survey. Only the **character half** came across; the
live values are still M6's, and §6.1 says what was left behind and why nothing in
the lexer can reach it.

**It also owns `satellite.variable.binary` `1 6 5` and `.hex` `1 6 11`**, and that was
unsaid until 2026-08-27. DESIGN §8.5 makes them real types **with literals** —
`x00FF` and `b1010`, where **the width is part of the value**, so `x0009` is not `x9`.
A literal is lexed, so the lexer decides them whether or not a milestone says so.
`hexadecimal` is one of the language's aliases for `hex` (WORD_NUMBERS §2.3), which
the lexer's spelling table has to know.

**"One alias" was wrong and it was wrong in a way that hid work.** *(Corrected
2026-08-28; the six/three split below landed 2026-08-29.)* §2.3 has **three rows**, not one: `hexadecimal`; the three
`satellite.random.<tier>.range(min, max)` spellings, which are the only duplicate
numbers in the language and are M13's; and `arg` `args` `argz` `argument`
`arguments` `argumentz` — *one node, six spellings* (DESIGN §7.7), which is M20's
headline demonstration and M7's to recognise at resolve. **M2's `words.def` landed
holding nine aliases**, so the mechanism exists and what this milestone owes it is
the lexer's half of the spelling table.

**Six of the nine are lexical and three are not, which that sentence could not have
known.** *(2026-08-29.)* An alias is written relative to its node's parent and is
**free to carry a dot**, and that is the line the lexer cuts on: `hexadecimal` and
the five extra spellings of `arguments` are single bare words, so `intern_word()`
resolves all six to the aliased node's own spelling id and *one node, six spellings*
becomes true of the token stream rather than only of the registry. The three
`satellite.random.<tier>.range(min, max)` rows rewrite a **two-segment path**, which
no amount of looking at one bare word can decide — they stay M13's, and the filter
that leaves them there is one `find('.')`. `range` is a node spelling nowhere in the
language, so the test can assert the filter held by asking for it and getting
nothing back.

**M4 — the arena AST and the parser. LANDED 2026-08-30**, and
[MILESTONES/M4.md](MILESTONES/M4.md) is the review: what it built, the three
things DESIGN §6 did not settle and how they were settled, what being M2's first
caller found, and the eight things left open. `uint32_t` node indices into a contiguous
arena, no `shared_ptr` anywhere in the tree. `satl --unparse file.satl` round-trips,
which is how we know the parser is right before anything can run.

*A `Node` came out at **24 bytes** against the first satellite's 96, and both
numbers are `static_assert`s rather than claims. `satl --unparse` round-trips the
four acceptance programs in `example/`; the two sketches beside them do not parse
and M4.md §6 item 1 says which construct in each and whose decision it is.*

**It is also the first caller of M2's name allocator.** *(2026-08-28.)* §8.1
requires every node to keep a live count of its children so a user's capsules and
spacesuits can take the next number free under the node that owns them, allocated
**when the name is first met** — and the parser is what meets a name for the first
time. `words::Words::intern(parent, name)` is built, tested and called by nothing
until here. Two things M4 inherits with it: a name the language already owns under
that parent is **refused** rather than renumbered, and DESIGN §2's reservation rule
decides at M7 whether that refusal is the right policy; and a user's `PathId` is
valid **inside one run only**, so M4.5's `.satc` writer must record the name.

**It owns all eleven of DESIGN §6.1's segment-1 words, and naming them is a fix rather
than an addition.** *(2026-08-27.)* §6.1's table is authoritative: a segment-1 word
either has a parse rule of its own or it does not, and these are the ones that do —

> `variable`, `container` · `library` · `statement` ·
> `include`, `capsule`, `spacesuit`, `return`, `returns`, `protected`, `public`

Until now this milestone described the arena and the round-trip and referenced none of
them, so eight of the eleven appeared in `SCRATCH.md/MILESTONE.md` as work with no
milestone when the truth was **a milestone owned them and did not say so.** That is a
worse failure than an unscheduled namespace, because nothing looks wrong.

**§6.1's table is the checklist**, and it is how the last gap of this kind was found:
a word present in that table and missing from the numbering is a word the parser
cannot reach. All eleven are now numbered — `returns` at `1 21` was the last, and
`statement`'s four children are `1 13 1`–`1 13 4`.

`satellite.returns` is the one to be most careful with. DESIGN §13 records the
return-type syntax as **decided** — an optional `satellite.returns(TYPE)` after the
parameter list, defaulting to the `satellite` type, which leaves hello world
byte-identical — and §6's grammar already has the rule written. It is not new work;
it is work that had no name in this list.

**M4.5 — `.satc`. LANDED 2026-08-30**, and [MILESTONES/M4.5.md](MILESTONES/M4.5.md)
is the review. The cache [SATC.md](SATC.md) specifies: check for a `.satc` before
walking a source, read it when its three header lines match, and write a fresh
one afterwards on its own thread. It lands **after M4** because it serialises
a parsed program and there is nothing to serialise before the parser exists, and
**before M5** because a malformed `.satc` is the first thing in the language that
has to say something to a user in plain words — and that sentence is now written,
in `src/satellite_cache/read.cpp`, as the model DESIGN §9 asks for. Its digest is
over **the numbering** and not over `words.def`'s bytes; SATC §6's second bullet
is where that was settled and why, and M2 produces it as a `constexpr`.

*`satl --satc file.satl` is the consumer and it runs the whole loop rather than
only printing one: it reads the cache when it hits, walks and writes when it
misses, prints the `.satc` on stdout either way, and says on stderr which of the
two happened. **The done-when is a fixpoint one step longer than M4's** — write a
program, read the file back, write that, and the two files are identical — which
is checked over all four acceptance programs.*

**Three things it decided that SATC.md did not say, all now corrected there.**
Every `.satc` lives in `$HOME/.satl/cache` rather than beside its source, which
closes §6's first bullet. A path is written `#1.5.1` and not `1.5.1`, because
`satellite.console` closes up to `1.5` and so does the float one-and-a-half. And
§5's write thread is **joined and not detached**, because a detached writer is a
thread a sub-millisecond process exits out from under, so the cache would never
actually be on the disk.

**What it does NOT yet buy is the numbering, only the walk.** The tree a reader
hands back has nowhere to put the `PathId`s the file already carries — `ast.hpp`
reserves that side table for M7 — so **M7's resolve has to learn to skip a path
the `.satc` has already numbered**, and until it does, a cache hit saves the walk
and nothing else. SATC §4.1 and M4.5.md §5 both carry it; it is written down
rather than left for whoever wonders why the cache is not faster.

**M5 — the error reporter. LANDED 2026-08-30**, and
[MILESTONES/M5.md](MILESTONES/M5.md) is the review. Built **before** the
evaluator, deliberately. Codes, spans, a source excerpt with a caret, notes with
their own spans, and "did you mean" over the trie level that failed. Every
milestone after this reports properly from its first commit. Retrofitting this is
exactly how the first satellite ended up with 200 bespoke message sites.

*`src/error_reporter/` is `errors.def` and four files over it, and
`satl --check file.satl` is the consumer: nothing on stdout ever, one block per
problem on stderr, and the exit status is the answer. `satl --errors` is the
second one, and it is `--words` a registry later — a code exists to be looked up,
so a code registry with no way to look a code up is not a smaller version of the
feature.*

**Three things it decided that no document had.** A code's number is a **column**
where a word's number is a **position**, and the two registries encode
oppositely on purpose: a word's number is what a program means and a code's is
what a person searches for. The **arity is checked at the call site** —
`errors::make<Code::X>` is a template, so a site handing two arguments to a
three-hole sentence is a compile error naming the code, which is the one check
199 bespoke sites could not have had because there the sentence was the argument.
And the first two digits of a code say **who is complaining**, with `S05xx`,
`S06xx` and `S07xx` reserved for M7, M8 and the evaluator rather than left to
be taken by whoever appends first.

**It closed the exit status three milestones had been carrying.** `EXIT_MALFORMED
= 1` — MILESTONES/M3.md §6 item 2 opened it, M4.md §6 item 3 added the second arm
and M4.5.md §6 item 5 the third, and each declined to invent it because
`opening.hpp` exists so that two arms cannot disagree about what a failure is
worth. It also closed `TokenKind::Error`'s missing code, `ParseError` (the type is
gone, not wrapped), and **DESIGN §4.6's own worked example**, which had been
written since M2 and answered by nothing: `satl --words satellite.consle.display`
now says *did you mean `console`?*, and so do four sites in the parser.

**The finding to carry forward is what a message registry makes possible.** It
removes two sites saying one thing in two ways, and it creates a defect bespoke
strings did not have — **a site raising the wrong row**, which renders perfectly,
builds quietly, and describes a different problem. No assert can see it;
`tests/reporter_test/parsing.cpp` catches it with one assertion per row the
parser owns — nineteen of twenty-two, the other three named as unreachable from
a program — all of them through `parse()`, and that is the check M7 and M9 have
to extend when they take their blocks.

**M6 — the machine limits: the file, the pool and the ceiling. LANDED
2026-08-30**, and [MILESTONES/M6.md](MILESTONES/M6.md) is the review. *(New
2026-08-28. After M5, before M7.)* `satellite_config.ini` (§4.5), the thread pool
§4.5.1.2 rules starts at startup **always**, the memory watchdog §4.5.2 describes,
and the three fact readers all of it is built out of. It closes three of
`SCRATCH.md/MILESTONE.md` §3's rows at once, and those three were the oldest
un-milestoned work in this plan — specified in §4.5 on 2026-08-27 and never
scheduled.

*`src/machine_limits/` is five sources over two headers and `src/system_facts/`
is the three readers; `satl --limits` is the consumer, and it is the whole
visible surface of the milestone, because nothing in the language reads a limit
until M8 and nothing runs until M10. `satl --watchdog` is the second one and it
exists because the done-when below cannot be met without it — see the correction
after it.*

**Five numbered paths, and it owns one of them as behaviour**:
`satellite.library.system` `1 14 2 (0)` and its four dials — `division_digits`
`1 14 2 1`, `max_depth` `1 14 2 2`, `min_free_mb` `1 14 2 3`, `float_digits`
`1 14 2 4`. The node and the storage land here; **`min_free_mb` is the only one
whose meaning is this milestone's.** `division_digits` is M8's, because `Number`
reads it (§6.1, open question 1); `max_depth` is M9's by DESIGN §7.5, and as of
2026-09-01 it is a memory ceiling on the control stack in bytes — which is what
took M16's search walk off it as a second consumer, because a ceiling in bytes is
not a count of levels a search can compare itself against; `float_digits` is M15's, and DESIGN §13 has already redefined
it from "the dial" into **the default length of a float's right half**. A milestone
that quietly built all four would be building three other milestones' decisions.

- **The file.** `THREAD_COUNT`, `CORE_COUNT`, `MEMORY_MAX`, read at startup and
  refused in M5's plain words when it is malformed rather than ignored — which is
  the same argument M4.5 makes about a bad `.satc`, one milestone earlier in the
  reading order and one later in this list.
- **The pool**, with §4.5.1.2's two implementation requirements as its
  specification rather than as advice: the main thread **spawns one thread which
  builds the other 23** — ~20 µs on the main thread instead of ~590 µs — and
  **below ~170 satellite-rooted lines the walk stays single-threaded**, which
  §4.5.1.2 says is a rule the pool's owner enforces. Both numbers were measured on
  2026-08-28 and both are in §4.5.1.1 beside the decision they justify.
- **The watchdog.** §4.5.2's instruction is *"keep the thread, keep the exit path,
  change what it compares, and ideally keep both checks"*, so both are here:
  `process_memory_bytes()` against `MEMORY_MAX`, which is the stronger promise and
  the one nothing has ever demonstrated, and `mem_available_mb()` against
  `min_free_mb`, which is v1's check ported as a regression.
- **The fact readers**, `old_versions/first_satellite/src/system_facts/`:
  `memory_facts.cpp` (236 lines), `host_facts.cpp` (44) and `stack_facts.cpp` (52).

**It lands before M7 because three later milestones cannot be built without it and
none of them says so today.** M8's `Number` reads `division_digits`. M9 bounds
recursion, and DESIGN §7.5 derives that ceiling from `RLIMIT_STACK` rather than
fixing it — which is `stack_facts.cpp`'s `stack_limit_bytes()`; M9's `Str` needs
`mem_total_mb()`, `mem_used_mb()` and `hardware_threads()` besides, because §6.1
records that `satellite_string`'s codes 97, 98 and 99 are **live values resolved at
decode time**. ~~M10's printer thread is the pool's first tenant.~~ *(Struck
2026-09-02 — §4.5.1. It was never a reason for anything: the two that are left are
both real, and this milestone's ordering never rested on the third.)* That is the
machine draft in `SCRATCH.md/MILESTONE_DRAFTS.md` turned inside out: it claimed
these three files for a milestone after M16, and its own lens found every one of
them consumed at M8 or M9. **The seam is between the readers and the language
surface** — the readers are here, and `satellite.system`'s twenty-eight paths are
M20.

**`parallel_for` does not exist, and this milestone is where that stops being
invisible.** §4.5.1.2's decision — start the pool always, because most satellite
code will call `parallel_for` — rests on a construct that is in no numbering, no
document and no milestone, including this one. The pool is still right without it,
for the reason §4.5.1 gives on its own terms (amortisation across a run, the second
tenant onward), and **naming the missing construct is the honest form of the
dependency.**

**Blockers, all three of them §4.5's own and none of them this milestone's to
take — and all three were taken by it, 2026-08-30**, because a milestone whose
done-when needs an answer cannot land without giving one. Each is restated with
its answer; §4.5.3 and §4.5.4 carry them where the question was asked, and
MILESTONES/M6.md §3 carries the arguments.

- **What unit `MEMORY_MAX` is in, and what its default is** (§4.5.4). 61.9 GiB and
  64.9 GB are the same memory. The watchdog's headline check cannot be written
  until this is answered, which is why it is a blocker and not an open question.
  → **The file writes the unit and a bare number is refused with a caret**; the
  default is the whole machine, and a file claiming more than the machine has is
  clamped to it and told so.
- **Whether the file seeds the namespace** (§4.5.3, *"Not yet decided"*). It is the
  difference between one authority at runtime and a reconciliation rule.
  → **It seeds them, and the namespace is the only authority afterwards.** What
  §4.5.3 was missing is that the file holds two kinds of line: three machine
  settings that are in no numbering, and four dials that are.
- **Whether a setting may differ from a fact** (§4.5.4). `THREAD_COUNT` says what
  satl may *use*; `arguments.machine.threads` (DESIGN §7.7) says what the machine
  *has*. The answer decides whether M20 builds one reader or two.
  → **They may differ, and M20 builds one reader.** `THREAD_COUNT` is in no
  numbering, so nothing in the language can reach it by accident; `satl --limits`
  prints both side by side.

**Done when** `satl --limits` prints every value it is holding to and where each one
came from — the file, or the OS fallback §4.5.1.2 specifies, which is what the OS
reports and **not** cores × 2; a malformed file is refused with a code, a span and
a caret; `satl --words` shows the pool parked before the walk begins and the walk
itself single-threaded, because 254 nodes is far under the 170-line floor; and
**the watchdog fires** — `MEMORY_MAX` set below what satl is already using, exit
status 2, one line in plain words on stderr, and the process gone within a second
of the threshold being crossed. A watchdog that never fires is indistinguishable
from no watchdog, so the demonstration is the one that kills the process, and §9
means the *installed* binary.

**Every clause of that was met on 2026-08-30 and two of them were met
differently from how they are written, so both are corrected here rather than in
the review alone.**

- **The exit status is 4 and not 2.** 2 is v1's `_exit(2)` carried forward, and
  in this tree `programs/opening.hpp` has said since M1 that 2 means *"the
  command line did not name something satl can do"*. A watchdog kill is the
  opposite: the command line was right and the machine ran out, and a script
  testing for 2 would report a memory ceiling as a typo. That enum exists so
  that two arms cannot disagree about what a failure is worth, and its own rule
  — *"assigned by what a failure IS"* — decides it. `EXIT_LIMIT = 4`.
- **The walk is single-threaded, and NOT because 254 is under 170.** It is over
  it. The ~170 figure is §4.5.1.1's crossover in **satellite-rooted source lines
  being interned**, and `satl --words` interns nothing — it prints a `constexpr`
  table, which §4.5.1 settles separately and more strongly: *"threading the table
  at startup is a guaranteed loss ... the table is `constexpr` and lands in
  rodata."* Both halves of the clause hold and only the reason moves.
- **And `satl --watchdog` is a flag this milestone had to add to meet the last
  clause at all.** Nothing satl does today lasts a second — the longest command
  in the tree is `satl --words` at ~0.8 ms — so there is no run for a
  once-a-second thread to fire during. It holds the process open, and it is
  named in the usage list rather than hidden, because a demonstration harness
  nobody can find is one that stops being run.

**The terminal-restoring hook is not here, and that is a correction to the draft
this milestone comes from.** v1 leaves through `run_emergency_exit_hook()`, whose
only registrar anywhere in v1 is the line editor's raw mode — M22's. Until a
prompt exists nothing has put the terminal into raw mode and the hook has nothing
to undo, so a done-when clause asserting *the terminal is still usable* would be a
test that cannot fail. This milestone registers no hook; **M22 inherits the exit
path and adds the registration**, and its line says so.

**M7 — resolve. LANDED 2026-08-31**, and [MILESTONES/M7.md](MILESTONES/M7.md) is
the review. Names to integer frame slots. Capsules, frames, the `SLOT_*`
sentinels. Resolved data in a side table indexed by arena node id, not `mutable` on
the node. DESIGN §7. `src/name_resolver/` is six sources over three headers and
`satl --resolve` is the consumer.

**Three things it owns that its own line did not say.** *(2026-08-28.)*

- **`satellite.capsule` `1 2 (0)`.** M4 parses `capsule_decl`; this is where a
  capsule name becomes a slot, and §7 already orders the work — resolve runs every
  capsule name first, then every spacesuit name (M26), which is what makes forward
  reference work without a second pass.
- **The literal-option fold, WORD_NUMBERS §1.5.** It is a **resolve-time** decision
  that changes which `PathId` a call site interns — `sort("down")` becoming
  `sort_down()` `1 4 2 5` — so it is this milestone's and not M9's.
  `SCRATCH.md/MILESTONE.md` §1 filed it as *"M7 or M9 — nothing says whether resolve
  or closure compilation owns it"*, and the answer is that closure compilation is
  already past the point where the option is a literal. ~~**M19 is waiting on this**:
  whether `satellite.file.open`'s four mode words fold decides whether its bad-mode
  message is M5's suggester or a runtime check.~~

  **CLOSED 2026-09-08 AT M19, AND WHAT CLOSED IT IS A MEASUREMENT THAT CONTRADICTS
  THIS MILESTONE'S OWN CODE.** `name_resolver/numbers.cpp`'s `fold_option` carries a
  comment saying the mode words "fold with no edit to this file, on the day
  `words.def` gains `open_read_append`". **They do not, on two counts.**
  `fold_option` has exactly one caller — `names.cpp`'s `call_target_done`, the
  SELECTOR arm, reached only when the callee is a `Member` over a receiver with a
  declared type — and `satellite.file.open(p, "read")` is a whole-PATH module call
  resolved one arm earlier, which never reaches it. And it inspects **argument 0
  only**, while `open`'s mode word is argument 1. `sort("down")` folds because the
  option is a selector's first argument; nothing about a module's second argument
  was ever built. **The author settled it the same day: the mode words do NOT
  fold**, and the bad-mode message is M19's own runtime refusal through M5's
  *reporter*. The fold stays what M7 built it as — selector calls, argument 0 — and
  this bullet is the record that its generality was overstated by a comment rather
  than by a test.
- **The six spellings of `arguments` become the special variable here.** WORD_NUMBERS
  §2.3's third alias row is one node with six spellings; M2 and M3 hold the spelling
  table that says so, and **resolve is where a parameter named `argz` is recognised
  as §7.7's object rather than as a user's name.** M20's demonstration rests on it,
  and until this pass no milestone claimed it.

**Done when** `satl --resolve <file>` prints every capsule with its frame —
parameter slots first, then locals, each with its slot number, its name and its
declared type — and every path in the file beside the number it resolved to.

- **A name that resolves to nothing is refused** with an S05xx code, a span and a
  caret, and DESIGN §4.6's *did you mean* over the names that are actually in
  scope. A **path** whose segment the numbering does not have is refused the same
  way, over the node that segment failed under — which is the error
  `satellite_cache/paths.cpp` names in its own comment and declines to raise,
  because it is this milestone's.
- **A redeclaration in one scope takes a fresh slot and the dump shows both**
  (§7.4). Two parameters of one capsule with one name is an error, because a call
  has one value for each.
- **`sort("down")` folds to `1 4 2 5`** at resolve and the `.satc` still reads
  `"down"` — WORD_NUMBERS §1.5's *"the file keeps the literal, the runtime keeps
  the number"*, asserted from both ends.
- **A parameter of `satellite.main` named any of §7.7's six spellings becomes the
  special variable**, and a seventh is **refused** when the suggester answers with
  one of the six — `argv` is told to write `arg` — and is an ordinary list
  otherwise. That is DESIGN §7.7's own open question answered: the silence it
  calls wrong is broken where somebody has plainly *meant* the object, and a
  parameter that was never trying to be one is left alone.
- **A capsule called before it is declared resolves**, because pass 1 runs before
  pass 4 — which is the whole reason §7.3 has four passes and not one.
- **Resolve skips a path the `.satc` already numbered**, which is what
  MILESTONES/M4.5.md §5 says has to happen before the cache pays for itself.
  `satl --resolve` **counts** the paths it walked against the paths it took from
  the file, and a warm run's walked count is lower. **Counted and not timed**: on
  a 273-byte program the walk is far under the clock's noise, and M4.5's own table
  is what says so.

**~~The resolver's recursion bound is not §7.5's~~ — WITHDRAWN 2026-08-31, THE
SAME DAY IT LANDED.** This clause asked for a bound, M7 built one at a fixed 2000
levels with **S0501** behind it, and the author's answer on reading the review was
that *"if the recursion shuts the program off after 20,000 then that is not a
working interpreter, that's a broken interpreter."* §2.5 was un-deferred and
DESIGN §7.5 rewritten within the hour.

**What the clause got right is that there were two bounds and one section.** What
it got wrong is that the answer to either of them is a bound. The resolver keeps
its own stack instead, `kMaxDepth` and S0501 both go, and
`tests/resolve_test/frames.cpp`'s S0501 assertions become an assertion that a
2,200-deep program **resolves**.

**And it is not the only walker, which is what the review found and this clause
could not have.** MILESTONES/M7.md §4.5 measured `satl --unparse` dying at 19,000
nested brackets, `--satc` at 20,000 and the parser at 32,000 — three crashes that
were in the tree before M7 and that a bound in the resolver does nothing about.
`SCRATCH.md/NO_LIMITS.md` is the plan for all four.

**Spacesuits are M26 and pass 2 is a named hole.** It exists in the order,
resolves nothing, and **says so**: `satl --resolve` over a file with a spacesuit
in it names the milestone rather than printing an empty frame and letting somebody
believe it.

**Every clause of that was met on 2026-08-31, and three of them turned out to be
narrower or wider than they are written. All three are corrected here rather than
in the review alone.**

- **"Every path in the file" is every path a NAME OR A TYPE names.** A `.satc`
  numbers more than resolve does — `#1.2` for `satellite.capsule`, `#1.13.1` for
  `satellite.statement.if` — because the writer is a text substitution over words
  and those are words. Nothing dispatches on them: DESIGN §6.1 puts that decision
  at **parse time**, on segment 1, and the parser has already made it. A number
  recorded here that no consumer reads is the thing the `SLOT_*` sentinels were
  cut from six to three to avoid.
- **The skip is COUNTED and the draft asked for it TIMED.**
  `SCRATCH.md/M7_START.md` offered "a warm hit beats `--unparse` on
  hello_world", which is a comparison neither command can win: `--unparse` does
  not resolve, so it is not doing the work the skip saves, and
  MILESTONES/M4.5.md §5's own table says the difference is under the clock's
  noise at this size. `satl --resolve` prints the two counts instead, and they
  are exact — 28 walked cold against 14 walked and 14 taken warm on
  `example/frames.satl`. MILESTONES/M7.md §5, and §7 records that a count is also
  the only thing that could catch the skip disappearing.
- **And it gained a clause the done-when did not ask for: S0514.** A file may
  declare `satellite.main` **twice** and parse clean, because `capsule_decl`'s
  reserved arm looks the name up rather than defining it and the numbering has
  nothing to say about a name it did not allocate. Pass 1 is the only place that
  can see it. M7.md §3.2.

**And it took `main.cpp`'s seam, which M6 named and declined.** MILESTONES/M6.md
§6.1 called it "a reshaping of M2 through M5's arms rather than of this
milestone's" and was right to leave it; that file was 427 lines with a fourteenth
arm about to be added. `programs/dump_commands.cpp` and
`programs/file_commands.cpp` are the two subjects, every comment moved unchanged,
and `main.cpp` is **297 lines**.

**M8 — `satellite.variable.number`. LANDED 2026-08-31**, and
[MILESTONES/M8.md](MILESTONES/M8.md) is the review. *(Its own milestone as of
2026-08-27; it was a bullet inside M9.)* The port of the first satellite's `satellite_number` — 10 files,
1509 lines, internally closed, every file already under §3's ceiling (§6.1) — plus the
one thing the port does not bring with it.

**The sign becomes an explicit `satellite.variable.bool` named `positive`, defaulting
to `true`**, and the magnitude never carries one (DESIGN §8.1). A number with no sign
written is positive; something has to flip the flag for it to be otherwise.

**It lands here rather than inside M9 because M9's `Value` contains one**, and because
it is the milestone that builds the sign **both** numeric types share. M15's float
inherits it rather than defining a second one, which is the whole reason the two can
be milestones apart instead of one large one.

Done when: exact arbitrary-precision arithmetic runs, negation and `abs` and ordering
of negatives are right, there is no negative zero, and `sizeof` is inside DESIGN
§8.2's 40-byte `Value` budget — which §6.1 names as the one number that could make
this port not fit, and which is cheap to check first. **All four met**, and
`sizeof` was checked before a line was written, which is what §6.1 asked for and
what decided the layout: 32 bytes flat, 40 with the magnitude cut out as its own
type. **A fifth clause was added when the consumer was chosen** — `satl --number
<a> <op> <b>`, because M6 and M7 each shipped one and §8's opening rule is that a
milestone is a thing that can be demonstrated.

Four things to settle before copying, in `SCRATCH.md/PORTING.md`, and **the sign is a
fifth**: where v1 currently keeps it has to be checked against DESIGN §8.1 before the
copy rather than after. *(All five answered; §6.1 carries the four and §6.1's
closing paragraph carries the sign. Three of the four came out differently from
what the list expected.)*

**Eleven numbered paths, and the four it does not take.** *(2026-08-28.)*
`satellite.variable.number` `1 6 4 (0)` and ten of its fourteen methods land here —
`shift_left` `1 6 4 1` through `round` `1 6 4 9`, and `shift_right(n)` `1 6 4 11`.
**`power(a, b)` `1 6 4 10`, `modulus(a, b)` `1 6 4 12`, `truncate(a)` `1 6 4 13` and
`sqrt(a)` `1 6 4 14` are M15's**, because none of them can be finished before the
rounding rule is chosen: `sqrt` is irrational in general, `modulus` cites DESIGN
§8.6, and truncating a float is its left half. That is
`SCRATCH.md/MILESTONE.md` §0.4's *"either those four move to M15 or M15 moves ahead
of M11"* answered the cheaper way — **four methods move, and no milestone is
reordered.**

**THE TWO SHIFTS MOVED TO M15 AND CAME BACK THE SAME DAY, AND THE ROUND TRIP IS
THE FINDING.** *(2026-08-31, both moves on the author's decision.)* The paragraph
above says `shift_left` `1 6 4 1` and `shift_right(n)` `1 6 4 11` land here.
**The first satellite has neither**, and no document defined what either means:
they enter the language through DESIGN §5.5, as an aside that bit shifts *"if ever
needed"* would be spelled this way — an argument about where a path hangs, not a
specification of an operation. So they were moved out, on the grounds that "what
does a bit shift mean on an exact base-10⁹ decimal" is the same shape of
undecided question as the rounding rule.

**Then the question was answered, and the answer took ten minutes rather than a
milestone.** DESIGN §5.5 now says it: a shift is `× 2ⁿ` and `÷ 2ⁿ`. **2 divides
10, so both directions terminate** — `1 ÷ 2¹⁰` is exactly `0.0009765625` — which
puts both in DESIGN §8.6's first class, *exact and bounded, never rounds*, and
means neither was ever blocked by the rounding rule. A fractional receiver needs
no rule of its own because a multiplication does not care: `7.5` shifted left once
is exactly `15`. The rejected readings are recorded in §5.5 — `×10ⁿ` makes the
name a lie, and refusing a fractional receiver invents a refusal the arithmetic
does not need.

**AND `modulus(a, b)` `1 6 4 12` WAS NEVER M15'S EITHER**, which is the same
finding one method over and the one that says this entry's sentence was too
broad. "None of them can be finished before the rounding rule is chosen" is true
of `power` and `sqrt` and is not true of `modulus`: DESIGN §8.6 specifies it in
full — `a − b × trunc(a/b)`, truncated, taking the sign of the dividend, with
`7.5 % 2.1 = 1.2` worked out in the text — and files it under **class 1, exact
and bounded, never rounds**, in the same section this entry cites as its reason
for deferring it. The quotient is only ever wanted as an integer, so the inexact
tail of a division is discarded before it can matter. `Number::modulo` was ported,
worked, and had an error row apologising for a gap that was not there.

**SO M8 LANDED THIRTEEN PATHS WHERE THIS ENTRY NAMED ELEVEN.** All eleven —
`1 6 4 (0)`, `shift_left(n)`, `max`, `min`, `abs`, `clamp`, `to_string`, `floor`,
`ceil`, `round`, `shift_right(n)` — plus `modulus(a, b)` `1 6 4 12`, taken back
from M15, plus `digits` `1 6 4 15`, appended to the numbering below. **M15 keeps
three of the number's methods** — `power(a, b)`, `truncate(a)` and `sqrt(a)` —
and its own `satellite.variable.float` `1 6 10`, so §8.2's table reads **13 for
M8 and 4 for M15** where it read 9 and 7. The total moves from 222 to 223, and
the one row is `digits`.

**Three of those nine were written rather than ported, and this entry did not say
so either.** v1's number method surface is `abs`, `ceil`, `floor`, `round`,
`to_string`, `digits` and `negate` plus the arithmetic; it has no `max`, `min` or
`clamp` at all. They are one comparison each over `compare()`, so the cost is
nothing — what matters is that "ten of its fourteen methods land here" read as ten
things being carried across, and five were.

**And `digits` had no number, which is the mirror image.** v1 exposes
`.digits()`, and its implementation carries a whole paragraph on why it is not
`.length()` — *"one word means one thing"*. WORD_NUMBERS §2.2 had no row for it
under `1 6 4`, so `Number::digit_count()` was a C++ method the language could not
call — the mirror image of the two shifts, which had numbers and no meaning.
**`1 6 4 15` was appended on the author's decision, 2026-08-31.** §1.2 is *never
renumber, never reuse*, and appending is neither: nothing moved, and no
already-written program changed meaning. §2.2 goes from 222 rows to 223 and from
219 distinct numbers to 220, which is the only count in this document that this
change touches.

**It reads `satellite.library.system.division_digits` `1 14 2 1` and M6 built the
node it lives on.** §6.1's open question 1 is what makes the dial `Number`'s; M6
owns the file, the storage and `min_free_mb`, and this is the first milestone to give
one of the four dials a meaning. **The meaning is one function** —
`limits::division_digits()`, beside the dial rather than inside `Number` — and it
is *unset means 34*, decimal128's precision. `satl --number` prints the count and
where it came from on every run, which is the only way a person can see a
`satellite_config.ini` having done anything at all before M10.

**What it left open was the clamp, and the fix was to delete it rather than to
announce it.** *(2026-08-31.)* `Number::divide` capped a count at 10,000, so a
file setting `division_digits=50000` got 10,000 and nothing said so. The first
reading of that was a missing sentence — DESIGN §1.1 forbids doing things behind
a program's back — and it is really a missing sentence about a constant DESIGN
§7.5 does not allow to exist: *"no constant in a header deciding how big a thing
the user may write."* **`kMaxDivisionDigits` is gone.** `division_digits=50000`
is fifty thousand digits and satl computes them; what ends a runaway is M6's
watchdog at `MEMORY_MAX`, which is the same answer `errors.def`'s S06xx block
already gives for why there is no overflow row.

**What the file may say is bounded, and that is a different claim.** A count
below 1 keeps nothing and a count satl cannot hold in a machine word cannot be
acted on, so both are refused **at the line of the `satellite_config.ini` that
wrote them, with a caret** — S0807 and S0808, rows M6 already had. That also
removed two silent substitutions nobody had noticed: `limits::division_digits()`
turned a zero into 34 and anything past `UINT_MAX` into `UINT_MAX`, both without
a word. **M11 is still where a program can retune the dial at run time**, and it
inherits a function with nothing left in it to explain. *(Overtaken 2026-09-03:
M11 landed without it — no document said what assigning to a language path
MEANS, and inventing that mid-milestone was worse than saying so — and the
author moved the retune to **M15**, whose entry now carries it; the clean
`limits::division_digits()` waits there instead. MILESTONES/M11.md §6 is the
record.)*

**It also owns the uniform draw, and its own line did not say so.** *(2026-08-28,
and this is the fourth instance of that failure after M3/M4's eleven words, M16's
thirty-four methods and M11's `satellite.bool`.)* `satellite_number/random.cpp` — 121
lines: the limb-aligned draw in base 10⁹, the rejection sampler that exists because a
bare `%` would skew 2:1, and `MAX_RANDOM_DIGITS = 100000` — is one of the ten files
this port brings across. `SCRATCH.md/PORTING.md` has the row; this milestone had it
and never repeated it, and **M13 needs to be able to say it is inheriting the bignum
half of the dice rather than writing one.**

**`MAX_RANDOM_DIGITS` DID NOT COME ACROSS, AND NEITHER DID `Bits32`.** *(2026-08-31.)*
Both already exist in this tree, in `satellite_random/random.hpp`, with the same
100,000 and the same argument — the module landed at M2 ahead of any milestone
that calls it. Copying v1's would be two facts in two places each, against
FORMAT §1's second rule. So `satellite_number` includes that header, which is the
one include that makes §6.1's *"internally closed"* wrong by one line, and the
trade is written down in `bignum_bigint.hpp` rather than left to be found.

**And this is what gives `satellite_random` a consumer**, which LAYOUT.md has
called *"the one module in the tree with no consumer"* since M2 and 040-sources.mk
explained by naming this milestone: the half that could not be built then was
*"drawing an N-digit number, which needs the arbitrary-precision half (M8)."*
`tests/number_test/draw.cpp` links both modules. **`satl` still does not** — nothing
in the interpreter draws a number, because `satellite.random.*` reaches no
milestone — so the sentence narrows rather than disappearing.

**The 2:1 skew was re-measured rather than quoted, and it is one level in from
where v1's comment points.** §9's rule is to measure on this machine. The
rejection in `draw_below` is against a raw 32-bit `%`, and for a bound of 3 that
bias is one extra preimage in 4.29 billion — nothing a test can see. The visible
2:1 is the **second** rejection, in `BigInt::random_below`: the top limb is drawn
over `[0, top+1)`, and folding that back with `%` instead of redrawing is what
skews the low residue. `tests/number_test/draw.cpp` runs both over three million
draws and asserts the shape.

**This closes half of `SCRATCH.md/MILESTONE.md` §3's porting row**; M9 closes the
other half with `satellite_string`.

**M8.5 — the walkers keep their own stacks. LANDED 2026-09-01**, and
[MILESTONES/M8.5.md](MILESTONES/M8.5.md) is the review. *(Its own milestone as of
2026-09-01, on the author's decision, and a decimal for the reason M1.5 and M4.5
are: a milestone that lands between two others. The alternative was a dense
integer here and M9 through M28 each moving by one, which is a second old-to-new
table over this section for no gain — no work moved and nothing was renumbered.)*
`SCRATCH.md/NO_LIMITS.md` §5 is the plan and this is all four of its steps.

**DESIGN §7.5 says the language has no depth limit and until this milestone four
walkers broke it.** `satl --unparse` died with signal 11 at 19,000 nested
brackets, `--satc` and `--resolve` at 20,000, and the parser at 32,000 — measured
2026-08-31 at the 8 MiB a login shell hands out, and every one of them a CRASH
rather than a refusal. §2.5 was un-deferred the same day and §2.6 put the fix
ahead of M9: **the four static passes must stop using the C++ stack before the
evaluator is written, or the evaluator gets written twice.**

**The rewrite is four machines and the count is the finding.** The grammar has
four cycles, not one: expressions (`expression → unary → postfix → primary → '('
expression`, plus an argument list and a subscript), statements (`statement →
block → statement`, plus the three compound forms), types (`type →
generic_arguments → type`) and **suit bodies** (`suit_body → section →
suit_body`, which no document had named as recursion at all). The resolver's walk
and both printers are three more. **And the cycles are not nested in each other**
— every edge between them runs one way — so the deepest a program can drive the
C++ stack is one frame per machine, whatever it is nested in.

Done when: `--check`, `--unparse`, `--satc` and `--resolve` answer at 100,000
nested brackets AND 100,000 nested blocks, at the default `ulimit -s`, with the
acceptance fixtures in the test binaries rather than in a scratch file. **All
met**, and the fixtures are `tests/parser_test/depth.cpp`,
`tests/satc_test/depth.cpp` and `tests/resolve_test/frames.cpp` — none of those
binaries links `machine_limits`, so every one of them runs against 8 MiB and a
walker that still recursed would fail rather than pass.

**§2.5's 2–3× IS NOW MEASURED RATHER THAN BORROWED, AND IT IS TRUE OF THE
PRINTERS AND NOT OF THE PARSER.** §9's rule is that a figure decides nothing here
until this project measures it. Measured 2026-09-01 over 32,000 lines of ordinary
satellite, best of nine runs of ten: **parsing is unchanged** — 147.4 ms before,
145.4 ms after — **the unparser's own step went 6.7 ms → 16.6 ms (2.5×)** and the
`.satc` writer's 48.0 ms → 60.1 ms (1.25×). As a share of the command each
printer costs about 5%. The evaluator's figure is still unmeasured and is M9's,
which is the half §2.5 was actually arguing about.

**What it cost in lines, and one file was split.** Making the printer keep its own
stack took `abstract_syntax_tree/unparse.cpp` from 341 lines to 565 — 452 of code,
the widest in the tree — so it is now three files on the seam
`satellite_cache/write_*.cpp` already uses, which is DESIGN §6's own three levels.
**The two printers are twins by construction and now have the same shape as well
as the same walk**, which is what write.cpp's header has claimed since M4.5.

**The `.satc` writer lost a rule rather than gaining one.** Its header carried a
paragraph requiring every subexpression to go into a named local before being
joined, because `note()` appends to the comment column as a side effect and C++17
leaves the operands of `a + b` indeterminately sequenced. A piece is expanded when
its output position is reached, so the order of the comment column is now the
order of the output by construction — a discipline every future line had to
remember, deleted.

**M9 — the value model and closure compilation. LANDED 2026-09-01**, and
[MILESTONES/M9.md](MILESTONES/M9.md) is the review. `Value` (40 bytes, the
static_assert comes too) and `Str`. `Number` arrives at M8 and this milestone is its
first consumer. The arena AST compiles to a closure tree.

**Done when — WRITTEN AT M9, BECAUSE THIS ENTRY DID NOT HAVE ONE.** It is the
first milestone entry since M2 with no done-when at all, which M9.md §2 records
rather than quietly fixing: §8's opening calls a done-when in prose "the weaker
kind" and this had neither. The clauses are what this entry and
`SCRATCH.md/NO_LIMITS.md` §5.5 between them already asked for — `Value` is 40
bytes and the small case never allocates; `Str` answers DESIGN §5's six live
codes from the machine; the arena AST compiles to a closure tree and `satl
--compile` prints it; module calls dispatch through `handlers[path_id]` with
DESIGN §6.4 q2's receiver tag; a call site caches what it resolved to;
`satellite.library.system.max_depth` is read, in bytes, unset meaning the
machine; a capsule 100,000 frames deep answers at the default `ulimit -s`; a
runaway one is refused **in words about recursion** with a status a script can
read; and the evaluator's 2–3× is measured rather than quoted. **All met**, and
the depth fixture went to 1,000,000 rather than 100,000 because 100,000 could
still have been a large fixed number somewhere.

**Its two consumers are `satl --compile` and `satl --call`**, which is M2's rule
that a thing built gets a reader in the milestone that writes it. `--compile`
prints the closure tree the way `--resolve` prints frames — and prints the
REFUSALS, which is the part a user reads: every piece of DESIGN §6's grammar this
evaluator does not run yet, with the milestone that will build it. `--call` names
one capsule and prints what it answered, with no console behind it and no `main`
in front of it, which is `satl --number`'s shape one milestone on.
Module calls dispatch through `handlers[path_id]`, and the **inline caches of §2.4
land here too** — third of the three adoptions §2.6 orders, and the milestone that
owns them.

**~~Recursion depth is bounded here.~~ IT IS NOT BOUNDED ANYWHERE, AS OF
2026-08-31, AND THAT SENTENCE IS THE ONE THING ABOUT THIS MILESTONE THAT
CHANGED.** §2.5 was un-deferred and DESIGN §7.5 was rewritten the same day: the
language has no depth limit, so **M9 compiles onto an explicit control stack**
rather than onto the C++ one, and a satellite program's recursion is bounded by
memory the way a list's length is. What that buys beyond not crashing is §2.5's
own first paragraph — pausable and resumable execution, which is what M13's
`satellite.time`, M22's Ctrl-C (DESIGN §10.2), M23's threads and M24's GTK idle
callback (§10.3) each need and none of which has a milestone that says so.

**This is not extra work bolted on; it is the same work with the stack made
explicit.** §2.3 already argues that closure compilation *"is not bytecode"* and
that the tree stays the tree — a closure tree evaluated against a heap stack is
still that. What it is not is `eval(node)` calling `eval(child)`.

**And M9 will not be first to need it.** §2.6 puts the four static passes ahead
of this milestone, so the tree M9 inherits already cannot crash under a walk;
`SCRATCH.md/NO_LIMITS.md` is the plan and the order.

**Five things it owns and had never written down.** *(2026-08-28. Four of them are
consumed by later milestones that had each assumed somebody else built them.)*

- **`Str` is the port of `satellite_string`, and it needs M6's fact readers.** §6.1
  records that the code table is not only characters: **codes 95–100 are live values
  resolved at decode time**, and 97, 98 and 99 are `threads`, `mem_total_mb` and
  `mem_used_mb`. So this milestone calls `hardware_threads()`, `mem_total_mb()` and
  `mem_used_mb()`, which M6 ports. **The module itself arrived at M3** — the lexer
  could not wait for an alphabet — so what is left here is the live half and `Str`
  itself, not the file. §6.1 has the split and names the six lines. **This closes the other half of
  `SCRATCH.md/MILESTONE.md` §3's porting row**, whose whole complaint was that M9
  needs `Number` and does not say the port happens here.

  **BUILT, AND THE SIX CALLS ARE NOT WHERE THIS PARAGRAPH PUT THEM.** §6.1
  predicted "replacing six lines with six calls" inside `satellite_string/`. They
  are in `satellite_value/render.cpp` instead, one module up, and the reason only
  became visible once there was something to move: **the lexer calls `decode()`
  on every token's text**, a string literal's body included, and `Token::text` is
  what `--unparse` prints back. A live decode inside the alphabet would write this
  machine's thread count into the source of any program containing a `\threads`
  escape, silently, and round-tripping would stop being a fixpoint. So `decode()`
  gained a second entry point taking a table of the six, the alphabet fills none
  of them and the value module fills all of them — which is exactly the split
  `lexer.hpp` already draws between a token's two halves: `text` is what the file
  SAYS and `str` is what the program MEANS.

  **AND THREE OF THE SIX READERS DID NOT EXIST.** M6 ported `hardware_threads()`,
  `mem_total_mb()` and `mem_used_mb()` and left `username()`, `home_dir()` and
  `cwd()` behind, saying in `facts.hpp` that "they are forty lines and they come
  with M9". They came, in `system_facts/user_facts.cpp`, at seventy-five — and the
  difference is `cwd()` losing v1's fixed `char buf[4096]`, which is a constant in
  a header deciding how long a path the user may have and is what DESIGN §7.5
  forbids in as many words.
- ~~**The recursion ceiling is derived, not fixed**~~ — **THERE IS NO CEILING, AS
  OF 2026-08-31.** §2.5 was un-deferred and DESIGN §7.5 rewritten: M9 compiles
  onto an explicit control stack and a program's depth is bounded by memory. This
  bullet used to say the ceiling came from `RLIMIT_STACK` via M6's
  `stack_limit_bytes()`; **that reader now has one consumer fewer**, and M6's own
  note already says `RLIM_INFINITY` answers *unknown* rather than *unbounded* —
  which was the right care to take about a number nothing will read.
- **`satellite.library.system.max_depth` `1 14 2 2` IS A MEMORY CEILING ON THE
  CONTROL STACK, IN BYTES, AND UNSET MEANS THE MACHINE.** *(The author's decision,
  2026-09-01, from three readings `SCRATCH.md/NO_LIMITS.md` §8 carried as open —
  which now carries the argument instead.)* A numbered path cannot be deleted —
  WORD_NUMBERS §1.2 is *never renumber, never reuse* — so the dial had to mean
  something, and the reading picked is the one that keeps its NAME honest: a depth
  measured in what depth actually costs.

  **Three things follow and M9 builds all three.** The check happens when the
  stack GROWS rather than on every push, so it costs nothing in the walk. The
  refusal is a sentence about RECURSION — which is what M6's watchdog cannot give,
  since it can only say the run is using N and `MEMORY_MAX` is M, and
  `SCRATCH.md/NO_LIMITS.md` §8's first question is what that gap was. And the
  default is the machine, which is not a new rule: §4.5.4 settled it for
  `MEMORY_MAX` in the same words, *"a fraction is a number satl would have
  invented about a program it has never seen"*.

  **A number large enough never to fire was considered and refused.** It is
  §4.1.1's *"a bigger number and not the absence of one"* one level up, and DESIGN
  §7.5's kept measurement is the second refusal: v1's ~10000 sat past both stack
  cliffs, so *"the guard could never fire and the segfault it existed to prevent
  was exactly what a runaway recursion got."*

  ~~**THE RANGE AND THE READER LAND TOGETHER, AT M9**~~ — **THE READER LANDED AND
  THERE IS NO RANGE.** `kDialRanges` still reads `{false, 0, 0}` for this dial and
  **the reason under it changed**: it was `float_digits`'s reason, which is "no
  consumer yet", and it is now `min_free_mb`'s, which
  `config_internal.hpp` already spelled two rows away as *"a meaning with no bound
  ever claimed for it"*. Any number of bytes is a number of bytes — zero refuses
  the first push and says so with a caret, and a number wider than any machine
  means the machine — so neither end is a value satl cannot act on, which is the
  only thing S0807 and S0808 are for. **DESIGN §7.5 is untouched by any of this**:
  a ceiling the USER sets on their own program is not a limit the language has,
  which is the distinction M8 drew for `division_digits` in the same words.

  **WHAT THE DIAL DID NEED WAS A KIND, AND M9 FOUND THAT BY TYPING IT.**
  `max_depth=64MiB` was answered with "max_depth is a whole number and `64MiB` is
  not one". Every dial was a bare integer, which was right while no dial meant a
  quantity of memory; this one is a sibling of MEMORY_MAX now and is written the
  way MEMORY_MAX is. §4.5.4's argument for units is the same here as there.
  `kDialKinds` is the table, and it forced apart a test that had been doing two
  jobs — "is this a size" and "may this be a fact" were one comparison, and
  DESIGN §7.7 pairs three settings with three paths, not four.
- **`satellite.library.system.max_depth` `1 14 2 2` is this milestone's dial**, and
  **M16's search walk was named as its second consumer** with a depth error of its
  own. Both entries said so, because "M9 builds it and M16 reuses it" is fine and
  "neither milestone ever says it" is how this gets built twice. **The 2026-09-01
  reading changes what M16 inherits rather than whether it does** — a ceiling in
  BYTES is not a count of levels a search can compare itself against, so M16 reads
  no dial: it is covered if its walk runs on this milestone's control stack, and
  owes its own answer if it does not. M16's entry carries that.
- **The variant is append-only, and three appends are already known.** v1 appended
  `ArgsRef` and `ResultRef` and `sizeof(Value)` stayed at 40 with the static_assert
  holding. The three are: **`Value`'s reference-type handle arm, which M19 appends**
  — DESIGN §8's table makes a file a reference type, two variables holding one file
  share one descriptor, and v1 carries it as `FilePtr`, the eighth of its twelve
  arms; **the arguments object's arm, which M20 appends**; and whatever Orbit's
  result becomes at M28. **Each re-runs this milestone's static_assert.** DESIGN
  §8.2 budgets the 40 bytes and §6.1 calls `sizeof` the one number that could make
  the port not fit, so the rule belongs here rather than in the milestone that trips
  over it.
- **The dispatch table carries DESIGN §6.4 qualification 2's receiver-binding tag.**
  It is the only reason `satellite.file.new(path)` `1 8 1` and
  `satellite.variable.file.new` `1 6 2 1` can coexist — WORD_NUMBERS §4 calls that
  pair *"the kind of thing that gets decided by accident at M16"* — and M19 needs it
  built rather than legislated from three milestones later. **Built, and the table
  is empty in `satl`**, which is this section's own ledger being kept: M9 holds
  none of the 227 numbered paths, so the first rows are M10's console and
  `tests/eval_test/dispatch.cpp` is the reader that proves the mechanism before
  anything owns one.

- **AND ONE THING THIS ENTRY DID NOT LIST: the compiler is a walk, so it keeps its
  own stack too.** §2.6 put the four static passes ahead of this milestone and the
  compiler is a fifth — it did not exist when M8.5 rewrote them, and DESIGN §7.5
  has no exception for a pass that runs once. A program that parses at 100,000
  deep and then segfaults being COMPILED would have moved M8.5's crash rather than
  removed it. **The general form, which is what every milestone after this one
  inherits: no walk over user-controlled depth uses the C++ stack, from its first
  commit.** M16's search walk is the next one it applies to.

**M10 — the console, and the first program that runs. LANDED 2026-09-02**, and
[MILESTONES/M10.md](MILESTONES/M10.md) is the review. *(Split on 2026-08-28
from a milestone then called M8, as its first half; it was M8.A until the
2026-08-30 renumber.)* Console with its printer thread, `satellite.main`, `satellite.return`.
**This is the milestone at which satellite executes anything at all**, and
everything from M11 on depends on it for the same reason every milestone after M1
depends on there being a binary: without it there is nothing to print through and
nothing can be demonstrated.

**Why the split.** M8 was one milestone until the empty `satellite.container.list`
that `satellite.main`'s parameter binds to was ruled M16's, which pushed M10 after
M16 and left M11 and M16 with no console — against §8's opening rule. **The
parameter is the only part that ever needed a list.** So the console, `main` and
`return` stay here, and hello world itself is **M17**, after M16.

**Its done-when cannot be DESIGN §3**, which is the thing worth saying out loud:
§3's hello world declares the parameter, and the parameter is M17's. What runs here
is the **bare `satellite.main()` form**, which is still legal and always was — §6's
grammar reads `"(" [ param_list ] ")"` and DESIGN §3 keeps both shapes. ~~`example/`
holds no bare-main program, so this milestone has no acceptance file yet; writing
one is the author's, and until then its done-when is prose.~~ **The author wrote
two on 2026-09-02, the day it was built**: `example/console.satl` prints and
`example/bare_main.satl` does not, and both return the runtime. So this
milestone's done-when is a pair of files somebody can read after all.

**Its seven paths, and the two unnumbered mechanisms M14 consumes.**
*(2026-08-28.)* `satellite.main` `1 3 (0)`; `satellite.console` `1 5 (0)` and
`display` `1 5 1`; `satellite.return` `1 15` with all three shapes — `return()`
`1 15 0`, `return(satellite)` `1 15 1`, `return(value)` `1 15 2`, where DESIGN §4 is
why the middle one means success. The other eight children of `console` are
**M14's**. What is not numbered and is still this milestone's: **`display`'s
un-newlined form**, which lives under `1 5 1`, and **the `drain()` barrier**, which
§6 keeps in as many words — *"the Console with its own printer thread, and the
`drain()` barrier before reading input"* — and which DESIGN §10.1 justifies by *"a
prompt written with no trailing newline."* ~~**Both exist for input**~~, three
milestones before anything reads any, and M14 consumes them rather than rebuilding
them. *(Corrected 2026-09-02. The un-newlined form does; `drain()` does not —
it is the first step of the shutdown every program that prints takes, and the
paragraph below is why that makes it this milestone's rather than a piece of M14
built early.)*

**THE PRINTER CREATES ITS OWN THREAD, AND IT DOES NOT TAKE ONE FROM M6.**
*(The author, 2026-09-02: "the plan was always that the printer would create its
own thread." This paragraph said the opposite — "the printer thread is the pool's
first tenant and takes a thread from M6 rather than spawning one of its own" —
from 2026-08-28 until that day, and §4.5.1 carries what it changes and why the
error was a category one: M6's pool is a batch runner and a printer is a resident
thread.)* So the console owns a thread for the life of the run, starts it when the
console starts and joins it at exit, and `machine_limits/pool.hpp` is not in its
include list.

**M6's pool therefore still has no tenant after this milestone, and that is a
finding rather than a gap.** MILESTONES/M6.md §8 carried the question forward in
these words: *"M10 is the first milestone that can test that justification, and
its printer thread is the first tenant; if the pool is still doing nothing after
M10, §4.5.1.2 is a decision that should be re-taken with a number rather than
defended."* **The premise was false, so the test does not happen here** — the
first real batch is `satellite.include` of a second file at M25. The question M6
asked is not answered by M10 and is not failed by it either; what M10 owes it is
this sentence, so that nobody reads the pool's silence after M10 as the evidence
M6 was asking for.

**THE BARRIER IS A PREFIX OF SHUTDOWN AND NOT A SECOND MECHANISM.** *(The
author, 2026-09-02: "the block is similar to a shutdown -- combine shutdown with
the block.")* The console stops in four steps, and `drain()` is the first one
rather than a thing built beside them:

    drain    wait until the printer's queue is empty
    flush    fflush the fd -- DESIGN §10.1, because glibc buffers fully to a
             pipe or a file and line-buffers only to a tty
    stop     close the queue to new work
    join     the printer thread ends

`satellite.console.input` takes step 1 and carries on; `satellite.return` from
`satellite.main` takes all four. **So this milestone builds a console that knows
how to stop, and the input barrier falls out of it** — which is also why the
barrier can be built three milestones before anything reads input without being
speculative: the same wait is on the exit path of every program that prints.

**WHAT "BLOCKING" IS IN THIS MACHINE, AND IT IS ALREADY BUILT.** A handler that
blocks is a handler that takes a while to return. `evaluator/dispatch.hpp`'s
`HandlerFn` returns a `bool`, so the machine's work stack — `machine.hpp`'s
`work_`, the `{op, step}` pairs that ARE the currently running series of
instructions — simply does not advance while one is in progress. No queue, no
new op, no `Ending` state, nothing added to §2.3's list of what closure
compilation is. **This milestone is the first of six sites that block the walk**,
and the list is here rather than in six milestone entries because what they share
is a rule and not a mechanism — they block on six different things and every one
of them has to be interruptible in the same way:

| | site | blocks on |
| --- | --- | --- |
| **M10** | `drain()`, and the shutdown it is the first step of | the printer's queue emptying |
| M14 | `satellite.console.input` `1 5 2`–`1 5 4` — DESIGN §10.1's *"ask, and wait"*, beside `typed()` `1 5 5`, which does not | stdin |
| M19 | `satellite.variable.file.read_line` `1 6 2 3` | a disk |
| M22 | the prompt | a key |
| M23 | `satellite.variable.thread.join()` `1 6 13 2` | another thread |
| M27 | `satellite.network.receive` `1 20 5` | a socket |

**AND THE TWO FAMILIES SPLIT BY MILESTONE, WHICH IS WHY M10 DOES NOT BUILD A
GENERAL WRAPPER.** Both of this milestone's waits are on a condition variable,
so **there is no `EINTR` case here at all**: `pthread_cond_wait` does not return
one, and the flag it would test does not exist until M11. The other five block in
a syscall, and they inherit a rule this milestone cannot write and must not
pretend to — SIGINT is installed **without `SA_RESTART`** (§6, *hard-won; do not
rediscover*), so `EINTR` is not merely how an interrupt is reported, **it is the
only thing that returns control to the walk** so that M11's *"stop itself at the
next statement"* can run at all. A blocked handler is INSIDE a statement. The
retry loop every C programmer writes from memory — `while (read(...) < 0 &&
errno == EINTR)` — silently removes Ctrl-C from one site and no other, and M14 is
where that rule gets written down with its first consumer in front of it.

**`display` NEVER BLOCKS, AND THE QUEUE IS UNBOUNDED ON PURPOSE.** A bounded
queue makes a producer wait when it fills, which is a threshold nobody chose
appearing in the most-called path in the language — the hidden constant DESIGN
§1.1 refuses and §7.5 rules out in general. The bound is memory, the way a
list's is, and the watchdog is what notices.

**THE ONE PLACE THE TWO MUST NOT BE COMBINED IS THE WATCHDOG, AND THIS MILESTONE
OPENS A HOLE ABOVE A LEVEL M6 ALREADY CLOSED.** `machine_limits/watchdog.cpp`
gets the stdio half right and says why in capitals — *"fflush BEFORE `_exit`,
BECAUSE `_exit` FLUSHES NOTHING"* — but `fflush` reaches what the printer has
**already written**, and this milestone puts a queue ABOVE stdio that it cannot
reach. **Draining there is not the fix.** The watchdog is a detached thread
killing a process for taking too much memory; waiting on the printer is exactly
what it must never do, because a stuck printer would hang the one thing whose job
is to not hang. That is `_exit` over `exit` again, which the same file argues from
the same direction.

**So the shutdown drains and the emergency exit does not, and what that costs is
stated here rather than discovered later: lines queued but not yet written are
LOST when the watchdog fires.** M22 is the milestone that revisits it, because it
is already the emergency path's only registrar (§8's M22 entry, and M6's argument
for registering nothing yet).

**DONE WHEN — WRITTEN AT M10, AND IT HAS TWO ACCEPTANCE FILES AFTER ALL.**
`satl example/console.satl` prints `hello!` and exits 0, and `satl
example/bare_main.satl` prints nothing and exits 0 — the author wrote both on the
day this was built, which is what the struck sentence above was waiting for. What
neither can show is inside the console, so `tests/console_test` takes the four
claims a file cannot make: that a line stays atomic across eight threads, that
nothing queued is lost, that `drain()` means the bytes reached the descriptor,
and that the row is found by its number. The clauses, as they turned out: `satl file.satl` and `satl --run file.satl` run
a program that starts at `satellite.main` and prints through
`satellite.console.display` `1 5 1`, which is the first row `handlers[path_id]`
has ever held; the console owns a printer thread it made itself and joins it in
four steps — drain, flush, stop, join — so a line printed before a diagnostic
appears above it and not under it; `drain()` returns only when the bytes have
reached the descriptor, which is what M14's prompt needs; a line stays atomic
across eight threads; `satellite` is a value and `satellite.return`'s three
shapes `1 15 0`, `1 15 1` and `1 15 2` all run; a file with no `satellite.main`
is refused with **S0402** and a `main` that declares DESIGN §3's parameter with
**S0720** naming M16, exit 3; and `satl --version` constructs no console and
starts no thread, which the startup share says by not moving.

**AND WHAT A PROGRAM ANSWERS IS NOT ITS EXIT STATUS, WHICH THIS MILESTONE HAD TO
DECIDE AND NOBODY HAD WRITTEN DOWN.** The C shape — `main` returns a number, the
process exits with it — is wrong here for three reasons and the first is
sufficient: `programs/opening.hpp`'s four statuses "are assigned by what a
failure IS", so a program answering 2 would be reported as "the command line did
not name something satl can do". DESIGN §3 already gives the language a success
signal and it is not numeric — `satellite.return(satellite)` is "return the
runtime (that is, success)" — and a satellite number is exact and unbounded
while an exit status is eight bits, so the conversion would be the silent kind
§1.1 refuses. So the ENDING decides it: 0 finished, 4 a ceiling, 1 the program
was wrong. **The author can overrule this in one line** and MILESTONES/M10.md §6
carries it as an open item, which is the shape M6's `_exit(2)`-versus-`_exit(4)`
argument took.

**AND THE UN-NEWLINED FORM HAS NO SPELLING, WHICH BUILDING IT FOUND.** This
entry gives M10 "`display`'s un-newlined form, which lives under `1 5 1`", and
the M14 entry below names v1's `display(text, end="")` and `named_arg_misuse()`.
**DESIGN §6's grammar has no named arguments** — `args := expression { ","
expression }` — so that spelling is not writable in this language and no
positional one is specified anywhere. The capability is therefore built on the
console, where the queue is, and reached only from C++; `tests/console_test` is
its only caller until M14's `satellite.console.input(prompt)` `1 5 3` becomes
the real one, and whoever gives it a language surface decides the spelling.

**Ctrl-C has no ending to be reported as, and that is M11's to settle.**
`evaluator/machine.hpp` has three — `Finished`, `Refused`, `Stopped` — and
`Stopped` means a ceiling was reached and carries `EXIT_LIMIT` (4). **An
interrupt is not a ceiling**, so without a fourth, a person pressing Ctrl-C is
reported as a machine limit and a script testing for 4 reads it as a memory
ceiling. That is the mistake M6's entry above already corrected once, in the
`_exit(2)`-versus-`_exit(4)` argument. **Named here and decided there**: M10 has
no interrupt to end, M11 builds the SIGINT half, and the fourth `Ending` is
cheaper to add before five sites depend on the third one meaning two things.

**M11 — scalars and control flow. LANDED 2026-09-03**, and
[MILESTONES/M11.md](MILESTONES/M11.md) is the review — the semantics the
sixteen QUAD-sourced string methods were given (0-based positions, refusals
where another language hands back sentinels, the mutating rows' write-back
contract), the two-count selector fold that building the number methods
found, and the one thing this entry expected that did NOT land: M8's
"retune the dial at run time" sentence — no document says what assigning to
a language path means, and on 2026-09-03 the author moved it to **M15**,
where the third dial lands and the mechanism is decided once in front of
all four `1 14 2` rows.
`satellite.statement.if` `1 13 1`, `.for` `1 13 2`,
`.while` `1 13 3` and `.else` `1 13 4` — **their parse rules land at M4** (above);
what lands here is running them. `satellite.variable.bool`, `.number`, `.string` and
their methods.

**`satellite.bool` `1 17` is a different node and IS this milestone, as of
2026-08-28.** The type is `satellite.variable.bool` `1 6 6`; the module constants
`satellite.bool.false` `1 17 1` and `.true` `1 17 2` hang off a top-level namespace
`1 17 (0)` that DESIGN §6.1 cites as the module-constant case. Until this pass the
paragraph here read *"is not this milestone"* and no other milestone claimed them, so
three paths were refused by the only milestone in a position to build them — which is
the M3/M4 failure with the sign flipped, and worse, because it was written down
deliberately. **The distinction it was drawing is still true and still worth
keeping**: two nodes, two numbers, and reading `satellite.bool.true` as a way of
spelling the type would be a real error. What was wrong was the conclusion.
**Building them costs a `handlers[path_id]` entry each**, because a module constant
is a path that evaluates without a call, and this milestone already builds the type
they answer with.

**It also owns the SIGINT half of Ctrl-C, and that was nobody's.** *(2026-08-28.)*
v1's `install_interrupt_handler()` — 249 lines in `system_facts/interrupt.{hpp,cpp}`,
installed **without `SA_RESTART`**, which §6 marks *hard-won; do not rediscover* —
works by having the first SIGINT set a flag and *"let the walk stop itself at the
next statement, which is what makes an interrupted program report the line it was
on."* **The first walk long enough to be stopped is `while` `1 13 3`, here.** M22
owns the other half — the prompt's Ctrl-C, which arrives as the byte `0x03` because
raw mode turns ISIG off, so it never reaches a handler — and M14's `1 5 2`–`1 5 4`
need this half to tell an interrupted read from a closed stdin.

**M12 — the variant, and what "nothing" is.** *(New 2026-08-28. After M11.)* Two
numbered paths — `satellite.variable.variant` `1 6 14` and
`satellite.variable.expression` `1 6 9` — and it builds one of them. It is a small
milestone that exists first for a reason that has nothing to do with its size:
**two later milestones ask the same question in the same words, and it has to be
answered once, in front of both, or it gets answered twice.**

> `satellite.console.typed()` `1 5 5` — *a line, or nothing.* (M14.)
> `satellite.variable.file.read_line` `1 6 2 3` — *one line, or nothing at end.*
> (M19.)

**Nil is already in the value model and is not already in the language.** DESIGN
§8.2 requires that `bool` and nil never allocate, so M9's `Value` carries the state;
DESIGN §6.4 qualification 3 names it — *"two non-dispatchable states needing
different messages: an undeclared variable, and a declared variable holding
nothing"* — and that is a rule about **error messages**, not a way for a program to
ask. Between M9 and here a satellite program can be handed nothing and has no
sentence it can write about it. That is the gap this milestone closes, and DESIGN
§1.1 is why it is not acceptable to leave it to whichever of M14 and M19 lands
first.

**`satellite.variable.variant` is the type the question belongs on.** A variable
declared as one type either holds a value of it or is the declared-and-empty state
§6.4 names; a variable declared `variant` is the one that can truthfully answer
*what are you holding*, which is the answer both call sites above need. DESIGN §8's
table carries the row with a dash for its representation and *"deferred; PLAN §8,
Later"* as its whole specification, so **the representation is this milestone's
first piece of work and DESIGN §8 gains a real row here.**

**Blocker, and only the author can take it: is "nothing" a state every type has, or
a value only a `variant` can hold?** The two readings produce different languages
and both are consistent with what is written down today. Under the first,
`satellite.variable.string line = my_file.read_line()` is legal and `line` may be
empty-or-nothing; under the second, that declaration is a type error at end of file
and `read_line` returns a `variant`. **§6.4's "declared variable holding nothing"
reads as the first** and DESIGN §8's deferral of `variant` reads as the second, and
nothing reconciles them. M14 and M19 both inherit whichever answer is given, so the
answer is worth more than either milestone.

**Taken 2026-09-03, by delegation, and the milestone landed the same day.** The
author handed the blocker over whole — "I don't have a vote on this one" — and
the answer is the first reading: **nothing is a state every type has**, and the
`variant` is the type whose vocabulary can name it. DESIGN §8.7 is the
language's statement, with the declined reading kept beside it;
[MILESTONES/M12.md](MILESTONES/M12.md) §2 is the full argument from the code —
op_store already writes the state into every declared type's slot, S0714's
sentence already reads as the first reading verbatim, and the second reading
requires an assignment type-check the language has nowhere. **The
representation row is the `Value` itself** — no arm, no handle, no bytes — and
the vocabulary is four appended children, `holding` `1 6 14 1`, `holds(x)`
`1 6 14 2`, `held` `1 6 14 3` and `clear` `1 6 14 4`, assigned by
`example/variant.satl`'s walk and recorded in WORD_NUMBERS §2.5. What reading
two offered — a refusal at the moment nothing arrives somewhere unexpected —
survives as `held`, opt-in and by name.

**`satellite.variable.expression` `1 6 9` stays numbered and unbuilt, and saying so
is the work.** Its entire provenance is one row in `SCRATCH.md/WORD_SURFACE.md`
sourced to `v1docs` — no representation, no method, no v1 code, no sentence in
DESIGN. WORD_NUMBERS §1.2 means it cannot be withdrawn without leaving a hole, and
M2's density check refuses holes, so it keeps its number whatever happens to it.
This is `satellite.variable.duration` `1 6 8`'s shape exactly (M13 then, M29
since 2026-09-04), and both are here rather than in a future audit's table.

**Done when** one program declares a `satellite.variable.variant`, puts a number in
it, then a string, then nothing, and prints a different and correct answer at each
step through `satellite.console.display` — and when a `variant` that holds nothing
is refused by name if a method is called on it, which is §6.4 q3's second message
becoming something a person can read. Nothing here needs a float, a container, a
thread or a file; **it needs M9's `Value`, M10's console and M11's scalars, and
that is the whole of its dependency list.** *(Done 2026-09-03:
`example/variant.satl` is the program and `held` is the refusal — and "by name"
turned out to be a fix in the machine, not the row: `text_of` answered a call's
`(` token, so every S0713/S0714 sentence in the language said "`(` was asked"
until this clause caught it. MILESTONES/M12.md §3.)*

**M13 — the clock and the dice, the two sources of nondeterminism. LANDED
2026-09-04**, and [MILESTONES/M13.md](MILESTONES/M13.md) is the review. *(New
2026-08-28, corrected from the 2026-08-27 draft. After M12.)* **Twenty numbers on
twenty-three rows** *(seventeen on twenty since 2026-09-04 — `1 9 2`, `1 6 7`
and `1 6 8` are M29's, moved the day the blockers cleared, and each keeps its
line below with the move marked)*, named individually here because a milestone that says only
`satellite.random` leaves twelve children owned by nothing, which is the failure
this whole pass exists to end:

- `satellite.random` `1 7 (0)` and `1 7 1` through `1 7 12`.
- `satellite.time` `1 9 (0)`, `.now` `1 9 1`, `.new` `1 9 2`, `.sleep(n)` `1 9 3`.
  *(`.new` to M29, 2026-09-04.)*
- `satellite.variable.time` `1 6 3 (0)`, `satellite.variable.date` `1 6 7` and
  `satellite.variable.duration` `1 6 8` — **three siblings under
  `satellite.variable`**, not a subtree. A child of `1 6 3` would be `1 6 3 n`, and
  §1's per-parent rule is the reason that sentence has to be written out rather
  than abbreviated with leading dots. *(`date` and `duration` to M29,
  2026-09-04; `1 6 3` stays — `now` must answer a value of some type — with its
  methods M29's to number.)*

**`satellite.random` is thirteen numbers on sixteen rows.** `.range` is a second
spelling of the two-argument shape, not a fourth segment and not a path
(WORD_NUMBERS §2.3): `fast.range(min, max)` **is** `1 7 5`, `normal.range` **is**
`1 7 8`, `ultra.range` **is** `1 7 11`. **Those three are the only duplicate
numbers in the language**, and they are the whole of the difference between the 227
rows §2.2 holds and the 224 distinct numbers it carries. That reconciliation was
written down only in `SCRATCH.md/MILESTONE.md` §5, which is scratch; it lives here
now.

**M2 already built the alias, so this milestone inherits it rather than finding
it.** The draft this comes from reported the three rows as a finding *about* M2 —
that a tree whose position is its number cannot express `1 7 5` for a node named
`range` under `fast`, and that the density assert would fire. Both halves were true
of M2 as specified and neither survived M2 as built: `words.def` holds **254 nodes
and 9 aliases**, an alias is a second spelling rather than a second node, and
"no duplicates" is by construction because a number is a position. `hexadecimal`
and DESIGN §7.7's spellings of `arguments` — six then, seven since 2026-09-09 —
are the same mechanism at two other scales. What is left for this milestone is to use it and to test it, which
done-when clause 2 does.

**DESIGN §11 is stale about how many shapes there are and says so nowhere, and
correcting it is part of this milestone.** §11 says *"two shapes on each"*, which
was true of the first satellite — six random paths in its registry, and a header
saying the public surface is two shapes per tier. §2.2 gives each tier four slots.
§11 is one commit older than the numbering (`095eb49`, against `99fa6d9` and
`cc3f813`) and neither numbering commit touched it. **Building four shapes out of a
section that says two is the M3/M4 failure run backwards.** Everything else in §11
is still true and stays — the tier table, uniform over [0, 10⁴⁰), inclusive at both
ends, the one-in-ten paragraph and all of §11.1. What replaces the stale sentence
is a pointer to §2.2 and a line saying which numbers are specified, **plus a date**,
which §11 carries neither of and is how it went stale without ever looking wrong:

- `1 7 4`, `1 7 7`, `1 7 10` — `<tier>(digits)`. Specified, built in v1, tested.
- `1 7 5`, `1 7 8`, `1 7 11` — `<tier>(min, max)`, spelled `.range` in v1. The same.
- `1 7 6`, `1 7 9`, `1 7 12` — `<tier>(min, max, step)`. Specified in no document.
  *(Specified 2026-09-04 — DESIGN §11 — and built here rather than reserved: the
  zero-argument refusal text names the shape, and a refusal that advertises a
  shape nobody built would be the language lying.)*
- `1 7 1`, `1 7 2`, `1 7 3` — blocked below, and the answer decides whether the
  call surface is nine shapes or twelve. *(Answered 2026-09-04: twelve, three of
  them refusals by design — the blocker carries it.)*

**Nine of the sixteen rows are code v1 built and tested; seven are not.** v1's
registry carries exactly six random paths — its own `format.def` says *"Five words
buy six paths"* — and those six are `fast/normal/ultra(digits)` and the three
`.range` spellings, which is nine §2.2 rows over six numbers. The other seven rows
are `1 7 (0)` and the six numbers this milestone leaves reserved. **"Sixteen rows
of working code sitting in Later" was the draft's sentence and it overstates in the
direction that makes the finding look bigger**, which is the failure the last three
commits before this pass were about.

**Five of these twenty numbers were not in "Later" at all**, which is worth saying
because "Later" is what this milestone empties: all four under `satellite.time`
`1 9 (0)` — a different node from `satellite.variable.time` `1 6 3`, and
`SCRATCH.md/MILESTONE.md` §0.0 exists to correct exactly that confusion — and
`satellite.variable.duration` `1 6 8`, which is in no list anywhere except DESIGN
§12's deferrals.

**The bignum half of the dice is M8's and this milestone does not claim it.**
`satellite_number/random.cpp` — 121 lines: the limb-aligned uniform draw in base
10⁹, the rejection sampler that exists because a bare `%` would skew 2:1, and
`MAX_RANDOM_DIGITS = 100000` — is one of the ten files M8 ports, and §6.1's open
question 3 says as much while M8's own entry did not. **M8 now says it.** What
this milestone ports is the other half — `random_numbers/random.{hpp,cpp}`, 260
lines: the tiers, the seed, the fold and the watchdog — plus a dispatch that is a
**rewrite and not a copy**, because v1's `modules_random.cpp` is a string compare on
the tier and on `"range"` and §7 throws exactly that away. What survives is the
argument checking and v1's nine failure texts, reached through `handlers[path_id]`
and reported through M5.

**The seed path is broken as built, and porting it unchanged ports a crash.** v1's
own `plans/pcg_k16384_spec.md` records that under `strace` the built `./satl`
issues exactly one `getrandom` in the whole process — glibc's own 8-byte startup
draw — and that its 65,568 seed bytes come from RDRAND through libstdc++'s default
token, never touching the kernel, while three comments in the code say otherwise.
Worse, that path **throws**: libstdc++ retries RDRAND a hundred times and then
raises, and there is no `catch` anywhere under v1's `src/`, so the failure mode of
a language builtin is `SIGABRT`. The recommended replacement is `getrandom(2)` with
flags 0 in a loop — 12.4× faster, an `errno` instead of an exception, and `ldd`
unchanged. **This is a done-when, not a footnote.**

**The vendored PCG is the project's first third-party dependency and LAYOUT.md has
no row for one.** Three headers, 3,138 lines, Apache-2.0 inside an MIT tree,
header-only so `ldd` does not change. It is reached through `-isystem` for a
load-bearing reason rather than a stylistic one: `pcg_extras.hpp` warns under this
project's `-Wall -Wextra`, and §4's silent from-scratch rebuild depends on that
warning not being ours. **This milestone writes LAYOUT.md's first vendored row**,
and the Apache notice travels with it. *(2026-09-04, the author asked again for
the extended family's extreme member widened to 512 bits of output, as an
in-tree typedef over `pcg_engines::ext_*` — and the ask stands refused by the
tree's own dated investigation, `pcg/README.md`'s "A 512-bit variant was
investigated on 2026-08-27 and is NOT a conversion": no such typedef exists to
write — the family's widest output is 64 bits — the `uint_x4` composition
compiles and multiplies WRONG, and a fork means unpublished LCG constants and
1 MiB of state per generator. Width is a property of the SAMPLER, which already
answers at any digit count from 32-bit words, so the engine is v1's
`pcg32_k16384` exactly as it stands, and the seam is one file to satisfy the
day the author overrules this with constants in hand.)*

**The tier windows are a design choice and stand; the watchdog is a measurement and
does not.** §9 says measure on this machine and do not quote. v1's figures are a
Xeon E5-2670 v3's: about 153,600 draws per millisecond, `ultra` reaching roughly
249M draws at 2,000 ms and 310M at 3,000 ms against a cap drawn from [500M, 600M] —
a margin of about 1.6–1.9×. **On a machine 1.7× faster `ultra` reaches the cap and
the watchdog stops being a failsafe and becomes part of the mechanism**, silently
shortening the spin. Re-measure before claiming the tiers behave as documented; if
the margin has closed, the per-tier caps v1 recorded and declined are this
milestone's work.

**What this milestone produces for QUAD is a correction, not a mechanism.** QUAD.md
§2 files `random` under *"already decided in satellite, and fine."* It is not.
`quad_core.hpp`'s entire randomness is one primitive — a uniform `double` in [0, 1)
from a seeded `mt19937` — and `Rack::draw` is a roulette wheel over `std::pow`
weights. Three independent reasons `satellite.random` cannot write it: **no float
draw exists in any of the thirteen numbers**, and v1 refuses fractional bounds
deliberately; **there is no seed**, so QUAD's determinism invariant is not
expressible; and **the cheapest tier throws draws away for 50–100 ms before it
answers**, against a 90 ms tick that draws from the rack once and calls the rng
nineteen more times. The shape of an answer is a fourth thing under
`satellite.random` that takes a seed and does not spin, and **`1 7 13` is free** —
this milestone does not assign it, because minting a number is the numbering's and
the author's. M21 is where that debt comes due.

**Blockers — all cleared by the author on 2026-09-04.** Each keeps its
argument, with the answer written where the question was:

- **One clock, or two.** DESIGN §13 requires `satellite.variable.time`, `.date` and
  `satellite.time.now` to agree on **one clock and one epoch**, and says a monotonic
  clock and a wall clock cannot both be it. **v1 answered both halves and the
  answers do not compete**: `system_clock` and the Unix epoch for the *value*,
  because `steady_clock`'s epoch is unspecified and the value has to mean something
  outside this process; `steady_clock` for the *timer* — the spin deadline and the
  sleep — because `high_resolution_clock` is the same type as `system_clock` here,
  `is_steady` is false, both tick at 1 ns with 21 ns granularity so there is no
  precision to trade away, and NTP can step `system_clock` backwards under a
  running deadline. Only one of the two is ever a satellite *value*, so *"one clock,
  one epoch"* is a rule about the type and not about the implementation. **That
  reading measured the author's stated leaning and rejected it, so it is put to the
  author rather than taken here.** **Put, and taken: the author confirmed v1's
  split on 2026-09-04**, and DESIGN §13's Time entry now carries it as the
  language's statement.
- **What `1 7 1`, `1 7 2` and `1 7 3` name** — the tier node, or a zero-argument
  call. DESIGN §4's example writes all three tiers bare, and M2's acceptance test
  writes `satellite.random.normal` bare at `1 7 2`; the author's note writes the
  digit shape only, `satellite.random.fast(99)`, which is `1 7 4` and settles
  nothing. The parentheses appear first in §2.2's rewrite, whose preamble promised
  that every number was preserved — a promise about numbers, not about what they
  name. Under the call reading, v1 tests that source as an arity error and there is
  no number left for the node the spelling `ultra` lands on; under the node reading,
  the tier is a path that is not a value, which v1 also tests. **The answer is a
  precondition of this milestone's own number list**, which is why the draft could
  count `1 7 2` as covered on one page and reserved on another. **Answered
  2026-09-04: the zero-argument shape, and it is a refusal by design** — *"you
  must supply a digit_count, min and max, or min max and step"*, the author's
  text verbatim, one text for all three tiers. A bare tier folds to the same
  number and a path is not a value, so both spellings refuse and no number is
  left owning nothing. The inference not taken — the digit count read from the
  assignment's destination — is recorded in DESIGN §11 so it is not re-proposed.
- **The step forms `1 7 6`, `1 7 9`, `1 7 12`.** Assigned by rule, specified
  nowhere, built nowhere, and they collide with the one promise §11 makes: `.range`
  is inclusive at both ends, and `(min, max, step)` reaches `max` only when
  `max - min` is a multiple of `step`. Whether `(1, 10, 3)` can answer 10 is exactly
  what DESIGN §13 warns gets settled by accident. **Answered 2026-09-04 by rule
  rather than by accident: `step` must divide `max - min` exactly, or the call
  is refused naming the last value the step reaches** (DESIGN §11). *Inclusive
  at both ends* stays true of every call that answers, and the three rows move
  from reserved to built — done-when clause 11 is theirs.
- **What `satellite.time.new` `1 9 2` takes.** Its only specification anywhere is
  nine words in a deleted note — *"set the arguments for a point in time"* — which
  reads as a component constructor and contradicts v1's rule that an instant is read
  off the clock and never written down as a literal. It cannot be designed apart
  from `satellite.variable.date` `1 6 7`. **DESIGN §13 cites it as an established
  precedent** — *"exactly as `satellite.file.new` and `satellite.time.new` already
  do"* — when it has never existed; the argument still holds on `file.new` alone,
  and §13 should say so. **Answered 2026-09-04: moved.** Designed beside `date`
  at M29, and §13 now says both — the settled Time entry and the corrected cite.
- **The unit of `satellite.time.sleep(n)`.** QUAD's call is
  `sleep_for(milliseconds(90))` at `quad_main.cpp:260`. The one unit v1 spells is
  `100ms`, a lexed literal converted at parse time, and §7 throws away the special
  case that consumed it but not the literal. A bare number makes the unit invisible
  at the call site, which is the readability failure DESIGN §1.1 exists to prevent.
  **Answered 2026-09-04: the unit is seconds, whole or fractional** — one unit
  always, so there is nothing at a call site to misread, and `0.09` is an exact
  `Number`, so the fraction costs no float and no M15. The done-when below paces
  with `sleep(0.09)`, and §7's discard stands: v1's `100ms` literal dies with
  the special case that consumed it.
- **`satellite.variable.date` `1 6 7` has a number and nothing else** — no row in
  DESIGN §8's types table, no representation, no constructor, no method, no v1 code.
  Either the specification work is carried here or the number stays reserved and
  this milestone says which. **Answered 2026-09-04: reserved here, designed at
  M29** beside `time.new` and the instant's methods — the calendar, one
  milestone for everything a time value can be beyond *now* and *sleep*.
- **The two-or-more time methods v1 ships are unnumbered.** `.minus(t)`,
  `.nanoseconds()` and `.to_string()` are all handled on a `Time` in v1, and help
  advertises `.size()` on the next line. `satellite.variable.time` `1 6 3 (0)` has
  **zero** children in §2.2 while its three hand-written siblings have 16, 7 and 14.
  **WORD_NUMBERS has to number them and this milestone must not**; until it does, a
  program can obtain an instant and do nothing whatever with one — which also means
  this milestone cannot time its own tier floors in satellite. **Answered
  2026-09-04: they are M29's.** WORD_NUMBERS numbers them there; the tier floors
  are timed in C++ where this tree's timing tests already live, and what M13
  owes an instant is a rendering through `display` — which is display's, not a
  method's.

**`satellite.variable.duration` `1 6 8` stays numbered and unbuilt, and saying so is
the work.** The sweep sourced it from v1's documents; the v1 source that uses the
phrase says there is no such type and gives four reasons, and DESIGN §12 still
defers durations today. It cannot simply be struck either: §1.2 leaves a hole where
a child is removed and M2's density check refuses holes. So it keeps its number
whatever is decided about it, and it is listed here rather than left to an audit —
the same treatment `satellite.variable.expression` `1 6 9` gets at M12.
*(2026-09-04: the number is M29's now, decided beside `date` — moved rather than
re-argued, and this paragraph travels as the argument it was moved with.)*

**Done when** this runs inside `satellite.main` and every clause below holds:

```satellite
satellite.variable.number big = satellite.random.ultra(40)
satellite.console.display(big)

satellite.variable.number die = satellite.random.fast.range(1, 6)
satellite.console.display(die)

satellite.variable.number i = 0
satellite.statement.while (i < 10)
{
    satellite.console.display(satellite.time.now)
    satellite.time.sleep(0.09)
    i = i + 1
}
```

1. `ultra(40)` is uniform over [0, 10⁴⁰), and a hundred runs give **about ten
   answers of 39 digits or fewer**. §11 says the leading-zero fact is stated in the
   header and in the test, so the test asserts the short answers *happen*; one that
   merely tolerates them is not the test §11 asked for.
2. `.range(1, 100)` answers both 1 and 100, `.range(7, 7)` is 7, and
   `.range(-100, -90)` lands inside itself — v1's assertions, ported, and the proof
   that M2's alias rewrite carries a real call.
3. All three tier windows are honoured, timed against `steady_clock` and asserted
   **on the floor only** — a ceiling is a machine's to miss under load, and a test
   that fails when the build box is busy is a test nobody trusts.
4. Draws per millisecond and `ultra`'s real draw count at 3,000 ms are measured **on
   this machine, with the date, beside the watchdog cap they justify**, and the
   margin is stated (§9).
5. **Eight refusals through six ported texts**, plus two that are not random's at
   all: `fast("x")` and `fast(1.5)` share one text, `fast(0 - 1)` and
   `fast(100001)` share another, and the two unlisted texts — `.range` wanting two
   numbers, and the could-not-draw and spans-more-than failures — come across with
   them. **An unknown tier and an unknown tail segment are M5's did-you-mean over
   the failing trie level, not this milestone's strings**; v1 answers both from one
   generic *"no such module function"* and §7 throws that dispatch away.
6. `satellite.time.now` twice in succession returns two different values — the
   one-line proof that int64 nanoseconds is the representation and a `double` is
   not, since 61 bits of epoch against a 53-bit mantissa is 198 ns of resolution.
7. The loop paces at 90 ms — `sleep(0.09)`, the unit being seconds — which is
   QUAD's main loop shape and the whole of what
   QUAD.md §2 asked `sleep` for, naming `quad_main.cpp:260`. *(90 ms is QUAD's
   number, not this machine's; §9 means it is re-measured here or sourced there.)*
8. **No tier is described as secure, anywhere** — not in a header, not in
   `satellite.help`, not in an error message. DESIGN §11.1, and the first
   satellite's reason for wording the refusal that hard travels with it: the word
   "secure" in a language's documentation is load-bearing, because someone will key
   something on it. The answer not taken, `getrandom(2)` at 256 bits for about a
   microsecond and no new dependency, travels with it too.
9. The seed comes from the kernel and cannot abort, or the refusal to change it is
   written down beside the measurement that condemns it.
10. The zero-argument call on each tier is refused with the author's text — *you
    must supply a digit_count, min and max, or min max and step* — and a bare
    tier used as a value is refused as a path that is not a value. One text,
    three tiers, both spellings. *(Both decided 2026-09-04.)*
11. `fast(1, 10, 3)` answers 1, 4, 7 and 10 and nothing else across a hundred
    runs, all four seen — clause 2's both-ends proof carried to the step shape —
    and `fast(1, 10, 4)` is refused, and the refusal names 9.

**No number stays reserved behind this milestone, and this sentence was written
on 2026-09-04 over one that reserved nine.** `1 7 1`–`1 7 3` are built as
refusals by design, `1 7 6`, `1 7 9` and `1 7 12` are built as the step shape,
and `1 9 2`, `1 6 7` and `1 6 8` are M29's, listed there. **A milestone that
quietly leaves numbers behind it is how this document came to have a 121-path
ledger** — this one leaves none.

**It does not depend on M15 and must not be placed behind it.** The draft put
itself after the float *"because this one has to report that the dice cannot use
it"*, which is a type confusion: DESIGN §8.1 makes `satellite.variable.number` an
exact arbitrary-precision decimal, so `1.5` is a `Number` and every fractional
refusal v1 tests is a `Number` test. Nothing here needs a float, and M15 is the one
milestone in this list that cannot land until an undecided rule is chosen. Its real
dependencies are M8 for the bignum, M10 for `display` and `satellite.main`, and
M11 for `while`.

**M14 — the console's other half: the reader thread and the terminal's facts.
LANDED 2026-09-04**, the same day as M13 and behind it, and
[MILESTONES/M14.md](MILESTONES/M14.md) is the review. *(New 2026-08-28,
corrected from the 2026-08-27 draft. After M13.)* **Eight paths —
`1 5 2` through `1 5 9`, every child of `console` except `display` `1 5 1`**, which
is M10's. §2.2 has no tenth child, so with M10 this namespace is finished:

- `satellite.console.input()` `1 5 2`, `input(prompt)` `1 5 3` and
  `input(prompt, target)` `1 5 4` — ask, and wait, in three shapes. WORD_NUMBERS
  §1.3 is why that is three numbers rather than one with an arity check: a
  user-owned argument cannot extend a path, so the count takes its own slot and
  **the arity is the identity**, settled at M4's parse before an argument is
  evaluated. `1 5 4` writes a place and returns nothing — the only out parameter in
  the language, and deliberately not the start of a general facility.
- `satellite.console.typed()` `1 5 5` — a line, or nothing, immediately. **The one
  genuinely new mechanism here**; v1 has no non-blocking input of any kind, and
  M12 is what makes *nothing* a thing a program can ask about.
- `satellite.console.width` `1 5 6` and `.height` `1 5 7` — property-shaped in the
  table because they are facts, asked fresh rather than sampled. v1 has no `height`
  at all: `ws_row` appears zero times in it.
- `satellite.console.clear()` `1 5 8` and `.home()` `1 5 9` — call-shaped because
  they are actions, and **both go through the printer's queue**. v1 writes them as
  one escape straight to the fd, which is a frame shredded between two queued lines.

**The reader thread is DESIGN §10.1's printer with the polarity named.** The
printer's producer is the program and its consumer is a dedicated thread; the reader
is that reversed — the dedicated thread blocks on stdin and pushes whole lines in,
and the program asks and is answered at once. The invariant that survives both
directions, and the reason no satellite program ever learns what a terminal mode
is: **the program's own thread never blocks on the terminal.** §1.1 applied to
input. QUAD.md §3.4 is the requirements document, and it is what retires QUAD's
`VMIN=0` poll loop.

**M10 owns two things this milestone consumes and must not rebuild**, and M10's
entry now says so: `display`'s **un-newlined form**, which lives under `1 5 1`, and
the **`drain()` barrier**, which has no number at all and which §6 keeps in as many
words — *"the Console with its own printer thread, and the `drain()` barrier before
reading input"* — while DESIGN §10.1 justifies the flush by *"a prompt written with
no trailing newline."* Both exist **for input** and neither is `1 5 3`. The draft
this milestone comes from wrote that M10 *"owns the output half of `1 5 3`"*, which
would split one numbered path across two milestones — the failure it was written to
prevent, committed in the sentence preventing it. **This milestone owns all of
`1 5 3`.**

**This is not a line editor, and M22's prompt is not `satellite.console.input`.**
M22's prompt is a second, unrelated reader: v1's `console_input/` is 1,449 lines
of raw mode, key decoding, history and a wrap-aware renderer, and its own header
states that no satellite program can reach anything in it, while
`satellite.console.input` runs through `std::getline` in the evaluator. Two readers,
one language. **Only 14 of those 1,449 lines come here** — `terminal_columns()`'s
ioctl with its 80-column fallback, and the clear — and **M22 consumes both from
here** rather than reimplementing them, which its entry now says.

**Ctrl-C is two halves and this milestone takes one of them.** DESIGN §10.2 gives
the key two meanings; raw mode turns ISIG off, so the prompt's Ctrl-C arrives as the
byte `0x03` and never reaches a handler — **that half is M22's, and M22's line
now says `0x03` rather than bare "Ctrl-C"**, because a bare "Ctrl-C" reads as owning
both. The SIGINT half is not this milestone's either, and the draft's claim that it
was is refuted by the draft's own dependency argument: v1's handler *"sets the flag
and lets the walk stop itself at the next statement"*, and the first walk long
enough to be stopped is M11's `while`. **`install_interrupt_handler()` — 249 lines,
installed without `SA_RESTART`, which §6 marks *hard-won; do not rediscover* — is
M11's, and M11's entry now names it.** What is genuinely this milestone's is the
read side: **`eof()` is the entire discrimination between a closed stdin and an
interrupted read**, so `1 5 2`–`1 5 4` cannot report truthfully without it.

**`width` and `height` are not `arguments.machine.*`**, which the ledger wondered
about. DESIGN §7.7's test is three surfaces over one set of facts:
`arguments.machine.threads` `1 14 1 1 1 3`, `.machine.cores` `1 14 1 1 1 1` and
`arguments.memory.total` `1 14 1 1 2 1` — **`memory` is a sibling of `machine`, not
a child of it** — live there because they are also `THREAD_COUNT`, `CORE_COUNT` and
`MEMORY_MAX` and `satellite_string`'s live codes, and *"must not be allowed to
disagree."* Terminal size has no second surface, no config counterpart, and
**changes during a run**, which a startup-sampled object cannot express. And
`arguments.clear()` would already mean something: `arguments` is a
`satellite.container.list` (DESIGN §7.7), so §1.5 resolves the bare `clear` through
the receiver's type to `satellite.container.list.clear` `1 4 2 11`. **That is a
spelling collision and never a numbering one** — §1's lists are per-parent, so the
numbers could not clash.

**Roughly 126 lines port close to unchanged** — `evaluator/modules.cpp` 62–187
(`read_input_line`, `console_input`, `console_input_into`), where the comments are
the specification and come across with the code. **Lines 188 onward are `display`'s
and stay with M10**: `named_arg_misuse()` is the message for
`display(text, end="")` and `display_with_end()` begins at 205, so the draft's
62–204 pulled seventeen of M10's lines into this port — the ownership blur two
paragraphs above exist to prevent. `set_display_pace` is the `100ms` special case §7
throws away. `typed()` and the reader thread port nothing.
`console_output/console.cpp` is not ported here and **is the file to read before
writing the reader**, because every hazard it documents reappears reversed —
swapping the vector out under the lock rather than erasing the front, and **three**
condition variables (`arrived_`, `emptied_`, and `pace_woken_` on a mutex of its
own) so that a waiting drain and a sleeping printer are never woken for each other's
reason.

**Open until 2026-09-04, when the author took every answer in one sitting —
each keeps its argument, with the answer written where the question was:**

- **One reader of stdin, or two?** A reader thread parked in `read()` and an
  `input()` calling `getline` on the walking thread are two consumers racing one fd.
  Routing `input()` through the same queue is almost certainly right, and it changes
  code §6 marks *do not rediscover*, so it is a decision and not a detail.
  **Answered: one consumer, and §10.1's own invariant had already decided it** —
  *"the program's own thread never blocks on the terminal"*, and `getline` on the
  walking thread IS that thread blocking on the terminal. `input` asks the queue
  and waits; `typed()` asks and does not; nothing else in the process reads
  descriptor 0. ("Two readers, one language" stands untouched — M22's raw-mode
  prompt is the other reader, never live at the same time.)
- **Ctrl-C across a thread boundary.** §10.2's mechanism was designed for a read on
  the walking thread. Move the read and the signal lands on an arbitrary thread,
  `EINTR` surfaces where there is no error to report, and the
  interrupted-versus-EOF answer has to travel back through the queue. **Answered
  by the self-pipe, and the fragility predicted here never arises**: the reader
  parks in `poll()` on stdin AND a pipe, the SIGINT handler writes one byte into
  it (`write(2)`, already the handler's one verb), and the wake is deterministic
  no matter which thread the kernel picked — `EINTR` stops being load-bearing
  across threads entirely. The three answers travel back through the queue as
  *different* answers — a line, the end, interrupted — so `eof()`'s old
  discrimination is done by construction and cannot be gotten backwards.
- **How the thread stops.** ~~The pool's fourth tenant~~ *(stale when written:
  §4.5.1 was corrected 2026-09-02 — that pool takes work that FINISHES, neither
  console thread is a tenant, and the reader creates its own thread exactly as
  the printer does)*. A thread blocked in `read()` cannot
  be joined at exit the way `~Console()` joins the printer. **Answered by the
  same pipe**: the reader never sits bare in `read()` — it parks in `poll()`
  where a byte can always reach it — so the shutdown writes one byte and joins,
  the printer's four steps gain a fifth, and `satl` exits cleanly with no
  detach and no leak. One mechanism, both questions.
- **How the registry declares that a parameter is a place.** §7 condemns v1's route
  to `1 5 4` — its first bullet, *"the joined path built per call"*, covers
  `expr_call.cpp:113–114` flattening the path and comparing it against a string
  literal before evaluating arguments, the same hack as the `100ms` case forty lines
  above it. The replacement is a declaration on the word, the way `display` will
  *declare* that it accepts a pace argument. **§6.4 qualification 2's
  receiver-binding tag is the nearest precedent — and now something extends it:
  words.def grew a third list**, `SAT_PLACE`, one row long, naming the written
  argument that receives the answer. The mechanism is a real column any word
  could take; the POLICY is that it never does again — *"deliberately not the
  start of a general facility"* is a sentence beside the list, which is exactly
  where a policy can be enforced by review. The compiler is the consumer: the
  place compiles as a slot and never as an expression, a non-name refuses
  (S1002) and a position that could read the result refuses (S1003), both
  before the prompt prints, and v1's flatten-and-strcmp has no descendant.
- **Does `clear()` imply `home()`?** v1 emits both as one sequence. If `1 5 8`
  homes, `1 5 9` is only ever useful alone; if it does not, QUAD's frame draw is two
  calls where `view.hpp` had one. Small, and it is a promise. **Answered: it
  homes** — v1's byte sequence kept whole, terminfo's own meaning for `clear`,
  and `home()` alone stays independently useful as the flicker-free repaint
  QUAD's frame wants: home and overdraw, no erase, one call either way.
- **What the reader does during M22's prompt**, which puts the terminal in raw
  mode and thinks it owns stdin. Whichever of the two lands second decides it, and
  saying so now is cheaper than finding it at M22. **Said now, as an invariant
  and a mechanism rather than as M22's design**: stdin has exactly one consumer
  at any instant, and whoever takes the terminal parks the reader first. The
  control pipe is already the verb — a byte means stop today, and pause/resume
  are two more bytes M22 builds beside their only caller, because a verb built
  three milestones before anything calls it is the registry-without-a-consumer
  failure M2 exists to refuse.

**Done when** one program run under `pty.fork()` — assert on the screen, not on the
bytes — proves all eight paths, each clause a separate assertion:

1. **A counter keeps rising while a line is half-typed.** Send `/q` a byte at a
   time; the number advances across every pause. QUAD.md §3.4 requirement (1), with
   `VMIN=0` retired, and the one claim this milestone exists to make.
2. **And it does not spin.** CPU near idle between keystrokes, because the blocking
   happens on a thread that waits — *"strictly better than the poll loop the obvious
   alternative produces"* (DESIGN §10.1). **This clause is why M13 comes first**:
   it needs `satellite.time.sleep(n)` `1 9 3`, a busy-spin loop makes it
   untestable, and in the draft that path was reached by no milestone at all.
3. **`typed()` tells an empty line from no line.** Return on an empty line is a
   line; nobody typing is nothing. Two answers, not one empty string — **M12's
   answer, consumed here rather than invented here.**
4. **The prompt appears before the cursor waits.** `1 5 3` prints with no trailing
   newline and with the queue drained; a prompt arriving after the program has
   already blocked is the failure `drain()` exists to prevent, and it is observable
   only here.
5. **`1 5 4` writes a place and yields nothing**, assigning its result is an error,
   and a non-variable second argument fails *before* the prompt prints — so nobody
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

**Clauses 1, 2 and 8 cannot be demonstrated by any earlier milestone and cannot be
demonstrated by the eight paths one at a time**, which is §8's own argument that
this is one milestone rather than a bullet added to two.

**M15 — `satellite.variable.float`. LANDED 2026-09-04**, and
[MILESTONES/M15.md](MILESTONES/M15.md) is the review — the rounding rule
chosen by delegation (half away from zero, ratifying what the code had taken
three times), the retune given its meaning (a handler-shaped write into the
running machine's Policy, `evaluator/dispatch.hpp`'s Assigners, in front of
all four `1 14 2` rows), the three waiting methods answering, and the float
itself behind its handle as the value model's seventh arm. *(Its own
milestone as of 2026-08-27. It spent the morning in "Later, in no fixed
order", was moved into M11, and was separated out here because it is a type
with a specification of its own and one undecided rule.)*

A `satellite.variable.bool` and **two `satellite_number`s** — `positive`, then the
integer part and the fractional part, each an exact base-10⁹ magnitude. **DESIGN §8.6
is the specification**: the three invariants, `normalize`, the four operations,
modulus, power, and the classification that says which operations round and which
cannot.

**It costs no new arithmetic.** M8 brought `satellite_number` across and built the
sign; a float is composition over two of them plus rounding.

**AND THE SIGN IS BUILT AND IS INHERITED RATHER THAN REDEFINED**, which is the
whole reason these two are milestones apart. `Number` carries a `bool positive_`
with the magnitude carrying none; negation, `abs`, the sign of a product and the
ordering of negatives are written once in `satellite_number/` and are true of
both types. **One thing about the layout does not carry over and is worth knowing
before the C++ is written**: a `Number` is 32 bytes only because the bool sits in
the padding after `int exp_`, and `bignum.hpp` measured that a float built by
wrapping two of them cannot repeat the trick. §8.6's own note about a
zero-initialised float reading as negative zero is the same fact from the other
side, and it is a difference between the two types rather than something M8 left
undone: a zero-initialised `Number` is the number 0, because `positive_` defaults
to true.

Two documents used to disagree about whether this was urgent — PLAN filed it under
"Later" while DESIGN §13 and QUAD.md §3.1 called it the critical path. It is the
critical path: QUAD is 164 `double`s and cannot be written without it.

**Four numbered paths, and three of them are M8's type rather than this one's.**
*(2026-08-28; seven on 2026-08-31, and four by the end of the same day.)*
`satellite.variable.float` `1 6 10`, plus `satellite.variable.number.power(a, b)`
`1 6 4 10`, `truncate(a)` `1 6 4 13` and `sqrt(a)` `1 6 4 14` — the three of the
number's fifteen methods that **cannot be finished before the rounding rule is
chosen**, which is this milestone's blocker and not M8's. `power` at a fractional
or negative exponent and `sqrt` are irrational in general; truncating a float is
its left half and so waits on the float itself.

**IT READ SEVEN FOR ONE DAY, AND THE THREE THAT LEFT AGAIN ARE WORTH KEEPING IN
THE RECORD.** `shift_left` `1 6 4 1` and `shift_right(n)` `1 6 4 11` arrived here
on 2026-08-31 because building M8 found that the first satellite has neither and
that no document said what a bit shift means on an exact base-10⁹ decimal — an
undecided question of the same kind as the rounding rule, so it went to the same
place. **The question was then answered rather than scheduled**: DESIGN §5.5 says
a shift is `× 2ⁿ` and `÷ 2ⁿ`, both terminate because 2 divides 10, and §8.6's
first class is where that puts them. `modulus(a, b)` `1 6 4 12` went the other
way without ever having belonged here at all — §8.6 specifies it in full and
files it under *exact and bounded, never rounds*.

**THE LESSON IS ABOUT THE SENTENCE THAT MOVED THEM.** *"None of them can be
finished before the rounding rule is chosen"* was written over four methods at
once, and it was true of two. A blocker named over a GROUP cannot be checked
against any one member, which is the same failure `MILESTONES/M7.md` §7.1 records
about a claim asserted over a whole behaviour. `SCRATCH.md/MILESTONE.md` §0.4
named the trade — *"either those four move to M15 or M15 moves ahead of M11"* —
and the answer turned out to be that only three of the four had to move
anywhere. **`satellite.library.system.float_digits` `1 14 2 4` is
this milestone's dial**, on the node M6 builds, and DESIGN §13 has already redefined
it from *the* dial into **the default length of a float's right half** for a value
that does not state one.

**And since 2026-09-03 this milestone owns the RUN-TIME RETUNE** — the author's
direction at M11's landing. §4.5.3's arrangement, *"the file seeds the namespace
at startup and the namespace is what everything reads afterwards"*, has had no
run-time half: no dial has a handler row, and the evaluator's Assign arm knows
frame slots and declared globals, not language paths. M8's entry expected the
retune at M11 and M11 landed without it, because what an assignment to
`satellite.library.system.division_digits` MEANS is undecided — a store the
machine's Policy reads back, or a handler-shaped write — and deciding it beside
ONE dial risks deciding it three times. It lands here because the third dial is
this milestone's own: one mechanism, decided once, in front of `1 14 2 1`
through `1 14 2 4` together — the reads are module-constant-shaped and M11
built that road, the write is the decision. MILESTONES/M11.md §6 carries the
hand-off, and the done-when below gains its clause.

**Done when the four operations run, `satellite.library.system.division_digits
= 40` retunes a running program's division, and — the blocker — the rounding rule is chosen.**
Truncate, half-up, or half-even. No representation escapes it: `pow` at a fractional
exponent is irrational, so the fractional half must be rounded to exist. QUAD's
determinism invariant means a program's behaviour depends on the answer.
*(All three clauses held on 2026-09-04: `example/floats.satl` runs the four
operations and the retune's own line, and the rule is DESIGN §8.6's — half
away from zero, with the two declined rules recorded beside it.)*

**Half of that decision is already made in code and nobody decided it.**
*(2026-08-31.)* `Number::divide` rounds **half-up** — it keeps one guard digit and
bumps when it is `5` or more — which is v1's behaviour, ported unchanged, and is
why `satl --number 2 / 3` ends in a `7`. So the language already rounds one way at
one operation. That is not the same as the rule being chosen: this milestone's
question is what rule the FLOAT uses and whether the number's division should
agree with it, and the honest position is that a default arrived by porting rather
than by decision. **`tests/number_test/arithmetic.cpp` asserts the current
behaviour**, so changing it is a visible edit and not a silent drift.

**M16 — containers and the search power. LANDED 2026-09-06** (`1fb1df7`), and
[MILESTONES/M16.md](MILESTONES/M16.md) is the review. `satellite.container.list`,
`satellite.container.map`, **and their methods** — the map's nine `1 4 1 1`–`1 4 1 9`
and the list's twenty-five `1 4 2 1`–`1 4 2 25` — plus the search power ported close
to unchanged.

**Forty-one paths as built, not thirty-nine, and the two extra were minted by the
author on 2026-09-05**: `search(pattern)` under the map at `1 4 1 10` and under the
list at `1 4 2 26`. v1 gives both containers the rich spelling — a map per hit with
the value, the key, the path and the score — and the 2026-08-28 transcription
carried no row for it, so the milestone that ports the power is the one that minted
the number. WORD_NUMBERS §2.6 records it.

**Three things it decided that no document had settled.** A declaration does NOT
construct a container — DESIGN §8.7's "a declared variable of any type holds
nothing" stands as written, and the bare call shapes `1 4 2 0` / `1 4 1 0` are what
build one, which is where v1 and this tree part company. A fold may land on the bare
word it was spelled from, which closes MILESTONES/M7.md §6 item 1 without minting an
alias. And `m[k] = v` is built, overruling v1's refusal, because DESIGN §12 lists it
and `l[i] = v` as ONE deferred entry that says "fixing one fixes both".

**And it found that WORD_NUMBERS §1.5's literal-option fold had never once run.**
M7 built it at `1 4 2 5`'s expense and nothing in the language could reach it — `sort`
is the only word with `<word>_<option>` siblings and there were no lists — so the
first folded call ever compiled was this milestone's, and it died on arity because
the compiler was still passing the absorbed literal. **That is PLAN §2's rule about a
registry needing a consumer, arriving four milestones late**, and it is the argument
for the rule rather than a lapse in it.

**That clause is a fix, not an addition.** *(2026-08-27.)* M11 writes "`.bool`,
`.number`, `.string` **and their methods**" and this milestone did not, so
**thirty-four** numbered paths sat under a type name that a milestone mentioned and
were owned by nothing that said so.

*(Was "twenty-nine" until 2026-08-28, in the same paragraph that names nine and
twenty-five two lines above — 9 + 25 is 34. The 29 came from
`SCRATCH.md/MILESTONE.md` §0.3 counting the list at **twenty** when §2.2 gives it
twenty-five; the ledger is corrected too, and its "35 implied" becomes 40. Found
while transcribing the numbering at M2, which is the first time anything counted
those rows rather than repeating the count.)* Same failure as M3/M4 and DESIGN §6.1's eleven words, one
milestone later.

**This milestone also owns the empty list `satellite.main`'s parameter binds to.**
*(Decided 2026-08-28.)* DESIGN §3's hello world declares
`satellite.container.list<satellite.variable.string> arguments` and never reads it,
so what it needs is one empty `satellite.container.list` and no `string` at all.
**That is this milestone's type, not M10's** — an empty list is still a list, and a
milestone that constructs one has built the type. **M17 therefore runs after this
one**, and it is why M10 was split at all; §8's opening carries the split.

**Thirty-nine numbered paths, and three of them are not under `container`.**
*(2026-08-28.)* `satellite.container` `1 4 (0)`, the map and its nine, the list and
its twenty-five — **and `satellite.system.threshold()` `1 22 5` and `(n)` `1 22 6`,
which move here from a namespace M20 otherwise owns.** They are spelled under
`system` because that is where a knob belongs beside `max_depth`, but they set how
loose a search may be over the ten-level ladder, and v1 says it in its own comment:
*"it is not a system FACT: uname and getpwuid answer what the machine is, and this
sets how the search behaves."* **The search power cannot ship without its dial.**

**`satellite.container.arguments` `1 4 3` and `satellite.container.result` `1 4 4`
are not this milestone's**, and saying so is what stops a reader taking "containers"
as the namespace rather than the two types. `1 4 3` is the type name M20's arguments
object answers to; `1 4 4` is §2.2's *"Satellite Orbit's answer"* and is M28's.

**Its second dial is M9's and it must not invent one.** The search walk reads
`satellite.library.system.max_depth` `1 14 2 2` — v1 has its own depth error text
for it, separate from the recursion ceiling's — and M9 is where the dial is built.
Two consumers of one dial is fine; two milestones each building it is not, and
neither entry said which until this pass.

~~**And as of 2026-08-31 this milestone is the dial's ONLY certain consumer.**~~
**AND AS OF 2026-09-01 IT IS NOT A CONSUMER OF IT AT ALL, WHICH IS THE DECISION
ARRIVING HERE.** That paragraph asked what M9's reading of `max_depth` would turn
out to be and said this milestone should take the path over if the answer were
*"a diagnostic aid"* or *"nothing at all"*. **The answer is neither: it is a
memory ceiling on the control stack, in bytes** (M9's entry has the argument), and
a count of levels is exactly what it is not — so there is nothing here for a
search walk to compare itself against.

**What this milestone owes instead is one sentence about its own walk.** DESIGN
§7.5 binds it either way: a search over nested containers is a depth the user's
program chooses, so it may not use the C++ stack for it. **If the walk runs on
M9's control stack it inherits M9's ceiling and M9's sentence and needs nothing**;
if it is a walker of its own, it keeps its own stack the way M8.5's four do and is
bounded by memory like everything else. Which of those it is, is this milestone's
design and not a dial. **Its search-tolerance knobs are unaffected and were never
this path**: `satellite.system.threshold()` `1 22 5` and `(n)` `1 22 6` are how
loose a match may be, and they are still M16's.

**The sort primitive is part of this milestone and is not the search power.**
`sort()` `1 4 2 3` through `sort_up(key)` `1 4 2 7` are §1.1's *one primitive rather
than comparators*; the search power is v1's comparator ladder, ported. Two different
things that happen to land together, and saying so is what stops the next reader
assuming "the search power" covered sorting.

**M17 — hello world. LANDED 2026-09-06** (`264a9c8`), and
[MILESTONES/M17.md](MILESTONES/M17.md) is the review. *(Split on 2026-08-28 from a
milestone then called M8, as
its second half; it was M8.B until the 2026-08-30 renumber. M10 is the other half,
the console, seven positions above — and two numbers say that where one number and
a letter hid it.)* DESIGN §3 runs, byte for byte. What is left once M10 has built
the console, `satellite.main` and `satellite.return` is **one thing: the parameter**,
and the parameter is why this half is here rather than there.

**`satellite.main` declares `satellite.container.list<satellite.variable.string>
arguments`, and this milestone must say what it hands over.** *(Restored 2026-08-28,
reversing the 2026-08-27 removal.)* For one day this half read "`satellite.main` takes no
arguments at this milestone, and that is what makes the milestone reachable",
resting on WORD_NUMBERS.md §2.2 writing `satellite.main` as `1 3 (0)` with `(0)`
read as *zero arguments*. WORD_NUMBERS §1.3 defines that marker as **a node reached
both bare and as a parent** — a fact about reaching `satellite.main`, not about
declaring it — so the argument was reading the authority backwards. DESIGN §3 has
the full reversal.

**What it costs is one empty list, and the author has decided whose it is.**
*(Decided 2026-08-28.)* Hello world never reads `arguments`, so no
`satellite.variable.string` value is ever constructed — **M11 is not a dependency** —
and none of M16's twenty-five list methods is reached. What remains is a single
empty `satellite.container.list` bound to the slot. **That list is M16's**, which is
what puts this milestone after M16 and is the whole reason M10 was split at all.

**The reason it went that way rather than the other is that owning it here would
have been M16's type built twice.** An empty `satellite.container.list` is still a
`satellite.container.list`; a milestone that constructs one has built the type, and
the type belongs to the milestone that says so. The alternative — a private
empty-list shape that M16 later replaces — is the kind of thing that looks free and
is discovered later as two implementations of one type.

**It hands `satellite.main` a slot named `arguments` before anything can make it
real**, and §7.7 puts the recognition of the name at resolve, which is M7. A program
written between here and the milestone that builds §7.7 will ask for
`arguments.username` and get an error that no document predicts unless this
milestone writes the handover down. That is the M3/M4 failure caught before it
happens instead of after.

**`example/hello_world.satl` is the done-when**, rather than a paragraph describing
one. DESIGN §3 is a byte-for-byte copy of that file so the two cannot drift. The
`arguments` **object** — `arguments.username`, `arguments.machine.cores` and the
rest of §7.7 — belongs to the milestone that builds §7.7, and **that milestone does
not exist yet**; `SCRATCH.md/MILESTONE_DRAFTS.md` has the draft. **Startup is
measured again against M1's number here**, because this is where DESIGN §3 itself
first runs.

**M18 — `satellite.help`, and the trie answering for itself.** *(Its own milestone
as of 2026-08-28.)* Three paths — `satellite.help` `1 19`, `satellite.help()`
`1 19 0`, `satellite.help(x)` `1 19 1` — **moved here out of
`SCRATCH.md/MILESTONE.md` §0.1**, which had them under *"nothing"* and sized them as
the cheapest row in the ledger: *"3 paths and DESIGN §4.6 makes it a walk of the
trie — nearly free once M2 lands."*

**Everything it needs is behind it.** M2 gives the trie and the interner, M5 the
refusal text, M9 the `handlers[path_id]` table, **M10** the console to print
through. Nothing later is required, which is the argument for putting it here rather
than at the end: **help that arrives last is help nobody had while the language was
being built.**

**Its real floor is M10, not M17, and that is worth knowing.** *(2026-08-28.)*
Help needs a console and a `main` to run inside; it does not need the parameter, so
nothing stops this milestone landing immediately after M10 and giving M11, M15 and
M16 a live account of themselves while they are being built. It is left after M17
because that is where the split put it and moving it is a second decision — but **if
help is wanted during M11 and M16, this is the one that can move, and it moves
without consequence.**

**Help is a walk, not a document, and v1 is the evidence for why.**
`old_versions/first_satellite/src/evaluator/help.cpp` is 221 lines and
`help_topics.cpp` another 315, all of it string literals, and it had already
drifted from the language it describes. `help_for_module()` answers for six modules
— `console`, `time`, `file`, `directory`, `system`, `random` — and returns the empty
string for every other name, while its own comment states the contract that makes
that a defect: *"An empty answer means the name is not a module, which is how both
callers tell."* `satellite.analyze` is a module in v1, advertised two groups above
in the same file's overview as `satellite.analyze("file.satl")`, and asking help
about it answers as though it does not exist. **A help text that drifts is the
thing DESIGN §4.6 exists to end**, and it ended up inside the file that was supposed
to be the language's own account of itself.

**§4.6 needs one correction before it is safe to build, and this milestone is where
it lands: the trie is what is *numbered*, not what is *built*.** §4.6 says *"help
cannot drift from what exists — the trie **is** what exists."* After M2 the trie
holds every path in `words.def`, so a help that walks it at this milestone
advertises `satellite.network.https(port, cert, key)` `1 20 7` and the other 121
paths the ledger counts as unscheduled. That is **worse than v1**, which at least
only listed what somebody had written, and it breaks DESIGN §1.1 outright — telling
a user a path works when nothing implements it is doing something behind their
back, and *a refusal in plain words beats a guess.*

***AND THAT RULE, AS WRITTEN, CANNOT SATISFY THIS MILESTONE'S OWN DONE-WHEN.***
*(Found 2026-09-07, before any of it was built; the author settled it the same
day.)* The done-when below asks for **"everything through M16 and M17, in build
order"** — and M17's four paths are `satellite` `1`, `satellite.include` `1 1`,
`1 1 0` and `1 1 1`, **none of which has a handler row and none of which ever
will**. `include`, `capsule`, `main`, `return` and the type names are recognised
by the parser, the resolver and the compiler and are never dispatched: of the six
paths hello world is written in, **exactly one — `console.display` `1 5 1` — is a
row in that table.** So help-as-specified would print `satellite.console.display`
and stay silent about every other word in the language's own first program. That
is not "cannot omit what can run"; it is omitting the whole front end.

**The author's answer is a `built()` predicate over three kinds**, and it is this
milestone's real content: a node is named by help when it has a **handler** row,
an **assigner** row (M15's dials), or is a **front-end word** the parser or
resolver recognises. There is no such predicate in the tree today —
`words_nodes.hpp` has only `is_language_word` — and the front-end set has to be
written down as data rather than inferred, or it drifts the first time a word
moves. **§4.6's sentence then needs one more word than it has**: the trie is what
exists, the handler table is what *runs*, and help answers for what is *built*,
which is the larger set. DESIGN §4.6 points here for the argument, so the
correction lands there when this milestone does.

**The fix needs no new machinery: help prints a node when `handlers[path_id]` is
non-null.** That table is already the dispatch mechanism (DESIGN §4.5, and this
plan's §6 where M9 builds it), so **the same table that decides whether a call runs
decides whether help mentions it.** Help cannot advertise what cannot run and
cannot omit what can. §4.6's sentence then becomes true as written, one word
narrower — the trie is what exists; the handler table is what *works*.

**The value listing stops being a switch, and that closes an open handoff.** v1's
`help_for(const Value &)` dispatches on `value.index()`, one of exactly two places
in v1's tree that read a raw variant index, and the **M10.5 draft in
`SCRATCH.md/MILESTONE_DRAFTS.md` leaves its new arm *"either taken here or left for
whoever gets `satellite.help`."*** This milestone takes neither, because the switch
should not exist: **a value's type is a node and its methods are that node's
children.** `satellite.variable.string` is `1 6 1` and its sixteen methods are
`1 6 1 1`–`1 6 1 16`, so *what does this value answer to* is the same walk started
lower down. The function is deleted rather than extended, and M10.5 appending
`ResultRef` to `Value` stops touching help at all.

**The three shapes are one walk at three depths.** Bare `satellite.help` and
`satellite.help()` start at the root; `satellite.help(x)` starts at `x`'s node. The
parentheses being optional is why WORD_NUMBERS §2.2 carries a bare `1 19` row
alongside `1 19 0` and `1 19 1` — one of only three parents in that table written
without the `(0)` marker (`SCRATCH.md/THREADS.md` finding 211 has the other two).

***THE ARGUMENT IS NOT AN EVALUATED ARGUMENT, AND NOTHING SAID SO.***
*(2026-09-07.)* `satellite.help(satellite.console)` cannot work as an ordinary
call: a bare language path read as a value compiles to `op_dispatch` and refuses
at run time, so the argument dies before help is entered — **and it refuses
identically for a module that IS built**, measured:

    satl: error S0721: `satellite.console` is a path satellite has a number
          for and nothing behind yet -- a later milestone

So the done-when's second self-verifying check —
`satellite.help(satellite.network)` refusing in plain words — **passes today by
accident, through the wrong mechanism**, and would give the same answer to the
central question help exists to answer. `1 19 1`'s argument is therefore a
**path**, or a **name whose declared type resolve already knows**
(`resolve::Info::type`), and in neither case is anything evaluated. That makes it
the first unevaluated argument in the language, which is a language decision and
is recorded here rather than discovered during the build.

**The author settled the shapes around it the same day**: a bare word that
nothing declares is refused with `no variable with the name "x"; did you mean
satellite.x?` and a pointer to `satellite.help()`; **quotes are gone** — a topic
is reached by its path, `satellite.help(satellite.random)`, and
`satellite.help()` lists them; and `satellite.help()`'s own text is rewritten to
say that a variable's name can be passed and help will answer for its type.

**Open, and it is the one thing here that is not already decided: a topic is not a
path.** v1 accepted `satellite.help(random)` and `satellite.help("random")` for
seven topic pages — `arguments`, `random`, `fast`, `normal`, `ultra`,
`random.ultra`, `wide` — and none of those is a path with a number. Under §1's
generating rule a bare word is user-owned, so **this is the second place in the
language where a bare identifier means something language-owned**, the first being
§7.7's six spellings of `arguments`. It is the same shape and it should be settled
the same way: the language *recognises* a name rather than introducing one. Whether
the seven survive at all, and where their text lives if they do, is undecided —
**but it must not be a second document**, because a second document is the drift
§4.6 is removing. Per-node text in `words.def` is the shape that keeps the walk the
only source.

**Not this milestone.** `satellite.analyze` `1 16`, which still has no milestone
anywhere, and the topic *pages* as prose. Adding a node's one-line description to
`words.def` is this milestone; writing seven essays is not.

***AND THE STORE IS BUILT. M22 BUILT IT ON 2026-09-07, THE SAME DAY.*** The
author asked for the prompt's variables to persist, and the mechanism that does
it IS the store: `satellite::prompt::Session::kept()` answers a vector of
`{name, type, value}` for everything the session is holding — a typed line's
locals and a file's alike, because `run <file>` and `satl -i` keep a program's
variables the same way. **The type is already the node its declared type ends
at**, which is exactly what `satellite.help(x)` needs to walk, so help reads a
list rather than building one. `satellite.system.persist` `1 22 7` / `1 22 8` is
what turns it off. **So this milestone inherits a filled store with a reader
waiting**, which is the shape M2's rule asks for arriving from the other
direction. MILESTONES/M22.md §2.9.

**WHAT REMAINS OF THE PARAGRAPH BELOW IS THE READING, NOT THE KEEPING.**
*(2026-09-07.)* The author's account of help includes asking about a **variable**
— `satellite.help(my_name)` answering with the node its declared type ends at,
which `resolve::Info::type` already knows statically, so no value is ever
evaluated and `1 19 1`'s argument is a path or a name and never a run-time thing.
Asking **after the program has finished** is the part that needs keeping: the
last run's names and types, discarded when the terminal closes. `resolve::Frame`
carries `names` and `types` for every capsule already, so the data is computed;
what M22 built is the prompt it is asked at. **The store belongs here and not
there**, for M2's rule — help is what reads it, and a registry needs its consumer
in the milestone that writes it. MILESTONES/M22.md §6.2.

**Done when** `satl` runs a program whose whole body is `satellite.help` and the
output names exactly the paths that are built when it runs — everything through M16
and M17, in build order, and no others — so the same unedited program run again at
M24 prints a different and equally correct language. Two
checks make it self-verifying, which no earlier milestone is: the output is
comparable to the non-null entries of `handlers[]` by construction, and
`satellite.help(satellite.network)` **refuses in plain words** rather than printing
seven shapes nobody has written.

**M19 — persistence: files and directories.** *(New 2026-08-28, corrected from the
2026-08-27 draft. After M18.)* **Twenty numbered paths, and four more minted on
2026-09-08 when the author settled the open items below** — `ok` `1 6 2 8`,
`path` `1 6 2 9`, `error` `1 6 2 10`, `write(x)` `1 6 2 11` and
`exists(path)` `1 8 5`, which is five rows for four questions because `exists`
now has a module face beside the handle's. **Of the twenty-five, twenty-one are
built and four are not**: the three bare `(0)` shapes are not callable rows, and
`1 6 2 1` is a number with nothing behind it by the decision recorded below. The
original twenty sit in three of
`SCRATCH.md/MILESTONE.md` §0.1's rows — the 5th, 8th and 10th of seventeen, not a
contiguous block and not the largest one; `satellite.system` alone was thirty:

- the module face — `satellite.file` `1 8 (0)`, `.new(path)` `1 8 1`, `.open`
  `1 8 2`, `.clear` `1 8 3`, `.new(path, mode)` `1 8 4`, which is WORD_NUMBERS
  §1.3's own worked example of the variadic split
- the handle — `satellite.variable.file` `1 6 2 (0)`, `new` `1 6 2 1`, `open`
  `1 6 2 2`, `read_line` `1 6 2 3`, `write_line(s)` `1 6 2 4`, `read_all`
  `1 6 2 5`, `close` `1 6 2 6`, `exists` `1 6 2 7`
- the directory — `satellite.directory` `1 18 (0)`, `.change` `1 18 1`, `.current`
  `1 18 2`, `.exists` `1 18 3`, `.list()` `1 18 4`, `.list(d)` `1 18 5`
- **`satellite.system.delete` `1 22 1`**, which is the twentieth and which the draft
  argued at length belonged to somebody else — see below.

**Of the nineteen in the first three groups, "Later" reached only
`satellite.variable.file`'s eight, and Later is not a milestone.** The other eleven
were reached by nothing: §0.0 corrected the reading that had Later covering
`satellite.file` `1 8` — Later names `satellite.variable.file` and
`satellite.variable.time`, **different nodes**, which is the same one-spelling-two-nodes
hazard DESIGN §4.4 and WORD_NUMBERS §2.3 describe. **This milestone strikes
`satellite.variable.file` from that list** and closes `SCRATCH.md/MILESTONE.md`
§1's five-methods row.

**It writes no new syntax, and a plan for it that contains a parse rule is wrong.**
M4 owns DESIGN §6.1's eleven segment-1 words, `variable` among them, so
`satellite.variable.file f = satellite.file.open(p, "read")` already parses as a
declaration on `path[1] == "variable"`; §6.1's last row parses `satellite.file.*`
and `satellite.directory.*` as modules, needing a `(`; and §6.2's postfix loop gives
`f.read_line()` and `satellite.file.new(p).close()` for nothing. That is M4 owning
the eleven words and not saying so, one milestone further on — so this one says so.

**What it does not own.** M2 registers all twenty nodes and their numbers, **because
all twenty are in §2.2** — which is M2's own rule, *"a transcription of
WORD_NUMBERS.md and nothing else"*, and not because the v1 sweep found them; five of
them (`1 6 2 3`–`1 6 2 7`) came from settling QUAD.md §3 on 2026-08-27, among the
71 numbers that took §2.2 from 144 rows to 215. M5 owns the shape of every message
here, **but not the mode-word suggestion** — see the open item. M9 owns `Value`'s
reference-type handle alternative and §6.4 qualification 2's receiver-binding tag,
**and M9's own entry now says both**, because a constraint that lives only in a
later milestone's prose is the constraint that gets settled by accident. M16 owns
the list that `1 18 4` and `1 18 5` return, sorted by M16's `sort()` `1 4 2 3` and
not by a second sort here.

**SATC.md §5's atomic write is a different mechanism**, not an early version of this
one: tmp, `fsync`, rename is M4.5 doing the interpreter's own C++ file I/O, and a
reader who has just landed M4.5 could reasonably think the ground was taken.

**`satellite.system.delete` `1 22 1` moves here, and the demonstration is why.**
v1 argues in twenty lines that `unlink` acts on a **name**, so one verb covers a
file and an empty directory and belongs under neither — which is why the number is
under `system` and why M20 does not have it. But v1's body accepts an **open
`satellite.variable.file` handle** as well as a string, so the only milestone that
can give it its second argument shape is this one; and without a removal verb
**this milestone's own done-when runs exactly once.** `1 8 1` is `O_EXCL` — *new
says make one that is not there* — so on a second run the first `new` is the call
that comes back not-ok and the round trip fails, while `1 8 3` clear *"empties a
file that goes on existing"*. A done-when that passes once and cannot restore its
own tree is not a thing that works and can be demonstrated. **Taking `1 22 1` here
is the smaller of the two fixes** and it removes a circular dependency the draft
asserted from the outside.

**`satellite.directory` has no create verb, and that is a real hole under a node
this milestone otherwise finishes.** §2.2 gives it change / current / exists /
list() / list(d) and nothing that makes a directory. The done-when below is written
so that it needs none; **whether one is minted is the numbering's, and it is named
here so it is not discovered inside a demonstration.**

**The port is a rewrite, and what survives it is the arguments.** About 849 lines
under `old_versions/first_satellite/src/`: `modules_file.cpp` (304),
`methods_file.cpp` (324), `modules_directory.cpp` (221), plus `FileHandle` in
`satellite_value/value_types.hpp` and the six file/directory `format.def` rows,
which are the arity evidence WORD_NUMBERS §4 says has to be read out of the v1
evaluator rather than guessed. Two of the three files are already over §3's target.
Every arm is the `full == "satellite.file.open"` string compare §7 throws away;
every failure is `fail()`-then-return-`nullptr`, which DESIGN §9.1 throws away. What
ports is the comment culture §6 says to keep — the `O_RDWR|O_APPEND` paragraph, the
rewind paragraph, the `O_EXCL` paragraph, the `.` and `..` paragraph, the
sort-as-`SatString`s paragraph. The `std::atomic` fd comes across with it, because
DESIGN §8's reference semantics are what make two handles race; **M23 is when that
first gets exercised, not when it gets written.**

**`helpers_listing.cpp` and the two `helpers_file_facts` files do not port** — 418
lines of columnar `ls`-style rendering, with no number in §2.2 and nothing to do
with `.list`, which returns plain sorted names. A reader sweeping v1 for "file"
finds that half first. **It is not only the REPL's echo, though**: v1 also reaches it
as a list method, `.lines()`, and v1's own comment says the rendering exists because
*"the commonest list anyone types at this prompt is `satellite.directory.list()`."*
`.lines()` has no number in §2.2, and **whether it gets one is M16's question or
M22's, not this milestone's to settle by declining it.**

**Open no longer — every one of these was settled by the author on 2026-09-08,
before a line of the milestone was written, and the answers are recorded here
rather than in the milestone note alone, because a question closed only in a
review is a question the next reader of this entry still thinks is open:**

- ~~**The failed-open contract has no numbers.**~~ **Three, minted 2026-09-08:**
  `ok` `1 6 2 8`, `path` `1 6 2 9`, `error` `1 6 2 10`. v1's contract exactly —
  a failed open is a VALUE, the handle comes back holding `errno`, and the
  caller asks. **v1's handle `clear` does NOT come across**, which is what
  settles `1 8 3` two bullets down: with no handle spelling, the module face is
  the only spelling and is not a duplicate of anything.
- ~~**The mode word is not a trie level.**~~ **They do not fold** — see M7's
  bullet above, which this milestone corrected by measurement. The bad-mode
  message is a runtime check this milestone owns, using M5's *reporter* and not
  M5's *suggester*, and it names all four words.
- ~~**Eleven of the twenty rows carry no call shape.**~~ **Written into §2.2 on
  2026-09-08**, out of v1's recorded arities and not guessed:
  `open(path, mode)` `1 8 2`, `clear(path)` `1 8 3`, `change(d)` `1 18 1`,
  `current()` `1 18 2`, `exists(d)` `1 18 3`, `delete(x)` `1 22 1`, and
  `exists(path)` on the new `1 8 5`. **`words.def` carries the same shapes, and
  it has no choice**: `words_walk.hpp`'s `match_shape` compares a row's argument
  list to the written one CHARACTER FOR CHARACTER, so a §2.2 that says
  `open(path, mode)` over a registry that says `open` is a path `words_test`
  cannot walk. The two files are one fact and this is the mechanism that says so.
  **The shapes are also what makes the arity a resolve-time fact rather than a
  handler-table one** — §1.3's whole claim — and they are the tree's own pattern:
  `sleep(n)`, `find(x)`, `split(separator)` and `write_line(s)` are all single-arity
  rows carrying their shape, and `display` is the outlier. Measured before the
  change: `satellite.file.open("x", "read")` resolved to `1 8 2` through the
  bare-word arm and refused at the handler table; `satellite.file.open("x")`
  resolved just as happily and would have refused one layer later.
- ~~**Which spelling constructs a file.**~~ **Confirmed, and WORD_NUMBERS §4's
  paragraph now says so.** A program writes `satellite.file.new(path)` `1 8 1`;
  `1 6 2 1` is a number with nothing behind it, because §6.4 qualification 2
  names `my_file.new()` as the confusing arity error the receiver tag EXISTS TO
  PREVENT rather than as a spelling to build. `open` is not a second collision:
  `1 8 2` opens a path, `1 6 2 2` reopens a handle.
- ~~**`1 8 3` `clear` has no v1 module form to read.**~~ **It is `clear(path)`,
  the module face, and it is the only face** — the handle keeps no `clear` of
  its own, per the first bullet. Its siblings under `1 8` all take a path and so
  does it.
- ~~**`read_line` `1 6 2 3` wants the one thing v1 refused.**~~ **It advances a
  per-handle cursor**, and DESIGN §8.7 had already required it: *"`read_line`
  answers a line, or nothing"*, which is a sentence with no meaning on a handle
  that rewinds to 0 every call. `read_all` `1 6 2 5` reads the whole file **from
  the beginning** and leaves the cursor **at the end**, so a `read_line` after
  one answers nothing — the two verbs share one offset and neither pretends
  otherwise. What v1 actually refused was *read from wherever the offset happens
  to be*, and a cursor the language advances is not that.
- ~~**`write_line(s)` `1 6 2 4` overrules an argument that is written down.**~~
  **Both verbs exist and neither argument loses.** `write_line(s)` writes `s`
  and a newline. **`write(x)` `1 6 2 11`, minted 2026-09-08, writes exactly the
  bytes** — v1's `.write`, under the name that says so. That is what keeps a
  file with no trailing newline and a line assembled from several writes
  writable, and it is the verb **M19.5's `binary` and `hex` values are written
  with**.
- ~~**`exists` `1 6 2 7` sits on the type node.**~~ **Both, and they are two
  questions.** `satellite.file.exists(path)` `1 8 5`, minted 2026-09-08, is
  *is there a file there* asked with no handle, mirroring
  `satellite.directory.exists(d)` `1 18 3`. `1 6 2 7` keeps the narrow question
  — *is MY path still there* — which is what the done-when's last clause asks
  after a `satellite.system.delete`.
- ~~**`.list` is the one failure in its module that is an error and not a
  value.**~~ **Re-affirmed rather than inherited.** The empty list already means
  an empty directory; spending it on *there was no directory* makes the two
  indistinguishable, which is the silent wrong answer DESIGN §9 exists to
  refuse. Loud costs a caller one `.exists(d)`; quiet costs them a program that
  lists a mistyped path as empty and does nothing.
- **`satellite.directory` still has no create verb, and none is minted.** The
  done-when needs none, and a verb minted to round out a namespace is a verb
  designed by symmetry. It stays named here and in `MILESTONES/M19.md` so that
  the next milestone to want one finds a hole rather than a surprise.

**Done when two programs run under `satl --run`.** The first is a round trip over
all twenty paths printing one `PASS`, and it **leaves the tree as it found it**, so
it can run twice: create with `1 8 1`, confirm a second `new` on the same path comes
back as a value that is not ok rather than clobbering the file (DESIGN §1.1 — *never
silently truncate*), write N lines, `close` and read the status, reopen with `1 8 2`
and `read_line` until nothing with the lines identical and in order, `read_all` and
assert it equals them joined, **open a path that does not exist, print the reason,
and keep running** — DESIGN §9's ordinary case, and the one assertion that cannot be
faked — ask for a mode word that is not one of the four and get a code, a span and a
caret naming them, then `1 18 2`, `1 18 4` with the new file in it, `1 18 3` true
then false, `1 18 1` into a directory that exists and a `1 18 1` onto a plain file
that returns false without dying, `1 18 5`, `1 8 3` leaving the file at zero bytes,
and `1 22 1` removing it — **both shapes, the path and the open handle** — with
`1 18 3` false afterwards.

The second is **QUAD's corpus reader** — `quad_main.cpp:184–197` — written in
satellite by hand against DESIGN.md: `read_line` until nothing, skip empty lines, a
line starting `"# "` opens a document (`starts_with(x)` `1 6 1 6`,
`substring(start, end)` `1 6 1 5` — §2.2 has no one-argument shape, so the call is
written with both), count lines and documents, print the two counts, and they must
match the C++ QUAD's on the same file. It needs no float and no `pow`.

**It is not the milestone QUAD.md §4 asks for, and this milestone does not claim to
be.** §4 wants *"one mechanism out of `mind.hpp`, chosen because it exercises floats,
containers, sorting and persistence at once"*; the corpus reader is in
`quad_main.cpp`, touches no float and no sort, and QUAD.md §5, `MILESTONE.md` §4 and
SESSION.md §5.7 all independently name the candidate as `Sky::decay` plus
`Rack::draw`. **That is M21**, and claiming it here would strike the ledger's row
and orphan the one thing three documents agree on.

**Its slot, stated as a relation rather than a decimal.** After M16, because
`.list()` returns M16's list and there is no faking it — a directory listing that is
not a list is not the thing. The other seventeen need only M11's strings: a path is a
string, a mode word is a string, a line is a string, and a `.sky` record is
`split(separator)` `1 6 1 10` and `to_number` `1 6 1 13`. **It inherits M15's
undecided rounding rule only through M16**, so if the float stalls, the thirteen
paths under `1 8` and `1 6 2` can run at M11 — that is the seam, and splitting there
orphans `satellite.directory` a second time, which is the precise failure
`MILESTONE.md` exists to record.

**M19.5 — binary and hexadecimal.** *(New 2026-09-08. After M19, before M20.)*
***LANDED WHOLE: BINARY 2026-09-08, HEX 2026-09-09.*** *The split was the
author's — `satellite.variable.binary` `1 6 5` first, so the type's shape could
be settled on one radix before the second inherited it — and it paid: every
question the second half faced was either already answered by the first or was
genuinely new to hex, and the new ones are named in DESIGN §8.5. Both radices
carry six methods at matching numbers, `1 6 5 n` and `1 6 11 n` asking the same
question for every n. MILESTONES/M19.5.md is the review, DESIGN §8.5's
subsection is what got decided, and the entry below stands as it was written so
that the two can be compared.*

***AND THE OPERATORS ARE NOT IN IT.*** `+`, `!!`, `[` and the shifts were all
DECIDED on 2026-09-09 and none was built, at the author's call — *"we are doing
just too much at one time"* — after the author spotted what the operator set
actually costs: *"we forgot about order of operations like completely"*. The
answers are written into DESIGN §8.5 and §6.6 so the milestone that builds them
starts from them rather than re-arguing them. **A method needs a `words.def`
row and an operator does not**, which is the line that made leaving them out
cost nothing: not one path number is spent by the delay.
**Two numbered paths and no third** — `satellite.variable.binary` `1 6 5` and
`satellite.variable.hex` `1 6 11`, which WORD_NUMBERS §2.2 has carried since M2
and which no milestone in this list has ever claimed.

**It exists because DESIGN §8.5 said out loud that it had to.** That section has
specified `x00FF` and `b1010` since the language was written — *"real types with
literals, and the width is part of the value: `x0009` is not `x9`"* — M3 lexes
both, and §8.5 records in capitals that M9 went looking for the milestone that
builds them and found none. The answer until 2026-09-08 was S0720, *"a binary or
hexadecimal literal parses and does not run yet — no milestone in PLAN.md §8 owns
the value"*, which is DESIGN §1.1's honest refusal and is not a place to stop.

**It is a design milestone and not a port, and that is the whole reason it is
its own number rather than a paragraph inside M19.** §8.5 specifies the literals
and the width rule **and nothing else**. Nothing anywhere says whether a bit can
be indexed, whether two binaries concatenate, whether either converts to a
`satellite.variable.number` and what happens to the width when it does, what
`b1010 == b00001010` answers, or which of the two the `.satc` writes. v1 has no
answer to read out of: **there is no binary or hex value in the first satellite
at all** — §8.5 is new design, not a migration — so the arity evidence
WORD_NUMBERS §4 says to read out of the v1 evaluator does not exist for these
two. Every one of those is a sentence somebody has to write, and writing them in
the margin of a milestone that already owns twenty-five file paths is how a type
gets decided by accident.

**Why here and not later.** M19 gives bytes a destination:
`satellite.variable.file.write(x)` `1 6 2 11` is byte-exact and takes a
`satellite.variable.string` on the day it lands. **The verb does not change when
these two arrive** — it gains two arms and no new spelling — so the milestone
that makes `write` mean what the author asked it to mean is the one directly
after the milestone that built it. Waiting would leave `write(x)` looking like a
duplicate of `write_line` for however long the gap ran.

**What it must not do.** There is **no `"binary"` mode word on
`satellite.file.open`**, and DESIGN §8.5 now says why at length: a satellite
string already holds arbitrary bytes and POSIX has no newline translation to
switch off, so the word would arrange nothing while telling a program that
something had been arranged. This milestone adds value arms, not a fifth mode.

**Its floor is M19 and its ceiling is `Value`.** DESIGN §8.2's forty-byte assert
is the constraint that shapes the representation: the width is part of the value
and a width is unbounded, so neither type lives inline any more than a float
does — they arrive behind handles, sixteen bytes each against the 32-byte widest
arm, appended at the END of the variant per value.hpp's append-only rule. That
is the sixth and seventh appends and the assert does not move.

**Two things to know before writing a line of it, both paid for at M19:**

- **A method on a call's answer does not compile.** `h.to_number().to_string()`
  is S0720 — a selector folds only through a DECLARED name, WORD_NUMBERS §1.5's
  one hop, which that section now states as a consequence rather than leaving
  each milestone to discover. **This milestone will feel it more than most**,
  because a conversion verb is exactly the shape people chain. Put a name in
  between; no milestone owns loosening it.
- **`make <name>_test` exiting 0 is not the same as the binary being new.** A
  compile error leaves the previous binary in place, and running it by hand
  then prints `ok` from code that predates the change —
  `make_support/065-tests.mk` carries the whole family of this failure. Check
  the mtime.

**Done when** a program declares one of each, displays them at their written
width, and writes both to a file with `write(x)` — and `satl --compile` no
longer has S0720 in reach for a literal, because there is a producer.

**M19.6 — the `.satc` written after resolve, and the option token.**
***LANDED 2026-09-09. MILESTONES/M19.6.md is the review.*** *(New
2026-09-08, at the author's ask. Slot open — after M19.5 in the list, and
nothing depends on it.)*

**The three open questions below were answered by the author on 2026-09-09 and
the entry is kept as written, because it is the plan this was built against.**
The token is spelled `0#down` and **names the OPTION**; a program that parses
and does not resolve is no longer cached; **the selector does NOT take its
number** — declined with the measurement in hand, and M19.6.md §2 carries why.
**The stamp does answer the stale-fold question today**, because
`satellite.include(satellite)` is the only include there is and a declared type
therefore always moves the source's own mtime; M25 is when that stops being
true, and it is item 3 of M19.6.md §5.

**And the move measured free** — §3 of the review, against `89dc0c4` on the same
machine with the same flags. **One measurement had to be thrown away to say so**,
and the reason it is in the review is that the wrong number was the believable
one. **No new paths.** It is a change to the cache format
and to when the cache is written, and it closes a hole WORD_NUMBERS §1.5 has
carried since M2 by giving up on it.

**The author's idea, in the author's words: a token that encodes an option —
`#` stands for a number in a `.satc` file, so `0#` stands for an option.**
`my_list.sort("down")` folds to `sort_down()` `1 4 2 5` at resolve, and today
that decision is taken again on every run. Writing it down is the obvious move
and SATC.md §5.1 currently forbids it:

> **A literal option is not folded here.** [...] §3's "literals stay literal"
> governs the file. The `.satc` keeps `"down"`. The runtime keeps the number.
> This is the one place the numbering deliberately says more than the file does,
> and it is not a contradiction as long as nobody tries to make the file say it.

**THE TOKEN ANSWERS THAT OBJECTION AND IS NOT WHAT BLOCKS IT.** §3's rule is
about literals, and `0#down` is not a literal — it is a third kind of token
beside `#1.4.2.5` and `"down"`, so §3 gains a row rather than losing one. What
actually blocks it is **ORDER**, measured 2026-09-08:

    out.satc = cache::satc_text(out.parsed.ast, words, out.stamp);

**The writer takes the PARSE TREE and nothing from resolve**, in both arms of
`programs/cache_command.cpp`. Whether `"down"` is an option depends on the
receiver's declared TYPE — `sort` on a list takes options and `sort` on anything
else does not — and a type is resolve's answer. §1.5 says exactly this about the
neighbouring case: *"at the moment the file is written the selector's identity is
unknowable."* So the milestone is not "add a token", it is **move the write to
after resolve**, and the token is what the move makes writable.

**And the same move settles the bigger one §1.5 gave up on.** That section's
table has a row saying a selector is "numbered for dispatch only" and "stays
bare" in the file, for the same reason — the receiver's type is not known yet.
After resolve it is. **Whether a selector should then carry its number is this
milestone's real question and it is not obviously yes**: SATC §3.1 keeps
selectors bare, `words_runtime.hpp` warns that a user's PathId is valid inside
one run only, and a language-owned selector's number is a property of the build
rather than of the run — so the two halves of §3.1's rule have different answers
and the milestone has to say which is which.

**What it must not cost: §5's promise that the first run is never slower.**
`cache_command.cpp` starts the write ON A THREAD and does not wait for it —
*"the write happens beside the work instead of in front of it"*. Resolve runs
before the program does, so moving the write after resolve keeps that whole
argument intact and delays the thread's start by one pass. **Measure it rather
than assert it**, which is what §9 asks of every milestone and what M4.5's own
numbers exist to be compared against.

**What it must not become: a cache that is wrong rather than stale.** SATC §4's
whole safety story is that a `.satc` is a faster spelling of the source and a
bad one is detected and ignored. A file that records a FOLD records a decision
taken about a program's types, so the day a declared type changes, a stale
`.satc` would carry a fold that is no longer the right one — and unlike a stale
number, a stale fold reads as a legal program that calls the wrong row. The
stamp is what has to answer that, and whether it already does is the first thing
to check.

**Open:**

- **Whether a program that parses and does not RESOLVE still gets a `.satc`.**
  It does today, in the second arm above, and it stops being possible.
- **What `0#` is spelled**, which is the author's to pick — the sketch is theirs
  and §1.1.1's argument about `#1.5.1` closing up to a float is the trap to
  avoid repeating. **The option itself carries NO QUOTES** — the author,
  2026-09-08 — so it is `0#down` and never `0#"down"`: the prefix has already
  said this is an option rather than a string, and quoting it would be the file
  saying the same thing twice in two notations that could disagree. That also
  keeps §3's line clean from the other side, since a token with quotes in it
  would read as a literal wearing a prefix.
- **Whether the option token names the OPTION or the ROW.** `0#down` says which
  word was written and leaves the fold to the reader; `#1.4.2.5` would say the
  answer and lose what the program said. §3's "literals stay literal" points at
  the first, and so does the ability to print the file back as source.

**And the same two working notes M19.5's entry carries apply here**, because
this milestone rewrites `satc_test` and will write fixtures: a method on a
call's answer is S0720 (WORD_NUMBERS §1.5's one hop, named there), and a test
alias exiting 0 does not mean the binary was rebuilt — check the mtime, and
`make_support/065-tests.mk` says why in three places.

**Done when** a `.satc` for a program containing `my_list.sort("down")` carries
the option token, reading it back reaches `1 4 2 5` without the resolver
deciding again, `satl --satc` prints the file legibly, and M4.5's startup
measurement is re-taken and recorded.

**M20 — the machine's facts, in the language.** *(New 2026-08-28, corrected from
the 2026-08-27 draft. After M19.)* **Thirty-seven numbered paths** — the
`satellite.system` namespace, the `arguments` object DESIGN §7.7 specifies, and the
type name an error message needs. It is the second-largest single milestone in
this list by paths, behind M16's thirty-nine once `threshold` moves there — **the
two are within two of each other, and the comparison is over behaviour rather than
over `words.def`, since M2 names all of them.**

- **`satellite.system` `1 22 (0)` and twenty-six below it**: `.environment`
  `1 22 2`, `.home` `1 22 3`, and all of `.memory` `1 22 4 (0)` — `.bit` `1 22 4 1`
  and `.frequency` `1 22 4 2` off SMBIOS type 17, `.main()` `1 22 4 3`, `.swap`
  `1 22 4 4 (0)` with `1 22 4 4 1`–`1 22 4 4 6`, `.this` `1 22 4 5 (0)` with
  `1 22 4 5 1`–`1 22 4 5 5`, then **`.free()` `.total()` `.used()` at `1 22 4 6`–
  `1 22 4 8`, `.main`'s unit shape at `1 22 4 9` — six slots from `.main()`
  `1 22 4 3`, which is §1.2 doing exactly what it promises — and `.free(unit)`
  `.total(unit)` `.used(unit)` at `1 22 4 10`–`1 22 4 12`.** *(That enumeration is a
  correction: the draft glossed the whole seven-wide run as three verbs and their
  unit forms, which is six things, and its total of 27 came out right only because
  the range is seven wide. `1 22 4 9` is the number a reader building from the
  sentence would never build.)*
- **`satellite.library.main` `1 14 1 (0)` and `satellite.library.main.arguments`
  `1 14 1 1 (0)` with its seven** — `.machine` `1 14 1 1 1 (0)`, `.machine.cores`
  `1 14 1 1 1 1`, `.machine.cpu` `1 14 1 1 1 2`, `.machine.threads`
  `1 14 1 1 1 3`, `.memory` `1 14 1 1 2 (0)`, `.memory.total` `1 14 1 1 2 1`,
  `.username` `1 14 1 1 3`. **`memory` is a sibling of `machine`, not a child**, and
  §8.1 discusses `satellite.library.main`'s numbering at M2 while nothing built it.
- **`satellite.container.arguments` `1 4 3`**, the type name an error message uses
  so it does not send the reader to the list's method table.

**Three of `satellite.system`'s thirty are not this milestone's**, and all three are
named here so the subtraction is visible. `satellite.system.threshold()` `1 22 5` and
`(n)` `1 22 6` are **M16's**: they are spelled under `system` because that is where
a knob belongs beside `max_depth`, but they set how loose a search may be over the
ten-level ladder, and v1 says it in its own comment — *"it is not a system FACT:
uname and getpwuid answer what the machine is, and this sets how the search
behaves."* `satellite.system.delete` `1 22 1` is **M19's**, because its second
argument shape is an open `satellite.variable.file` handle and M19 is the only
milestone that can hand it one.

**M6 built the readers; this milestone builds the language over them.** That is the
seam the 2026-08-27 draft was half-arguing for and got the wrong way round:
`memory_facts.cpp`, `host_facts.cpp` and `stack_facts.cpp` are consumed at M8, M9
and M10, so a milestone here cannot introduce them. **What was never inside
`system_facts/` at all is the language surface**, and it is most of the work:
`modules_system.cpp` (321 lines) is the actual `satellite.system.*` dispatch, the
unit table and every error message — over §3's target, so it splits by subject;
`methods_containers.cpp:118–199`, `subscripts.cpp`, `helpers.cpp:174–190`,
`value_arguments.hpp` and `value_printer.hpp:114–135` are the arguments object.
**`system_facts/system.cpp` (263 lines) splits across two milestones**: its
`arguments_for()` — the assembler nothing else has — comes here with
`arguments_facts.cpp` (113), and its `library_path()` and the
`-DSATELLITE_LIB_DIR` / `VERSION_DEFS` build coupling go to **M25**, which is where
`satellite.include` of another file lands. **`helpers_limits.cpp` is not this
milestone's at all**: its 121 lines are entirely `max_depth` and `division_digits`,
which are M9's and M8's dials, and `min_free_mb` — the one dial that was ever in
question — is eight lines inside M6's watchdog loop and needs no file.

**`satellite.system.environment` `1 22 2` is the one path here that is not a port.**
`SCRATCH.md/WORD_SURFACE.md` sources it to `v1docs` alone — not the registry, not
the evaluator, not the programs, which every other `satellite.system` row carries —
and v1's own comment says *"environment in general is
`satellite.system.environment(name)`'s job"*, which is to say it never built it. It
takes an argument and §2.2 has no `(name)` shape for it. So it is **new work with a
decision of its own** — whether an environment answer is the whole block, one named
variable, or a map — and it is in the open list rather than in the port.

**It needs no float, and that is worth saying because everything around it is
waiting on one.** Every unit is `b`/`kb`/`mb`/`gb`/`tb`, default `mb`, every divisor
a power of 1024, and a decimal division by 2ⁿ terminates exactly. M15's undecided
rounding rule does not reach this milestone.

**M2 owns the name and this milestone owns the behaviour.** All thirty-seven are in
`words.def` and come out of `satl --words` long before any of them answers anything,
so a sweep that reads the dump as coverage reads `1 22 4 3` as done. Saying it in
both places is what stops the next audit making that mistake — as is the fact that
`satellite.library` `1 14 (0)` parses at M4 as one of DESIGN §6.1's eleven segment-1
words, so `satellite.library.system.min_free_mb = 8192` **parses three milestones
before it means anything**, and M6 is where it starts meaning something.

**`Value` gains one alternative here and M9 has been told.** v1's `ArgsRef` is a
variant arm appended after the fact, and appending it and `ResultRef` both left
`sizeof(Value)` at 40 with the static_assert holding. DESIGN §8.2 budgets that 40
bytes and §6.1 calls `sizeof` *"the one number that could make this port not fit"*,
so **M9's entry now records that the variant is append-only and that these two are
the known future appends**; this milestone appends one and re-runs M9's assert.
v1's other consequence does not follow: its `help_for(const Value &)` switch on a
raw variant index is **deleted rather than extended** at M18, because a value's
type is a node and its methods are that node's children.

**Open, and none of these is small.** *(Six of the nine below were settled by the
author on 2026-09-09, in one sitting, and are struck through where they sit rather
than deleted — a decision is worth more beside the argument that produced it than
in a commit message. What is left open is the ten selectors, and the two SMBIOS
rows that are a done-when clause rather than a question.)*

- ~~**The 33.**~~ **SETTLED 2026-09-09 BY THE AUTHOR: THEY ARE NUMBERED, AND THEY
  ARE NESTED.** DESIGN §7.7 asked for each of v1's flat entries to be placed under
  a parent or dropped, and offered v1's own third road — **no registry ids at all**,
  the bare selector lowering to `.get(name)`, on the grounds that *"spending a
  permanent registry id on `kernel_release` would be the registry recording a fact
  about somebody's machine."*

  **v1 decided that with no `satellite.help` in existence, and M18 is why it does
  not hold.** 264 entries are generated into the binary from the file that writes
  the document; a name with no number is one `satl --words` cannot print and
  `satellite.help` cannot answer for. That is a second-class word in a language
  whose tie-breaker is *do absolutely everything for the user*, and the cost of
  the other road is nothing but rows — **a number is a position among its parent's
  children, so appending renumbers nothing** (§8.1, and the whole of why the author
  could say "just take the next available number").

  **The shape, and these spellings are permanent.** Two of v1's 33 are already
  numbered — `username` `1 14 1 1 3` and `thread_count`, which is
  `machine.threads` `1 14 1 1 1 3`. The other 31 nest under `arguments`
  `1 14 1 1`, whose children today are `machine` 1, `memory` 2, `username` 3:

      machine     (append)  architecture  byte_order  page_size  pointer_bits
      system      1 14 1 1 4   name  kernel  kernel_version  distribution
                               distribution_id  distribution_version  hostname
      build       1 14 1 1 5   compiler  compiler_version  standard  flags
                               make  standard_library  c_library  built
      interpreter 1 14 1 1 6   bare = the path; version  library_path
                               library_path_source
      process     1 14 1 1 7   id  parent
      session     1 14 1 1 8   shell  terminal  language  home  directory
      count       1 14 1 1 9

  **`machine.cpu` `1 14 1 1 1 2` is new work and not a port**, and it is the one
  row here with no reader in either tree: nothing in v1 or in `src/` reads
  `/proc/cpuinfo`'s model name, and v1's `architecture` is `uname.machine`
  (`x86_64`), which is a different fact and keeps its own row above.
- **A TOOL BEFORE THE ROWS, AND IT IS THIS MILESTONE'S FIRST COMMIT.** `satl
  --words <path>` answers number, path and depth, and says nothing about a node's
  children or what is free under it — while `next_free(NodeId)` has been sitting
  in `satellite_words/words_runtime.hpp` since M2 with ~~**no caller anywhere in
  the tree**~~ **no caller a PERSON can reach**. Minting forty rows by opening
  `words.def` and counting is the one way this milestone can silently renumber
  something; asking the binary cannot be wrong about the binary. So `--words
  <path>` grows the children and the next free number first, which is M2's own
  *a registry gets a consumer* rule applied once more, and the rest of this entry
  is then mechanical: ask, append, run `words_test`.

  **BUILT 2026-09-09, AND THE SENTENCE ABOVE WAS OVERSTATED.**
  `tests/words_test/runtime.cpp` calls `next_free()` five times and has since
  M2, which that milestone's own entry wrote down as deliberate — *"until then
  the only consumer is tests/words_test"*. A test is a caller and a real one;
  what there was none of is a caller reachable from a command line, and that is
  the thing M2's rule is actually asking for. **The correction matters because
  the overstatement pointed at the wrong risk**: an uncalled function might be
  wrong, and this one was covered — what was missing was a way for a person
  minting a row to ask it.

  **AND BUILDING IT FOUND TWO THINGS THE ENTRY DID NOT ANTICIPATE.** First,
  `next_free()` is `frozen_children() + 1` only for a numbering **that has
  defined nothing** — a sweep asserting it over the `Words` `runtime.cpp` had
  already been allocating into failed on `satellite.library`, whose counter that
  same function moves twice forty lines earlier. That is the property rather
  than a mistake in stating it, and it is why the command builds a fresh `Words`
  per call rather than keeping one. The sweep now runs over all 254 nodes on a
  fresh numbering, and it is what proves the printed number is the allocator's
  answer and not file order read twice. Second, **`next_free()` cannot report
  position 0**: it counts numbered children, and §1.3's bare shape is the one
  child that takes no position from its siblings — so a parent with no `()` row
  has a free number the allocator will never offer. This milestone needs that
  answer **five times** (`system`, `build`, `interpreter`, `process`, `session`
  are all new parents under `arguments`, and every parent in `words.def` carries
  a `()` row), so the command says whether 0 is taken as well as what is next.

  **THE THREE ASSIGNED CALL SHAPES BELOW WERE THEN CONFIRMED BY THE BINARY**
  rather than by counting the file: next free under `swap` is `1 22 4 4 7`,
  under `this` is `1 22 4 5 6`, under `system` is `1 22 9`, and under
  `arguments` is `1 14 1 1 4` — which is where `system` goes.
- ~~**Three call shapes v1 accepts have no number.**~~ **ASSIGNED 2026-09-09 BY
  THE AUTHOR, AND ONE OF THEM WAS BLOCKING THE DONE-WHEN.** The unit block is
  reached for every swap and `this` form, so v1 answers `.swap.used(unit)` and
  `.this.used(unit)` while §2.2 writes `1 22 4 4 3` and `1 22 4 5 3` without the
  parens their siblings carry — the table recording a sweep's arity rather than
  the code. **This was not hypothetical: the done-when below cites
  `example/full_test.satl:605–613`, whose last line is
  `satellite.system.memory.this.used("kb")`, so M20 could not have met its own
  done-when without minting at least one number.**

      satellite.system.memory.swap.used(unit)   1 22 4 4 7
      satellite.system.memory.this.used(unit)   1 22 4 5 6
      satellite.system.environment(name)        1 22 9

  **All three are appends and nothing renumbers.** The first two take the next
  slot under `swap` and `this`. **`environment(name)` is a SIBLING and not a
  child**, which is §4's rule and not a preference: a *user-owned* argument
  contributes no number of its own, so its call shape takes a slot beside its
  siblings — and `memory.main()` `1 22 4 3` against `main(unit)` `1 22 4 9`,
  six slots apart, is the same rule already in the table.

  **WORD_NUMBERS.md §2.2 is still the authority and does not yet carry these
  rows.** Writing them there before `words.def` has them would fail
  `tests/words_test`, which walks §2.2 against the registry — so the three rows
  and the forty above land in **one commit, in both files**, as M20's first act.

  > **LANDED 2026-09-11, AND IT WAS FOUR FILES RATHER THAN TWO.** `words.def`
  > and §2.2 moved together as this paragraph requires, and the build refused
  > until two more did: `help_lines/nodes.tsv` and the written entries, because
  > `help_text.hpp` static-asserts that `help.def` carries **exactly one row per
  > node of words.def, in words.def order**. So a registry row is not a registry
  > row until it has a help line — M18's machinery holding a later milestone to
  > its own rule, from a direction this entry did not see coming. **43 nodes
  > from 38 rows** (five of the new parents carry a `(0)`), `kNodeCount` 282 →
  > 325, and the digest moved `ba12d6fa81d0fe61` → `1ef8ec50a203c593`, which
  > invalidates every cached `.satc` on the machine exactly as SATC.md §3 says
  > it should.
  >
  > **AND THE OBJECT ITSELF LANDED 2026-09-11, THE COMMIT AFTER.** `Arg` is the
  > NINTH arm of the variant, sixteen bytes of shared const handle against a
  > 32-byte widest arm — **`sizeof(Value)` re-measured at 40**, which is the
  > check the M9 entry asked the milestone that landed this append to re-run.
  > The compiler's arm for the object is DELETED rather than written: resolve
  > has walked `argz.machine.threads` to `1 14 1 1 1 3` since M7, and a language
  > path read without being called is already a module constant, so the S0720
  > that used to name M20 simply came out and the rows were installed instead.
  > `display(arguments)` prints all thirty-six lines (§7.7), `arguments[i]`
  > reads the command line and nothing else, and `argz.machine.threads` answers
  > 24 beside `\threads`.
- ~~**`satellite.container.arguments` `1 4 3` has no children and the object
  answers ten selectors.**~~ **BUILT 2026-09-11, AND THE COLLISION IT MADE IS
  BELOW.** The ten are `1 4 3 1`-`1 4 3 10`, in the order this entry lists
  them, with **no `()` row**: `list()` and `map()` are constructors and there
  is exactly one arguments object per run which the language hands over, so a
  program has nothing to construct. They are reached through the RECEIVER'S
  TYPE and `satellite.main`'s parameter is declared
  `satellite.container.list<satellite.variable.string>` -- v1's compatibility
  spelling, which DESIGN §7.7 keeps so hello world stays five lines -- so
  names.cpp gives the bound name the object's own type and the fold reaches
  `1 4 3` from there. The four command-line rows never touch `entries_of()`,
  which is the laziness above still holding.

  > **AND `count` WAS SPELLED TWICE UNDER ONE RECEIVER, WHICH NEITHER DECISION
  > COULD SEE ON ITS OWN.** `satellite.library.main.arguments.count`
  > `1 14 1 1 9` is v1's `argument_count` and `1 4 3 2` is how many entries the
  > object holds -- 3 against 37 here -- and BOTH are reached off the
  > parameter, the object's own children first. So `argz.count()` answered 3
  > while the help line this milestone wrote promised 37: **answering the wrong
  > fact rather than failing**, which is the shape the resolver defect below
  > had and the reason it was worth a commit.
  >
  > **SETTLED 2026-09-11 BY THE AUTHOR: `length` is the command line, `count`
  > is the entries, and `1 14 1 1 9` was RESPELLED rather than renumbered.**
  > Nothing moved but a string. `1 4 3 1` is `length` too and answers the same
  > number from the same vector, so what is left is two rows agreeing rather
  > than one hiding the other -- which is the whole difference between this and
  > what it replaced. The paragraph below was written before the collision and
  > is left as it was argued; its last sentence is the one that did not survive
  > contact.

  **SETTLED 2026-09-11 BY THE AUTHOR, AND ONE WORD
  CHANGED.** The ten are `.length()`, `.count()`, **`.keys()`**, `.to_string()`,
  `.lines()`, `.has(k)`, `.get(k)`, `.first()`, `.last()`, `.contains(x)` — this
  entry had written `.names()` and the author chose `keys`, which is the word
  `satellite.container.map` `1 4 1 8` already uses for the same idea. That is M16's
  thirty-four container methods again, one level down, in a namespace nobody has
  looked at.

  **`length` AND `count` ARE NEW WORDS AND THE AUTHOR KEPT THEM KNOWING SO.** The
  question was put on 2026-09-11 with the finding that made it one: `length`
  appears **nowhere** else in the language, and `size` is what
  `satellite.container.map`, `satellite.container.list` and
  `satellite.variable.string` all answer to — so these two arrive beside `size`
  rather than instead of it, which DESIGN §1's *a word means one thing
  everywhere* would otherwise argue against. **The reading that makes them one
  thing each is that this object is TWO containers at once**: `length()` is how
  long the command line is and `count()` is how many named facts there are, and
  a single `size` could only ever answer one of those while looking like it
  answered both. `keys` then had no such defence — the map's word already fits —
  and that is the one the author moved.

  **`count()` AND THE `count` ROW ARE DIFFERENT THINGS AND THE NAME IS REUSED ON
  PURPOSE.** `satellite.library.main.arguments.count` `1 14 1 1 9` is v1's
  `argument_count`: how many words were on the command line. `.count()` here is
  how many entries the object holds. Both are counts of what their own receiver
  holds, which is the same word meaning the same thing about two different
  things — the distinction §2.3 draws between a word and the node it hangs
  under.
- **DESIGN §7.7's live-code mapping is wrong and this milestone is where it is
  fixed.** §7.7 reads *"`arguments.machine.threads`, `arguments.machine.cores` and
  `arguments.memory.total` … the same numbers again as codes 97, 98 and 99"*, which
  positionally makes 98 `cores`. v1's header and PLAN §6.1 both give **97 threads,
  98 mem_total_mb, 99 mem_used_mb**, and there is no live code for cores at all — so
  §7.7's *"three surfaces, one set of facts"* is **two** surfaces for
  `arguments.machine.cores`. Either a code is assigned or the rule is restated, and
  this milestone must not quietly pick a side of a contradiction whose whole point
  is that these must not disagree.

  > **SETTLED 2026-09-09: §7.7's SENTENCE IS RESTATED AND NO CODE IS MINTED.**
  > `cores` has **two** surfaces — the configuration and the arguments object —
  > and the live code table keeps 97 threads, 98 mem_total_mb, 99 mem_used_mb as
  > the code and PLAN §6.1 already have them. Minting code 100 for `cores` would
  > add a fourth live code to a table whose rows are a permanent wire format, to
  > satisfy a sentence's arithmetic; restating the sentence costs nothing and is
  > true. **The done-when keeps its assertion for `threads`**, where all three
  > surfaces really do exist, and that is the check §7.7 was asking for.
- ~~**The seventh spelling**~~ **— ANSWERED 2026-09-09, AND IT IS A SPELLING.**
  This item read *"a parameter named `argv` gets a plain list with no properties,
  silently, and DESIGN §9 says that silence is wrong"*, and M7 had already ended
  the silence with S0531. The author ended the refusal: `argv` is an alias row in
  `words.def` and resolves to `1 14 1 1` like the other six. **It cost one line
  and no code**, because the suggester and the error's own sentence both read the
  alias rows — which is the mechanism M2 built and the reason §7.7 could name
  this as the author's call rather than a milestone's. **S0531 keeps its number**
  and still fires for `argu`, `arrgs` and `argvs`. What this milestone inherits is
  narrower than it was: **seven spellings to recognise instead of six.**
- ~~**`arguments[0]`**~~ **CONFIRMED 2026-09-09: index 0 is the program name**,
  which is v1's answer kept rather than a new one — `.length()` and numeric `[i]`
  cover the command line and nothing else. DESIGN §13's open row closes with it.
- **The arguments object is a startup cost. BUILD IT LAZILY, AND MEASURE BOTH** —
  the author, 2026-09-09. v1 builds all 33 eagerly — `uname`, `/etc/os-release`,
  `getpwuid`, `gethostname`, `getcwd`, two `readlink`s, `sysconf` — a dozen
  syscalls and a file parse against satl's own share of §4.3's budget. **Measured
  2026-09-09 on this machine: satl's whole startup is 2 ms**, against CPython
  3.12's 17 on the same box, so the budget this is spent out of is real and small.
  §9 says take the number rather than quote it, and the number wanted is *both*
  arms — eager and lazy — because "lazy is faster" is the kind of claim this
  section exists to stop being assumed.

  > **BUILT LAZY 2026-09-11, AND THE LAZINESS IS PER-PROCESS RATHER THAN
  > PER-VALUE.** The thirty-two machine facts are a function-local static in
  > `system_facts/arguments_facts.cpp`, assembled on the first ask; a program
  > that never reads one pays for none of them. What is NOT deferred is the
  > command line, which is already in hand, and **`session.directory`, which is
  > `getcwd()` and had to be taken at startup** — `satellite.directory.change`
  > `1 18 1` has been built since M19, so an eager build and a lazy one would
  > answer differently for that one fact after a program changed directory. The
  > object is what the program WOKE UP to (§7.7), so the starting directory is
  > captured with the words and the other thirty-two are not. **That is the one
  > thing the measurement would have hidden** — both arms would have been fast
  > and one of them would have been wrong.
  >
  > **BOTH ARMS, MEASURED 2026-09-11 ON THIS MACHINE.** Best of five runs of
  > 200 invocations each, load 2.06, the same shape §9's other figures are
  > taken in. The eager arm is this tree with one line added to
  > `run_command.cpp` calling `machine_answers()` before the program runs, and
  > it was removed again after the numbers were taken:
  >
  >           program                  lazy      eager
  >           reads no fact          1.156 ms   1.638 ms
  >           reads one fact         1.644 ms   1.647 ms
  >
  > **Lazy costs nothing and saves 0.48 ms**, which is 42% of the startup of a
  > program that never asks — so the claim §9 exists to stop being assumed is
  > true here, by about the margin that would have been guessed. What it does
  > NOT do is make the first ask cheaper, and the two numbers in the right-hand
  > column are the same number: a program that reads one fact pays for all
  > thirty-two either way.
  >
  > **AND THE 0.48 ms IS TWO READERS OUT OF THIRTY-TWO**, measured separately
  > in a harness linked against the tree's own objects: `physical_cores()` is
  > **303 µs** (it walks `/sys` topology, once per CPU) and reading
  > `/proc/cpuinfo` for `machine.cpu` is **305 µs** (the kernel generates a
  > block per thread, and this machine has 24). Against those,
  > `hardware_threads()` is 6 µs, `/proc/meminfo` is 35 and `/etc/os-release`
  > is 18. **So a per-SOURCE laziness would make `arguments.machine.threads`
  > nearly free** while `cores` and `cpu` stayed expensive, and it is a named
  > and quantified follow-up rather than something this milestone took in
  > passing — the author's call, with the number attached.
- **`satellite.system.environment` `1 22 2` — the shape, settled 2026-09-09.**
  The bare form answers **a map** of the whole environment; `environment(name)`
  `1 22 9` answers **the one variable**. M16 has maps, so the bare form costs
  nothing it did not already have, and the pair is the same read-one / read-all
  shape `satellite.system.memory` carries. It remains the one path in this
  milestone that is not a port — v1 never built it and said so.
- **WHAT A BARE GROUP WORD UNDER `arguments` ANSWERS — SETTLED 2026-09-11 BY
  THE AUTHOR: EVERY GROUP DISPLAYS ITS CHILDREN.** `arguments.machine` answers
  all seven of its facts as a map, `arguments.session` all five, and so on for
  `memory`, `system`, `build` and `process`. It was put to them because the
  question could not be ducked and had two defensible answers: **DESIGN §7.7
  wrote `arguments.memory` as FREE memory and `arguments.machine` as what the
  processor is**, while the help lines minted the day before had written the
  same words as groupings. The resolver is what made it a fork rather than a
  wording problem — `arguments.machine` and `arguments.machine()` reach the
  same node, so the `(0)` row cannot carry a second meaning and one of the two
  readings had to go.

  **THE COST WAS NAMED BEFORE IT WAS TAKEN AND IT IS REAL**: free memory was
  `arguments.memory`'s only spelling and is now reachable only as
  `satellite.system.memory.free()`, which this same milestone built. DESIGN
  §7.7's two lines are corrected there rather than here.

  **`interpreter` IS NOT ONE OF THE SIX**, because this entry's own shape table
  settled it on 2026-09-09 — *"bare = the path"* — so it stays the one group
  word in the object that answers a fact.
- **`library_path` `1 14 1 1 6 2` AND `library_path_source` `1 14 1 1 6 3` DO
  NOT ANSWER AT M20, AND THIS ENTRY ALREADY SAID SO WITHOUT NOTICING.** The
  split above sends v1's `library_path()` and its `-DSATELLITE_LIB_DIR` build
  coupling to **M25**, "which is where `satellite.include` of another file
  lands" — and `arguments_for()` CALLS it, which is the collision the sentence
  did not see. This tree's Makefile has no install prefix at all, so an answer
  here would be a fact about a mechanism that does not exist. **Both rows
  refuse with the reason and name M25**, which is what `retune_min_free_mb`
  does one module over. Thirty-four of the thirty-six answer.
- **FOUR BARE GROUP SHAPES UNDER `satellite.system` ANSWERED NOTHING, AND TWO
  OF THEM DO NOW.** `satellite.system()` `1 22 0`, `memory()` `1 22 4 0`,
  `memory.swap()` `1 22 4 4 0` and `memory.this()` `1 22 4 5 0` all refused
  with S0721 — numbered at the 2026-08-28 transcription, and this milestone's
  third commit built everything under them without reaching them.

  **SETTLED 2026-09-11 BY THE AUTHOR FOR TWO OF THE FOUR.** `memory()`
  **displays its children**, which is the answer they gave for `arguments`'
  groups the same day; `swap()` answers **how much swap is available**.

  **THE OBJECTION THIS ENTRY RAISED WAS ANSWERED RATHER THAN OVERRULED.** It
  said "display its children" could not transfer, because `memory`'s children
  are `free()`/`free(unit)` pairs and a map of them would have to pick a unit.
  It picks the **arity-0 row**: `free()` and `free(unit)` are two numbers and
  one WORD, so the map is keyed by the word and answered by the shape that
  takes nothing, which is the default unit by construction and not by a choice
  made here. A program that wants gigabytes asks `memory.total("gb")`.

  **A CHILD WITH NO NO-ARGUMENT ANSWER IS LEFT OUT** rather than filled with
  `nothing` — today that is `this` `1 22 4 5`. Leaving it out says so; a blank
  beside seven numbers would read as a fact about this thread's stack.
  `satellite.system()` `1 22 0` and `this()` `1 22 4 5 0` are **still open**,
  and `satellite_system/group_map.cpp` is the reader either of them would use.
- **AND ASKING THE QUESTION FOUND A RESOLVER DEFECT THAT WAS ANSWERING THE
  WRONG FACT RATHER THAN FAILING.** *(2026-09-11.)*
  `satellite.system.memory.swap("mb")` never reached `swap(unit)` `1 22 4 4 6`
  at all: it resolved to `swap.free(unit)` `1 22 4 4 4`, the first child of
  `swap` with matching arity, so that row's handler was dead code and the
  install table said `total` while the language answered `free`.

  **WORSE ONE LEVEL UP, AND SILENTLY.** `satellite.system.memory("mb")` **is
  not a row at all** and matched `memory.main(unit)` `1 22 4 9` — so a program
  asking how much memory the machine has was told this process's resident set,
  **3.39 against 63430**, with nothing anywhere saying so.

  **THE CAUSE IS IN `satellite_cache/paths.cpp`'s `shape_of()`.** When a word
  has a number of its own, its call shapes are its children — and the loop
  that looks there matched on ARITY ALONE and never on the spelling. A named
  child is a different word and can never be what `<word>(...)` meant. **Six
  rows in the registry are spelled as a bare argument list** —
  `include(satellite)`, `include(spaceship)`, `return(satellite)`,
  `return(value)`, `help(x)` and `swap(unit)` — and only the last sat beside a
  named sibling of its own arity, which is why four milestones walked past it.

  **NO `.satc` NEEDED INVALIDATING AND THAT WAS CHECKED RATHER THAN ASSUMED.**
  The numbering did not move, so the words digest did not either — which means
  a cache written before the fix would have kept the wrong number. All 39 files
  under `$HOME/.satl/cache` were read: two match the current digest
  `1ef8ec50a203c593` and neither holds `1 22 4 9` or `1 22 4 4 4` from a call
  of this shape. **SATC.md §2's three header lines have no reading for "the
  resolver's rules changed while the numbering did not"**, and that gap is
  worth a sentence there whenever the author next touches it; the format
  version is the only lever, and it is an error rather than a silent walk.

  **A DEAD READER WENT WITH IT.** `stack_total()` in
  `satellite_system/memory_methods.cpp` was written on 2026-09-11 beside
  `stack_used` and `stack_free` and installed nowhere, because
  `satellite.system.memory.this` has no `total` row — its children are
  `available`, `free` and `used` and their unit forms. It was a
  `-Wunused-function` warning in a tree built `-Wall -Wextra`, which is how it
  was found, and it is deleted.
- **A BUG IN THE COMPILER, FOUND BY WRITING THE HELP LINES RATHER THAN BY ANY
  SUITE.** `arguments.machine.threads()` answered 24 while `arguments.count()`
  was refused with S0722 — *"takes 0 arguments and was given 1"* — and the two
  depths disagreeing is what gave it away. `method_receiver()` in
  `evaluator/compile_expressions.cpp` reads a call whose receiver sits in a
  frame slot as DESIGN §6.4's method sugar, so a fact one hop under the bound
  name arrived with the object as argument 0; two hops down the receiver is a
  member, names no slot, and the module road was taken instead.

  **The object's facts are facts about the PROCESS and not methods on it**, so
  the fix is one clause: an arguments member is never a method receiver. **The
  ten selectors are the opposite case and the clause must not catch them** —
  `arguments.length()` folds through `satellite.container.arguments` `1 4 3`,
  the receiver's type, and needs the object as argument 0. The milestone that
  builds them has to leave `arguments` false on a selector in names.cpp's
  `arguments_member()`, and the comment at the clause says so.
- **`.bit` `1 22 4 1` and `.frequency` `1 22 4 2` cannot be demonstrated as an
  ordinary user.** Both come from SMBIOS type 17 through
  `/sys/firmware/dmi/entries/*/raw`, and v1's comment on the failure path reads
  *"root-only, which is the usual answer"*, so both answer 0. **0 is a truthful
  answer and not an error**, and the done-when says so rather than letting two paths
  ship answering 0 with nothing that fails.

**The recognition of the spellings is M7's and the spelling table is M2's**, and
neither said so before this pass. WORD_NUMBERS §2.3's third alias row is
`arg` `args` `argz` `argument` `arguments` `argumentz` `argv` — *one node, seven
spellings since 2026-09-09* — which is the same mechanism `words.def` already
carries ten of; **resolve is where a parameter name matching one of them becomes
the special variable**, and M7's entry now says it. This milestone's headline demonstration rests on that table, and the
draft it comes from asserted the whole mechanism was M7's while M3's own sentence
called `hexadecimal` *"the language's one alias"*. **M3's word was wrong and is
corrected**: §2.3 has three rows.

**`SAT_PATH`'s four-segment limit does not come across.** v1's
`SAT_PATH(ident, s1, s2, s3, s4, arity)` could not hold
`satellite.system.memory.swap.used` `1 22 4 4 3`, which is five. **WORD_NUMBERS §1.3
is what refuses a segment limit** — §1.4's argument is about integer width, that one
`uint32_t` carries the whole path — and §4 credits the arguments subtree, six
numbers deep, as the clearest case for the refusal. That is a fix rather than a
port.

**Done when** one program prints `arguments.memory.total`,
`satellite.system.memory.total("mb")` and `satellite_string`'s live code 98 and
**asserts all three are the same number** — DESIGN §7.7's *"they must not be allowed
to disagree"* turned into something that can fail — and the same for threads across
`arguments.machine.threads` and code 97; when **`argz.machine.threads` answers 24 on
this machine**, which is the one line that proves §2.3's one-node-six-spellings
rather than five words that happen to be registered; when
`satellite.console.display(arguments)` prints all of it (§7.7); when `.bit` and
`.frequency` answer 0 as an ordinary user **and the program says that is the
truthful answer**; and when `satellite.system.home` and the memory verbs match
`example/full_test.satl:605–613`, which ports almost verbatim and is the regression
floor — five assertions over `.home`, `.total`, `.free`, `.main` and
`.this.used("kb")`, and that last line is also the proof that the missing `(unit)`
numbers are a real problem and not a hypothetical one.

***"ALMOST VERBATIM" HAS A SECOND CLAUSE AND IT IS A SPELLING, NOT A NUMBER.***
*(Found 2026-09-09 by walking the cited lines through the tool this milestone's
first commit built.)* Three of those five are written with parentheses in v1 and
without them here: `format.def` has `satellite.system.home()`,
`.memory.bit()` and `.memory.frequency()` at arity 0, while `words.def` numbers
all three as plain words — `home` `1 22 3`, `bit` `1 22 4 1`, `frequency`
`1 22 4 2` — with no call shape under any of them, so `satl --words
'satellite.system.home()'` answers *"has that word, but not with those
arguments"* and exits 2. **All three moved the same way, so it is a decision this
tree already made and not three rows anybody forgot**, and the port adjusts the
spelling rather than minting a number. Saying so here is what stops the person
writing the demonstration reading a refusal as a missing row and appending a
fourth `()`. **`.this.used("kb")` is the one line of the five that really is a
missing number**, which is exactly what the entry above says.

**The watchdog is not in this done-when**, and that is the M6 seam holding: the
ceiling, the file and the exit path are demonstrated at M6 against no language at
all, and what is left here is `min_free_mb` being **readable and retunable from a
running program** through `satellite.library.system` — which is §4.5.3's *"the file
is where a machine's settings live before a program starts; the namespace is how a
running program reads and changes them"*, and the first time any milestone can show
both halves.

> **BUILT 2026-09-11, AND THE DESIGN THE REFUSAL WAS WAITING FOR IS ONE WORD.**
> The row refused with a true reason -- the watchdog is a detached thread and a
> torn read there kills a healthy process -- and what it needed was a single
> atomic `unsigned long long`, written whole by the evaluator and read once per
> wake-up. **Two words, a flag and a value, would have brought the problem back
> in a smaller form**, since a reader can take the flag from one write and the
> value from another; so "not watched" is carried in the same word and the
> stored value is the floor PLUS ONE.
>
> **`0` IS A FLOOR OF ZERO AND `"disabled"` IS THE OFF SWITCH**, settled by the
> author on 2026-09-11 in those words. A floor of zero is armed and never
> crossed -- no machine has less than 0 MB free -- and reading it as "stop
> watching" would be the language guessing at a number's meaning. It is QUOTED
> because a bare `disabled` is a variable name in this language; the four file
> modes at S1201 are the same shape and the same decision, and S1302 is the
> refusal that enumerates rather than guesses.
>
> **DEMONSTRATED BY BEING STOPPED.** A program that sets `min_free_mb` above
> what the machine has is killed within the second, on stderr, exit 4 -- the
> watchdog acting on a floor no file ever carried.

**M21 — a piece of QUAD, running.** ***LANDED 2026-09-11 —
`MILESTONES/M21.md`, and its §2.3 is how the blocker below was cleared.***
*(New 2026-08-28. After M20.)* **The milestone
QUAD.md §4 has asked for since its first draft and this list did not have**, in §4's
own words: *"one whose done-when condition is a piece of QUAD, running. Not the whole
program — one mechanism out of `mind.hpp`, chosen because it exercises floats,
containers, sorting and persistence at once."* QUAD.md §5, `SCRATCH.md/MILESTONE.md`
§4 and SESSION.md §5.7 independently name the same candidate: **`Sky::decay` plus
`Rack::draw`.**

**It adds no numbered path, and it is not an acceptance test bolted onto the list.**
Every milestone before it demonstrates a mechanism the language provides. This one
demonstrates that a program somebody else wrote in another language can be written
in this one, which is the only claim that cannot be made by building any single
mechanism correctly — and QUAD.md §5 says why it has to be a separate milestone
rather than a clause in M16: *"the gap between 'the language has floats' and 'this
expression is writable' is where languages actually fail."*

**The two halves are not the same size and the milestone says so.**

- **`Sky::decay` uses no randomness at all.** `sky.hpp`'s decay is float arithmetic
  over a live list, and `quad_core.hpp`'s persistence term is `0.15 + 0.85·alt²` with
  no fractional `pow`. **It is M15 plus M16 and nothing else**, and it is
  demonstrable the day both have landed.
- **`Rack::draw` is a roulette wheel over `std::pow` weights and cannot be written
  today.** M13 records the three reasons in full: `satellite.random` has **no float
  draw** in any of its thirteen numbers, **no seed** — so QUAD's determinism
  invariant is not expressible — and a cheapest tier that **throws draws away for
  50–100 ms** against a 90 ms tick. `1 7 13` is free and M13 declines to assign it,
  because minting a number is the numbering's.

**So this milestone carries one blocker, and it is a number rather than a
decision.** A fourth shape under `satellite.random` that **takes a seed and does not
spin**, and a draw that can answer a fraction. Until it exists, `Rack::draw` is
writable only by drawing an integer and dividing, which is a `satellite.variable.float`
built out of two `satellite_number`s and is exactly the thing DESIGN §11's tiers
refuse to pretend to do. **Done when the number is assigned and the shape is built,
or when the refusal is written down beside the mechanism it blocks** — the same form
M15 uses for the rounding rule.

***CLEARED BY THE AUTHOR 2026-09-11: a fourth TIER word, and the number is
assigned rather than refused.*** `satellite.random.seeded` `1 7 13`–`1 7 16` —
`1 7 13` being the number this paragraph and M13's entry both reserved for it.
Three shapes and not four, **and the registry chose that rather than a
preference**: a call shape is keyed by its word and its ARITY, there are no
dotted node names in `words.def` at all, and so the one-argument slot holds
either a digit count or a seed. The seed is what the tier is for, so
`seeded(digits)` is not offered — which costs QUAD nothing. The fraction is
uniform over a **grid** of `float_digits` decimals, which is the answer to this
paragraph's own objection: S0905 refuses fractional bounds because there is no
uniform draw over the REALS, and a grid is countable. DESIGN §11.0 is the
specification and M21.md §2.3 is the argument.

**AND IT TURNED OUT TO CARRY A SECOND BLOCKER THAT NOBODY HAD WRITTEN DOWN**,
found by trying to write the `Sky::save` half this entry assigns below:
**`satellite.variable.float` answered no methods at all** — `1 6 10` was a leaf
in the numbering from M2 until M21 — so a float could be computed and displayed
and could not be turned into text. `Sky::save` writes 164 doubles into a text
file and `write_line` refuses a float; `Sky::load` already worked. **Cleared the
same sitting by the author: `satellite.variable.float.to_string()` `1 6 10 1`**,
the type's first method. DESIGN §8.6 carries it, and M21.md §2.4 is why a
round-trip available in one direction only is what made it a number.

**Its floor is M15, M16, M13 and M19**, one for each thing QUAD.md §4 names: floats,
containers and sorting, the dice, and persistence. **`Sky::save` / `Sky::load` is the
fuller persistence half** and round-trips 164 doubles at six significant digits, which
is DESIGN §13's *"the right half's length IS the precision"* under load; it belongs
here rather than at M19, because M19's corpus reader deliberately touches no float.

**Done when** `Sky::decay` and `Rack::draw`, written in satellite by hand against
DESIGN.md, produce the same numbers as the C++ QUAD on the same input — and when the
day of writing them has produced its own list of what the language still cannot say.
**QUAD.md §5 predicts that list exists and this milestone is what finds it**: *"§3 is
a floor on what is missing and not a ceiling."* A milestone whose output includes new
holes is not a failed milestone; it is the only one in this section positioned to
find them before a user does.

***AND IT IS DONE, WITH ONE CLAUSE THAT HAD TO BE READ MORE CAREFULLY THAN IT IS
WRITTEN.*** *(2026-09-11.)* `example/decay.satl` and `example/draw.satl` are the
two programs and both assert rather than print. **`Sky::decay` matched an
independent exact-decimal referee on 40 of 40 activations and the C++ double
matched it on 6 of 40**, worst double error 6e-17 — so "the same numbers" is
true in the strongest available sense, and the two places the two disagree in
the sixth digit are exact ties where satellite holds the value and the double
does not.

**"The same numbers" cannot mean what it says for `Rack::draw`, and the program
says so instead of pretending.** A mechanism with a generator in it can only be
compared where the generators agree, and QUAD's is `std::mt19937` against
satellite's PCG. So the claim splits: the **deterministic core is compared** —
all four annealing exponents, exact in satellite and not in the double (`4.65`
against `4.6500000000000004`), and all twenty wheel choices identical — while
the **stream is demonstrated**, by seeding, drawing, reseeding and asserting the
replay, which is the property QUAD's invariant 8 actually asks for. The sequence
itself is deliberately not asserted, because `satellite_random/random.hpp` keeps
the 32-bit seam replaceable and a frozen stream would undo that.

**The list of what the language cannot say has six entries and QUAD.md §5 now
carries it.** Two were closed by this milestone's own two mints. The other four
are open: a container is passed by value so no capsule can mutate its caller's
list (**and M26 inherits that question for a spacesuit method**); there is no
`break` and no `continue`; spacesuits are M26 so the sky is parallel lists; and
a float's precision is decimal PLACES rather than significant digits, which is
the one place in either mechanism where the double is the more precise of the
two. M21.md §3 has each with its measurement.


**M22 — the prompt, and the window stops closing.** ***LANDED 2026-09-07, OUT OF
ORDER — `MILESTONES/M22.md`, and its §0 is why.*** It was built before M18 at the
author's direction: M18's help is meant to answer about a program's variables
after it has run, which needs a prompt to be typed into, and a name-and-type
store built at M18 would have had its only reader here — the no-consumer shape
M2's entry above forbids. **The build order below is unchanged and nothing
renumbers.** The REPL itself: the prompt,
**the prompt's Ctrl-C — the byte `0x03`, because raw mode turns ISIG off and the
signal never arrives** — and the exit words. DESIGN §10.2 is why Ctrl-C is two
different things, and **the other one is M11's**: the SIGINT that sets a flag and lets
the walk stop itself at the next statement. *(The word was bare "Ctrl-C" until
2026-08-28, which read as owning both halves of a mechanism this milestone owns half
of.)*

**Two things it inherits rather than writes.** *(2026-08-28.)* `terminal_columns()`'s
ioctl with its 80-column fallback, and the clear — 14 lines that **M14 builds** for
`satellite.console.width` `1 5 6` and `clear()` `1 5 8`, and that v1's line editor
calls from two places. And **the emergency-exit hook**: M6 builds the watchdog's
exit path and deliberately registers no hook, because until there is a prompt nothing
has put the terminal into raw mode; **this milestone is the only registrar**, exactly
as v1's is, and it is what makes a `_exit(EXIT_LIMIT)` from the watchdog leave a
usable terminal behind. *(That said `_exit(2)` until 2026-08-31. The status is **4**
— `EXIT_LIMIT`, `programs/opening.hpp` — and M6's own entry above carries the
correction and the argument for it: 2 is `EXIT_USAGE` in this tree, so a script
testing for it would report a memory ceiling as a typo. The number was fixed where
M6 was described and not here, where it is also written, which is why an inherited
line is worth re-reading when the thing it inherits changes.)*

**And a third thing it inherits, which nothing said until 2026-08-31: there is no
signal handler anywhere in this tree.** `satl --watchdog` holds the process open and
its only clean way out is an interrupt, so the status a caller sees is the one the
shell synthesises from the signal — measured on this machine: **SIGINT 130, SIGTERM
143**, against 4 when the ceiling actually fires. That is *correct* and it is not
nothing: 130 is 128 + SIGINT and is exactly what a program that dies by a signal
should report, so there is no defect here to fix, only a milestone to attach it to.
This is the one, because a handler is only worth writing where something must be
undone before leaving — and raw mode, the only such thing, arrives here. MILESTONES/
M6.md §6.5 is where it was found and §9.3 is what found it.

**M1.5 closes the window when the interpreter exits cleanly, and M22 ends that.**
Until there is a prompt, the child runs for milliseconds and a window that outlived
every one of them would only ever be a window nobody asked to keep — so M1.5 closes
on a clean exit and **holds on a failure**, because a failed child is holding the
only copy of the reason and destroying the window destroys the message. That is what
makes M1.5 demonstrable before this milestone exists: `satl --repl` answers
"not built yet" and exits `EXIT_NOT_YET`, so the window stays up with the
explanation on it.

Once the prompt is there the question is the other way round. **The window does not
close.** A person who has been typing at a prompt has a screen full of what they
did, and the exit word is the end of a session rather than the end of a window; the
close button is how a window closes. `on_child_exited` in
`src/programs/terminal.cpp` is the one function that changes, and it is written
knowing this — the clean-exit arm is marked as M1.5's and this milestone removes
it rather than discovering it.

***And it NARROWED that arm rather than removing it, because this paragraph was
written before tabs existed.*** *(2026-09-07, and the file moved too:
`src/programs/satl-term/terminal.cpp`.)* `TERM.md` now says **"a tab closes when
its interpreter is finished with"**, and the File menu's `Open…` runs a file in a
tab — so deleting the arm would leave every finished program sitting in a tab the
user has to dismiss by hand. The prompt opts out of it instead: `hold_always ||
file.empty()`, and `child.cpp` already adds `--repl` in exactly that case. **The
reason given above for closing is still exactly true of a file and is no longer
true of a prompt**, which is the sentence doing the work rather than the
instruction it produced when there was only ever one child. MILESTONES/M22.md
§2.6.

Done when: a person can start `satl-term`, type at the prompt, and have what they
typed still on the screen after the interpreter is gone.

**M23 — threads.** ***LANDED 2026-09-12 — `MILESTONES/M23.md`.***
`satellite.variable.thread`. The arena makes the walk atomic-free;
the Console already keeps output lines atomic.

**AND BOTH OF THOSE HELD WITH NO CHANGE, WHICH IS THE MILESTONE'S MAIN RESULT.**
§2.2's arena and DESIGN §7.2's frames were built for this and neither needed a
line; the inline cache was already in a side table because `evaluator/closure.hpp`
put it there at M9 naming this milestone. DESIGN §7.1's receipt — *"eight threads
... produced 1585 wrong results out of 1600"* — now runs the other way, 1600 of
1600, in `example/threads.satl` §3.

**WHAT THIS ENTRY GOT WRONG IS THE POOL, AND §4.5.1's TENANT LIST IS CORRECTED
WHERE IT IS WRITTEN.** `machine_limits/pool.hpp` names *"M23's threads"* as a
tenant, on the test *does the work end*. A satellite thread's body does end, so
it passes that test — and the test is necessary and not sufficient. The half it
is missing is **and does the caller wait for it**: `run_over()` is a range, a
split and a join, and `start()` must come back immediately. Underneath that is a
worse problem, which is the one that settles it: a pool has `THREAD_COUNT`
workers, so a program starting one more thread than that, each joining the next,
would wait forever — **a ceiling on the language wearing an optimisation's
clothes**, which is what SCRATCH.md/NO_LIMITS.md exists to refuse. The author
settled it on 2026-09-12: a fresh `std::thread` per `start()`, measured at about
28 us created-run-joined. **The pool keeps parse-time interning and whatever
`parallel_for` becomes, and loses this one.**

**Three decisions the author took that no document had.** *(2026-09-12.)*
`join()` **answers what the capsule returned** — it is the one of the three verbs
that waits until there is an answer, so it is the one that can have one, and
`start()` answering a promise would be a second type and a second word.
`satellite.return(satellite)` **closes everything** — every thread nobody joined
is told to stop and then waited for, which is what keeps a detached walk off an
arena that is about to be destroyed, and which means a thread started on the last
line may never run at all. And there is **no ceiling** on how many a program may
start.

**`satellite.library` IS THE ONE THING TWO WALKS SHARE, and DESIGN §7.2 asked for
it by name four years of milestones early**: *"permanent identity, CROSS-THREAD
SHARING, lock-free reads and atomic read-modify-write are exactly what globals
need."* Lock-free reads are what it asks for and not what M23 built — a `Value` is
a 40-byte variant and no machine this tree targets reads one atomically — so
`evaluator/globals.hpp` takes a mutex, and what is lock-free is DECIDING WHETHER
TO TAKE IT. A program with no second walk pays one relaxed load: 3,000,000 global
reads and 3,000,000 local reads are indistinguishable on this machine, 1.19-1.25 s
either way.

**Done when: `example/thread_test.satl` runs** — the file that has defined this
syntax since M2 and that nothing could execute. It does, and `example/threads.satl`
is the acceptance program §7.1 asked for.

**Six numbered paths, and its own line named one of them.** *(2026-08-28.)*
`satellite.thread` `1 23 (0)` and `.new` `1 23 1` — the module face, which is what a
program actually writes — and `satellite.variable.thread` `1 6 13 (0)` with
`start()` `1 6 13 1` and `join()` `1 6 13 2`, the two children assigned on
2026-08-28 out of `example/thread_test.satl`, which writes `my_thread.start()` before
`my_thread.join()`. **The ledger caught this one the day the numbers were minted**:
"M23 names only `satellite.variable.thread`" was true of five of these six.

**And `satellite.variable.capsule` `1 6 16` — the deferred call — is this
milestone's.** `satellite.thread.new(f(x))` takes an unevaluated call expression as
the thread body, which is the packaging semantics DESIGN §12 and §13 lean on to keep
§2 shut, and **nothing earlier needs it**: M16's `sort_down(key)` is §1.1's one
primitive rather than a comparator, which is exactly why sorting did not need
first-class capsules. `SCRATCH.md/MILESTONE.md` §1 filed it as *"M23 at the latest,
and probably earlier"* — it is M23, and if something earlier turns out to need it,
the argument that sorting did not is the thing that has to fall first.

**`parallel_for` is not here, and §4.5.1.2's decision assumes it.** The whole
parallelism surface of the language is the six paths above; the pool starts at
startup because *most satellite code will call `parallel_for`*, and that construct is
in no numbering, no document and no milestone. **This is the milestone it would
belong to**, and it is named here so the gap is visible from the one entry a reader
would look in.

**M24 — windows.** `libsatellite_window.so`, `dlopen`ed on first use. Marshalling to
the UI thread is satellite's job, never the user's (DESIGN §10.3).

**Three numbered paths, and "windows" in prose was reaching none of them.**
*(2026-08-28.)* `satellite.window` `1 24 (0)` and `satellite.window.new` `1 24 1` —
**a different node from `satellite.window.console` `1 24 2` and its
`new(title, width, height)` `1 24 2 1`, which are M1.5's** and which M1.5 names —
plus the type `satellite.variable.window` `1 6 15`, which M1.5's own example
declares:

    satellite.variable.window my_console =
        satellite.window.console.new("window_title", 800, 600)

So M1.5 wrote a program against a type no milestone built. **The declaration is
this milestone's and the constructor on the right-hand side is M1.5's**, which is
the split DESIGN §4.4 predicts whenever one spelling is two nodes, and it is why
naming all three here is a fix rather than an addition.

**M25 — another file: `satellite.include(spaceship)` and `satellite.analyze`.**
*(New 2026-08-28. After M24.)* **Two numbered paths and one mechanism.**
`satellite.include(spaceship)` `1 1 2` loads another `.satl` file and runs it;
`satellite.analyze` `1 16` reads another `.satl` file and reports on it without
running it. **They are the same act — the front end turned on a file that is not
the one being run — and they were the last two paths in this list that nothing
reached**, `1 1 2` sitting in "Later" as *"`satellite.include` of other files"* and
`1 16` sitting in nothing at all, which M18 named and declined in as many words.

**`satellite.include`'s other three shapes are already owned and this milestone
takes only the fourth.** `satellite.include` `1 1`, `include()` `1 1 0` and
`include(satellite)` `1 1 1` are M17's: DESIGN §3's hello world writes
`satellite.include(satellite)`, and §4 explains it as *"the one include form that
does nothing — include the runtime, which a running program already has. Every other
form names a **spaceship** and loads it."* **This milestone is "every other form".**

- **A spaceship is a unit of includable code** (DESIGN §13, decided), and
  `include_decl` is already in §6's grammar as a top-level form taking an
  expression, so **M4 parses it** and nothing new is parsed here.
- **What is new is a second program in one run**, and everything hard is in that
  sentence: whose numbering the loaded file's user names take (WORD_NUMBERS §3 — the
  next number free under the node that owns them, *allocated when the name is first
  met*, so two files met in one run allocate from one counter and a `PathId` is still
  valid for one run only); whether a spaceship is loaded once or once per include;
  and what a cycle does.
- **`satl` already knows how to find one.** v1's `system_facts/system.cpp` carries
  `library_path()` and the `-DSATELLITE_LIB_DIR` / `VERSION_DEFS` build coupling —
  *"system.o is the one object the Makefile compiles with `-DSATELLITE_LIB_DIR`"* —
  and **M20 hands that half of the file here** rather than porting it with the
  machine facts it has nothing to do with.
- **It is the pool's clearest tenant.** §4.5.1's own list of the tenants that collect
  the ~170-line figure rather than the ~2,650 one names *"`satellite.include` of
  another file"* first: the second file parsed in a run is the second batch, and the
  pool is already warm.

**`satellite.analyze` `1 16` is real in v1 and is the last of its modules with no
milestone.** v1 advertises it as `satellite.analyze("file.satl")` — one argument,
present in the registry, the evaluator and the programs — and M18 records the
defect that makes it worth naming: v1's own help answers for six modules and returns
the empty string for `analyze`, *"which is how both callers tell"* a name is not a
module, so **asking v1 about `analyze` answers as though it does not exist.** A help
that walks the trie (M18) cannot repeat that; a path that nothing builds still can.

**Its call shape has no number, and that is the milestone's first blocker.** §2.2
writes `satellite.analyze` `1 16` bare — one of only three parents in that table
written without the `(0)` marker — and v1 takes exactly one argument. So
`satellite.analyze(path)` needs a number and **only WORD_NUMBERS can assign it**;
the same question `1 8 2` and `1 18 1`–`1 18 3` raise at M19, and the answer should
be given to all of them at once.

**Second blocker: what an analysis says.** M4 already gives `satl --unparse` and M5
gives codes, spans, carets and *did you mean*; **what `analyze` adds over running
`satl` on the file is a decision nobody has taken.** v1's answer is not recoverable
as a specification — it is a module that existed and was never described. The honest
options are that it is the front end's diagnostics as a value a program can read,
which makes it the first piece of Satellite Orbit's shape (M28), or that it is
`--unparse` with a report, which makes it small. **It must not be a third help
system**; M18's whole argument is that a second document is the drift being removed.

**Done when** a program includes a spaceship that declares a capsule, calls it, and
prints its answer — with the spaceship's own `satellite.library` globals visible
under §7's rules and its user names numbered from the same counter the host file
used — and when `satellite.analyze` over a file with a deliberate error prints the
same code, span and caret M5 would print for it **without running a line of it**,
and over a clean file says so. **Two files, one run, one numbering** is the whole
claim.

**M26 — spacesuits.** *(New 2026-08-28. After M25.)* **Three numbered paths** —
`satellite.spacesuit` `1 10 (0)`, `satellite.protected` `1 11 (0)` and
`satellite.public` `1 12 (0)` — and the largest feature in this list by everything
except path count. DESIGN §13 has it under **Decided**: *"Classes are
`satellite.spacesuit`, with `satellite.protected` / `.public` blocks and a bare-name
type. Reference semantics."*

**Nothing here is speculative and that is unusual for a milestone this late.** §6's
grammar already writes the rule — `spacesuit_decl`, `suit_block`, `suit_section`,
and `type := IDENT` for a spacesuit named bare — **so M4 parses it**, as three of
DESIGN §6.1's eleven segment-1 words; §7 already says resolve runs every capsule
name first and then every spacesuit name, so **M7 resolves it**; and §7 already
records that a spacesuit is a reference type with a fresh slot that *"is not an
implementation detail."* What has never had a milestone is the part that runs.

**It sat in "Later, in no fixed order" with a grammar rule already written**, which
`SCRATCH.md/MILESTONE.md` §3 called out as its own row: *"a whole feature, with a
grammar rule already written, in the unordered pile."*

- **The type.** A bare `IDENT` in type position is a spacesuit (§6 grammar), which
  is the **second** place in the language where a bare identifier means something
  other than a user's own name — §7.7's six spellings of `arguments` is the first,
  and M18's topic pages would be the third. All three should be settled the same
  way: the language *recognises* a name rather than introducing one.
- **The two sections.** `satellite.protected` and `satellite.public` are blocks
  inside the suit, not modifiers on a member, which is why they are segment-1 words
  with numbers of their own rather than syntax.
- **Reference semantics, and DESIGN §12's refusals stay refused**: bare field access
  is accessor methods only, *"a spacesuit field is reachable from inside the
  spacesuit and nowhere else"*; user-defined operators and a spacesuit `to_string`
  the printer consults remain deferred, and this milestone must not quietly grant
  either by needing one for its own demonstration.
- **`satellite.returns(TYPE)` reaches its interesting case here.** DESIGN §13
  settles the return-type syntax and adds *"a constructor is the one member forbidden
  to declare one, since what a constructor produces is the object"* — a rule with
  nothing to apply to until spacesuits exist. M4 parses `returns` at `1 21`; this is
  where the refusal is enforced.
- **It is measured against a number v1 already has.** DESIGN §8.1 records the first
  satellite's spacesuit benchmark — a million constructions, a million prints and a
  million string transfers, at 2.79 s against the `double` version's 2.79 s — and
  §9 of this plan says a figure is re-measured on this machine rather than quoted.

**Blocker: two objects naming each other, and DESIGN §12 has already conceded the
argument.** Its garbage-collection entry says the original reasoning *"no longer
holds"* — refcounting sufficed because every value was immutable and cycles could
not be constructed, and **a spacesuit instance is mutable**, so *"two objects can
name each other and neither is ever freed. This is a real leak, it is the price of
reference semantics, and it is not fixable by being careful."* §12 names both
answers — a weak-reference field as the cheap partial one, a tracing collector over
the object table as the complete one — and **names neither as chosen.** This is the
milestone that creates the leak, so it is the milestone that has to say which, and
the choice is the author's.

**Done when** a program declares a spacesuit with a protected field and a public
accessor, constructs two of them, passes one into a capsule that mutates it, and
proves the caller sees the mutation — reference semantics, demonstrated rather than
asserted — and when reaching a protected field from outside is refused by name with
M5's caret rather than by silence.

**M27 — the network.** *(New 2026-08-28. After M26.)* **Nine numbered paths** —
`satellite.network` `1 20 (0)`, `.http(port)` `1 20 1`, `.https(host, port)`
`1 20 2`, `.new` `1 20 3`, `.open` `1 20 4`, `.receive` `1 20 5`,
`.http(host, port)` `1 20 6`, `.https(port, cert, key)` `1 20 7`, and the type
`satellite.variable.network` `1 6 12`.

**This is the one milestone in this list whose first job is a specification rather
than a build, and it says so instead of pretending otherwise.**
`SCRATCH.md/WORD_SURFACE.md` sources the whole namespace to `v1docs` and records the
position in one clause: *"v1 surface; nothing in this tree's documents promises it
yet."* There is **no v1 implementation** — five words in a first-satellite document,
swept into the numbering because §1's generating rule numbers what it finds — no
DESIGN section, and no sentence in this plan outside the ledger that counts it.
**That is a different and worse position than unscheduled**, and it is why this
milestone is late in build order and honest in shape: eight of its nine numbers
describe a protocol stack in five words.

**What the numbering already commits to, and it is more than it looks.** The two
`http` shapes are a **server** (`port`) and a **client** (`host, port`), and the two
`https` shapes are the same split with the server's `(port, cert, key)` — so §1.3's
*the arity is the identity* has already decided that one word serves both ends of a
connection and that the distinction is carried by argument count rather than by two
names. **That is a language decision made by a sweep**, and it is the first thing to
confirm or overturn, because §1.2 makes it permanent the moment anything is built
against it.

**M18 already refuses these paths and that is the shape of the guarantee.** Help
prints a node when `handlers[path_id]` is non-null, so
`satellite.help(satellite.network)` **refuses in plain words** rather than printing
seven shapes nobody has written — one of M18's two self-verifying checks. Until
this milestone lands, that refusal *is* the language's honest answer about the
network, and DESIGN §1.1 is why that beats a stub.

**Blockers, and none of them is this milestone's to take alone:**

- **What a `satellite.variable.network` `1 6 12` value is.** DESIGN §8's types table
  has no row for it. A socket is a reference type with the same two-handles-one-fd
  problem M19's file has, and DESIGN §8's reference semantics plus M19's
  `std::atomic` fd is the precedent to follow or to depart from deliberately.
- **What `receive` `1 20 5` blocks on, and on whose thread.** DESIGN §10.1's rule is
  that the program's own thread never blocks on the terminal; a socket is the same
  argument with a different fd, and M14 has already built the machinery — a reader
  thread and a queue — for the terminal case. **Whether that generalises is the
  design question this namespace exists to ask**, and answering it in a network
  milestone without saying so would be building a second reader.
- **Whether `https` implies a dependency.** Certificates and a TLS stack are the
  first thing in this language that cannot be written from libc, and §4's `ldd`
  discipline — six shared objects for `satl`, measured, not quoted — has been a
  stated property since M1.5. **A milestone that silently takes satl from six to
  a dozen would be changing a promise nobody wrote down as a promise**, and M13's
  vendored-PCG row in LAYOUT.md is the precedent for how a dependency arrives.
- **Nothing about it is QUAD's.** QUAD.md's table credits it with none of this, so
  unlike every other milestone in this list it has no acceptance program waiting for
  it. Its done-when has to be invented rather than found.

**Done when** a satellite program serves one HTTP request on a port and another
satellite program on the same machine fetches it and prints the body — two
processes, no C++ in the middle — **and when DESIGN has a section for the namespace
that this milestone wrote before it built anything.** The order matters: this is the
one milestone in the list where demonstrating first would *be* the specification,
which inverts what every other section of this plan does.

**M28 — Satellite Orbit and the wire format.** *(New 2026-08-28. Last in build
order until 2026-09-04, when M29 was appended behind it.)* **One numbered
path, `satellite.container.result` `1 4 4`** — §2.2's own
gloss for it is *"Satellite Orbit's answer"* — and one rule that three documents
already impose on a thing that does not exist.

**It is last because everything it distributes has to exist first**, and it is a
milestone rather than a line in a pile for one reason: **§8.1's constraint has no
enforcer until this exists.** *"Anything that persists a PathId must record the name
instead."* A language word's `PathId` is frozen forever and quotable; **a user's
capsule or spacesuit takes the next number free under its parent, allocated when the
name is first met, and is valid inside one run only** (WORD_NUMBERS §3, DESIGN §4.3,
§8.1's table). Orbit and the wire format are the two things that will want to write
one down, and SATC.md §3 imposes the same rule on the `.satc` — where M4.5 already
obeys it. **Nothing checks the rule for Orbit, and nothing can until Orbit is a
milestone**, which is precisely what `SCRATCH.md/MILESTONE.md` §3's row said: *"fine
where it is, but WORD_NUMBERS §3 and SATC.md §3 both impose a constraint on it, and
nothing will check that until it exists."*

**`1 4 4` is the whole of its numbered surface today, and that is a warning rather
than a size.** A result type under `container` says that Orbit's answer is a value a
program holds and asks about — the shape M25's `satellite.analyze` also gestures at
— and nothing else about the feature is numbered at all. **Every other path this
milestone needs would be new**, which under §1.2 means permanent, and under §1.1
means numbered in the order they are first written down. **Doing that badly is the
one mistake in this list that cannot be undone**, and it is the argument for this
milestone being last rather than for it being small.

**Blockers, and the first is not about the network:**

- **What Orbit is.** It has no section in DESIGN, no paragraph in this plan, and one
  gloss in the numbering. Every other milestone here could name the document that
  specifies it; this one cannot.
- **What a wire format persists.** The rule says never a user `PathId`; the name
  instead. Then a receiver has to allocate its own number for that name **under the
  same parent**, and §8.1's *"valid inside one run only"* becomes *valid inside one
  process only* — which is the same sentence and a much harder one, because two
  processes met the same name at different times.
- **Its relationship to M27.** Orbit over a socket is the obvious reading and the
  numbering does not say so; `satellite.container.result` is under `container`, not
  under `network`.

**Done when** two `satl` processes exchange a value and the receiver's answer is
identical to a local computation of it — and when a user name that both processes
know is proved to have **different numbers in each**, and to work anyway. That
second clause is the whole milestone: it is §8.1's rule turned into a test, and it
is the only test in this list that can fail for a reason no single process can see.

**M29 — the calendar: what a time value can be beyond *now* and *sleep*.** *(New
2026-09-04. After M28 — appended rather than slotted, because a mid-list
insertion renumbers every milestone behind it, nothing anywhere depends on this
one, and the author can pull it forward the day something does.)* **Three
numbers arrive with it — `satellite.time.new` `1 9 2`, `satellite.variable.date`
`1 6 7` and `satellite.variable.duration` `1 6 8`, moved from M13 the day M13's
blockers cleared** — plus a debt that is not a number yet:
`satellite.variable.time` `1 6 3` has **zero children in WORD_NUMBERS §2.2**
while its three hand-written siblings have 16, 7 and 14, and v1 handles
`.minus(t)`, `.nanoseconds()` and `.to_string()` on a `Time` today.
**WORD_NUMBERS numbers them here and nowhere earlier** — until then a satellite
program can obtain an instant and do nothing with one but display it, which is
M13's stated boundary.

**The reason these four things are one milestone is that none can be designed
apart from the others.** `time.new`'s entire specification is nine words in a
deleted note — *"set the arguments for a point in time"* — which reads as a
component constructor and contradicts v1's rule that an instant is read off the
clock and never written down as a literal; whether it takes a year and a month
or a count of seconds **is** the design of `date`, and what `now.minus(then)`
answers — a number of seconds, or the `duration` DESIGN §12 still defers —
**is** the fate of `1 6 8`. Settling any one of the three alone settles the
other two by accident, which is DESIGN §13's own warning applied to itself.

**What is already fixed, and this milestone inherits rather than reopens** (§13,
settled 2026-09-04): the value is `system_clock` on the Unix epoch, int64
nanoseconds; a `date` is therefore a *reading* of that value and never a second
clock; and no instant is ever a literal in source.

**Blockers — the kind M15 carries, decisions before code:**

- **What `date` is** — representation, constructor, methods. It has a number, no
  row in DESIGN §8's types table and no v1 code; the specification is written
  here or the number stays reserved another milestone, said out loud either way.
- **What `time.new` takes**, which is the same question from the other side.
- **Whether `duration` exists.** The v1 source that uses the word says there is
  no such type and gives four reasons; DESIGN §12 defers it; WORD_NUMBERS §1.2
  means the number cannot be struck without a hole. Decide it, or write the
  refusal down where the number is.

**Done when** the designs exist in DESIGN with dates, WORD_NUMBERS carries
`1 6 3`'s children with §2.5-style attribution, and one program takes `now`,
sleeps, takes it again, and displays the difference through whatever `.minus`
answers — the ten-line proof that an instant stopped being display-only.

**"Later, in no fixed order" is empty, and this is where it used to be.** It held
eight entries — `satellite.variable.file`, `.time`, `.date`; `satellite.random.*`;
`satellite.variable.variant`; spacesuits; `satellite.include` of other files;
Satellite Orbit and the wire format — and every one of them now has a milestone that
names it: M19, M13, M13, M13, M12, M26, M25 and M28 in that order.
*(`satellite.variable.float` left it on 2026-08-27 and is M15, not M11, which this
sentence said until 2026-08-28. And `.date` moved on again on 2026-09-04, M13 to
M29 — the sentence above keeps the count of the day of the emptying, and §8.2's
table is where ownership is current.)*

**The list is not a milestone and things hid in it**, which is the only thing that
was ever wrong with it. `SCRATCH.md/MILESTONE.md` is the audit that proved it: run
per path rather than per namespace on **2026-08-27**, it found **122 of 218 numbered
paths reached by no milestone at all** and another 40 reached only by a sentence
about their parent. The three namespace sweeps before it reported 50, then 41, then
29 — none of them could see a path hiding under a parent §8 happens to name, which is
where most of them were. `satellite.random` was sixteen of them, sitting in this
list, with working and tested v1 code behind nine of its rows.

### 8.2 Every numbered path is named, and here is exactly what that claims

*(2026-08-28, at the end of the pass that added eleven milestones; renumbered
2026-08-30 and re-sorted, and the totals were unchanged because no path moved.
**Three paths moved on 2026-08-31 and one was added** — `shift_left` `1 6 4 1`,
`shift_right(n)` `1 6 4 11` and `modulus(a, b)` `1 6 4 12` are M8's rather than
M15's, so M8 reads 13 and M15 reads 4, and `digits` `1 6 4 15` is a new row. **The
total moved for the first time**, from 222 to 223, and that is the distinction
this table is for: paths moving between milestones must leave it alone, and a row
appended to WORD_NUMBERS §2.2 must change it by exactly one. M8's entry has both
reasons. **And it moved again on 2026-09-03, by four at once**: M12 appended the
variant's `holding`, `holds(x)`, `held` and `clear` at `1 6 14 1` through
`1 6 14 4` — WORD_NUMBERS §2.5 says who assigned them and by what walk — so M12
reads 6 and the total reads 227. **And three moved on 2026-09-04 without the
total moving**: `1 9 2`, `1 6 7` and `1 6 8` left M13 for M29 — new that day,
the first milestone appended since the renumber — so M13 reads 20, M29 reads 3,
and the total stays 227.)*
WORD_NUMBERS.md §2.2
holds **227 rows and 224 distinct numbers** — the three duplicates are §2.3's
`.range` aliases and nothing else. **All 227 rows are named by exactly one milestone
above**, counted mechanically against §2.2 rather than read off the prose:

| | paths | | | paths |
|---|---:|---|---|---:|
| M1.5 | 2 | | M16 | 39 |
| M3 | 2 | | M17 | 4 |
| M4 | 3 | | M18 | 3 |
| M6 | 5 | | M19 | 20 |
| M7 | 1 | | M20 | 37 |
| M8 | 13 | | M23 | 6 |
| M10 | 7 | | M24 | 3 |
| M11 | 26 | | M25 | 2 |
| M12 | 6 | | M26 | 3 |
| M13 | 20 | | M27 | 9 |
| M14 | 8 | | M28 | 1 |
| M15 | 4 | | M29 | 3 |
| | | | **total** | **227** |

***THIS TABLE IS STALE AND THE HEADLINE NUMBERS ABOVE IT ARE TOO — FOUND
2026-09-11, WHILE M20 WAS LANDING, AND SAID HERE RATHER THAN QUIETLY FIXED.***
`tests/words_test` counts WORD_NUMBERS §2.2 mechanically on every build and it
now reads **302 rows and 298 distinct numbers**, against the 227 and 224 this
section claims. **The gap is 75 rows and none of them is a renumber**: M19.5's
eight, M19.6, M20's fifty-one — thirty-eight for the arguments subtree and
the three call shapes, ten for `satellite.container.arguments`' selectors, and
`satellite.variable.string.resolved` `1 6 1 17` — and **M21's six**: the seeded
tier's four at `1 7 13`–`1 7 16`, the float's first method `1 6 10 1`, and the
`.range` alias of `1 7 14` that takes no number. Each landed in a commit that
moved `words.def`, §2.2, `help_lines/nodes.tsv` and the entries together, and
**none of them moved this table.**

***AND M21 WALKED PAST IT TOO, WHICH IS THE SECOND MILESTONE TO DO SO SINCE
THE NOTE ABOVE WAS WRITTEN.*** It updated these two headline numbers, because
they are derivable from a checked thing in one command; it did not recount the
per-milestone column, for the reason the next paragraph gives. **A note that
says "somebody has to run a sweep" and is then walked past twice is evidence
about the note rather than about the sweep** — M21's own §3.8 records the same
shape one file over, where `words.def`'s header tallies had gone stale for the
fifth time and its own comment had predicted it. The difference between the two
is that `words.def`'s counts are now derivable from `tests/words_test`'s four
assertions, and this column is derivable from nothing.

**THE PER-MILESTONE COLUMN CANNOT BE PATCHED AND MUST BE RECOUNTED**, which is
why this is a note and not an edit. Its own rule is that it is *"counted
mechanically against §2.2 rather than read off the prose"*, and adding 69 to
whichever rows look likeliest would be reading it off the prose — the exact
thing `SCRATCH.md/MILESTONE.md` §2 says produced the 122. **The recount is a
sweep somebody has to run**, and the two totals above are what it has to come
out at.

**WHAT DID NOT GO WRONG IS WORTH SAYING TOO.** The staleness is in this
section's arithmetic and nowhere else: §2.2, `words.def`, `help.def` and
`nodes.tsv` agree with each other on every build, by three separate asserts,
and it is those four that decide what the language is. What this table decides
is whether a path has an owner — and the check that every row is named by
exactly one milestone has not run since M19.5 either.

***And one of the 227 is named by no sentence above, which M17 found by trying
to write its own four down.*** *(2026-09-06.)* `satellite` `1` — the root, and
the runtime singleton — appears in WORD_NUMBERS §2.2's first row and in no
milestone entry here. **M17 holds it**, by elimination over DESIGN §3: every
other path hello world writes is named — `capsule` `1 2` at M7, `main` `1 3`,
`console.display` `1 5 1` and `return`'s three shapes at M10, `container.list`
`1 4 2` at M16, `variable.string` `1 6 1` at M11 — and the seven milestones
below hold none. **The count was always 4 and nothing renumbers**; what was
wrong is the sentence above it, which now says 226 rather than 227 unless this
paragraph is read as part of it. **A table counted mechanically and a prose
that names things one at a time agree on the total long after they stop
agreeing on which**, and the total is the half that cannot show the drift.
*(M17's three named rows are named in **M25's** entry rather than in M17's,
while it explains which shape it is not taking — so a reader looking for M17's
paths under M17 finds none of the four.)*

**M1, M2, M4.5, M5, M9, M21 and M22 hold none, and that is right rather than a
gap.** M2 registers all 227 and owns no behaviour; M5 and M9 build the machinery
every other row dispatches through; M21's whole content is a program. **The table
counts the milestone that makes a path answer, not the one that parses it** — M4
parses DESIGN §6.1's eleven segment-1 words and appears here with three, and M11's
`satellite.statement.*` rows say their parse rules land at M4.

**Four things this table does not claim, said plainly so nobody reads it as
finished:**

- **Named is not built, and it is not even specified.** M27's nine paths are five
  words in a first-satellite document, and its own entry says the first job is
  writing the DESIGN section that does not exist. M28 has one numbered path and no
  specification anywhere.
- **Five numbers are reserved and unbuilt on purpose**, listed by the milestone
  that owns them rather than left to a later audit: `1 9 2`, `1 6 7`, `1 6 8`
  (M29); `1 6 9` (M12); `1 22 2` (M20, the one `satellite.system` path with no
  v1 code behind it). *(Eleven until 2026-09-04, when M13's blockers cleared and
  its nine left the list — six into its build, three to M29.)* A
  milestone that quietly leaves numbers behind it is how this document came to have
  a 121-path ledger.
- **Some numbers do not exist yet and are owed.** The failure contract's `.ok()`,
  `.path()` and `.error()` (M19); `.swap.used(unit)`, `.this.used(unit)` and
  `.environment(name)` (M20); a call shape for `satellite.analyze` `1 16` (M25) and
  for `1 8 2`, `1 8 3` and `1 18 1`–`1 18 3` (M19); a fourth `satellite.random`
  shape that takes a seed, for which `1 7 13` is free (M13, declined; M21, needed).
  **Only WORD_NUMBERS can assign them**, and when it does they arrive as new rows
  under nodes this table calls finished — which is exactly the shape of the failure
  that produced the 122 in the first place, and the reason they are enumerated here.
- **Ten milestones state something only the author can clear**, six of them under a
  heading that says *Blocker* and four inside an open list that stands in front of a
  demonstration: M15 (the rounding rule — **cleared 2026-09-04 by delegation,
  the day it landed: half away from zero, ratifying what the code had taken
  three times; DESIGN §8.6 and MILESTONES/M15.md §2**), M6 (three, all §4.5's), M12 (whether
  "nothing" is a state or a value — **cleared 2026-09-03 by delegation, the day
  it landed: a state every type has, DESIGN §8.7**), M13 (the clock, and what
  `1 7 1`–`1 7 3` name — **cleared 2026-09-04 by the author: v1's clock split
  stands, and the zero-argument shape is a refusal by design**),
  M19 (the mode-word fold, and the failed-open contract's missing numbers), M20
  (DESIGN §7.7's live-code mapping, and the 33), M21 (a seeded draw with no number),
  M26 (cycles), M27 (four, starting with what a `satellite.variable.network` is),
  M28 (what Orbit is). **A milestone with a blocker in its done-when is scheduled; it
  is not startable**, and the two are worth telling apart.

**The cheapest ten came from a clause, not from a milestone.** Ten of the 121
uncovered paths were closed by adding a sentence to a milestone that already owned
the work and had never said so: **`satellite.bool`'s three to M11** — which had said
outright that they were not its — **`satellite.thread` and its `new` plus
`satellite.variable.thread`'s `start()` and `join()`, four, to M23**,
**`satellite.variable.capsule` `1 6 16` to M23**, and **`satellite.window` `1 24 (0)`
and `.new` `1 24 1` to M24**. Two more moved from *implied* to *named* in the same
clauses — `satellite.variable.window` `1 6 15`, which M24 now names beside the `.so`,
and `satellite.capsule` `1 2 (0)`, which M7 now names beside its frames. That is the
M3/M4 and M16 fix for the fourth and fifth time. **The lesson has not changed since
the first time it was written down here: the dangerous half is not the unscheduled
namespace, it is the path sitting under a parent some milestone happens to name.**

---

## 9. Measurement discipline

- **Measure on this machine, do not quote.** The first satellite's source says gtk4
  and vte pull "119 shared objects"; the real number here is 78. The shape held and
  the count did not, so the count is not repeated anywhere.
- **A number goes beside the decision it justifies**, in the file that makes the
  decision — not in a commit message, where nobody looks for it again.
- **Say what was measured and when.** Every figure in this document carries the
  machine or the date or both.
- **Startup is re-measured every milestone** against §4.3's floor. **`make startup`
  is what does it**, as of 2026-08-31 — and for every milestone before that this line
  was a rule with no target behind it, so M4, M4.5 and M5 landed with no measurement
  at all and a 34× regression went three milestones before anybody looked
  (MILESTONES/M6.md §9.1). That is the argument for the rule and it is also the
  argument against writing one down without building the thing that runs it.
- **Verify through the real code path.** If a check passes and the thing is still
  broken, the check is wrong. Running the *installed* binary is what proves an
  install, not comparing bytes.

---

*Companions: [DESIGN.md](DESIGN.md) — the generating rule, the syntax, the
numbering, scope, types, and what the language refuses. [LAYOUT.md](LAYOUT.md) —
every file in the tree and what it is for.*
