# MISSING SYNTAX — every element a program leaves out is named, with the lines that put it back, and a tutorial that builds a whole program

**A PLAN, 2026-10-03. NOTHING IN IT IS BUILT.** Written from the author's prompts of
2026-10-03 on satellite 004 revision 09 build 0001, and rewritten the same day from his ninth
message into milestones. Section 1 is his words, as he wrote them. Sections 3 and 6 are
MEASURED — every example in this file was run. Section 7 is HIS to decide. Section 8 is the
milestones, **built one after the other, each proved before the next begins, until the last
one** — his words: *"properly followed, one after the other, until it is completed"*.

---

## 1. His words, in order (2026-10-03)

It began with `.open()`, which is built (776dbba, SATELLITE_WINDOW.md Part 2c):

> *"for satellite.window on a satellite.variable.window object, the user needs to be able to
> type in: window_object.open() and it opens the window object that was already declared, as
> in: ./gtkcar/satl_window.satl"*

Then, with the window built and the revision raised to 09:

> 1. *"When the window is declared, I don't think the window should open, and the interpreter,
>    if the file is ran without .open but has .window.new() it should report you forgot open,
>    and it should give a lesser error code, "missing syntax element: satellite.capsule .open()
>    and the corrected syntax with like, 3 line example. Let's expand errors at least to
>    include all types of codes like this for missing syntax.."*
> 2. *"the missing syntax should apply to every single syntax element, capsule/object/variable"*
> 3. *"and different missing for capsule than for object missing, which, wihtout
>    my_window.open() "my_window" object is missing, and "open()" is missing, and if you run a
>    blank file it should give you a correct hello, world! to get started"*
> 4. *"We could expand this into an entire satellite tutorial to build a window, put a console
>    inside of it, in like a missing sequence of lines"*
> 5. *"hello, world! has no window, it's a minor error... the syntax has no window even, and
>    the satl has gtk code in it, so it's missing a window by default"*
> 6. *"this is more complex"* — *"and we must stop here"*
> 7. *"and convert these prompts into a plan..."* — *"MISSING_SYNTAX.md"*
> 8. *"with milestones for each element of a full program, with a window, with a separate
>    console, unless the program sets arguments.window = false or arguments.interpreter = true,
>    it by default includes a way to travel to a program that contains a window"*
> 9. *"so we need to just stop and format an entire MISSING_SYNTAX plan that includes
>    milestones for every single missing syntax in an entire program, that can optionally be
>    turned off with arguments.missing = false, which by default is set to true, and gives you
>    the tutorial by default... for an entire program that is a self contained .satl file, and
>    it includes it's own window with it's own prompt, and it accepts optionally a few simple
>    commands: and this will all be added, milestone by milestone: copy a file, delete a file,
>    rename a file, create a new file (we already have that) but we don't have a error command
>    built for it, so this very simple skeleton of a program that accepts a command, and
>    accepts as input --help as an argument, AND parses it, all of it needs to be built into
>    milestones, and properly followed, one after the other, until it is completed, THEN we
>    expand the missing syntax program with something that parses input(parsing input beyond
>    "--help" is the final milestone of MISSING_SYNTAX.md) so we need to build this all into
>    milestones... as soon as you can stop, anyway"*

## 2. What those words decide

Each of these is a requirement, not a proposal.

| | decided | from |
|---|---|---|
| D1 | `satellite.window.new` DECLARES a window and does not open it; `.open()` opens it. This reverses SATELLITE_WINDOW.md Part 2c's "new still shows the window" | 1 |
| D2 | a file that makes a window and never `.open()`s it is told it forgot `.open()` — a **lesser** code, "missing syntax element", and the corrected lines, about three | 1 |
| D3 | a missing-syntax code for **every** syntax element in an entire program — capsule, object, variable and the rest | 1, 2, 9 |
| D4 | a missing CAPSULE and a missing OBJECT are different codes: without `my_window.open()`, the object `my_window` is missing and the capsule `open()` is missing | 3 |
| D5 | a blank file answers with a correct Hello, World!, to get started | 3 |
| D6 | Hello, World! has no window, and that is a **minor** error: satl carries GTK, so a program is missing a window by default | 5 |
| D7 | milestones for each element of a full program — with a window, and a separate console | 8 |
| D8 | unless the program sets `arguments.window = false` or `arguments.interpreter = true`, it is given a way to travel to a program that contains a window | 8 |
| D9 | all of it can be turned off: `arguments.missing = false`. **The default is true** | 9 |
| D10 | **the tutorial is on by default** | 9 |
| D11 | the tutorial's program is ONE self-contained `.satl` file, with its own window and its own prompt | 9 |
| D12 | it accepts a few simple commands, added milestone by milestone: **copy, delete, rename** and **create a new file** | 9 |
| D13 | it accepts `--help` as an argument, and parses it | 9 |
| D14 | the milestones are followed in order until the program is complete; **THEN** input parsing beyond `--help` — **the final milestone of this file** | 9 |

## 3. What satl has today — measured on revision 09 build 0001

Every program here was run with no display and a scratch HOME, or on a compositor of its own
where it opens a window.

### 3a. What satl says when an element is missing

| what is missing | what satl says | code | exit |
|---|---|---|---|
| everything — a blank file | "this file has no satellite.include(satellite), so it is a spaceship and not a program" | S101 FILE_HAS_NO_INCLUDE | 10 |
| `satellite.include(satellite)` | the same | S101 | 10 |
| `satellite.main` | "this file has no satellite.main -- there are no globals, so there is nowhere else to begin" | S102 FILE_HAS_NO_MAIN | 11 |
| `satellite.return(satellite)` | "this file has no satellite.return -- execution ends inside main" | S103 FILE_HAS_NO_RETURN | 12 |
| a capsule called and never written | "in satellite.main, no capsule named greet" | S110 LINE_NOT_UNDERSTOOD | 13 |
| a variable used and never declared | "greeting has no satellite.variable line declaring it" | S201 NAME_NOT_DECLARED | 25 |
| an object of a spacesuit never written | "no spacesuit named rocket -- a type is a satellite.variable or satellite.container word, or the name of a satellite.spacesuit this line can reach" | S201 | 25 |
| a thread made and never `.start()`ed | **nothing at all** | — | 0 |
| a program made and never `.start()`ed | **nothing at all** | — | 0 |
| a window made and never `.open()`ed | nothing; the window shows anyway, because `new` opens it — D1 reverses that | — | 0 |
| a file made where one already is | **nothing**, unless the program asks: `.ok` is false, and `.error` is *"something is already at …/made.txt, and new never replaces it: nothing was made (open opens it)"* | — | 0 |

**Where each is said:** S101 and S102 in `bytecode/include_shape.cpp`; S103's sentence in
`machine/s_codes.hpp`; "no capsule named" in `program_check.cpp`, `capsule_scopes.cpp`,
`capsule_calls.cpp`, `capsule_reach.cpp`, `bytecode_registry.cpp` and `program_walk.cpp`;
"line declaring it" in `program_check.cpp`, `expression.cpp` and `program_walk.cpp`; "no
spacesuit named" in `suit_reach.cpp`. All are the CHECKER's (`satl(check)`, before a line
runs), apart from the walker's copies.

**A PRECEDENT FOR "NEVER DONE":** S742 PROGRAM_NEVER_JOINED is checked when the run ends —
*"Every .start() is paired with a .join()"*.

### 3b. What the tutorial's program needs, and what is there

| it needs | today |
|---|---|
| to read `--help` | **built.** `satellite.main(satellite.variable.arguments args)`; `satl files.satl --help` gives `args.argument_1` = `--help` and `args.length` = 2 — satl does not take the flag for itself |
| a prompt that reads a line and splits it | **built, on satl's own console:** `satellite.console.input("> ")` in a `satellite.statement.while`, and `line.split(" ")` — run with `new made.txt` and `quit` piped in, exit 0 |
| its own window | **built:** `satellite.window.new` and `.open()` |
| its own prompt, in a window of its own | **half built.** `satellite.console.new(...)`, `.open()` and `.display(...)` are there. The only way to READ from a program's own console is `.typed(a_capsule)`, whose capsule runs after `main` — and every capsule after `main` exits 64 since c7ec8a1 (SATELLITE_WINDOW.md Part 2c). There is no line-at-a-time read like `satellite.console.input` on a console a program made |
| create a file | **built:** `satellite.file.new(path)`; a file already there gives `.ok` false and `.error` in words (above) |
| does a file exist | **built:** `satellite.file.exists(path)` |
| copy, rename, delete a file | **no words.** `satellite.file` has `new`, `open`, `clear` and `exists` |
| the errors for them | S510 FILE_NOT_THERE, S511 FILE_ALREADY_THERE, S512 NOT_A_FILE, S513 FILE_UNREADABLE, S514 FILE_NOT_TEXT are in the register |
| a switch a program may set | `access` is the ONE today: `args.access = satellite.bool.false` writes it into `~/.satl/config.ini`; every other row satl holds is read-only, refused before the program runs. A bare `false` is refused too — a program writes `satellite.bool.false` |
| `arguments.missing`, `arguments.window`, `arguments.interpreter` | **none of the three rows exists** |

## 4. The band a missing element lands in

SATELLITE_ERROR.md Part 4, the author 2026-09-18: **the hundreds digit is the severity** —
*"S00 is stuff that doesn't stop the interpreter, so you have 99 warnings to use, then S01
stops the interpreter on something stupid"*:

| band | means |
|---|---|
| S0xx | a WARNING — the run carries on, exit 0 |
| S1xx | stops, and it is something simple: a missing line, a typo, a word satl does not take |
| S2xx | stops: a NAME — not declared, declared twice, not built yet |

And his severity policy: *"let's not make more things fatal, let's make less things fatal"*
— **"fatal" is the FRAME**: a notice is one quiet line, and the eighty-column report is kept
for what deserves it.

**PROPOSED — THE BAND OF EACH IS HIS (Q1).** "Lesser" read as: if the program can still run
correctly, it is S0xx and carries on; if it cannot, it stops in the lowest band that fits.

| missing | can the program run? | proposed band |
|---|---|---|
| `.open()` on a declared window (D2) | yes — the window never appears | **S0xx** |
| a window, in a program with none (D6) | yes | **S0xx**, the quietest of all |
| `.start()` on a thread, a program, a bash | yes — the work never happens | **S0xx** |
| a blank file (D5) | no | S1xx |
| `satellite.include` / `satellite.main` / `satellite.return` | no | S1xx (S101–S103 today) |
| a capsule called and never written | no | S1xx (S110 today) |
| a statement's part — a `{ }`, a condition, a `}` or `)` never closed | no | S1xx |
| a variable used and never declared | no | S2xx (S201 today) |
| an object whose spacesuit was never written | no | S2xx (S201 today) |

## 5. One report, proposed — every corrected example below was run, and exits 0

**THE QUIET SHAPE, for S0xx** — the one-line notice satl already prints
(`[satellite] S010 CONFIG_FILE_MISSING: ...`), one line for each missing element with its
KIND's own code (D4), and the corrected lines once, under them:

    [satellite] S0?? MISSING_OBJECT: my_window -- the object of a line this program is missing
    [satellite] S0?? MISSING_CAPSULE: open() -- my_window is declared on line 5 and never opened
        the lines, corrected:

            satellite.variable.window my_window = satellite.window.new("gtkcar", 450, 250)
            my_window.open()
            my_window.close()

**THE STOP SHAPE, for S1xx and S2xx** — today's report, with the same `missing <kind>:` lines
and `the lines, corrected:` under its sentence.

**A BLANK FILE (D5)** — the corrected lines are the whole of Hello, World!, the README's own:

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("Hello, World!")

        satellite.return(satellite)
    }

**A CAPSULE** — `greet()` called and never written:

    satellite.capsule greet()
    {
        satellite.console.display("Hello, World!")
    }

**A VARIABLE** — `greeting` used and never declared:

    satellite.variable.string greeting = "Hello, World!"
    satellite.console.display(greeting)

**AN OBJECT** — `rocket my_rocket` with no `satellite.spacesuit rocket` anywhere:

    satellite.spacesuit rocket()
    {
        satellite.public
        {
            satellite.capsule launch()
            {
                satellite.console.display("launched")
            }
        }
    }

**A DOING NEVER DONE** — a thread made with `satellite.thread.new(work())`, and the two lines
that run it: `t.start()` and `t.join()`; a program `{"echo", "hello"}`, and `p.start()` and
`p.join()`.

## 6. The tutorial — the path, and the program at the end of it

**THE PATH (D7, D8, D10).** Each step is what the program before it is missing. With
`arguments.missing` true — the default — a program on this path is shown the next step.

| step | element | its kind | what the step adds |
|---|---|---|---|
| 0 | nothing — a blank file | — | the whole of Hello, World! |
| 1 | `satellite.include(satellite)` | the file's word | the first line |
| 2 | `satellite.capsule satellite.main()` | capsule | where the program begins |
| 3 | `satellite.return(satellite)` | the file's word | main's last line |
| 4 | `satellite.console.display("Hello, World!")` | capsule, on the console satl gives every program | a line out |
| 5 | `satellite.variable.string greeting = ...` | variable | a value kept |
| 6 | `satellite.capsule greet()` | capsule | a capsule of the program's own |
| 7 | `satellite.spacesuit rocket()` and `rocket my_rocket` | object, and its type | an object |
| 8 | `satellite.statement.if`, `else`, `while` | statement | a choice and a loop |
| 9 | `satellite.variable.window my_window = satellite.window.new(...)` | object | a window, declared (D1) |
| 10 | `my_window.open()` | object + capsule (D4) | the window on the screen |
| 11 | `satellite.window.label(...)`, `my_window.append(...)` | object + capsule | something in it |
| 12 | `satellite.console.new(...)`, `.open()`, `.display(...)` | object + capsule | its own prompt — a separate console (D7) |
| 13 | a line read from the prompt, split into words | capsule | a command loop |
| 14 | `new`, then `copy`, `rename`, `delete` | capsule, one a command | the commands (D12) |
| 15 | `satellite.main(satellite.variable.arguments args)` and `--help` | variable | its own `--help` (D13) |

**THE PROGRAM AT THE END OF IT (D11).** One self-contained file. Every line marked
`NOT BUILT` waits on the milestone it names; every other line was run (§3b): `--help`, the
loop, the split, and `new` with its `.ok` and `.error` — piped `new made.txt` twice and then
`quit`, it made the file, then printed the file's own error, then exited 0.

    satellite.include(satellite)

    satellite.capsule satellite.main(satellite.variable.arguments args)
    {
        satellite.variable.bool asked_for_help = satellite.bool.false
        satellite.statement.if(args.length > 1)
        {
            asked_for_help = args.argument_1 == "--help"
        }

        satellite.statement.if(asked_for_help)
        {
            satellite.console.display("files -- new, copy, rename and delete a file, at its own prompt")
        }
        satellite.statement.else
        {
            satellite.variable.window my_window = satellite.window.new("files", 600, 200)
            my_window.append(satellite.window.label("type a command at the prompt"), 300, 100)
            my_window.open()

            satellite.variable.window my_prompt = satellite.console.new("files -- prompt", 800, 400)
            my_prompt.open()

            satellite.variable.bool running = satellite.bool.true
            satellite.statement.while(running)
            {
                satellite.variable.string line = my_prompt.input("> ")         // NOT BUILT: MS-14
                satellite.container.list<satellite.variable.string> words = line.split(" ")
                satellite.statement.if(words[1] == "new")
                {
                    satellite.variable.file made = satellite.file.new(words[2])
                    satellite.statement.if(made.ok)
                    {
                        my_prompt.display("made " + words[2])
                    }
                    satellite.statement.else
                    {
                        my_prompt.display(made.error)
                    }
                }
                satellite.statement.if(words[1] == "copy")
                {
                    satellite.variable.file copied = satellite.file.copy(words[2], words[3])   // NOT BUILT: MS-19
                }
                satellite.statement.if(words[1] == "rename")
                {
                    satellite.variable.file renamed = satellite.file.rename(words[2], words[3]) // NOT BUILT: MS-20
                }
                satellite.statement.if(words[1] == "delete")
                {
                    satellite.variable.file deleted = satellite.file.delete(words[2])          // NOT BUILT: MS-21
                }
                satellite.statement.if(words[1] == "quit")
                {
                    running = satellite.bool.false
                }
            }
            my_prompt.close()
            my_window.close()
        }
        satellite.return(satellite)
    }

`copy`, `rename` and `delete` are written here answering a file, with `.ok` and `.error` as
`new` does — that shape is a proposal (Q12). The window and the separate console, opened and
written to, ran on a compositor of their own: exit 0, both drawn.

## 7. Open — the author's, and nothing below is decided

- **Q1. Which band is each missing element in?** Section 4 is a proposal; the one D2 fixes is
  that a missing `.open()` is LESSER.
- **Q2. Renumber, or keep the numbers?** S101–S103, S110 and S201 already say most of what §3a
  shows. Keep their numbers and give them the `missing <kind>:` and corrected lines, or move
  them into a MISSING_ family, one code a kind? SATELLITE_ERROR.md names the trade —
  *"Severity ordering and 003 compatibility cannot both be true"* — and check.sh asserts many
  of these exits by number. Recommendation: keep the numbers, add the lines, and give new codes
  only to what has none today.
- **Q3. "Missing a window by default" — said on EVERY run of a program with no window?** Hello,
  World! and every console script would say it each time; the severity policy wants a notice
  to be one quiet line, and S020 goes to satellite.log only. The console, the log, or both —
  once a program, or every run?
- **Q4. Where are `arguments.missing`, `arguments.window` and `arguments.interpreter` set — and
  what does each turn off?** None exists yet. A row comes from config.ini, the command line, or
  a program line — and `access` is the only one a program may write today, by writing it into
  config.ini for every program after. Is `missing = false` for one program or for the machine?
  Does `arguments.missing = false` turn off everything this file builds — the quiet lines, the
  corrected lines on a stopping report, and the tutorial — while `arguments.window = false` and
  `arguments.interpreter = true` turn off only the window (D6, D8)? Does `satl --repl` count as
  `arguments.interpreter = true`?
- **Q5. What does the tutorial SHOW?** The next step's line or two (§6's path), the whole next
  program, or a command that walks the steps — `satl --tutorial`, or a `satellite.help` topic?
- **Q6. A console INSIDE the window (message 4), or a SEPARATE one (messages 8 and 9)?** A
  separate one is built and is §6's prompt. One inside a window is not: a console is a frame of
  its own (GTK_AND_NO_DEPENDENCIES.md GTK-17), and `.append` refuses a frame.
- **Q7. Before the run, or at its end?** The checker could say "made and never opened" before a
  line runs, but it cannot follow a window into a capsule that opens it, through `w2 = w`, or
  into an included file — and a wrong "you forgot" is worse than none. The end of the run knows
  exactly, as S742 does. Recommendation: the end of the run for "never done"; the checker for
  what already lives there.
- **Q8. What does a declared, unopened window look like?** `.ok` is false — it is not on a
  screen. `satellite.console.display(my_window)` says `(window "t")` open and
  `(closed window "t")` closed; a third word for "not opened yet", or `(window "t")`?
- **Q9. What may a program do to a window before `.open()`?** Proposed: everything that builds
  it — `.append`, `.title`, `.resize`, `.colour`, `.background`, `.font`, `.menu`, `.key`,
  `.clicked`, `.every`, `.closed` — and refuse what needs it on a screen: `.focus()`,
  `.message`, `.ask`, `.choose_a_file`, and `.press()` on a button inside it.
- **Q10. D4's two missing pieces — two lines, each with its kind's code, as §5 shows; or one
  line that names both?** §5 is a reading of message 3, not a ruling.
- **Q11. The prompt: a line-at-a-time read on a console the program made, or the capsules
  after `main` mended first?** A program's console has only `.typed(a_capsule)`, and every
  capsule after `main` exits 64 (since c7ec8a1 — his semantics question of 2026-10-01, whether
  `satellite.return(satellite)` inside a capsule closes the windows). Recommendation:
  `my_prompt.input("> ")`, the same word `satellite.console.input` already is — a loop in
  `main` needs no capsule at all, and is the skeleton message 9 describes.
- **Q12. *"create a new file (we already have that) but we don't have a error command built for
  it"* — which error is missing?** Measured: `satellite.file.new` on a file already there makes
  nothing, says nothing, and answers `.ok` false with `.error` in words (§3a). So it is one of:
  (a) satl SAYING it, rather than waiting to be asked; (b) a missing-syntax line for a program
  that never asks `.ok`; (c) an error command in the tutorial's program, a command that reports
  what went wrong; (d) the words for copy, rename and delete, which have no errors because they
  have no words yet. And: do `copy`, `rename` and `delete` answer a file with `.ok` and
  `.error`, as §6 writes them, or something else?
- **Q13. Is `delete` undone by anything?** A deleted file is gone; does the prompt ask "are you
  sure", or does the program decide, or neither?
- **Q14. Parsing beyond `--help` (D14) — how far, and whose?** Options with values
  (`--from a.txt --to b.txt`), a path with spaces in quotes, more than one word after a
  command, and one parser for the command line and the prompt alike. Is the parser satellite
  words a program calls, or a capsule the tutorial's program writes for itself?

## 8. The milestones — one after the other, each proved before the next begins

**The gate for every milestone:** its check.sh rows pass (check.sh opens no window, so any
window stays headless there); the whole of check.sh passes; anything that opens a window is
proved on a compositor of its own; every corrected line it prints was run first; and it is
committed and pushed before the next starts. Q1 and Q2 are answered before MS-2.

### Part A — what everything after it stands on

- **MS-1 — `arguments.missing`, the switch (D9).** A new row, default true. False turns off
  what Q4 says it turns off. Set where Q4 says. Proved: one program run with it true and with
  it false, side by side.
- **MS-2 — the register (D3, D4).** One code a missing KIND — capsule, object, variable, the
  file's words, a statement's part, a close, a window, a doing — in the bands Q1 picks,
  numbered as Q2 says; the quiet shape and the stop shape of §5. SATELLITE_ERROR.md's table,
  `s_codes.hpp`, and `machine_codes.hpp` for the exits of the ones that stop.
- **MS-3 — `new` declares, `.open()` opens (D1).** Recommendation for HOW: `new` makes the
  GtkWindow and does not present it, so everything Q9 allows works on a real widget, and
  `.open()` presents it and waits for its first frame, as it does today. The desk holds a
  declared window — its handlers point into it — but the run waits only for OPENED ones, and
  destroys any never opened when the program ends; `.every`'s clock starts at `.open()`
  (`the_window_answers_again` already does this for a reopened window). **Every program that
  relies on `new` showing a window gains `.open()`:** `examples/window.satl`,
  `tests/threads_window_on_a_thread.satl`, the programs inside `press-a-button.sh`,
  `prove-bare-machine.sh`, `prove-canvas-tabs-menus.sh` and `prove-console.sh`, the examples
  in the `satellite.window`, `satellite.variable.window`, `satellite.console` and
  `satellite.terminal` help pages, `experiments/window_test/windows_10.satl` — and the
  author's own `window_and_extras.satl`, which is his to change. Proved on a compositor,
  WAYLAND_DEBUG counting buffers: a declared window draws nothing, `.open()` draws it.

### Part B — every missing element of a program, in the order the tutorial meets them

- **MS-4 — a blank file answers with Hello, World! (D5).** Nothing in it, or nothing but blank
  lines and comments: the whole program as its corrected lines. Step 0.
- **MS-5 — `satellite.include(satellite)` missing.** S101, with its line. Step 1.
- **MS-6 — `satellite.main` missing.** S102, with main written around what the file has. Step 2.
- **MS-7 — `satellite.return(satellite)` missing.** S103, with main's last line. Step 3.
- **MS-8 — a capsule missing.** Called and never written; a method a kind does not have; the
  corrected lines write the capsule. Steps 4 and 6.
- **MS-9 — a variable missing.** Used and never declared; the corrected line declares it, with
  the type its use suggests where there is one. Step 5.
- **MS-10 — an object missing.** A spacesuit never written; an object used and never made.
  Step 7.
- **MS-11 — a statement's missing part.** An `if` or `while` with no condition or no `{ }`; an
  `else` with no `if`; a `for` without its three parts; a `}` or `)` never closed. Step 8.
- **MS-12 — the window a program does not have (D6, D8).** The quietest line, off with
  `arguments.window = false`, `arguments.interpreter = true` or `arguments.missing = false`
  (Q3, Q4). Step 9.
- **MS-13 — `.open()` missing (D2, D4).** The object and the capsule, each with its code, at
  the run's end (Q7). Step 10.
- **MS-14 — the program's own prompt (D11).** A console of its own, opened, and a line read
  from it — `my_prompt.input("> ")`, as Q11 recommends — with `.open()` and the read each a
  missing element of their own. Proved on a compositor: a line typed through mutter's
  RemoteDesktop keyboard comes back to the program. Steps 12 and 13.
- **MS-15 — a doing never done.** A thread, a program or a bash made and never `.start()`ed —
  the three that say nothing today. S0xx, at the run's end.

### Part C — the tutorial's program, one command at a time (D10–D13)

- **MS-16 — the tutorial (D10).** On by default; with `arguments.missing` true, a program on
  §6's path is shown its next step, as Q5 says. Proved: each step of §6, run in turn, is shown
  the step after it, and the last is shown nothing.
- **MS-17 — the skeleton.** One `.satl` file: its window (`.open()`), its prompt, and the loop
  that reads a line, splits it into words and stops on `quit`. Built today on satl's own
  console (§3b); here on its own (MS-14).
- **MS-18 — `new <file>`.** Built as a word; here it gets its error said, as Q12 decides.
- **MS-19 — `copy <from> <to>`.** A new word, `satellite.file.copy(from, to)`, and its errors:
  nothing at `from` (S510), something already at `to` (S511), not a file (S512), unreadable
  (S513). Never over the top of a file, as `new` never is.
- **MS-20 — `rename <from> <to>`.** A new word, `satellite.file.rename(from, to)`, and its
  errors, the same four.
- **MS-21 — `delete <file>`.** A new word, `satellite.file.delete(path)`, and its errors —
  nothing there, not a file — and Q13's answer.
- **MS-22 — a command it does not know.** The program says so, in its own words, and the
  prompt carries on.
- **MS-23 — `--help` (D13).** `satl files.satl --help` prints what it does and exits 0, read
  from `args.argument_1` (built, §3b); any other first argument is said to be unknown.
- **MS-24 — the skeleton is complete.** §6's program, the whole of it, run on a compositor of
  its own: the window drawn, the prompt drawn, `new`, `copy`, `rename`, `delete` and an unknown
  command typed in turn, the files checked on disk after each, `quit`, exit 0.

### Part D — the last milestone

- **MS-25 — parsing input beyond `--help` (D14).** Only after MS-24. What Q14 rules: options
  and their values, a quoted path with spaces, more words after a command, and one parser for
  the command line and the prompt's lines. **The final milestone of this file.**

## 9. What this must not break

- **S0xx carries on, exit 0.** A "you forgot" that stops a program which would have run
  correctly is the more-fatal direction the severity policy rules out.
- **check.sh opens no window.** Every window row stays headless; MS-3, MS-13, MS-14 and MS-24
  are proved on a compositor of their own, as SATELLITE_WINDOW.md Part 2c was.
- **Every corrected line satl prints is a line that runs.** Every example in this file was run
  before it was written down.
- **A wrong "you forgot" is worse than none** (Q7).
- **003's numbers** (Q2).
- **A file is never overwritten by a word that makes one** — `new` never replaces, and `copy`
  and `rename` must not either.
