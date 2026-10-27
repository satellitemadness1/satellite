# OPEN ITEMS -- 2026-10-20: what is open, and what was done today

Written at the author's request before a /clear: *"document all of this into .md files, and then I can
run /clear"*. The older files, SCRATCH.md/OPEN_ITEMS_2026-10-11.md and OPEN_ITEMS_2026-10-10.md, still
hold everything before them. Nothing there is closed by this file unless it says so.

## 0. Where things stand

- **The tree**: branch milestones-install-and-no-console-handover, HEAD b48fb5d56 (BUILD 0058). build/satl
  and the installed ~/.satl/satl are BUILD 0066 (the author's own plain make, 02:43 today).
- **Uncommitted**:
  - the icons work (11 files; section 2);
  - the Debian 13 build fixes, make_support/047-window.mk and
    satellite/satellite_variable_window/window_wayland_fixes.cpp;
  - satellite/config/satellite_config.hpp (BUILD row 66).
- **Untracked**: LINK_PLAN.md and SATELLITE_WINDOW_SECTIONS.md.
- **Today's session** wrote two plans' worth of documents and changed no code. It committed, built and
  installed nothing.

## 0b. THE NEXT TASK, IN THE AUTHOR'S WORDS -- start here after /clear

**SUPERSEDED the same afternoon: KEY_PLAN.md at the repo root is the live plan** (KEY-1 to KEY-10,
nothing built). The author's later messages ask for:
- satellite.variable.strings;
- `.c4` files in dated folders, `~/.satl/console/2026-10-20/console-001.c4`;
- Left and Right across consoles, every console typed into included;
- a 32 MB cap;
- the settings `arguments.input.cache` and `arguments.input.file.cache`.

The author answered all six of KEY_PLAN's questions the same day, and its Section 6 lists the answers.
The proposal below is overruled; only its "each finished line" survives, by the author's answer 3.

The author, 2026-10-20:

> *"Then we will have to add "press the up arrow key" to re-appear what you just typed into the
> satellite.console, a feature of the console, just storing every key that is pressed into the
> console, and then if the up arrow is pressed, it scrolls through previous responses... so every time
> enter is pressed into the console, a new string is started, stored into a satellite.variable.string?
> Or how do you want to store it into the interpreter??"*

**What the tree has** (looked at 2026-10-20, not yet read in full):
- **The prompt already does it.** `satl` with no file reads each line through prompt::LineReader in raw
  mode (satellite/prompt/line_reader.cpp). Its Editor (editor.cpp) browses a History with Up and Down
  (history.hpp, history.cpp). The History holds each finished line in memory for one session, with no
  ceiling, and never the same line twice in a row. Where a history that outlives the session would
  live is "undecided: ~/.satl is an install directory, not a place for a person's data"
  (history.hpp:5-7). The unit cases are in prompt_cases.cpp (the arrows, Home, End, Delete, word
  moves).
- **A running program's `satellite.console.input()` has none of it.** It reads with
  `std::getline(std::cin, line)` (satellite/bytecode/console_calls.cpp:489): the terminal's own line
  editing and no history, so an Up arrow shows as `^[[A`.
- **A console a program makes** (`satellite.console.new`, `.typed`) reads lines in the kernel's
  canonical mode on the pty satl holds (a_line_was_finished, window_console.cpp:225-266). It has no
  history either.

**The answer proposed to the author** (not yet agreed):
- **Not a satellite.variable.string.** The history belongs to the console, not to a program: it has to
  work for any program's input without the program declaring anything, and a program's names end with
  their capsule.
- **Each finished line, not every key.** The line editor handles the keys (Backspace, the arrows,
  Home, End), and the history keeps what Enter sent.
- **The prompt's own History and Editor, reused.** `satellite.console.input()` reads through
  prompt::LineReader when its input is a terminal, and keeps std::getline for a pipe or a file. One
  History per run, in memory, as the prompt's is per session.

**Open for the author:**
- Only satl's own console and the terminal satl runs in (where `satellite.console.input()` reads), or
  also a console a program makes with `satellite.console.new`?
- Kept for one run, or written to a file so that the next run has it too? Where such a file lives is
  undecided (history.hpp).
- Shared with the prompt's history when a program is run from the prompt, or its own?

**Details for whoever builds it:**
- `.input(prompt)` writes its prompt with no newline, and takes display's options (console help).
- Ended input stops with S830 INPUT_ENDED, exit 55.
- Display lines go through satl's printing thread, so the queue must be flushed before a line is read.
- satl's own console is a VTE terminal on a pty (console_launch.cpp), and Ctrl-C, Ctrl-V and the rest
  are its key controller's.
- The suggestion not asked for, and only a suggestion: a word that lets a program read the history.

## 1. Questions waiting on the author

### SATELLITE_WINDOW_SECTIONS.md (written today: SEC-0 to SEC-6, nothing built)

Its Section 6 holds nine questions, each to answer before the milestone named:
1. Every window, or only a window with sections? (SEC-2)
2. One display in a section, or several stacked? (SEC-0)
3. A name in quotes for every display? (SEC-0)
4. Is 800x600 the inside, so that every window grows by its title bar? (SEC-1)
5. Rows as NM-7 built them (A) decide which layouts are whole? (SEC-2)
6. The source view's spelling, `display.source.view()`? (SEC-4)
7. The button's capsule last, as the author wrote it, or first, as GTK-11 has it? (SEC-6)
8. A progress value as a number 0 to 100, as the author wrote it, or a percentage? (SEC-6)
9. The call through a window too, `its_window.right.bar(75)`? (SEC-6)

Its "Taken without the author's word" list holds the readings the plan took on its own.

### LINK_PLAN.md (2026-10-18), Section 6, still open

- Where the capsule's name goes: the same question as 7 above, so one answer settles both.
- Red or blue links.

### Carried from this morning's session (2026-10-19/20)

- **satellite_toolkit's step 9 refuses on Debian 13.** The question is whether to teach it to accept an
  already-built GTK. It assumes the carried GTK must be rebuilt, which needs wayland >= 1.24 (Debian 13
  has 1.23.1). vendor/build_stack.py:296 copies wayland .pc files from /usr/lib64/pkgconfig only, and
  the GitHub zip has no vendor/new/gtk-* sources.
- **The clang rebuild** (`./debian_clang_tool.py build --version 23.1.3 --cc ~/opt/clang-23.1.3/bin/clang`)
  has not been started. A further question: when step 2 re-runs with the system clang, should a
  finished build made by hand with another `--cc` only be re-linked?
- **`~/.local/bin/satl`.** satellite_enterprise/install.sh's header promises that link, and no code makes
  it, so `satl` is not on PATH. Making the link was recommended (~/.profile already puts ~/.local/bin on
  PATH); there is no answer yet.
- **Four checker gaps found 2026-10-19 on BUILD 0065**, not yet in SCRATCH.md/ERRORS4. Each is refused
  only when its line runs, instead of before the run:
  - a capsule called on a returned object, `a.make().secret()` (S230); **FIXED 2026-10-25** (open-work checker):
    refused before anything runs when no spacesuit has a secret that line can reach, in a sentence that holds
    whatever object the answer is (program_check.cpp, a_capsule_this_line_reaches); check.sh rows "M3: ...";
  - `file.satellite.append(<a time>)` (S301);
  - a bad `satellite.file.open` kind such as "append";
  - `satellite.time.now` without brackets. **FIXED 2026-10-26** (TIME-1): S110 before anything runs, told it is
    called with brackets -- satellite.time.now() or satellite.time.now("s") (program_check.cpp); check.sh's M4 row.

  `satellite.time.now()` is not built either. **FIXED 2026-10-26** (TIME-1): built, the time since 1970 in the unit
  asked for (TIME_PLAN.md); check.sh's "TIME-1: ..." rows.

## 2. What was done today

- **SATELLITE_WINDOW_SECTIONS.md.** The author's three messages became a plan:
  - sections respelled `.satellite.window.section`;
  - bare section names;
  - fraction sizes;
  - the whole-window rule, S157 MISSING_SECTION, "you need to section off the entire window";
  - displays file, text, image, console, text.input, text.view, source.view, button and progress, with
    `bar(75)` setting a progress bar.

  Nine read-only readers mapped the code first. The plan was then reviewed in three rounds, with a
  fourth round for SEC-6: 61, 18 and 22 findings, of which 52, 16 and 14 held, and every finding that
  held is fixed. Its Section 3 holds the facts with file:line.
- **Suggestions offered in chat and not added** (the author chose the button and the progress bar):
  - a slider, a checkbox and a choice (a dropdown) as displays;
  - a canvas;
  - `display.list(a_list)`;
  - `display.terminal()`, a real shell (GTK-18);
  - SVG pictures through GTK 4.24's own GtkSvg;
  - calling other displays by name, such as `files("/tmp/")`.
- **The icons work, round 2** (workflow wf_152c6543-cbd, started this morning) finished and was checked.
  - Its fixer changed four files: satellite_enterprise/install_support/072-icons.sh, 030-arguments.sh,
    020-saying-things.sh and 010-defaults.sh.
  - The one code fix is in 072-icons.sh:73: `rm -f -- "$2.installing" 2> /dev/null || :`. With
    `set -e` on, a failed rm there ended install.sh with no reason given.
  - The rest are comment and usage text.
  - BUILD is still 66, HEAD is unchanged, and nothing was written into ~/.local/share.
  - Not done: a build of a full copy and a check.sh run. The proofs ran in mini repos.
  - Its suggestion: 065-config.sh:73 has the same unguarded rm under `set -e`. **Done 2026-10-23:** 065-config.sh's
    two rms and 070-desktop.sh's two cleanup rms now end `|| :`. check_install.sh passed 30 and failed 0. The patch is
    backups/2026-10-23-install-rm-guards/. 060-install-tree.sh:74 is left alone: it removes an old satl-term on
    purpose, and a failure there prints rm's own reason.
  - The author's next plain make should offer to install the icons.
- **Side effects on the home folder.** ~/.satl/satellite.log grew from 100,624 to 237,576 bytes. About
  210 lines come from today's read-only probes, which ran satl with the real HOME until the probe
  command was given a scratch HOME. One entry comes from round 2's fixer. Nothing else in the home
  changed. The log has not been trimmed.
- **Scratch space in RAM.** /tmp is tmpfs (32 GB, 9.1 GB free). Two old scratch folders are still
  there:
  - /tmp/claude-1000/-home-madness-code-cxx-satellite/ef2672b0-62fb-4a82-97b1-6695f86cd2c3/scratchpad/icons
    (13 GB, round 1);
  - /tmp/claude-1000/-home-madness-code-cxx-satellite/cbefb06a-6222-44d5-a51b-610c172b5443/scratchpad/round2
    (5.4 GB).

  They are kept until the author says to delete them.
- **mutter is not installed.** The by-hand GUI proofs (prove-sections.sh, press-a-button.sh,
  prove-canvas-tabs-menus.sh and prove-console.sh) need `sudo apt install mutter`.
