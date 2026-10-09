# NEW_MILESTONES — a running interpreter that keeps what each file left: satellite.delete, satellite.access(filename), and satellite.history last

**Written 2026-10-06, on build 0027, from the author's two messages after that day's /clear.**
Section 1 is his words, whole. Section 2 is the order they give. Section 3 is what satl has today,
read that day with the file and line. Section 4 is the milestones. Section 5 is what is still his,
and nothing in it is decided. **NM-2, `satellite.delete(name)`, IS BUILT (build 0029)** — his
brief for it came in the second message; every other milestone waits for his word, one at a time.

His words for the size of it: *"This is a massive undertaking to do this, so we will build it in
like 3 - 4 prompts"*.

---

## 1. His words (2026-10-06)

### The first message — the design, and the order

> *"I wanted to re-design but it's only a small change, I want the interpreter to be already
> running if the console window is open, and if the running interpreter has ran a file, then that
> interpreter has .access and .history for that file, which will be accessed via
> satellite.access(filename) and it keeps up to 20% (this percent defineable under
> arguments.access = percent_value so either 20% or a satellite.variable.percent object,) UNLESS
> arguments.access is already taken as something else, if it is, we will call it
> arguments.access.percent(percent_object)) and satellite.history needs to be disabled by default,
> however if it is enabled then it is ready-to-run on any file that is ran on that interpreter,
> however we will not build satellite.history part right now, and only add a milestone to a new
> milestones file that we will be using, called NEW_MILESTONES.md where we will build a milestone
> for satellite.history but not yet, as we need to build satellite.access() now for an interpreter
> that is running, and it is enabled by default, so satellite.access(filename) will give a list of
> all things for a file that has been ran, the last state of each variable... This is a massive
> undertaking to do this, so we will build it in like 3 - 4 prompts, for this prompt all that I
> want you to do is build a few more milestones into NEW_MILESTONES.md:*
>
> *the satellite.history milestones will be last;*
>
> *then the first milestone will be:*
>
> *making sure that the interpreter is already running for when it receives a file that has
> satellite.include(satellite), satellite.capsule satellite.main(), and satellite.return(satellite)
> (if those 3 things are in a file and it receives a command to run the file, THEN it runs that
> file,) so actually building a running interpreter is beyond the scope of this particular work,
> so we will not do that now, we will just tack it on to NEW_MILESTONE.md as another milestone near
> the end...*
>
> *So for that milestone we will just ensure that the interpreter is ready to receive a file that
> has those 3 things in it, for the next milestone that we write we will ensure that it has made a
> satellite.access.filename object, which contains an object that exists in memory for each
> variable unless the variable has been deleted, which brings me to the next milestone,
> satellite.delete, and actually to build this we must build satellite.delete before we build
> satellite.access, so let's write those 2 - 3 milestones now, and in the next prompt I will give
> you satellite.delete, which you can follow after you have written the 2 - 3 milestones..., BUT I
> just realized that satellite.delete is going to delete FROM satellite.access, but we will still
> build satellite.delete first, then build satellite.delete for satellite.access so just write the
> 2 - 3 milestones into NEW_MILESTONES.md, and then receive the next prompt with
> satellite.delete()"*

### The second message — satellite.delete

> *"satellite.delete(name) will delete everything about "name" from everywhere except for in the
> actual file where the text is written, and the copy of that file that exists inside of the
> running interpreter (as this would almost be the same thing as deleting that variable from the
> file, we will not erase the value inside of the bytecode -- HOWEVER everywhere else the "name"
> will be deleted, and this is a clear enough instruction to follow along with, just make sure
> that you remove everything about "name" when satellite.delete(name) is called, everywhere except
> exactly two places:*
>
> *IN the actual file that is running on the disk,*\
> *IN the copy of the running file that is loaded into memory,*
>
> *everywhere else, the "name" needs deleting, which is only in memory and in history, and we will
> not delete it from inside of .history, which will just become a more detailed satellite.access
> component....*
>
> *Once you have done all of that, detail the changes that you have made in your report to me, so
> I can make sure you've deleted everywhere "name" is."*

### Later the same day — satellite.legal and global_object

After the same-block rule was explained to him: *"this points to a design flaw that we need to fix -- or
to something even greater than that, a design that would perhaps lead us to way faster, more efficient
software!"* Then:

> *"So we need to build two separate lists of variables, and they need to be used for two separate things
> -- the "live" list of objects, the "passed" list of objects, then the "full" list of objects, and objects
> will have 2 states: "accessible" and "inaccessible", and it will be a simple bool as to whether the
> object is accessible so it doesn't slow anything down, so EVERY object needs a new bool, "accessible =
> true/false", then this is flipped to OFF when the object goes out of scope, and it becomes inaccessible,
> though the record of the object exists for satellite.access(object) for the life of the interpreter --
> so this prevents a single kind of programming, and allows for super faster other types of programming,
> it's a trade off that i'm willing to do though... so we need a list of accessible objects kept, and a
> list of inaccessible objects, and both belong to satellite.access, and then satellite.access becomes
> something that we CANNOT turn off slightly, though you can turn it off and it will allow for that other
> type of programming to be done, so that is how we will build it, so you can turn off satellite.access
> and if you run delete on an object without satellite.access turned on, the object is completely wiped
> from memory, and only exists as something that you could create only with the original file of code,
> ... there is satellite.library for accessing an object anywhere, then satellite.access for all objects
> that are held in memory -- if it's in memory, it's on satellite.access, but not necessarily on
> satellite.library"*
>
> *"So each object could carry around what it belongs to, and we can then run a check to see if the
> operation is legal or not -- "SUPER" Type Checking!.. so for this we need to develop another list,
> something that holds all of the legal operation places, we will call this list satellite.legal! and
> then we ourselves will use satellite.legal for... checking whether operations are legal or not, so just
> to start out, there will be satellite.legal.main (for satellite.main) and it will contain the
> "satellite" object, thus, using the name "satellite" for anything becomes illegal, "satellite" becomes
> the first legal object..."*

His answers to the questions put to him: *"1. satellite.legal answers this question"* (what delete does
with access on), *"2. it's just, bad programming habits that some people have..."* (what turning access
off allows), *"3. I don't know"*, *"4. I don't know, but let's keep a std::string on each object of it's
satellite.legal status for each object, so this needs to be added to every object, we need a
"global_object" class to add this stuff to, then EVERY class is a "global_object","*, then *"Okay go with
your recommendation"* (global_object is the box a named object lives in, its place's name kept once and
pointed at), and *"A. I suggest a single master satellite.legal list B. I suggest using satellite.legal
for every capsule, so I dunno, C. Satellite should have a special variable suggesting that it is
immutable D. where is D? E. correct, F. Yes"*.

---

## 2. What those words decide

| | milestone | his words for where it goes | state |
|---|---|---|---|
| **NM-1** | the interpreter is ready to receive a file that has the three things | *"then the first milestone will be"* | not built |
| **NM-2** | `satellite.delete(name)` | *"to build this we must build satellite.delete before we build satellite.access"* | **BUILT 2026-10-06, build 0029** |
| **NM-2a** | satellite.legal and global_object: names end at their block's `}` (access on) | *"we will call this list satellite.legal!"* | **BUILT 2026-10-06, build 0039; the checker reads the list itself since 2026-10-07, build 0047** |
| **NM-2b** | boxes by number: a name read by its box, not searched for by its spelling | *"way faster, more efficient software"* | not built |
| **NM-3** | `satellite.access(filename)` — the `satellite.access.filename` object | *"for the next milestone that we write"* | not built |
| **NM-4** | `satellite.delete` for `satellite.access` | *"then build satellite.delete for satellite.access"* | not built |
| **NM-5** | the interpreter already running while the console window is open | *"another milestone near the end"* | not built |
| **NM-6** | `satellite.history` | *"the satellite.history milestones will be last"* | not built |

- **NM-2 is built before NM-1** because its brief came first — *"in the next prompt I will give you
  satellite.delete, which you can follow"*. Neither needs the other. NM-3 needs both.
- A milestone he adds later goes where he puts it; the running interpreter stays near the end and
  satellite.history stays last, renumbered if it has to be.
- **`arguments.access` IS ALREADY TAKEN.** It is the true/false switch built 2026-09-18 (249d748):
  default true, saved in config.ini, bit 0 of the feature register, and the valve for this very
  store (SATELLITE_ARGUMENTS.md Part 4B). So by his own rule the percent is
  **`arguments.access.percent(percent_object)`** — and, by the 2026-10-06 rule for every row,
  `arguments.access.percent = 20%` is the same line. `arguments.infinity` and
  `arguments.infinity.counter` are already a row and a row beneath it, so the shape exists.
- **satellite.access is on by default** — and arguments.access already reads true with no row.
- **satellite.history is off by default** — as MILESTONES.md M20.C and its help page already say.

- **satellite.legal (his "SUPER" type checking), as he settled it:** ONE MASTER LIST of the places an
  operation is legal in — `satellite.legal.main`, `satellite.legal.greet`, `satellite.legal.prompt` — each
  holding `satellite` first, legal everywhere and immutable (nothing may be named it, change it or delete
  it). Every named object's box is a `global_object` carrying its place and `accessible`. **With
  arguments.access on (the default) a name ends at the `}` of the block that declared it**, and a line after
  it that names it is refused before anything runs; its box stays, inaccessible — the record satellite.access
  will read. With access off, the old way: legal to the end of the capsule (*"bad programming habits"*), and
  nothing kept. E, his *"correct"*: with access on, delete makes a name illegal and satellite.access keeps
  its record; with access off it is wiped. D (files, windows, threads): his *"4. I don't know"* — so it stays
  his, open — and *"D. where is D?"* asked where it had gone from the lettered list. Until he rules, my
  default: a file is saved and closed when its name ends, as at a capsule's end; a thread, program or window
  carries on; the record describes it.

---

## 3. What satl has today — read 2026-10-06 on build 0027 (line numbers as of build 0029)

1. **THE CONSOLE WINDOW AND THE INTERPRETER ARE ONE PROCESS** (the header of
   `satellite/satellite_variable_window/console_launch.cpp`: *"In THIS process -- no second binary,
   no exec, no fork"*). `satl` alone opens it with the prompt; `satl file.satl` opens it running the
   file, and a file that finishes cleanly closes it at once. A run that stopped, and the prompt,
   hold it open.
2. **THE PROMPT RUNS A FILE IN A CHILD satl.** `interpret <file>` and `run <file>`
   (`satellite/satl/prompt_run.cpp`) fork and exec `satl --run <file>`, on purpose since
   2026-09-22: *"a program run inside it would have to be taken apart again by hand after every
   run"*. So every name a file declares dies with the child, and the prompt's interpreter never
   holds one. That is what NM-1 and NM-3 change.
3. **HIS THREE THINGS ARE ALREADY THE GATE BEFORE ANY RUN.** `file_can_run`
   (`satellite/bytecode/include_shape.cpp:217-298`) refuses a file with no
   `satellite.include(satellite)` (S101, exit 10), no `satellite.main` (S150, exit 11) or no
   `satellite.return` (S103, exit 12) before anything is checked or run, and program_check.cpp
   refuses a main whose last line is not `satellite.return(satellite)`. MISSING_SYNTAX.md MS-8,
   not built, would make a missing return S031 and let the run carry on — see 5.3.
4. **ONE RUN A PROCESS.** `satellite.return(satellite)` sets `program_quit()`
   (`satellite/bytecode/program_walk.cpp:1906`) and nothing ever clears it — the same flag that
   leaves a window's handlers dead after main (the exit-64 fix, still open).
5. **A NAME LIVES IN ITS BODY'S `VariableTable`** (`satellite/bytecode/value.hpp`) — one table a
   running body, no globals — and the table goes when the body ends, its files saved and closed
   first (`close_files`, `program_walk.cpp:1595`). Nothing outlives a run. The prompt keeps one
   table for the whole session (`TypedLineMemory`, `program_walk.hpp`).
6. **`satellite.access(name)` IS BUILT** (2026-09-26, fdc586b, words `1 31` and `1 31 1`): what a
   name IN SCOPE is, its value, and the line that reaches every level of it. One name in its
   brackets and nothing else — a string there is refused today.
7. **THE LAST-KNOWN STORE WAS DESIGNED AND NEVER BUILT**: SATELLITE_ARGUMENTS.md Part 4B (his
   2026-09-18 words, *"for satellite.history and for satellite.access we keep the LAST KNOWN
   variable's name type and value"*), MILESTONES.md M34 in Part 3, SATELLITE_ERROR.md R11. Part
   4B's question 7 was *"Does the last-known store have a bound?"* — his 20% is the answer.
8. **`satellite.variable.percent` IS BUILT** (satellite.help/satellite.variable.percent): `50%`, 32
   digits after the point, the % required.
9. **`satellite.history` IS NOT A WORD.** `arguments.history` is numbered (`1 14 3 11`) with no
   library; MILESTONES.md M20.B and M20.C are his earlier words for it.
10. **`satellite.delete` WAS NOT A WORD** — `1 32` was free. `satellite.system.delete(x)`, `1 22 1`,
    is numbered and unbuilt: 003's word for deleting a FILE, which is a different thing.

---

## 4. The milestones

**THE GATE FOR EVERY ONE** — MISSING_SYNTAX.md section 8's, with no git: the tree snapshotted into
`~/.claude/projects/-home-madness-code-cxx-satellite/backups/<date>-<what>/` before the first edit
and the patch written beside it when it is done; its own check.sh rows, then the whole of
check.sh; every help example run; one fresh reader of the diff; and each built only on his word,
one after the other.

### NM-1 — the interpreter is ready to receive a file that has the three things — NOT BUILT

His words: *"making sure that the interpreter is already running for when it receives a file that
has satellite.include(satellite), satellite.capsule satellite.main(), and
satellite.return(satellite) (if those 3 things are in a file and it receives a command to run the
file, THEN it runs that file,)"*, and *"for that milestone we will just ensure that the interpreter
is ready to receive a file that has those 3 things in it"*. The interpreter that is ALREADY
running is NM-5's, not this.

- WHAT "READY" HAS TO MEAN, read from his words: the interpreter that receives the file runs it
  ITSELF — *"THEN it runs that file"* — and NM-3's *"object that exists in memory for each
  variable"* is in that interpreter's memory, which a child's never is (3.2). So: ONE way in, "run
  this file here", that asks file_can_run's three questions first, refuses a file missing one with
  the code it has today, runs the file in this process, and leaves the interpreter ready for the
  next file. (5.1 and 5.2 are his.)
- BUILD — what stands in the way, each read on build 0027:
  - `program_quit()`, set by the file's own `satellite.return(satellite)`, put back before the next
    file (3.4);
  - what the file started — threads (`close_every_thread`, `bytecode/thread_calls.cpp`), programs
    (`satellite_variable_program/program_stop.cpp`), windows — ended or waited for at the end of
    its run, as `satl file.satl` ends them, and its open files saved and closed (`close_files`);
  - a file's settings are that run's only (SATELLITE_ARGUMENTS.md Part 4G), so the run's rows are
    put back after it — or a file's `arguments.x(v)` would outlive it here, which the child made
    impossible;
  - the same output, the same refusals and the same exit status as `satl file.satl` — the reason
    prompt_run.cpp chose a child.
- PROVE: at a piped prompt, two files run one after the other in the prompt's own process; a file
  missing each of the three refused with the code it has then (S101, S150, and S103 -- or S031 if
  MS-8 has landed, 5.3) while the prompt still answers; a file's setting gone after its run; a third
  file runs after a refused one.

### NM-2 — `satellite.delete(name)` — BUILT 2026-10-06, build 0029

His brief is the second message, whole, in section 1. What it says, in his words:
- *"delete everything about "name" from everywhere except for in the actual file where the text is
  written, and the copy of that file that exists inside of the running interpreter"*;
- *"we will not erase the value inside of the bytecode"*;
- *"everywhere else, the "name" needs deleting, which is only in memory and in history, and we will
  not delete it from inside of .history"*;
- and the report he asked for: *"detail the changes that you have made in your report to me, so I
  can make sure you've deleted everywhere "name" is"*.

AS BUILT — words `1 32` `satellite.delete` and `1 32 1` `satellite.delete(name)`;
`satellite/bytecode/delete_calls.hpp` and `.cpp` (the word, the one reader of the line's shape, and
`forget_the_name_in`), `run_delete` in `satellite/bytecode/program_walk.cpp` (the deleting),
`judge_a_delete` in `satellite/bytecode/program_check.cpp` (the forgetting); satellite.help/satellite.delete.

**WHERE "name" IS DELETED FROM — every place in memory a variable's name is kept:**
1. **The table of the body that holds it** (`VariableTable`, value.hpp): its one entry, which IS the
   name, the word that declared it, what was between its `<` and `>`, and its value. In a capsule or
   in main, that call's table; at the prompt, the session's own table (`TypedLineMemory`), so no
   line typed after it sees it.
2. **Its value**, let go at that line, and freed when this name was the last thing holding it: a
   number, a string, a list or a map at once; a spacesuit's object when nothing else holds it — `b =
   a` is a second name for ONE object, and a list or a thread it was handed to holds it too — and
   every `.pointer()` at it then reads as gone.
3. **A file it held**, saved and closed at that line when nothing else holds it, by the same
   `close_files` a body's end uses, and a save that fails is said. At the prompt it is then no longer
   among the kept files saved after every line (`files_kept`).
4. **A program started under the name** — the value itself, or one in a list, a map or an object's
   fields that the name held — forgets it (`forget_the_name_in`): the program kept the name it was
   started under for S742, which now says a program whose name was deleted by the line it started
   on, *"the name it was started under was deleted with satellite.delete"*, never by the name.
5. **The checker's picture of the body, from that line on**: its declared names, the three lists of
   names that hold objects (`objects`, `lists`, `multiples`) and the block it was declared in — so a
   later line that names it is refused before anything runs, *"big was deleted by
   satellite.delete(big) on line 7, and no line after that one can use it -- declare it again
   first"*, and the name may be declared again, as anything. The line and the block it went in are
   kept only while the checker reads that body, for that sentence and the rule below.
6. **And no thread keeps a variable's name at all.** A thread run on an object's capsule was labelled
   by the variable it was called on — `(thread secret_box.call_take, running)` — and is labelled by
   its capsule now, `(thread box.call_take, running)`, whether or not anything is deleted
   (`thread_calls.cpp`). That is the one change here a program that never deletes can see.

**WHERE IT IS NOT DELETED — his two places, and what they hold:**
1. The file on the disk.
2. The copy of the running file in memory: the bytecode rows read from it (`BytecodeRegistry`), and
   what was read out of them before the run — the capsule table (a capsule's parameter names, so a
   parameter deleted in one call is bound again in the next), the statement ring (positions), the
   loaded text a report quotes, and S742's `p.start()` line, which is that text.
3. `.history`, as he said — and there is no satellite.history yet.

**AND WHAT I LEFT, HIS TO RULE ON — text, or not the variable:**
- the prompt's typed lines (its up-arrow recall): the prompt's copy of its own text, as the file is;
- what is already on the screen, and satellite.log's entries that quote the name — on disk, a log
  of what was said;
- `arguments.x`, a row of the arguments called x, and `satellite.library.x`, a library value — not
  the variable x;
- a variable spelled the same in another capsule's call — a different variable;
- a started thread or program, and an opened window, are the run's, not the name's, so they carry
  on as they do when the capsule that named them ends (measured: the same output and exit code with
  the delete and without it) — holding no name now (4 and 6 above). A started program that no name
  holds any more can never be joined, so the run stops it at the end and says S742.

**REFUSED BEFORE ANYTHING RUNS — my choices, his to overrule:**
- anything but one name in the brackets, `satellite.delete` inside another line, and anything after
  its `)`;
- **a name declared in another block** — a delete inside an if or a loop of a name declared outside
  it. The checker reads a body once, top to bottom, so it could not know, after the block, whether
  the name was still there; the other way is for the walker to refuse a later use at run time;
- **a name deleted inside a block, declared again outside it** — in the else beside it, or on a line
  after it — is declared twice (S202), as it was before satellite.delete existed: the fresh reader
  showed the else letting a run stop half way. Inside the block that deleted it, after the delete,
  it may be declared again;
- a field of the object a capsule runs on (it goes with its object), satellite.statement.for's own
  number inside its loop, and `arguments` (the run's own rows).
- A name deleted and then used gets S201 NAME_NOT_DECLARED with its own sentence (above), but S201's
  paragraph under it still says *"no satellite.variable line ever declared it"* — a code of its own
  is his to number, if he wants one.

**ONE FRESH READER (2026-10-06)** found that two run-time objects still kept the name — a started
program (S742 said *"write p.join()"*) and a thread's label — and that an if's else could declare a
deleted name again and stop a run half way; all three are fixed (4, 6, and the rule above). Also
fixed from it: a for that declares a deleted name again is no longer called deleted after its loop;
at the prompt, after it has declared capsules, "on line N" counts the lines that were typed, not the
hidden capsule's; the delete test moved off the path every statement takes, to where only a line
that begins with a word reaches it; two check.sh rows that claimed more than they proved, reworded;
the help page's claims made true. It found nothing changed for a program that never deletes, besides
6.

PROVED: check.sh (16 rows, then 6 more after the review: S742 without the name, a thread's label,
the else and the line after a block, the for, the prompt's line); the file's control — without the
delete, the file opened again is empty and `again[1]` stops at 47; a thread's and a program's runs
the same with the delete and without.

### NM-2a — satellite.legal and global_object — BUILT 2026-10-06, build 0039

His words are section 1's "Later the same day"; what they decide is in section 2.

AS BUILT — `satellite/bytecode/satellite_legal.hpp`/`.cpp` (the master list, `global_object`, `satellite`
first and immutable); `Variable` in `value.hpp` IS a `global_object` (its place, `accessible`, `immutable`);
the checker (`program_check.cpp`) gives each capsule its place, ends a block's names at its `}`
(`close_the_block`) while access is on, and refuses a later use: *"x is not legal here -- it was declared
inside the block that ended at the } on line 10, and satellite.legal.main holds it no further; declare it
above that block to use it after it"*; the walker (`program_walk.cpp`'s `run_block`) makes those boxes
inaccessible at the same `}` and saves and closes a file only they held; at the prompt a name whose block
ended is legal on no later line. `satellite.access(x)` shows the line `legal  satellite.legal.main`. Naming
anything `satellite` is S202 and deleting it S110, both saying why.

- WHAT IT CHANGES: an if and its else may each declare `x` — each is its own block — and so may a line
  after the block; a name made inside a block can no longer be used after it, unless the file says
  `arguments.access = false`.
- 2026-10-07, ON HIS *"Can you build that part of it now then?"* (build 0047) -- THE CHECKER READS THE MASTER
  LIST ITSELF. Until then it judged each line on a block-by-block map of its own, which satellite.legal only
  recorded, and nothing read the master list. That map is gone: `DeclaredNames` is `LegalNames`
  (`satellite_legal.hpp`), a view of the capsule's own `LegalPlace` -- a name declared is written into the place,
  a name ended (its block's `}`, its for's end, satellite.delete) is ended there, and every name a line uses is
  looked up there and nowhere else, the word that declared it beside it (`LegalName::declared`). The four literal
  checkers (float, fraction, hex, colour) are handed the same view. A read of a capsule begins by ending every
  name its place holds but satellite, so the place says exactly what the checker has read so far. The run-time
  check stays the bool on every box (his *"a simple bool ... so it doesn't slow anything down"*): the walker is
  untouched, and loop speed with it. My one choice, overrulable: a spacesuit's field, judged alone, reads an
  unlisted place of its own rather than a place named after its spacesuit. THE FRESH READER (61 programs
  identical on 0045 and 0046; 0 HIGH, 0 MEDIUM, 4 LOW): a capsule's place is now found by the capsule (its shown
  name) and only called by its place name, so a capsule a program names `main` or `prompt` keeps a record of its
  own instead of reading and writing satellite.main's or the prompt's -- their place names still read the same,
  which is his to rule on (refuse `main`/`prompt` as capsule names, or spell them apart); and the header no
  longer claims the checker refuses every ended name a run could reach: a BARE `{ }` block's names are never
  ended by the walker, and a bare block inside main ends main at its `}` -- pre-existing, found 2026-10-06, still
  his call.
- NOT YET: a program cannot list satellite.legal (it is satl's own); delete keeps its same-block rule until NM-4;
  the boxes are still found by spelling (NM-2b).
- THE FRESH READER (2026-10-06) found that a name declared again while still live (an object of the same
  spacesuit, a kept name at the prompt) moved into the inner block — so a delete there crashed the run, a for
  could free a box still in use, and valid programs were refused; that the walker still read ended boxes (a
  colour's hex digits read a dead variable); that the prompt could run a capsule under a rule it was not
  checked under; that a block's file stayed open when an inner alias had held it; and that checking slowed as
  capsules grew. All fixed and pinned in check.sh; satellite is refused under every spelling that tries to
  name it.
- SPEED (his rule: slower is a failure): loops of 300,000 passes, three runs each against build 0029 — a block
  declaring 1 name +1.1%, 4 names +1.2%, 8 names +0.4%, no names +0.7%, a capsule call +1.4%; callgrind counts
  +1.2% instructions for 8 declarations a pass, +0.3% for calls. The cost is the bool flipped at each `}` and
  the bigger box.

### NM-2b — boxes by number — NOT BUILT

His words: *"a design that would perhaps lead us to way faster, more efficient software!"* With every name's
block known before the run, each name can be given a box number, and a line that reads `x` opens box 3
instead of spelling `x` out and searching the capsule's table. Timed against the build before it, by a
command he runs; slower means stop.

### NM-3 — `satellite.access(filename)`: the `satellite.access.filename` object — NOT BUILT

His words: *"if the running interpreter has ran a file, then that interpreter has .access and
.history for that file, which will be accessed via satellite.access(filename) and it keeps up to
20%"*; *"satellite.access(filename) will give a list of all things for a file that has been ran,
the last state of each variable..."*; *"we will ensure that it has made a satellite.access.filename
object, which contains an object that exists in memory for each variable unless the variable has
been deleted"*; and *"it is enabled by default"*.

- NEEDS NM-1 (the file ran in THIS interpreter) and NM-2 (a deleted variable has no object here).
- WHAT IT KEEPS: one object a file the interpreter ran, `satellite.access.filename`, holding one
  object a variable — its last state — for every variable that was not deleted, in memory, while
  the interpreter runs. `satellite.access(filename)` answers the list of them.
- ITS BOUND: *"up to 20%"*, set by `arguments.access.percent(20%)` (or `= 20%`), taking a
  percentage written with its % or a `satellite.variable.percent` (section 2 says why it is not
  `arguments.access = 20%`). Typed at the prompt it is saved in config.ini; in a file it is that
  run's alone (SATELLITE_ARGUMENTS.md Part 4G).
- ITS SWITCH: `arguments.access`, built, true by default — which today decides satellite.legal's rule
  (NM-2a: names end at their block's `}`) and would turn this store on too.
- PROVE, when it is built: a file run in the interpreter, then `satellite.access(filename)` at the
  prompt lists each variable's last state; a variable deleted in the file is not there; the bound
  read back from `arguments.access.percent`.
- His: 5.5 to 5.10.

### NM-4 — `satellite.delete` for `satellite.access` — NOT BUILT

His words: *"BUT I just realized that satellite.delete is going to delete FROM satellite.access,
but we will still build satellite.delete first, then build satellite.delete for
satellite.access"*; and, in the second message, the name *"needs deleting, which is only in memory
and in history"*.

- WHAT IT ADDS TO NM-2: NM-3's store is memory, so `satellite.delete(name)` takes the name out of it
  too — a name deleted while its file runs never reaches `satellite.access.filename`, and NM-2's
  list of where a name is deleted gains that row.
- His: 5.11, how a name is written for a file that has already run.

### NM-5 — near the end: the interpreter already running while the console window is open — NOT BUILT

His words: *"I want the interpreter to be already running if the console window is open"*, and *"so
actually building a running interpreter is beyond the scope of this particular work, so we will not
do that now, we will just tack it on to NEW_MILESTONE.md as another milestone near the end..."*.

- WHAT IT ADDS TO NM-1: NM-1's way in, reached from outside the console — a file to run handed to
  the interpreter whose console window is open, instead of a second satl with a console of its own
  (3.1). Then `satellite.access(filename)` at that console's prompt reads what the file left.
- His: 5.12 and 5.13.

### NM-6 — LAST: `satellite.history` — NOT BUILT

His words now: *"satellite.history needs to be disabled by default, however if it is enabled then it
is ready-to-run on any file that is ran on that interpreter, however we will not build
satellite.history part right now"*, and of a deleted name: *"we will not delete it from inside of
.history, which will just become a more detailed satellite.access component...."*.

His earlier words for it, kept and not replaced: MILESTONES.md M20.B (*"records the values of
everything onto disk -- type, name and value of every object"*); M20.C (`arguments.history`;
`arguments.history_path`, which *"must be a directory"*; *"name_of_satl_file.history files with the
extension .number.history for each time that file is ran"*; the free-space check, *"at least... 32
gigabytes?"*); and SATELLITE_ARGUMENTS.md Part 4B (*"history is the lasting copy"*).

- WHAT THIS MESSAGE ADDS: it is kept for each FILE the running interpreter runs; it is off by
  default; and `satellite.delete` never deletes from `.history`.
- His: 5.14.

---

## 5. Open — his, and nothing here is decided

1. **(NM-1) In this interpreter, or in a child?** His words read as this interpreter (NM-1 says
   why). What goes with the child is its protection: a program that crashes cannot take the prompt
   with it.
2. **(NM-1)** Do `interpret <file>` and `run <file>` become NM-1's way in, or stay a child beside it?
3. **(NM-1) A missing `satellite.return(satellite)`.** His words make all three the condition;
   MISSING_SYNTAX.md MS-8, planned, makes a missing return S031 and carries on. Which holds for the
   receiving interpreter?
4. **(NM-1)** The windows a file opened: closed at the end of its run, or kept while the interpreter
   runs?
5. **(NM-3) 20% of what?** My reading is the machine's memory (`arguments.memory.total`), because the
   store lives in memory — or the memory free when the interpreter starts?
6. **(NM-3) When the 20% is full:** the oldest file's objects let go, or nothing more kept?
7. **(NM-3) How `filename` is written:** `satellite.access("hello.satl")` — a string, free today, so
   it can never be taken for a variable — or a bare name; and `satellite.access.filename` for a
   name with a dot, a dash or a folder in it, and for two files of one name in two folders.
8. **(NM-3)** A file run twice: the second run's objects replace the first's?
9. **(NM-3)** Two capsules each with a variable `x`, or one capsule called many times: one object a
   NAME, the last write winning, or one a capsule and name?
10. **(NM-3)** `satellite.access(name)` for a name in scope stays as built. His 2026-09-18
    `satellite.access(object_name)` "when the program stops" — is it now
    `satellite.access.filename.object_name` only?
11. **(NM-4)** How a name is deleted from a file that has already run, when the name is no longer in
    scope: `satellite.delete(satellite.access.filename.name)`?
12. **(NM-5)** More than one console open: which interpreter receives the file? And with none open,
    does `satl file.satl` open one that then stays open, so the store is there to read?
13. **(NM-5)** Where the file's output goes, and what exit status the shell gets back.
14. **(NM-6) "history" and ".history":** `satellite.delete` leaves the name in `.history`. Will any of
    history be held in memory, and is the name deleted there?
