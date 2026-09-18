# satellite-004 — SATELLITE_ERROR

The error system: what an S-code is, what a report looks like, which pieces are
built, and what each unbuilt piece costs.

Written 2026-09-18 from the author's brief, which is kept whole in Part 1.

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
- **One caller exists**: `S0721 CONFIG_FILE_MISSING`, in
  `structured-library.cpp`, raised when `$HOME/.satl/config.ini` is absent.
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

Blocks, so a number never has to move. **Add at the end of a block; never
renumber.** Same rule as the word table, for the same reason.

| block | what it is for |
|---|---|
| **S00xx** | starting up: config, libraries, the machine |
| **S01xx** | the command line |
| **S02xx** | reading a file: it is not there, not readable, not satellite |
| **S03xx** | the lexer: a character or a token that cannot be read |
| **S04xx** | the checker: shapes refused before anything runs |
| **S05xx** | names: declared twice, never declared, out of scope |
| **S06xx** | types: two kinds that do not meet |
| **S07xx** | settings and arguments |
| **S08xx** | numbers: division by zero, an answer that is not whole |
| **S09xx** | strings, binary, percentage |
| **S10xx** | input and the console |
| **S11xx** | threads and memory |
| **S12xx** | directories and files a program opens |

## Assigned

| S-code | name | machine code | where |
|---|---|---|---|
| **S0721** | `CONFIG_FILE_MISSING` | *(notice — the run carries on)* | `structured-library.cpp` |

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

- **E1** — Record a byte offset per token, or re-tokenise one line at report time; pick the second unless the first is free.
- **E2** — `line_of(row, at)`: count `line_end_token` before `at`.
- **E3** — `source_line(file, n)`: re-read the file, answer that line's text.
- **E4** — `column_of(row, at)`: the caret's position inside that text.

## Phase 1 — the seam

- **E5** — `raise(CriticalReport)`: one call that renders, prints and answers the machine code.
- **E6** — A report knows its file and position from `ExpressionContext`, so a caller passes neither.

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

# Part 6 — left to the author

Red notes. None of them blocks E1–E6.

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
