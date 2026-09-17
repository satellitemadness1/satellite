# satellite-004 — PLAN.md

Started 2026-09-14; reordered the same night around the author's three files:
`.satc`, `.satb` and `.sati`. DESIGN.md holds the standards; this file is the
order of work. One milestone at a time. **For each milestone the author writes
the pseudocode, Claude writes the C++**, and the milestone lands when its
entries are cleared.

**The order from here (author, 2026-09-15):** M0.5 (the build, the installer and
satl-term) → M0.6 and M0.7 (the prompt) → M0 onwards. The prompt comes before
`satellite_number`, which PROGRESS.md had as next.

---

**REWORKED 2026-09-17, because the plan stopped describing the program.** The
author: *"our plan doesn't match our 16-bit bytecode reality ... we are not really
building a session loop... we are just building fast paths for various token
combinations and calling those fast paths"*, and then: *"some milestones must be
impossible now, and need to be reworked"*.

What changed under this file, in his own rulings: **there is no `.satc`, no
`.satb` and no `.sati`** (D0.1, 2026-09-16: *"we threw away satc and satb in
favor of all 16-bit"*), and **there is no parser** -- a `.satl` is lexed straight
into 16-bit codes, checked, and walked. So four milestones here had no subject
left, and two described work that was already built by another route.

**Every milestone below now says which it is:** BUILT, DEAD, or still owed. A
milestone struck through is one whose subject no longer exists -- its live parts
are named where they went, and nothing is deleted silently. Work that lost its
milestone entirely is listed at the end of this file, under **WITHOUT A
MILESTONE**, for the author to place.

**MILESTONES.md carries everything that is not in this file** -- M13 upward, and
the debt found while building. It was audited the same day (`bc14937`).

## M0.5 — port the build, the installer and satl-term — **BUILT 2026-09-17**

**Built in `71f7836`, `5fc0c7d`, `a4ac456`, `6ea6f01`**; PROGRESS.md's row says
what it is and how it is checked, and MILESTONES M0.5 holds what is left to the
author (D0.5.1, where 004 installs). The text below is the record of what was
decided, kept as written.

(author, 2026-09-15) On 2026-09-15 satellite 004 moved to the top of the
repository, and satellite 003 revision 07 went, unchanged, into
`old_versions/second_satellite/`. 003's build and install machinery stays there,
and 004 needs its own. The same day: **004's interpreter is named `satl`**;
**satl-term is ported with make_support, nearly as-is, and reads 004's
`version.hpp`**; and the command line is Claude's to decide ("`satl --run <file>
[args...]` and `satl --repl` look good"). **Neither port needs the author's
pseudocode:** it is plumbing, not a speed path.

**make_support**
- **003's Makefile is an index over numbered fragments.** Port the ones 004
  needs, one by one, so 004's `Makefile` becomes the same kind of index: 005
  jobs, 010 compiler, 020 version, 030 directories, 040 sources, 047 window
  (satl-term, built only when pkg-config finds `vte-2.91-gtk4`), 050 build, 060
  compile, 065 tests, 070 clean.
- **048 comes without `STATIC`.** 004 is dynamic on purpose: satl and every
  library must share one libstdc++ (DESIGN §3.4), and GTK4/VTE ship no `.a`
  anyway. Only `LINK_ENV = env -u LD_RUN_PATH` (on the `.so` links too) and the
  flags stamp come across.
- **Not ported:** 045 (the haswell pair), 067 (003's start-up rows are 003
  commands), and 080 (a bare `make` installs nothing while D0.5.1 is open).
- **`build_libraries.py` takes `CXX` and the flags from make,** and rebuilds
  every library when either changes. Today it reads `CXX` from the environment,
  hardcodes `-O2` and rebuilds on mtime only, so satl and its libraries can come
  from two different compilers without anything saying so.
- **Version, revision and build** (author, 2026-09-15; built the same day in
  the old Makefile): all three are rows of `return_arguments_vector()` in
  `satellite/config/satellite_config.hpp`, and every make that builds something
  raises the `arguments.build` row (`satellite/config/build_number.py`). The
  ported fragments keep that rule. **The three numbers show in four places** —
  `satl --version`, `satl --help`, every start of satl (unless
  `arguments.startup_display` is false), **satl-term's start-up, and the
  installer** — always as
  `THE SATELLITE PROGRAMMING LANGUAGE` / `VERSION 004 REVISION 04 BUILD 0051`.
  The last two land here, with satl-term and the installer.
- **`make test`** runs `check.sh` and the two string checks. The string checks
  run 003's in-tree `old_versions/second_satellite/satl`, so that binary stays
  built — but never with a bare `make` in 003's tree, which re-installs 003 into
  `~/.satl`.
- **satellite_debian is not affected.** It lives in
  `old_versions/second_satellite/satellite_debian/`, and its
  `make_support/010-tree.mk` sets `ROOT = ..`, so it builds from 003's
  fragments, which do not move. (This entry's first version said the port had to
  keep it working.) satellite_debian and distribute stay in second_satellite
  until 004 has something to package.

**The name `satl`**
- **`build/satellite-004` becomes `build/satl`,** beside `build/satellite-numbers/`
  and `build/satl-term`. satl loads the libraries beside its own path, and
  satl-term runs the `satl` beside its own, so **the three are built into one
  folder and installed as one folder.**
- **Renamed** in the Makefile, `check.sh` (which takes `SATL=`, default
  `build/satl`, so an installed copy can be checked), `race.sh`, the usage note,
  README.md and PROGRESS.md. `words/make_words.py`'s `satl` is 003's and stays.
- **Never a file named `satl` at the repository top.** `~/.bashrc` puts the top
  on PATH ahead of `~/.local/bin`, so every script's `satl` would quietly become
  004.

**The command line** (Claude's, delegated by the author)

```
satl                                   the opening lines: the version and the ways to start; exit 0
satl --version | -V                    the title lines (version, revision, build); exit 0
satl --help | -h                       the title lines and usage on stdout; exit 0
satl [--debug] --run <file> [words…]   run <file>
satl [--debug] <file> [words…]         the same, when <file> does not begin with '-'
satl [--debug] --repl                  the prompt (M0.6); until then answers 14 not_built_yet
```

- **`--debug` is the only option, and it comes before the command word.**
  `--version` and `--help` must be the whole command line.
- **After the file, every word is the program's,** `--version`, `--debug` and
  `""` included, kept in order as `arguments.program`, `arguments.argument_1` …
  `arguments.argument_N` and `arguments.length` (003's names). Today `--version`
  is matched anywhere in argv, and every word after the file is dropped.
- **`--run` takes the next word literally** (`satl --run -x.satl`), so there is
  no `--`.
- **Anything else is refused by name** on stderr — an unknown `-` word, `--run`
  with no file, words after `--repl`, `--version` or `--help` — with the new
  machine code **`command_line_not_understood`** (the next free code in
  `machine_codes.hpp`), added to `machine_codes.hpp`
  first. satl-term uses it too, in place of 003's `EXIT_USAGE` 2, which is
  `display_error` in 004.
- **`arguments.session.directory`** is the directory satl started in, recorded
  once. M0.6's `change` never moves it.
- **Bare `satl` gives 003's answer,** the opening lines and 0: "nothing to do is
  not an error" (003 `main.cpp`). `check.sh`'s "no file named" check changes
  from 8 to 0.
- **An exit status is never cut to 8 bits** (ERROR #7, brought forward from M1).
  A code outside 1–255 exits as one reserved status (D0.5.2), with the full code
  on stderr, because satl-term closes a tab on 0 and 256 exits 0.

**satl-term**
- **`old_versions/second_satellite/src/programs/satl-term/` → `satl-term/` at the
  top:** its six sources, five headers and TERM.md — not the `.o` files beside
  them — with a 004 note at the top of each, and include paths without 003's
  `-Isrc`.
- **What changes:**
  - it runs the `satl` beside it with `--repl`, or `--run <file> [words…]`, as
    before — now understood;
  - `version.hpp` replaces `system_facts/version.hpp`;
  - `opening.hpp`'s `EXIT_FINE` and `EXIT_USAGE` become `success` and
    `command_line_not_understood`;
  - a held tab names the code: "satl stopped on machine code 14 (not_built_yet)".
- **Not ported: the window handover** (003's `window_handover.cpp`). The launcher
  starts satl-term directly; the handover loses the machine code, and turns every
  `>/dev/null` check into a false pass.
- **The app id stays `org.satellite.terminal`, and no 004 launcher is installed**
  (D0.5.1). A 004 `.desktop` under that id would take 003's launcher, icon and
  `.satl` association.

**The installer** (`satellite_enterprise/`), until the author decides where 004
installs (recommended):
- installs `satl`, `satellite-numbers/` and `satl-term` together, **only into a
  `--root` it is given**, and refuses `~/.satl`, `/usr/local`, and any root that
  holds a `satl` this installer did not put there (known from its own record,
  never by running the file);
- **refuses `--link`, `--desktop` and `--system`**, because 003's installer lets
  an explicit `--link` win over `--root`;
- writes `satellite-numbers/` to a fresh folder and renames it into place, so no
  stale `.so` survives;
- proves the install by running the installed `satl` on
  `examples/hello_world.satl`, never with `--version`, which answers before any
  library loads.

**Done when**
- `make` in the top folder builds `build/satl`, every library and
  `build/satl-term` through the ported fragments, with no warnings; `make` again
  builds nothing; a different `CXX` rebuilds every library.
- `make test` runs check.sh and the string checks.
- `install.sh --root <scratch>` installs the set, and
  `SATL=<scratch>/satl ./check.sh` passes.
- satl-term opens a tab that runs `examples/hello_world.satl` and closes on 0,
  and a prompt tab that holds on 14.
- **003's install is untouched:** the sha256 of its installed files
  (`~/.satl/satl`, `satl-cpu-level`, `satl-term`, `~/.satl/share/`,
  `~/.local/bin/satl*`, and 003's launcher, mime type and icons) is the same
  before and after. Not all of `~/.satl`: 003 writes `cache/` and
  `satellite.log` there while it runs.
- The entries write no `[entry]`.

**Entries:**
- **Command lines:** `satl`; `satl --run`; `--run ""`; `--run -x.satl`;
  `satl --`; `satl -x.satl`; `--rum x.satl`; `—run` with an em dash;
  `--repl extra`; `--version extra`; `satl --debug --help`;
  `satl prog.satl --version --debug --repl ""` (all four words are the
  program's); 100,000 program words; a 1 MB word.
- **Exit statuses** at 0, 1, 255, 256, −1 and 4294967298; `--version` into
  `/dev/full`; `| head -1`.
- **DESIGN §9, as the file and as program words:** `!@#$%^&*()`,
  `poly.#@($*&$&$`, empty, one character, a Unicode look-alike, invalid UTF-8,
  `ESC]2;title BEL` (shown escaped under `--debug`, never raw), and objdump bytes
  of a compiled call; a directory as the file, an unreadable file, a symlink, a
  path longer than 4,096 bytes.
- **Libraries:** a copy of `satl` alone (5); `satl` through a symlink (runs);
  `make STATIC=full` still dynamic (`ldd` shows `libstdc++.so.6` for satl and
  every `.so`); `readelf -d` shows no RPATH on anything built.
- **satl-term:** no `satl` beside it; a 003 `satl` beside it; `--nice -21`, `20`,
  `abc`; `--size 0x0`, `99999999999x1`, `x600`; `--title` with nothing after it;
  run from a path deeper than 4,096 bytes; a child exiting 0, 14 or 20, or killed
  by a signal.
- **Install:** no `--root`; `--root ~/.satl`; `--root /usr/local`; a root
  holding 003's satl; `--root X --link`; a root holding a space or a newline; a
  read-only root; a stale `9.9.9.so` already in the root.
- **`command -v satl`** in a non-interactive shell still answers 003's, after
  `make` and after an install.

**Decisions:**
- **D0.5.1** Where 004 installs, and whether its window keeps
  `org.satellite.terminal`. Until then, an explicit `--root` only, as above.
- **D0.5.2** The status a code outside 1–255 exits with. Recommended: 255, never
  given to any code.

## M0.6 — the prompt in satl, with `satellite.directory.change` and `.list` — **BUILT 2026-09-17**

**Built in `28364f2` (the terminal layer) and `29319d8` (the session, the three
directory words and the table).** `satl --repl` reads a line with its own editor,
tokenises it with the lexer a `.satl` gets, judges it with the checker a capsule's
body gets, and walks it with `run_statements` -- no wrapper, no second runner.

**What M0.6 still owes**, all named in PROGRESS: `list()` inside a PROGRAM answers
`not_built_yet` until satellite has a list type (MILESTONES M14); the listing's
order is the bytes' where 003's was its character table's (a library cannot reach
the language's string yet -- MILESTONES M21); and the 100,000-entry race below has
not been run.

**One thing this milestone changed that was not in its plan:** a word whose row
spells its arguments (`satellite.directory.list()` and `list(d)` are two rows over
one path) never lexed at all, because no dotted path matches either. The lexer now
tries the whole path first, then the path plus the shape of the brackets that
follow. 332 shaped rows were waiting on it.

The text below is the record of what was decided, kept as written.

(author, 2026-09-15) "Build the prompt as the next milestone. Build it partly in
satl-term as much as you can and put the rest of the prompt in satl, or design
the milestones so that it's built that way … with its
satellite.directory.change("path") and satellite.directory.list(), and we will
build these functions as satellite-numbers; this part will be the part that is
built into the satl interpreter."

**Built in two milestones (recommended).** M0.6 builds the whole prompt inside
satl — the session, the directory libraries, the table and a line editor — so
`satl --repl` works in a satl-term tab (through its pty), over ssh, and from a
pipe. M0.7 then moves into satl-term everything a window can own.
- **Why this order:** in a terminal window, the line editor normally belongs to
  the program on the other side of the pty. Moving it into the window needs a
  second channel, and whether the window can then draw its prompt strictly after
  a line's output is not yet measured (M0.7 step 0).
- Building satl's half first means the prompt never waits on that measurement,
  and a console user never gets less than 003's prompt.

**What a prompt can run before M6.** 004 has no variables, expressions, blocks or
includes yet. So this prompt runs **one statement a line**:
`satellite.console.display(literal)`, `satellite.directory.change(d)`,
`satellite.directory.list()` and `satellite.directory.list(d)`. Everything else
is refused with its reason, and the session goes on.
- **Between lines it keeps** the number index (loaded once, because every `Call`
  points into it), the working directory, and the history.
- **Nothing typed is kept as text or run again.** Re-running kept text is what
  caused 003's ERROR #26.

**satl — the session**
- **One statement reader for `--run` and the prompt.** `compile_satl`'s loop body
  becomes `compile_statement`, which finds a written call by its path and its
  argument count, in a table built once when the index loads: `list()` is
  `1 18 4`, `list("/tmp")` is `1 18 5`.
- **Refused by name at the prompt, never skipped** (ERROR #1's shape): `{`, `}`,
  `satellite.include(satellite)`, `satellite.capsule satellite.main(`,
  `satellite.return(satellite)`, and `help` (1 19 is not built).
- **`exit`, `quit`, or Ctrl-D on an empty line end the session** with 0.
- **Ctrl-C:**
  - while typing, the line is abandoned;
  - while a line runs, SIGINT sets a flag that the directory scenario checks
    between entries, and the line answers `interrupted`;
  - a second press restores the terminal and exits `interrupted` — added as 130,
    128 + SIGINT, which shells and 003 already read as Ctrl-C.
- **SIGHUP** (a closed tab) restores the terminal and ends the session.
- **Brought forward from M1, because a prompt meets them first:**
  - SIGPIPE is ignored (#3);
  - **no library is ever loaded from the current directory** (#8) — once
    `change` exists, the current directory can be anyone's;
  - whitespace inside the brackets is accepted (#14);
  - an unknown escape is refused (#16), so `"dir\qx"` never becomes `dirqx`.
- **`std::cout` is cleared after a refused write** and flushed before every prompt
  is drawn, or one `display_error` fails every later line and output lands after
  the prompt.

**satl — the line editor**
- **003's terminal layer comes across into `prompt/`:** `raw_mode`, `keys`,
  `editor`, `history`, `render` and `line_reader`, 1,232 lines with no dependency
  on 003's language. **The editor and the history keep no terminal type**, so
  M0.7's window can drive the same code.
- **Changes on the way:**
  - no 16-byte escape buffer and no 1,000-line history cap (DESIGN §1);
  - the width is asked on SIGWINCH, and a character's columns come from
    `wcwidth`;
  - every write has a length, so a NUL never cuts a draw short (ERROR #25's
    shape);
  - bracketed paste is on while a line is read, so a pasted block arrives whole.
- **When stdin or stdout is not a terminal,** lines are read with `read(2)` into
  the session's own buffer, never `std::cin`, and no banner or prompt is printed
  (D0.6.2).

**satellite-numbers — the directory words**
- **Folders spelled exactly as the words:**
  `satellite.directory.change(d)/` → `1.18.1.so`,
  `satellite.directory.list()/` → `1.18.4.so`, and
  `satellite.directory.list(d)/` → `1.18.5.so`, which is list's own function
  given a path.
- **One new scenario, appended last in `Scenarios`,** so every `.so` built before
  keeps its layout. It takes its arguments as bytes (`std::string`: a POSIX name
  is bytes, and 004's strings refuse anything that is not UTF-8) and a pointer
  to the Ctrl-C flag, and answers a flag, a text, or names. Its header is a file
  in `satellite-numbers/`, and joins build_libraries.py's shared headers.
- **The behaviour is 003's** (`satellite_directory/handlers.cpp`, `listing.cpp`),
  checked against 003's real satl the way the string libraries were:
  - `change(d)` answers true or false and is never an error — "a VALUE and not an
    error";
  - `list()` reads `"."` through one open descriptor (`fdopendir`, `fstatat`), so
    a directory deeper than PATH_MAX still lists; `.` and `..` are dropped,
    dotfiles kept, in 003's sort order; a read that fails answers its code, never
    half a list;
  - a path holding a NUL is refused, because `c_str()` would act on the part
    before it.
- **Machine codes added first:** `directory_not_found`, `not_a_directory`,
  `directory_unreadable` (with the reason), `path_holds_a_nul`, `interrupted`.
- **`change` is never part of a parallel batch** (a note for M2): it moves every
  thread's relative paths at once.

**satl — the table**
- **`list()` or `list(d)` typed alone draws 003's table** (name, type,
  permissions, owner, created, modified); in a program it answers the names. It
  is decided on the compiled line — one statement whose row is `1 18 4` or
  `1 18 5` — never on the text, and never through 003's one-shot global request.
- **Nothing reaches the terminal raw.** Every name, path and the prompt text show
  control bytes, ESC and invalid UTF-8 as `\xNN` (D0.6.3), so a file named
  `ESC]2;…` cannot retitle the window. Columns are counted in cells.
- **`change(d)` typed alone prints nothing,** as in 003.

**ERROR #26, carried to M6:** a kept include keeps the canonical path it first
resolved to, and is never resolved again from its text.

**Pseudocode:** by PLAN's rule the author writes it for the session loop and the
directory libraries. The editor is a port of 003's and, like M0.5, needs none
(D0.6.4).

**Race:**
- `satellite.directory.list(d)` of a 100,000-entry directory in a `--run` file,
  against C++ `readdir` with the same sort: within ×1.05.
- `make race` after the shared statement reader: no slower than today's ×1.065.
  The ×1.002 `write`/`put` fix is M1's.

**Done when** `satl --repl` runs `display`, `change` and `list` lines at a real
terminal (`pty.fork()`), in a satl-term tab, and from a pipe; the entries write
no `[entry]`; the build has no warnings; both races pass; and 003's install
hashes the same.

**Decisions:**
- ~~**D0.6.1** Does a typed line go through `.satc` / `.satb` / `.sati`?~~
  **DEAD**: there are no such files. A typed line becomes registry codes, exactly
  as a file does.
- ~~**D0.6.2**~~ **TAKEN, both halves:** from a pipe there is no banner and no
  prompt text; the status is the FIRST failing line's code. At a terminal `exit`
  is 0, because the person has already seen every refusal.
- **D0.6.3** Names that are not UTF-8, or hold control bytes, shown as `\xNN` and
  `\\`, so the shown form is exact. How such a name is typed back is decided when
  M6 puts names into strings.
- ~~**D0.6.4** Which parts of M0.6 the author writes pseudocode for.~~
  **ANSWERED 2026-09-17:** *"you write the session and directory code"*. None.
- **D0.6.5** Keys typed while a line runs. 003 throws them away on purpose
  (`TCSAFLUSH`, `raw_mode.cpp:85-87`); a shell keeps them. Recommended: 003's.
- ~~**D0.6.6**~~ **TAKEN: not built.** `current()` `1 18 2` and `exists(d)`
  `1 18 3` are numbered and answer `not_built_yet`; the author named `change` and
  `list`, and a word built for symmetry is a word designed by symmetry.
- ~~**D0.6.7**~~ **TAKEN: no.** The `type` column is what the system knows --
  dir, file, link, fifo, sock, dev -- and never reads a file to guess. Reversing
  it is one function in `satl/listing.cpp`.

**Entries:**
- **Typed (DESIGN §9):** `!@#$%^&*()`; `poly.#@($*&$&$`; an empty line; one
  character; a 10 MB line; `sаtellite.directory.list()` with a Cyrillic а; a NUL
  alone and inside a path literal; a byte-order mark; CR-only line endings;
  `satl --repl < build/satl`; objdump bytes of a compiled call as a line, as a
  path, and as the name of a listed directory (data only, shown escaped);
  `display( 42 )`; `display("a\qb")`; `change("x" // note`; `list(,)`;
  `list("a", "b")`; `list(list())`; `change(42)`; the five refused lines; `help`.
- **Keys (`pty.fork()`, asserted on the screen):**
  - Ctrl-C on half a line, at an empty prompt, 100 times a second, and during
    `list` of 1,000,000 entries and then again (afterwards `stty -a` shows
    `icanon echo isig`, and bracketed paste is off);
  - Ctrl-C between Enter and the run;
  - Ctrl-D on an empty and on a non-empty line; stdin closed mid-line; Ctrl-Z
    during a run; SIGHUP at the prompt and during a run;
  - a paste of 3 lines, of 100,000 lines, and one holding `ESC[201~`;
  - a resize while typing; a line exactly the screen's width, and one wider; a
    prompt text wider than the screen;
  - truncated UTF-8, a bare ESC, a 100,000-byte escape sequence, CJK and
    combining marks.
- **Pipes:** `satl --repl < file`, `< /dev/null`, `| tee log` (and Ctrl-C there),
  `> /dev/full`, `| head -1`; stdout to a file with no controlling terminal (no
  window opens).
- **Directories:**
  - `change` to a missing path, a file, a directory without search permission, a
    link to a directory, a dangling link, a link loop, `""`, `"."` and `".."` at
    `/`, a trailing slash, `"~"` (no home expansion), and a path longer than
    PATH_MAX;
  - `list` of an empty directory, one without read permission, 1,000,000
    entries, a working directory deleted from another shell, and one 5,000 bytes
    deep (at start-up and after `change`);
  - names holding a newline, a tab, `ESC]7;file:///etc BEL`, `ESC[2J`, invalid
    UTF-8, CJK, combining marks, a leading dash, a backslash;
  - a fifo, a socket, a device and a dangling link in the listing; a file swapped
    for a fifo between `readdir` and the type check.
- **What a `change` must not move (#8, #26):** change into a folder holding a
  hostile `satellite-numbers/` and a different `hello_world.satl`, then run
  `display` and `list()`. No library is mapped from that folder
  (`/proc/<pid>/maps`), and `satellite.log` is still written where it was.

## M0.7 — the prompt in satl-term *(THE NEXT MILESTONE: M0.6 is built)*

(author) "as much as you can" in satl-term. M0.6's prompt already runs in a
satl-term tab, through the pty. This milestone moves into the window everything
the window can own, and measures where that stops.

**Step 0 — measure the one thing this rests on.** With the editor in the window,
satl's output travels through the pty while the window draws with
`vte_terminal_feed`: two queues. The next prompt must appear after the last byte
the line wrote. Before anything else is built, an entry that writes 10 MB shows
whether VTE 0.78.6 lets the window draw strictly after that output (`FIONREAD`
on the pty, or a marker the terminal parses in stream order).
- **If it cannot be made exact, the editor stays in satl** (M0.6's), and M0.7
  builds only the window's share below.
- Either way, the result is written into DESIGN.

**The channel**
- **satl-term makes a socketpair and starts `satl --channel <fd> --repl`** with
  `vte_terminal_spawn_with_fds_async`. `--channel` joins M0.5's grammar, is
  allowed only with `--repl`, and satl sets `FD_CLOEXEC` on it at once.
- **Out of band,** so a program's output or a listed file name can never forge a
  message, and nothing lands in Save output. A message is
  `<kind> <length>\n<bytes>`, with no maximum length.
- **Messages:** satl → window: `hello`, `prompt` (the last code and the
  directory), `running`, `bye`. Window → satl: `entry`, `end`.
- **A bad message ends the channel, not the session:** the tab falls back to
  M0.6's editor in satl.

**satl-term — the editor** (if step 0 allows)
- **A key belongs to the window only between satl's `prompt` and the window's
  `entry`.** From `entry` to the next `prompt` every key goes to the pty, Ctrl-C
  included, so a running line can always be stopped. Keys typed in between are
  held, and handed to whichever side the answer names. This replaces 003's
  `child_alive` rule (`keys.cpp:3-19`).
- **The same editor and history as M0.6** (its terminal-free core), driven by GTK
  key events: Enter sends, Shift-Enter adds a line, the rest of 003's keys, and
  input-method text through `gtk_event_controller_key_set_im_context`.
- **Whole-entry history,** kept by the window, so it survives a restarted satl;
  no cap; no file until D0.5.1.
- **Paste** (Ctrl-V, middle-click) goes into the entry, and a pasted newline is a
  new line of it, never Enter.
- **Drawing** uses VTE's column count and ambiguous-width setting; names and the
  directory go through M0.6's `\xNN` rule.

**satl-term — the window's share** (built either way)
- **The tab label and title, New tab, and the Open and Save output choosers
  follow the directory satl reports:** through the channel, or without it
  through OSC 7, read with
  `vte_terminal_ref_termprop_uri(VTE_TERMPROP_CURRENT_DIRECTORY_URI)` (the older
  getter is deprecated and would warn). A deleted directory falls back to
  satl-term's own.
- **A held tab names the machine code** and its name.
- **`SATL_TERM=1`,** which nothing reads, is dropped, or given this job.

**Done when,** in a real satl-term window (headless mutter + XTest, asserted on
the rendered screen), a session: types `change("..")` and sees the prompt's
directory and the tab change; types `list()` and sees the table; brings back a
whole entry with Up; pastes a three-line block whole; abandons a half-typed entry
with Ctrl-C, and stops a long `list()` with it; and leaves with Ctrl-D. The same
session through `satl --repl` on `pty.fork()` still passes M0.6's entries. No
warnings; no `[entry]`.

**Decisions:**
- **D0.7.1** A prompt that dies (a crash, two Ctrl-Cs): restart it in the last
  directory, keeping the history, but only if it had said `hello` (recommended);
  or hold the tab, as 003 does.
- **D0.7.2** In a prompt tab, may Ctrl-C copy when text is selected? Today it
  always goes to the live child.

**Entries:**
- **Step 0:** an entry writing 10 MB; one writing a partial line with no newline;
  output still arriving when `prompt` is sent.
- **The channel:** satl killed with SIGKILL at the prompt and mid-entry; a length
  of a million nines, a negative one, one that is not digits; an unknown kind;
  half a message; `prompt` before `hello`; `--channel` naming a closed
  descriptor, 0, 1, 2, a regular file, a pipe, or a number too large for an
  `int`; a program line displaying a forged message or `ESC ]` sequences (the
  window's state does not change).
- **Keys:** Ctrl-C in the instant between Enter and `running`; keys typed during a
  run and right after Enter; Up with no history; an entry exactly as wide as the
  window, and one wider; a resize while it is drawn; double-width, combining and
  ambiguous-width characters; a 10 MB paste; a paste holding `ESC [ 2 0 1 ~`, NUL
  or CR-only endings; input-method text; a file dropped on the prompt.
- **The window's share:** New tab after `change`; a second tab's `change` moves
  nothing in the first; a directory deleted before New tab; an OSC 7 printed by a
  listed file name (ignored while the channel is up); `SATL_TERM=1 satl --repl`
  in a non-VTE terminal (no stray bytes).

---

## M0.8 — the prompt's second half: blocks, kept variables, `run <file>` *(NEW)*

**Added 2026-09-17 in the rework**, because old M6 owned this and M6 is dead. M0.6
runs ONE statement a line and refuses a block by name; these three are what turns
that into a prompt a person can work in, and each is a shape the walker already
has inside a capsule:

- **A block that is typed over several lines.** `satellite.statement.while(x)`
  and a `{` at the prompt must keep reading until the braces balance, which is
  what the line reader's prompt-per-line already allows for (003 changed the
  prompt text under the cursor as the depth changed; M0.6 dropped that and left
  the hook).
- **Variables that outlive their line.** A typed line is walked with a fresh
  `VariableTable` today, so `satellite.variable.number n = 1` is forgotten
  immediately. The session keeps one table for as long as it lives -- and that is
  the first place "there are no globals" has to be answered for a PROMPT, where
  there is no capsule to be inside.
- **`run <file>`**, so a session can run a program without leaving.

**Done when** a `while` block typed at the prompt runs, a variable declared on one
line is still there on the next, `run` runs a file and reports its code, and none
of the three changes what a `.satl` file does.

**Entries:** a block left unclosed at the end of input; `}` with nothing open; a
variable declared twice; a name the file's own `main` also uses; `run` of a
missing file, a directory, and a file that fails half way.

---

## How a milestone is done

(author) "Instead of testing our programs, we will define entries with each of
the milestones." Each milestone lists its **entries**: every input that could
break it, including DESIGN §9's hostile names, bytes and machine code. A run of
the list writes an `[entry]` to `satellite.log` for anything handled wrongly.
**A milestone is done when that run writes no entries**, the build has no
warnings, and its speed race (where it has one) is within **×1.05 of compiled
C++**.

**Until M5 builds `write_entry` into the interpreter** (recommended, 2026-09-15),
each milestone's entries are a script (`entries/M0.5.sh`, …). It runs the real
binaries by absolute path, after checking that `--version` says satellite 004,
and for every input handled wrongly writes an `[entry]` block in DESIGN §7's
format to `satellite.log` at the repository top. `~/.satl/satellite.log` is
003's. Prompt entries drive a real terminal with `pty.fork()` and assert on the
rendered screen; satl-term entries use headless mutter and XTest (TERM.md).

---

## The shape of a run

**THE AUTHOR'S FIRST SHAPE, 2026-09-14, kept because it is where the numbering
came from** -- and superseded by his own ruling two days later:

```
source .satl ──24 threads──▶ .satc ──combine──▶ .satb ──▶ .sati
   numbers for every word    batch marks: wait here,   strings as bits,
                             run these in parallel     32 bits per character
```

**THE SHAPE THAT RUNS, since 2026-09-16** (the author: *"we threw away satc and
satb in favor of all 16-bit"*, and *"first start 256 threads, then load the tiny
C++ libraries, then convert the .satl to 16-bit"*):

```
source .satl ──256 warm threads, batches of lines──▶ bytecode registry
                                                    one row a FILE, 16 bits a code
                                                            │
                     every statement judged ◀───────────────┤  program_check.cpp
                                                            │
                     walked where it stands ◀───────────────┘  program_walk.cpp
                          six shapes, a call is a POSITION, nothing allocated
```

- **There is no intermediate file to build or to be stale.** The registry IS the
  numbered program: a word is one code, a string is codes inline and counted, and
  combine's marks have rows reserved in it (`batch_start`, `batch_end`, `wait`,
  `batch_size`).
- **`.sate` is the one file**, and it is the registry written out. It is written
  today and not yet read back (MILESTONES M31).
- **The threads are warm before anything loads**, and they tokenise in batches of
  lines -- 256 batches of a 100,000-line program in 1.5 ms, 27x one thread.
- **A typed line takes the same three steps** (M0.6), which is what proves there
  is one runner and not two.

---

## What exists now (2026-09-17)

PROGRESS.md is the full list, row by row, with how each is checked. In short:

- **The program as 16-bit codes:** `satellite/bytecode/` -- the registry (one row
  a file, tokenised on the warm threads), the function table (a word's code IS
  the index into it), the checker, the walker, the expression fast paths, and
  `.sate`.
- **The types:** `satellite_number` (checked against Python), the author's 16-bit
  `satellite_string` with a wide character as 40000 and two codes,
  `satellite_binary_number`, `satellite_percentage`, and the object model
  (`satelliteObject`, `satelliteSpacesuit`) with reference semantics.
- **27 numbered libraries**, built by `satellite-numbers/build_libraries.py` from
  `words/words.tsv` -- `satellite.console.display`, the 23 string methods, and
  `satellite.directory`'s three.
- **satl and satl-term:** the build is an index over `make_support/` fragments,
  `build/satl` and `build/satl-term`, and `satellite_enterprise/install.sh`.
- **The prompt:** the terminal layer (`satellite/prompt/`) and the session
  (`satellite/satl/session.cpp`), which runs a typed line out of the bytecode.
- **The checks:** `make test` -- check.sh (183), the installer's 14, the string
  checks, and the harnesses each of those drive.

**The prototype that this file was written against is retired.** `compile_satl`,
`check_satl` and `run_calls` in `satellite/satl/satl_file.cpp` are no longer
called by anything; only `load_satl`, which reads a file, survives.

---

## M0 — `satellite_config.hpp` and the `arguments` values — **BUILT**

The file is in the satellite-004 folder, written by the author. **Every value is
quoted text, turned into a `satellite_number` at start-up, and used as a
ceiling** (DESIGN §1). Built: `satellite/config/satellite_config.hpp` is read by
the interpreter AND by the build (`build_number.py` is the one reader), every
row shows under `--debug`, and `build/arguments_cases` proves no name satl fills
in can be a row. A row of a million nines is accepted and capped by what the
machine allows; a row written `-1ULL`, `-4u` or `-9223372036854775808` is refused
before anything compiles.

| name | value | meaning |
|---|---|---|
| `object_bytes_max` | `"34359738368"` | 32 GB |
| `threads_max` | `"1000000"` | the target: one million threads running |
| `threads_startup` | `"256"` | the pool started first, then recalled |
| `file_size_max_bytes` | the same as `object_bytes_max` | |
| `max_memory_bytes` | `"61847529062"` | 64 GB less 10% |
| ~~`satc`~~ | ~~`1`~~ | **DEAD with the file (D0.1)** |
| ~~`satb`~~ | ~~`1`~~ | **DEAD with the file (D0.1)** |

~~**Decision D0.1:** which value means "never".~~ **ANSWERED 2026-09-16:** there
is no `.satc` and no `.satb` to never build. `arguments.sate` took their place as
the one flag about the one file.

**What this milestone still owes:** nothing. Its entries live in check.sh.

## ~~M1 — the `.satc`, built by 24 threads~~ — **DEAD; what it was for is BUILT**

**There is no `.satc`.** What this milestone existed to produce -- the program as
numbers, made by many threads -- is the bytecode registry, and it is built:
`add_file_to_bytecode_registry` cuts a file into batches of lines across the warm
threads (a recall costs ~12.5 µs, so one thread per line would be ~400x slower --
DESIGN §6, now measured: 227 ms against 40 ms for 100,000 lines on one thread, and
1.5 ms on 256).

Its parts, and where they went:
- **The word table** (BUILT 2026-09-14): `words/make_words.py` → `words.tsv`,
  364 words numbered first-available, and `word_codes.hpp` generated from it.
- **Strings**: not `char32_t` in the language any more -- `satellite_string` is
  the author's 16-bit table, a wide character being 40000 and two codes (D3.1).
- **A method written on a variable** (`my_list.append`) still needs the
  variable's declared type: that is the selector, and it lexes through the
  `.find(` trigger rather than waiting for a resolver.
- **The prototype's defects** in the loader and the library path are NOT fixed
  here any more; they are MILESTONES M26, re-run one at a time on 2026-09-17.
- ~~**Decision D1.1**~~ **DEAD**: there is no `.satc` to run ahead of.

**Owed, and it is the one live thing left in this milestone:** a stale-input
guard. A `.satc` carried a header so it could not be believed after the word list
changed; the registry is built fresh every run, so nothing can go stale -- but
`.sate` (M3.6, MILESTONES M31) will need exactly that header when it is read
back.

## M1.5 / M2.5 / M3.5 — the converters — **ONE converter, over the registry**

(author, 2026-09-16) *"We just convert `.satc` back into `.satl`, that is the
whole milestone."* With the three files dead there are not four converters: there
is **one**, and its input is the registry's codes.

- **Half of it exists.** `bytecode_text()` prints a row as sixteen binary digits a
  code -- REGISTRY.satellite's own first column, and the reason a row is
  `std::bitset<16>` rather than `uint16_t`.
- **Codes become spellings** out of `words.tsv`, so `4163` comes back as
  `satellite.console.display`, and a payload comes back as the literal it holds.
- **Combine's marks come back as comments** (what M2.5 was for): where the main
  thread waits, which codes are one batch, and the batch size chosen. `--plain`
  drops them, and that output must equal the unmarked program's.
- **Comments and spacing do not come back, and this says so rather than
  pretending:** `//` never reaches the registry (003 DESIGN §5.6).

**Done when** the second pass is identical: `.satl` → codes → `.satl'` → codes',
and the two code streams are equal. The source round-trip is normalised; the
numbered one is exact, and the exact one is asserted.

**Blocked only by M24** (MILESTONES) for what it may RECORD from a stored
program, not for what it may print from a live registry.

**Entries:** a word deeper than six numbers; a payload holding `"`, `\` and a
newline; a code no word has; a 100 MB literal; an empty program; DESIGN §9's list.

## M2 — satellite combine — **the marks live in the bytecode, not in a file**

(author) *"It's just a .satc file with marks."* There is no file, so combine
marks the REGISTRY: rows are reserved for it already (`batch_start`, `batch_end`,
`wait`, `batch_size`). Everything else about this milestone stands as written.

Combine studies the numbered program and marks:
- **where the main thread must wait:** input, random, time, file writes, and
  anything depending on the line before;
- **where commands can run in parallel,** such as independent loop iterations
  split into batches; each batch runs its calls in order and writes its output
  into its own buffer, and the buffers are printed in order (DESIGN §6);
- **how big a batch must be to pay for a recall,** at least several hundred
  commands (measured break-even about 450).

Combine is conservative: what it cannot prove independent stays in order.
- **Spacesuits are references:** two names can be one object.
- **Capsules called inside a loop** need a summary of what they read and write.
- **`polymorph` voids those summaries** for the object it changes.
- **An error inside a batch** is reported for the earliest failing iteration,
  exactly as one thread would report it.

**Done when** every program in the entries gives byte-identical output with the
marks and without them.

**Entries:**
- a loop writing a shared total
- a loop through two names for one spacesuit
- a loop calling `console.input`, and one calling `random`
- an error in iteration 700,001 of 1,000,000
- a `polymorph` inside a loop

## ~~M3 — the `.sati`: strings as bits~~ — **DEAD, and D3.1 answered**

~~Every string in the program turned into bits, 32 per character, so the numbered
file carries no text at all.~~

**ANSWERED 2026-09-16** (the author: *"32-bits only when we use the number 40000
as a 16-bit code"*). A string is 16-bit codes INLINE in the registry, counted,
with the author's character table; a character above U+FFFF is `wide_token`
(40000) and two codes. There is no `.sati`, and **D3.1 died with it** -- built
2026-09-17, checked by `check_strings16.py` against Python over 3,075 wide cases.

~~**M3.5 — the `.sati` converter**~~ **DEAD with it.** What a person reads a
string back from is the registry, and that is the one converter above.

## M3.6 — the `.sate` converter — **unblocked: `.sate` is defined and written**

(author, 2026-09-16) The author named a fifth file, `.sate`, and the same day said
*"there is no `.sate` yet"*. There is now: `sate_file.hpp` writes the registry out,
`arguments.sate` asks for it, and saving is a side effect of a run rather than a
pass of its own.

**What it holds** is the codes and the file names beside them -- the numbered
program, nothing re-derived. **What it does not have yet is a reader**
(MILESTONES M31), and this converter is the reader's twin: read a `.sate`, print
`.satl`, re-convert, and prove nothing was lost.

**It needs the header M1 used to carry:** a `.sate` is meaningless against a
different word list, so it records the word-list digest and refuses one it does
not have, in plain words, rather than printing a word whose number it guessed.

**Entries:** the converter's list above, against a `.sate` -- plus a truncated
one, one from another word list, and one that is a directory.

## M4 — `satellite_number` — **BUILT**

DESIGN §4: limbs laid side by side, the sign as a bool, and the digit count and
byte size each as a `satellite_number`. Small numbers stay inline. Built in
`satellite/satellite_variable_number/`, with division, text and power in their
own files, and checked against Python over 477,253 cases
(`number_cases`/`check` in check.sh). `satellite.variable.binary` and
`.percentage` came with it, each keeping its sign.

**The race is still owed, and it is the one that matters:** a million
`i = i + 1` against a C++ `long long`. What is measured today is that satellite
loses about 5x to CPython on that loop, which is the number M13's position and
M12's watching both have to move.

**Entries:** 0, −0, the largest `unsigned long long int` + 1; 27 nines and the
maximum number of digits; hostile text as a number. All in the harnesses.

## M5 — names and `satellite.log`

DESIGN §7:
- the name vectors, the `a–z A–Z 0–9 _` rule, and `write_entry`, with a mutex
  and its path from config;
- a clash in any vector is an entry.

**Entries:**
- `!@#$%^&*()` and `poly.#@($*&$&$` as names
- machine-code bytes as a name
- one name declared as both an object and a class

## ~~M6 — the parser, ported from 003 06 with fast paths~~ — **DEAD; there is no parser**

**Nothing was ported and nothing is owed under this heading.** A `.satl` is lexed
straight into 16-bit codes (`bytecode_registry.cpp`), every statement is judged by
`program_check.cpp` before anything runs, and `program_walk.cpp` walks it: six
shapes, a call is a POSITION rather than an object, and an expression reaches its
fast path through a switch on the token (`expression.cpp`). Variables,
expressions, `while`, capsules and calls all run. M0.6 put a TYPED line through
the same three steps, which is what proves there is one runner.

**What this milestone named that is still owed has moved:**
- `if` and `for` have no shape yet -- only `while` does. **No milestone owns
  them** (see WITHOUT A MILESTONE).
- Blocks, kept variables and `run <file>` **at the prompt** are M0.8, added below.
- A kept include keeping its canonical path after a `change` (ERROR #26) belongs
  with includes, and **no milestone owns it**.
- The 1,000,000-iteration loop race against C++ is owed, and is M4's race above.

## ~~M7 — the runtime that runs a `.satb`~~ — **DEAD as written; the runtime runs**

There is no `.satb` to run. The main thread runs the numbered program at full
speed today, and `satellite.thread.new` taking a thread from the pool is a word
with no library yet (MILESTONES M25).

**What is still owed is the POOL doing work,** and it belongs with combine (M2)
and M12: the 256 threads are warm at start-up and tokenise the program, then stay
parked -- nothing has ever handed them a marked batch to run, because nothing
marks batches yet.

**Race, when it lands:** a splittable 1,000,000-iteration loop on the pool
against the same loop in C++ on 24 threads. (Starting 1,000,000 threads doing
math is the author's target and is NOT run on this machine -- a 200k busy-wait
test froze it on 2026-09-16.)

## M8 — user-defined classes (spacesuits)

As 003 06 has them: fields, capsules, `satellite.protected`, `satellite.public`
and `satellite.constructor`. Every library is reachable from their capsules,
and a spacesuit inside a spacesuit takes the slower path.

**The machine is built and the grammar is not** -- MILESTONES M8 holds the detail
and the exact seam (`satellite.spacesuit my_class` already lexes as a declaration
and is refused in one branch of `check_statement`). **It does not wait on M6:**
there is no parser to port. MILESTONES M35 is the author's 2026-09-17 sketch --
a spacesuit naming its supertype, and `satellite.protected(args)` taking
arguments -- and it waits on three rulings, not on code.

## M9 — polymorph

Both spellings, `object_name.polymorph(args)` and `satellite.polymorph(args)`.
The area comes first. A class declared inside is reached as
`satellite.library.poly.class_name`, `poly.class_name`, or plain `class_name`
when that name is free.

**Decisions:**
- **D9.1** What "re-included into the individual capsules" means.
- **D9.2** What `args` are passed to.
- **D9.3** A class declared twice.

## M10 — `satellite.cxx() { C++ }`

An ordinary C++ file inside the block, compiled into a numbered library. This
is also the test for commands deeper than six numbers. POLYMORPH/M6 holds the
open questions.

## M11 — `satellite.infinity`

`satellite.variable.infinity x = satellite.infinity.new()`, in
`satellite/infinity.cpp`.

**Decision D11.1:** the arithmetic and comparison rules, before any code.

## M12 — finding more batches while the program runs *(one of the last milestones)*

(author) "As the interpreter runs, it continues to look for spots where it can
unmark things, where it can turn code into batches … we monitor int.int.int that
is being run, what names of variables are being called."

- **The pool is split:** some threads look for new batches while the rest run
  them. (author) "64 of them to search … and 196 of them to run". **64 + 196
  is 260**, so a 256-thread pool splits **64 + 192**.
- **Watching is cheap or it fails the bar.** Names are recorded at loop
  boundaries or by sampling, never on every command.
- **Seeing is not proving.** 1,000 iterations that never touched a shared
  variable say nothing about iteration 1,001: a branch that has not run yet
  may. So a batch found while running is either proven by combine's reading of
  the code (M2), or guarded: the first shared write drops that loop back to
  running in order, with its output held until then (the guess-then-check
  approach).
- **A new batch plan starts at the next time a loop begins,** never partway
  through a batch.

**The parallel group in the numbered file.** The author sketched two forms:

```
#1.1.1(args).#1.1.1(args)          -- two commands in parallel, inline

parallel_start:                    -- a group that ends at the first empty line
#1.1.1(args)
#1.1.1(args)
#1.1.1(args)
```

**Decision D12.1 — and two of its three problems died with the files.** The group
is CODES in the registry now, not lines in a text file, and rows are already
reserved for it (`batch_start`, `batch_end`, `wait`, `batch_size`):
- ~~An empty line cannot end the group in a `.sati`~~ -- there is no `.sati`, and
  a code marks the end.
- ~~The inline `.` could be read two ways~~ -- there is no text form to be
  ambiguous.
- **One cheap command per line costs about 12,500 ns** to hand to a thread
  against about 28 ns to run, so three such commands in parallel are about
  400× slower. Every line in a group must be a big unit: a range of loop
  iterations, a heavy capsule call, a file read.

Recommended: one form with a count, so no spacing, dot or end marker is
needed, and every line is a batch:

```
#parallel 3
#1.1.1(args)
#1.1.1(args)
#1.1.1(args)
```

**Done when** a program whose batches are only discoverable while running (a
loop behind a condition combine cannot resolve) speeds up after its first
entries; the guarded fallback keeps output byte-identical when iteration
1,000,001 suddenly writes a shared variable; and watching costs less than the
×1.05 bar on a program with no batches at all.

---

## Not in this plan yet

`satellite.access` (ACCESS_PLAN.md) -- now MILESTONES M34, with the register of
every object name and its type behind it -- and the network (003's M27).

~~The single compile to bytecode that the author expects to replace `.satc`,
`.satb` and `.sati` someday.~~ **It arrived, and it is what runs:** the 16-bit
registry replaced all three on 2026-09-16, which is why four milestones above are
struck. (The prompt and satl-term moved into M0.5-M0.7 on 2026-09-15.)

---

## WITHOUT A MILESTONE — the red notes (2026-09-17)

**Work this rework found with no milestone to live in.** Each one was real before
the rework and is real after it; what it lost was its owner, because the milestone
that owned it described a file or a pass that no longer exists. They are listed
for the author to place -- nothing here was invented to fill a gap, and nothing
was quietly folded into a milestone it does not belong to.

**1. `if` and `for` have no shape.** The walker has six shapes and `while` is the
only one that branches or loops. Old M6 owned "variables, expressions, `if` /
`for` / `while`, capsules and calls"; five of those are built and these two were
never written. They are one shape each in `program_check.cpp` and
`program_walk.cpp`. **This is the largest hole in the language today.**

**2. A kept include's canonical path (ERROR #26).** An include must keep the path
it first resolved to and never be resolved again from its text -- or a
`satellite.directory.change` at the prompt silently changes which file a program
includes. Old M6 carried it "at the prompt (from M0.6)"; M0.6 built the `change`
that makes it reachable, and no milestone owns the fix.

**3. Entries, as this file defines them.** "How a milestone is done" says each
milestone has an `entries/M0.5.sh` script that writes an `[entry]` block to
`satellite.log` for anything handled wrongly, until M5 builds `write_entry`.
**No `entries/` folder exists.** check.sh, the harnesses and the pty checks do
the work, and they answer exit statuses rather than writing entries. Either M5
adopts them, or the method changes to match what is actually run.

**4. Two documents still describe the dead files.** DESIGN.md §14 is titled "The
files: `.satc`, `.satb`, `.sati`" and describes all three as the shape of a run;
README.md's third bullet tells a reader the same. PLAN.md and MILESTONES.md were
reworked today; **those two were not, and I did not touch them without your say-so.**
DESIGN.md is the standards document, so it is the one that matters.

**5. Minting a word number.** M35 needs `satellite.supertype`, which does not
exist in `words.tsv`, and its shape (`satellite.supertype.<a name the user
chose>`) is unlike every path in the table. The numbering is yours and is frozen;
no milestone covers adding to it, and M25's "340 words numbered and not built" is
about the other direction.

**6. Giving the parked threads work.** 256 threads are warm and tokenise the
program, then park for the rest of the run. M2 marks batches and M12 finds more of
them while running, but neither says "hand a marked batch to a parked thread and
collect its buffer" -- that was M7's, and M7 described a `.satb` runtime. The
rework filed it under M2/M12; it is thin there, and it is the whole point of the
pool.

**7. `.sate`'s header.** A stored program is meaningless against a different word
list. M1 carried that rule for the `.satc`; M3.6 and MILESTONES M31 now mention
it, but no milestone owns writing the digest into the file that already exists.

**Decisions waiting on you, gathered from the rework:** what `&` `|` `^` `<<`
`>>` `!!` `~` mean (the nineteen QUESTION rows, MILESTONES M24); whether an
unknown escape is refused (ERROR #16 is half fixed); whether a stored program may
be re-lexed (M24's other half); and M35's three -- the supertype's meaning,
`satellite.protected(args)`, and the word number in 5 above.
