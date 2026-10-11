# satellite-004 — MILESTONES

Checked against satl BUILD 0058 on 2026-10-09.
The full text as it stood before this update is in [history/MILESTONES.md](history/MILESTONES.md).

Everything decided and **not yet built**, one milestone each. Written 2026-09-16
at the author's asking: *"let's build milestones for everything that we didn't
do"*, then swept comprehensively against PLAN.md, ERROR.md, PROGRESS.md,
`words.tsv` and `REGISTRY.satellite` so that nothing outstanding is missing.

**Read PROGRESS.md first** — it is what IS built. This file is the debt. A milestone
that has been built keeps a short record here: what, when, and where its full record is.

- **The author's current plan is NEW_MILESTONES.md** (2026-10-06, NM-1 to NM-6). NM-2,
  `satellite.delete(name)`, and NM-2a, satellite.legal, are built; the others are not.
  This file is the older list of what is not built.
- **The window has its own file, SATELLITE_WINDOW.md** (WIN-1 to WIN-10), and what it
  costs has GTK_AND_NO_DEPENDENCIES.md (DEP-1 to DEP-9). The author asked for that one as
  its own file: *"we need no dependencies, if you need something, include a copy of it so
  it exists inside of this project folder"*. Today satl links no GTK, VTE or glib: they are
  compiled into it from `vendor/`. `ldd ~/.satl/satl` lists nine system libraries, and
  `satl --version` says the binary carries 26 other projects.
- **Audited 2026-09-17** against what the bytecode actually does: each entry was RUN
  rather than reasoned about. **Checked again on BUILD 0058, 2026-10-09**, when the
  finished milestones became short records.

**The numbering continues PLAN.md's.** M0–M12 are PLAN's own and keep their
numbers and meanings; M13 up are new, most of them found while building the
object model on 2026-09-16. Where a decision is still the author's it says so
rather than guessing. M34 and M35 are each used twice (see "Two number collisions"
in Part 3), and satl's own refusals and help text name them, so neither is renumbered
without him. **Ruled 2026-10-09:** neither is renumbered; M34 and M35 each stay used
twice (the author, answering the editors' questions: *"looks good"*).

---

# Part 1 — PLAN.md's milestones, with what is actually true now

## M0.5 — port the build, the installer and satl-term — **BUILT 2026-09-17**

Built in `71f7836`, `5fc0c7d` and `a4ac456`; the record is PROGRESS.md section 1, "PLAN
M0.5 — the build, `satl`, satl-term and the installer". Since then satl-term is gone
(2026-09-22) and every word is compiled into satl (2026-10-07). What is still the
author's:

- **D0.5.1** where 004 installs -- **RULED 2026-09-22:** *"yes let's install 004
  to ~/.satl with every make"*. On 2026-10-07 he ruled *"plain make should build the
  program"*, and that evening *"we need plain make to run make install by default, and
  plain make to ask if you want to install satellite or just build it"*. So a bare `make`
  builds with a new BUILD number every time and then ASKS: Enter, y or a minute's
  silence installs into `~/.satl`, n does not; `make install` installs and asks nothing
  (make_support/050-build.mk, 080-install.mk). 003's satl found there is kept as
  `satl.bak-<date>`. **The window half, settled 2026-09-22** when he clicked the app and
  got 003's satl-term around 004's satl ("settings is gone, and there's no line at the
  bottom"): 004's window keeps `org.satellite.terminal`, and the install writes its
  launcher with satl's absolute path (`satellite_enterprise/install_support/070-desktop.sh`).
- **D0.5.2** taken as PLAN recommends: a code below 0 or above 254 exits 255,
  and 255 is never given to a code (`machine/exit_status.hpp`).
- **Readings to confirm:** `satl --debug` alone is bare `satl`; `--debug` twice is
  `--debug`; `build/satellite-004` stays a link to `build/satl` so the `satl`
  alias keeps working. All three are still so on 0058.
- **Found by the review, left to the author:** `shown()` does not escape a
  backslash, so a name holding the text `\x1b` prints like one holding ESC (still so,
  `machine/shown.hpp`). The review's two other findings went with satl-term: a tab
  running a 003 satl named 003's exit status with 004's names, and a tab showed nothing
  from a long program because satl held its output until the run ended (on 0058 a
  displayed line reaches a file before the run ends).

## M0.6 / M0.7 — the prompt — **M0.6 BUILT 2026-09-17; M0.7 went with satl-term**

The terminal layer (`28364f2`, 003's line editor ported) and the session (`29319d8`):
`satl --repl` runs a typed line through the lexer, checker and walker a `.satl` uses, and
`satellite.directory.change(d)`, `list()` and `list(d)` answer. The record is PROGRESS.md
section 1, "PLAN M0.6, first part" and "second part". M0.7 was the same prompt inside
satl-term, which went on 2026-09-22; the prompt runs in satl's own console.

**Still owed:**
- `list()` and `list(d)` inside a PROGRAM are refused (S210), and the refusal still says
  "satellite has no list type built yet to hold them (MILESTONES M14)", though the list
  type has run since 2026-09-18 (M14).
- `current()` `1 18 2` and `exists(d)` `1 18 3` are NOT built (D0.6.6; S210 on 0058):
  the author named `change` and `list`. (`system()` `1 18 6` was built on 2026-10-01.)
- The listing's order is still the bytes' (`std::sort`, satellite-numbers/directory_words.hpp),
  where 003's was its character table's. The 100,000-entry race has not been run.

**What the port chose, each one line to reverse** (2026-09-17):
- a pasted block's lines run one after another, as if typed, and its unfinished
  last line waits at the prompt (rather than all of it held until Enter);
- a piped line is split at `\n` only and keeps a `\r`, for the statement reader to
  judge (so on 0058 a line piped in with CRLF is refused);
- keys typed while a line runs are read and dropped (D0.6.5 as PLAN recommends), a
  paste among them whole; when the reader still holds keys typed before the run,
  nothing is dropped;
- a resize assumes the terminal reflows the line (VTE, Konsole, kitty; xterm does not);
- the prompt's first draw clears the row it starts on, so the session ends its own
  output with a newline before a prompt.

**What the session chose, each one line to reverse:**
- from a pipe the status is the first failing line's code (still so on 0058), and at a
  terminal `exit` is 0 -- a person has already seen every refusal (D0.6.2's other half);
- the table's `type` column is what the system knows (dir, file, link, fifo, sock,
  dev) and never reads a file to guess (D0.6.7 answered: 003 read up to 64 KB of
  every file to say "text"). Since 2026-09-25, at the author's asking, the table does
  read every regular file whole, for the line count in its `sub` column
  (`satl/listing_counts.hpp`).

## ~~M1, M2, M3 — the `.satc`, the `.satb`, the `.sati`~~ — **DEAD, and the author said so**

(the author, 2026-09-16, D0.1) *"we threw away satc and satb in favor of all
16-bit"*. The bytecode registry IS the numbered program, one code a word, and a
string is 16-bit codes inline and counted. There is no `.satc`, no `.satb` and no
`.sati` to build, and D3.1 died with them. What survives is work, not files: combine's
marks, reserved rows in the bytecode (`batch_start`/`batch_end`/`wait`/`batch_size`)
that belong to M12. The one file that existed, `.sate`, was dropped on 2026-10-03 (M31).

## M1.5 / M2.5 / M3.5 / M3.6 — the converters — **now ONE converter, with nothing to convert yet**

One converter is left to write: from the registry's 16-bit codes back to readable
`.satl`. Part of it exists and nothing calls it: `row_as_bits()`
(`bytecode/bytecode_registry.cpp`) prints a row as sixteen binary digits a code,
REGISTRY.satellite's own first column. (This entry used to call it `bytecode_text()`; no
function of that name was ever in the source.) M3.5 died with the `.sati`, and M3.6, the
`.sate` converter in PLAN.md, with the `.sate` on 2026-10-03. Nothing stores a program
now, so the converter has nothing stored to print, and what it may record still waits on
M24's question. Already known: comments do not round-trip. `//` is discarded in the
lexer (003 DESIGN §5.6), so a comment is a marker with no text.

## M5 — names and `satellite.log` — **BUILT 2026-09-25**, `10bfb9c`

The author, 2026-09-25: *"can we do M5 and then this?"* (this = M16's string methods).
Every report satl makes goes once into `satellite.log` (`machine/satellite_log.hpp`,
DESIGN §7's `write_entry`, the author's own function), at `arguments.log_path` =
`~/.satl/satellite.log`. A name declared as both an object and a class is refused, S202.
**S020 NUMBER_TAKEN_AS_TEXT, the author's 2026-09-16 ruling built whole:** *"just convert
the number to the string ... but record the warning in satellite.log"* -- in the log
only, not on the screen. All three are still so on 0058. The record is PROGRESS.md
section 1, "`satellite.log` and names"; the fresh reader's six defects, all fixed, are in
this file's old text. The reader also found no exit status or output changed in 140
existing programs other than the two new refusals (2026-09-25). The worst of the six: a warning re-read the program file on every conversion, so 100,000 conversions at line 5010 went from 0.16 s to 4.9 s (measured 2026-09-25; fixed the same day).

**Chosen here, the author's to overrule** (each is one place):
- **every report goes in the log, not only warnings.** 003 logged warnings only;
  DESIGN §7 (clashes) and M29 (*"checking reports into satellite.log"*) read as more.
  A person with no config.ini gets an S010 entry every run.
- **the file is 003's, shared.** 003 (~/.satl-003) still writes its one-line warnings to
  ~/.satl/satellite.log, so the file holds both shapes.
- **two repeated joins on two lines are two S724 notices now,** where there was one:
  the notice has a line, and the tally's rule is that a different line is a different
  mistake.
- **a spacesuit's field and a capsule of ANOTHER spacesuit may share a name:** the rule
  follows the scope chain, not the whole file.

**What the two stores a statement cost** (measured 2026-09-25, BUILD 0049 against 0055,
both PGO + LTO + BOLT, alternating runs on a machine that was not quiet, so only the
first three rounds count): a 2,000,000-turn `while` of two assignments went from
2.674–2.680 s to 2.699–2.742 s, about 1–2.5%. `haswell_test/race_program.satl` went from
23.19–23.65 s to 22.96–22.99 s, no slower. The outputs are identical.

**Not done under this heading:** DESIGN §7's literal `object_name_vector,
class_name_vector, command_name_vector`. The capsule table's per-scope maps and each
body's `VariableTable` already are those vectors, so no second copy was made.

## ~~M6 / M7 — the parser and the runtime~~ — **BUILT, and there is no parser**

There is no `.satb` to run and no recursive-descent parser to port: the registry
is lexed directly, `program_check.cpp` judges every statement and `program_walk.cpp`
walks it -- eight shapes today (return, declaration, assignment, while, if/else, for, a
word, a capsule). M0.6 (2026-09-17) put a TYPED line through the same three -- lexer,
checker, walker -- which is the proof that there is one runner and not two. Nothing is
owed here now: the blocks and declarations it once listed are built.

## M8 — user-defined classes (spacesuits) — **BUILT 2026-09-22**, `b6b2a98`

Spacesuits as the author writes them -- `satellite.protected`, `satellite.public` and
`satellite.constructor(args) { }`, objects made by declaring them, a spacesuit inside
another's section (**new -- 003 had none**) or extending another, `eclipse(view)` -- and
`satellite.return(x)` handing x back and ending the capsule FROM ANY DEPTH. The record is
PROGRESS.md section 1, "Spacesuits". It was built on the author's model (2026-09-16, and he was right against an argument): a spacesuit is just a collection of bytecode, so it reuses what already runs. **2026-09-24: `satellite.returns(T)` was TAKEN OUT**
(`8a69ae3`; the author never asked for it, in 003 or 004). The one thing owed inside
it -- every object held a COPY of its spacesuit's bytecode -- was done on 2026-09-22: an
object is its spacesuit's layout and its own values
(`satellite_object/satellite_spacesuit.hpp`).

**Chosen here, his to overrule** (each is one place; as of 2026-09-22): a method
dispatches on the OBJECT's spacesuit, not the declared one (003 was static); a field
declared again in a subtype is a second slot and the supertype's capsules read theirs
(003's rule); `darkening() { }` -- C++'s constructor, 20 of his lines -- is refused with
the sentence to write instead (003 refused it too); a spacesuit-typed field with
nothing after its name starts empty (003's rule, and 4 of his are self-typed).
**Still his** (each still refused on 0058): a built type as a supertype (M35, S210); a
spacesuit inside a CAPSULE (POLYMORPH M1's D1/D2; S210, naming POLYMORPH M1);
`Type(args)` inside a line (neither 003 nor 004; S110, which says to declare the object
on a line of its own first).

## M9 — polymorph

`POLYMORPH/M1.md`–`M7.md` hold the discussion. They are in git now, and `M7.md` still says
`words.def` has 342 rows, where `words/words.tsv` has 548 today. **Two of the author's
decisions are open:** D9.1 what "re-included into the individual capsules" means, D9.2
what `args` are passed to. **D9.3 is answered** (2026-09-16): *"a class declared twice is
an ERROR: name collision"*. It was blocked by M8, which is built.

## M10 — `satellite.cxx() { C++ }`

Not started: `satellite.cxx` has no row in words.tsv, and a line using it is refused
(S110). PROGRESS §6.5 settled the piece that looked hardest: a C++ file round-trips
byte for byte through the bytecode because a **count** needs no closing marker, so
`foreign_text_token` needs no code.

## M11 — `satellite.infinity` — **BUILT IN PART (INF-1, INF-2); its arithmetic is owed**

SATELLITE_INFINITY.md is this milestone's file now (its Part 13 lists the milestones).
**Built:** INF-1 (2026-09-18, `6408294`) and INF-2 (2026-09-19, `78b1447`):
`satellite.variable.infinity` (`1 6 17`) and `satellite.infinity()` (`1 26 0`), which
displays `(infinity)`, and unary minus, `(-infinity)`. **Owed:** everything else, INF-3 to
INF-9 in that file's Part 13. On 0058 `a + a` is refused: "an infinity's + is not built
yet (SATELLITE_INFINITY.md, INF-3)" (S210).

**What changed on 2026-09-18:** an infinity is no longer a `satellite_float` multiplier
but a list of terms, each an exact count (SATELLITE_INFINITY.md, "The design in one
paragraph"), so `infinity + infinity` is to display `(2 infinity)`, not `infinityx2`.
`arguments.infinity` is **128**, not 4096 -- the author: *"let's build it out of 128 digit
width, and set arguments.infinity = 128 for precision"* -- the nines width and the most
decimal places a count may have. `arguments.infinity_display` is 32, how many places
`.nines()` shows, and `arguments.infinity.counter` (999999999) counts toward the
SATELLITE INFINITY WARNING, which is INF-9 and not built: nothing reads the row yet (`config/satellite_config.hpp`; all three displayed on 0058).

**The first brief, 2026-09-16, with his words as he wrote them** (the whole entry is in history/MILESTONES.md). **D11.1 is answered** (the author,
2026-09-16): *"we answer what we can, and give an error on what we can't"*. **An infinity
carries a multiplier with it:**

- `infinity + infinity` displays **`infinityx2`**, and the infinity *"just carries
  an "x2" with it unless it goes away or causes an error"*.
- `infinity - 50%` is **`infinityx0.5`** -- *"unless the infinity is definitely
  reduced to nothing"*.
- What satellite does not know the answer to is an ERROR, never a guess.
- **The multiplier is ONE `satellite_float`, for going up or down** -- *"it will
  carry around a satellite float"*, *"an infinity will come with a single satellite
  float for going up or down"*, *"so we will have extreme precision for our
  infinities"*.
- **Held to 4096 digits, DISPLAYED rounded to 32, and BOTH are configurable** --
  *"that is 4096 digits in precision by default, but displayed as a rounded thing...
  we round to 32 digits"*, *"both digits configurable"*. The name
  `arguments.infinity_display` was proposed after `arguments.startup_display`, the
  author's to change.
- **An infinity keeps a sign, as a bool** (the author, 2026-09-17: *"give it a different number and keep a sign with all of these things a satellite.variable.bool with percentages and with infinities keep satellite.variable.bool with them"*) --
  so there is a negative infinity. How the sign and the multiplier's own sign meet
  (`-infinity` against `infinityx-1`) was left a question for the author.
  SATELLITE_INFINITY.md reads his message five as the answer -- a count carries
  its own sign, so `-infinity` and `infinity * -1` are one value -- and asks him to
  confirm it (its Part 12, Q20; to be built that way unless he says otherwise -- `infinity * -1` is not built yet, S210 on 0058).
  **Ruled 2026-10-09:** confirmed. A count carries its own sign, so `-infinity` and
  `infinity * -1` are one value (the author, answering the editors' questions:
  *"looks good"*).

## M12 — finding more batches while the program runs

One of the last milestones. **D12.1 is open:** the parallel-group syntax in the
numbered file. The measurements (2026-09-16; DESIGN §13 and PROGRESS.md section 3): a
thread costs 34.3 KB, math stops speeding up at 24 threads, one recall is 12,486 ns
against 28 ns a command, so threads take BATCHES and never single commands.

**TBB IS NOT ADOPTED** (the author, 2026-09-16): *"we are not adopting TBB for the
runners, they are just one line per thread at 1024 threads.... we are doing
something different later after the 1024 threads are done, we keep the 1024
threads warm for now... they just... don't do anything just yet"*. The measurement
(9× our own pool on a small loop repeated 10,000 times, equal on one huge loop)
stays in PROGRESS.md section 3 as a measurement, not a recommendation; TBB is in no
make file and not in `vendor/`. **The warm threads are the machine's cores x 2 since
2026-10-03** (24 here; `arguments.threads_startup_per_core` = 2), where they were 1024
when he wrote that. They convert the program and then wait: converting is the only work
anything hands them (`bytecode/cascade_convert.cpp`). What runs on them next is the
author's.

---

# Part 2 — new milestones, found building the object model

## M13 — the chain that remembers where it came from

**The author's own example, and the one chain that does not work yet:**

```
string_object.find("str").add("str")      find and add at location of find
```

`.find` answers a POSITION, and a position is a bare `satellite_number` that has
forgotten which string it came from — so `.add` adds to a number. Every other chain
he asked for runs (`s.bin.find("1010111")` and `n.to_string().add("x")`, both on 0058)
because each segment needs only the segment before it. **This one needs the segment
before that.** On 0058, as a value, `w.find("world").add("!")` on "hello world" answers
`6!` -- the position with the text after it -- and `w` is unchanged; alone on its own
line, `s.find("b").add("x")` runs nothing and says nothing (exit 0).

**The plan, as of 2026-09-16, is one more field:** the chain's loop in `expression.cpp`
carries the ORIGIN the chain started on, the CURRENT value, and the POSITION when the
last segment produced one; then `.add` asks whether there is a position and inserts into
the origin at it.

**Open:** does a position survive a conversion? `s.find("a").bin` — the position
in base 2, or the origin lost? (Today `"banana".find("n").bin` displays `10`, the
position in base 2.) And `.find` counts from 0 where every other position counts from 1
(M16, his to rule).

## M14 — satelliteContainer: list, map, variant — **the list BUILT 2026-09-18 and 2026-09-23 (`7263d2c`), the map 2026-09-26 (`fdc586b`)**

`satellite.container.list<T>`, lists of lists to any depth, `grid[x][y]` read and written,
`b = a` copying, the `<T>` checked on every way in, and `satellite.container.list()`
(PROGRESS.md section 1, "`satellite.container.list` takes what 003 had").
`satellite.container.map` since 2026-09-26, with `satellite.container.index` (a Python
dict, insertion order) as its other spelling; `multiple<A, B>` is the variant's type
half. All run on 0058.

**Still owed:** `satellite.variable.variant`'s asking half (`.holding .holds .held
.clear`, word `1 6 14`) -- whether it is `multiple` with no `<>` or a word of its own is
the author's. Today `satellite.container.multiple u = 5` runs, and
`satellite.variable.variant v = 5` is refused as a type not built yet (S110).
**Found and not fixed:** `{1, 2}` alone on a line ends the capsule silently with
exit 0, because a line-leading `{` is walked as a block (still so on 0058).

**The author:** *"a satelliteContainer which is satellite.container.list
satellite.container.map satellite.container.variant and containers can hold other
containers so we need a shared_ptr"*. The `shared_ptr` is right for his reason: a
container holding a container is a type that contains itself.

## M15 — satellite_file, and `file_object.seek()`

**The author:** *"file_object.seek(\"str\") and file_object.seek(\"entire_file\")
and file_object.seek(\"collection_of_str\")"*.

**Grown into SATELLITE_FILE_OPERATIONS.md (2026-09-18).** `satellite.variable.file` is
built: FO-1 to FO-4, committed the same day (`1547deb`; PROGRESS.md section 1,
"`satellite.variable.file`, and a program that edits its own source") -- 004's own
`satellite_file`, a list of lines counting from 1, not a port of 003's `Fil`. The
author's model for it, in that file's Part 1: *"we'll build it so you can iterate over
the lines as if they were objects ... As if each line were a list of objects."* (This
entry used to quote "a text file is a list of lines" as his; those words are the
proposal in that file's Part 3.) Its R9 answers the three `seek` overloads: the first is
`index_of`/`search`, the second `read_all` (all run on 0058), the third `index_of` given
a list -- FO-10, not built (S301 on 0058).

**WHAT IS LEFT OF M15 IS FO-5 TO FO-10**, in the order that file's Part 8 lists them (that
file is its 2026-09-18 text, not re-checked here). The one most likely to bite a real
program rather than a test is **FO-6, the race**: two runs of a self-editing program can
both read 7 and both write 8. satl has no lock on a file between two runs -- the only
`flock` in satl's own code is config.ini's (`config/config_file.hpp`); inside one run, M39's
`.lock()` makes threads take turns.

## M16 — the rest of the string methods — **BUILT 2026-09-25 and 2026-09-26**

`upper` and `lower` on 2026-09-25 (`c7ec8a1`), on the author's word, with his second
spellings `.uppercase()`, `.up()` and `.lowercase()` -- every language's letters through
glibc's C.UTF-8 case table (`satellite_object/string_case.hpp`), where 003 changed a-z
alone. The other thirteen on 2026-09-26 (`c2b3500`), on "do M16 and M21", and `s[n]` with
them: `size` `empty` `contains` `starts_with` `ends_with` `at` `substring` `split`
`replace` `trim` `resolved` `append` `clear`, on the language's own 16-bit string
(`bytecode/string_calls.cpp`, `satellite_object/string_pieces.hpp`). M21 was not needed.
PROGRESS.md has no record of M16; the full one is this file's old text.

**Choices he may overrule** (each still so on 0058):
- **Positions count from 1**, as a list's items do: `s[1]` and `s.at(1)` are the first
  character, and `substring(start, end)` keeps both ends -- 003's `substring(start - 1, end)`.
  An empty piece is `substring(n + 1, n)`. A first position of 0 is refused before anything
  runs (S413 COUNTS_FROM_ONE); past the end is S411, a negative S410, backwards S412.
  **`.find` still counts from 0**, as it has since 2026-09-16 and as check.sh
  pins it, so `s.substring(s.find(","), ...)` is one character off -- his to rule.
- **`s[n]` reads; `s[n] = x` is refused** (S301), and so is `"abc"[1]` straight after a
  literal (S110; a list literal has no `[ ]` either).
- **A number where text goes is its digits, with S020** -- `.find` too. A bool, a list or
  any other kind is refused (S301).
- **`append` and `clear` change the name**, as a list's `append` does, and are refused on a
  literal inside an expression -- `display("abc".append("d"))` is S110. **Alone on its own
  line, `"abc".append("d")` runs nothing and says nothing**: a line that starts with a literal
  is passed over whole, by the checker and the walker alike (`5 + 3` and `"abc".size` alone
  on a line too), before M16 as after it -- his to rule whether such a line is refused.
  On 0058 a method on a NAME that changes nothing is passed over alone on a line too
  (`s.size`, `s.upper`), while `u.append("d")` alone does change `u`.
  `"".split("")` is an empty list and `"".split(",")` one empty piece, as in 003.
- **`resolved` answers the string as it is**: 004 has no live escapes.
- **A float or a bool where text goes is refused (S301)** while a whole number is its digits:
  `s.contains(42)` is true of "n42n", `s.contains(4.5)` stops the program. Whether his
  "just convert the number to the string" covers 4.5 is his; until then it is refused only
  when the line runs, not before.
- **`[ ]` straight after a method's answer or a literal is not built** -- `s.split(",")[2]`,
  `"abc"[1]`, `l.reverse()[1]` -- and is refused by name (S110) -- on a string before anything runs, on a list (`l.reverse()[1]`) only when the line runs -- with
  the way round it: give the value a name first. Building it is his to decide.

**The review of 2026-09-26 found two quadratic loops** (measured then, not re-run):
`t.append("ab")` in a loop -- 200,000 appends 6.10 s, 50,000 0.35 s -- because a
statement's last append copied the whole string; a change that ends a statement answers
nothing now, and 200,000 appends took 0.23 s. And `s[i]` walked from the string's front
whenever it held one wide character: 32,769 characters with one emoji 1.87 s, 65,537 13 s
(`satellite_string.hpp` records the same cases as 2.40 s and 13.04 s). The string keeps a
bookmark of the last character a walk reached, and 32,769 took 0.10 s. **Reading forwards
is linear; reading backwards is not** -- a wide character's halves can be any sixteen
bits, so no walk can step back, and a loop from `s.size` down to 1 on a string with a
wide character still walks from the front each time.

**Answered, and one ruling covers both** (the author, 2026-09-16): for
`string_object.replace(number1, number2)` and `string_object.find(number)`, a
number where a string is expected is *"just convert the number to the string and
run that piece, obviously the programmer meant convert to string, but record the
warning in satellite.log"*. M5 built the warning (S020, 2026-09-25).

**Answered and built: a string minus a string** (the author, 2026-09-17): *"minus
takes away the smallest string"*, `"dfksjghjfff" - "fff"` is `dfksjghj`; asked which
copy goes when there are two, *"first occurrence"*; and *"minus - fff"*, the string
after the minus is the one taken away. So `"abcab" - "ab"` is `cab` and
`"fff" - "dfksjghjfff"` is `fff`. Nothing to take away is an answer, not a refusal.

## M17 — the number methods, and `number_methods.cpp`

`satellite/bytecode/number_methods.cpp` has been written, unwired, in no make file and
referenced by nothing since 2026-09-16 (its one commit, `715ac3a`). It holds `power`,
`modulus`, `max`, `min`, `shift_left`, `shift_right`, `abs`, `digits`, `bytes` and the
conversions. **Two gaps:** `clamp` is numbered `1 6 4 5` and is not in the file; `bytes`
is in the file and is not numbered (next free `1 6 4 21`). On 0058 the numbered methods
are refused two ways: `n.power(3)` is S210 (not built), while `n.abs`, `n.floor`,
`n.digits` and `n.sqrt(4)` are S110 ("not one of its methods").

**Finish it or delete it. Not a fourth session.** (Written 2026-09-16. Neither has
happened; which one was the author's.) **Ruled 2026-10-09:** finish it, not delete it:
`power`, `max`, `min`, `abs` and the rest are numbered and refused today (the author,
answering the editors' questions: *"looks good"*).

## M18 — `satellite.statement.if` — **BUILT 2026-09-17**, `52fc971`

`if` and `else` run (words `1 13 1` and `1 13 4`; PROGRESS.md section 1,
"`satellite.statement.if` and `.else`"). Still owed from it: ERROR.md's depth entry asks
in as many words to be re-read when `if` lands, because a recursive capsule could not
choose to stop before it did. It has not been re-read.

## ~~M19 — `n = 1 & 2` answers 1, silently~~ — **FIXED**

`read_to_the_end` (`program_walk.cpp`) makes a variable's value and a loop's bound demand
the whole expression. On 0058 `satellite.variable.number n = 1 & 2` is refused, "could not
be read to the end -- it stops at something with no meaning there yet (& | << >> !! are
undecided)", machine code 13, where it used to store 1 and say nothing.

**What was left was a ruling, and he made it on 2026-09-22:** `&&` `||` `!` for logic;
`&` `|` `~` `<<` `>>` for bits; and `.and()` `.or()` `.xor()` (SCRATCH.md/ERRORS3.md;
SCRATCH.md/LANGUAGE_IN_SATELLITE.md, "What satellite is missing"). Of these only `!` is
built (`!done` on false is true); `&&` and `||` are refused like `&`. `^` has been power
since 2026-09-16, with `**` its second spelling, and `!!` (bit runs end to end) was
decided 2026-09-09. satl still calls `& | << >> !!` "undecided", and the registry rows of
`&& || ! & | ~ << >>` still say QUESTION (M24).

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

**BUILT 2026-09-17** (`b4bf0f9`), everything in this entry except satellite.history
(PROGRESS.md section 1, "`satellite.statement.for`"). Your own line runs:

    satellite.statement.for(satellite.variable.number my_int = 0; my_int < 3; my_int + 1)

The third part is built as written, with no `=`: the loop's own name goes in front --
*"in every circumstance, we are adding `my_int = ` to the code"* -- and `++` / `--` are
read inside this one bracket only, this entry's own *"a form that is not consistent with
other parts of the language, so the for loop is the only place where this exists"*. Any
other step is refused before anything runs (on 0058: `--i`, a bare `i`, a touching
`i**2`). That rule came from a fresh reader's find: `--i` ran forever printing 0, nine million lines in five seconds, saying nothing (2026-09-17). **`**` IS POWER, IN THE STEP AND EVERYWHERE, SINCE INF-1** (2026-09-18): on
2026-09-16 you ruled that power is `^`, and you write `**` ("power ** infinity",
`my_number ** my_number ** my_number`), so a spaced `**` is `^`'s second spelling. Of
*"it exists while the for loop is running, then it exists under... satellite.history"*
the first half is built (the name is erased after the last turn, and refused after the
loop); the second half is M20.B.

**FOR YOU, ON THIS ONE** (written 2026-09-17), with what is true on 0058:

1. **`satellite.return` inside a loop did not end the capsule. FIXED 2026-09-22:** a
   return ends the capsule from any depth, with its answer (`program_walk.cpp`). The fix
   took the two things this item left to you -- whether return leaves the whole capsule
   or only the loop, and what its ARGUMENT means -- from your own 003 programs, not from
   a ruling in words: `tagged_report.satl`'s `role_of` returns from inside two ifs, which
   is only right if a return leaves the capsule with its value. Whether that is your ruling -- return leaves the whole capsule, not only the loop, and its argument is the value handed back -- was yours to say.
   **Ruled 2026-10-09:** it is. A return leaves the whole capsule, not only the loop, and its
   argument is the value handed back (the author, answering the editors' questions:
   *"looks good"*).
2. **8,000 nested `for`s SEGFAULT (exit 139, core dumped)** (2026-09-17). 6,500 was fine;
   `if` survived 8,000 and died later, because `run_for`'s C++ frame was the biggest of
   the three. It was stack exhaustion, proven: `ulimit -s 65536` made the 8,000 case print
   and exit 0. **This is the thing "the language has no limits" is about, and a depth
   bound is not the fix** -- the fix is the walker keeping its own stack instead of
   recursing through C++, which is the same decision as item 3 and wants to be made once.
   Not re-run; satl widens its own stack at start since 2026-09-22, so the depth where
   it dies has likely moved.
3. `program_walk.cpp` was **939 lines** then, against your 300; it is **2,636** today
   (`program_check.cpp` 3,238). The three statement runners (`run_while`, `run_if`,
   `run_for`) and `run_statements` call each other, so splitting them out means putting
   `run_statements` in a header -- a real change to what that header promises, not a
   move. Say whether to do it.
4. `satellite.statement.for(...; ...; )` with an empty step is accepted -- this
   entry marks only the third part "optionally". Is an empty CONDITION
   (`for(number i = 0; ; i++)`, C's forever loop) wanted too? It is refused today (S110).
5. Smaller: `i ++` with spaces is taken as `i++` (the lexer makes both signs touching
   whatever the gap), and junk after the header's own `)` is ignored -- `for(...; i++))`
   runs clean. Both still so on 0058, and for a `for` only (this item said `while` shared
   them; `while(n < 2))` is refused when it runs). Also still so: a step that starts right
   and stops being readable later, `i + 1 & 2`, is refused only after one turn has run.

## M20.B -- satellite.history
before satellite.history is created, we need the arguments variable to be created...

satellite.history if it's arguments are set to true, records the values of everything onto disk -- type, name and value of every object, and satellite.history is similar to satellite.library, except satellite.history is also a record of past values, and even past types of objects, written to disk. This is very easy to program, it just has alot of requirements, and it adds additional lines of code to every action taken by the satellite interpreter -- especially when turned on!

**Not built.** NEW_MILESTONES.md holds it now as NM-6, *"the satellite.history milestones
will be last"*. The arguments variable it waits on is built. `satellite.history` is not
a word: a line `satellite.history(x)` is refused (S110).

## M20.C satellite.history arguments
satellite.history relies on two variables from arguments...

arguments.history is a satellite.variable.bool, when set to true, it records type name and value of every object, and writes it to disk, then satellite.history(object_name) will be the way to access the disk...

arguments.history_path = "/dir/" this must be a directory, and is an error when you try to set it to a particular file... in this directory, we will be saving name_of_satl_file.history files with the extension .number.history for each time that file is ran, now the satellite.history extension is going to have to check if turned on, whether there is at least... 32 gigabytes? Free in the location of the directory, otherwise the interpreter must set arguments.history to false. So the entire history system is hidden behind something: a single check of whether arguments.history is set to true or false, if set to false, then none of the satellite.history stuff has to be accessed, somehow we have to program this, because this adds alot to the interpreter when turned on, so we need this way of turning all of it off, so it doesn't slow us down as much...

**Not built.** The switch is only a number, `satellite.library.arguments.history`
(`1 14 3 11`), and a program reading `arguments.history` is refused (S201). The feature
register's `history` bit is not built (`config/feature_register.hpp`). No
`arguments.history_path` row exists, and the free-space check is not written.

## M20.D satellite.statement.if
satellite.statement.if is just (condition) { code }

**BUILT 2026-09-17** (`52fc971`) in that shape, with `satellite.statement.else` (M18).

## M20.E satellite.statement.break

break is part of if, for, while and exits the loop

**BUILT 2026-09-26** (`15f5119`, word `1 13 5`): it leaves the nearest `while` or `for`,
from inside any `if`/`else` in it; a `break` with no loop around it is refused (S110).

## M20.F satellite.statement.continue

continue is part of if, for and while and jumps out of the current statement correct me if that is wrong?

**BUILT 2026-09-26** (`15f5119`, word `1 13 6`): a `continue` ends the bodies it is in,
up to the nearest `while` or `for`, and starts that loop's next turn -- in a `for`, by
way of the step, as C's does. That answer was the build's, not his: whether it is what
he meant was his to say. **Ruled 2026-10-09:** it is what he meant: `continue` starts the
loop's next turn (the author, answering the editors' questions: *"looks good"*).

## M20.G satellite.statement.finally

finally is for 
satellite.statement.if
satellite.statement.while
satellite.statement.for

**Not built:** `satellite.statement.finally` is not a word (S110).

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

**Not built:** `switch` and `case` are not words (S110). His question inside the design
-- what else the interpreter should provide for "switch" -- is still his.

## M20 — satellite_float, and hex as its own type — **BUILT 2026-09-22**, with the colour and the fraction

`satellite.variable.float` `1 6 10` and `satellite.variable.hex` `1 6 11` were built on
2026-09-22 at the author's asking, with the colour and the fraction (PROGRESS.md section
1, "The float, the hex, the colour and the fraction"). **Decided 2026-08-27:** a float is
a bool and two `satellite_number`s — left of the point exact and unbounded, right of it
bounded, because repeated multiplication grows digits downward. Its precisions are rows:
`arguments.float.whole` (4096) and `arguments.float.decimal` (128).

The three refusals it was to retire, on 0058: `2 ^ -1` displays `0.5`; `5/4`, touching,
is the fraction `5/4`; and `5 / 4`, spaced, displays `1` -- two whole numbers still divide
to a whole number, cut toward zero (`satellite_variable_number/number_arithmetic.hpp`,
which says to revisit this when the fraction lands). `tests/not_understood.satl` went red
on purpose when the float landed and was moved onto `satellite.variable.time`. **When that
is built it goes red on purpose again** -- move it again; never delete it. (It was built on
2026-10-10, M32, and the file moved onto `satellite.variable.network`, `1 6 12`.)

**Binary is built** (2026-09-16): `satellite.variable.binary` `1 6 5` is
`satellite_binary_number`, arm 7 of `satelliteObject`, width kept. **Owed on it:**
003 DESIGN 8.5's DECIDED AND UNBUILT rulings -- `+` on bit runs answers a bit run,
the width grows to fit, the left operand's type wins, `!!` joins, indexing counts
from the right -- are not built; today arithmetic reads a binary by worth and
answers a number (`b0101 + b0011` displays `8` on 0058). That is the author's to
schedule, not to be decided in passing.

**A binary keeps a sign** (the author, 2026-09-17: *"give it a different number and keep a sign with all of these things a satellite.variable.bool with percentages and with infinities keep satellite.variable.bool with them"*). `-b0101` is a binary --
displayed `-b0101`, worth -5, width 4, `.bin` `-0101`, not equal to `b0101`; `= -1010` is
`ERROR: expected -b1010` (all still so on 0058). The sign is the bool the bits'
`satellite_number` already carries, so there is one bool; `-b0000` is `b0000`. Built in
`741a017` (2026-09-17). My reading of "give it a different number" (a negative binary
is a different value, not a new machine code) is not confirmed by the author.

## Open questions from the 2026-09-17 wide-strings review

The review of the wide-strings patch (2026-09-17) left four questions for the author --
design, not bugs. They were in a session note pasted into this file, which is cut now
(history/MILESTONES.md has it whole). The questions, as written:

- percentage * number answers a number, but percentage / number answers a
  percentage, so `50% / 4 * 4` is refused (24);
- percentage / percentage has no scenario;
- the `50% - 5` hint suggests `5 - 50%`, a different quantity;
- how infinity's sign meets its multiplier's sign.

On 0058 the first three are unchanged: `50% * 4` displays `2`, `50% / 4` displays `12.5%`,
and `50% / 4 * 4` is refused (S402, exit 24); `50% / 25%` is refused, "there is no
scenario" (S301); `50% - 5` is refused with the hint "the number goes first -- 5 - 50%"
(S301). The fourth was M11's open question (SATELLITE_INFINITY.md, Q20).
**Ruled 2026-10-09:** a count carries its own sign, so `-infinity` and `infinity * -1` are
one value (the author, answering the editors' questions: *"looks good"*).

## ~~M21 — the 23 string words still on `satellite_string32`~~ — **DROPPED 2026-10-09: to be deleted, not moved**

Owed since 2026-09-15: the 23 string words were written on the old 32-bit `strings/`
type, renamed `satellite_string32` on 2026-09-16 when the object model made the two
collide. They were to move onto the language's own string, checked against 003's satl
as they were the first time, and `strings/` was to fold into
`satellite/satellite_variable_string/`.

**On 0058** there are no `.so` libraries (2026-10-07). The 23 words
(`satellite-numbers/words/1.6.1.0.cpp` to `1.6.1.22.cpp`) are compiled into satl and
still take `satellite_string32`, but satl never calls them: a string method runs through
`bytecode/string_calls.cpp` on the language's own string (M16), and nothing in
`satellite/` reads a word's `string_method`. Only `make test` still runs them, through
`strings/test_string_methods.cpp` and `strings/check_string_methods.py`, against 003's
satl. **The question was:** fold `strings/` and the 23 words into the tree's own tests, or
drop them? **Ruled 2026-10-09:** drop them. `strings/` and the 23 words are to be
deleted, not folded into the tests (the author, answering the editors' questions:
*"looks good"*). Nothing is deleted yet.

## M22 — threads on objects — **answered by M38 and M39**

003's `SuitObject` carries a `recursive_mutex` and a thread access list (THREAD.md D1,
D2) because **two threads sharing one object wrote one value at once and freed a string
twice**. 004's answer is M38 (a program's own threads) and M39 (the author's lock: objects
and files are shared, each with a lock that is off until `.lock()`). **Still open from
here:** a chain of objects each holding the next is a linked list, and freeing a long one
recurses once per object. 003 answered it with a staged burial. Not re-checked (a deep
chain is a crash probe).

## M23 — satellite_number's own debts (ERROR §5, never reviewed adversarially)

Nobody has tried to break this type — the four adversarial reviewers were killed
by a session limit and never ran.

1. **`satellite_number x = -1;` holds 18,446,744,073,709,551,615.** The
   one-argument constructor is not `explicit`, so a signed literal converts
   silently under `-Wall -Wextra`. **A wrong value, one keyword, do it first.**
   Still so (`satellite_variable_number/satellite_number.hpp`; `from_signed()` is the
   safe way in).
2. `to_text` is quadratic — 8.5 s at a million digits, schoolbook multiply and
   divide, no Karatsuba. (Not re-checked.)
3. Out of memory threw `std::bad_alloc`, not a machine code. **Since answered for satl:**
   it catches `std::bad_alloc` at its top and reports S999, machine code 48
   `out_of_memory` (`structured-library.cpp`); the type itself still throws.
4. The header exposes `unsigned __int128` and `[[gnu::always_inline]]` to every
   file that includes it. Still so.
5. The speed bar is missed in places: `i = i + 1` is ×1.49 (clang) and ×3.0 (g++)
   of C++ that refuses to wrap; satellite_string's wide path is ×1.19–×1.24.
   (Measured before 2026-09-17; not re-run.)

## M24 — the stored program: transcription, or code stream

**PROGRESS §6, open, the author's** -- and NARROWER than it was. There was one
stored file, `.sate`, holding the registry's codes, until the author dropped it on
2026-10-03 ("skip .sate altogether", M31): no program is stored at all now, so
this question waits until something stores one. What is still undecided is
whether a stored program may be RE-LEXED as a transcription, which is what a
converter needs to know before it records anything.

With it: **the registry rows marked QUESTION** -- 23 token rows in REGISTRY.satellite
today (this entry said nineteen) -- which name forms this language cannot emit:
`+= -= *= /= %=`, `&& || !`, `& | ~ << >>`, `/* */`, `:: -> ? '`, and the four that only
ever occur inside a literal. The author asked for several of them on 2026-09-16, so
they keep their codes until he rules. He ruled the spellings of `&& || !` and
`& | ~ << >>` on 2026-09-22 (M19); their rows still say QUESTION.

## M25 — what is numbered and not built

`words/words.tsv` has 548 rows today (365 on 2026-09-17), and every word is compiled
into satl (2026-10-07). A numbered word with nothing behind it is refused, mostly
`not_built_yet` (S210, exit 14), sometimes S110. Not built on 0058, each one run:
- `satellite.time()` (`1 9 0`), `satellite.time.now` (`1 9 1`) and `satellite.time.new`
  (`1 9 2`) -- the time itself, its words and the sleep were built on 2026-10-10 (M32);
- `satellite.network` (`1 20`) and `satellite.variable.network` (`1 6 12`);
- the variant's asking half (M14) and the number methods (M17);
- `satellite.console.input(prompt, target)` (`1 5 4`) and `satellite.console.typed()`
  (`1 5 5`), both S210 (M36);
- `satellite.directory.current()` and `.exists(d)`, both S210 (M0.6);
- `satellite.help` inside a program, S210 (at the prompt it answers; M34 in Part 4).

Built since this entry listed them as waiting: the list and the map (M14), the file (M15),
`satellite.directory`'s change, list and system, `satellite.variable.thread` (M38) and
`satellite.access` (M34). **Also open (PROGRESS §4):** one more thing to remove from
004's words that the author could not remember.

## M26 — the prototype's seventeen defects — **all settled on 0058**

ERROR.md §1 was written against the prototype runner, which is retired. Every one was
re-run against the bytecode path on 2026-09-17, and the seven still listed then are
settled: #7 exit codes cut to 8 bits, fixed (`exit_status_of`, M0.5); #13 a file that
starts with a byte-order mark runs, and so does a file whose lines end in CR alone (it
was read as one line on 2026-09-17); #14 `display( 42 )` prints 42; #10 every `*.so`
loaded, #11 no ABI check at load and #17 a library's file name never checked went with
the loader -- no word is loaded from a file since 2026-10-07; and #16, unknown escapes:
the author ruled on 2026-09-25 *"refuse an escape that is unknown"* (PROGRESS.md section
2, A6), and `"a\qb"` is refused before anything runs (S110).

## M27 — the test harnesses can pass for the wrong reason

ERROR.md §3. All three are still so in the source: **#22**
`strings/test_string_methods.cpp` splits text arguments on `|`, so an argument holding
`|` is silently two. **#23** `strings/check_string_methods.py` writes 003 programs without
escaping (its 004 programs do escape). **#24** `words/make_words.py` needs 003's built
`satl` (`old_versions/second_satellite/satl`). A harness that can pass wrongly is worse
than one that fails, so this is smaller and more urgent than it looks.

## M28 — the toolchain on this machine

ERROR.md §6. **#33** g++ 17.0.0 miscompiles at `-O2` (a reserve + nested push_back
loop read `text.size()` as 0) -- not re-checked. **#34** g++ here is
`--with-arch=native` (still so, `gcc -v`), so a g++-built satellite dies with an illegal
instruction on an older CPU — **which matters for the distribute package** and is the
reason clang builds the shipped binary (`satl --version`: CLANG++ 24).

---

# Part 3 — the parallel machine, and the rest of 2026-09-16's session

The cascade landed (`75a5b43`) and `.sate` landed (`da4f3e3`), both on 2026-09-16. These
are what the author described that is NOT yet built. Both have changed since: the same
night (`9065262`) the cascade and its watermark gave way to his design that converts every
line before a single line runs, and the `.sate` was dropped on 2026-10-03 (M31).

## ~~M29 — converting ahead WHILE running~~ — **DROPPED 2026-10-09**

**The author:** *"the main thread, after it starts 1 thread, that 1 thread starts
256 threads, the main thread will be converting the first line of code into
16-bits, then running it, after it has run the next line, the thread that gets
line 2 is handing it to main"*.

**On 0058 every line is converted before a single line runs.** `bytecode/cascade_convert.hpp`
quotes the author, 2026-09-16, after this entry was written: *"let's create 1024 threads,
and have 1024 convert every line of the program into 16-bits before a single line runs,
that makes it so much easier to program, and there's nothing to test that way"*. The warm
threads (cores x 2, 24 here) each convert one block of lines, and main waits until all are
done; there is no watermark and no atomic counter any more. **Whether M29 is still wanted
was the author's.** **Ruled 2026-10-09:** M29 is dropped. Nothing is converted while the
program runs: his design of that same night converts every line before any line runs
(the author, answering the editors' questions: *"looks good"*). The design as it was
written:

- **The hard part is not the threading, it is the unit.** The walker runs STATEMENTS and
  a watermark counts LINES, and a statement can span lines (a `while` and its body), so
  the watermark has to mean "every code up to here is final", and the walker has to
  refuse to step past it rather than read a half-converted row.
- **The author's own fallback is already stated:** *"if main gets pieces out of
  order, then it has to convert"* — so when the watermark is behind, main converts
  the line itself rather than waiting: an optimisation that can never deadlock.
- **THE CHECKER GETS ITS OWN THREAD — ANSWERED 2026-09-16.** The author: *"Checking goes
  in a separate 1/256 threads, and so that means 250 or so threads are available for
  conversion.. checking reports into satellite.log with warnings, and dies at that"*.
  `check_program` walks every capsule before main runs, which is what makes check.sh's
  "nothing ran before the refusal" true. This file read his answer as neither of the two
  options put to him: **the checker RACES the runner** -- one thread follows the
  watermark and checks, the rest convert, main runs, and when the checker finds something
  it writes the warning and kills the program. It needs `satellite.log`, which M5 built.

## M30 — the lookahead: running paths in parallel

**The author:** *"eventually we will build some logic that looks ahead at paths
that it can... run in parallel, but until we build that, we are just saving the
16-bit conversion"*.

This is the one that makes satellite parallel rather than its CONVERSION parallel.
Converting a line is pure and independent; running one is not — line 2 may read what
line 1 wrote. So this milestone is a DEPENDENCY analysis: which statements touch which
names, and which runs of statements touch nothing in common. **Do not start this before
M29** (this file's order, not his): overlapping conversion with execution is safe on its
own; overlapping execution with execution changes what programs mean, and needs the
analysis first. REGISTRY.satellite reserves `batch_start`, `batch_end`, `wait` and
`batch_size` for these marks, as "a PROPOSAL against open decision D12.1".

## ~~M31 — running a `.sate` without its `.satl`~~ — **DROPPED 2026-10-03, and the `.sate` with it**

The author, 2026-10-03, after asking "does a .sate file even speed anything up?" and seeing it
timed: *"let's just delete writing the .sate files and move on?"*, then *"skip .sate altogether"*.
So satl writes no `.sate` (`ee0f605`) and has no `arguments.sate` row: the codes are made
fresh from the `.satl` on every run.

**What the timing said** (2026-10-03; satl's own conversion on the 1024 warm threads it
had then, best of seven): his programs of 2,760 to 3,534 lines convert in 3.4 to 3.7 ms,
where checking a fingerprint and reading 2-byte codes back would have cost about 0.1 ms
-- so running from a `.sate` saves about 3.5 ms of a 60-70 ms start, most of which is satl
starting itself. The text `.sate` cost more than it could ever save: written on every run
at 17 bytes a code, it took 3 ms a run at his programs' size and 212 ms at 100,000 lines,
and reading the text back cost more than converting. A line is 13 to 15 codes in his
programs (a 100,000-line program of his shape is about 1.4 million codes), and a binary
read of 1.4 million codes is 1.7 ms warm, 7 ms cold. It took `bytecode/sate_file.{hpp,cpp}`
with it; check.sh's row that read U+1F600's three codes off a `.sate` is now a case in
`build/count_cases`.

## M32 — `satellite_time`

The author named it as an arm of `satelliteObject`: *"satelliteCapsule,
satellite_number satellite_string satellite_bytecode, satellite_bool,
satellite_time, satellite_file, and any other variable we have"*. 003 had it
(`Time`, eight bytes, an instant in nanoseconds). `satellite_file` is M15 and is built;
this is its sibling and is much smaller. Not built on 0058: its words are numbered
(`satellite.variable.time` `1 6 3`, `satellite.time` `1 9`) and refused (M25).

**BUILT 2026-10-10 (revision 10, BUILD 0009), as a LENGTH of time, at the author's asking:**
*"we need to build satellite.time.sleep(1s or 1ms or 1ns) since we are taking in 3 types ...
satellite.variable.time my_time = satellite.time.second(1) or satellite.time.microsecond(1) or
satellite.time.nanosecond(1) and accept here s and ms and ns as aliases of those time spans"*.
`satellite_time` is arm 21 (`satellite_variable_time/satellite_time.hpp`): a whole number of
nanoseconds in a satellite_number -- exact, no ceiling -- and the unit it is shown in. Written
`1s`, `250ms`, `30ns`, `1.5s` (`time_token`, 0x090C, a number with its unit touching it), or
made by `satellite.time.second(n)` `1 9 5`, `.millisecond(n)` `1 9 6` and `.nanosecond(n)`
`1 9 7`, whose second spellings are `.s(n)`, `.ms(n)` and `.ns(n)`. `satellite.time.sleep(t)`
waits at least `t` against one monotonic deadline, looking up every tenth of a second for
`.stop()`, `satellite.return(satellite)` on any thread and the prompt's Ctrl-C; a bare number
is refused (write `5s`, `5ms` or `5ns`), a time below zero is S440, and 003's
`sleep(n, unit)` is told to write `90ms` (`bytecode/time_calls.hpp`). **His to rule:** `ms` is
built as the MILLIsecond -- he wrote "microsecond" beside it, and every earlier time he meant a
thousandth (003's `sleep(90, "ms")` was QUAD's 90 milliseconds). Not built: `satellite.time.now`
and `.new` (an instant, 003's meaning of the arm), and `+ - * /` on times.

## ~~M33 — the step as a JUMP, not only a front~~ — **DROPPED 2026-10-09, and `arguments.magic` is to go with it**

The author's other shape — *"one of the level 3 threads just jumps onto line say pick a
magic number... so it starts converting at line 5"* — is a STRIDED start: threads
beginning at different offsets rather than one chain from line 0. It was worth doing only
if M29 showed the front-loaded cascade leaving threads idle: filed so the idea is not
lost, not because it is owed. **Since then** the cascade and its front are gone, and each
warm thread already starts at its own offset (one block each, M29). `arguments.magic`
(default 5) still exists and is read by nothing (`arguments/argument_settings.hpp`).
Whether M33 is still wanted was the author's. **Ruled 2026-10-09:** M33 is dropped, and
`arguments.magic` goes with it: its row is to be taken out (the author, answering the
editors' questions: *"looks good"*).

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

**Measured rather than argued** (2026-09-18). clang 24 -O2, 2,000,000,000 statements,
eight features off, two runs agreeing to the fourth digit:

| how the features are gated | ns a statement | over the floor |
|---|---|---|
| **no gates at all** (the floor) | **0.216** | — |
| eight separate `bool`s | 1.81 | +1.59 |
| one bitmask, eight bit tests | 1.84 | +1.62 |
| nested switch **per statement** | 0.708 | +0.49 |
| one bitmask, **one** test per statement | 0.72 | +0.50 |
| **nested switch WRAPPING the loop** | **0.216** | **+0.000** |

(`config/feature_register.hpp` records the `bool` and bitmask rows against a floor of
0.224, `feature_switch.hpp` the switch rows against 0.216.)

**So: switch, and the milestone is right — but the win is not the switch, it is WHERE
the switch is.** An `if` hierarchy and a `switch` hierarchy measure the same inside the
loop (1.81 against 1.84). **The hierarchy WRAPS the interpreter** so the choice is taken
once, and the loop it chose has no test in it at all. In the real interpreter, a
2,000,000-turn satellite loop through four different leaves came in at 2.272, 2.276,
2.269 and 2.288 seconds — within 1%. **Built 2026-09-18** (`364f22c`) as
`satellite/config/feature_switch.hpp` -- 268 lines (this entry said 230) -- inside the
milestone's own estimate of *"100 lines ... or maybe 200 300 400"*; SATELLITE_ERROR.md
Part 14 has the whole record. The sketch's `switch (default_variables)` cannot be written
in C++, which switches on integers only: that is why the register is a `std::uint64_t`,
and `written()` / `read_written()` in `feature_register.hpp` convert between it and the
binary a person reads in config.ini.

### What IS in `--rebuild` today

Fourteen feature bits, composed from the named keys in config.ini into one satellite
binary, with a table saying which are on and **which are turned on and not built yet**
(`config/feature_register.hpp`): `access` `history` `frames` `statements` `trace`
`coverage` `word_counts` `capsule_timing` `watchpoints` `memory_accounting` `thread_state`
`report_on_success` `report_file` `replay`. **Three are built** (this entry said two):
`access` (the switch `arguments.access`; today it decides whether a name declared in a
block ends at its `}` -- M34 in Part 4), `statements` (the statement ring) and
`word_counts` (per-word call counts). Since 2026-10-03 `--rebuild` also sets
`first_start = true`, so the next start reads the machine's facts again.

### What is NOT in `--rebuild` — the list this milestone is for

**1. Eleven features have a bit and no behaviour** (this entry said twelve). Ten have
their milestone in SATELLITE_ERROR.md Parts 8 and 12; `history` has none there (it is
M20.B/M20.C here and NM-6 in NEW_MILESTONES.md). `--rebuild` names them when they are
turned on, so this is visible rather than silent.

**2. Booleans that could be bits and are not:** `arguments.startup_display` (compiled in;
gates the start-up block) and `arguments.debug_mode` (`--debug`; gates every state line).
The third, `arguments.sate`, went with the `.sate`. Newer flags, not in the register:
`arguments.missing` (2026-10-03) and `arguments.scroll.vertical` / `.horizontal`
(2026-10-07). **`--debug` is the interesting one**, because it is the only one a person
changes per run rather than per machine — so folding it into a register composed once
would take that away. It may want to stay a flag, or want both. (Since 2026-10-06 a line
in a program changes any setting for that run only.)

**3. Numbers that CANNOT be bits, and this is the real limit of the design.** A register
bit is one yes-or-no; these are values. There are fourteen number rows today
(`config/satellite_config.hpp`), where this entry listed ten: `arguments.magic` (5),
`.version`, `.revision`, `.build`, `.object_bytes_max` (34359738368), `.threads_max`
(1000000), `.threads_startup_per_core` (2; it was `.threads_startup`, 1024),
`.file_size_max_bytes` (549755813888), `.infinity` (128; it was 4096),
`.infinity_display` (32), and since then `.float.whole` (4096), `.float.decimal` (128),
`.infinity.counter` (999999999) and `.display.buffer` (131072). **So `--rebuild` composes
the FLAGS and cannot compose the NUMBERS**, and a milestone that says "pack everything
into --rebuild" has to decide what that means for a number. Three ways, each one line to
reverse:

- **leave them where they are** — they are read once at start-up and never in a
  loop, so they cost nothing and the register buys them nothing;
- **compose a second value** — `numbers = 5,4,4,115,...` beside `features`, one
  parse instead of ten;
- **bit-pack the ones with small ranges** — `magic` and `infinity_display` fit in
  a few bits each, and nothing else does.

**The first is almost certainly right** (this file's view on 2026-09-18, not his ruling):
the register earned its place because it is tested in a LOOP, and a number read once
before the program starts is not on any hot path.

**4. The machine facts are not settings and do not belong.** `arguments.memory.total`,
`arguments.machine.cores`, `arguments.machine.threads`, `arguments.disk.free`,
`arguments.username`, `arguments.system.*` are facts about the machine; composing them
would cache a fact that changes, a register that goes stale (S011 REGISTER_IS_STALE; this
entry said S0723). Since 2026-10-03 the six that do not change -- the processor and its
cores, the host name and the system -- are read on a first start and kept in config.ini
under `first_start` (`satellite-numbers/machine_facts.hpp`, the author's ruling that
day); memory, disk and the user name are still read every run.

**5. `satl --config`'s measured thread count is built** (2026-09-18, SATELLITE_ARGUMENTS
Phase 8, C1–C8 all done): `satl --config [n]` writes what the machine can really do to
`~/.satl/machine.conf`, its own file and not config.ini. Whether it and `--rebuild` are
one command or two was SATELLITE_ERROR red note 13; SATELLITE_ERROR.md Part 15 (the
author's rulings, 2026-09-18) records that they stay two commands.

**6. The deferred values** (SATELLITE_ARGUMENTS Phase C and D, B7–B15). Built and
answering on 0058: `arguments.memory.free`, `.memory.used`, `.directory`. Still refused
on 0058: `arguments.memory.swap.*`, `.directory.history`, `.memory.system.reserved` (S110)
and `.system.stack` (S201). None is a flag, so none is a register bit.

### Two number collisions to fix — **not renumbered, ruled 2026-10-09**

**M34 and M35 are each used twice**, found 2026-09-18:

| number | in Part 3 | and in Part 4 |
|---|---|---|
| **M34** | optimizing the `--rebuild` command | *the names a session has used, `satellite.access` and `satellite.help`* |
| **M35** | Optimizing The Interpreter | *a spacesuit takes a supertype, and `satellite.protected` takes arguments* |

Four different milestones, two numbers. **The M34 pair is the one that matters**,
because they were written as related: `access` is bit 0 of the first M34's register, and
was to be the valve of the last-known store the second M34 described (today that store
is NEW_MILESTONES.md's NM-3, and the bit decides where a block's names end). A person
reading "M34" in a commit message cannot tell which is meant. satl's own text names three of the
four: the supertype M35 in its refusals (`bytecode/suit_scan.cpp`) and check.sh, the
optimizing M35 in `bytecode/expression.cpp`, and the access M34 in
`bytecode/access_calls.hpp` and in `satellite.help`'s help text.

Renumbering was the author's call: the two in Part 4 came first by date (2026-09-17),
and the two in Part 3 came first in the file. **Ruled 2026-10-09:** neither number is
renumbered. satl's refusals, check.sh and the help text name M35 (the supertype) and
M34 (access), and all of those would have to change too (the author, answering the
editors' questions: *"looks good"*).

## M35 -- Optimizing The Interpreter

we are going to rebuild the interpreter as we rebuilt the features to be a switch-hierarchy of everything with pathways, so that the entire interpreter is a switch-hierarchy of different cases and different switches, this is the very fastest way I can think of to build this interpreter, and it's way faster than it will ever need to be.

---

# Part 4 — asked for on 2026-09-17, after the prompt, and since

(This was a second "Part 3" until 2026-10-09.)

## M34 — the names a session has used, `satellite.access` and `satellite.help` — **access BUILT 2026-09-26; help at the prompt only; the store not built**

(the author, 2026-09-17) *"there is satellite.access(object_name) so before we do
this, we need to keep a list of the names of every object that we use and their
type, this is used by satellite.help(object_name) to bring up the methods on that
type, this logic we are keeping from 003. the satellite.help(object_name) code
doesn't necessarily need to be fast, but it needs to be coded in a way that we can
work on it, so anytime we add a type, then that type's methods need to be
registered within some directory structure"*

**ANSWERED 2026-09-18 — they are TWO things, and the milestone had them as one.**

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

**`access` may also print what `help` would** -- the author allows it -- so a person who
asks about a name gets the object AND the type without asking twice. (Not built: on 0058
`access` prints no help text.)

**What is true on 0058:**
- **`satellite.access(name)` is built** (`1 31 1`, 2026-09-26, `fdc586b`). It answers text
  about a name in scope -- its type, its value, the satellite.legal place it is legal in,
  and for a container or a spacesuit object the ways into it -- read where the name is
  now, not from a store (`bytecode/access_calls.hpp`).
- **`satellite.help()` and `satellite.help(topic)` answer at the prompt** (since
  2026-09-22): `satellite.help()` lists 31 topics from `satellite.help/`
  (`satl/prompt_help.cpp`). `satellite.help(name)` looks up a TOPIC by name, never a
  declared name's type: for a declared string `zq` it says "there is no help on zq yet".
  Inside a program `satellite.help` is refused (S210).
- **The last-known store** -- every name's last value, readable after the program stops
  -- **is not built.** It is NEW_MILESTONES.md's NM-3 now (`satellite.access(filename)`).
  The switch that was to be its valve, `arguments.access` (bit 0 of the feature
  register, `249d748`, 2026-09-18; on by default, and it lasts in config.ini), decides
  something else today: while it is on, a name declared inside a block stops being legal
  at that block's `}` (NM-2a, `bytecode/satellite_legal.hpp`).
- **The directory structure he asked for:** `words/words.tsv` numbers every method of
  every type, but the folders `satellite-numbers/<the word>/` are gone (2026-10-07), and
  the help text is a second list, written by hand (`satellite.help/<topic>/help_text.txt`):
  adding a type's methods to words.tsv adds nothing to help.

**Decisions:** whether the register is per session, per capsule or per
program (today `access` reads the name in scope); whether `help` with no argument lists
the types (today it lists the topics); what `access` answers for a name nothing declared
(today the checker refuses the line, S201). Each "today" was what the build chose, not his
ruling. **Ruled 2026-10-09:** keep what the build chose. `access` reads the name where it
is now, in scope; `help` with no argument lists the topics; `access` of a name nothing
declared is refused (the author, answering the editors' questions: *"looks good"*).

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

**On 0058:** a spacesuit extending another SPACESUIT is built (M8, 2026-09-22), and so is
the grammar that declares one, so what this waits on is the three decisions below. A
BUILT TYPE in the brackets is refused, naming this milestone: "a spacesuit extending a
built type is MILESTONES M35, certain and not built yet: what it means is still the
author's to decide" (S210). `satellite.protected(value)` is refused as parked (S210,
naming M35). `satellite.supertype` has no row in words.tsv; a line using it is refused
(S110).

**Decisions:** what `satellite.protected(args)` takes, exactly; whether a
supertype must be a built type or may be another spacesuit (today another spacesuit
works); what `satellite.supertype.<name>` is when the supertype has no such parameter.

## M36 — the console's colour and 003's screen words — **BUILT 2026-09-23**, `dfdc64b`

**What runs.** 003's M30 ("what ncurses does, without ncurses" -- 003 never linked
ncurses) and M14's screen words: `display`'s `end= foreground= background= bold=
italic=`, `satellite.console.input()` and `input(prompt)`, `.width`, `.height`,
`.clear()`, `.home()`. And, at the author's *"build 004 differently then, so every
single one of those errors is legal"*, what 003 refused: `"OK".foreground(c)`,
`s.background(c)`, `satellite.console.foreground(c)` / `.background(c)`,
`satellite.terminal.foreground(c)` / `.background(c)`, and
`input(prompt, foreground=c)`. bytecode/console_style.hpp says what each means.
The record is PROGRESS.md section 1, "Colour on the console".

**Still owed:** `satellite.console.input(prompt, target)` (`1 5 4`) and
`satellite.console.typed()` (`1 5 5`) are numbered and refused (S210 on 0058). Regions,
`line=` / `column=` and the alternate screen were 003's plan and never built there
either. A stack overflow on a thread other than the one that changed the terminal's
colours, and Ctrl-Z, leave them changed until satl ends (so `console_style.cpp` says;
not run). **The author's to overrule:** a coloured string holds its codes everywhere
(display leaves them out of a pipe -- and out of a file, on 0058); a see-through colour
is refused; `()` puts back the terminal PROFILE's colours, not a theme a shell set with
the same codes.

## M37 — satl for every processor, and `satl-cpu-level` to choose — **BUILT 2026-09-23**, `0d875d1`

The author, 2026-09-23: *"let's build more satl.cpu things for cpu's that I don't
currently have ... we just can't test the latest stuff, but we can still compile for
it"*, then *"if there are 137 targets, we could technically just build all targets, and
alter satl-cpu-level to check for them"*. 003 built two (baseline, haswell) and chose
between them at install; 004 built one.

`make cpus` builds the ordinary satl and `build/satl-cpu-level` first, then one satl
(its words inside it since 2026-10-07; until then with 62 libraries) for each of **53
processors** into `build/cpu/<processor>/`, and last runs the chooser. `make CPU=haswell`
builds one. `build/satl-cpu-level` prints the build this machine runs best (`haswell`
here). make_support/055-cpus.mk has the whole argument; the record is PROGRESS.md section
1, "satl for every processor, and `satl-cpu-level`".

| clang 24 names | build a 64-bit satl | distinct instruction sets | built |
|---|---|---|---|
| 136 | 79 (the rest are 32-bit chips and `__builtin_cpu_is` spellings) | 54 | 53 + the ordinary build (plain x86-64) |

(Measured 2026-09-23. Checked again 2026-10-09 with the same clang 24: two of
055-cpus.mk's folds are not one set -- `btver1` needs `__SSSE3__` beyond `amdfam10`, and
`c86-4g-m8` needs `__SHSTK__` beyond `c86-4g-m7` -- so those two chips have no build of
their own and run the smaller one.)

**Chosen by what a build needs, never by a name** (003's rule: *"check what you compiled
for; compile for what you check"*): each build carries `needs`, the macros `-march` made
the compiler define, and the chooser picks the runnable build whose deciding list is
longest. A need it cannot ask about makes a build unrunnable -- never a crash; on
2026-09-23 that was AVX512BMM, so **znver6 is never chosen** until a compiler can ask.
**Known:** `make clean` removes `build/cpu/` (about 13 minutes to make again, 2026-09-23);
`make cpus` installs nothing; the fingerprint has never covered the licences, the window
data or the two scripts that generate them; and the chooser's note that satl calls no
intrinsic is stale (`display/printing_satellite.cpp` calls `__builtin_ia32_pause()` since
2026-09-26; PAUSE is baseline x86-64). On 2026-10-09 `build/cpu/` is two weeks old:
`build/cpu/haswell/satl` is REVISION 08 BUILD 0024, beside 62 libraries nothing reads.
**Tested** (2026-09-23): the chooser against all 53 `needs` lists and against /proc/cpuinfo's own flags (ADX, RDSEED, PREFETCHW, SSE4A absent here, LAHF/SAHF present); and the whole suite against `build/cpu/haswell/satl` passed 783 of
786; the three were the help folder, which pass since each processor's build carries a
link to `satellite.help/`. **Not testable here:** anything this Xeon lacks -- raptorlake
on the author's 13700K; AVX-512, AMX and APX builds only under an emulator (Intel SDE),
which is not installed.

**Still the author's:** whether an install should put the chosen build into `~/.satl`
(today `make install`, or a bare `make` answered yes, installs the ordinary build --
never a processor's, 080-install.mk); and whether `arguments.cpu.architecture` should
answer satl-cpu-level's word (`raptorlake` on the 13700K) instead of 003's two
(`haswell` / `baseline`). On 0058 it answers `haswell` here.

## M38 — a program's own threads (THREADS.md T1) — **BUILT 2026-09-23**, `796f175`

The author, 2026-09-23: *"I meant let's build satellite.variable.thread my_thread =
satellite.thread.new(capsule_name(args))"*, then *"we need my_thread.stop(),
my_thread.start(), and my_thread.join()"*, and *"my_thread.wait() as another name for
.join()"*.

Words `1 6 13` and `1 23 1` came from 003's tables; `.stop()` and `.wait()` are new rows,
`1 6 13 3` and `1 6 13 4` (`bytecode/thread_calls.cpp`). A thread runs on its own OS
thread with a 128 MB stack and its own copy of MachineState. THREADS.md (T1) has what
each word does, what made the interpreter safe for two walkers, the fresh reader's nine
defects (all fixed) and the tests: `tests/threads.satl`'s **8 threads × 200 activations
gives 1600 of 1600 right**, and the author's 003 `thread_test.satl` prints
`HELLO, WORLD!`. There are 22 `tests/threads_*.satl` programs today (this entry said
twelve); S725 THREAD_CANNOT_START and S726 THREAD_STOPPED have no test of their own.
M38's share-nothing rule was replaced the same night by M39: only a window is refused
now (S727).

**Still the author's:** T2, sharing with `.lock()`, as he settled it: *"by default it's not
on, so the programmer has to lock everything themselves"*. It has two proposals of mine
waiting on him: a check mode for unlocked sharing, and a lock that lets go when its
capsule ends. Also still his: whether `satellite.library` becomes writable at run time,
and T3, windows on threads.
(T2 was built that night as M39; the two proposals and the other two are still open.)

## M39 — the author's lock: objects and files shared by threads, `.lock()` locks only writes (THREADS.md T2) — **BUILT 2026-09-23** `ebfce50`

(`ebfce50` was committed at 00:04 on 2026-09-24.)

The author, 2026-09-23, on M38's share-nothing threads: *"this was supposed to be done like
completely differently than how you built it, we were locking objects, at the users
discretion ... it only turns the lock on when something goes to write to that object"*.

Objects and files are **shared** with threads -- handed over, run as
`satellite.thread.new(obj.call_x())`, a spacesuit's own capsule by its bare name, or
answered by `join()`. **Every object and file has a lock, off.** `.lock()` turns it on
and `.unlock()` off. While it is on, a statement that writes the object holds it for that
statement, and one that only reads waits only while a write is happening (and, since
`8f8804e` on 2026-09-24, while a writer waits: writers go first). While it is off,
nothing is taken: one relaxed load per statement of a capsule running on an object.
`satellite_object/object_lock.hpp`; THREADS.md T2 has the rest.

**Measured** (2026-09-23): four threads each adding 1 to one field 5,000 times gave
**20,000** with `.lock()`; without it they gave 12,528, 12,691 and 12,179. Four threads
each appending 1,000 items gave **4,000** with `.lock()`. A locked file written by two
threads gave **1,000 of 1,000** lines. (`tests/threads_lock.satl` holds the first two;
the file one is in no test.)

**Still open:** `satellite.library` writable at run time (still S250,
LIBRARY_VALUE_IS_FIXED); the offered check mode for unlocked writes. A circle of two
locks waited on (003's S1408) **is caught since 2026-09-24** (`8f8804e`): the line stops
with S728 WAIT_NEVER_ENDS instead of the program freezing (`object_lock.hpp`,
`tests/threads_lock_circle.satl`).

---

# The author's open decisions — not milestones, but they block them

| | what | blocks |
|---|---|---|
| **D0.1** | which value of `arguments.satc` / `satb` means "never build" -- **ANSWERED 2026-09-16: DEAD.** *"we threw away satc and satb in favor of all 16-bit"* | — |
| **D1.1** | with `satc = 0`, run line 1 before the file is converted? -- **ANSWERED: DEAD**, same reason: there is no `.satc` | — |
| **D3.1** | 32 bits a character everywhere, or only in the `.sati`? -- **ANSWERED 2026-09-16:** *"32-bits only when we use the number 40000 as a 16-bit code"* -- everything is 16 bits, and a code of 40000 (`wide_token`) says the next two codes are one 32-bit integer. **Done 2026-09-17:** the lexer writes a character above U+FFFF as `wide_token` and two codes, `wide_run_32_token` is retired, and `satellite_string` holds it the same way | — |
| **D9.1–2** | polymorph: re-inclusion, `args` -- **open** | M9 |
| **D9.3** | a class declared twice -- **ANSWERED 2026-09-16:** *"a class declared twice is an ERROR: name collision"* | — |
| **D11.1** | infinity's arithmetic -- **ANSWERED 2026-09-16:** *"we answer what we can, and give an error on what we can't"*. The rest of that day's answer (a `satellite_float` multiplier, 4096 digits) changed on 2026-09-18; see M11 | — |
| **D12.1** | the parallel-group syntax in the numbered file -- **open** | M12, M30 |
| — | adopt TBB for the runners? -- **ANSWERED 2026-09-16: NO.** The warm threads (1024 then, cores x 2 since 2026-10-03) convert the program and then stay warm, doing nothing yet; *"we are doing something different later"*. See M12 | — |
| — | the leading-slash rule: filesystem root, or program root? -- **ANSWERED 2026-09-16:** *"program root first, then filesystem root when it's not found, and if it's not found in either, we report file not found, and we keep a cwd for files that are included... so the files directory becomes the cwd for each file"*. (The author wrote it as D12.1; D12.1 is the parallel-group syntax and is still open.) **`include` does not do this:** `bytecode/include_shape.cpp` keeps another rule of the same day, also his -- *"satellite.include(/test) would actually just be ./test"* -- four spellings and no looking on the disk, so `include(/test)` is `test.satl` beside the including file and nothing else is tried. Which one stands was his. **Ruled 2026-10-09:** `include(/test)` keeps meaning `./test` beside the including file -- four spellings and no probing -- and the *"program root first, then filesystem root"* answer is about something else, not `include` (the author, answering the editors' questions: *"looks good"*) | — |
| — | a number argument where a string is expected -- **ANSWERED 2026-09-16:** *"just convert the number to the string and run that piece, obviously the programmer meant convert to string, but record the warning in satellite.log"*. The warning is S020 in `satellite.log`, built with M5 on 2026-09-25 | — |

**The recursion depth -- REOPENED AND BUILT 2026-09-25 (`8c8dc35`), on the author's
word:** a capsule calling itself mid-body moves onto a fresh stack segment when less than
1 MiB is left (`machine/stack_segments.hpp`) -- 1,000,000 deep ran in 5.3 s (measured
2026-09-25). Only the walker's self-calls use the segments (`program_walk.cpp` is the one
file that includes them); a nested expression still recurses through C++ (`evaluate_at`,
`expression.cpp`). For nesting inside ONE statement, what the author accepted on
2026-09-16 still holds: the walker's ~27,000 recursion depth, an order of magnitude past
any recursion a person writes, and the fix a real rewrite of the walker. What would
reopen it is a real program that runs out, and generated code is the likely source since
QUAD writes satellite. (Not re-run: no deep-nesting probe is run.)

---

# What is NOT a milestone, and why

**"Every combination of every type."** It looked like the largest item here and it
is not an item at all. With N types there appear to be N×N pair functions to
write; there are not, because `satellite_number` is a hub every type converts
through. `.bin` on a string is `string_to_number` then `number_to_binary` — two
hops through functions that already existed and are already checked, and no
`string_to_binary` is ever written.

- **conversions** cost N in and N out, not N×N — `object_convert.cpp`
- **operations** are written once per type. Their numbers in `words.tsv` are frozen, but
  the list is not: a new word takes the next free number at its end (365 rows on
  2026-09-17, 548 today)
- **aliases** cost nothing: several spellings lex to one code, and nothing after
  the lexer learns there was more than one
- **chaining** is a loop, and costs one local however long the chain

The only genuine combination work left is **M13's position**, and that is one more
field on the state the loop already carries (the 2026-09-16 estimate).

---

# Found 2026-10-07 to 2026-10-09

Found or re-checked while the documents were checked; only the first was in this file before (M14). What satl does today,
each run on BUILD 0058. None is fixed, and what to do about each is the author's.

- `{1, 2}` alone on a line ends the capsule silently: the lines after it never run, and
  satl exits 0 (M14).
- `s.find("b").add("x")` alone on a line runs nothing and says nothing: `s` is unchanged
  and satl exits 0. As a value, `w.find("world").add("!")` answers `6!` (M13).
- `5 / 4` displays `1`: two whole numbers divide to a whole number, cut toward zero
  (`number_arithmetic.hpp`, which says to revisit this when the fraction lands).
  `5.0 / 4` displays `1.25`, and `5/4`, touching, is the fraction `5/4` (M20).
- A line that is only `satellite.<word>(x)`, for a word that does not exist --
  `satellite.history(x)`, `satellite.cxx()`, `satellite.nonsense(x)` -- is refused (S110)
  with a sentence about `satellite` itself: "satellite is the first legal object of every
  place in satellite.legal, and it is immutable -- nothing else may be named satellite,
  and it is never changed or deleted". Inside an expression the same call says "no
  capsule named history" (M10, M20.B).
