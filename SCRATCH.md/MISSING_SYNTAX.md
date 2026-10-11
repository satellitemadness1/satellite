# MISSING SYNTAX — every element a program leaves out is named, with the lines that put it back, and a tutorial that builds a whole program

**A PLAN, 2026-10-03. MS-1 IS BUILT (e943259, revision 09 build 0004), MS-2 (1172148, build 0008) AND MS-3 (926e7db, build 0010); MS-4 IS NEXT, on the author's word.** Written from the author's prompts of
2026-10-03 on satellite 004 revision 09 build 0001, rewritten into milestones from his ninth
message, and given build instructions and a register from his tenth. Section 1 is his words,
as he wrote them. Section 3 is MEASURED — every example in this file was run. Section 4 is the
register, numbered on his word: *"add numbers && renumber as you want to"*. Section 7 is what
is still his. Section 8 is the milestones, each with how to build it and how to prove it, **built
one after the other, each proved before the next begins** — his words: *"properly followed, one
after the other, until it is completed"*.

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
> 10. *"can you make the milestones slightly (slightly now) smaller? You have "every single
>    element the interpreter is missing" as a single milestone, that should be broken into M...
>    oh okay that's what you did, nevermind... and can you change that before we start to
>    arguments.arg1 || args1 || arguments1 and no _underscore?? They are going to have to look
>    it up no matter what almost, but we will try to guess.. || you could alias them as the
>    same thing. Severity is minor for this stuff, unless it's satellite.include(satellite),
>    where  I don't even think that the program would execute.. add numbers && renumber as you
>    want to, whatever you think you should do, arguments.missing = false right where a
>    satellite word would go, satellite.something.something... the prompt reads a line from
>    the console built into the new window, almost everything is not built just yet, and we
>    need to find a way to build all of it somehow, using a milestone + some instructions that
>    you have written..."*
> 11. *"okay start MS-1; and let's have argument1 = "--help" as in the case of "satl
>    filename.satl --help" as the command entered into a prompt, but then where does that leave
>    the argv at?? Let's just put argv as argument1, then argument2 will be filename.satl, then
>    argument3 will be --help, so it's kept in order, and arguments returns a note about every
>    argument that exists, starting from argv at argument1, and the entire arguments special
>    variable..."* — and: *"let's allow bare false/true on the special arguments variable as it
>    breaks all of the rules, and also allow it to take satellite.bool.true, or
>    satellite.bool.false as that is clear by what the user meant,"*

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
| D7 | milestones for each element of a full program, with a window | 8 |
| D8 | unless the program sets `arguments.window = false` or `arguments.interpreter = true`, it is given a way to travel to a program that contains a window | 8 |
| D9 | all of it can be turned off: `arguments.missing = false`. **The default is true** | 9 |
| D10 | **the tutorial is on by default** | 9 |
| D11 | the tutorial's program is ONE self-contained `.satl` file, with its own window and its own prompt | 9 |
| D12 | it accepts a few simple commands, added milestone by milestone: **copy, delete, rename** and **create a new file** | 9 |
| D13 | it accepts `--help` as an argument, and parses it | 9 |
| D14 | the milestones are followed in order until the program is complete; **THEN** input parsing beyond `--help` — **the final milestone of this file** | 9 |
| D15 | **before anything else:** a program's own words have **no underscore**, and `arg1`, `args1`, `argument1` and `arguments1` are the SAME row, aliases, *"we will try to guess"*; the row is shown as `argument1`, message 11's own spelling | 10, 11 |
| D16 | **severity is MINOR** for missing syntax — **except `satellite.include(satellite)`**, without which the file does not execute | 10 |
| D17 | the numbers are this file's to choose: *"add numbers && renumber as you want to"* — section 4 | 10 |
| D18 | `arguments.missing = false` is written **where a satellite word goes** — a line of the file's own, as `satellite.library.x = 5` is | 10 |
| D19 | **the prompt reads a line from the console BUILT INTO the window** — not a separate console. This settles message 4 against message 8, and D7 and D11 mean this window | 10 |
| D20 | almost nothing here is built; each milestone carries the instructions to build it | 10 |
| D21 | **the command line is numbered as it was typed:** `argument1` is argv — satl itself — `argument2` the file, `argument3` onwards its words, *"so it's kept in order"*; and the arguments variable, shown, begins with every argument from `argument1`, then the whole of the variable | 11 |
| D22 | **a bare `true` or `false` is taken by the arguments special variable** — *"as it breaks all of the rules"* — and so are `satellite.bool.true` and `satellite.bool.false` | 11 |

**The milestones keep their size** — message 10 asked for smaller ones and then saw that Part B
already gives every missing element its own: *"oh okay that's what you did, nevermind"*. The
one large milestone left, the prompt, is split in three (MS-18 to MS-20).

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

**Where each is said today** — the places section 8's instructions change: S101 and S102 in
`satellite/bytecode/include_shape.cpp`; S103 raised at `program_check.cpp:2677` and
`include_shape.cpp:294`; "no capsule named" in `program_check.cpp`, `capsule_scopes.cpp`,
`capsule_calls.cpp`, `capsule_reach.cpp`, `bytecode_registry.cpp` and `program_walk.cpp`; "line
declaring it" in `program_check.cpp`, `expression.cpp` and `program_walk.cpp`; "no spacesuit
named" in `suit_reach.cpp`. All are the CHECKER's (`satl(check)`, before a line runs), apart
from the walker's copies. **A precedent for "never done":** S742 PROGRAM_NEVER_JOINED is checked
when the run ends.

### 3b. What the tutorial's program needs, and what is there

| it needs | today |
|---|---|
| to read `--help` | **built**: `satellite.main(satellite.variable.arguments args)`; satl does not take the flag for itself. Measured before MS-1 as `args.argument_1` = `--help`, `args.length` 2; since MS-1 it is `args.argument3`, `args.length` 3 |
| a prompt that reads a line and splits it | **built on satl's own console:** `satellite.console.input("> ")` in a `satellite.statement.while`, and `line.split(" ")` — run with `new made.txt` and `quit` piped in, exit 0 |
| its own window | **built:** `satellite.window.new` and `.open()` |
| a console built into that window (D19) | **not built.** A console is a frame of its own today (`satellite.console.new`, GTK_AND_NO_DEPENDENCIES.md GTK-17), `.append` refuses a frame, and the only way to read from a console a program made is `.typed(a_capsule)`, whose capsule runs after `main` — where every capsule exits 64 since c7ec8a1 |
| create a file | **built:** `satellite.file.new(path)`; a file already there gives `.ok` false and `.error` in words (3a) |
| does a file exist | **built:** `satellite.file.exists(path)` |
| copy, rename, delete a file | **no words.** `satellite.file` has `new`, `open`, `clear` and `exists` |
| their errors | S510 FILE_NOT_THERE, S511 FILE_ALREADY_THERE, S512 NOT_A_FILE, S513 FILE_UNREADABLE, S514 FILE_NOT_TEXT are in the register |
| a switch a program may set | `access` is the ONE today: `args.access = satellite.bool.false` writes it into `~/.satl/config.ini`; every other row satl holds is read-only, refused before the program runs. A bare `false` is refused — a program writes `satellite.bool.false` |
| `arguments.missing`, `arguments.window`, `arguments.interpreter` | **none of the three rows exists** |
| an `input` method | **no token:** REGISTRY.satellite has none; `satellite.console.input` is a WORD (walked in `program_walk.cpp`, checked in `program_check.cpp`) |

## 4. The register — decided here, on D16 and D17

**THE RULE (D16).** Missing syntax is MINOR. Where the program can still run and its meaning is
certain, the code is **S03x: the run carries on, exit 0**, one quiet line and the corrected lines
under it. Where it cannot — a call to a capsule nobody wrote has no meaning to run — it stops in
the LOWEST stopping band, **S15x: "stops, and it is something simple: a missing line"**
(SATELLITE_ERROR.md Part 4), with the corrected lines in the report. **`satellite.include(satellite)`
is the exception he named:** without it the file is not a program and does not execute — S101,
kept. Every new code begins MISSING_, so one search finds the family, and the kind is in the name
(D4): capsule, object, variable, spacesuit, part, close, argument.

**THE RUN CARRIES ON — S03x, exit 0, turned off by `arguments.missing = false`:**

| code | name | what is missing | today |
|---|---|---|---|
| **S030** | MISSING_PROGRAM | everything — a blank file; the corrected lines are Hello, World! (D5) | S101, exit 10 |
| **S031** | MISSING_RETURN | `satellite.return(satellite)`, main's last line — the run carries on as if it were there | S103, exit 12 |
| **S032** | MISSING_WINDOW | a window, in a program that has none (D6) — the quietest; also off with `arguments.window = false` or `arguments.interpreter = true` | — |
| **S033** | MISSING_OBJECT | the object of a line the program needs and never wrote — `my_window` of `my_window.open()`; `t` of `t.start()` (D4) | — |
| **S034** | MISSING_CAPSULE | the capsule of that line — `open()`, `start()` (D4) | — |

**THE RUN STOPS — S1xx, each with its own exit:**

| code | name | what is missing | exit | today |
|---|---|---|---|---|
| **S101** | MISSING_INCLUDE (was FILE_HAS_NO_INCLUDE) | `satellite.include(satellite)` — **the exception: not minor** | 10, kept | S101 |
| **S150** | MISSING_MAIN (was S102 FILE_HAS_NO_MAIN) | `satellite.capsule satellite.main()` — nowhere to begin | 11, kept | S102 |
| **S151** | MISSING_CAPSULE_LINES | a capsule's own lines — `greet()` called and never written | 78 | S110, exit 13 |
| **S152** | MISSING_DECLARATION | a variable's declaration — `greeting` used and never declared | 79 | S201, exit 25 |
| **S153** | MISSING_SPACESUIT | an object's spacesuit — `rocket my_rocket` with no `satellite.spacesuit rocket` | 80 | S201, exit 25 |
| **S154** | MISSING_PART | a statement's part — an `if` or `while` with no condition or no `{ }`, an `else` with no `if`, a `for` without its three parts | 81 | various |
| **S155** | MISSING_CLOSE | a `}` or a `)` | 82 | S110 |
| **S156** | MISSING_ARGUMENT | an argument a call needs — `satellite.window.new("t", 800)` | 83 | S110, exit 13 |

**Exits 78–83, not 77–82 as first written (MS-3):** automake's and meson's test drivers read an
exit of 77 as SKIPPED, so 77 is never given.

**Free when chosen (2026-10-03, and after MS-3 took the rows above):** S017–S019, S021–S029 and
S035–S099 in the warning band; S104–S109, S111–S119, S122–S129, S131–S139, S141–S149 and S157–S199
in the first stopping band; exits 84–129 and 131–254 (0–76, 78–83 and 130 are taken, 77 and 255
are never given).
S102 and S103 are left unused rather than reused, so an old log line never means something new.
Exit 12 (`satl_file_missing_satellite_return_satellite`) is left in `machine_codes.hpp`, unused,
for the same reason.

## 5. One report — every corrected example below was run, and exits 0

**THE QUIET SHAPE, S03x** — the one-line notice satl already prints
(`[satellite] S010 CONFIG_FILE_MISSING: ...`, `print_notice`), one line for each missing
element, each with its kind's own code (D4), and the corrected lines once, under them:

    [satellite] S033 MISSING_OBJECT: my_window -- the object of a line this program is missing
    [satellite] S034 MISSING_CAPSULE: open() -- my_window is declared on line 5 and never opened
        the lines, corrected:

            satellite.variable.window my_window = satellite.window.new("gtkcar", 450, 250)
            my_window.open()
            my_window.close()

**THE STOP SHAPE, S1xx** — today's eighty-column report, with `the lines, corrected:` and the
lines under its sentence.

**A BLANK FILE — S030** — the corrected lines are the whole of Hello, World!, the README's own:

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("Hello, World!")

        satellite.return(satellite)
    }

**S151** — `greet()` called and never written:

    satellite.capsule greet()
    {
        satellite.console.display("Hello, World!")
    }

**S152** — `greeting` used and never declared:

    satellite.variable.string greeting = "Hello, World!"
    satellite.console.display(greeting)

**S153** — `rocket my_rocket` with no `satellite.spacesuit rocket` anywhere:

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

**S033 + S034, a doing never done** — a thread made with `satellite.thread.new(work())` and the
two lines that run it, `t.start()` and `t.join()`; a program `{"echo", "hello"}` and `p.start()`
and `p.join()`.

## 6. The tutorial — the path, and the program at the end of it

**THE PATH (D7, D8, D10).** Each step is what the program before it is missing. With
`arguments.missing` true — the default — a program on this path is shown its next step.

| step | element | its kind | what the step adds |
|---|---|---|---|
| 0 | nothing — a blank file | — | the whole of Hello, World! (S030) |
| 1 | `satellite.include(satellite)` | the file's word | the first line (S101) |
| 2 | `satellite.capsule satellite.main()` | capsule | where the program begins (S150) |
| 3 | `satellite.return(satellite)` | the file's word | main's last line (S031) |
| 4 | `satellite.console.display("Hello, World!")` | capsule, on the console satl gives every program | a line out |
| 5 | `satellite.variable.string greeting = ...` | variable | a value kept (S152) |
| 6 | `satellite.capsule greet()` | capsule | a capsule of the program's own (S151) |
| 7 | `satellite.spacesuit rocket()` and `rocket my_rocket` | object, and its type | an object (S153) |
| 8 | `satellite.statement.if`, `else`, `while` | statement | a choice and a loop (S154) |
| 9 | `satellite.variable.window my_window = satellite.window.new(...)` | object | a window, declared (D1, S032) |
| 10 | `my_window.open()` | object + capsule | the window on the screen (S033, S034) |
| 11 | `satellite.window.label(...)`, `my_window.append(...)` | object + capsule | something in it |
| 12 | `my_window.display(...)` | capsule | a line into the console built into the window (D19) |
| 13 | `my_window.input("> ")`, split into words | capsule | a command loop at the window's prompt |
| 14 | `new`, then `copy`, `rename`, `delete` | capsule, one a command | the commands (D12) |
| 15 | `satellite.main(satellite.variable.arguments args)` and `args.argument3` | variable | its own `--help` (D13, D21) |

**THE PROGRAM AT THE END OF IT (D11, D19).** One self-contained file. Every line marked
`NOT BUILT` waits on the milestone it names; every other line was run (section 3b): `--help`
under today's spelling, the loop, the split, and `new` with its `.ok` and `.error` — piped
`new made.txt` twice and then `quit`, it made the file, then printed the file's own error, then
exited 0.

    satellite.include(satellite)

    satellite.capsule satellite.main(satellite.variable.arguments args)
    {
        satellite.variable.bool asked_for_help = satellite.bool.false
        satellite.statement.if(args.length > 2)
        {
            asked_for_help = args.argument3 == "--help"
        }

        satellite.statement.if(asked_for_help)
        {
            satellite.console.display("files -- new, copy, rename and delete a file, at its own prompt")
        }
        satellite.statement.else
        {
            satellite.variable.window my_window = satellite.window.new("files", 800, 500)
            my_window.append(satellite.window.label("type a command at the prompt below"), 400, 40)
            my_window.open()

            satellite.variable.bool running = satellite.bool.true
            satellite.statement.while(running)
            {
                satellite.variable.string line = my_window.input("> ")          // NOT BUILT: MS-20
                satellite.container.list<satellite.variable.string> words = line.split(" ")
                satellite.statement.if(words[1] == "new")
                {
                    satellite.variable.file made = satellite.file.new(words[2])
                    satellite.statement.if(made.ok)
                    {
                        my_window.display("made " + words[2])                   // NOT BUILT: MS-19
                    }
                    satellite.statement.else
                    {
                        my_window.display(made.error)                           // NOT BUILT: MS-19
                    }
                }
                satellite.statement.if(words[1] == "copy")
                {
                    satellite.variable.file copied = satellite.file.copy(words[2], words[3])   // NOT BUILT: MS-24
                }
                satellite.statement.if(words[1] == "rename")
                {
                    satellite.variable.file renamed = satellite.file.rename(words[2], words[3]) // NOT BUILT: MS-25
                }
                satellite.statement.if(words[1] == "delete")
                {
                    satellite.variable.file deleted = satellite.file.delete(words[2])          // NOT BUILT: MS-26
                }
                satellite.statement.if(words[1] == "quit")
                {
                    running = satellite.bool.false
                }
            }
            my_window.close()
        }
        satellite.return(satellite)
    }

`copy`, `rename` and `delete` are written here answering a file, with `.ok` and `.error` as `new`
does — a proposal (Q12). A window and a console, opened and written to, ran on a compositor of
their own: exit 0, both drawn.

## 7. Open — the author's, and nothing below is decided

Answered by messages 10 and 11, and gone from this list: which band (D16), renumbering (D17),
where `arguments.missing` is written (D18), where the prompt is (D19), which spellings of an
argument answer (D15, D21), and the bare `false` (D22).

- **Q3. S032 "missing a window" — every run of every program with no window?** Hello, World!
  and every console script would say it each time. The severity policy wants one quiet line, and
  S020 goes to satellite.log only. Console, log, or both; every run, or once a program? Does
  `satl --repl` count as `arguments.interpreter = true`?
- **Q4. Where else are the three switches set?** *(ANSWERED 2026-10-06 -- typed at the prompt it
  is saved in config.ini for every run; see MS-2's last note. arguments.window and
  arguments.interpreter do not exist yet.)* D18 puts `arguments.missing` in the program.
  config.ini (`missing = false`) would make it the machine's; the command line, the run's.
- **Q5. What does the tutorial SHOW?** The next step's line or two (section 6), the whole next
  program, or a command that walks the steps — `satl --tutorial`, or a `satellite.help` topic?
- **Q6. Does EVERY window carry a console, or only one the program writes to or reads from?**
  Recommended: every window has it and it takes no room until the program first uses it, so a
  window that never prompts looks as it does today.
- **Q7. Before the run, or at its end?** The checker could say "made and never opened" before a
  line runs, but it cannot follow a window into a capsule that opens it, through `w2 = w`, or
  into an included file — and a wrong "you forgot" is worse than none. Recommended: S031 and the
  S15x at the checker, where they already are; S032, S033 and S034 when the run ends, as S742 is.
- **Q8. What does a declared, unopened window look like?** `.ok` is false. Display says
  `(window "t")` open and `(closed window "t")` closed; a third word for "not opened yet"?
- **Q9. What may a program do to a window before `.open()`?** Recommended: everything that builds
  it — `.append`, `.title`, `.resize`, `.colour`, `.background`, `.font`, `.menu`, `.key`,
  `.clicked`, `.every`, `.closed`, `.display` — and refuse what needs it on a screen: `.focus()`,
  `.message`, `.ask`, `.choose_a_file`, `.input` and `.press()` on a button inside it.
- **Q10. Is a console made by `satellite.console.new` declared too, and opened by `.open()`?**
  Recommended: yes — one rule for every frame; 003's own example wrote `my_terminal.open()`.
- **Q11. *"create a new file (we already have that) but we don't have a error command built for
  it"* — which error is missing?** Measured: `satellite.file.new` on a file already there makes
  nothing, says nothing, and answers `.ok` false with `.error` in words. So it is one of: (a) satl
  SAYING it rather than waiting to be asked; (b) a missing-syntax line for a program that never
  asks `.ok`; (c) an error command in the tutorial's program; (d) the words for copy, rename and
  delete, which have no errors because they have no words yet.
- **Q12. What do `copy`, `rename` and `delete` answer?** A file with `.ok` and `.error`, as
  section 6 writes them — `delete`'s being the file that is gone — or a bool?
- **Q13. Is `delete` undone by anything?** Does the prompt ask "are you sure", does the program
  decide, or neither?
- **Q14. Parsing beyond `--help` (D14) — how far, and whose?** Options with values
  (`--from a.txt --to b.txt`), a path with spaces in quotes, more than one word after a command,
  one parser for the command line and the prompt alike. Satellite words a program calls, or a
  capsule the tutorial's program writes for itself?

## 8. The milestones — one after the other, each proved before the next begins

**THE GATE FOR EVERY MILESTONE:** its own check.sh rows pass, and then the whole of check.sh —
which opens no window, so a window row stays headless; anything that opens a window is proved
on a compositor of its own, WAYLAND_DEBUG counting each toplevel's buffers (SATELLITE_WINDOW.md
Part 2c's rig); every corrected line it prints was run first; one fresh reader reviews the diff;
and -- NO GIT since 2026-10-05, the author: *"Nothing at all"* -- the tree is snapshotted before
the first edit and the milestone's patch written beside that snapshot when it is done
(~/.claude/projects/-home-madness-code-cxx-satellite/backups/), before the next begins. **Read before building any of it:**
`satellite/machine/s_codes.hpp`, `satellite/machine/critical_report.hpp` (`print_notice` at
`:301`), and `structured-library.cpp:248-262`, where S010's notice is printed.

### Part A — what everything after it stands on

**MS-1 — the command line numbered as it was typed, without an underscore (D15, D21). BUILT
2026-10-03, revision 09 build 0004** — check.sh and one fresh reader as below.
- BUILD: `CommandLine` keeps argv[0] (`satellite/arguments/command_line.hpp`, set first in
  `read_command_line`). `gather()` (`arguments.cpp`) adds `arguments.argument1` = satl,
  `argument2` = the file, `argument3` onwards the words, and `length` = all of them. ONE
  function, `the_argument_row_spelled`, makes `arg3`, `args3`, `arguments3` and `argument03`
  the row `argument3`, called by the read in `bytecode/main_arguments.cpp`'s
  `read_an_argument`; `filled_in_by_satl` refuses every spelling as a config row or a program's
  write. **The old `argument_N` meant the first word after the file, and that is
  `argument(N+2)` now** — so it is refused, naming the row it means
  (`the_row_argument_underscore_is_now`), never read as `argument1`; `arg0` is refused with S413.
  `every_row_arguments_first` gives the variable (`the_arguments_value`) and `satl --debug` the
  command line first. Then `arguments_cases.cpp`, `command_line.cpp`'s help lines,
  `satellite.help/arguments` and `satellite.help/satellite.main`, `SATELLITE_ARGUMENTS.md`,
  `tests/arguments_*.satl` (three moved, three new: `arguments_typed`, `arguments_old_name`,
  `arguments_zero`), check.sh's rows — and the author's `experiments/quad_test/quad_test.satl`,
  whose `x.length > 1` and `x.argument_1` would otherwise have changed meaning without a word.
- PROVE: `satl prog.satl one` — `argument1` is satl as it was started, `argument2` the file,
  `argument3`, `arg3`, `args3`, `arguments3` and `argument03` each `one`, `length` 3; the variable
  displayed begins `{"argument1": ..., "argument2": ..., "argument3": "one", "length": 3,`;
  `args.argument_1` refused naming `args.argument3`; `args.arg0` S413; a write to any spelling
  refused before the run.
- WHAT THE FRESH READER CHANGED: a key handed over as TEXT takes every spelling too —
  `args["arg3"]`, `args.contains("arguments3")`, `args.remove("arg3")` — through one helper,
  `an_argument_key`, with the old name and 0 refused in the same words; and `satl "" a b` keeps
  `argument2` for the file. Its second finding, `args.set("argument3", ...)` writing past satl's
  rows, does not happen: the checker refuses `.get` and `.set` on the arguments variable before
  anything runs ("no capsule named set") — measured, so no guard was added that could never run.
- WHAT IT TOUCHED OUTSIDE THE REPO, AND DID NOT CHANGE: the author's own probes under
  `~/code/satl` — `mythkeepers/probes/threads_skeleton.satl`, `display_mock.satl`,
  `threads_overlap_skeleton.satl`, `shell_skeleton.satl`, `acorps/probes/arguments.satl`,
  `guard_probe.satl` — read `argument_N` and guard on `args.length > 1` or count
  `args.length - 1`. Each old name is now refused with its new one; each `> 1` must become `> 2`
  and each `- 1` become `- 2`, or the count is one off. His to change, or to ask for.

**MS-2 — `arguments.missing`, the switch, where a satellite word goes (D9, D18). BUILT 2026-10-04,
1172148, revision 09 build 0008** — check.sh and one fresh reader as below.
- BUILD: a flag row in `satellite/config/satellite_config.hpp`,
  `{"arguments.missing", 0, true, true}`, beside `arguments.startup_display`. A line of the file's
  own, `arguments.missing = false` (and its full name, `satellite.library.arguments.missing`), read
  by the same scan as `satellite.library.x = 5` (`bytecode/library_values.cpp`, "the scan's line")
  and checked where those are (`program_check.cpp`), setting the run's row BEFORE the checker
  says anything missing — every MISSING code asks it. **D22:** a bare `true` or `false` is taken
  here and by every write to the arguments variable (`args.access = false`), and so are
  `satellite.bool.true` and `satellite.bool.false` — `write_an_argument` in
  `bytecode/main_arguments.cpp`, and the checker's view of the same line.
- PROVE: a file with the line and a file without, side by side; the row read back.
- AS BUILT, WHERE IT DIFFERS FROM THE LINES ABOVE: the line's reader is a file of its own,
  `satellite/bytecode/argument_switches.cpp` (`scan::switch_line`), called by the scan beside
  `library_line` (`capsule_scopes.cpp`; `suit_scan.cpp` refuses one in a spacesuit) -- by subject,
  and library_values.cpp was 443 lines already. Its refusals are the scan's, said by check_program
  where a satellite.library line's are. **To set the row before EVERY missing code, run_satl now
  scans the files before `file_can_run`**: S030, S150 and S031 are include_shape.cpp's, raised
  ahead of check_program; the scan says nothing itself and always read every included file whatever
  its shape, so the order changes nothing it finds (the fresh reader ran 36 broken files and 603
  stripped tests through both binaries: the same reports). `set_the_program_s_switches` gives the
  row the main file's value (`Arguments::the_program_sets`). **D22 is read where the value is,
  not in `write_an_argument`**, which is handed a value already worked out -- and `false` worked
  out is an undeclared name: `a_bare_true_or_false` (main_arguments.cpp), asked by three arms in
  each of program_check.cpp and program_walk.cpp (`args.row`, `args["row"]`, and
  `satellite.library.arguments.access`). Written once, true or false and nothing else (S310); twice
  is S202; `access` at the top is S220, any other row S110 as before; the old place
  `satellite.library.main.arguments.missing` is refused naming the spelling; inside a capsule
  (`args.missing`, `args["missing"]`, `arguments.missing`, the full name) S220 before anything runs,
  and at the prompt it is told the prompt has no file. tests/arguments_missing.satl,
  arguments_missing_not_written.satl, arguments_missing_full_name.satl, arguments_bare.satl; 30
  check.sh rows; satellite.help(arguments); SATELLITE_ARGUMENTS.md Part 4F.
- PROVED: tests/arguments_missing.satl reads `false` and the same file without its line `true`,
  each read back by main; the full name with satellite.bool.false, `false`; under --debug, the
  program's own line is said before `satl.file(runnable)` -- before the file's shape is checked,
  where the first MISSING code will be raised. **Nothing reads the row yet:** MS-3 numbers the
  codes and MS-5 on raise them.
- MY CHOICES, HIS TO OVERRULE: (1) a file the program includes keeps its switch for when it runs
  by itself, and does not set the program's row -- one place, `set_the_program_s_switches`; (2) on
  the arguments the bare word is the value ALWAYS, even where a variable is named false (read as
  `(false)`) -- no program of his declares one; (3) the full name is a spelling of the line only:
  `satellite.library.arguments.missing` is not readable in a capsule, as no config row is
  (`satellite.library.arguments.infinity` is not either), so the row is read back as
  `args.missing`; (4) the old place is refused rather than taken. Where else the switch may be
  set -- config.ini, the command line -- is still Q4.
- WHAT THE FRESH READER CHANGED (seven, all low to medium-low, none a crash or a regression): the
  bare word decided by "was a variable named false declared" made the checker and the walker
  disagree -- a `false` declared further down a loop's body gave one line false and then true --
  so it is read off the codes alone; `args["missing"] = ...` was refused only after main's lines
  had run, under a sentence saying it is too late, and is now refused before anything runs; the
  old place's refusal suggested lines that are refused themselves; a switch in a spaceship was
  accepted and read by nothing; the sentence for another row at the top was untrue of the
  program's own rows; "said before main begins" was untrue of S032-S034, which come at the run's
  end, and at the prompt it sent a person to a file there is none of; and `arguments.missing =`,
  its value forgotten, was refused under the next line written -- a line ending in = goes on into
  the next, so that line WAS its value -- and the line is now read back as it was written.
  **Seen, older, and left:** a closed `{ }` at a file's top is transparent, so a switch inside one
  is read; `args[1] = satellite.bool.false` files a row under the key 1 while `args.argument1`
  still says satl's path.
- **CHANGED 2026-10-06, BEFORE MS-4, ON HIS WORD** (SATELLITE_ARGUMENTS.md Part 4G): *"before we
  build this, we need to build into the interpreter the syntax arguments.item(value_to_change_to)
  and if it's typed ON the prompt it permanently changes the value, and if it's typed in a file it
  temporarily changes a value, and the alias syntax using = ... that does the same thing"*. So this
  milestone's line is `arguments.missing(false)` as well as `arguments.missing = false` -- the same
  line, this run's -- and Q4 has its answer: typed at the prompt, `arguments.missing(false)` is
  saved in config.ini (`missing = false`) for every run. Every setting, not only the switch, may be
  written at a file's top now. **No `.permanent`**: his 2026-10-04 `arguments.missing.permanent(value)`
  was built that morning and taken out the same day -- *"so you took out the .permanent() right??"*
  Built as revision 09 build 0025 (check.sh 1229/1229); no git, the patch beside the snapshot in
  ~/.claude/projects/-home-madness-code-cxx-satellite/backups/2026-10-06-before-arguments-call/.

**MS-3 — the register (section 4). BUILT 2026-10-04, 926e7db, revision 09 build 0010** — check.sh and one
fresh reader as below.
- BUILD: the S15x rows in `satellite/machine/s_codes.hpp`, beside S101; exits 77–82 in
  `satellite/machine/machine_codes.hpp`; S101 renamed MISSING_INCLUDE and S102's row moved to
  S150. The S03x are notices: a `CriticalReport` printed by `print_notice`, as S010 is, and kept in
  satellite.log, as S020 is (`s_codes.hpp:497`). A list of corrected lines on `CriticalReport`,
  printed under the sentence in both shapes. SATELLITE_ERROR.md Part 4 gets the rows.
- PROVE: one check.sh row a code — number, name, exit — before any code is raised anywhere.
- AS BUILT: machine codes **78–83** (`missing_capsule_lines`, `missing_declaration`,
  `missing_spacesuit`, `missing_part`, `missing_close`, `missing_argument`) -- 77 is never given,
  because automake's and meson's test drivers read it as SKIPPED -- and their S151–S156
  rows in `s_code_for`, with code 10 renamed S101 MISSING_INCLUDE and code 11's row moved to S150
  MISSING_MAIN -- S102 is nobody's now, and S103 stays exit 12's until MS-8. The five S03x are
  `SCode` constants beside them, `kMissingProgram` to `kMissingCapsule`, all five in
  `kMissingNotices`, each with no machine code. `CriticalReport::corrected` holds the lines, one
  line of satellite a string: `render()` prints them under the sentence, after `the lines,
  corrected:`, four columns in and before `directory:`; `notice_text()` -- what `print_notice`
  prints and logs now -- under the notice's one line, the heading four in and the lines eight, as
  section 5 drew it. Never wrapped, each shown() but a tab kept a tab, a blank line kept blank, and
  a report with no corrected lines byte for byte what it was. The lines are part of the report
  tally's key, so two notices of one code missing different lines are both said. **S101 and S150 are raised today, so their names
  show at once** -- `satl tests/missing_main.satl` says `S150: MISSING_MAIN`, exit 11; the rows for
  missing_include.satl and missing_main.satl now assert the code and the name, and the
  satellite.main help page and two rows say S150. SATELLITE_ERROR.md Part 4 has the family.
- PROVED: `build/missing_cases` (satellite/machine/missing_cases.cpp, one of ALL_TARGETS) asks
  every code through the `s_code_for` and `exit_status_of` satl reports with: thirteen numbers,
  names and exits; S102 reached by no machine code; each MISSING number one code's; S103 still
  exit 12's; 77 no code's; both shapes, a long line unwrapped, an escape shown, a tab kept, the
  unchanged report; and `print_notice` itself, on the screen and into a satellite.log of its own,
  one entry a notice. check.sh runs it -- one row a code, thirteen, and one for the harness.
- MY CHOICES, HIS TO OVERRULE: the `means` sentences, drafted from his words; the corrected lines
  under the sentence, so `directory:` and `syntax:` move down in a report that has them (the
  report header's "four rows always in the same place" now says so -- under the caret is the
  other place); 78–83 for 77–82.
- WHAT THE FRESH READER CHANGED (no regression: 13 probes and 159 tests identical between the old
  binary and the new but for the two names): a tab printed as `\x09`, so a tab-indented corrected
  line would not run; two notices of one code with no place were one to the tally, and the second
  lost from the screen and the log; the harness never called `print_notice`; exit 77 is SKIPPED to
  test drivers; and untrue sentences -- S032 said settled when Q3 is open, "the run stops here"
  for codes raised before anything runs, "exits 83 onward", and three S102s. MS-6, MS-7, MS-16 and
  MS-17 carry what they must do about it. **Seen, older, and left:** S150 (FILE_HAS_NO_MAIN before)
  names a bare `satellite.main()` line even in a file that has a main.

**MS-4 — `new` declares, `.open()` opens (D1).**
- BUILD: in `satellite_variable_window/satellite_window.cpp`, `put_the_frame_up` stops presenting;
  `frame_new` builds the GtkWindow, the column and the fixed, and holds it on the desk (its
  `destroy` handler points into it) with `on_the_screen` false. `window_open.cpp`: a declared
  window is PRESENTED (no new frame) and waited for, a closed one is rebuilt as today.
  `window_desk.cpp`: `a_program_window_is_open()` counts only windows on a screen, and
  `close_the_desk_when_the_windows_are` destroys the declared, never-opened ones before it quits.
  The builders of Q9 work on a declared window (`widget != nullptr`), the screen-needing ones keep
  `still_there`; `.every`'s clock starts at `.open()` (`the_window_answers_again` already does
  that for a reopened window). A console (Q10) the same. **Every program that relies on `new`
  showing a window gains `.open()`:** `examples/window.satl`,
  `tests/threads_window_on_a_thread.satl`, the programs inside `press-a-button.sh`,
  `prove-bare-machine.sh`, `prove-canvas-tabs-menus.sh` and `prove-console.sh`, the examples in the
  `satellite.window`, `satellite.variable.window`, `satellite.console` and `satellite.terminal`
  help pages, `experiments/window_test/windows_10.satl` — and the author's own
  `window_and_extras.satl`, which is his to change.
- PROVE: on a compositor — a declared window draws 0 buffers, `.open()` draws it, and a run whose
  only windows were never opened ends by itself.

### Part B — every missing element of a program, in the order the tutorial meets them

**MS-5 — a blank file answers with Hello, World! — S030.**
- BUILD: `bytecode/include_shape.cpp`, before S101: a file with no tokens — nothing, blank lines,
  comments — is S030 with section 5's program as its corrected lines, and exit 0.
- PROVE: all three shapes of blank; the printed program, saved and run, prints Hello, World!.

**MS-6 — `satellite.include(satellite)` missing — S101 MISSING_INCLUDE.**
- BUILD: `include_shape.cpp`'s S101 gets its corrected line, the include above the file's first
  line. It still stops: exit 10. Its `means` in `s_code_for` ends in its own "Add this as the first
  line" block today; the corrected line replaces it, or the report says it twice.
- PROVE: the corrected file runs.

**MS-7 — `satellite.main` missing — S150.**
- BUILD: S150 (MS-3 moved S102's row to it; raised from `include_shape.cpp` and, for
  `satellite.main()` written without `satellite.capsule`, `capsule_scopes.cpp`) gets its corrected
  lines: main written around the lines the file has outside any capsule. Exit 11. Its `means` ends
  in its own "Add:" block, which the corrected lines replace.
- PROVE: the corrected file runs and does what the loose lines said.

**MS-8 — `satellite.return(satellite)` missing — S031, the run carries on.**
- BUILD: the two raises of S103 (`program_check.cpp:2677`, `include_shape.cpp:294`) become the
  S031 notice, and main's closing `}` returns as `satellite.return(satellite)` does.
- PROVE: a main without it runs to its end, exit 0, with the one line said.

**MS-9 — a capsule's lines missing — S151.**
- BUILD: "no capsule named" (`program_check.cpp`, and the five other files of section 3a) becomes
  S151, its corrected lines the capsule written out — its name, and one parameter for each
  argument the call hands it.
- PROVE: the corrected capsule, pasted in, runs.

**MS-10 — a variable's declaration missing — S152.**
- BUILD: "line declaring it" (`program_check.cpp`, `expression.cpp`) becomes S152, its corrected
  line a declaration with the type the use suggests — a string where text is given it.
- PROVE: as MS-9.

**MS-11 — an object's spacesuit missing — S153.**
- BUILD: "no spacesuit named" (`suit_reach.cpp`) becomes S153, its corrected lines a spacesuit
  holding a capsule for each one the program calls on the object.
- PROVE: as MS-9.

**MS-12 — a statement's part missing — S154.**
- BUILD: where `program_check.cpp` reads `satellite.statement.if`, `else`, `while` and `for`.
- PROVE: each statement with each part left out; each corrected one runs.

**MS-13 — a close missing — S155.**
- BUILD: wherever "( is never closed on its line" and "a list left open" are said today.
- PROVE: a `}` and a `)` each left out, at the end of a file and in its middle.

**MS-14 — an argument missing — S156.**
- BUILD: the arity refusals that say "takes N arguments, and was given M", when M is fewer
  (`window_word_arity`, a capsule's own count in `program_check.cpp`); the corrected line fills the
  missing ones with an example value, and says that it is one.
- PROVE: `satellite.window.new("t", 800)`; a capsule of two called with one.

**MS-15 — the window a program does not have — S032 (D6, D8).**
- BUILD: when the run ends, a program that made no window says the quietest line — not with
  `arguments.window = false`, `arguments.interpreter = true` or `arguments.missing = false` (two
  new flag rows beside MS-2's), and as Q3 rules.
- PROVE: Hello, World! with each switch, and without.

**MS-16 — `.open()` missing — S033 + S034 (D2, D4).**
- BUILD: the desk knows every window made and never opened (MS-4); each handle keeps the line it
  was made on; when the run ends each is named, object and capsule, with the three corrected lines.
  **Each notice carries its place in `directory`** -- the file and the line -- or two windows'
  S033s with no corrected lines are one to the report tally (MS-3's fresh reader).
- PROVE: `gtkcar/satl_window.satl` without its `.open()` — the two lines, exit 0.

**MS-17 — a doing never done — S033 + S034.**
- BUILD: when the run ends, every thread, program and bash made and never `.start()`ed
  (`bytecode/thread_calls.cpp`, `program_calls.cpp`), the way S742 finds one never joined -- each
  notice placed, as MS-16's are.
- PROVE: section 3a's two silent programs say their two lines each.

### Part C — the prompt, in the window (D19), in three

**MS-18 — the console built into the window.**
- BUILD: `put_the_frame_up`'s column gets a VteTerminal below the fixed, made with the code of
  `window_console.cpp`'s `a_terminal_to_type_in` — the pty, the slave, and the teardown in
  `the_console_went_away` — and taking no room until it is first used (Q6).
- PROVE: on a compositor, a window that never uses it draws as it does today.

**MS-19 — writing to it: `my_window.display("words")`.**
- BUILD: `console_display` (`window_console.cpp`) on a window's own console; `window_methods.cpp`
  and `window_shapes.cpp` already know `display` for a console.
- PROVE: a line written shows in the window's console; `.columns` and `.rows` answer.

**MS-20 — reading a line: `my_window.input("> ")`.**
- BUILD: a new method token `input` (REGISTRY.satellite, then
  `python3 satellite/bytecode/make_token_codes.py` — GTK-0's step 3). It writes the prompt to the
  slave and reads ONE line from it on the INTERPRETER's thread — the pty is in canonical mode, so a
  read is a whole line — while the desk keeps drawing. Refused while `.typed` is set on the same
  console. A `satellite.console.new` console takes it too.
- PROVE: on a compositor, a line typed through mutter's RemoteDesktop keyboard (the clicker of
  `press-a-button.sh`, `NotifyKeyboardKeycode`) comes back to the program.

### Part D — the tutorial's program, one command at a time (D10–D13)

**MS-21 — the tutorial, on by default (D10).**
- BUILD: with `arguments.missing` true, a program on section 6's path is shown its next step, as
  Q5 rules — carried by the line its step is missing.
- PROVE: each step of section 6, run in turn, is shown the step after it; the last, nothing.

**MS-22 — the skeleton.** One `.satl` file — its window, `.open()`, and the loop that reads a line
at the window's prompt, splits it, and stops on `quit`.
- PROVE: on a compositor, `quit` typed ends it, exit 0.

**MS-23 — `new <file>`.** Built as a word; the skeleton prints `.error` when `.ok` is false, and
Q11's answer is built here.

**MS-24 — `copy <from> <to>`.**
- BUILD: a row in `words/words_004.tsv` at the next free number under `1 8`
  (`satellite.file.copy(from, to)`), then `python3 words/make_words.py` and
  `python3 satellite/bytecode/make_word_codes.py` — never `word_codes.hpp` by hand; the call in
  `bytecode/file_calls.cpp`'s `call_file_word` (`:327`); the copy in
  `satellite_variable_file/satellite_file.cpp`, NEVER over a file already there (as `new` never
  is); its errors S510, S511, S512, S513.
- PROVE: each error made happen, and the copy's bytes the same as the file's.

**MS-25 — `rename <from> <to>`.** The same three steps, `satellite.file.rename(from, to)`, never
over a file already there (`renameat2` with `RENAME_NOREPLACE`), the same errors.

**MS-26 — `delete <file>`.** The same, `satellite.file.delete(path)`; nothing there, not a file;
and Q13.

**MS-27 — a command it does not know.** The skeleton says so in its own words and the prompt
carries on.

**MS-28 — `--help` (D13).** `satl files.satl --help` prints what it does and exits 0, read from
`args.argument3` (MS-1) — argument1 being satl and argument2 the file; any other first word
is said to be unknown.

**MS-29 — the skeleton is complete.** Section 6's program, all of it, on a compositor: the window
and its prompt drawn; `new`, `copy`, `rename`, `delete` and an unknown command typed in turn, the
files checked on disk after each; `quit`; exit 0.

### Part E — the last milestone

**MS-30 — parsing input beyond `--help` (D14).** Only after MS-29, and as Q14 rules: options and
their values, a quoted path with spaces, more words after a command, one parser for the command
line and the prompt's lines alike. **The final milestone of this file.**

## 9. What this must not break

- **S03x carries on, exit 0.** A "you forgot" that stops a program which would have run correctly
  is the more-fatal direction the severity policy rules out.
- **check.sh opens no window.** MS-4, MS-16, MS-18–MS-20, MS-22 and MS-29 are proved on a
  compositor of their own.
- **Every corrected line satl prints is a line that runs.** Every example in this file was run
  before it was written down.
- **A wrong "you forgot" is worse than none** (Q7).
- **An old code is never reused** — S102 and S103 stay empty, and so does exit 12.
- **A file is never overwritten by a word that makes one** — `new` never replaces, and `copy` and
  `rename` must not either.
