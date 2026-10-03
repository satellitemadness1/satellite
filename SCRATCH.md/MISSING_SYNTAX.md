# MISSING SYNTAX — every element a program leaves out is named, with the lines that put it back

**A PLAN, 2026-10-03. NOTHING IN IT IS BUILT.** Written from the author's prompts of
2026-10-03, on satellite 004 revision 09 build 0001. Section 1 is his words, in order and
as he wrote them; everything after it is measured, proposed, or his to decide, and says
which. Read section 7, the open questions, before building any milestone.

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

## 2. What those words decide, and what they leave open

**DECIDED, by his words** — each is a requirement, not a proposal:

| | decided | from |
|---|---|---|
| D1 | `satellite.window.new` DECLARES a window and does not open it; `.open()` opens it. This answers SATELLITE_WINDOW.md Part 2c's first question, and reverses that part's "new still shows the window" | 1 |
| D2 | a file that makes a window with `satellite.window.new` and never `.open()`s it is told it forgot `.open()` — a **lesser** code, "missing syntax element", and the corrected lines as a short example (about three) | 1 |
| D3 | not only `.open()`: a missing-syntax code for **every** kind of missing syntax element — capsule, object, variable, and the rest | 1, 2 |
| D4 | a missing CAPSULE and a missing OBJECT are different codes. Without `my_window.open()`, two things are missing: the object `my_window` and the capsule `open()` | 3 |
| D5 | a blank file answers with a correct Hello, World!, to get started | 3 |
| D6 | Hello, World! itself has no window, and that is a **minor** error: satl carries GTK, so a program is missing a window by default | 5 |
| D7 | milestones for each element of a full program — with a window, and with a separate console | 8 |
| D8 | unless the program sets `arguments.window = false` or `arguments.interpreter = true`, satl by default gives it a way to travel to a program that contains a window | 8 |

**FLOATED, NOT DECIDED** — *"We could..."*: the whole thing as a satellite tutorial, a
missing sequence of lines that builds a window with a console (4). Message 8 says a
SEPARATE console; message 4 said one INSIDE the window. Which is Q6.

## 3. What satl says today — measured, build 0159 and revision 09 build 0001

Each program run with no display and a scratch HOME. Every "fixed" version of these was run
too, and every one exits 0 (section 5 quotes them).

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
| a window made and never `.open()`ed | nothing; the window shows anyway — `new` opens it, D1 reverses that | — | 0 |

**Where each is said:** S101 and S102 in `bytecode/include_shape.cpp`; S103's sentence in
`machine/s_codes.hpp`; "no capsule named" in six files (`program_check.cpp`,
`capsule_scopes.cpp`, `capsule_calls.cpp`, `capsule_reach.cpp`, `bytecode_registry.cpp`,
`program_walk.cpp`); "line declaring it" in `program_check.cpp`, `expression.cpp`,
`program_walk.cpp`; "no spacesuit named" in `suit_reach.cpp`. All of them are the CHECKER's
(`satl(check)`, before a line runs) except the walker's copies.

**A PRECEDENT FOR "NEVER DONE":** S742 PROGRAM_NEVER_JOINED is already checked when the run
ends — *"Every .start() is paired with a .join()"* — so an end-of-run check for a pair that
was never made exists, in the S7xx band.

## 4. The band a missing element lands in

SATELLITE_ERROR.md Part 4, the author 2026-09-18: **the hundreds digit is the severity** —
*"S00 is stuff that doesn't stop the interpreter, so you have 99 warnings to use, then S01
stops the interpreter on something stupid"*:

| band | means |
|---|---|
| S0xx | a WARNING — the run carries on, exit 0 |
| S1xx | stops, and it is something simple: a missing line, a typo, a word satl does not take |
| S2xx | stops: a NAME — not declared, declared twice, not built yet |

And the severity policy, his: *"let's not make more things fatal, let's make less things
fatal"* — **"fatal" is the FRAME**: a notice is one quiet line, and the eighty-column
report is kept for what deserves it.

**PROPOSED, AND THE BAND OF EACH IS HIS (Q1).** "Lesser" read as: if the program can still
run correctly, it is S0xx and carries on; if it cannot, it stops in the lowest band that
fits.

| missing | can the program run? | proposed band |
|---|---|---|
| `.open()` on a declared window (D2) | yes — the window just never appears | **S0xx** |
| a window, in a program with none (D6) | yes | **S0xx** — the quietest of all |
| `.start()` on a thread, a program, a bash | yes — the work just never happens | **S0xx** |
| a blank file (D5) | no | S1xx |
| `satellite.include` / `satellite.main` / `satellite.return` | no | S1xx (S101–S103 today) |
| a capsule called and never written | no | S1xx (S110 today) |
| a `}` or `)` never closed | no | S1xx |
| a variable used and never declared | no | S2xx (S201 today) |
| an object whose spacesuit was never written | no | S2xx (S201 today) |

## 5. One report, proposed — each example below was RUN, and exits 0

**THE QUIET SHAPE, for S0xx** — the one-line notice satl already prints
(`[satellite] S010 CONFIG_FILE_MISSING: ...`), one line a missing element with its KIND's
own code (D4), and the corrected lines once under them:

    [satellite] S0?? MISSING_OBJECT: my_window -- the object of a line this program is missing
    [satellite] S0?? MISSING_CAPSULE: open() -- my_window is declared on line 5 and never opened
        the lines, corrected:

            satellite.variable.window my_window = satellite.window.new("gtkcar", 450, 250)
            my_window.open()
            my_window.close()

**A BLANK FILE (D5)** — the corrected lines are the whole program, the README's own:

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.console.display("Hello, World!")

        satellite.return(satellite)
    }

**A CAPSULE** — `greet()` called and never written; the corrected lines write it:

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

**A DOING NEVER DONE** — a thread (`t.start()`, `t.join()`) and a program (`p.start()`,
`p.join()`): the declaration line and the two that make it run.

**THE STOP SHAPE, for S1xx and S2xx** — today's report, unchanged, with the same "missing
<kind>:" lines and "the lines, corrected:" under its sentence.

## 6. The elements of a full program, in the order a person meets them

This is D7's list, and the order is the way D8 travels: each step is what the program
before it is missing.

| step | element | its kind | what is added |
|---|---|---|---|
| 0 | nothing — a blank file | — | the whole of Hello, World! |
| 1 | `satellite.include(satellite)` | the file's word | the first line |
| 2 | `satellite.capsule satellite.main()` | capsule | where the program begins |
| 3 | `satellite.return(satellite)` | the file's word | main's last line |
| 4 | `satellite.console.display("Hello, World!")` | capsule, on the console satl gives every program | a line out |
| 5 | `satellite.variable.string greeting = ...` | variable | a value kept |
| 6 | `satellite.capsule greet()` | capsule | a program's own capsule |
| 7 | `satellite.spacesuit rocket()` and `rocket my_rocket` | object, and its type | an object |
| 8 | `satellite.variable.window my_window = satellite.window.new(...)` | object | a window, declared (D1) |
| 9 | `my_window.open()` | object + capsule (D4) | the window on the screen |
| 10 | `satellite.window.label(...)` and `my_window.append(...)` | object + capsule | something in it |
| 11 | `satellite.console.new(...)`, `.open()`, `.display(...)` | object + capsule | a separate console (D7) |

**WHERE IT ENDS** — a full program with a window and a separate console. Run on a headless
compositor of its own (with two `.close()` lines added so the run ends): exit 0, both
drawn, no GTK warning. Written here as a person would keep it — the windows stay until a
person closes them:

    satellite.include(satellite)

    satellite.capsule satellite.main()
    {
        satellite.variable.window my_window = satellite.window.new("my program", 800, 600)
        my_window.append(satellite.window.label("Hello, World!"), 400, 300)
        my_window.open()

        satellite.variable.window my_console = satellite.console.new("my program's console", 800, 400)
        my_console.open()
        my_console.display("Hello, World!")

        satellite.return(satellite)
    }

## 7. Open — the author's, and nothing below is decided

- **Q1. Which band is each missing element in?** Section 4 is a proposal. The one D2 fixes
  is that `.open()` is LESSER.
- **Q2. Renumber, or keep the numbers?** S101–S103, S110 and S201 already say most of what
  section 3 shows. Either they keep their numbers and gain the "missing <kind>:" and
  "corrected" lines, or they move into a MISSING_ family with one code a kind. SATELLITE_ERROR.md
  already names the trade: *"Severity ordering and 003 compatibility cannot both be true"*,
  and check.sh asserts many of these exits by number. Recommendation: keep the numbers, add
  the lines, and give NEW codes only to what has none today (the S0xx rows of section 4).
- **Q3. "Missing a window by default" — said on EVERY run of a program with no window?**
  Hello, World! and every console script would print it each time. The severity policy wants
  a notice to be one quiet line; S020 goes to satellite.log only. Which: the console, the
  log, or both — and is it said once a program, or every run?
- **Q4. Where are `arguments.window = false` and `arguments.interpreter = true` written?**
  Neither row exists yet. A program writes a row today as `any_name.some_var = some_value`
  (SATELLITE_ARGUMENTS.md Part 4D); config.ini and the command line are the other two places
  a row comes from. Does `satl --repl` count as `arguments.interpreter = true`?
- **Q5. What is "a way to travel to a program that contains a window"?** The next missing
  line or two (section 6's next step), the whole next program, or a command that walks the
  steps — `satl --tutorial`, or a `satellite.help` topic?
- **Q6. A console INSIDE the window (message 4) or a SEPARATE console (message 8)?** A
  separate one is built and is section 6's last step. One inside a window is not: a console
  is a frame of its own (GTK_AND_NO_DEPENDENCIES.md GTK-17), and `.append` refuses a frame.
- **Q7. Before the run, or at its end?** The checker could say "made and never opened"
  before a line runs, but it cannot follow a window into a capsule that opens it, through
  `w2 = w`, or into an included file — and a wrong "you forgot" is worse than none. The end
  of the run knows exactly, as S742 does. Recommendation: the end of the run for "never
  done", the checker for everything that already lives there.
- **Q8. What does a declared, unopened window look like?** `.ok` is false — it is not on a
  screen. `satellite.console.display(my_window)` says `(window "t")` open and
  `(closed window "t")` closed; a third word for "not opened yet", or `(window "t")`?
- **Q9. What may a program do to a window before `.open()`?** Proposed: everything that
  builds it — `.append`, `.title`, `.resize`, `.colour`, `.background`, `.font`, `.menu`,
  `.key`, `.clicked`, `.every`, `.closed` — and refuse what needs it on a screen: `.focus()`,
  `.message`, `.ask`, `.choose_a_file`, and `.press()` on a button inside it.
- **Q10. D4's two missing pieces — two lines, each with its kind's code, as section 5
  shows; or one line that names both?** Section 5 is a reading of message 3, not a ruling.

## 8. The milestones — one for each element, in building order

Each says what it adds, what it detects, and how it is proved. None may begin before Q1
and Q2 are answered; MS-0 needs only D1.

- **MS-0 — `new` declares, `.open()` opens (D1).** Recommendation for HOW: `new` makes the
  GtkWindow and does not present it, so everything Q9 allows works on a real widget, and
  `.open()` presents it and waits for its first frame, as it does today. The desk holds a
  declared window — its handlers point into it — but the run waits only for OPENED ones,
  and destroys any never opened when the program ends. `.every`'s clock starts at `.open()`
  (`the_window_answers_again` already does this for a reopened window). **Every program
  that relies on `new` showing a window must gain `.open()`:** `examples/window.satl`,
  `tests/threads_window_on_a_thread.satl`, the programs inside `press-a-button.sh`,
  `prove-bare-machine.sh`, `prove-canvas-tabs-menus.sh` and `prove-console.sh`, the examples
  in the `satellite.window`, `satellite.variable.window`, `satellite.console` and
  `satellite.terminal` help pages, `experiments/window_test/windows_10.satl` — and the
  author's own `window_and_extras.satl`, which is his to change. Proved on a compositor of
  its own, WAYLAND_DEBUG counting buffers: a declared window draws nothing; `.open()` draws it.
- **MS-1 — the register.** One code a missing KIND (D4), in the bands Q1 picks: capsule,
  object, variable, the file's words, a close. SATELLITE_ERROR.md's table, `s_codes.hpp`,
  `machine_codes.hpp` for the exits of the stopping ones, and the quiet shape for S0xx
  (section 5). A check.sh row a code.
- **MS-2 — a blank file answers with Hello, World! (D5).** A file with nothing in it — or
  nothing but blank lines and comments — gets the whole program as its corrected lines.
- **MS-3 — the file's own words: include, main, return.** S101–S103 gain the missing lines.
- **MS-4 — a capsule missing.** Called and never written, a method a kind does not have,
  `satellite.main` itself.
- **MS-5 — a variable missing.** Used and never declared; the corrected line declares it,
  with the type the use suggests where there is one.
- **MS-6 — an object missing.** A spacesuit never written; an object used and never made.
- **MS-7 — a doing never done.** A window never opened (D2, and its two missing pieces,
  D4), a thread, a program or a bash never started — the three that today say nothing.
- **MS-8 — the window a program does not have (D6, D8),** and the two rows that turn it
  off, `arguments.window` and `arguments.interpreter` (Q3, Q4).
- **MS-9 — the separate console** as the last step of the path (D7, Q6).
- **MS-10 — the travel (D8) and, if he wants it, the tutorial (message 4, Q5):** each run
  of an unfinished program shows the step after it, until it is section 6's full program.

## 9. What this must not break

- **S0xx carries on, exit 0.** A "you forgot" that stops a program which would have run
  correctly is the more-fatal direction the severity policy rules out.
- **check.sh opens no window** — every window row stays headless; MS-0 and MS-7 are proved
  on a compositor of their own, as Part 2c was.
- **Every corrected example is run before it is written down** — every one in this plan
  was, and every line satl prints as "corrected" must be a line that runs.
- **A wrong "you forgot" is worse than none** — Q7.
- **003's numbers** — Q2.
