# satellite-004 — MILESTONES

Everything decided and **not yet built**, one milestone each. Written 2026-09-16
at the author's asking: *"let's build milestones for everything that we didn't
do"*, then swept comprehensively against PLAN.md, ERROR.md, PROGRESS.md,
`words.tsv` and `REGISTRY.satellite` so that nothing outstanding is missing.

**Read PROGRESS.md first** — it is what IS built. This file is only the debt.

**AUDITED 2026-09-17, against what the bytecode actually does.** This file was
written while PLAN's shape was still the plan -- three stored files, a parser, a
prototype runner -- and several entries described work that cannot be done because
its subject no longer exists. Each one was RUN rather than reasoned about, and the
entries now say fixed, dead or still real: M1/M2/M3 dead, M6/M7 built, M19 fixed,
M26 re-run one defect at a time, M24 narrowed, M8 and M35 cut loose from a parser
that was never written.

**The numbering continues PLAN.md's.** M0–M12 are PLAN's own and keep their
numbers and meanings; M13 up are new, most of them found while building the
object model on 2026-09-16. Where a decision is still the author's it says so
rather than guessing.

---

# Part 1 — PLAN.md's milestones, with what is actually true now

## M0.5 — port the build, the installer and satl-term — **BUILT 2026-09-17**

Built in `71f7836`, `5fc0c7d` and `a4ac456`; PROGRESS.md's M0.5 row says what it is
and how it is checked. `make` builds `build/satl`, its libraries and
`build/satl-term`; `make test` runs check.sh, `satellite_enterprise/check_install.sh`
and the string checks. What is still the author's:

- **D0.5.1** where 004 installs, and whether its window keeps
  `org.satellite.terminal`. Until then `install.sh --root <folder>` only.
- **D0.5.2** taken as PLAN recommends: a code below 0 or above 254 exits 255,
  and 255 is never given to a code.
- **Readings to confirm:** `satl --debug` alone is bare `satl`; `--debug` twice is
  `--debug`; `build/satellite-004` stays a link to `build/satl` so the `satl`
  alias keeps working.
- **Found by the review, left to the author:** `shown()` does not escape a
  backslash, so a name holding the text `\x1b` prints like one holding ESC; a
  satl-term tab running a 003 satl names 003's exit status with 004's
  machine-code names; satl holds its output until a run ends (DESIGN §8), so a
  satl-term tab shows nothing from a long program and loses it on Ctrl-C.

## M0.6 / M0.7 — the prompt, in satl and then in satl-term

`satellite.directory.change` and `.list` come with M0.6. M0.7 is the same prompt
inside satl-term. M0.5 is built, so both can start.

**M0.6's terminal layer is built (`28364f2`):** `satellite/prompt/`, 003's line
editor ported with PLAN's changes and reviewed; PROGRESS.md's row says what it is
and how it is checked. It is not in satl yet. The session that runs a line, the
directory libraries and the table are not started: by PLAN's rule they wait on
the author's pseudocode (D0.6.4). What the port chose, each one line to reverse:
- a pasted block's lines run one after another, as if typed, and its unfinished
  last line waits at the prompt (rather than all of it held until Enter);
- a piped line is split at `\n` only and keeps a `\r`, for the statement reader to judge;
- keys typed while a line runs are read and dropped (D0.6.5 as PLAN recommends), a
  paste among them whole; when the reader still holds keys typed before the run,
  nothing is dropped;
- a resize assumes the terminal reflows the line (VTE, Konsole, kitty; xterm does not);
- the prompt's first draw clears the row it starts on, so the session ends its own
  output with a newline before a prompt.

**AND THE SESSION IS BUILT (`29319d8`):** `satl --repl` runs a typed line out of
the bytecode -- tokenised by the lexer a `.satl` uses, judged by the checker a
capsule's body uses, walked by `run_statements`. `satellite.directory.change(d)`,
`list()` and `list(d)` are built, and a bare listing line draws the table.
PROGRESS.md's row says what it is and how it is checked. What M0.6 still owes:
`list()` inside a PROGRAM answers `not_built_yet` until there is a list type
(M14); the listing's order is the bytes' where 003's was its character table's
(M21, and a library cannot reach the language's string yet); the 100,000-entry
race has not been run; and M0.7 is the same prompt inside satl-term.

What the session chose, each one line to reverse:
- from a pipe the status is the first failing line's code, and at a terminal
  `exit` is 0 -- a person has already seen every refusal (D0.6.2's other half);
- the table's `type` column is what the system knows (dir, file, link, fifo, sock,
  dev) and never reads a file to guess (D0.6.7 answered: 003 read up to 64 KB of
  every file to say "text");
- `current()` `1 18 2` and `exists(d)` `1 18 3` are NOT built (D0.6.6): the author
  named `change` and `list`.

## ~~M1, M2, M3 — the `.satc`, the `.satb`, the `.sati`~~ — **DEAD, and the author said so**

(the author, 2026-09-16, D0.1) *"we threw away satc and satb in favor of all
16-bit"*. The bytecode registry IS the numbered program, one code a word, and a
string is 16-bit codes inline and counted. There is no `.satc`, no `.satb` and no
`.sati` to build, and D3.1 died with them.

**What survives is work, not files:** combine's marks, which live in the bytecode
(`batch_start`/`batch_end`/`wait`/`batch_size` are reserved rows already) and
belong to M12; and the ONE file that does exist, `.sate`, which M31 makes
runnable.

## M1.5 / M2.5 / M3.5 / M3.6 — the converters — **now ONE converter**

With the three files dead, there are not four converters to write: there is one,
from the registry's 16-bit codes back to readable `.satl`. Half of it exists --
`bytecode_text()` already prints a row as sixteen binary digits a code, which is
REGISTRY.satellite's own first column and the reason a row is `std::bitset<16>`.
M3.6 died with `.sati`.

**Still blocked by M24** only for what it may RECORD: a converter that prints a
stored program has to know whether that program is a transcription to re-lex or a
code stream.

Already known: comments do not round-trip. `//` is discarded in the lexer
(003 DESIGN §5.6), so a comment is a marker with no text.

## M5 — names and `satellite.log`

Not started. The testing method in PROGRESS §2 depends on it: *"done when a run
writes no `[entry]` to satellite.log"*.

## ~~M6 / M7 — the parser and the runtime~~ — **BUILT, and there is no parser**

There is no `.satb` to run and no recursive-descent parser to port: the registry
is lexed directly, `program_check.cpp` judges every statement and
`program_walk.cpp` walks it, six shapes and their fast paths. M0.6 (2026-09-17)
put a TYPED line through the same three -- lexer, checker, walker -- which is the
proof that there is one runner and not two.

**NOTHING ELSE MAY DEPEND ON "M6's parser work", and entries that did have been
corrected** (M8, M35): what those milestones actually need is one more statement
shape in the checker and the walker. What is still owed under this heading is only
the GRAMMAR that has no shape yet -- blocks that are not `while` bodies, and the
declarations M8 describes.

## M8 — user-defined classes (spacesuits) — **the machine is built, the grammar is not**

`satelliteSpacesuit` and `satelliteUserDefinedObject` exist and compile.
**Nothing can declare one yet, and the seam is exact** (checked 2026-09-17):
`satellite.spacesuit my_class` ALREADY lexes as a word followed by a name -- the
declaration shape -- and is refused in ONE place, `check_statement`'s declaration
branch: *"satellite.spacesuit my_class is a declaration, and only
satellite.variable.number, .string, .binary and .percentage are built yet"*. The
grammar is that branch, its twin in `run_assignment`, and a sibling of
`capsules_in` scanning for word `1 10`. No parser is in the way.

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

- **An infinity keeps a sign, as a bool** (the author, 2026-09-17: *"give it a different number and keep a sign with all of these things a satellite.variable.bool with percentages and with infinities keep satellite.variable.bool with them"*) --
  so there is a negative infinity, and its sign is held with it the way a binary's
  and a percentage's are. Recorded here with M11 unbuilt; how the sign and the
  multiplier's own sign meet (`-infinity` against `infinityx-1`) is a question for
  the author when M11 is built.

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

**Answered and built: a string minus a string** (the author, 2026-09-17): *"minus
takes away the smallest string"*, `"dfksjghjfff" - "fff"` is `dfksjghj`; asked which
copy goes when there are two, *"first occurrence"*; and *"minus - fff"*, the string
after the minus is the one taken away. So `"abcab" - "ab"` is `cab` and
`"fff" - "dfksjghjfff"` is `fff`. Nothing to take away is an answer, not a refusal.
Until this ruling `-` on two strings was refused (27): `str_minus_str.cpp` had been
written, taking away EVERY occurrence, and nothing dispatched to it.

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

## ~~M19 — `n = 1 & 2` answers 1, silently~~ — **FIXED**

It was the worst item in this file and it is done: `read_to_the_end`
(`program_walk.cpp`) makes a variable's value and a loop's bound demand the whole
expression, as `call_word` already did. Run today, `satellite.variable.number n =
1 & 2` answers *"could not be read to the end -- it stops at something with no
meaning there yet (& | << >> !! are undecided)"* and machine code 13, where it
used to store 1 and say nothing.

**What is left is not a defect but a ruling:** what `&` `|` `^` `<<` `>>` `!!`
`~` are to MEAN. They keep their registry rows and their QUESTION marks until the
author says (M24's list).

## M20 — satellite_float, and hex as its own type

**Binary is built** (2026-09-16): `satellite.variable.binary` `1 6 5` is
`satellite_binary_number`, arm 7 of `satelliteObject`, width kept. **Owed on it:**
003 DESIGN 8.5's DECIDED AND UNBUILT rulings -- `+` on bit runs answers a bit run,
the width grows to fit, the left operand's type wins, `!!` joins, indexing counts
from the right -- are not built; today arithmetic reads a binary by worth and
answers a number. That is the author's to schedule, not to be decided in passing.

**A binary keeps a sign** (the author, 2026-09-17: *"give it a different number and keep a sign with all of these things a satellite.variable.bool with percentages and with infinities keep satellite.variable.bool with them"*). `-b0101` is a binary --
displayed `-b0101`, worth -5, width 4, `.bin` `-0101`, not equal to `b0101` -- where
the commit before (`be4869b`) had refused a minus in a binary as having no spelling.
`= -1010` is `ERROR: expected -b1010`. The sign is the bool the bits'
`satellite_number` already carries, so there is one bool; `-b0000` is `b0000`.

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

**PROGRESS §6, open, the author's** -- and NARROWER than it was. There is one
stored file now, `.sate`, and it holds the registry's codes, so the code-stream
half is already real in what is written. What is still undecided is whether a
stored program may be RE-LEXED as a transcription, which is what a converter needs
to know before it records anything. M31 (running a `.sate`) settles it in
practice.

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

ERROR.md §1 was written against the prototype runner, which is retired. **Every
one was re-run against the bytecode path on 2026-09-17**, and the list is now:

**Fixed, and struck:**
- ~~#7 exit codes cut to 8 bits~~ — `exit_status_of` (M0.5), checked by
  `build/exit_status_cases`
- ~~#13, the byte-order mark half~~ — a file starting with a BOM runs
- ~~#14 whitespace inside brackets~~ — `display( 42 )` prints 42

**Still real, one with a new face:**
- **#13, the CR-only half** — and the symptom MOVED: a file whose lines end in CR
  alone is now read as ONE line, so `satellite.return` looks like a call and the
  answer is 14 *"has no library built for it yet"*. Misleading in a new way.
- **#10 every `*.so` in the folder is loaded** and **#11 no ABI check at load**
- **#17 a library's file name is never checked against the numbers it describes**

**Half done, and the rest is a ruling:**
- **#16 unknown escapes** — `"a\qb"` no longer displays `aqb`: the backslash is
  kept, so nothing is silently dropped. Whether an unknown escape should be
  REFUSED (PLAN M0.6 says so) is the author's.

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
| ~~**D3.1**~~ | ~~32 bits a character everywhere, or only in the `.sati`?~~ **ANSWERED 2026-09-16:** *"32-bits only when we use the number 40000 as a 16-bit code"* -- everything is 16 bits, and a code of 40000 (`wide_token`) says the next two codes are one 32-bit integer. **Done 2026-09-17:** the lexer writes a character above U+FFFF as `wide_token` and two codes (`bytecode_registry.cpp` `character_codes`, read back by `text_at`), `wide_run_32_token` is retired, and `satellite_string` holds it the same way | — |
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

---

# Part 3 — asked for on 2026-09-17, after the prompt

## M34 — the names a session has used, `satellite.access` and `satellite.help`

(the author, 2026-09-17) *"there is satellite.access(object_name) so before we do
this, we need to keep a list of the names of every object that we use and their
type, this is used by satellite.help(object_name) to bring up the methods on that
type, this logic we are keeping from 003. the satellite.help(object_name) code
doesn't necessarily need to be fast, but it needs to be coded in a way that we can
work on it, so anytime we add a type, then that type's methods need to be
registered within some directory structure"*

**The register comes first, and it is one list: every object name in use and the
type it is.** `satellite.access(object_name)` reaches an object by its name;
`satellite.help(object_name)` reads the same register, finds the object's TYPE and
shows that type's methods. 003's logic is kept.

**A type's methods are registered where the type is added,** so adding a type is
adding its methods -- never a second list somewhere else to keep in step. Help may
be slow; it must be easy to work on.

**What 004 has already, and what is missing.** `words/words.tsv` numbers every
method of every type, and `satellite-numbers/<the word>/` is the directory
structure the author asks for -- one folder a method, named as the word is
written. What does not exist is the run-time register: the walker's
`VariableTable` is made per body and holds a name's value and the word that
declared it (`value.hpp`), and nothing outlives the line or the capsule. The
milestone is that register, `access`, and `help` reading it.

**Decisions:** whether the register is per session, per capsule or per program;
whether `help` with no argument lists the types; what `access` answers for a name
nothing declared. `satellite.help` is `1 19`, `satellite.access` is numbered in
words.tsv, and the prompt refuses `help` by name until this lands (M0.6).

## M35 — a spacesuit takes a supertype, and `satellite.protected` takes arguments

(the author, 2026-09-17) *"when a user defines a type, they should almost be able
to do this"*:

```
satellite.spacesuit my_class_name(satellite.variable.string)
{
    satellite.protected
    {
        satellite.supertype.parameter = some_object
```

*"where the word parameter would be something like "value" or "option" so then
satellite.protected(args) would take arguments where it didn't used to take
arguments"*

**Three things, and the third is the one that changes a grammar that exists.**
A spacesuit names its SUPERTYPE in its brackets (`satellite.variable.string`
above); inside `satellite.protected` a line gives the supertype's own parameter a
value (`satellite.supertype.parameter = some_object`, where `parameter` is a name
like `value` or `option`); and `satellite.protected` GAINS ARGUMENTS --
`satellite.protected(args)` -- where today it takes none.

**M8 is where the machine for this already is** (MILESTONES M8: "the machine is
built, the grammar is not"): `satelliteSpacesuit` carries a user-defined type's
fields today, and the grammar that declares one is what is owed. This milestone is
that grammar plus the supertype. **It does NOT wait on "M6's parser work"** --
there is no parser (M6/M7 above): what it waits on is the three decisions below.

**Decisions:** what `satellite.protected(args)` takes, exactly; whether a
supertype must be a built type or may be another spacesuit; what
`satellite.supertype.<name>` is when the supertype has no such parameter.
