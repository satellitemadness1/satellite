# satellite-004 — SATELLITE_ERROR

The error system: what an S-code is, what a report looks like, which pieces are
built, and what each unbuilt piece costs.

Written 2026-09-18 from the author's briefs, which are kept whole in Parts 1 and 7.

**PART 7 IS THE FULL REPORT** — thirteen sections holding everything the interpreter
knows, four of which need nothing built. **PART 10 IS THE FEATURE REGISTER** — one
binary and `satl --rebuild`, measured, and the reason the whole system is free when
it is off. **PART 11 IS WHAT WE ARE NOT DOING.**

**THIS IS NOT `ERROR.md`.** That file is the list of *bugs in 004* — things that
give a wrong answer, and where they are. This one is the *machinery a failure is
reported through*. A bug goes in ERROR.md; the shape it is told in goes here.

---

# Part 1 — the brief, as the author wrote it

> and don't run the interpreter without /home_dir/.satl/config.ini say "cannot
> find /home/username/.satl/config.ini" please reinstall satellite, or create a
> config.ini there... and well, let's build default config values so the
> interpreter still runs without the config.ini file, we'll set some default
> config values, and then our critical error report will look like this
>
> ```
> ------------------------------------------------------------------------ (80 chars)
> SATELLITE CRITICAL ERROR REPORT
> ------------------------------------------------------------------------
> S0721: BASIC_ERROR_NAME
> FULL_ERROR_DESCRIPTION_HERE
>
> directory: path
> syntax: code here
>                      /\
>                    however you had 003
>
> ------------------------------------------------------------------------
> ```
>
> and that ends the critical error report we will give them, it'll look like it
> came out of some space kinda novel or something!

> and then before the last ------- line, add anything else you want to add that
> we can, we could have super huge error reports, in fact, let's build a
> SATELLITE_ERROR.md whenever your done doing what your doing, and we'll build an
> entire error system...

---

# Part 2 — what is built, as of 2026-09-18

- **The renderer is built and running** —
  [satellite/machine/critical_report.hpp](satellite/machine/critical_report.hpp).
  Header-only, so a numbered library can raise a report too (each library is
  compiled from exactly one `.cpp`, so anything shared has to be inline).
- **`CriticalReport` carries** `code`, `name`, `description`, `directory`,
  `syntax`, `caret_at`, `caret_note`, and `notes` — the author's "anything else"
  room, kept last.
- **Four callers exist**: `S0721`–`S0724`, all of them about config.ini and the
  feature register. Part 4's table says where each one lives.
- **THE FEATURE REGISTER IS BUILT** —
  [satellite/config/feature_register.hpp](satellite/config/feature_register.hpp),
  fourteen bits, and `satl --rebuild`
  ([config/rebuild.hpp](satellite/config/rebuild.hpp)) composes them into one
  binary in config.ini. **F1–F4 of Part 12 are done**; F5, the split walker that
  makes the features free, is the optimisation the author is milestoning himself.
- **Everything else still reports the old way** — `report_error()` in
  `machine/machine_state.cpp`, one line to stderr:
  `[satellite] satl(run): ... (machine_code: 34 setting_is_not_a_flag)`.

## The two numbering schemes, and they are not rivals

| | what it is | where it lives | range |
|---|---|---|---|
| **machine code** | what a function *answers*; becomes the exit status | `machine/machine_codes.hpp` | 0–254, 130 is Ctrl-C |
| **S-code** | what a *person* is shown in a report | this file, Part 4 | S0001– |

A machine code is the interpreter talking to itself and to the shell. An S-code
is the interpreter talking to a person. **They are many-to-many on purpose**:
`satl_line_not_understood` (13) is a hundred different things a person needs a
hundred different sentences for, and one S-code may be reachable from more than
one machine code. A report carries both — the S-code in its header, the machine
code in the exit status.

**003'S S-CODES ARE NOT PORTED WHOLESALE.** 003 numbered its own (`S0714`,
`S0723`, `S1001`) against a different set of refusals. Where a 004 refusal *is*
003's refusal, it takes 003's number so a person moving between them reads one
number; where it is new, it takes the next free one in its block. The author's
sketch used `S0721`, which is why `CONFIG_FILE_MISSING` is S0721.

---

# Part 3 — the `syntax:` row, and what it costs

The report has a `syntax:` row and a caret. **Nothing fills them in yet**, and
this section is why that is a small job rather than a large one — it was measured
against the tree on 2026-09-18 rather than guessed.

- **The file name is already there.** `BytecodeFilenames` is a
  `std::vector<std::string>` kept row for row with the registry
  (`bytecode_registry.hpp`), and **one row is one FILE** — not one line. So
  `filenames[which_row]` is the file, free.
- **The line number is a count, and it is exact.** `line_end_token` (`0x0100`) is
  put at the end of every tokenised line, and `add_file_to_bytecode_registry`
  splits the source on newlines and tokenises **every** line — blank lines and
  comment lines included. So the number of `line_end_token`s before a position
  `at` is that position's 0-based source line, with no drift.
- **The line's TEXT is not kept, and should not be.** The registry holds codes,
  and the header says why: *"a row stays nothing but codes and a name never has
  to be spelled in tokens to be carried."* **Re-read the file at report time.**
  An error is rare and the path is already failing, so one `open`+`getline` costs
  nothing — and it shows the person's *actual* text, not a reconstruction from
  tokens that would quietly differ in spacing from what they wrote.
- **The caret column is the one real piece of work.** A token's position in the
  stream is not a column in the line. Either the lexer records a byte offset per
  token, or the reporter re-tokenises the one line it re-read and counts. The
  second costs nothing at run time and adds no field to the hot path, and is the
  recommendation.

---

# Part 4 — the S-code register

**THE NUMBER SAYS HOW BAD IT IS.** The author, 2026-09-18: *"S00 is stuff that
doesn't stop the interpreter, so you have 99 warnings to use, then S01 stops the
interpreter on something stupid, then S02 is a little bit more serious"* — and
*"we don't fill every single one out, we guess what each error is... in
severity"*. So `Sxxx`, 000 to 999, and **the hundreds digit is the severity**.

| band | means | exits |
|---|---|---|
| **S0xx** | a WARNING — **the run carries on** | 0 |
| **S1xx** | stops, and it is something simple: a missing line, a typo, a word satl does not take | |
| **S2xx** | stops: a NAME — not declared, declared twice, not built yet | |
| **S3xx** | stops: what two things ARE — types that do not meet | |
| **S4xx** | stops: arithmetic and positions | |
| **S5xx** | stops: files and directories | |
| **S6xx** | stops: the settings | |
| **S7xx** | stops: the machine could not answer | |
| **S8xx** | stops: something outside the program — Ctrl-C, a closed pipe | |
| **S9xx** | **satl itself is in trouble** | |
| **S999** | **the top of the scale: no memory** | 48 |

**S999 IS "NO MEMORY" AND NOT "NO LIBRARIES", ON THE AUTHOR'S OWN CORRECTION**
mid-sentence: *"999 will be... cannot load the C++ libraries, that is the worst
possible error, well, the worst possible error is that we cannot set the memory
for them"*. He is right, and the difference is exact: **a library that will not
load leaves an interpreter that runs and cannot do everything; no memory leaves
no interpreter.** So `S980` is a library that loaded and did not describe itself,
`S901` is libraries that did not load, and `S999` is the one above them.

**GAPS ARE LEFT ON PURPOSE.** `S101`, `S110`, `S120`, `S130`, `S140` — a band is
filled at its tens, so a refusal added beside an existing one has somewhere
obvious to go without renumbering anything.

**WHAT THIS COST: NOTHING, AND THE EARLIER NOTE HERE WAS WRONG.** This file said
the renumber cost "003 compatibility" -- that a person moving between 003 and 004
would no longer read one number for one refusal. **There are no such people.**
The author, 2026-09-18: *"We do not have users yet, this is all sitting BEFORE a
release campaign"*. 004 is what ships; 003 is where it came from.

**AND THAT IS THE GENERAL RULE WHILE THIS IS PRE-RELEASE.** Every "never
renumber, never rename, a number somebody wrote down has to keep meaning what it
meant" argument in this file is protecting users who do not exist yet. Before
release those changes are FREE, and the time to make them is now -- after the
first advert runs, every one of them has a price. `S0716` becoming `S420` cost
nothing at all.

## Assigned

| S-code | name | machine code | where |
|---|---|---|---|
| **S0721** | `CONFIG_FILE_MISSING` | *(notice — the run carries on)* | `structured-library.cpp` |
| **S0722** | `REGISTER_NOT_WRITTEN` | 32 `config_file_unwritable` | `config/rebuild.hpp` |
| **S0723** | `REGISTER_IS_STALE` | *(notice — the run carries on)* | `structured-library.cpp` |
| **S0724** | `REGISTER_NOT_READABLE` | *(notice — the run carries on)* | `structured-library.cpp` |
| **S0725** | `NO_THREAD_CEILING_STATED` | 36 `machine_fact_not_read` | `config/run_config.hpp` |
| **S0726** | `CAP_ABOVE_THE_CEILING` | 37 `setting_out_of_range` | `config/run_config.hpp` |
| **S0727** | `MACHINE_CONF_NOT_WRITTEN` | 38 `machine_conf_unwritable` | `config/run_config.hpp` |
| **S0201** | `FILE_HAS_NO_INCLUDE` | 10 `satl_file_missing_satellite_include_satellite` | `machine/s_codes.hpp` |
| **S0202** | `FILE_HAS_NO_MAIN` | 11 `satl_file_missing_satellite_main` | `machine/s_codes.hpp` |
| **S0203** | `FILE_HAS_NO_RETURN` | 12 `satl_file_missing_satellite_return_satellite` | `machine/s_codes.hpp` |
| **S0401** | `LINE_NOT_UNDERSTOOD` | 13 `satl_line_not_understood` | `machine/s_codes.hpp` |
| **S0402** | `NOT_BUILT_YET` | 14 `not_built_yet` | `machine/s_codes.hpp` |
| **S0501** | `NAME_NOT_DECLARED` | 25 `name_not_declared` | `machine/s_codes.hpp` |
| **S0502** | `NAME_DECLARED_TWICE` | 26 `name_declared_twice` | `machine/s_codes.hpp` |
| **S0601** | `TYPES_DO_NOT_MEET` | 27 `types_do_not_meet` | `machine/s_codes.hpp` |
| **S0728** | `SETTING_IS_NOT_A_FLAG` | 34 `setting_is_not_a_flag` | `machine/s_codes.hpp` |
| **S0729** | `WORD_TAKES_NO_ASSIGNMENT` | 35 `word_takes_no_assignment` | `machine/s_codes.hpp` |
| **S0730** | `CONFIG_FILE_UNREADABLE` | 33 `config_file_unreadable` | `machine/s_codes.hpp` |
| **S0801** | `DIVISION_BY_ZERO` | 22 `division_by_zero` | `machine/s_codes.hpp` |
| **S0802** | `ANSWER_IS_NOT_WHOLE` | 24 `answer_is_not_whole` | `machine/s_codes.hpp` |
| **S016** | `CONFIG_ROW_NOT_UNDERSTOOD` | *(notice -- the run carries on; 2026-09-25, the author's A9: each unknown `config.ini` row and each line that is not key = value, by line)* | `structured-library.cpp` |
| **S020** | `NUMBER_TAKEN_AS_TEXT` | *(warning, satellite.log only -- the run carries on; M5, 2026-09-25, in the Part 4 numbering the rows above predate)* | `machine/s_codes.hpp` |

**SINCE 2026-09-25, S0203/S103 IS ALSO "main's last line is not satellite.return(satellite)"**, said
with a caret on main's last statement (program_check.cpp); and machine code 64
`program_returned` is `satellite.return(satellite)` reached anywhere -- never reported, it unwinds
every frame and satl exits 0.

**S0201–S0203 CARRY NO CARET, AND THAT IS NOT A GAP.** They are about a whole
file, and a line that is MISSING has no position to point at — so they take
`raise()` and not `raise_at()`. What the report gives them that the old single
line could not is **room to print the exact line to type**. S0201 is the
commonest mistake in the language and its old message said what was wrong
without ever saying what to do about it.

**S09xx DRIFTED, AND IT IS RECORDED RATHER THAN REPAIRED.** The block was
written for strings; the file work landed S0901–S0905 in it before anything
string-shaped was numbered. **Renumbering them is refused by this file's own
rule** — never renumber, because a number a person has written down has to keep
meaning what it meant, and those five are already in `check.sh`. So the block
holds both, files first, and the strings continue from S0906. The cost is that
S09xx no longer says one thing; the cost of the alternative is a number that
changed meaning.

**EVERY REFUSAL IN 004 NOW HAS AN S-CODE, as of 2026-09-18.** Every machine code
that means a failure has a row in `s_codes.hpp`. The six without one —
`success`, `error`'s successes, `number_vector_defined`,
`satellite_loading_successful`, `successfully_loaded_satl_file`,
`display_error`'s siblings — are reports and not failures, which Part 2's own
table already said.

**S0000 `REFUSED` IS THE FALLBACK AND IS NOT A FAILURE.** A machine code with no S-code yet still reports with the file, the line and the caret; S0000 says the number is owed rather than pretending the refusal is nameless.

**003'S STRING REFUSALS ARE DELIBERATELY UNASSIGNED.** 003 numbered them in S07xx, which is 004's *settings* block. Taking 003's number puts a string error in the settings block; taking a 004 number breaks the promise that a person moving between them reads one number. **Red note: the author picks.**

**S0724 IS THE FIRST REPORT IN 004 WITH A WORKING CARET.** A damaged
`features = bZZZZ` prints the row and points at it, which is Part 3's `syntax:`
machinery proven on a value the reporter already has in hand — no line number, no
re-read, no column arithmetic. The program-source version is still owed.

## Proposed, 2026-09-18 — SEVERITY IN THE NUMBER ITSELF. Not built.

The author: *"S00 is stuff that doesn't stop the interpreter, so you have 99
warnings to use, then S01 stops the interpreter on something stupid, then S02 is
a little bit more serious"* — the number ordered by **how bad it is**, rather
than by which part of the interpreter it came from.

**It is a good idea and it is worth writing down before it is lost.** Today's
blocks say WHERE a refusal came from; this says WHAT IT COSTS YOU, and the second
is what a person wants first. `S0xx` and you can carry on; `S9xx` and you cannot.

| band | means |
|---|---|
| `S0xx` | a warning — **the run carries on**. 99 of them, S000 the least important |
| `S1xx` | stops, and it is something simple: a typo, a missing line |
| `S2xx`–`S7xx` | stops, escalating |
| `S8xx` | stops, and something is wrong beyond the program |
| `S9xx` | the interpreter itself is in trouble |

**WHAT IT WOULD COST, measured 2026-09-18 rather than guessed:** eight live call
sites (`structured-library.cpp` ×3, `rebuild.hpp`, `run_config.hpp` ×3,
`include_shape.cpp`), every row in `s_codes.hpp`, three comments in `check.sh`,
one in `065-config.sh`, and one in a test program. **That is small.** What is
not small is that `S0721` is in the author's own first sketch of the report, and
`S0716` is 003's number for `text_not_found` which 004 deliberately kept so a
person moving between them reads one number. **Severity ordering and 003
compatibility cannot both be true**, and that is the real decision — not the
renumbering, which is an afternoon.

## Recorded and DELIBERATELY NOT BUILT — the interpreter that checks itself

The author, same message: *"999 is like, the machine code is corrupted that the
interpreter consists of, so we do a scan of the interpreter itself... we check
for certain functions and certain compiled C++, like an anti virus but for the
interpreter, we are feature creepin' HARD"* — and then, unprompted: *"maybe
that's taking the error thing a bit too far"*.

**He is right, and the reason is worth keeping so nobody re-proposes it.** A
checksum of satl's own code that refuses to run when it does not match is an
antivirus, and every antivirus's real cost is its FALSE POSITIVES. This one's
false positive is *"your interpreter will not start"* — and the things that would
trigger it are all legitimate: a rebuild, a different compiler, a distribution's
patched libstdc++, a static build against a different libc,
`LD_RUN_PATH` baking a different RPATH. Satellite already builds four ways on
this machine alone.

**The useful 5% of it, if it is ever wanted:** satl already knows its own build
fingerprint (`020-version.mk`, `arguments.build`), and `--version` could print a
hash of the binary it is running. That is *reporting* what you have, which is
useful in a bug report, rather than *refusing* what you have, which is the part
that bites. One is a line in the report; the other is a project.

## Owed — the refusals that exist and have no S-code yet

Each of these already answers a machine code and already prints the one-line
form. They are the first candidates for the report, in the order a person is
likeliest to meet them:

| machine code | what a person did | block |
|---|---|---|
| 34 `setting_is_not_a_flag` | `arguments.access = 5` | S07xx |
| 35 `word_takes_no_assignment` | `satellite.console.display = ...` | S07xx |
| 25 `name_not_declared` | used a name with no `satellite.variable` line | S05xx |
| 26 `name_declared_twice` | two declarations of one name in one capsule | S05xx |
| 27 `types_do_not_meet` | `"a" - "b"`, or a number given to a string | S06xx |
| 22 `division_by_zero` | a divisor of 0 | S08xx |
| 24 `answer_is_not_whole` | `2 ^ -1` | S08xx |
| 14 `not_built_yet` | a word that is numbered and has no library | S04xx |
| 13 `satl_line_not_understood` | everything the checker has no shape for | S04xx |
| 32 `config_file_unwritable` | a setting could not be saved | S07xx |
| 33 `config_file_unreadable` | config.ini is there and unreadable | S07xx |

---

# Part 5 — the milestones

**E1–E6 make the report reachable.** E7–E12 move the refusals onto it.

## Phase 0 — the position

**E1-E6 ARE DONE, 2026-09-18 (`bb64597`, `8c1593c`).** `satellite/machine/source_position.hpp` and `s_codes.hpp`. The `syntax:` row and the caret are filled in, in both halves of the interpreter.

- **E1** — ~~Record a byte offset per token, or re-tokenise one line at report time; pick the second.~~ **Done, and the second — with the LEXER'S OWN offsets.** `tokenise_one_line` takes an optional `offsets` vector and fills it index for index with the row. Every path that runs a program passes `nullptr`; the reporter passes a vector, for one line, after something has already failed. Re-deriving the splitting rules in the reporter was the version that would have drifted.
- **E2** — ~~`line_of(row, at)`.~~ **Done earlier** — `statement_ring.hpp` already had it, for the same reason it exists at all.
- **E3** — ~~`source_line(file, n)`.~~ **Done** — one `open` and one `getline`, showing what the person ACTUALLY TYPED.
- **E4** — ~~`column_of(row, at)`.~~ **Done, and it is exact rather than estimated.** The row is every line tokenised one after another, so the tokens between a line's start and `at` are index for index the tokens `tokenise_one_line` makes from that line's text. It is the same lexer answering the same question a second time.

## Phase 1 — the seam

- **E5** — ~~`raise(CriticalReport)`.~~ **Done**, plus `raise_at()`, which is the one every caller uses.
- **E6** — ~~A report knows its file and position, so a caller passes neither.~~ **Done** — a caller passes the row it is walking and a position, and nothing else. `state.program` and `state.program_files` are two pointers set once after load; the ROW FINDS ITS OWN INDEX BY ADDRESS, because half the walker's functions are handed one row and not which row it is, and threading an index through all of them for a path that runs once a run is a cost on the hot path for a benefit that is not on it.

**NOTHING WAS ADDED TO THE HOT PATH.** No field on a token, no store per statement, nothing kept in memory against the possibility of an error. A refusal carries its own position (`ExpressionContext::refused_at`), written once, on the refusal.

**`stage` SAYS WHICH HALF REFUSED, AND IT IS NOT DECORATION.** `satl(check)` means NOTHING RAN; `satl(run)` means it got this far and stopped. That is the first thing a person needs, before the S-code and before the line.

## Phase 2 — the refusals, one at a time

- **E7** — S07xx: the two setting refusals, 34 and 35. They are the newest and the smallest.
- **E8** — S05xx: `name_not_declared` and `name_declared_twice`.
- **E9** — S06xx: `types_do_not_meet`, which is the one a person meets most.
- **E10** — S08xx: `division_by_zero` and `answer_is_not_whole`.
- **E11** — S04xx: `not_built_yet` names the word and says which milestone owes it.
- **E12** — S03xx: the lexer's `error_token`, which already carries the character it could not read.

## Phase 3 — the room at the bottom

The author: *"we could have super huge error reports"*. `notes` is already there
and already prints; these are what to put in it.

- **E13** — The nearest spelling, when a word is misspelled.
- **E14** — The line above and the line below, when they explain the one that failed.
- **E15** — What 003 did with the same program, where 003 is installed and did something different.
- **E16** — `satl --help <S-code>`: the whole entry for one code.

---

# Part 7 — THE FULL REPORT: everything the interpreter knows

The author, 2026-09-18:

> what all can we pack into SATELLITE_ERROR.md do you think? We can display a
> full error report with every argument that arguments was at even if it wasn't
> active, and we need to build the interpreter so it has access to arguments no
> matter what, only the user doesn't have access to it if it's not defined as one
> of the names, and what else can we pack into the error report? I am imagining a
> HUGE list of values and information so that we shut the interpreter down and the
> user is left with the entire error report, like so they can see EVERY VARIABLE
> every STATEMENT, EVERYTHING the interpreter knows, and we can use this as a
> debug tool anyways, so it will facilitate development

And, in the same breath: *"we are feature creepin' hard"*. He is right, and this
is the creep that pays for itself — **every section below is a thing a person
building the interpreter wants to see anyway**, so the crash report and the
debugger are one piece of work rather than two.

## 7.1 — The one that is already true

**THE INTERPRETER ALREADY GATHERS EVERY ARGUMENT, UNCONDITIONALLY.**
`structured-library.cpp` calls `arguments.gather_config()` and
`arguments.gather(command_line)` at start-up on every run, before anything about
`arguments_active` is consulted. Run `satl --debug` on any program and thirty
rows print. So the author's requirement — *"every argument that arguments was at
even if it wasn't active"* — **costs nothing and needs nothing built**.

**WHICH INVERTS A1–A3 IN SATELLITE_ARGUMENTS.md, AND MAKES THEM CHEAPER.** The
first brief had `arguments_active` gate whether the interpreter *looks for*
arguments at all, for speed. The second makes it gate whether the **user** may
read them: *"only the user doesn't have access to it if it's not defined as one
of the names"*. That is a **visibility gate, not a gathering gate** — one test in
the expression reader, nowhere near the hot path, and the crash report always has
the data because the data was always collected.

## 7.2 — The inventory: what exists, and what each section costs

Thirteen sections. The cost column is the honest one — **free** means the
interpreter already holds it and the report only has to print it.

| § | section | where it comes from | cost |
|---|---|---|---|
| A | **identity** — version, revision, build, compiler, flags, OS, pid, started at, ran for | `version.hpp`, `arguments.*` | **free** |
| B | **arguments** — all ~30 rows, name, kind, value, and WHERE each came from (config / command line / machine) | `Arguments::all()` | **free** |
| C | **the failure** — S-code, machine code, name, description, file, line, column, the source line, the caret | the report itself + Part 3 | small |
| D | **the call stack** — capsule → capsule, innermost first | **needs a frame stack** | medium |
| E | **variables** — every name in every live frame: name, declared type, current value | `VariableTable` per frame | **free per frame**, needs D for all of them |
| F | **variables that are GONE** — every name the run ever held, last type, last value | **the last-known store** | see 7.3 |
| G | **statements** — the last N statements run, innermost frame first | **needs a ring** | see 7.4 |
| H | **the program** — every file loaded, which was an include and by whom, token count each | `BytecodeFilenames`, registry | **free** |
| I | **the words** — 28 of 367 have a library; whether the failing word had one, and which are owed | `FunctionTable` | **free** |
| J | **the machine** — memory total/free/used, swap, cores, threads, disk, page size | `arguments.cpp` readers | **free** (Phase C of ARGUMENTS) |
| K | **this interpreter** — resident memory, threads started/parked, how much of `threads_max` used | `/proc/self/statm`, `StartupThreads` | small |
| L | **the config** — config.ini's path, whether it was there, every key, and which settings took defaults | `config_file.hpp` | small |
| M | **what to do next** — nearest spelling, the milestone that owes the word, `satl --help <S-code>` | E13–E16 | medium |

**FOUR OF THE THIRTEEN NEED NOTHING BUILT** and are the whole of milestone R3.
That is the order: print what is already known first, so the report is useful
before any of the expensive sections exist.

## 7.3 — §F is the last-known store, and that is the good news

**THE CRASH REPORT'S BEST SECTION AND `satellite.access(object_name)` ARE THE
SAME MACHINERY.** SATELLITE_ARGUMENTS Part 4B describes a store keeping *the last
known name, type and value* of everything, so a person can ask about an object
after the program has stopped. That is **exactly** §F, and `arguments.access` —
already built, already lasting in config.ini — is **already its valve**.

So neither feature pays for it alone:

- `satellite.access()` gets its store,
- the report gets every variable including ones out of scope,
- and one `arguments.access = satellite.bool.false` turns off the cost of both.

**AND IT IS BOUNDED BY NAMES, NOT BY ASSIGNMENTS**, which is what makes it
affordable at all — "only the last" means one row per name, so a variable written
ten million times in a loop is ten million writes to one row, not ten million
rows. That bound is the whole reason the author's "only the last" is the right
design and not a limitation.

## 7.4 — §G is the one to be careful with

*"every STATEMENT"* read literally is unbounded: a loop running a billion times
is a billion statements. **A ring buffer of the last N**, not a log.

**THIS IS THE CONSOLE QUEUE AGAIN.** On 2026-09-17 an unbounded queue of
already-rendered output took 5.9 GB in 48 seconds and would have had the machine.
Anything appended once per statement is on the hottest path there is, so §G is
the section that needs its bound decided **before** it ships, and needs measuring
rather than arguing. N wants to be a config.ini key.

## 7.5 — Where a huge report goes

**NOT ALL OF IT TO THE TERMINAL.** A two-thousand-line dump into stderr scrolls
the thing a person needs off their screen, and if they were piping the program
it lands in the pipe. So:

- **stderr** gets the frame — the eighty-column report of Part 1, §A–§C — which
  is what a person reads to know what happened;
- **`~/.satl/reports/<when>-<S-code>.txt`** gets the whole thing, every section;
- and the last line on stderr says where it was written.

That also answers *"we shut the interpreter down and the user is left with the
entire error report"* better than printing it would: the file is still there
tomorrow.

## 7.6 — Three rules the report has to keep

1. **IT MUST NOT FAIL WHILE REPORTING A FAILURE.** The report runs at the worst
   moment there is, and the worst moment includes *out of memory* — which is the
   exact state the console queue put this machine in. A reporter that allocates
   to describe an allocation failure is a reporter that is not there when it is
   needed. **Its buffer is taken at start-up and never grown.**
2. **IT MUST NOT LEAK WHAT IT WAS NOT ASKED TO.** A report is made to be pasted
   into a bug entry. The environment holds tokens, keys and passwords, so
   `environ` is **never** dumped wholesale — named variables only, and a list
   that is written down rather than a sweep.
3. **IT MUST SAY WHAT IT COULD NOT READ.** A section that failed to gather prints
   as that, never as empty. An empty `variables:` and a variables section that
   could not be walked look identical, and one of them is a second bug.

---

# Part 8 — the full report's milestones

**R1–R6 make the report exist and print what is already known.** R7–R12 are the
sections that need building. R13–R18 are the debugger the author wants it to be.

## Phase R0 — the frame, and the four free sections

- **R1** — `ReportBuffer`: taken at start-up, fixed size, never grown. Rule 1 of 7.6.
- **R2** — `--report=short|full`, and a config.ini key for the default. Short is today's eighty-column frame.
- **R3** — §A identity, §B arguments, §H the program, §I the words. All four are printing what is already held.
- **R4** — Write the full report to `~/.satl/reports/<when>-<S-code>.txt`; stderr's last line says where.
- **R5** — §L the config: every key, its value, and which settings fell back to a default.
- **R6** — §J the machine and §K this interpreter. §J lands free once ARGUMENTS Phase C is built.

## Phase R1 — the frame stack

Today a capsule call makes `VariableTable theirs;` with no link to its caller
(`program_walk.cpp`), so **only the innermost body's variables can be reached**.
This phase is what makes §D and §E whole.

- **R7** — A `Frame`: the capsule's name, the file and line it was called from, and its `VariableTable`.
- **R8** — A stack of them, pushed and popped around a capsule call. **Measure the cost of the push before keeping it.**
- **R9** — §D prints the stack, innermost first, the way a person reads a traceback.
- **R10** — §E prints every frame's variables, with each value rendered by the same code `display` uses, so the report and the program agree about what a value looks like.

## Phase R2 — the two stores

- **R11** — §F, the last-known store: one row a NAME — last type, last value, last file and line that wrote it. **This is SATELLITE_ARGUMENTS' store, and `arguments.access` is already its valve.**
- **R12** — §G, the statement ring: the last N statements, N from config.ini. **Bound first, measured second, shipped third** (7.4).

## Phase R3 — the debugger it turns into

The author: *"we can use this as a debug tool anyways, so it will facilitate
development"*. Everything above is collected whether or not anything failed, so
these cost almost nothing more.

- **R13** — `satl --report <program.satl>`: run it, and write the full report at the end whether it failed or not.
- **R14** — `satellite.report()`: a program asks for its own report, mid-run, without stopping.
- **R15** — A SIGSEGV/SIGABRT handler that writes the report. **There is none today** — a crash gives nothing at all — and this is the case a person is most helpless in.
- **R16** — Ctrl-C writes one, so a hung program says where it was hanging.
- **R17** — `satl --report-diff <a> <b>`: two reports, what changed. Two runs that differ by one value is the commonest debugging question there is.
- **R18** — §M: the nearest spelling, and the milestone that owes an unbuilt word.

## Phase R4 — the report as a satellite value

- **R19** — The report as a `satellite.container.arguments`, so a program can read its own state.
- **R20** — `satellite.access(object_name)` answers out of §F's store. **This is the second brief's whole purpose, and by here it is one line.**

---

---

# Part 10 — THE FEATURE REGISTER: one binary, and `satl --rebuild`

The author, 2026-09-18:

> those are things that we actually have to build that will also slow down the
> interpreter slightly, we just have to hide all of them behind
> satellite.variable.bool's so it only slows the interpreter down by a nanosecond
> while it checks all of the values, we could technically hide all of these into
> something that we build -- a satellite.variable.binary where each value in the
> width is a feature, then we can just scan this single binary and it flips on or
> off all of the features, this is how we'll keep the interpreter really fast!

> so we do this... satl --rebuild is the command to rebuild the arguments string,
> so we have to set the arguments how we want them, then run satl --rebuild to
> rebuild the binary string that is stored inside of config.ini, it saves it and
> then it just loads that single value...

**THE IDEA IS RIGHT AND THE REASON IS NOT THE ONE IT LOOKS LIKE.** Measured
2026-09-18, clang 24, -O2, this machine, 2,000,000,000 statements, all eight
features OFF (the common case), two runs agreeing to the third digit:

| how the eight features are gated | ns per statement | over the floor |
|---|---|---|
| **no gates at all** (the floor) | **0.224** | — |
| eight separate `bool`s | 1.81 | +1.59 |
| **one bitmask, eight bit tests** | **1.84** | **+1.62** |
| one bitmask, **one** test per statement | 0.72 | +0.50 |
| **split loop, tested ONCE on entry** | **0.225** | **+0.001** |

Read the middle row twice. **A bitmask tested bit by bit is not faster than eight
separate bools** — 1.84 against 1.81, which is noise. It cannot be: either way
the processor runs eight compare-and-branches, and a branch that is always false
is already free to predict. Scanning one binary instead of eight bools does not
remove the eight tests; it only changes where the bits are stored.

**WHAT THE ONE BINARY ACTUALLY BUYS IS THE ROW BELOW IT.** Because all eight
features live in ONE value, one test can gate ALL of them:

```
    if (flags != 0) { ... the eight checks ... }
```

That is 0.72 ns against 1.84 — **two and a half times cheaper**, and it gets
cheaper the more features are added, because the number of tests stops growing
with the number of features. **That is the author's idea, and it is a good one.**

**AND THE LAST ROW IS THE ONE TO BUILD.** Hoist the test out of the loop
entirely: two walker loops, one plain and one instrumented, chosen ONCE.

```
    if (flags == 0) run_statements_plain(...);     // no check in the loop at all
    else            run_statements_watched(...);
```

**0.225 ns against the 0.224 ns floor — the features cost NOTHING when they are
off.** Not a nanosecond, not a branch: the checks are not in the compiled loop.

At a billion statements that is the difference between **1.6 seconds of pure
overhead and none**. For a language whose client reads corpora, that is the whole
argument.

## 10.1 — `satl --rebuild`, which is exactly the right shape

The author's command is what makes the split affordable, because it moves every
cost to a moment nobody is timing:

1. A person sets the features they want — settings, the way `arguments.access`
   already works.
2. **`satl --rebuild`** composes them into ONE binary and writes it to
   `config.ini` as a single value.
3. Start-up reads **that one value** — one parse, one integer, once per run.
4. The walker branches on it **once** and then runs a loop with no checks in it.

So the per-run cost of the whole debug system, with everything off, is: one line
read from config.ini, and one integer compare. **And the per-statement cost is
zero.**

**`--rebuild` IS ALLOWED TO BE SLOW, AND THAT IS THE POINT.** The author:
*"this way it remains fast, and the --rebuild takes a long time only, that is
what is slow"*. Nothing it does is on any path a person waits on twice, so it may
do work that would be unthinkable per run — validate every bit against the
setting it came from, walk every library and check the features it claims,
re-measure the machine, write a human-readable table of what it turned on. **A
step that runs once per machine has no budget**, and the only mistake available
is putting something in start-up that belongs here.

**AND IT IS THE SECOND COMMAND OF THAT SHAPE, WHICH MEANS IT IS A PATTERN.**
SATELLITE_ARGUMENTS Phase 8 has `satl --config`: a thread probe that takes 9.6
seconds and is *"unthinkable at every startup and nothing at all once per
machine"*. Same argument, same answer, and the two are siblings — `--config`
measures what the MACHINE can do, `--rebuild` composes what the PERSON asked for.
They write to the same config.ini and should agree about how: whether they are
one command with two halves, or two commands, is red note 13.

## 10.2 — The rules the register has to keep

1. **A BIT'S MEANING NEVER MOVES. APPEND ONLY.** This is the words.tsv rule
   again, and it bites harder here: a `config.ini` written by an older build
   holds a number, and if bit 5 stopped meaning "trace" and started meaning "dump
   memory", that old number silently turns on the wrong feature. A bit is
   retired by being **abandoned**, never reused.
2. **THE FAST PATH IS A `uint64_t`; THE SATELLITE VALUE IS A BINARY.** satellite
   has no limits and `satellite.variable.binary` is any width — but the walker's
   one compare has to be a register compare, not a bignum compare. So: 64 bits
   for the features the WALKER tests, and the satellite-visible binary may be
   wider for everything else. If 64 is ever not enough for hot features, a second
   word is one more compare, not a redesign.
3. **READ-ONLY AFTER START-UP, OR IT IS A DATA RACE.** `arguments.threads_startup`
   is 1,024 threads. A flag a running program can flip is a value 1,024 threads
   read while one writes. Either the register is fixed once at start-up — which
   is what `--rebuild` naturally gives — or every read is atomic and the split
   loop above is impossible.
4. **THE REPORT PRINTS THE REGISTER, SPELLED OUT.** A report gathered with half
   the features off is a report with holes in it, and a person reading it has no
   way to tell a section that was empty from a section that was never collected.
   §A of Part 7 carries the register as bits AND as names.
5. **`arguments.access` IS ALREADY A SETTING AND HAS TO BECOME BIT 0.** It is
   built, it lasts in config.ini, and it is the valve for the last-known store.
   Either `--rebuild` folds it into the register, or there are two mechanisms for
   one job — which is the thing R6 of SATELLITE_ARGUMENTS refused for arguments
   and should refuse here.

---

# Part 11 — WHAT WE ARE NOT DOING

The author asked: *"tell me about what we are not doing"*. Honestly, then —
fourteen things, with the ones that are nearly free marked, because those are the
ones being left on the table for no reason.

## 11.1 — A defect in this file's own plan

**THE `syntax:` ROW CAN LIE, AND PART 3 IS WHY.** Part 3 recommends re-reading
the source file at report time rather than keeping the text. That is right for
cost and it has a hole nobody has closed: **if the file changed since it was
loaded, the line printed under `syntax:` is not the line that ran.** A person
editing while a long program runs gets a caret pointing at the wrong code, and
nothing says so. The fix is small — hash or stat the file at load, check it at
report time, and say "the file changed since this ran" instead of printing a line
that is not true. **It is written here because it is my own plan's flaw, found
while answering this question rather than by somebody debugging at 3am.**

## 11.2 — The ones that are nearly free

Each of these is cheap *because* something already planned is being built, and
each costs nothing when its bit is off.

| what | why it is nearly free |
|---|---|
| **Per-word call counts** — how many times each of 367 words ran | The word's code IS an array index (`function_table.hpp`). One `++counts[code]` |
| **Coverage** — which statements never ran at all | One bit per statement position; the statement ring already walks there |
| **Watchpoints** — stop when a named variable changes | The last-known store (§F) already intercepts every write |
| **The reproduction command** — the exact line to run it again, with the cwd | Every piece is already in `Arguments` |
| **Timing per capsule** — where the time went | The frame stack (R7) is already push/pop; a clock read is one instruction |
| **`satellite.assert(x)`** — a program raises its own report | The report exists; this is a word and a library |

## 11.3 — The ones that are real work, and worth it

- **A STEP DEBUGGER.** Break at a line, step one statement, look. The walker
  already keeps a position and the frame stack gives the rest. This is the single
  biggest debugging win not in Parts 5 or 8, and it is *reachable* — most of its
  machinery is being built for the report anyway.
- **A POST-MORTEM PROMPT.** 004 already has `--repl`. Dropping into it **at the
  point of failure, with that frame's variables loaded**, turns every crash into
  an investigation instead of a report to read. The two pieces exist separately
  and have never been joined.
- **DETERMINISTIC REPLAY.** Record what came in — `console.input`, file reads,
  the random seed, the clock — so a failing run can be run again identically. For
  a language with 1,024 threads and a PCG generator this is the difference
  between a bug that is fixed and a bug that is "not reproducible".
- **A THREAD SECTION.** Part 7's §K counts threads. It does not say **what each
  one is doing**, who is waiting on whom, or which of them was the one that
  failed. With threads as the point of the language, a report with a thread
  *count* is half a report.
- **MEMORY ACCOUNTING — WHAT IS HOLDING IT.** Not how much this process uses
  (§K), but **which variables and which structures**. On 2026-09-17 a queue took
  5.9 GB in 48 seconds and finding it took a measurement rig; a report that named
  the biggest holder would have said it outright.
- **A STREAMING TRACE.** The statement ring (§G) keeps the last N for a crash. A
  trace writes **all** of them to a file as they happen, which is the other
  question — not "what was it doing when it died" but "what did it actually do".
  Different bit, different cost, same collection point.
- **WHAT KILLED IT.** If the OOM killer took the process, or a signal did, the
  report should say so rather than not existing. `/proc/self/status` and a
  handler; related to R15 and red note 7.
- **A STABLE, PARSEABLE FORM.** R17 proposes `--report-diff`. Diffing only works
  if the report has a fixed order and a machine-readable shape — decided now, or
  every tool built on it later fights the formatting.

---

# Part 12 — the milestones for Parts 10 and 11

**F1–F8 are the feature register.** They come first: every debug feature below
hides behind it, and building the features before the register means retro-fitting
each one.

## Phase F — the register

- **F1** — ~~`FeatureRegister`: one `uint64_t`, one named bit per feature, **append-only**.~~ **Done 2026-09-18** — `satellite/config/feature_register.hpp`, fourteen bits.
- **F2** — ~~Read it from config.ini at start-up: one value, one parse.~~ **Done** — `start_register()`, and it answers three ways rather than two (absent / unreadable / read), because Part 7's rule 3 says a section that could not be gathered must not print as empty.
- **F3** — ~~`satl --rebuild`: compose the settings into the binary, write it, print what it turned on.~~ **Done** — and it names every feature that is turned on and **not built yet**, which only belongs in a once-per-machine step.
- **F4** — ~~Fold `arguments.access` in as bit 0.~~ **Done** — bit 0, and red note 14 is closed: **both spellings live.** The named keys are what a person edits and what a program writes; `features` is the one value satl reads and is DERIVED from them. So `arguments.access` keeps working exactly as built, and `--rebuild` is what makes the fast value out of it.
- **F5** — ~~**Split the walker**~~ **SUPERSEDED 2026-09-18 by the author's switch hierarchy, and built as F5a.** F5 said two loops, plain and instrumented. He generalised it: *"BUILD A SWITCH-BASED HIERARCHY ... switch statements inside of other switch statements ... for now, we just wrap the entire interpreter in a switch hierarchy and we end up running the exact same interpreter that we have, just encompassed by switch statements"*. A hierarchy is two loops with room for eight, so F5 is its first case rather than a rival.
- **F5a** — ~~The hierarchy, wrapping `run_main`: eight leaves, all calling the same interpreter.~~ **Done** — `satellite/config/feature_switch.hpp`. See Part 14.
- **F5b** — ~~Give `RunPlan::plain` a loop with no feature tests compiled into it.~~
  **MEASURED 2026-09-18 AND NOT WORTH BUILDING.** The two gated sites were
  compiled out entirely (`if (false)`) and the tree rebuilt and timed against a
  10,000,000-statement program:

  | | run 1 | run 2 | run 3 | |
  |---|---|---|---|---|
  | both tests present, bits off | 4.79 s | 4.84 s | 4.67 s | |
  | **both tests compiled OUT** | 4.96 s | 4.94 s | 4.81 s | **slower** |

  Compiling them out measured SLOWER than leaving them in, which is how noise
  answers: the run-to-run spread is ±0.17 s and the whole effect is smaller than
  that. **A statement costs about 470 ns; two predictable branches cost about
  one.** F5b would buy 0.2% of one percent, in exchange for templating the
  interpreter across two translation units.

  **THE SAME MEASUREMENT KILLED A SECOND IDEA.** `add`, `subtract`, `multiply`,
  `divide`, `modulus` and `power` reached their number fast path THIRD, behind a
  percentage test and a by-worth test. Hoisting it to first — which is the
  author's own design order — measured 4.82 / 4.77 / 4.86 / 4.85: noise again.
  The hoist was kept anyway, because it costs nothing and it makes the code say
  what the design says, but **it is not an optimisation and must not be recorded
  as one.**

  **WHERE THE TIME ACTUALLY IS — PROFILED 2026-09-18, NOT GUESSED.** gprof, a
  40,000-turn loop, 200,000 statements. valgrind/callgrind dies on this CPU;
  `make OPT="-O2 -pg" LDFLAGS="-pg"` is what works.

  | calls per 200,000 statements | what |
  |---|---|
  | **1,680,028** | `std::variant` `_M_reset` — **25% of the run** |
  | **560,134** | **`text_at`** — builds a `std::string` a character at a time — **25%** |
  | 560,134 | `count_at` (inside `text_at`) |
  | 1,000,015 | `refuse_if_reserved` — five a statement |
  | 640,034 | `word::code_of` |
  | **400,005** | `unordered_map<std::string, Variable>::find` — **ten a statement** |
  | **160,014** | **`satellite_number::from_text`** — every number literal is re-parsed **from text on every evaluation** |

  **Three things that table says, and none of them is a branch:**
  1. **A name is rebuilt as a `std::string` and re-hashed every time it is
     touched.** `VariableTable` is `unordered_map<std::string, Variable>`, so
     `a = a + 1` costs three string builds and three hashes. Resolving a name to
     a SLOT once, at check time, is the single biggest win available and it is
     a real milestone rather than a tweak.
  2. **`1` is parsed from text every time it is evaluated.** A literal's worth
     never changes; converting it once at load is free after that.
  3. **The variant's destructor is a visit.** Ten arms, and `_M_reset` is the
     top entry.

  **SEVEN `text_at` CALLS BUILT A STRING AND THREW IT AWAY**, purely to move a
  cursor past a payload. `skip_payload()` does that arithmetic and nothing else.
  Kept for being obviously less work — **not measured, and not claimed as a
  speed-up**: the machine ran at load average 6 and a 1% effect cannot be
  resolved through that.

  **AND F6's CHOSEN PROGRAM DOES NOT RUN.** `experiments/energy/release.satl`
  exits 13: it needs `satellite.container.list`, which is numbered and unbuilt.
  The measurements above use a purpose-built arithmetic loop instead. **The author is milestoning the interpreter's own optimisation (MILESTONES M35); this is the leaf it lands in.** As of 2026-09-18 there is one gated site to hoist: the `word_counts` test in `call_word`.
- **F9** — ~~The register has to reach the places that test it.~~ **Done 2026-09-18** — `FeatureRegister features` on `MachineState`, which SATELLITE_ARGUMENTS A1 already chose for the reason it gives: there are no globals, and a register a thread cannot see the right copy of turns features on for some threads and not others. `MachineState &state` was already threaded everywhere, so nothing new is passed.
- **F6** — Re-measure with a real program after F5. `experiments/energy/release.satl` is about a million statements a second and is the shape that would show any regression.
- **F7** — §A of the report prints the register as bits AND as names (rule 4).
- **F8** — The register is fixed after start-up; a program that tries to write one says so (rule 3).

## Phase G — the nearly-free ones (11.2)

- **G1** — ~~Per-word call counts: `++counts[code]`, and a report section ordered by count.~~ **Done 2026-09-18** — `satellite/bytecode/word_counts.hpp`, and it is **the first feature in 004 that reads its own bit**. Verified at 100,000 calls counted exactly, and the bit off against on was 0.136s to 0.132s on that program — noise. Its one honest limit: it counts words reached through `call_word`, which is words called with `(`. A statement word like `satellite.statement.while` does not pass through there and is not counted.
- **G2** — Coverage: one bit per statement, and a section naming statements that never ran.
- **G3** — Watchpoints, off the last-known store's write path.
- **G4** — The reproduction command line, in the report.
- **G5** — Timing per capsule, off the frame stack.
- **G6** — `satellite.assert(x)`: a word, a library, and the report it raises.

## Phase H — the real work (11.3)

- **H1** — The file-changed check (11.1). **Small, and it closes a hole this file opened.**
- **H2** — What killed it: signal, OOM killer, exit status.
- **H3** — The thread section: what each thread was doing.
- **H4** — Memory accounting: which variables hold the most.
- **H5** — The streaming trace, its own bit.
- **H6** — A stable, parseable report form, and `--report-diff` on top of it.
- **H7** — Deterministic replay: record the inputs and the seed.
- **H8** — The step debugger: break, step, inspect.
- **H9** — The post-mortem prompt: `--repl` at the point of failure, that frame loaded.

---

# Part 13 — left to the author

Red notes. None of them blocks E1–E6, R1–R6 or F1–F5.

1. **Does a NOTICE use the same block as a failure?** S0721 is not fatal and
   prints the identical frame. A person may read two rules of dashes as "this run
   is over" — and that reading is wrong exactly once, on the commonest report
   there is. A `SATELLITE NOTICE` header on the same frame would fix it and would
   be a second shape to keep.
2. **Do both numbers show?** The report shows the S-code; the one-line form shows
   the machine code. A person who gets a report and then reads `echo $?` sees two
   unrelated numbers for one failure, and nothing on screen connects them.
3. **How many reports does one run print?** A refusal inside a loop can print the
   same 16-line report every turn. 003's warning log kept every warning for the
   whole run and drained at the end; the console queue's lesson from the same
   night is that anything per-iteration needs a bound before it is shipped, not
   after.
4. **Is `S` followed by four digits enough?** Twelve blocks of a hundred is 1,200
   codes. 003 reached S1001, so the shape is proven; the question is only whether
   a block ever needs more than a hundred.

## From the full-report brief, 2026-09-18

5. **What is N for the statement ring (§G, R12)?** *"every STATEMENT"* read
   literally is unbounded — a loop running a billion times is a billion
   statements. A ring of the last N is the answer, and N is a number only the
   author can pick, because it is a trade he owns: 64 is a page and tells you
   almost nothing about a loop; 100,000 is megabytes on the hottest path in the
   language. **This is the one number in Part 8 that has to be measured rather
   than chosen**, and it is the one most likely to hurt if it is guessed.
6. **Does the statement ring cost anything when nothing fails?** It is written
   once per statement whether or not there is ever an error, which is the
   definition of a hot path. `arguments.access = false` turns it off — but the
   default is ON, on this machine, on the author's instruction. **Measure before
   and after with a real program** (`experiments/energy/release.satl` is a
   million statements a second and is exactly the shape that would show it).
7. **Should a crash report be written by a SIGSEGV handler at all (R15)?** It is
   the case a person is most helpless in, and it is also the case where the
   interpreter's own structures may be the thing that is broken — walking a
   corrupt frame stack inside a segfault handler turns one crash into two and
   loses the report. 003's watchdog met the same question and answered `_exit`
   with no destructors, deliberately. The safe version writes only the sections
   that need no pointers chased (§A, §B, §C, §L) and says the rest were skipped.
8. **Where does `~/.satl/reports/` get cleaned up?** R4 writes a file per
   failure. A person debugging a loop makes hundreds in an afternoon, and nothing
   in Part 8 ever deletes one. Keep the last N, keep N days, or keep them all and
   say so — but it is a decision, and an unbounded folder is the same bug as an
   unbounded queue with a slower fuse.
9. **Which environment variables are named in §M?** 7.6's rule 2 forbids dumping
   `environ`, because a report is made to be pasted into a bug entry and the
   environment holds tokens and keys. So there is a written list: `HOME`, `PATH`,
   `PWD`, `SHELL`, `TERM`, `LANG`, `SATELLITE_*`. The author should say whether
   that list is right before it ships, because adding to it later is easy and
   taking something out of a report somebody already pasted is impossible.

## From the feature-register brief, 2026-09-18

10. **How many bits does the WALKER get?** Rule 2 of 10.2 puts the hot features
    in one `uint64_t` so the split test is a register compare, and leaves the
    satellite-visible value a full-width binary. Sixty-four hot features is a lot
    — Phases G and H together want about fifteen — but the number has to be said
    out loud once, because a 65th hot feature is a second compare in the branch
    that Part 10 measured at 0.001 ns, and nobody should discover that by adding
    one.
11. **Can a program turn a feature on mid-run?** Rule 3 says no, and says why:
    1,024 threads reading a value one thread writes is a data race, and the split
    walker is impossible if the answer can change inside the loop. But it forbids
    something genuinely useful — `satellite.trace_on()` around one suspect
    capsule. A middle answer exists: the register is fixed, and the SPLIT is
    re-chosen at a capsule boundary, where no loop is in flight. That is a real
    design and it is not free, so it is the author's.
12. **What does `--rebuild` do with a bit this build does not know?** A newer
    config.ini read by an older satl holds bits with no meaning here. Ignore them
    silently, refuse, or say so and carry on — and the same question upside down
    for an older file read by a newer satl, which is the ordinary case after
    every upgrade.
13. **Is `--rebuild` the same command as `--config`?** They are siblings: both
    run once per machine, both may be slow, both write config.ini. `--config`
    measures what the MACHINE can do (the 9.6-second thread probe,
    SATELLITE_ARGUMENTS Phase 8); `--rebuild` composes what the PERSON asked for.
    One command with two halves is fewer things to explain; two commands let a
    person recompose their features without re-probing threads for ten seconds.
14. **Which bit is `arguments.access`?** Rule 5 says it becomes bit 0, because it
    is already built, already lasting, and already the valve for the last-known
    store — and two mechanisms for one job is what R6 of SATELLITE_ARGUMENTS
    refused. But it ships TODAY as a `flag_setting` library, so folding it in is
    a change to something that works, and the author should say whether the
    setting stays the spelling with the register behind it, or is replaced.

---

# Part 14 — the switch hierarchy, as built

`satellite/config/feature_switch.hpp`, 2026-09-18. **Nested switches wrapping the
interpreter**, choosing once which shape of run to do.

## What it measured

Two questions, both answered by running them rather than arguing them.

**Does a switch hierarchy cost anything?** Only inside the loop. 2,000,000,000
statements, clang 24 -O2, all features off, two runs agreeing to the fourth digit:

| | ns a statement | |
|---|---|---|
| floor: plain loop, no switches | **0.216** | — |
| nested switch **per statement** | 0.708 | 3.3x the floor |
| nested switch **wrapping the loop** | **0.216** | **the same loop** |

**And in the real interpreter?** A 2,000,000-turn satellite loop, best of four,
through four different leaves:

| register | plan | time |
|---|---|---|
| nothing on | `plain` | 2.272 s |
| `access` | `assignments` | 2.276 s |
| `trace` | `statements` | 2.269 s |
| `frames trace access` | `statements+capsules+assignments` | 2.288 s |

**Within 1% across every plan** — which is what "the choice is made once, outside
the loop" looks like from the outside.

## The one correction the author made

I wrote that fourteen features nested one to a switch is 2^14 = 16,384 leaves and
therefore unwritable. He answered: *"no you have to run the same interpreter for
different cases inside of different cases, so it's NOT 16k"*, and *"it will be
like... 100 lines long to do this, or maybe 200 300 400"*.

**He is right.** 16,384 is the number of distinct BODIES you would write if every
combination got its own specialised interpreter. It is not the cost of the switch
nest, because cases running the same thing share it. The file is 230 lines.

So the 16,384 is a budget on SPECIALISATION, not on structure, and it only starts
being spent the night a leaf gets a body of its own.

## Why it switches on tiers

A feature's cost is decided by how often it is looked at, so that is what the
switches ask about:

| tier | looked at | features |
|---|---|---|
| **S** | once a statement | `statements` `trace` `coverage` `word_counts` |
| **C** | once a capsule | `frames` `capsule_timing` |
| **A** | once an assignment | `access` `history` `watchpoints` `memory_accounting` |
| **O** | once a run | `report_file` `report_on_success` `thread_state` `replay` |

Tier O is **not in the hierarchy**: a thing done twice in a program's life cannot
be made cheaper by specialising a loop, and putting it in would double the leaves
to buy nothing. Three tiers, two states each: **eight leaves**, all of which are
reachable and were checked one at a time.

**TWO `static_assert`s KEEP IT TRUE.** Every `Feature` must be in exactly one
tier, and no feature may be in two. A feature added to the enum and forgotten
would be tested nowhere — it would never turn on and nothing would say so. That
is a build failure now instead of a silent one.

## What was verified

- All eight leaves reachable, each register landing on the right one.
- **Byte-identical output through every leaf** on a program with a loop, a
  capsule call and arithmetic — same md5, seven lines, eight times.
- No measurable cost, above.
- 223 of check.sh passing, 0 failing.

**A CHECK THAT PASSED VACUOUSLY FIRST, recorded because it nearly counted.** The
identity test compared md5s across eight leaves and all eight matched — because
the program failed to run and every output was empty. `d41d8cd98f00` is the md5
of nothing. The test only became a test once the program produced seven lines.

---

# Part 15 — the author's rulings, 2026-09-18

Answers to Part 13. The questions are kept above; these are the decisions.

## The severity policy, and it is the one that governs the rest

The author, in two sentences that look opposed and are not:

> let's not make more things fatal, let's make less things fatal and only use
> fatal when we absolutely have to use it

> But we kinda want to crash the interpreter, provide information, we don't want
> it to operate incorrectly, we are trying to build a precision tool here, we need
> this to run exactly correctly in case someone uses it inside of a data center

**They are about different things, and reading them together is the policy.**

- **"Fatal" is about the FRAME, not the stopping.** Do not invent new categories
  of alarm, do not dress a notice as a catastrophe, do not print a wall of dashes
  at somebody because a file was missing. Fewer things should LOOK fatal.
- **"Crash" is about CORRECTNESS.** A program whose meaning is not certain must
  stop, because running it wrong is worse than not running it. *"we don't want it
  to operate incorrectly ... in case someone uses it inside of a data center"*.

So: **stop readily, alarm rarely.** A refusal is ordinary and is reported in
ordinary words; the eighty-column report is for the times a person needs
everything.

**AND THERE IS EXACTLY ONE KIND OF RECOVERY, NAMED BY THE AUTHOR.** He has
*"only written once 'keep running'"*, and it is the obvious typo:

> we only keep running when the code that is typed is incredibly obvious, such as
> satellite.variable.binary my_number = 110101001111 -- here the user forgot the
> "b", which that is okay, we can pick it back up, but other than that, let's
> crash the interpreter on just about anything other than typos that we know what
> the user meant, we use aliases to keep the code going

**The test is "do we KNOW what they meant", not "can we guess".** A binary
declaration given digits that are all 0 and 1 has one possible reading. `"a" + 5`
has two and the language refuses it today, rightly. **Recovery needs a reason it
could not have meant anything else** — and every recovery is warned about, never
silent, because a silent fix is the interpreter operating incorrectly with a
smile.

> the last thing users want is a picky interpreter that just crashes on
> everything, and they have to learn this new system -- this system was designed
> to do two things at the same time -- the ease of use of something like python,
> and the power of C++ at the exact same time, that is the purpose of the project

**That is this project's "C with Classes" sentence, and it is written here so the
error system can be measured against it.** Ease is not leniency about meaning; it
is aliases, spellings, and a refusal that says what to type instead.

## Note by note

**1. Does a notice use the same frame as a failure? — ANSWERED: fewer fatal
things.** S0721 (config.ini missing) keeps running and should not look like a
catastrophe. **Do not** add a second scary frame; make the notice quieter. A
missing config is one ordinary line, and the eighty-column report is kept for
what deserves it.

**2. Do both numbers show? — THE AUTHOR LEFT IT TO ME.** *"report is something
that you made, so you decide that one"*. **Both, in the header row.** The S-code
is what a person searches for and the machine code is what `echo $?` gives them;
a report showing one of the two makes the other look unrelated.

**3. How many reports does one run print? — ANSWERED: track them.** *"this is
going to require some tracking so we are not reporting the exact same thing
twice"*. **Same S-code at the same file and line is reported ONCE**, and the
repeats are counted: `(this happened 40,199 more times)` at the end. The key is
the three together — the same code at a different line is a different event.

**4. Is `Sxxxx` enough? — ANSWERED: yes, and the author asked me to decide the
scheme.** *"there isn't going to be more than... 9999 errors!! Just use every
single number! ... You actually designed these errors, so you tell me that one!"*

**My answer: keep the blocks, and here is why it is not waste.** A block is not
a reservation that throws numbers away — every number in a block still gets used,
in order, as errors are added. What the block buys is that **the number itself
says where to look before anybody looks anything up**: `S07xx` is settings,
`S05xx` is names, `S08xx` is arithmetic. A person pasting "S0723" into a message
has already told you which part of the interpreter they were in.

Sequential numbering makes `S0341` and `S0342` neighbours that have nothing to do
with each other, and the only way to learn what one is, is a table. With 9999
numbers and perhaps 150 ever needed, density is the one thing this design can
afford to spend. **And 003 already did this** -- S0714, S0723, S1001 -- so it is
the scheme a person moving between the two already knows.

**If a block ever fills**, it continues in the first free range and Part 4 records
where it went. A full block is a good problem and it is one line to answer.

**5 and 6. The statement ring — THE AUTHOR ASKED WHAT A RING IS, WHICH MEANS THE
NOTE WAS BADLY WRITTEN.** *"I don't know what you mean by 'What is N for the
statement ring'? what do you mean by 'ring'?? If you mean the encompassing switch
hierarchy..."*

**It is not the switch hierarchy.** Said plainly:

> A **ring buffer** is a fixed-size list that overwrites its own oldest entry when
> it is full. Keep 256 of them and you always have the most recent 256 and never
> any more, with no allocation and no growth.
>
> The **statement ring** is §G of Part 7: remember the last N statements the
> program ran, so that when it stops, the report can show **what it was doing just
> before**. That is the difference between "it crashed" and "it crashed on line
> 412, having just gone round this loop 40,000 times".

**Why N is a question at all:** the ring is written **once per statement**, which
is the hottest path in the language, and it holds N entries in memory forever. The
switch hierarchy costs nothing because it is taken once; the ring is the opposite
kind of thing, which is why it needed a number and the hierarchy did not.

**Proposed N: 256.** One entry is a file, a line and a word code -- 16 bytes -- so
256 is **4 KB, one page, allocated once at start-up and never grown**. It is deep
enough to show a loop's shape and the path into it, and small enough that the
answer to "does it cost anything" is measurable rather than argued. Overridable
from config.ini; the author may want 1,024.

**7. SIGSEGV. — Standing, and it agrees with the severity policy:** write only
what needs no pointer chased -- §A identity, §B arguments, §C the failure, §L the
config -- and say the rest was skipped. Walking a corrupt frame stack inside the
handler turns one crash into two and loses the report.

**8. Who cleans `~/.satl/reports/`? — ANSWERED: I do, and from the start.** The
author: *"We clean .satl/reports/ both of us ... So currently you have to clean
it, because it's too long for me to read"*.

**Note for whoever builds R4: the folder does not exist yet** -- `report_file` is
a bit with no behaviour, so there is nothing to clean today. The rule is written
now so it is built in rather than retro-fitted: **keep the newest 50 and delete
the rest on every write**, and a report is one file per failure, named by time and
S-code. An unbounded folder is the console queue with a slower fuse.

**9. Which environment variables? — the author asked what I propose.** The list
was `HOME PATH PWD SHELL TERM LANG SATELLITE_*`. **Four to add, and two of them
matter much more than the rest:**

| add | why |
|---|---|
| **`LD_LIBRARY_PATH`** | **satl `dlopen`s its own libraries.** This variable changes WHICH `.so` files a run loads, so it can make two identical satl binaries behave differently. A report that does not name it cannot explain that |
| **`LD_PRELOAD`** | the same, and worse: it can replace functions inside the interpreter. It is the first thing to suspect in a "works on my machine" that nothing else explains |
| `USER` / `LOGNAME` | satl already reads the real user from `getpwuid`. Keeping the environment's copy too is worth it precisely when **they disagree** -- which is what `sudo` looks like |
| `TZ` | every time in the report means something different without it |

**And the allowlist stays an allowlist.** These are named, one at a time, and
nothing is swept: a report is made to be pasted into a bug entry, and `environ`
holds tokens and keys. Adding a name later is easy; taking one out of a report
somebody already pasted is impossible.

**Shell commands in the prompt is a different feature**, and a real one -- but it
belongs in the prompt's own milestone, not the error system's. It also brings a
question this file has no opinion on yet: a language that can run shell commands
is a language whose programs can do anything the person can, and that is a
decision about satellite rather than about reports.

**10-14.** Unchanged: 64 hot bits; mid-run flips still the author's (note 11);
unknown bits dropped silently; `--rebuild` and `--config` stay two commands;
`arguments.access` is bit 0 and both spellings live.
