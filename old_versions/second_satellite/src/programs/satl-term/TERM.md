# `satl-term` — the window, its File menu, and what the keyboard means in it

This folder is the whole of the `satl-term` binary. It links gtk4 and vte and
**nothing of the runtime**: it never interprets, it spawns the `satl` sitting
beside it into a pty and renders the bytes that come back. `MILESTONES/M1.5.md`
is where the two-binary split is argued and `DESIGN.md` §10.3–10.4 is why the
window exists at all.

Created 2026-09-06, when the two files were lifted out of `src/programs/` so the
program could be worked on without reading past the interpreter's command line.
Named for the binary it builds — `satl-term`, not `term` — so the folder and the
thing it produces cannot be told apart.

**The File menu and the tabs under it landed 2026-09-06**, on the author's ask.
Three files joined the folder and two lost their file statics; the section
below is the whole of what was decided and why.

## The files

Six sources, and the table reads from the outside in — which is also the order
they are worth reading.

| file | what it is |
| --- | --- |
| `window.cpp` | The command line, the `GtkApplication`, the window. Its title and size are the same three arguments `satellite.window.console.new("title", 800, 600)` takes, and this binary must not be a special case of that signature. It assembles the menu above and the tabs below in a plain box, and opens the first tab. |
| `menu.cpp` | The File menu: four items, four window actions, and the two dialogs. The only file here that opens one. |
| `tabs.cpp` | The notebook — how many terminals a window is holding, which is in front, and what happens to a page whose interpreter is finished. **The file the one-child assumption moved into.** |
| `terminal.cpp` | One VTE widget and the life of the child in it: the palette, the font, the exit policy — hold on failure always, and **since M22 close on success only when a FILE was run** — and the per-terminal state that used to be file statics. |
| `child.cpp` | Where `satl` is, and what it is told. The only file that spells `--repl`, `--run` and `SATL_TERM`. |
| `keys.cpp` | What Ctrl-C and Ctrl-V mean, and the "any key closes a held terminal" rule. One controller, so a keystroke has one answer. |
| `*.hpp` | One door each, and each says what its file is separate from. |

Still on the author's backlog and deliberately not started: a
dependency-missing window, a status bar, themes, fonts, the 80×24 rule. (The
`satl_term_todo.txt` this table used to name is gone from the folder; it was
untracked, so nothing in git remembers it.)

## The File menu

    File
      New tab
      New window
      ────────────
      Open…
      ────────────
      Save output as…

**Four items and no fifth.** New tab and New window make somewhere to work,
Open puts a program in this window, and Save output as writes down what a
terminal is holding — which matters most in the state this binary was built
around, where a failed run is held on screen and the reason is on it. **There is
no Quit and no Close**: the window's close button is already there, and a menu
entry duplicating it is a second spelling of something the desktop has spelled
for thirty years.

**No accelerators, deliberately.** `keys.cpp`'s rule is that a key is only taken
when there is nobody to give it to, and a menu shortcut is a key taken from
every program that will ever run on the other side of the pty, forever, to save
one mouse click. It is the one decision here that cannot rot, because there is
nothing to rot.

**A menu bar and not a hamburger in a header bar.** The title bar belongs to the
desktop, which draws it with this machine's buttons and puts `--title` in it. A
window that draws its own to hold four items has taken that over to save a row
of pixels.

### What each item does, and the decision inside it

**New tab** — a tab running the prompt, opened by the same call the command line
makes for the first one. A window started with a program and a window handed one
an hour later hold the same kind of tab, which is what stops the menu from being
a second way of doing this with its own bugs.

**New window** — a second **process**, not a second window in this one. That is
the shape `window.cpp` already argues for where it registers the application
`G_APPLICATION_NON_UNIQUE`, and the shape `keys.cpp`'s statics are written
against. `/proc/self/exe` rather than `PATH`, so the new window is *this*
`satl-term` and spawns the same `satl` beside it.

**Open…** — a chooser filtered to `*.satl`, opening in the directory
`satl-term` was typed in, and then **the program runs in this window**: in the
tab in front if its interpreter has finished, and in a new tab if it has not.

> **One item, two places it can land, and that is a decision.** The author asked
> for "run it in this window", against a new window. The alternative to taking
> the free tab is either killing somebody's running program — which nothing in
> this binary may do, and `keys.cpp` spends forty lines saying why — or greying
> the item out while a child is alive. **Greying it out died at M22**, which has
> now landed and the prompt with it: the
> prompt runs until the person leaves, so the File menu's main item would be
> permanently unavailable in the window it was built for. The promise the item
> keeps is "in this window, and never at the cost of a run".

A file the chooser hands back with no path on this machine — a gvfs share — is
refused with a sentence, because handing `satl` a URI it cannot open would
surface as the interpreter failing to find a file the person had just picked
from a list.

**Save output as…** — the screen **and the scrollback**, through
`vte_terminal_write_contents_sync`, because the interesting half of a failed run
is usually the part that has already scrolled off. The terminal is **reffed for
the life of the dialog**: a chooser is a conversation, the window keeps running
underneath it, and what gets saved is the screen that was in front when the
person asked rather than whichever tab is there when they click Save.

Errors from either dialog go to a `GtkAlertDialog` and **not** onto the
terminal: the terminal is a transcript of what a *program* did, and feeding it
a sentence `satl` never printed puts that sentence in the file somebody is about
to save. A dismissed chooser is an answer, not a failure, and gets no dialog.

## Tabs, and what they cost

**A single tab shows no tab bar**, so a window nobody has asked for a second one
in is pixel for pixel the window M1.5 built. The bar arrives with the second
terminal and leaves with it.

**A tab closes when its interpreter is finished with; the last tab takes the
window.** That is how M1.5's behaviour survives unchanged — one tab, a clean
exit, and the window closes — while a clean exit in one of three tabs now closes
only its own.

**Since M22 that sentence is about a FILE.** *(2026-09-07.)* A tab with no file
runs the prompt — `child.cpp` adds `--repl` in exactly that case — and a prompt
that exits cleanly **keeps its tab**, because the exit word ends a session and
the screen is full of what the person did. PLAN §8's M22 entry asked for the
clean-exit arm to be deleted outright; it was written before tabs existed, and
deleting it now would leave every program opened from the File menu sitting in a
tab somebody has to dismiss. So the arm stays and the prompt opts out:
`hold_always || file.empty()`. **`hold_forced` is what keeps `--hold` alive**
across `Open…`, which recomputes the policy when it starts a file in a tab that
was a prompt a moment ago.

**The state that made this possible.** `terminal.cpp` held two file statics —
is a child alive, is the screen being held — and `keys.cpp` held three more. A
second tab would have been a second interpreter writing to the first one's
flags, which is not a bug that announces itself: it looks like Ctrl-C in the
idle tab closing the busy one. They are now per-terminal, hung off the widget
with `g_object_set_data_full`, and the widget frees them.

**A page and its terminal point at each other, stored rather than derived.**
`gtk_scrolled_window_get_child()` would answer correctly today, because VTE is a
`GtkScrollable` and a scrolled window parents those directly — and would
silently answer with an interposed viewport the day that stopped being true,
which reads as tabs that will not close.

**Tab labels need both widths.** A label that may ellipsize asks for no minimum
width at all, so the notebook hands it the smallest box it will take: the first
run of `tabs.cpp` produced a tab reading `p… t` where it meant `prompt`.
`gtk_label_set_width_chars` is the floor, `max_width_chars` the ceiling.

## The keyboard, as it stands

| interpreter | selection | Ctrl-C does |
| --- | --- | --- |
| **running** | either | stops the run — the key is **passed through untouched** |
| not running | **highlighted** | copies to the clipboard, nothing closes |
| not running | none | closes **the tab in front**, and the last tab takes the window |

The third row used to say "closes the window", and it still does when there is
one tab. It goes out through `terminal_finish` so that a tab closes by the same
road whether its child ended or somebody dismissed it.

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

### The menu takes no key at all

Every item is reachable by mouse only. That is the invariant above applied to
the newest way of asking for something: no Ctrl-O, no Ctrl-S, no Ctrl-N, so
nothing the menu can do is spelled with a chord a program on the other side of a
pty might have wanted. Verified: a held tab still closes on a bare letter and
still does **not** close on a bare `Control_L`.

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

**`GDK_DEBUG=no-portals`, or the two menu items that open a chooser cannot be
tested at all.** `gtk_file_dialog_open` and `..._save` **always go through the
desktop portal** — `gtk/gtkfiledialog.c:848-850` in 4.16.7 sets
`use_portal` TRUE unless that debug flag is set, and `GTK_USE_PORTAL=0` does
*not* turn it off. From a headless rig the request goes over the session bus to
the portal running in **the user's real session**, so nothing appears on the
test display, no error is printed, and the app is left holding an invisible
modal — which reads exactly like a dead menu item. Worse, it is a dialog
appearing on somebody else's screen. With the flag, GTK shows its own chooser on
the test display and the whole path is drivable.

Six things that cost real time and will cost it again:

1. **The first synthetic keystroke is always swallowed** — and so is the first
   **click** after a window appears. Send a warm-up key or click before the one
   under test, or you will conclude the feature is broken.
2. **Aim the drag with real geometry.** `XQueryTree` + `XTranslateCoordinates`;
   the window is not at `(0,0)` — it was at `(240,119)` under this compositor.
   A drag across the root window selects nothing, and Ctrl-C then correctly
   closes the window, which looks exactly like a copy bug.
3. **Never probe for the child with a loose `pgrep`.** `pgrep -f "satl --run"`
   matched *the author's own* `satl --run example/advanced.satl` sitting at a
   prompt, and reported a running interpreter that was not this test's. Match
   the test program's full path — and note that a scratchpad path can itself
   contain the string `satl-term`, which makes even that match too much; compare
   `readlink /proc/<pid>/exe` instead.
4. **A menu popover is a separate X window.** Capturing the toplevel does not
   show it. `XGetImage` on the **root** fails outright under XWayland
   (`BadMatch`), so capture windows by id, and find the popover in `XQueryTree`
   by its class.
5. **Clicking a menu item means measuring it.** Screenshot the popover, find the
   rows that contain dark pixels, and click the centre of the one you want.
   Guessed offsets landed on separators and looked like items that do nothing.
6. **Poll for the popover; do not re-click.** A second click on an open menu
   closes it, so a helper that retries on a slow map turns "not yet" into
   "never".

Scripts live in the session scratchpad, not in the tree — they are a rig, not a
suite. If this becomes a real test it belongs beside `tests/console_test/`,
whose `pty.hpp` already does the pty half without a display.

## Build wiring

`TERM_DIR` in `make_support/030-directories.mk` is the one directory variable
pointing inside another. **`TERM_DIR` and not `TERM`**: every login shell exports
`TERM`, and under `make -e` the environment wins — the directory would silently
become `xterm-256color`.

**One explicit rule and one pattern rule in `060-compile.mk`**, because these
objects need `$(WINDOW_CFLAGS)` and the generic `$(SRC)/%.o` pattern does not
carry it. It was three explicit rules until the File menu made them six, and six
copies of one command line is a list somebody adds a file to and forgets — the
reward for forgetting being a gtk header not found, forty lines from the file
they added. So `$(TERM_DIR)/%.o` compiles all of them and `window.o` keeps an
explicit rule for its `$(VERSION_DEFS)`, since an explicit rule outranks any
pattern.

**The shortest stem wins**, which is what makes that pattern beat the generic
one rather than tie with it: both match `src/programs/satl-term/keys.o`, with
stems `keys` and `programs/satl-term/keys`, and make considers matching pattern
rules shortest-stem first. Checked with
`make -n src/programs/satl-term/keys.o`, whose answer must carry
`$(WINDOW_CFLAGS)`. If it ever does not, every object here is being compiled
without gtk on its include path — and that failure is loud, which is what makes
the subtlety affordable.

**`070-clean.mk` needed a second glob.** A shell glob does not descend, so
`$(SRC)/*/*.o` cannot reach `src/programs/satl-term/`. Without
`$(SRC)/*/*/*.o` a clean silently leaves these objects behind and the next link
takes them. A third level would need a third glob; there is no third level.

`satellite_debian/make_support/060-compile.mk` carries out-of-tree twins of both
rules — same argument, output moved, checked with
`make -n build/objects/programs/satl-term/keys.o` from `satellite_debian/`. Its
clean and its object directories are derived from `$(OBJS)` and needed nothing.

A new source in this folder is therefore **two lines**: `TERM_SRCS` in
`047-window.mk` and the header in `$(HDRS)` in `040-sources.mk`. Neither
`060-compile.mk` needs touching again.

## Known defect, in `satl` rather than here

`install_interrupt_handler()` is at `src/programs/run_command.cpp:167`, but
`build_program()` runs at `:96` (line numbers as of `1fb1df7`). **A Ctrl-C during compilation hits the default
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

**The File menu and tabs landed inside `264a9c8`** — M17's commit, on the same
branch. They are not that milestone's work and its message says so in three
lines: another session committed the tree while this was sitting in it, which is
what happens when two sessions share a checkout. Nothing was lost and the code
in that commit is byte for byte the code driven below; this file is the record
that commit could not carry.

Every behaviour below was driven end to end on a headless compositor before it
was committed, with the rig this file describes:

| what was driven | what happened |
| --- | --- |
| the bar itself | `File` on screen; the popover holds the four items and the two separators |
| New tab | second tab, both labelled `prompt`, the bar appears |
| New window | a second **process**, default title, default size |
| Open… in an idle tab | ran there, scrollback kept, tab relabelled to the file |
| Open… while that tab was **busy** | went to a **new** tab; the running interpreter untouched |
| the `*.satl` filter | only satellite programs listed; the chooser's own type column says "Satellite source code" |
| Save output as… | wrote screen **and** scrollback, `satl`'s caret line included |
| a clean exit with two tabs | closed **its** tab; the window and the other tab stayed |
| **M22: type at the prompt, then `exit`** | the window **stayed**, with the whole session on it and "press any key to close" under it |
| **M22: `satl-term example/hello_world.satl`** | still closed when the file was done — the file arm is unchanged |
| Ctrl-C, nothing running, nothing selected | closed the tab in front; the bar hid at one tab; the **last** tab closed the window |
| Ctrl-C with a selection | copied, and nothing closed |
| a bare `Control_L` in a held tab | did **not** close it |

What is **not** done, and was left rather than missed:

- **The two chooser items were verified with `GDK_DEBUG=no-portals`.** On a real
  desktop they go through the portal instead — the same road every GTK4
  application takes, and the road the code is written for — but that road was
  not exercised here, because from this rig it reaches the portal in the
  author's own session.
- `LAYOUT.md` still lists `src/programs/window.cpp` and `terminal.cpp` at their
  old paths and has no rows for `keys.*`, and now for `menu.*`, `tabs.*` and
  `child.*` either. Path citations in `PLAN.md`,
  `source_file.hpp`'s comment and the installer scripts under
  `satellite_debian/` and `satellite_enterprise/` also still name the old
  location. The author said to adjust these later; none of them affect the build.
- `MILESTONES/` and `SCRATCH.md/SESSION.md` name the old paths too and should
  **not** be rewritten — they are dated records of what was true then.
- No test in `tests/` covers any of this. The pty half of Ctrl-C is already
  covered by `tests/console_test/terminal.cpp`'s clause 6; the window half is
  not, and the rig above is what a real one would be built from.
- **No way to close a tab whose program is still running**, other than the
  window's close button. That is the "nothing here may take a run away" rule
  meeting a tab bar, and it wants a decision rather than a `×`: today a tab ends
  when its program does, or when Ctrl-C ends it with nothing running.
