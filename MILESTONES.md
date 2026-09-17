# satellite-004 — MILESTONES

Everything decided and **not yet built**, one milestone each. Written 2026-09-16
at the author's asking: *"let's build milestones for everything that we didn't
do"*, then swept comprehensively against PLAN.md, ERROR.md, PROGRESS.md,
`words.tsv` and `REGISTRY.satellite` so that nothing outstanding is missing.

**Read PROGRESS.md first** — it is what IS built. This file is only the debt.

**The numbering continues PLAN.md's.** M0–M12 are PLAN's own and keep their
numbers and meanings; M13 up are new, most of them found while building the
object model on 2026-09-16. Where a decision is still the author's it says so
rather than guessing.

---

# Part 1 — PLAN.md's milestones, with what is actually true now

## M0.5 — port the build, the installer and satl-term — **THE VERY NEXT ONE**

PLAN.md calls it "the very next milestone" and it has not moved. The build port,
the installer, and satl-term. **Nothing in Part 2 unblocks it and it unblocks
nothing** — it is simply owed, and it is the one the author named first.

## M0.6 / M0.7 — the prompt, in satl and then in satl-term

`satellite.directory.change` and `.list` come with M0.6. M0.7 is the same prompt
inside satl-term. Both wait on M0.5.

## M1, M2, M3 — the `.satc`, the `.satb`, the `.sati`

**PROGRESS §6.5 RECOMMENDS COLLAPSING ALL THREE AND IT IS NOT YET APPLIED TO
PLAN.md.** The bytecode registry already IS the numbered program, one code a
word, so `.satc` is superseded; strings are 16-bit codes inline and counted, so
`.sati` is superseded **and D3.1 dies with it**; `.satb` survives as *work* but
not as a *file*, because combine's marks live in the bytecode and the registry
already reserves `batch_start`/`batch_end`/`wait`/`batch_size`.

**This is a decision the author has to take before the converters are built**, and
it is worth taking early: it turns M1 + M2 + M3 + M1.5 + M2.5 + M3.5 into about
two milestones.

## M1.5 / M2.5 / M3.5 / M3.6 — the converters

Numbers and marks and bits back into readable `.satl`. **Blocked by M24** (below):
until the author rules whether a stored program is a token transcription that is
re-lexed or a 16-bit code stream, a converter cannot know what it may record.
M3.6 is blocked twice over — `.sate` is not defined yet.

Already known: comments do not round-trip. `//` is discarded in the lexer
(003 DESIGN §5.6), so a comment is a marker with no text.

## M5 — names and `satellite.log`

Not started. The testing method in PROGRESS §2 depends on it: *"done when a run
writes no `[entry]` to satellite.log"*.

## M6 / M7 — the parser and the runtime

**Built, but not the way PLAN describes.** There is no `.satb` to run and no
ported recursive-descent parser: the bytecode registry is lexed directly and
`program_walk.cpp` walks it. PLAN's text should be rewritten to match what runs,
or it will read as owed work forever.

## M8 — user-defined classes (spacesuits) — **the machine is built, the grammar is not**

`satelliteSpacesuit` and `satelliteUserDefinedObject` exist and compile.
**Nothing can declare one**: `satellite.spacesuit` has no parse rule in 004 — no
lexer shape, no sibling to `capsules_in`, no constructor.

**Build the author's model** (2026-09-16, and he was right against an argument):
a spacesuit is just a collection of bytecode, so it reuses what already runs —

| a spacesuit needs | what already does it |
|---|---|
| find the declaration | `capsules_in()` — the same scan, word `1 10` |
| remember where it is | `CapsuleSite{row, body}` |
| its fields | a `VariableTable` — what every frame already is |
| call a method | `run_statements` at that site |
| reference semantics | the handle that is already an arm |

003's `parser_declarations.cpp` and `resolve.cpp` are the port.
`satellite.protected` and `satellite.public` (`1 11`, `1 12`) come with it.

**Owed inside this one:** `satelliteCapsule` holds a COPY of bytecode where the
interpreter's own capsules hold a site. Two sources of truth, and PROGRESS §6.5
already names that shape of bug — fix before anything depends on it.

## M9 — polymorph

`POLYMORPH/M1.md`–`M7.md` hold the discussion; it is uncommitted and `M7.md`
still says 342 words (it is 364). **Two of the author's decisions are open:**
D9.1 what "re-included into the individual capsules" means, D9.2 what `args` are
passed to. **D9.3 is answered** (2026-09-16): *"a class declared twice is an ERROR:
name collision"*. Blocked by M8.

## M10 — `satellite.cxx() { C++ }`

Not started. PROGRESS §6.5 settled the piece that looked hardest: a C++ file
round-trips byte for byte through the bytecode because a **count** needs no
closing marker, so `foreign_text_token` needs no code.

## M11 — `satellite.infinity`

Not started. **D11.1 is answered** (the author, 2026-09-16): *"we answer what we
can, and give an error on what we can't"*. **An infinity carries a multiplier with it:**

- `infinity + infinity` displays **`infinityx2`**, and the infinity *"just carries
  an "x2" with it unless it goes away or causes an error"*.
- `infinity - 50%` is **`infinityx0.5`** -- *"unless the infinity is definitely
  reduced to nothing"*.
- What satellite does not know the answer to is an ERROR, never a guess.
- **The multiplier is ONE `satellite_float`, for going up or down** -- *"it will
  carry around a satellite float"*, *"an infinity will come with a single satellite
  float for going up or down"*, *"so we will have extreme precision for our
  infinities"*. `x2` and `x0.5` are the same number moving, not two fields. How many digits
  it keeps is a new config row, **`arguments.infinity`**, a digit count held as a
  `satellite_number` so the user can enter anything; **the default is 4096**.
- **Held to 4096 digits, DISPLAYED rounded to 32, and BOTH are configurable** --
  *"that is 4096 digits in precision by default, but displayed as a rounded thing...
  we round to 32 digits"*, *"both digits configurable"*. The rounding is the
  display's only; the multiplier keeps every digit it holds. Two rows, both digit
  counts held as `satellite_number`s: `arguments.infinity` (the author's name,
  default 4096) and **`arguments.infinity_display`** (default 32 -- a name proposed
  after `arguments.startup_display`, the author's to change).

**Depends on M20**, because the multiplier is a `satellite_float`. Neither row is
added yet: `satellite_config.hpp`'s rows are the author's, and they go in with M11.

## M12 — finding more batches while the program runs

One of the last milestones. **D12.1 is open:** the parallel-group syntax in the
numbered file. The measurements are already in: a thread costs 34.3 KB, math
stops speeding up at 24 threads, one recall is 12,486 ns against 28 ns a command,
so threads take BATCHES and never single commands.

**TBB IS NOT ADOPTED** (the author, 2026-09-16): *"we are not adopting TBB for the
runners, they are just one line per thread at 1024 threads.... we are doing
something different later after the 1024 threads are done, we keep the 1024
threads warm for now... they just... don't do anything just yet"*. The measurement
(9× our own pool on a small loop repeated 10,000 times, equal on one huge loop)
stays in DESIGN §13 as a measurement, not a recommendation. The 1024 warm threads
convert the program and then wait; what runs on them next is the author's.

---

# Part 2 — new milestones, found building the object model

## M13 — the chain that remembers where it came from

**The author's own example, and the one chain that does not work yet:**

```
string_object.find("str").add("str")      find and add at location of find
```

`.find` answers a POSITION, and a position is a bare `satellite_number` that has
forgotten which string it came from — so `.add` adds to a number. Every other
chain he asked for runs today (`s.bin.find("1010111")`, `n.to_string().add("x")`)
because each segment needs only the segment before it. **This one needs the
segment before that.**

**The fix is one more field.** The loop in `expression.cpp` carries one local,
`receiver`. It needs three: the ORIGIN the chain started on, the CURRENT value,
and the POSITION when the last segment produced one. Then `.add` asks whether
there is a position and inserts into the origin at it. Nothing else moves — not
the pair files, not the conversions, not the token family.

**Open:** does a position survive a conversion? `s.find("a").bin` — the position
in base 2, or the origin lost?

## M14 — satelliteContainer: list, map, variant

**The author:** *"a satelliteContainer which is satellite.container.list
satellite.container.map satellite.container.variant and containers can hold other
containers so we need a shared_ptr"*. The `shared_ptr` is right for his reason: a
container holding a container is a type that contains itself.

**This is overdue rather than new** — `satellite.container.list` is in the `main`
signature of every example file, so the language has been writing it down and not
running it since the first program. `satellite.variable.variant` is already word
`1 6 14` with `.holding`, `.holds(x)`, `.held`, `.clear` designed.

## M15 — satellite_file, and `file_object.seek()`

**The author:** *"file_object.seek(\"str\") and file_object.seek(\"entire_file\")
and file_object.seek(\"collection_of_str\")"*. Needs `satellite_file` as an arm
first; 003 had it (`Fil`, M19), so it is a port. The third overload waits on M14.

**003's hazard, inherited:** `Fil` was the first arm to come off `const`, because
a file is not a value.

## M16 — the rest of the string methods

`find` and the four conversions run. **The list is already frozen** — 23 methods
at `1 6 1 n` — so none of this is a design question:

| owed | word |
|---|---|
| `replace(a, b)` | `1 6 1 12` — the token exists, the fast path does not |
| `contains` `starts_with` `ends_with` | `1 6 1 4`, `6`, `7` |
| `substring` `at` `split` `append` | `1 6 1 5`, `16`, `10`, `14` |
| `size` `empty` `lower` `upper` `trim` `clear` `resolved` | `1 6 1 1`, `2`, `8`, `9`, `11`, `15`, `17` |

Each is one registry row, one `str_*.cpp`, one dispatch line.

**Answered, and one ruling covers both** (the author, 2026-09-16): for
`string_object.replace(number1, number2)` and `string_object.find(number)`, a
number where a string is expected is *"just convert the number to the string and
run that piece, obviously the programmer meant convert to string, but record the
warning in satellite.log"*. The warning needs M5.

## M17 — the number methods, and `number_methods.cpp`'s fourth session

`satellite/bytecode/number_methods.cpp` has been written, unwired, in no Makefile
line and referenced by nothing for **three sessions**. It holds `power`,
`modulus`, `max`, `min`, `shift_left`, `shift_right`, `abs`, `digits`, `bytes`
and the conversions.

It is now **one hop from real** — the method-token family and the chain loop
exist, so wiring it is what wiring `find` was.

**Two gaps found:** `clamp` is numbered `1 6 4 5` and is not in the file; `bytes`
is in the file and is not numbered (next free `1 6 4 21`).

**Finish it or delete it. Not a fourth session.**

## M18 — `satellite.statement.if`

The missing half of `while`, which runs. ERROR.md's depth entry asks in as many
words to be re-read when this lands, because a recursive capsule cannot choose to
stop until it does.

## M19 — `n = 1 & 2` answers 1, silently — **the worst item in this file**

**ERROR.md, open, reproduced by running it.** `evaluate_at` ends an expression on
any code whose precedence is 0, and `&` `|` `&&` `<<` `!!` `~` have none. A wrong
value reaches a **variable** and a **loop bound**, where nothing shows.

`call_word` was fixed to demand its own `)`. `run_assignment` and `run_while`
were not. **The same three lines fix both.**

## M20 — satellite_float, and hex as its own type

**Binary is built** (2026-09-16): `satellite.variable.binary` `1 6 5` is
`satellite_binary_number`, arm 7 of `satelliteObject`, width kept. **Owed on it:**
003 DESIGN 8.5's DECIDED AND UNBUILT rulings -- `+` on bit runs answers a bit run,
the width grows to fit, the left operand's type wins, `!!` joins, indexing counts
from the right -- are not built; today arithmetic reads a binary by worth and
answers a number. That is the author's to schedule, not to be decided in passing.

`satellite.variable.float` `1 6 10` and
`satellite.variable.hex` `1 6 11` are numbered already. **Decided 2026-08-27:** a float is a bool and
two `satellite_number`s — left of the point exact and unbounded, right of it
bounded, because repeated multiplication grows digits downward.

Landing it retires three refusals that today say "there is no float yet": `2 ^ -1`,
non-whole division, and `5/4` the fraction.

**When it lands `tests/not_understood.satl` goes red on purpose** — it is
deliberately rebased onto the next unbuilt type. Move it again; never delete it.

## M21 — the 23 string libraries move off `satellite_string32`

Owed since 2026-09-15. They run on the old 32-bit `strings/` type, renamed
`satellite_string32` on 2026-09-16 when the object model made the two collide.
They move onto the language's own string, checked against 003's satl as they were
the first time, and `strings/` folds into `satellite/satellite_variable_string/`.

## M22 — threads on objects

003's `SuitObject` carries a `recursive_mutex` and a thread access list
(THREAD.md D1, D2) because **two threads sharing one object wrote one value at
once and freed a string twice**. 004's walker is single-threaded and 256 threads
are warm at start-up, so the bill arrives the moment it is not.

Also here: a chain of objects each holding the next is a linked list, and freeing
a long one recurses once per object. 003 answered it with a staged burial.

## M23 — satellite_number's own debts (ERROR §5, never reviewed adversarially)

Nobody has tried to break this type — the four adversarial reviewers were killed
by a session limit and never ran.

1. **`satellite_number x = -1;` holds 18,446,744,073,709,551,615.** The
   one-argument constructor is not `explicit`, so a signed literal converts
   silently under `-Wall -Wextra`. **A wrong value, one keyword, do it first.**
2. `to_text` is quadratic — 8.5 s at a million digits, schoolbook multiply and
   divide, no Karatsuba.
3. Out of memory throws `std::bad_alloc`, not a machine code.
4. The header exposes `unsigned __int128` and `[[gnu::always_inline]]` to every
   file that includes it.
5. The speed bar is missed in places: `i = i + 1` is ×1.49 (clang) and ×3.0 (g++)
   of C++ that refuses to wrap; satellite_string's wide path is ×1.19–×1.24.

## M24 — the stored program: transcription, or code stream

**PROGRESS §6, open, the author's.** Is a `.sat` file a token transcription that
is re-lexed, or a 16-bit code stream? PROGRESS §5 and SATC.md §4 say different
things, and the answer governs whether the writer may record what the parser knew.
**This blocks every converter.**

With it: **the nineteen registry rows marked QUESTION**, which name forms this
language cannot emit — `<<` `>>`, `/* */`, `+= -= *= /= %=`, `&& || ! & | ^ ~`,
`:: -> ? '`, and the four that only ever occur inside a literal. The author asked
for several of them on 2026-09-16, so they keep their codes until he rules.

## M25 — 340 of 364 words are numbered and not built

**24 libraries exist.** Every other word answers `not_built_yet` (14). That is the
true size of the language's remaining surface, and it is worth one milestone
holding the number rather than being discovered a word at a time.

The families waiting: the containers (M14), the file (M15), `satellite.time`,
`satellite.directory` (M0.6 needs `.change` and `.list`), `satellite.network`
(`1 6 12`), `satellite.variable.thread` (`1 6 13`, with `.start()` and `.join()`),
`satellite.access` (ACCESS_PLAN.md, not in PLAN at all), and `satellite.help`.

**Also open (PROGRESS §4):** one more thing to remove from 004's words that the
author could not remember.

## M26 — the prototype's seventeen defects

ERROR.md §1, all OPEN, all reproduced by review. They are the prototype runner's,
and PLAN M1 is where PROGRESS says they get fixed — but the prototype path is
mostly retired now, so **each needs re-checking against the bytecode path before
it is fixed or struck**. The ones that certainly still bite:

- **#7 exit codes are cut to 8 bits** — a machine code of 256 exits 0
- **#11 no ABI check at load**, and **#10 every `*.so` entry is loaded**
- **#13 a byte-order mark or CR-only line endings** give a misleading 10
- **#14 whitespace inside brackets is rejected** — `display( 42 )`
- **#16 unknown escapes are accepted** — `"a\qb"` displays `aqb`
- **#17 a library's file name is never checked against the numbers it describes**

## M27 — the test harnesses can pass for the wrong reason

ERROR.md §3. **#22** `test_string_methods.cpp` splits text arguments on `|`, so an
argument holding `|` is silently two. **#23** `check_string_methods.py` writes 003
programs without escaping. **#24** `words/make_words.py` needs 003's built `satl`.

A harness that can pass wrongly is worse than one that fails, so this is smaller
and more urgent than it looks.

## M28 — the toolchain on this machine

ERROR.md §6. **#33** g++ 17.0.0 miscompiles at `-O2` (a reserve + nested push_back
loop read `text.size()` as 0). **#34** g++ here is `--with-arch=native`, so a
g++-built satellite dies with an illegal instruction on an older CPU — **which
matters for the distribute package** and is the reason clang builds the shipped
binary.

---

# Part 3 — the parallel machine, and the rest of 2026-09-16's session

The cascade landed (`75a5b43`) and `.sate` landed (`da4f3e3`). These are what the
author described that is NOT yet built.

## M29 — converting ahead WHILE running

**The author:** *"the main thread, after it starts 1 thread, that 1 thread starts
256 threads, the main thread will be converting the first line of code into
16-bits, then running it, after it has run the next line, the thread that gets
line 2 is handing it to main"*.

Today `load_program` converts the WHOLE program before `run_main` starts. The
cascade made conversion parallel; it did not make it overlap with execution.

**What it needs, and most of it exists.** `cascade_convert` already counts
finished lines in an atomic — that counter IS the watermark. The walker starts
when line 1 is ready and may walk as far as the watermark says.

**The hard part is not the threading, it is the unit.** The walker runs
STATEMENTS and the watermark counts LINES, and a statement can span lines (a
`while` and its body). So the watermark has to mean "every code up to here is
final", and the walker has to refuse to step past it rather than read a half-
converted row.

**The author's own fallback is already stated:** *"if main gets pieces out of
order, then it has to convert"* — so when the watermark is behind, main converts
the line itself rather than waiting. That makes the pipeline an optimisation that
can never deadlock, which is the right shape.

**THE CHECKER GETS ITS OWN THREAD — ANSWERED 2026-09-16, and it is the ruling
that unblocks this milestone.** The author: *"Checking goes in a separate 1/256
threads, and so that means 250 or so threads are available for conversion..
checking reports into satellite.log with warnings, and dies at that"*.

This was the one conflict: `check_program` walks every capsule before main runs,
which is what makes check.sh's *"nothing ran before the refusal"* true, and
running at the watermark would lose it. The answer is neither of the two options
put to him — **the checker RACES the runner.** One thread of the 256 follows the
watermark and checks; the rest convert; main runs. Checking a statement is
cheaper than running one, so the checker stays ahead in practice, and when it
finds something it writes the warning and kills the program.

**It needs `satellite.log`, which is M5 and not built.** M5 therefore comes
before this, or a minimal log lands with it.

## M30 — the lookahead: running paths in parallel

**The author:** *"eventually we will build some logic that looks ahead at paths
that it can... run in parallel, but until we build that, we are just saving the
16-bit conversion"*.

This is the one that makes satellite parallel rather than its CONVERSION
parallel. Converting a line is pure and independent; running one is not — line 2
may read what line 1 wrote. So this milestone is a DEPENDENCY analysis: which
statements touch which names, and which runs of statements touch nothing in
common.

**Do not start this before M29.** Overlapping conversion with execution is safe
and is worth having on its own; overlapping execution with execution changes what
programs mean, and needs the analysis first.

The registry already reserves `batch_start`, `batch_end`, `wait` and `batch_size`
for exactly these marks — so where the answer is written down is already decided.

## M31 — running a `.sate` without its `.satl`

`.sate` is written and never read. A program saved as bytecode should run from
the bytecode: it is the numbered program, so nothing needs re-lexing.

**This is what makes `.sate` worth writing at all,** and it is small — the
registry is already exactly what the file holds. It also answers half of M24
(transcription or code stream) by making the code-stream half real.

## M32 — `satellite_time`

The author named it as an arm of `satelliteObject`: *"satelliteCapsule,
satellite_number satellite_string satellite_bytecode, satellite_bool,
satellite_time, satellite_file, and any other variable we have"*. 003 had it
(`Time`, eight bytes, an instant in nanoseconds). `satellite_file` is M15; this
is its sibling and is much smaller.

## M33 — the step as a JUMP, not only a front

`arguments.magic` exists and the cascade covers the front of a file. The author's
other shape — *"one of the level 3 threads just jumps onto line say pick a magic
number... so it starts converting at line 5"* — is a STRIDED start: threads
beginning at different offsets rather than one chain from line 0.

Worth doing only if M29 shows the front-loaded cascade leaves threads idle. Filed
so the idea is not lost, not because it is owed.

---

# The author's open decisions — not milestones, but they block them

| | what | blocks |
|---|---|---|
| ~~**D0.1**~~ | ~~which value of `arguments.satc` / `satb` means "never build"~~ **ANSWERED 2026-09-16: DEAD.** *"we threw away satc and satb in favor of all 16-bit"* | — |
| ~~**D1.1**~~ | ~~with `satc = 0`, run line 1 before the file is converted?~~ **ANSWERED: DEAD**, same reason — there is no `.satc` | — |
| ~~**D3.1**~~ | ~~32 bits a character everywhere, or only in the `.sati`?~~ **ANSWERED 2026-09-16:** *"32-bits only when we use the number 40000 as a 16-bit code"* -- everything is 16 bits, and a code of 40000 (`wide_token`) says the next two codes are one 32-bit integer. **Owed:** the lexer still writes a character above U+FFFF behind `wide_run_32_token` (`bytecode_registry.cpp` `character_codes`, read back by `text_at`), which this ruling retires in favour of `wide_token` | — |
| **D9.1–2** | polymorph: re-inclusion, `args` | M9 |
| ~~**D9.3**~~ | ~~a class declared twice~~ **ANSWERED 2026-09-16:** *"a class declared twice is an ERROR: name collision"* | — |
| ~~**D11.1**~~ | ~~infinity's arithmetic~~ **ANSWERED 2026-09-16:** *"we answer what we can, and give an error on what we can't"* -- an infinity carries a `satellite_float` multiplier (`infinity + infinity` is `infinityx2`, `infinity - 50%` is `infinityx0.5`), its digits set by `arguments.infinity`, default 4096, displayed rounded to `arguments.infinity_display` digits, default 32. See M11 | — |
| **D12.1** | the parallel-group syntax in the numbered file | M12 |
| — | ~~adopt TBB for the runners?~~ **ANSWERED 2026-09-16: NO.** The 1024 threads convert one line each and then stay warm, doing nothing yet; *"we are doing something different later"*. See M12 | — |
| — | ~~the leading-slash rule: filesystem root, or program root?~~ **ANSWERED 2026-09-16:** *"program root first, then filesystem root when it's not found, and if it's not found in either, we report file not found, and we keep a cwd for files that are included... so the files directory becomes the cwd for each file"*. (The author wrote it as D12.1; D12.1 is the parallel-group syntax and is still open.) | — |
| — | ~~a number argument where a string is expected~~ **ANSWERED 2026-09-16:** *"just convert the number to the string and run that piece, obviously the programmer meant convert to string, but record the warning in satellite.log"*. The warning needs `satellite.log`, which is M5 | M16, M5 |

**Decided and NOT owed, so nobody reopens it:** the walker's ~27,000 recursion
depth. The author accepted it on 2026-09-16 — it is an order of magnitude past
any recursion a person writes, and the fix is a real rewrite of the walker. What
would reopen it is a real program that runs out, and generated code is the likely
source since QUAD writes satellite.

---

# What is NOT a milestone, and why

**"Every combination of every type."** It looked like the largest item here and it
is not an item at all. With N types there appear to be N×N pair functions to
write; there are not, because `satellite_number` is a hub every type converts
through. `.bin` on a string is `string_to_number` then `number_to_binary` — two
hops through functions that already existed and are already checked, and no
`string_to_binary` is ever written.

- **conversions** cost N in and N out, not N×N — `object_convert.cpp`
- **operations** are written once per type, and the list is already frozen in
  `words.tsv`
- **aliases** cost nothing: several spellings lex to one code, and nothing after
  the lexer learns there was more than one
- **chaining** is a loop, and costs one local however long the chain

The only genuine combination work left is **M13's position**, and that is one more
field on the state the loop already carries.
