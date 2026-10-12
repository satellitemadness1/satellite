# satellite

Checked against satl BUILD 0058 on 2026-10-09.

**satellite 004 revision 10** — the third satellite, and the one being built from
here on. Its version, revision and build number live in
`satellite/config/satellite_config.hpp`; `make` raises the build number every time
it builds, and a new revision restarts the count at 1.

Revision 06 is the one that carries its own world: `make window` compiles GTK
and its whole stack *into* satl, so the binary needs nothing installed to open a
window. Every source it is built from is committed under `vendor/`, frozen at one
version — see [GTK_AND_NO_DEPENDENCIES.md](GTK_AND_NO_DEPENDENCIES.md). `satl
--license` lists 27 licences: satellite's own, and those of the 26 other projects
it carries. VTE, which draws the console, and the four libraries it needs (lz4,
simdutf, fmt and fast_float) are inside satl too, and are not in that list yet.
Ruled 2026-10-09: their licences are to be added to `licenses/` and to
`satl --license` (the author, answering the editors' questions: *"looks good"*).

Revision 07 is the one where a file is a namespace: every included file is reached
by its own name (`other.greet()`), `satellite.namespace name { }` (second spelling
`satellite.space`) makes a namespace inside one, and a program's own capsules are
its own -- an included file can no longer answer for them or replace its
`satellite.main`.

Revision 08 is the one where objects arrive and 003's list comes across:
`satellite.spacesuit` objects made by declaring them, with their sections, their
`satellite.constructor`, a spacesuit inside a spacesuit and one extending another;
`satellite.library.x = <value>` written once at a file's top and read by every
capsule, changed by nothing; and `satellite.container.list()` with `.sum`, `.max`,
`.min`, `.join(separator)` and `.reserve(n)` beside the lists of lists 004 already
had (`grid[x][y]`); and colour on the console -- 003's `display(text,
foreground=xFF8800, bold=satellite.bool.true, end="")`, `satellite.console.input`,
`.width`, `.height`, `.clear()` and `.home()`, and what 003 refused:
`"OK".foreground(x00FF00)`, `satellite.console.foreground(c)` for every line after,
and `satellite.terminal.foreground(c)` for the terminal's own colours.

Revision 09 is the one where a program reaches past itself: its own threads
(`satellite.thread.new(a_capsule(arguments))`, `.start()`, `.join()`) sharing
objects under `object.lock()`; other programs run as `satellite.variable.program`
and `satellite.variable.bash` — started, fed lines, waited for and stopped;
objects that point, with `.pointer()`, `.reference()` and `==` asking "the same
object?"; `satellite.random.fast()`, `.normal()` and `.ultra()` with
`satellite.random.seed(n)`; a string's own methods, a map's `.sort()`, containers
of any shape with `satellite.access` showing the way in, and
`satellite.statement.break` and `.continue`; and `my_window.open()`. satl itself
is laid out by PGO, ThinLTO and BOLT (each where its tool sits beside the
compiler), reads config.ini once a run, and reads the machine only on its first
start. The installed satl is the build every x86-64 processor runs; `make cpus`
builds one for each of 53 kinds of processor, and installs none of them.

Revision 10 is the one where satl stands on its own: every word of the language is
compiled into satl itself, with no libraries beside it; its console scrolls down
and sideways, each turned on or off by `arguments.scroll.vertical` and
`arguments.scroll.horizontal`; `satellite.legal` is the one master list of where
each name is legal, with `satellite` first — legal everywhere, never changed;
`satellite.delete(name)` deletes a name and everything satellite holds about it;
`satellite.window.text_box(text, wide)` says how wide a box is; and satl tests
itself from the inside with `satellite.test.full()`, `.speed()`, `.loss()` and
`.all()`. A bare `make` builds a new BUILD every time and then asks whether to
install it; `make install` installs without asking.

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")

    satellite.return(satellite)
}
```

The language is the same one: **a dotted path rooted at `satellite` names
something the language owns, and a bare identifier names something the user
owns.** The tie-breaker at every fork is still **do absolutely everything for the
user** — and never behind their back.

What changes in 004 is how it runs.

## What satellite 004 is being built to be

- **As fast as compiled C++.** The bar is within ×1.05 of the same work written in
  C++. Every word of the language — `satellite.console.display`,
  `satellite.variable.string.find(x)` — is a small piece of compiled C++ of *likely
  scenarios*, built into satl, so running a command is one call into code that is
  already in memory. Since 2026-10-07 no word is a library of its own; the thirty
  that were are files named by their numbers (`satellite-numbers/words/1.5.1.cpp`
  is `satellite.console.display`).
- **Built for many threads.** A pool of threads starts with the interpreter; some
  look for independent work — loops whose iterations do not touch each other,
  files, anything that waits — while the rest run it, and a program's output stays
  in order. Not built yet: the pool starts (the machine's cores × 2) and waits, and
  nothing hands it work (MILESTONES.md, M12 and M30). Ruled 2026-10-09: this stays an
  aim, marked not built yet (the author, answering the editors' questions:
  *"looks good"*).
- **No built-in limits.** Every maximum — threads, memory, digits — is a value you
  set, used as a ceiling on what the machine actually offers.
- **Strings of every language.** A satellite string holds any character, sixteen
  bits a character; one that needs more is stored wide, as three (the author,
  2026-09-16: *"32-bits only when we use the number 40000 as a 16-bit code"*).

## Where it is today

Revision 10 runs real programs. What runs:

- capsules with parameters and answers; `satellite.statement.if`, `else`, `while`
  and `for`; and a capsule whose last line calls itself runs in constant memory,
  as deep as it likes;
- numbers of any size, floats, fractions, percentages, binary, hexadecimal,
  colours and infinity; strings; lists, lists of lists and indexes (a
  fraction's + and - are built, its * / % ^ not yet);
- files and folders, and `satellite.variable.info` for what a folder holds;
- spacesuits (satellite's objects), namespaces, and every included file reached
  by its own name;
- threads and locks, windows, and colour on the console;
- since 2026-10-06: `satellite.delete(name)`; `satellite.legal` (the checker's own list, not a word a program writes), under which a
  name ends at its block's `}` while `arguments.access` is on (the default); `satellite.test.full()`, `.speed()`, `.loss()` and
  `.all()`, satl testing itself; `satellite.window.text_box(text, wide)`; and a
  console that scrolls -- each in PROGRESS.md, "1. What is built and checked";
- the prompt (`satl --repl`) and a console window of satl's own, which since
  2026-10-05 is where every run happens;
- every refusal as a report with its S-code, its line and a caret under it, and
  every report kept in `~/.satl/satellite.log`.

A statement may run over as many lines as it needs, and a string over as many as it spans.
The string methods are built -- `.satellite.size`, `.satellite.contains(x)`,
`.satellite.find(x)`, `.satellite.upper()` and the rest (MILESTONES.md, M16, 2026-09-25
and 2026-09-26).
See [PROGRESS.md](PROGRESS.md) and [MILESTONES.md](MILESTONES.md) for what is built and
checked, [NEW_MILESTONES.md](NEW_MILESTONES.md) for the author's plan now (NM-1 to NM-6),
[DESIGN.md](DESIGN.md) for the standards and the measurements behind them, and
[ERROR.md](ERROR.md) and the 85 bug pages in [SCRATCH.md/ERRORS4/](SCRATCH.md/ERRORS4/README.md)
(78 of them still there on build 0054, 2026-10-07) for the known errors.

```
make window                           # once: GTK and its stack, carried inside satl
make                                  # build/satl, a new BUILD number every time, then it
                                      # asks: Enter, y or a minute's silence installs; n does not
make install                          # build what changed, then install into ~/.satl, asking nothing
make test                             # check.sh, the installer's checks and the string checks
build/satl examples/hello_world.satl  # run a program, in a console window of satl's own
build/satl examples/hello_world.satl > out.txt   # into a file instead: no window opens
build/satl --help                     # every way to start satl
```

## What satl runs on

Linux, with a monitor. Since 2026-10-05 satl does not run on a machine without
one: every run -- a program, the prompt, a double-clicked `.satl`, `satl` typed
in a shell -- opens satl's own console window, and the only thing that keeps satl
where it was pointed is output sent to a file or a pipe (`satl x.satl > log`), or
`SATL_NO_WINDOW` set by a harness such as check.sh. With no screen a run is
refused with S730 NO_DISPLAY, exit 50. The author, that day: *"I am going to drop
support for machines that do not have a monitor ... we are dropping the whole
'start satl from a console and it runs' thing"*. Not decided: whether a GPU is
required -- *"we are not that far yet, we have only dropped support for machines
that do not have monitors"*. NVIDIA stays whatever is decided: *"I will never drop
support for nvidia GPUs"*.

**004's interpreter is `satl`** (`build/satellite-004` is a link to it). A plain
`make` builds it, with a new BUILD number every time, and then asks whether to
install it: Enter, `y` or a minute's silence installs, `n` leaves it in `build/`
(with no terminal to ask, it installs). `make install` installs without asking.
The install puts satl -- every word inside it -- and the help files into `~/.satl`,
and adds **Satellite 004** to the applications menu. One link puts the word `satl`
on your command line: `ln -sf ~/.satl/satl ~/.local/bin/satl`. So `satl` typed in
a shell is the last install, and `build/satl` is the last make. The whole install,
step by step, is on the INSTALL page at [satellite.foundation](https://satellite.foundation).

## The earlier satellites

Nothing earlier is thrown away. It is finished, and kept exactly as it was:

- **[`old_versions/second_satellite/`](old_versions/second_satellite/)** —
  **satellite 003 revision 07**, the second satellite: a complete interpreter
  (`satl`), its prompt and `satl-term`, and its installers. Also the tag
  `satellite-003-revision-07` and the branch `archive/satellite-003-revision-07`,
  in the author's own copy of the git history; the public repository on GitHub (made
  2026-10-08) has neither. Ruled 2026-10-09: this sentence about the tag and the
  branch stays, worded as it is (the author, answering the editors' questions:
  *"looks good"*).
- **[`old_versions/first_satellite/`](old_versions/first_satellite/)** — the first
  satellite (002).
- **[`old_versions/prototype/`](old_versions/prototype/)** — prototypes (folders
  `M3` to `M11`). Its readme, the author's words: *"THESE PROTOTYPES ARE NOT FOR
  PULLING CODE FROM, EVERYTHING IN THEM EITHER HAS BETTER CODE IN 003, OR THE SAME CODE AS
  003"*. Ruled 2026-10-09: the prototypes stay in this list (the author, answering
  the editors' questions: *"looks good"*).

## License

MIT — see [LICENSE](LICENSE).
