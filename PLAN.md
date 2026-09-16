# satellite-004 — PLAN.md

Started 2026-09-14; reordered the same night around the author's three files:
`.satc`, `.satb` and `.sati`. DESIGN.md holds the standards; this file is the
order of work. One milestone at a time. **For each milestone the author writes
the pseudocode, Claude writes the C++**, and the milestone lands when its
entries are cleared.

**The order from here (author, 2026-09-15):** M0.5 (the build, the installer and
satl-term) → M0.6 and M0.7 (the prompt) → M0 onwards. The prompt comes before
`satellite_number`, which PROGRESS.md had as next.

## M0.5 — port the build, the installer and satl-term *(the very next milestone)*

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

## M0.6 — the prompt in satl, with `satellite.directory.change` and `.list` *(after M0.5)*

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
- **D0.6.1** Does a typed line go through `.satc` / `.satb` / `.sati`?
  Recommended: no — a typed line is already its own numbered form.
- **D0.6.2** From a pipe: no banner and no prompt text (recommended). And the
  session's status at the end of piped input: the first failing line's code, or 0.
- **D0.6.3** Names that are not UTF-8, or hold control bytes, shown as `\xNN` and
  `\\`, so the shown form is exact. How such a name is typed back is decided when
  M6 puts names into strings.
- **D0.6.4** Which parts of M0.6 the author writes pseudocode for.
- **D0.6.5** Keys typed while a line runs. 003 throws them away on purpose
  (`TCSAFLUSH`, `raw_mode.cpp:85-87`); a shell keeps them. Recommended: 003's.
- **D0.6.6** Build `current()` `1 18 2` and `exists(d)` `1 18 3` here too? They
  share `change`'s code, and the author named only `change` and `list`.
- **D0.6.7** Keep 003's table `type` column, which reads up to 64 KB of every file
  in a directory in order to list it?

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

## M0.7 — the prompt in satl-term *(after M0.6)*

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

## The shape of a run (author, 2026-09-14)

```
source .satl ──24 threads──▶ .satc ──combine──▶ .satb ──▶ .sati
   numbers for every word    batch marks: wait here,   strings as bits,
                             run these in parallel     32 bits per character
```

- **If there is no `.satc`, make it. If there is no `.satb`, make it.** The
  main thread runs "as fast as it can" meanwhile.
- **Once a `.satb` exists, the 256 start-up threads stay dormant** until the
  program reaches a batch the `.satb` marks.
- **`arguments.satc` and `arguments.satb` choose when the files are built**
  (M0).

---

## What exists now

The prototype (2026-09-14):
- `satellite/structured-library.cpp`, `satellite/arguments/`, `satellite/machine/`,
  `satellite/satl/` (moved under `satellite/` on 2026-09-15)
- the number index in `satellite-numbers/call_number.satellite.cpp`
- one library, `satellite.console.display`
- `check.sh` (24 checks) and `satellite/race/race.sh`

It runs `satellite.console.display` with a string, a whole number or a bool.
The review proved 17 defects (DESIGN §11). Its speed is in DESIGN §12:
`display` about **119 times faster than 003 06's**, and ×1.002 of C++ with
`write`/`put`.

**003 already writes `.satc` files and never reads them while running.**
`satl --satc` writes `~/.satl/cache/<name>.<hash>.satc`, but a traced ordinary
`satl` run touched a `.satc` or the cache folder 0 times (measured). 004 puts
the file into the run.

---

## M0 — `satellite_config.hpp` and the `arguments` values

The file is in the satellite-004 folder, written by the author. **Every value is
quoted text, turned into a `satellite_number` at start-up, and used as a
ceiling** (DESIGN §1). The author's values:

| name | value | meaning |
|---|---|---|
| `object_bytes_max` | `"34359738368"` | 32 GB |
| `threads_max` | `"1000000"` | the target: one million threads running |
| `threads_startup` | `"256"` | the pool started first, then recalled |
| `file_size_max_bytes` | the same as `object_bytes_max` | |
| `max_memory_bytes` | `"61847529062"` | 64 GB less 10% |
| `satc` | `1` | build the `.satc` before running |
| `satb` | `1` | build the `.satb` before running |

They are also readable from a program through the special `arguments`
variable. (author) `arguments.satc` and `arguments.satb` choose when each file
is built:
- **`1`:** build it before anything runs (the default).
- **`0` (false):** build it while the program runs.
- **A special value:** never build it, so satellite-004 runs as a plain
  interpreter.

**Decision D0.1:** which value means "never". The author first said `3`, then
"we need a special value for this."

**Done when** the Makefile and the interpreter both read the file; each value
shows under `--debug`; and a value of a million nines is accepted and capped by
what the machine really allows.

**Entries:**
- a value that is not a number, a negative value, an empty value
- a million nines
- `satc` set to `0`, `1` and the "never" value

## M1 — the `.satc`, built by 24 threads *(the first thing to build)*

- **The word table: BUILT 2026-09-14** (`words/make_words.py`,
  `words/satellite_words.hpp`). It holds 364 words numbered first-available
  (DESIGN §3.2), generated from 003's registry so no number can drift. Six
  numbers are held inline, and deeper commands go in `longer`.
- **Strings as `char32_t`: BUILT 2026-09-14** (`strings/satellite_string.*`),
  ahead of M3. 30,055 cases agree with Python's strict UTF-8 decoder.
- **24 threads convert the program.** One job per file (the main `.satl` and
  every spaceship), each file cut into large pieces so every recall carries
  real work. A recall costs about 12.5 µs, so one thread per line would be
  about 400 times slower (DESIGN §6).
- **The format is 003's SATC.md, ported:** `#1.5.1("Hello, World!")`, with a
  header recording the word-list version and the source file's size and time,
  so a stale `.satc` is rebuilt and never believed.
- **Full `satellite.` paths are numbered from the text alone.** A method
  written on a variable (`my_list.append`) needs that variable's declared type
  first, which is why 003's `.satc` leaves those words un-numbered until the
  names are resolved. 004 does the same.
- **The prototype's defects** in the code that carries over — the loader, exit
  statuses, SIGPIPE, the library path — are fixed here (DESIGN §11).

**Race:** time to a finished `.satc` for a 10-line file, a 100,000-line file
and 100 spaceships, against converting on one thread and against 003's
`satl --satc`.

**Decision D1.1:** with `satc = 0`, the program starts before the `.satc` is
finished. A mistake on line 50 is then found after lines 1–49 have run. 003
checks everything first.

**Entries:**
- a stale `.satc` (the source changed)
- a `.satc` from another word-list version
- a truncated or hand-edited `.satc`
- a `.satc` that is a directory, or unwritable
- 100 spaceships
- DESIGN §9's list

## M1.5 — the `.satc` converter: numbers back into `.satl` *(after M1)*

(author, 2026-09-16) "We just convert `.satc` back into `.satl`, that is the
whole milestone." Every file in the pipeline is text so that a person can read
it; this is the other half of that promise, a file that reads *back*. It is also
how every later milestone gets checked — a converter that returns the program
you wrote is proof the numbering did not lose it.

- **`satl --satl <file>`** reads a `.satc` and writes `.satl` source. **One
  program serves all four converters** (M1.5, M2.5, M3.5, M3.6) and chooses its
  reader from the extension.
- **The header decides whether it may run at all.** SATC.md §2: a `.satc` is
  meaningless except against the numbering that produced it. A word-list digest
  the converter does not have is refused in plain words. It must never print a
  word whose number it guessed.
- **Numbers become spellings** out of `words/words.tsv`, so `#1.5.1` comes back
  as `satellite.console.display`.
- **A method left un-numbered** — M1 leaves `my_list.append` waiting on the
  variable's declared type — comes back exactly as the `.satc` carries it.

**Comments and spacing do not come back, and this milestone says so rather than
pretending.** `//` never reaches the parser (003's DESIGN §5.6), so it is not in
the file to recover. What comes back is the program, normalised.

**Done when the second pass is byte-identical:** `.satl` → `.satc` → `.satl′` →
`.satc′`, and `.satc` equals `.satc′`. The source round-trip is normalised; the
numbered round-trip is exact, and the exact one is what gets asserted.

**No race.** Nothing here is on a speed path.

**Entries:**
- a `.satc` from another word-list version, and one with no header at all
- a truncated `.satc`, and one hand-edited into nonsense
- a word deeper than six numbers (DESIGN §3.3)
- a `#1.5` path beside a `1.5` that is the number one-and-a-half (SATC.md §1.1.1)
- an un-numbered method on a variable
- 100 spaceships, and an empty program
- DESIGN §9's list

## M2 — the `.satb`: satellite combine

(author) "It's just a .satc file with marks." Combine studies the numbered
program and marks:
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

**Done when** every program in the entries gives byte-identical output with
`satb = 1` and with the "never" value.

**Entries:**
- a loop writing a shared total
- a loop through two names for one spacesuit
- a loop calling `console.input`, and one calling `random`
- an error in iteration 700,001 of 1,000,000
- a `polymorph` inside a loop

## M2.5 — the `.satb` converter: combine's marks, made readable *(after M2)*

The `.satb` is the `.satc` with marks, so this is M1.5's reader plus the marks —
and the marks are the point. **Combine decides what runs in parallel, and today
a person has no way to see why it decided that.** This converter is that way.

- **The program comes back as `.satl`,** exactly as in M1.5.
- **Every mark comes back as a comment** the source ignores: where the main
  thread waits and what it waits on, which lines are one batch, and the batch
  size combine chose. A reader can then check a wait against DESIGN §13's rules
  by eye, which is the only review combine's conservatism will ever get.
- **`--plain` drops the marks,** and that output must equal M1.5's for the same
  program. That equality is the test that the marks changed nothing but marks.

**Done when** the marks explain every decision M2's own entries produce — the
shared total, two names for one spacesuit, `console.input`, the `polymorph` in a
loop — and `--plain` matches M1.5 byte for byte.

**Entries:**
- a `.satb` whose marks contradict each other (two batches over one line)
- a mark naming a line that does not exist
- a batch size of `0`, and one larger than the loop it marks
- a `.satb` with no marks at all
- M2's entries, converted and read back

## M3 — the `.sati`: strings as bits

(author) Every string in the program is turned into its bits, **32 bits per
character**, so the numbered file carries no text at all. One program,
`bits_to_cxx_str.cpp`, turns a bit sequence back into the C++ string a library
takes. This is the step before satellite someday compiles to bytecode in one
pass.
- **32 bits a character is UTF-32,** which C++ already has as `char32_t` and
  `std::u32string`. Every character has the same width, but plain English text
  becomes 4 times larger.
- **To stay within ×1.05,** each string is turned back once, when the `.sati`
  is loaded, never on every library call.

**Race:** 10,000,000 `display` calls of a string that came from a `.sati`,
against `std::cout`.

**Decision D3.1:** does `satellite.variable.string` itself become 32 bits a
character everywhere (the author: "we are going to have to rebuild
satellite_strings into sequences of bits"), or only inside the file?

**Entries:**
- an empty string
- a string with `"`, `\` and a newline
- every Unicode plane
- an invalid UTF-8 byte in the source
- a 100 MB string literal

## M3.5 — the `.sati` converter: bits back into text *(after M3)*

The `.sati` carries every string as bits, 32 to a character, and nothing in the
tree turns a file of them back. `bits_to_cxx_str.cpp` (M3) turns one sequence
into a C++ string; this turns a whole file into source.

- **Bits → characters → a string literal,** with `"`, `\` and newline escaped
  again, so what comes out is something the lexer would accept.
- **It checks as it reads.** A bit run that is not a multiple of 32, or a value
  that is not a Unicode scalar, is refused and named — never printed as a
  replacement character. A converter that quietly repairs is a converter that
  hides a broken writer.
- **Decision D3.1 changes this milestone's size, not its shape.** If the `.sati`
  becomes binary at 4 bytes a character, the reader changes and the output does
  not.

**Done when** every string in M3's entries comes back as the literal that was
written, and re-converting that source gives the same `.sati`.

**Entries:**
- an empty string, and a string of one character
- `"`, `\` and a real newline inside a literal
- a character from every Unicode plane, and an unpaired surrogate value
- a bit run of 31 bits, and one of 33
- a 100 MB string literal

## M3.6 — the `.sate` converter *(blocked: `.sate` is not defined yet)*

(author, 2026-09-16) The author named a fifth file, `.sate` — the one that runs —
and the same day said "there is no `.sate` yet". **Nothing in the tree defines
it**, so this milestone is written down and left blocked on purpose rather than
guessed at.

**Before it can start** three things have to be decided: what a `.sate` holds
that a `.sati` does not, whether it is still text a person can read, and whether
it replaces the earlier files or follows them. "Not in this plan yet" already
names the candidate — the single compile to bytecode the author expects to
replace `.satc`, `.satb` and `.sati` someday — and `.sate` may be that thing
arriving early.

**When it starts it is M1.5 again:** read the file, write `.satl`, re-convert,
and prove nothing was lost.

**Entries:** M1.5's list, against whatever a `.sate` turns out to be.

## M4 — `satellite_number`

DESIGN §4: limbs laid side by side, the sign as a bool, and the digit count and
byte size each as a `satellite_number`. Small numbers stay inline; the largest
digit count comes from M0.

**Race:** a million `i = i + 1` against a C++ `long long`. 003 06 spends 283 ns
a line on this loop.

**Entries:**
- 0, −0, and the largest `unsigned long long int` + 1
- 27 nines, and the maximum number of digits
- hostile text as a number

## M5 — names and `satellite.log`

DESIGN §7:
- the name vectors, the `a–z A–Z 0–9 _` rule, and `write_entry`, with a mutex
  and its path from config;
- a clash in any vector is an entry.

**Entries:**
- `!@#$%^&*()` and `poly.#@($*&$&$` as names
- machine-code bytes as a name
- one name declared as both an object and a class

## M6 — the parser, ported from 003 06 with fast paths

003's parser and resolver come across one part at a time, rewritten around the
number table and likely scenarios: variables, expressions (DESIGN §5), `if` /
`for` / `while`, capsules and calls. Every walker keeps its own stack.

This is also where selectors written on a variable get their numbers in the
`.satc` (M1).

**At the prompt (from M0.6):** a kept include keeps the canonical path it first
resolved to, and is never resolved again from its text after a
`satellite.directory.change` (ERROR #26). Blocks, kept variables and `run <file>`
come to the prompt here.

**Race:** a 1,000,000-iteration loop that displays, against the same loop in
C++.

## M7 — the runtime that runs a `.satb`

- **The main thread runs the numbered program at full speed.**
- **The 256 pool threads** stay dormant until a marked batch is reached.
- **A `.satb` still being built** (`satb = 0`) is used only for the parts it
  has finished; **the main thread never waits for combine.**
- **`satellite.thread.new` takes its thread from the pool.**

**Race:** a splittable 1,000,000-iteration loop on the pool against the same
loop in C++ on 24 threads. Also, start 1,000,000 threads doing math (the
author's target; 32.7 GB measured).

## M8 — user-defined classes (spacesuits)

As 003 06 has them: fields, capsules, `satellite.protected`, `satellite.public`
and `satellite.constructor`. Every library is reachable from their capsules,
and a spacesuit inside a spacesuit takes the slower path.

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

**Decision D12.1, with three problems to weigh:**
- **An empty line cannot end the group in a `.sati`,** which is written "with
  the spacing removed".
- **The inline `.` already means "method on"** (`my_list.append`) and sits inside
  every number (`#1.5.1`), so `a(x).#1.6.1.1()` could be read two ways.
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

`satellite.access` (ACCESS_PLAN.md), the network (003's M27), and the single
compile to bytecode that the author expects to replace `.satc`, `.satb` and
`.sati` someday. (The prompt and satl-term moved into M0.5–M0.7 on 2026-09-15.)
