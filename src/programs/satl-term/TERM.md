# `satl-term` — the window, and what the keyboard means in it

This folder is the whole of the `satl-term` binary. It links gtk4 and vte and
**nothing of the runtime**: it never interprets, it spawns the `satl` sitting
beside it into a pty and renders the bytes that come back. `MILESTONES/M1.5.md`
is where the two-binary split is argued and `DESIGN.md` §10.3–10.4 is why the
window exists at all.

Created 2026-09-06, when the two files were lifted out of `src/programs/` so the
program could be worked on without reading past the interpreter's command line.
Named for the binary it builds — `satl-term`, not `term` — so the folder and the
thing it produces cannot be told apart.

## The files

| file | what it is |
| --- | --- |
| `window.cpp` | The command line, the `GtkApplication`, the window. Its title and size are the same three arguments `satellite.window.console.new("title", 800, 600)` takes, and this binary must not be a special case of that signature. |
| `terminal.cpp` | The VTE widget and the `satl` it spawns into a pty. Holds the exit policy — hold on failure always, close on success until M22 — and the one flag saying whether a child is alive. |
| `terminal.hpp` | One door to the above. |
| `keys.cpp` | What Ctrl-C and Ctrl-V mean, and the "any key closes a held window" rule. One controller, so a keystroke has one answer. |
| `keys.hpp` | Its two verbs. |
| `satl_term_todo.txt` | **The author's backlog** — a dependency-missing window, a file menu, a status bar, tabs, themes, fonts, the 80×24 rule. Deliberately untracked and deliberately not started. |

## The keyboard, as it stands

| interpreter | selection | Ctrl-C does |
| --- | --- | --- |
| **running** | either | stops the run — the key is **passed through untouched** |
| not running | **highlighted** | copies to the clipboard, window stays open |
| not running | none | closes the window |

Ctrl-V always pastes, running or not — so a person answering
`satellite.console.input` can paste the path they already had on their clipboard.

### The invariant the whole file is built on

> **The key is only ever taken when there is nobody to give it to.**

A capture-phase controller runs *before* the focused widget, so answering
`GDK_EVENT_STOP` means VTE never writes to the pty master. That is a loaded gun
pointed at M22, whose prompt cancels a line **by receiving the byte `0x03`** —
raw mode turns ISIG off, so no handler is involved. A window that swallowed
Ctrl-C would make the prompt it was built to host uncancellable, and the failure
would surface a milestone later in a file that never mentions windows.

So the running case returns `GDK_EVENT_PROPAGATE` and lets the kernel do the
work: VTE writes `0x03`, the line discipline raises SIGINT for the foreground
process group, and `satl`'s own handler (`src/system_facts/interrupt.hpp`, M11)
sets a flag the walk reads at its next statement boundary.

### Why the window does not kill the child

It would look identical and be wrong three ways:

1. It destroys `S0730`'s caret — the entire reason the first press sets a flag
   instead of killing (`interrupt.hpp:69-71`).
2. It skips the console drain, so output the program already produced is lost
   (`DESIGN.md` §10.2).
3. `src/programs/opening.cpp:127` promises the user that 130 means *"Ctrl-C
   stopped a running program **at a statement boundary**"*. A window-level kill
   yields the same 130 with that sentence made false.

`DESIGN.md:1813` rules the third meaning out in one line: neither half of Ctrl-C
*"is the same as taking the session."*

### The trap that had to be fixed to make copy reachable

The held-open window used to close on **any** key — including a bare
`Control_L`, which is the first half of every Ctrl-C. Ctrl-C-to-copy could never
once have fired. `is_only_a_modifier()` in `keys.cpp` is that fix, and it is
load-bearing rather than tidy. Verified: the pre-change binary closes on a bare
Ctrl, this one does not.

## How to test this without a human at the keyboard

There is no `xdotool`, no `Xvfb` and no `wtype` on this machine, but a full rig
is reachable and was used to verify every branch above:

- `mutter --headless --virtual-monitor 1280x800 --wayland-display=satl-test`
  gives a real compositor with a virtual monitor and **its own XWayland**, off
  the user's desktop. Its log names the X display; its auth file appears as a
  fresh `/run/user/1000/.mutter-Xwaylandauth.*`.
- Run the window with `GDK_BACKEND=x11`, and **`unset WAYLAND_DISPLAY`** — GTK
  prefers Wayland and will otherwise talk to the real session.
- Keystrokes: `XTestFakeKeyEvent` via `libXtst` (headers are installed).
  Mouse drags for a selection: `XTestFakeMotionEvent` + `XTestFakeButtonEvent`,
  **in steps**, or VTE does not register a selection.
- The clipboard needs an owner that stays alive; a ~50-line GTK4 helper doing
  `gdk_clipboard_set_text` / `gdk_clipboard_read_text_async` is enough.

Three things that cost real time and will cost it again:

1. **The first synthetic keystroke is always swallowed.** Send a warm-up key
   before the one under test, or you will conclude the feature is broken.
2. **Aim the drag with real geometry.** `XQueryTree` + `XTranslateCoordinates`;
   the window is not at `(0,0)` — it was at `(240,119)` under this compositor.
   A drag across the root window selects nothing, and Ctrl-C then correctly
   closes the window, which looks exactly like a copy bug.
3. **Never probe for the child with a loose `pgrep`.** `pgrep -f "satl --run"`
   matched *the author's own* `satl --run example/advanced.satl` sitting at a
   prompt, and reported a running interpreter that was not this test's. Match
   the test program's full path.

Scripts live in the session scratchpad, not in the tree — they are a rig, not a
suite. If this becomes a real test it belongs beside `tests/console_test/`,
whose `pty.hpp` already does the pty half without a display.

## Build wiring

`TERM_DIR` in `make_support/030-directories.mk` is the one directory variable
pointing inside another. **`TERM_DIR` and not `TERM`**: every login shell exports
`TERM`, and under `make -e` the environment wins — the directory would silently
become `xterm-256color`.

Three explicit rules in `060-compile.mk`, because these objects need
`$(WINDOW_CFLAGS)` and the generic `$(SRC)/%.o` pattern does not carry it. The
pattern rule itself needs no change: a make stem may contain a slash.

**`070-clean.mk` needed a second glob.** A shell glob does not descend, so
`$(SRC)/*/*.o` cannot reach `src/programs/satl-term/`. Without
`$(SRC)/*/*/*.o` a clean silently leaves these objects behind and the next link
takes them. A third level would need a third glob; there is no third level.

`satellite_debian/make_support/060-compile.mk` carries out-of-tree twins of the
three rules. Its clean and its object directories are derived from `$(OBJS)` and
needed nothing.

## Known defect, in `satl` rather than here

`install_interrupt_handler()` is at `src/programs/run_command.cpp:167`, but
`build_program()` runs at `:95`. **A Ctrl-C during compilation hits the default
disposition**: measured on a 2M-line program (7s to compile), pressing Ctrl-C at
1.5s gives `killed by signal 2` — no `SATELLITE: CTRL+C RECEIVED` line, no exit
130. In this window that reads *"the interpreter was killed by a signal."*

The comment at `run_command.cpp:161-166` also justifies its `clear_interrupt()`
with a slow-build scenario that **cannot happen today**, because no handler is
installed during the build. Left alone deliberately: it is the interpreter's bug,
not the window's, and that file was being edited in another session.

## State

The keyboard work and the folder move are commit `a719430`, on branch
`milestones-install-and-no-console-handover`. It was staged hunk by hunk: the
M16 containers work was live in another session inside `030-directories.mk` and
`040-sources.mk`, and landing it half-finished under this message would have
left a commit whose `SATL_SRCS` named source files the repository did not carry.
The committed tree was extracted and built on its own to prove it.

What is **not** done, and was left rather than missed:

- `LAYOUT.md` still lists `src/programs/window.cpp` and `terminal.cpp` at their
  old paths and has no rows for `keys.*`. Path citations in `PLAN.md`,
  `source_file.hpp`'s comment and the installer scripts under
  `satellite_debian/` and `satellite_enterprise/` also still name the old
  location. The author said to adjust these later; none of them affect the build.
- `MILESTONES/` and `SCRATCH.md/SESSION.md` name the old paths too and should
  **not** be rewritten — they are dated records of what was true then.
- No test in `tests/` covers any of this. The pty half of Ctrl-C is already
  covered by `tests/console_test/terminal.cpp`'s clause 6; the window half is
  not, and the rig above is what a real one would be built from.
