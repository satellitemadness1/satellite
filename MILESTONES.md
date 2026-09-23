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

**THE GTK+ WINDOW IS IN ITS OWN FILE: SATELLITE_WINDOW.md** (2026-09-19/20),
`WIN-1` to `WIN-10`, the same way the infinity has SATELLITE_INFINITY.md. It is
kept apart for two reasons: a GUI is a large enough subject, and this file's
numbering already carries two M34s and two M35s, so a new M-number would be
ambiguous on sight.

**WIN-3 IS BUILT: a window appears when the window syntax is called
(2026-09-20).** `satellite.variable.window`, `satellite.window.new(title, width,
height)`, `.button(text)`, `.append`, `.close`, `.focus`, `.title`, `.ok` —
`examples/window.satl` is the author's own two lines and it draws. Read
SATELLITE_WINDOW.md **Part 2a** first: it answers WIN-6, settles WIN-4, and
corrects this paragraph's own next sentence.

**WIN-1 was called "the blocker" and is not one.** It blocks shipping to a
machine with **no GTK** — without it a static satl SIGSEGVs inside `gtk_init()`
— and blocks nothing on a machine that has GTK installed, which is where the
language work happens. Still owed, no longer first.

**AND WHAT THE WINDOW COSTS IS IN GTK_AND_NO_DEPENDENCIES.md** (2026-09-20),
`DEP-1` to `DEP-9`. The author asked for it as its own file: *"we need no
dependencies, if you need something, include a copy of it so it exists inside of
this project folder"*. It holds the seven libraries satl still needs (eight until 2026-09-22) and why each,
the 12 GB size ceiling he set, the measurement that shows uninstalling the system
GTK would not work, and the vendoring a fresh clone needs to build at all.
**DEP-1 is next.**

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

- **D0.5.1** where 004 installs -- **RULED 2026-09-22:** *"yes let's install 004
  to ~/.satl with every make"*. A bare `make` runs install.sh
  (make_support/080-install.mk); 003's satl there is kept as satl.bak-<date>, its
  satl-term is left alone. Still the author's: whether 004's window keeps
  `org.satellite.terminal` -- no launcher is installed until then.
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
Written for: the next Claude session, pasted in after /clear.

Everything below is also in the project-state memory, so nothing is lost if the note is skipped.


Continue satellite 004 work. Repo /home/madness/code/cxx/satellite, branch
milestones-install-and-no-console-handover, NOT pushed, tree clean at 741a017.
Read memory satellite-project-state.md, section "2026-09-17", and its
"NEXT SESSION, IN ORDER" list before anything.

COMMITTED LAST SESSION (check.sh 92/92 at 741a017):
- be4869b  A minus no longer hides a missing % or b: `p = -50` is
  "ERROR: expected -50%" before anything runs (it used to print first, then fail).
- ef68855  build_number.py refuses config rows -1ULL, -4u and
  -9223372036854775808, which C++ read as positive. make_words.py now refuses a
  words_004.tsv row whose numbers sit under the wrong parent, an empty or padded
  path, a word under a () shape, non-numbers, and a bare shape not at 0.
  words/check_make_words.py (14 cases, real script + 003's real satl) runs in check.sh.
- 741a017  A binary keeps its sign. The author: "give it a different number and
  keep a sign with all of these things a satellite.variable.bool with percentages
  and with infinities keep satellite.variable.bool with them".
  -b0101 is a binary: shows -b0101, worth -5, width kept, .bin -0101.
  `= -1010` is "ERROR: expected -b1010". The sign is the bool inside the
  satellite_number (binary.bits, percentage.scaled), read by negative().
  -b0000 is b0000. Infinity's sign is recorded in MILESTONES M11; M11 is unbuilt.
  My reading of "give it a different number" (a negative binary is a different
  value, not a new machine code) is not confirmed by the author.

REFUTED BY SKEPTICS, NOT FIXED: an insert in words_004.tsv moves later codes (the
word-list digest is the guard); the arguments.infinity refusal wording;
bool/char/nullptr config rows.

STEP 1: READ THE WIDE-STRINGS PATCH REVIEW (workflow wf_4de53474-c44):
  ~/.claude/projects/-home-madness-code-cxx-satellite/368b2a06-ff3b-4eed-8dae-663db741fb8f/subagents/workflows/wf_4de53474-c44/journal.jsonl
  Three lenses (bytecode, string-contract, tests-and-claims), one skeptic per
  finding. At /clear, 5 of 8 agents had finished; tests-and-claims and two bytecode
  skeptics were still running. Re-run any lens with no result. Fold every
  real=true fix and its regression test into the patch, in a scratch copy.
  Patch: memory/wide_strings_and_40000.patch (patch -p0 from the repo top;
  dry-run passes at 741a017).

STEP 2: RACE BEFORE/AFTER, ON A QUIET MACHINE ONLY. Check uptime first. The
  author's own ~/.satl/satl --repl (from satl-term, in ~/code/satl/the_combine)
  was using 20 cores and 29 GB. Leave it alone.
  The harness is saved as memory/string_race_abba.sh.txt and string_race_parse.py.txt.
  1. Make two scratch copies: git archive HEAD into before/ and after/.
  2. Apply the patch in after/.
  3. In each copy: make -j4 build/string_race.
  4. Put the two .txt files beside the copies as run_race.sh and parse_race.py.
  5. Run ROUNDS=3 MB=16 ./run_race.sh, then python3 parse_race.py.
  The (a) contestants are the same code in both binaries: they are the noise floor.
  Text 3 (emoji) is refused by the plain reference decoder in both, so only texts
  1 and 2 race.

STEP 3: apply the patch, make, ./check.sh,
  make build/string16_cases build/string_table_check &&
  python3 satellite/satellite_variable_string/check_strings16.py, then commit.
  Update satellite_string.hpp's comment and PROGRESS if speed changed.

QUESTIONS FOR THE AUTHOR, NEW FROM THE REVIEW (skeptics: design, not bugs):
- percentage * number answers a number, but percentage / number answers a
  percentage, so `50% / 4 * 4` is refused (24);
- percentage / percentage has no scenario;
- the `50% - 5` hint suggests `5 - 50%`, a different quantity;
- how infinity's sign meets its multiplier's sign.
Older open questions are listed in the memory file.

RULES THAT BIT:
- One make at a time. Check that no make is running, in its own tool call.
  Use -j4 when load is high. Every tree make raises the build number; that's expected.
- Agents that build or mutate work in their own git-archive copy, never the checkout.
- Run the interpreter as build/satellite-004 file.satl < /dev/null.
  Run 003's satl with a pipe on stdin, never /dev/null.
- python3 is PyPy.
- Never run thread stress tests or change the threads rows.
- Always commit finished work. Record rulings in MILESTONES.md and PROGRESS.md,
  quoting the author. Don't edit PLAN.md or DESIGN.md without the author's say-so.
- Reversible choices: make them, write the reason at the seam, name them in the summary.
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

**GROWN INTO `SATELLITE_FILE_OPERATIONS.md` (2026-09-18), AND FO-1 TO FO-4 ARE BUILT AND COMMITTED THE SAME DAY (`1547deb`)** (its Part 7; PROGRESS §1). The author's brief
for the storyline generator: *"a text file is a list of lines"*, the file words
spelled like `satellite.container.list`'s, and milestones FO-1 to FO-10. Its R9
answers the three `seek` overloads above. FO-1 (a method call as a statement)
comes first and is not only for files.

**WHAT IS LEFT OF M15 IS FO-5 TO FO-10**, and Part 8 of that file lists them in
order. The one that matters most is **FO-6, the race**: two runs of a
self-editing program can both read 7 and both write 8, because there is no file
lock yet — the only open item likely to bite a real program rather than a test.

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

## M20.H satellite.statement.switch

switch syntax will be...

satellite.statement.switch(something)
{
  satellite.statement.case(piece_of_something)
  {
    // runs this code
  }
}

so for example, if the switch is a string, the piece can be anything: for the switch something "saiodjghmhwerkuudfk" a case can be any piece of that string, so it can be "od" as the case,

what else should the interpreter provide for "switch"? What do we do with a number, if it's a number, then the switch has to accept that number as a value, and code would be built in between the cases that runs no matter what, so you can have:

my_int = 29834783
satellite.statement.switch(my_int)
{
  my_int = 9
  satellite.statement.case(9)
  {
    // THEN this will run
  }
  satellite.statement.case(29834783)
  {
    // this will not run,
  }
}

switch will also have satellite.statement.finally(optional_condition) if no condition is given, then the condition is true... 

# M20.G satellite.statement.finally

finally is for 
satellite.statement.if
satellite.statement.while
satellite.statement.for

## M20.E satellite.statement.break

break is part of if, for, while and exits the loop

## M20.F satellite.statement.continue

continue is part of if, for and while and jumps out of the current statement correct me if that is wrong?

## M20.D satellite.statement.if
satellite.statement.if is just (condition) { code }

## M20.C satellite.history arguments
satellite.history relies on two variables from arguments...

arguments.history is a satellite.variable.bool, when set to true, it records type name and value of every object, and writes it to disk, then satellite.history(object_name) will be the way to access the disk...

arguments.history_path = "/dir/" this must be a directory, and is an error when you try to set it to a particular file... in this directory, we will be saving name_of_satl_file.history files with the extension .number.history for each time that file is ran, now the satellite.history extension is going to have to check if turned on, whether there is at least... 32 gigabytes? Free in the location of the directory, otherwise the interpreter must set arguments.history to false. So the entire history system is hidden behind something: a single check of whether arguments.history is set to true or false, if set to false, then none of the satellite.history stuff has to be accessed, somehow we have to program this, because this adds alot to the interpreter when turned on, so we need this way of turning all of it off, so it doesn't slow us down as much...

##M20.B -- satellite.history
before satellite.history is created, we need the arguments variable to be created...

satellite.history if it's arguments are set to true, records the values of everything onto disk -- type, name and value of every object, and satellite.history is similar to satellite.library, except satellite.history is also a record of past values, and even past types of objects, written to disk. This is very easy to program, it just has alot of requirements, and it adds additional lines of code to every action taken by the satellite interpreter -- especially when turned on!

## M20.A -- satellite.statement.for
satellite.statement.for begins with a place to declare a variable,
then has a place to declare a condition that evaluates to true or
to false, then it has an optionally declare increment or decrement
a number, and we have to accept ++, -- and number + number here,
in a form that is not consistent with other parts of the language,
so the for loop is the only place where this exists, and it will
look like this:

satellite.statement.for(satellite.variable.number my_int = 0; my_int < 9; my_int + 1)
{
  // the loop code
  // but my_int + number it's a statement with a single operand,
  // we must take any math operation here and then add the declared number in the beginning of the statement, so in this example we add "my_int = " to the final block, so here we take as valid input my_int + number, my_int - number, my_int / number, my_int * number, my_int ** number(power), my_int % number, and in every circumstance, we are adding "my_int = " to the code, so we always add the declared number from the beginning of the for loop, and you must declare a number here and then that number exists in 2 places: it exists while the for loop is running, then it exists under a new namespace we can call it satellite.history and we can have satellite.history.live and satellite.history.dead for every single variable: we need to save every objects LAST type, every objects name, and every objects value under satellite.history, it will become something similar to satellite.library, so now every object is getting registered with satellite.library and satellite.history, so we will have to build satellite.history and this opens a new door! We can spin another thread, and have it listen for input, while a worker thread records onto disk the values for object... this feature will be turned off by default, but if someone turns it on, it saves the values in a .json like format onto disk, so we
}

**BUILT 2026-09-17**, everything in this entry except satellite.history. Your own
line runs:

    satellite.statement.for(satellite.variable.number my_int = 0; my_int < 3; my_int + 1)

**The third part is the interesting one, and it is built as written.** It carries
no `=` because the loop's own name goes in front of whatever is there --
*"in every circumstance, we are adding `my_int = ` to the code"* -- so `my_int + 1`
IS `my_int = my_int + 1`. It is worked out by the ordinary evaluator and the
answer is given to the number, which is why `* 2`, `- 1`, `^ 2` and `% 7` all
followed without a line of code each: the evaluator does not care which operator
it is. `++` and `--` work too, and **neither is a token**: the lexer already
writes `i++` as the name and two TOUCHING pluses, and a touching sign is not an
operation anywhere in satellite, so the spelling could be read inside this one
bracket without giving it a meaning outside it -- which is this entry's own
*"a form that is not consistent with other parts of the language, so the for loop
is the only place where this exists"*, kept literally. No registry row was minted.

**THE STEP IS EXACTLY ONE OF THREE THINGS**, which is this entry's own list and
not a rule invented here: empty, or `<name>++` / `<name>--`, or `<name>` and one
of `+ - * / % ^` spaced, and then an expression. Anything else is refused BY THE
CHECKER. That rule was written after a fresh reader found what the looser version
did: **`--i` ran forever printing 0**, nine million lines in five seconds, saying
nothing -- it is double unary minus, so the step was `i = i`. A step of just `i`
did the same. `i * * 2`, `i++ + 1`, `i & 1`, `i^^` each printed one turn of the
loop and then stopped. The step is the only part of a for that runs AFTER the
body, so a step the walker cannot use is a loop that half-runs; it is now the
part most tightly checked. What is still a RUN-time refusal is `i + 1 & 2`, where
the step begins correctly and stops being readable later -- the same refusal
`while(n < 3 & 1)` gets, in the same place.

**`**` IS POWER, IN THE STEP AND EVERYWHERE -- SINCE INF-1** (SATELLITE_INFINITY.md,
2026-09-18). This entry lists `my_int ** number(power)`, and on 2026-09-16 you ruled
that power is `^`; the step then refused `i ** 2` by name and asked whether `**`
should be accepted. The infinity answered it: you write `**` ("power ** infinity",
`my_number ** my_number ** my_number`), so a SPACED `**` now lexes to power_token
itself -- `^`'s ruling stands, and `**` is its second spelling, in a step, a
condition and every expression. Only a TOUCHING `i**2` is refused by name: *"power is
written with a space on both sides -- write i ** ... or i ^ ..."*.

**The number exists while the loop is running, and no longer.** *"it exists while
the for loop is running, then it exists under... satellite.history"*. The first
half is what exists: the name goes into the enclosing body's own table before the
first turn and is erased after the last. The second half is M20.B and is not
built, so the number is simply gone -- and `i` written after the loop is refused
by the CHECKER, before the loop has printed anything. Two loops that do not
overlap may both call their number `i`; two that do overlap are "declared twice",
which is the same rule seen from the other side.

**A for is a while with two more parts**, the same economy `if` was: one
`evaluate_expression`, one `is_bool()` demand, one `run_statements` on the body
sharing this body's variables. Nothing is allocated per turn but the step's
answer, and the step's SHAPE is read once per loop rather than once per turn.

**What the checker refuses before anything runs:** a missing semicolon, a first
part that is not `satellite.variable.number <name> = <value>`, an empty condition,
no body, every step that is not one of the three shapes above, and the number used
after the loop. **What is refused at RUN time**, exactly as `if(5)` is, because the
checker does not evaluate: a condition that is not a bool, and a first part or a
step that does not answer a number.

Built: `satellite/bytecode/program_walk.cpp` (`for_header`, `for_step_moves_by`,
`run_for`, `run_for_step`), `program_walk.hpp`, `program_check.cpp`,
`tests/for_loop.satl` and fifteen refusal fixtures, 33 rows in check.sh
(223 passed). Reviewed by a fresh reader (~144k tokens): the step rule above is
its find; what it could NOT break is listed in PROGRESS.md.

**FOR YOU, ON THIS ONE -- two of these are bugs, and neither is mine to rule on:**

1. **`satellite.return` INSIDE A LOOP DOES NOT END THE CAPSULE. A real bug, and
   it is `while`'s too, not new.** `satellite.return(0)` in a for body printed
   0 1 2 3 4 and then the line after the loop; the same program with a `while`
   does the same. `run_statements` answers `success` for a return, and `success`
   is not `stops_the_program`, so the loop goes round again. I did not fix it
   because the fix decides two things that are yours: whether return leaves the
   whole capsule or only the loop (M20.E says `break` is the word that exits a
   loop, which reads as: return leaves the capsule), and what its ARGUMENT means
   -- `satellite.returns(TYPE)` is a different word, and `return(0)`'s 0 has no
   meaning written down anywhere yet.

2. **8,000 nested `for`s SEGFAULT (exit 139, core dumped).** 6,500 is fine. `if`
   survives 8,000 at the same depth and dies later, because `run_for`'s C++ frame
   is the biggest of the three. It is stack exhaustion, proven: `ulimit -s 65536`
   makes the 8,000 case print and exit 0. **This is the thing "the language has no
   limits" is about, and a depth bound is not the fix** -- the fix is the walker
   keeping its own stack instead of recursing through C++, which is the same
   decision as item 3 and wants to be made once.

3. `program_walk.cpp` is now **939 lines**, against your 300. The three statement
   runners (`run_while`, `run_if`, `run_for`) and `run_statements` call each
   other, so splitting them out means putting `run_statements` in a header -- a
   real change to what that header promises, not a move. Say whether to do it.

4. `satellite.statement.for(...; ...; )` with an empty step is accepted -- this
   entry marks only the third part "optionally". Is an empty CONDITION
   (`for(number i = 0; ; i++)`, C's forever loop) wanted too? It is refused today.

5. Smaller, both shared with `while`: `i ++` with spaces is taken as `i++` (the
   lexer makes both signs touching whatever the gap), and junk after the header's
   own `)` is ignored -- `for(...; i++))` runs clean.

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

## M34 -- optimizing the --rebuild command

for this milestone we have to tighten up the --rebuild command,
and to do that, we need to build some system, let me think about
how we can quickly turn on and off all of the features, the
general idea is already built into the interpreter -- --rebuild
builds a single long 10101111 binary number into config.ini so
it doesn't have to work on the defaults... and let's build a
default variable as a std::string as the defaults, and to optimize
it we can do this: 

std::string default_variables = "110010101111";

if (default_variables[4] == "1")
{
  // run this code
} else {
  // run this code
}

and we'll build the if statements into a hierarchy, so that
features that rely on other features can come after and before
other features that we turn on/off, and this shall be our
optimized strategy, I know what you're thinking, but the only
alternative is to do a fast path, so you decide claude, which is
going to be faster? A bunch of fast paths in the form of a switch
statement, or an if statement hierarchy,?

here is an example switch fast path setup:

switch (default_variables)
{
  case ("11111100000010101")
  {
    // code for that particular case
  }

  case ("11101010101011111")
  {
    // code for that particular case,
  }

  case ("101010101010101010")
  {
    // code for that case,
  }
}

so we have to write a SWITCH hierarchy actually! We shall combine the best features of both strategies to make a HYPER OPTIMIZED

### Answered 2026-09-18 — switch, and here are the numbers

The milestone asks: *"so you decide claude, which is going to be faster? A bunch
of fast paths in the form of a switch statement, or an if statement hierarchy?"*

**Measured rather than argued.** clang 24 -O2, 2,000,000,000 statements, eight
features off, two runs agreeing to the fourth digit:

| how the features are gated | ns a statement | over the floor |
|---|---|---|
| **no gates at all** (the floor) | **0.216** | — |
| eight separate `bool`s | 1.81 | +1.59 |
| one bitmask, eight bit tests | 1.84 | +1.62 |
| nested switch **per statement** | 0.708 | +0.49 |
| one bitmask, **one** test per statement | 0.72 | +0.50 |
| **nested switch WRAPPING the loop** | **0.216** | **+0.000** |

**So: switch, and the milestone is right — but the win is not the switch, it is
WHERE the switch is.** An `if` hierarchy and a `switch` hierarchy measure the
same inside the loop (1.81 against 1.84; both are compare-and-branch chains).
Neither is the answer. The answer is the last row: **the hierarchy WRAPS the
interpreter** so the choice is taken once, and the loop it chose has no test in
it at all. In the real interpreter, a 2,000,000-turn satellite loop through four
different leaves came in at 2.272, 2.276, 2.269 and 2.288 seconds — within 1%.

**Built 2026-09-18** as `satellite/config/feature_switch.hpp`, 230 lines — inside
the milestone's own estimate of *"100 lines ... or maybe 200 300 400"*.
SATELLITE_ERROR.md Part 14 has the whole record.

### Two things in the sketch above that will not compile

Written down because they are easy to fix and hard to see:

1. **`switch (default_variables)` cannot switch on a `std::string`.** C++ switches
   on integers and enums only. This is not a small detail — it is the reason the
   register is a `std::uint64_t` and the `case` labels are masks. The satellite
   binary in config.ini is what a PERSON reads; the register the switch tests is
   the integer behind it, and `written()` / `read_written()` in
   `feature_register.hpp` are the two conversions between them.
2. **`if (default_variables[4] == "1")` compares a `char` to a `const char *`.**
   It needs `== '1'`, single quotes. And a string index is a memory load plus a
   compare where a bit test is one instruction on a register already in flight —
   which is the other half of why the register is an integer and not a string.

### What IS in `--rebuild` today

Fourteen feature bits, composed from the named keys in config.ini into one
satellite binary, with a table saying which are on and **which are turned on and
not built yet**:

`access` `history` `frames` `statements` `trace` `coverage` `word_counts`
`capsule_timing` `watchpoints` `memory_accounting` `thread_state`
`report_on_success` `report_file` `replay`

Of those, **two actually do something**: `access` (reads and writes, and lasts)
and `word_counts` (per-word call counts, the first bit anything reads —
`satellite/bytecode/word_counts.hpp`). The other twelve are numbered and honest
about it.

### What is NOT in `--rebuild` — the list this milestone is for

**1. Twelve features have a bit and no behaviour.** Everything above except
`access` and `word_counts`. Each one's milestone is in SATELLITE_ERROR.md Parts 8
and 12. `--rebuild` names them when they are turned on, so this is visible rather
than silent.

**2. Three booleans that could be bits and are not.** These are real true/false
settings the interpreter already reads from somewhere else, and every one of them
is a candidate for the register:

| setting | where it lives now | why it is a candidate |
|---|---|---|
| `arguments.sate` | `satellite_config.hpp`, compiled in | gates whether the 16-bit file is written |
| `arguments.startup_display` | `satellite_config.hpp`, compiled in | gates the start-up block |
| `arguments.debug_mode` | the command line, `--debug` | gates every state line |

**`--debug` is the interesting one**, because it is the only one a person changes
per run rather than per machine — so folding it into a register composed once
would take that away. It may want to stay a flag, or want both.

**3. Ten numbers that CANNOT be bits, and this is the real limit of the design.**
A register bit is one yes-or-no. These are values:

`arguments.magic` (5) · `arguments.version` · `arguments.revision` ·
`arguments.build` · `arguments.object_bytes_max` (34359738368) ·
`arguments.threads_max` (1000000) · `arguments.threads_startup` (1024) ·
`arguments.file_size_max_bytes` (549755813888) · `arguments.infinity` (4096) ·
`arguments.infinity_display` (32)

**So `--rebuild` composes the FLAGS and cannot compose the NUMBERS**, and a
milestone that says "pack everything into --rebuild" has to decide what that
means for a number. Three ways, each one line to reverse:

- **leave them where they are** — they are read once at start-up and never in a
  loop, so they cost nothing and the register buys them nothing;
- **compose a second value** — `numbers = 5,4,4,115,...` beside `features`, one
  parse instead of ten;
- **bit-pack the ones with small ranges** — `magic` and `infinity_display` fit in
  a few bits each, and nothing else does.

**The first is almost certainly right**, and the reason is the measurement above:
the register earned its place because it is tested in a LOOP. A number read once
before the program starts is not on any hot path, so moving it changes nothing a
clock can see.

**4. The machine facts are not settings and do not belong.**
`arguments.memory.total`, `arguments.machine.cores`, `arguments.machine.threads`,
`arguments.disk.free`, `arguments.username`, `arguments.system.*` — these are
READ from the machine every run and cannot be composed, because composing them
would mean caching a fact that changes. A register of them would be a register
that goes stale, which is S0723 with no way to fix it.

**5. `satl --config`'s measured thread count is still owed.**
SATELLITE_ARGUMENTS Phase 8 (C1–C8): the 9.6-second probe that writes what the
machine can really do. It is the sibling command to this one — both run once per
machine, both may be slow, both write config.ini — and **whether they are one
command or two is SATELLITE_ERROR red note 13**, still open.

**6. The deferred values.** SATELLITE_ARGUMENTS Phase C and D (B7–B15):
`arguments.memory.free`/`.used`/`.swap.*`, `arguments.directory`,
`arguments.directory.history`, `arguments.system.stack`,
`arguments.memory.system.reserved`. None is a flag, so none is a register bit —
they are values, and they join the list in 3 and 4 above.

### Two number collisions to fix

**M34 and M35 are each used twice**, found 2026-09-18 while adding the notes
above:

| number | this one | and also |
|---|---|---|
| **M34** | optimizing the `--rebuild` command | *the names a session has used, `satellite.access` and `satellite.help`* |
| **M35** | Optimizing The Interpreter | *a spacesuit takes a supertype, and `satellite.protected` takes arguments* |

Four different milestones, two numbers. **The M34 pair is the one that matters**,
because they are genuinely related and will be worked on together: the other M34
is where the last-known store lives, and `access` — bit 0 of this milestone's
register — is its valve. A person reading "M34" in a commit message cannot tell
which is meant.

Renumbering is the author's call: the two in Part 3 came first by date, and the
two above came first in the file.


## M35 -- Optimizing The Interpreter

we are going to rebuild the interpreter as we rebuilt the features to be a switch-hierarchy of everything with pathways, so that the entire interpreter is a switch-hierarchy of different cases and different switches, this is the very fastest way I can think of to build this interpreter, and it's way faster than it will ever need to be.

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

### ANSWERED 2026-09-18 — they are TWO things, and the milestone had them as one

(the author) *"satellite.access is different from satellite.help, I want to build
two different things there, access is a little bit of info about the object, the
objects value and type only, whereas satellite.help is information on that type,
but we can combine them so access also displays whatever help has for that type,
that would work... so we'll have access as that, but then satellite.help will
still be the help system, whereas access is just info on that type, help is more
detailed info on that type"*

| | what it answers | about |
|---|---|---|
| **`satellite.access(name)`** | the **value** and the **type**. A little bit of info | the OBJECT |
| **`satellite.help(name)`** | the type's methods, and everything else known about it | the TYPE |

**One register feeds both**, which is why they were written as one milestone and
is the only thing that was right about doing so: `access` reads a name out of the
last-known store and answers its value and type; `help` takes that type and shows
what the type can do. **`access` may also print what `help` would** -- the author
allows it -- so a person who asks about a name gets the object AND the type
without asking twice.

**AND `access` IS ALREADY HALF BUILT.** `satellite.library.main.arguments.access`
-- bit 0 of the feature register, `249d748` -- is the VALVE that decides whether
the last-known store is kept at all. The store is what this milestone builds; the
switch for it already exists, lasts in config.ini, and defaults to on.

## M35 — a spacesuit takes a supertype, and `satellite.protected` takes arguments

> **SPLIT 2026-09-18 BY THE AUTHOR, AND ONLY THE FIRST HALF IS OWED.**
>
> *"Well right now I Don't think satellite.protected needs arguments just yet,
> just leave it marked down as it's own milestone and we'll just never get to
> it... but a spacesuit still has to take a supertype, that is certain."*
>
> So: **a spacesuit taking a supertype is CERTAIN and is this milestone.**
> `satellite.protected` taking arguments is **parked** -- kept written down,
> deliberately not scheduled, and explicitly not blocking the supertype.
> Two things in one heading was why they looked like one job.

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
