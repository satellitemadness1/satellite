# satellite-004 — SATELLITE_WINDOW

**WIN-3 IS BUILT AND A WINDOW APPEARS (2026-09-20).** `satellite.window.new(...)`
opens a real window with a real button in it, from a real `.satl` — run and
photographed, not reasoned about. Read **Part 2a** before anything else in this
file: it is what changed, and it moves WIN-1 off the critical path, answers WIN-6
and settles WIN-4. Parts 0 and 1 are still true and still worth not re-measuring.

**A BUTTON TALKS BACK (2026-09-21).** `my_button.pressed(when_pressed)` runs a
satellite capsule when the button is pressed, `my_button.press()` is the program
pressing it itself, and **a capsule now takes ARGUMENTS** — so a press can hand
one the piece that was pressed and the window it is in. That last piece is a
LANGUAGE milestone that the window forced; PROGRESS.md carries its row. **Part 2b is what was built, and
which thread runs it.** It was proved by clicking a real button three times on a
real compositor, not reasoned about:
`satellite/satellite_variable_window/press-a-button.sh`.

**WIN-1 IS ALSO BUILT (2026-09-20).** `make GTK=vendor` opens a window with no
GTK stack loaded from the machine: xkeyboard-config, the IBM Plex Mono family,
satl's own fonts.conf and GTK's schemas are carried as a GResource and spilled
before `gtk_init`. **What that cost, and the eight libraries satl still needs,
moved to its own file: GTK_AND_NO_DEPENDENCIES.md, `DEP-1` to `DEP-9`.** Read it
for anything about dependencies, vendoring or the static link; this file stays the
record for the window itself.

The GTK+ window: what is **proved**, what is **owed**, and what is still the
author's to decide. Written 2026-09-19/20, the same shape as SATELLITE_INFINITY.md
— one milestone each, `WIN-n`, because MILESTONES.md's numbering already carries
two M34s and two M35s and a GUI is a large enough subject to keep its own file.

**Read PROGRESS.md first** for what IS built of the *language*. It knows nothing
about any of this — grep it for `gtk`, `vendor` or `window` and you get zero hits —
so for the window, this file IS the record of what is built: `vendor/gtk/` (the static
stack, proved), `vendor/fonts/` (the font), and **since 2026-09-20
`satellite/satellite_variable_window/` and `satellite/bytecode/window_calls.cpp`,
which satl links.**

The brief, in the author's words (2026-09-19):

> *"we are wiring in GTK+ so that satl and satl-term become a single application"*
> *"we **must** statically build this application, and that way we can have a
> unified executable... nobody has gtk installed on their machines"*
> *"after this we can build all of the different widgets in to satellite, as tiny
> C++ executables"*

    satellite.variable.window my_window = satellite.window.new("window_title", 800x600)
    my_window.append(satellite.window.button("text") <position>)

---

# Part 0 — how to get back to the measured state

**NONE OF IT IS IN GIT.** A fresh clone has four scripts and nothing else: no GTK
tree, no venv, no build. Rebuilding costs ~2.2 GB for `build-static`, 753 MB for the
unpacked GTK, ~3,200 ninja targets, and six tarball downloads. Budget an hour.

    /usr/bin/python3 -m venv vendor/gtk/.mesonvenv          # NOT dnf -- see below
    vendor/gtk/.mesonvenv/bin/pip install meson packaging
    sh vendor/gtk/fetch.sh            # GTK 4.16.7, upstream sha256sum
    sh vendor/gtk/fetch-deps.sh       # the stack, pinned to what this machine runs
    sh vendor/gtk/configure-static.sh
    sh vendor/gtk/build-static.sh     # NOT a bare ninja -- see below
    STATIC=0 CC=/home/madness/opt/clang-current/bin/clang sh vendor/gtk/hello/build.sh
    sh vendor/gtk/hello/bare-machine.sh --with-xkb --with-fonts ./hello-dynamic

**The four gates, each of which cost a build the first time:**

- **meson must come from the venv, not dnf.** `configure-static.sh:20-25` gates on it.
  CRB ships 1.4.1; the venv gets 1.12 and they do not build the same thing.
- **`python3` on this machine is PyPy and has no `packaging`**, so the venv must be
  first on PATH **for ninja as well as for meson** — glib's `gdbus-codegen` starts
  `#!/usr/bin/env python3` and re-resolves at run time. Configuring in one shell and
  building in another is enough to break it, and it broke **2,235 targets in**. That is
  the only reason `build-static.sh` exists rather than a bare `ninja`.
- **`/usr/include/drm/drm_fourcc.h` must exist** (kernel-headers). `fetch-deps.sh`
  exits without it. GTK wants the header and never links libdrm.
- **`bwrap`, a live Wayland session, and unprivileged user namespaces** for
  `bare-machine.sh` — and that script binds `/opt/amdgpu/lib64` and a fixed list of
  AMD/mesa paths. **On a non-AMD machine it needs editing before it proves anything.**

**One more mismatch worth knowing:** GTK's archives are built by **clang 24** through
the generated `-w` wrapper, but `hello/build.sh` links with **`/usr/bin/gcc`** by
default. The measured binary is gcc-linked against clang-built archives. It works;
it is not what a reader assumes from "CC here is clang 24".

**Paths:** every GTK citation in this file is relative to `vendor/gtk/gtk-4.16.7/`,
which a fresh clone does not have. `grep gtk/gtktext.c` from the repo root finds
nothing, and the citation is not wrong — the tree is just not fetched yet.

# Part 1 — what was MEASURED, so nobody measures it twice

Everything here was run, not reasoned about. Commit `efc0ddd`.

## A GTK4 binary that carries GTK works on a machine with no GTK

`vendor/gtk/hello/hello-static.c` built with `STATIC=0` (the binary is
`hello/hello-dynamic`; `hello/hello-static` is the `-static` one below, which
crashes), run under `bwrap` in a root holding the binary, the GL driver, the
Wayland socket, glibc — **plus `/usr/share/X11/xkb` and one font directory**,
which `bare-machine.sh --with-xkb --with-fonts` binds and which **WIN-1 exists to
remove**. Without those two it is exit 139, not 0, and that is the whole point of
WIN-1:

    window presented
    clean exit
    --- exit status 0 ---

No GTK, no glib, no cairo, no pango, no GSettings schemas, no icon theme, no MIME
database, no gdk-pixbuf loaders, no GIO modules, no D-Bus. `hello/bare-machine.sh`
binds only the libraries `ldd` names, one at a time, so "a machine with no GTK" is
not quietly a machine that still had GTK on it.

## A FULLY static binary links and CANNOT DRAW

`-static` gives `not a dynamic executable`, 21 MB stripped — and then SIGSEGVs.
**Not** the static-glibc/`dlopen` hazard everyone expects; that part works, it
reaches `eglInitialize`. The cause is duplicate library state:

    #0  wl_list_insert ()          from /opt/amdgpu/lib64/libwayland-client.so.0
    #1  wl_proxy_create_wrapper () from /opt/amdgpu/lib64/libwayland-client.so.0
    #2  dri2_initialize_wayland () from /opt/amdgpu/lib64/libEGL_mesa.so.0
    #5  gdk_display_init_egl       at gdk/gdkdisplay.c:1836

Statically linked, libwayland-client exists **twice**: ours, and the one the GPU
driver dlopens. GDK makes its `wl_display` with ours and hands the pointer to EGL,
which passes it to its own copy, whose lists never saw that object. It crashes in
the dynamic build too, which is how we know it is not glibc.

**THE RULE, and it generalises: a library whose objects cross into a dlopened
driver cannot be static.** Check it before static-linking anything else the driver
also loads.

## So the shape that works

| carried inside satl | must exist on the target |
|---|---|
| GTK, GSK, GDK | glibc — every Linux has it |
| glib, gobject, gio | libwayland-client, libwayland-egl |
| cairo, pango, harfbuzz, freetype, fontconfig, pixman | libEGL + the vendor driver |
| gdk-pixbuf + png/jpeg/gif/tiff loaders | **xkeyboard-config data** (WIN-1) |
| graphene, libepoxy, libxkbcommon | **at least one font** (WIN-1) |

`ldd` on the working binary, in full: `libm`, `libresolv`, `libc`, `ld-linux`,
`libwayland-client`, `libwayland-egl` — and `libffi`, which is NOT the binary's own:
`readelf -d` names six NEEDED entries and libffi is not one, it comes in behind
libwayland-client (WIN-7). Nothing else. Every item on the right is present on any
machine that can show a window at all.

## What this build GIVES UP, on purpose

Each is a comment in `configure-static.sh` and a user-visible capability, not a build
detail. **A static satl cannot print and cannot play media.**

| option | what is lost |
|---|---|
| `-Dprint-cups=disabled` | printing, entirely |
| `-Dmedia-gstreamer=disabled` | video/audio playback in a widget |
| `-Dpango:libthai=disabled` | Thai word-breaking |
| `-Dfreetype2:brotli=disabled`, `:bzip2=disabled` | WOFF2 web fonts, bzip2 PCF fonts |
| `-Dfreetype2:harfbuzz=disabled` | auto-hinter quality for glyphs outside a cmap |
| `-Dvulkan=disabled` | the Vulkan renderer — **and satl-term links libvulkan today**, so the pair already differ in how they draw |
| `-Dxkbcommon:enable-xkbregistry=false` | the rules-XML registry (would drag in libxml2) |
| `-Dx11-backend=false` | X11 entirely (WIN-10) |

**`-Dgdk-pixbuf:gio_sniffing=false` is not a loss, it is what makes
`builtin_loaders=all` WORK.** Left at its default, gdk-pixbuf picks a loader only
through `g_content_type_guess()` — the system MIME database via `XDG_DATA_DIRS` — so
on a bare machine the loaders compiled *into* the binary are unreachable. There is no
`shared-mime-info` wrap, so the build resolves it from the system and **succeeds in
silence**. Anyone regenerating the meson line who drops this loses image loading with
no error anywhere.

## GTK's own wrap set does not build statically

Eleven defects in the table in `vendor/README.md`, each fixed in
`vendor/gtk/fetch-deps.sh` or `configure-static.sh` with the error that earned it
quoted beside it — **and two more that are not in that table**, recorded in prose
below it, because they were the compiler rather than the source: `CC` here is
clang 24 from LLVM trunk, and pango **1.54.0 — the release this machine runs (`pango-1.54.0-3.el10`), not an old one** — dies on
`-Wunused-but-set-global` over ordinary `G_DEFINE_TYPE` boilerplate.

`-Dc_args=-Wno-error` does **not** fix that, and the measurement is worth keeping:

    -Werror=unused-but-set-variable                                     FAILS
    -Werror=unused-but-set-variable  -Wno-error=unused-but-set-variable  OK   (after)
    -Wno-error=unused-but-set-variable  -Werror=unused-but-set-variable FAILS (before)
    -Wno-unused-but-set-global       -Werror=unused-but-set-variable    FAILS (before)

The fourth row is the one that matters most: a plain `-Wno-<name>`, not just
`-Wno-error=<name>`, ALSO loses from the front. (Measured on
`unused-but-set-variable`; the flag that actually kills pango is
`-Wunused-but-set-global`, a different clang diagnostic that orders the same way.)

Plain `-Wno-error` does not cancel a specific `-Werror=<name>`, only a blanket
`-Werror`; and meson puts user `c_args` BEFORE a subproject's own. Hence the
generated compiler wrapper that appends `-w` after the caller's arguments.

## The trap that would have proved the opposite

`pkg-config --libs gtk4` returns `-lgtk-4` — the **shared** library — in a build
configured `--default-library=static`. A binary linked from that line passes every
casual check and is not static. The static library is `libgtk_static`
(`gtk/meson.build:1116`), never installed, named in no `.pc`, and on disk it is a
**thin** archive of 557 members that references its objects by path.
**The consequence, which matters for shipping:** a thin archive cannot be copied,
cached or shipped — every machine that links satl must first build the whole of GTK.
That collides directly with the prebuilt `satellite_distribute` package, and it is a
WIN-6 problem, not a detail.

---

# Part 2a — WHAT WAS BUILT ON 2026-09-20, and what it changed

The author: *"I think we were actually trying to get a window to appear when the
window syntax is called, so let's do that"*. It does.

    satellite.variable.window my_window = satellite.window.new("window_title", 800, 600)
    my_window.append(satellite.window.button("text"), 400, 300)

`examples/window.satl` is that program. Run on a desktop it puts an 800x600
window on the screen with a button labelled `text` at its centre; the window was
photographed under a headless `mutter` to prove it rather than assert it.

## The four rulings that were made to get there

Each was reversible and each is written down here so the author can overturn it.

1. **The window words live in the interpreter, not in a `.so` — WIN-6 IS
   ANSWERED, and not by preference.** A window word answers a HANDLE, and
   `file_calls.hpp` had already ruled that a handle cannot be made inside a
   dlopened library. WIN-6's shapes (ii) `--export-dynamic` and (iii) one GUI
   `.so` both require exactly that crossing. So the word is shape (i). **A
   widget's drawing could still live in a `.so` one day; the word that answers a
   handle cannot.** That is the whole of the author's *"tiny C++ executables"*
   tension resolved, and it was resolved by a rule already in the tree.
2. **GTK links dynamically, gated on `pkg-config gtk4` — SO WIN-1 IS NO LONGER
   THE BLOCKER.** WIN-1's spill (xkb data, a font, the schemas) is what a
   **bare** machine needs. A machine that HAS GTK has all three already, so the
   language work never needed it. WIN-1 is still owed for shipping; it is no
   longer owed first, and this file said the opposite for a day.
   `make_support/047-window.mk` carries `GTK_PKGS`/`HAVE_GTK` **separate from**
   `WINDOW_PKGS`/`HAVE_WINDOW`: a machine can have gtk4 and no VTE, and there
   satl draws while satl-term is not built. Swapping in `vendor/gtk`'s static
   archives is a change to three make variables.
3. **`new("title", 800, 600)`, three arguments — WIN-4 IS SETTLED, and by the
   author's own precedent.** `800x600` genuinely does not lex, and while minting
   the words `make_words.py` printed 003's removed rows: one of them is
   **`satellite.window.console.new(title, width, height)`**. Three arguments is
   what 003 wrote. check.sh asserts `800x600` is still refused, so nobody
   rediscovers it.
4. **satl does not exit while a window is open**, and no word was needed for it.
   A program that opened a window and returned would take it down before anybody
   saw it. `main()` waits after `run_satl` returns; `.close()` is how a program
   ends it; a program with no window pays nothing, because the desk is not
   started until a window word runs.

## What was built

| where | what |
|---|---|
| `words/words_004.tsv` | four rows: `1 6 18` the type, `1 27` the family, `1 27 1` new, `1 27 2` button |
| `REGISTRY.satellite` | two method tokens, `focus` 0x0B27 and `title` 0x0B28 |
| `satellite/satellite_variable_window/window_desk.hpp/.cpp` | WIN-2's one GTK thread |
| `satellite/satellite_variable_window/satellite_window.hpp/.cpp` | the handle and the six operations |
| `satellite/bytecode/window_calls.hpp/.cpp` | the interpreter edge, and the ONE `#if SATELLITE_HAS_WINDOW` |
| `satellite_object.hpp` | **arm 13**, `window` |
| `machine_codes.hpp`, `s_codes.hpp` | `no_display` 50 / S730, `window_is_closed` 51 / S505 |

**THE ARM IS 13 AND NOT 15.** WIN-3 guessed 15 and said why it might not be:
*"15 is right only if the float and hex ... are built first"*. They were not, so
by the rule in `satellite_object.hpp:143-145` — arms take their numbers in the
order they are BUILT — the window is 13 and the float and hex move to 14 and 15.

## WIN-2, as built

One thread. It runs `gtk_init_check`, owns the `GMainContext` and every widget,
and parks in `g_main_loop_run`; the interpreter hands work over with
`g_main_context_invoke` and waits. **Measured before it was written**: GTK4 inits
on a second thread, the first thread posts a window in, clean exit 0.

**Not warmed at startup, and not yet warmed from the token pass either.** The
desk opens the first time a window word runs, so `satl batch.satl` on a headless
server pays nothing. WIN-2's better answer — satl already tokenises the whole
file, so it can know a window word is coming — is a speed-up on top of this and
is still owed.

## Two defects found by running it, both fixed, both worth keeping

- **A refused program HUNG with its window open.** The report printed and then
  `main` waited on a window nothing was ever going to close — a hang that reads
  exactly like the interpreter locking up. A run that STOPPED now takes its
  windows down; only a run that finished waits.
- **`w.ok` wanted brackets.** It is a question, not a doing, and a file's `.ok`
  has never wanted them. `.close()` and `.focus()` are doings and still do.

## The trap that put a window on the author's own desktop

Testing the headless path, `env -u DISPLAY -u WAYLAND_DISPLAY` **is not enough**:
libwayland falls back to `$XDG_RUNTIME_DIR/wayland-0`, so a run meant to have no
display connected to the real session and opened a window on it. A genuinely
headless run needs `XDG_RUNTIME_DIR` pointed at an empty directory too. check.sh's
window rows do that, and say why in a comment as long as this paragraph.

## What GTK COST, and it is not nothing

**check.sh's `satl links nothing that can reach a network` stopped being true.**
satl now has `libgio-2.0` in its `NEEDED` list, and gio can open a socket. That
row is the one the `satellite.feedback` flood promise rests on, so it was split
rather than loosened:

- **`satl imports no network entry point of its own` — still 0**, now matched on
  whole symbol names. The old row grepped for `connect` as a substring and began
  failing on `g_signal_connect_data`, which is GLib connecting a SIGNAL.
- **a second row names libgio out loud**, so nobody reads the first as the old,
  stronger claim.

**This is the author's to rule on.** The feedback promise is intact — no word
calls gio's network classes — but a static satl will carry that code, and
"cannot reach a network" is now "does not", which is a different sentence.

## What is STILL owed of the window

- **WIN-1**, for a machine with no GTK — still every word of it, just not first.
- **WIN-5** (cairo fallback), **WIN-8** (the notices), **WIN-9** (the console
  handover, still the author's and still unanswered), **WIN-10** (X11).
- **The font ruling of WIN-3** — 11px or 12px, and only Regular is in git. No
  font is set at all today; the window uses the theme's.
- ~~**a button that does something when pressed**~~ **DONE 2026-09-21 — WIN-11,
  Part 2b.** It was the queue back to the interpreter's thread, not the walker on
  the desk's. **More widgets than a button** is still owed: a button is the only
  piece `satellite.window` makes.
- **check.sh cannot prove a window appears.** Every window row runs headless on
  purpose, because a row that opened one would put it on the screen of whoever
  ran the suite. What is asserted is the shape; the appearing was proved by
  photograph.

---

# Part 2b — A BUTTON TALKS BACK, 2026-09-21 (WIN-11)

Commit `9c94eae`. check.sh 373 passed, 0 failed; `readelf -d` still names
exactly eight.

Part 2a's list of what was owed opened with *"a button that does something when
pressed. There is no signal from a widget back into a satellite program yet — that
is the next real piece, and it is bigger than it sounds: it means the walker
running a capsule on the desk's thread, or a queue back to the interpreter's."*

**It was the queue.** That was not a preference, and the rest of this part is
what it cost and what it bought.

    satellite.capsule when_pressed()
    {
        satellite.console.display("the button was pressed")
    }

    satellite.capsule satellite.main()
    {
        satellite.variable.window my_window = satellite.window.new("window_title", 800, 600)

        satellite.variable.window my_button = satellite.window.button("text")
        my_button.pressed(when_pressed)

        my_window.append(my_button, 400, 300)
        satellite.return(satellite)
    }

`examples/window.satl` is that program.

## THE RULING: the interpreter's thread runs every capsule, and the desk never does

GTK4 is not thread-safe and every GTK call must happen on the desk's thread —
that is WIN-2 and it has not moved. **The new half is its mirror image, and it is
just as absolute: the WALKER is not thread-safe either.** `run_statements` reads
one `BytecodeRegistry`, writes one `MachineState`, and the statement ring is a
global beside them. A capsule walked on the desk's thread while the interpreter
is still running the program is not a slow program, it is a corrupt one.

So GTK's `clicked` handler does the one thing it safely can: it copies the
capsule's **name** onto a queue and wakes whoever is waiting. The interpreter's
thread takes it off and walks it.

**What that shape gives, said out loud so nobody has to find out:**

| | |
|---|---|
| a press while the program is still running its own lines | **waits.** Not lost, and not run underneath the program |
| two presses | run **one at a time, in the order they were made** |
| a press in the instant the window closed | still runs — the queue is drained before the windows are counted |
| a press after a REFUSED run | thrown away. The report is printed, and a program told it stopped must not run on |
| a capsule that stops the program | stops the run; main() takes the windows down and satl exits with that code |

**Where to reverse it:** running the capsule on the desk would mean giving the
walker its own state per thread. That is a language decision and not a window
one, which is exactly why it was not made here.

## THE PUMP IS IN `run_satl`, AND THE WAIT IS STILL IN `main`

Part 2a put the "do not exit while a window is open" wait in `main()` with a
reason: *"that function returns from two dozen places, and a wait written at each
of them is a wait that will be missed from the next one added."* That reason is
still true and the wait is still there.

**The pump could not join it, and not by preference.** The registry, the capsule
table and the walker's state are `run_satl`'s own locals: by the time `main()`
sees anything they are gone. So `run_satl` pumps once the program's lines have
finished, and `main()`'s wait is what it always was — the safety net for the two
dozen early returns, every one of which is a refusal, and the thing that waits
out a window no button was ever wired to.

`windows_run_until_they_are_closed` takes **a way to run a capsule** and not the
walker: a `std::function<signed long long int(const std::string &)>`. That is
what keeps `program_walk.hpp` out of `window_calls.hpp`, which `expression.cpp`
and `program_check.cpp` both include for three integers about arity.

## `.pressed(when_pressed)` IS A NAME, AND THAT IS THE WHOLE POINT

A capsule is **arm 5** of the object model and nothing in the language makes one
yet (`satellite_object/satellite_capsule.hpp`). Worked out as an expression,
`when_pressed` is *"a name with no satellite.variable line declaring it"* — a
refusal of the program that is right. So the argument is read **as written**:
`expression.cpp` takes the `name_token` instead of evaluating it, gated on
`window_method_takes_a_capsule_name()` so that one file says which methods are
spelled this way.

**A name may stand there because the CHECKER proves it is a real capsule**, with
the whole `CapsuleTable` already in hand, before a line runs:

    b.pressed(nobody_wrote_this)    13  no capsule named nobody_wrote_this -- .pressed(...)
                                        names a capsule to run, and there is no
                                        satellite.capsule nobody_wrote_this() in this program
    b.pressed("when_pressed")       27  b.pressed takes the NAME of a capsule, written as it
                                        is written: b.pressed(when_pressed) -- not text and
                                        not a value worked out

**Text is refused ON PURPOSE, and it is not pedantry.** A quoted name would lex,
check, run, open the window, and fail at the moment somebody pressed the
button — with the window already up. That is precisely the refusal this checker
exists to move earlier.

`b.pressed` with no brackets reads the name back, the pair `.title` already has.

**AND THE CHECKER REMEMBERS THE TWO CODES IT VISITED, never `row[at - 1]` and
`row[at - 2]`.** It recognises the bare name by what stands before it, and a raw
index backwards can land INSIDE A PAYLOAD — where a string's characters are their
own Unicode numbers, and one of them may equal a token's code exactly. **This
tree has been bitten by that before**: `capsules_in()` carries a comment about a
string ending in U+1006 that made the next body a second `satellite.main` — *"the
only one checked and the one that ran"*. Two remembered codes cost three lines
and cannot be fooled, because the loop already skips every payload. **It is
hardening rather than a repair**: names lex as ASCII today, so no reachable
program could have got a payload character into that position. It is written
this way so that the day names stop being ASCII is not the day this breaks.
Reading BACKWARDS is also what makes a chain work —
`b.title("x").pressed(when_pressed)` puts the name far past the first method
the receiver's own check ever saw.

## `.press()` IS THE OTHER HALF, AND IT TAKES NOTHING

The author, 2026-09-21: *"`.press()` wouldn't include any arguments would it?
It's just a mouse click, so it's not like there's any arguments to clicking on
something"*. So `my_button.press()` is **the program pressing it**, and it runs
whatever `.pressed(...)` named, down the same path a person's click takes.

**IT EMITS `clicked` AND DOES NOT CALL `gtk_widget_activate()`.** Activate is
GTK's KEYBOARD path: for a button it requires the widget to be REALIZED and
does nothing at all when it is not (`gtkbutton.c:827`) — while still answering
TRUE. That is an answer that is wrong and does not say so. A mouse release emits
`clicked` directly (`gtkbutton.c:802`), which is what `.press()` does.

**A PRESS IS QUEUED, NOT RUN, even when the program is the one pressing.** So a
capsule that presses its own button adds a press to the line rather than
recursing into the walker — and a program that does that forever loops forever,
exactly as `satellite.statement.while` written the same way would.

**`.press` AND `.pressed` ARE ONE LETTER APART**, so each refuses the other's
argument by naming the other out loud:

    b.press(when_pressed)   13  .press() is the PROGRAM pressing it, and a click has
                                nothing to say, so it takes nothing. To name what a
                                press RUNS, that is .pressed(a_capsule)

**AND IT MAKES THE PRESS PATH PROVABLE WITH NO POINTER AT ALL**, which is the
second stage of `press-a-button.sh` below.

## A CAPSULE TAKES ARGUMENTS, AND THAT IS WHAT MAKES A PRESS USEFUL

The first version of this milestone shipped with this written under *what it
does not give*: *"A pressed capsule cannot reach the window. There are no
globals and a capsule gets its own frame, so `when_pressed` cannot see main's
`my_window` and cannot close it. It can print and it can write a file."*

**That is now built.** The author, 2026-09-21: *"let's do the arguments going to
that capsule when pressed right now, I know it's an entire milestone"*.

    satellite.capsule when_pressed(satellite.variable.window the_piece,
                                   satellite.variable.window its_window)
    {
        its_window.close()
    }

**THE WALKER IGNORED A CAPSULE'S PARAMETERS ENTIRELY**, and had since 004 began:
every capsule was entered with a fresh empty `VariableTable`. 004's own first
program has been written
`satellite.main(satellite.container.list<satellite.variable.string> arguments)`
from the start with nothing ever bound to it. So this was never a window
feature — it is the language, and PROGRESS.md is where its row lives.

**WHAT A PRESS HANDS IT IS DECIDED BY THE CAPSULE, and it has to be.** A press
has nobody to write its arguments: the program said `.pressed(when_pressed)` and
walked away. So the only thing that can say what `when_pressed` wants is
`when_pressed`, and there are exactly three shapes:

| the capsule declares | it is handed |
|---|---|
| `when_pressed()` | nothing |
| `when_pressed(satellite.variable.window p)` | WHAT was pressed |
| `when_pressed(p, w)` | ...and the window it was pressed in |

Anything else is refused where the capsule is NAMED, before the program runs —
a third parameter, or one declared anything but `satellite.variable.window`.

**THE PIECE KNOWS ITS WINDOW BECAUSE `.append` WROTE IT DOWN**, which is the one
place a piece ever enters one. The link is a `std::weak_ptr`: the window already
holds the piece strongly, and two strong references in a ring is a window and a
button that keep each other alive for ever.

**BOTH ARE SETTLED ON THE DESK, AT THE MOMENT OF THE PRESS**, and carried in the
queue rather than worked out later — because later the window may be gone. A
press that closes the last window and a press queued behind it are both
ordinary, and the second is still owed the window it happened in, closed or not.
A closed window is a thing satellite can hold and ask: `w.ok` is false.

**`satellite_window` gained `enable_shared_from_this` for this.** GTK hands a
handler a raw pointer and a capsule must be handed a VALUE, which is a handle.
It is always valid where it is used: every one is made with `make_shared`, and
the desk holds a strong reference for as long as a piece is on a screen — which
is the only time a press can arrive.

**ONE READER OF "CALL A CAPSULE".** The walker's own inline arm for `my_capsule()`
now calls the same `run_capsule` a press does, so the new frame, the binding and
the `close_files` on the way out cannot come to differ between them.

**AND `satellite.main`'s PARAMETERS ARE NOT SEEDED as declared names**, which
looks like an oversight and is the opposite. `run_main` is handed nothing and
binds nothing, so seeding them moved the refusal from the CHECKER to the WALKER:
measured, the program printed `before` and THEN stopped. check.sh asserts in as
many words that *nothing ran before the refusal*. **The real fix is to bind
them** — the words really are there, as the settings `arguments.argument_1`,
`arguments.length` and the rest — and that is a milestone of its own.

## WHAT WAS BUILT

| where | what |
|---|---|
| `REGISTRY.satellite` | two method tokens, `pressed` 0x0B29 and `press` 0x0B2A. `token_codes.hpp` is GENERATED from them |
| `satellite_window.hpp/.cpp` | `when_pressed`, `press_is_connected`, `window_pressed()`, `window_press()`, the `clicked` handler |
| `window_desk.hpp/.cpp` | the press queue, `the_desk_saw_a_press`, `the_desk_waits_for_a_press` |
| `bytecode/program_walk.hpp/.cpp` | `run_capsule()`, `CapsuleParameter`, and a header `capsules_in` now READS |
| `bytecode/window_calls.hpp/.cpp` | `.pressed`, and `windows_run_until_they_are_closed` in BOTH halves |
| `bytecode/expression.cpp` | the one argument in the language read as written |
| `bytecode/program_check.cpp` | the capsule must exist, the argument must be ONE name, and what a press may hand it |
| `structured-library.cpp` | the pump, after the program's lines and before the profiles print |
| `satellite_object/` (via the handle) | `enable_shared_from_this`, and a piece's weak link to its window |
| `examples/window.satl`, `check.sh`, `PROGRESS.md` | a button that closes its window, and twenty-two rows |

## IT WAS PROVED BY PRESSING ONE

`satellite/satellite_variable_window/press-a-button.sh`, about thirty seconds,
run by hand. It starts a mutter of its own and proves it **three ways**: with a
real pointer, with `.press()` which needs no pointer at all, and with a capsule
that closes the window it was pressed in.

    A REAL POINTER
      satl exit 0        (0 -- the window was closed and the run ended)
      main returned with the window open: 1   (want 1)
      capsule runs from real clicks:      3   (want 3)
        main is finished, and the window is still open
        the button was pressed
        the button was pressed
        the button was pressed
    THE PROGRAM PRESSING ITSELF -- .press()
      satl exit 0        (0 -- drained after the window closed)
      capsule runs from .press():         2   (want 2)
        main pressed it twice and is closing the window
        the button was pressed
        the button was pressed
    THE CAPSULE REACHING ITS OWN WINDOW -- a capsule taking the piece and the window
      satl exit 0        (0 -- and NOBODY closed the window but the capsule)
      the window arrived, by name:        1   (want 1)
      the capsule closed it:              1   (want 1)
        main is finished and has NOT closed anything
        the capsule was handed the window: reached from a press
        the capsule closed it

**THE THIRD STAGE'S EXIT 0 IS THE WHOLE PROOF.** Nobody closes that window but
the capsule: main presses and walks away, and if the argument had not arrived
the window would still be open when the timeout fired.

**THE SECOND STAGE PROVES WHAT A CLICK CANNOT.** Its program presses twice and
then CLOSES THE WINDOW on the next line, and both capsules still run — which is
`the_desk_waits_for_a_press` testing the queue BEFORE it counts the windows. The
ORDER is asserted and not just the count: main's own line comes first, because a
press waits for the program to finish.

**check.sh cannot do this, and the script says why** — every window row in check.sh
is headless on purpose, because a row that opened a window would put one on the
screen of whoever ran the suite. check.sh's six new rows assert that the CHECKER
knows `.pressed` and refuses both wrong spellings before anything runs. 366
passed, 0 failed.

**There is no sway, no wtype, no ydotool and no uinput on this machine, and XTest
cannot reach a Wayland client.** The click goes through mutter's own
`org.gnome.Mutter.RemoteDesktop`, and **that session dies with the connection
that made it** — so a shell loop of `busctl call` makes a session per call and
destroys each one before the next. That is why the clicker is one Python process
and not three lines of shell.

## THE TRAP THAT LOOKED EXACTLY LIKE satl LOCKING UP

**`gtk_init_check()` can block for ever on D-Bus, and nothing is printed.**
`gdk_display_should_use_portal` -> `check_portal_interface` (`gdk/gdk.c:525`) is a
**synchronous** `g_dbus_connection_call_sync` to `org.freedesktop.portal.Settings`
with no timeout of satl's. Under `dbus-run-session` the portal is activatable but
cannot finish starting, so the call never returns: the desk never sets
`desk_tried`, and the interpreter waits in `open_the_desk` for ever.

It cost an afternoon, and it was found only by attaching gdb:

    #2  open_the_desk()                        <-- the interpreter, waiting
    ...
    #6  check_portal_interface                 <-- the desk, inside gtk_init_check
    #11 gtk_init_check () at gtk/gtkmain.c:695

`env -u DBUS_SESSION_BUS_ADDRESS` is the fix for the test, and the script carries
it with the reason. **THE AUTHOR HAS TO RULE ON THE REAL VERSION OF THIS**, which
is Q-WIN-11a below: on a machine whose portal is wedged, `satl window.satl` hangs
with no message at all — and "a hang that reads exactly like the interpreter
locking up" is a defect Part 2a already fixed once.

## WHAT WIN-11 DOES NOT GIVE, and the first is the one that will be felt

- ~~**A pressed capsule cannot reach the window.**~~ **DONE the same day** — see
  *A capsule takes arguments* above. It was a language milestone and it was
  built, on the author's word: *"I know it's an entire milestone, but we can
  still accomplish it"*.
- **`satellite.main`'s own declared parameter is still not bound.** 004's first
  program asks for the program's words as a list and gets nothing; using the name
  is refused by the checker. The words exist as settings. THAT is the next one.
- **Nothing is kept between presses.** A press costs a capsule walk from cold
  each time, because there is no frame that outlives one.
- **A press at the PROMPT is not run**, and the reason it is safe is NOT the one
  this file first gave. The pump is `run_satl`'s, and the prompt returns 91 lines
  before it. What makes that harmless is that **no capsule can exist at the
  prompt at all** — `session.cpp` refuses `satellite.capsule` by name — so both
  spellings of `.pressed` are refused there, the name by "no capsule named ..."
  and text by "takes the NAME of a capsule". Checked over a pty, because a
  piped stdin never enters the prompt at all. **The first version of this bullet
  said "nothing is silently dropped" and was WRONG**: before the fix below, a
  typed `satellite.window.button("x").pressed("boom")` drew a window whose
  button was dead for ever and said nothing.
- **Only `clicked`.** No hover, no key, and no close-button signal a program can see.

## WHAT A FRESH READER FOUND, and both were real

Three agents were set on the diff with orders to refute rather than agree. Nine
of their twelve claims died under a second agent trying to break them; **two
were real and are fixed here**, and one led to the prompt correction above.

**1. THE "TAKES A NAME" RULE HELD FOR EXACTLY ONE SPELLING.** It was written
beside the RECEIVER's own check (`method_on_a_name`), which sees only the FIRST
method of a chain on a DECLARED name. So both of these passed the checker, drew
a window, and failed at the moment somebody pressed the button:

    w.append(satellite.window.button("y").pressed("no_such"), 400, 300)   no declared receiver
    b.title("t").pressed("no_such")                                      not the first method

That is **precisely the failure this milestone claims to have moved earlier**,
and it was proved by running it on a real compositor and clicking. Worse,
`.pressed("satellite.main")` chained that way would have re-entered `main` from
a press, because `run_capsule` looks a string up in the same table `satellite.main`
lives in — a spelling a bare NAME can never reach.

**THE FIX IS WHERE THE RULE LIVES, not what it says.** A method judged by its
receiver is judged once; the rule now sits in `names_in_statement`, the one loop
that walks a WHOLE statement and skips every payload, so every spelling passes
through it whatever it was written on. The same move closed a second hole beside
it: the old test was a one-code peek after the `(`, so `.pressed(when_pressed, 5)`
went through to be refused at run time. It now reads past the name and insists on
the `)`.

**THE LESSON, and it is bigger than this method:** a rule about HOW SOMETHING IS
WRITTEN belongs where statements are walked. A rule about WHAT A RECEIVER CAN DO
belongs with the receiver. Putting the first in the second's place is a rule that
holds for the example you tested and nothing else.

**2. THE PUMP BLOCKED IN FRONT OF THE ONLY `std::cout.flush()`.** satl's stdout is
buffered — `main()` calls `sync_with_stdio(false)`, and
`satellite.console.display` writes `'\n'` and never flushes, on purpose — and the
one flush on the program path is at the END of `run_satl`. The pump was put 34
lines in front of it, so **every line a program printed became invisible until
the last window closed**.

**It is a regression the pump introduced, and the shape of it is worth keeping.**
Before WIN-11 the only waiting was `main()`'s, which happens AFTER `run_satl` has
flushed — so moving a wait EARLIER moved it past a flush nobody was thinking
about. `run_satl` now flushes before the pump, and the pump flushes after every
capsule, so a person who pressed a button gets the answer to THAT press rather
than a page of them when the window finally closes.

## OPEN, AND THE AUTHOR'S — do not decide these

- **Q-WIN-11a: should satl defend against a wedged portal?** `GTK_USE_PORTAL=0`
  before `gtk_init` would take the hang away, and would also take away whatever
  the portal gives a sandboxed satl. The other answer is to leave it and say so
  in the refusal. Today satl does neither, and a hang is the failure a person can
  learn nothing at all from.
- **Q-WIN-11b: should a press that refuses take the whole run down?** It does
  today, on the argument that a capsule's refusal is the program's refusal. The
  other answer is that a GUI reports and carries on — which is what every other
  GUI does, and which would need somewhere to put the report.

# Part 2 — the milestones

## WIN-1 — the startup spill — **no longer the blocker; see Part 2a**

**CORRECTED 2026-09-20.** This was headed *"THE BLOCKER, nothing else matters
first"* and that was wrong: it blocks a **bare** machine, not the language work.
A machine with GTK installed already has the xkb data, a font and the schemas, so
WIN-3 was built against a dynamic GTK and a window appeared without any of this.
Every word below is still owed before satl can be SHIPPED to a machine with no
GTK. None of it is owed before the next widget.


Code links in; **data does not**. Three things must be carried inside satl as a
GResource and written to a writable directory before `gtk_init()`:

1. **xkeyboard-config** — `gdk/wayland/gdkkeymap-wayland.c:478-486` runs at seat
   creation and null-checks nothing, so a machine without this data gets a
   **SIGSEGV inside `gtk_init()`**, before any window exists and before anything
   is printed. The call GDK makes at seat creation,
   `xkb_keymap_new_from_names` (`xkbcommon.h:886`), resolves the
   rules/keycodes/types/compat/symbols tree off disk, and GDK offers no hook to
   substitute a keymap before it runs. libxkbcommon *can* take bytes —
   `xkb_keymap_new_from_string`/`_from_buffer` (`xkbcommon.h:929`, `:944`), which
   is how GTK accepts the compositor's keymap at `gdkkeymap-wayland.c:564` — but
   that wants an already-compiled keymap, not xkeyboard-config data, and the
   crashing path never reaches it. No meson option embeds the data either. Minimum closure for
   `evdev/pc105/us` is **34 files, 348,215 bytes**, and a *partial* tree fails
   exactly like no tree. **No script in the tree computes that closure**, so it cannot
   be re-derived or maintained, and an independent attempt landed at 37 files /
   357,882 bytes before trimming. Since a partial tree is a SIGSEGV rather than a
   warning, **commit the closure script or the explicit file list** as part of WIN-1;
   do not leave the number in prose. Reproduced independently here: the build compiles in
   `xkb-config-root=/nonexistent/satl-must-provide-xkb-data` on purpose, and the
   binary dies on a machine that HAS xkeyboard-config installed.
2. **A font.** Measured, and it corrects an earlier claim that this was cosmetic:
   **no font is FATAL, not degraded** — exit 139 twice, exit 0 twice with fonts
   present, after `GtkImage reported baselines of minimum -2147483648` and
   `g_object_ref: assertion 'G_IS_OBJECT (object)' failed`.
   **BUT THE MECHANISM IS NOT UNDERSTOOD, AND THE EVIDENCE IS n=2.**
   `hello-static.c` builds only a GtkLabel and a GtkButton — **there is no GtkImage in
   it**. The widget that reported the bad baseline is most likely GTK's client-side
   decoration close button, which is an ICON, not text. If so the crash is on the icon
   path and "embed a font" may not be the whole fix. **Re-measure this before building
   WIN-1 around it** — bind fonts but not the icon theme, and vice versa, and see which
   one actually stops the crash.
   `vendor/fonts/ibm-plex-mono/IBMPlexMono-Regular.ttf` is in git for this, 133 KB.
   GTK's own way is `gsk/gskrendernodeparser.c`, and the ORDER is the reverse of
   the obvious one: `FcConfigCreate()` + `pango_fc_font_map_set_config` on a fresh
   fontmap first (`ensure_fontmap`, :1138-1140), then spill the bytes to a temp
   file (`g_file_new_tmp`, :1195) and `FcConfigAppFontAddFile` +
   `pango_fc_font_map_config_changed` (:1168, :1180). There is no
   `FcConfigAppFontAddMemory`.
3. **GSettings schemas.** `g_settings_new()` on a missing schema calls
   `g_error()`, which is fatal and cannot be caught. Three unguarded sites in GTK,
   and one is the emoji chooser, whose menu item is in the default
   right-click menu of every editable text widget (`gtk/gtktext.c:6348`, and the
   same item at `gtk/gtktextview.c:9263`). **Right-clicking only builds the menu**
   — the abort comes one step later, when the item is chosen or Ctrl-. / Ctrl-; is
   pressed (`gtk/gtktext.c:1639-1647`), because the chooser is constructed lazily
   (`gtk/gtktext.c:7219`) and `gtk_emoji_chooser_init` calls `g_settings_new`
   (`gtk/gtkemojichooser.c:1015`). Four schemas, **2,426 bytes** compiled (`vendor/gtk/build-static/gtk/gschemas.compiled`; ~9 KB as XML).
   `GSETTINGS_SCHEMA_DIR` *prepends*, so it cannot regress a machine that has them.

**Ordering is load-bearing.** `initialise_schema_sources()` is wrapped in
`g_once_init_enter` (`glib/gio/gsettingsschema.c:342`): the first
`g_settings_schema_source_get_default()` freezes the source list permanently. All
of this goes at the very top of `main()`, before anything touches GSettings.

**Decisions:** where the spill goes (`$XDG_RUNTIME_DIR/satl-<pid>`, or
`g_dir_make_tmp` when unset); whether it is removed at exit or left for reuse.

## WIN-2 — one GTK thread, warmed without drawing

The author: *"the main gtk+ window starts automatically in another thread... load
as much as we can if that is possible without actually drawing the window"*. It is
possible, and the API exists — checked in the vendored source. **Every GTK path in
this file is relative to `vendor/gtk/gtk-4.16.7/`**; grepping them from the repo root
finds nothing.

    gsk/gskrenderer.h:51   gsk_renderer_realize_for_display (GskRenderer*, GdkDisplay*, GError**)
    gdk/gdkdisplay.h:72    gdk_display_prepare_gl           (GdkDisplay*, GError**)
    gdk/gdkdisplay.h:75    gdk_display_create_gl_context    (GdkDisplay*, GError**)

`gsk_renderer_realize_for_display` realizes the render pipeline against a
**display with no surface** — EGL up, GPU pipeline built, nothing on screen.

Warm, in order: the WIN-1 spill → `gtk_init()` → `gdk_display_prepare_gl()` →
`gsk_renderer_realize_for_display()` → a `PangoContext` and one measured string
(forces the fontconfig scan and the freetype face load) → one icon lookup.

Cannot be pre-done: the surface, its buffers, and the first frame's shader
variants, which GSK compiles lazily per drawing op.

**THE CONSTRAINT: it all runs on thread2 itself.** GTK4 is not thread-safe; every
call above must happen on **the thread that called `gtk_init()`**
(`docs/reference/gtk/question_index.md:87-89`) — which in this plan is also the thread
owning the `GMainContext`, because thread2 does both. So thread2 is:
spill → `gtk_init` → warm → `g_main_loop_run` and park. The 1024 and the walker
stay where they are. `satellite.window.new(...)` on the interpreter thread hands
work over with `g_main_context_invoke()`. **A thread per window object would
corrupt or crash** — that is why the author's "a thread per gtk object" becomes
this instead.

**Do not warm unconditionally.** Every satl run would otherwise pay for a Wayland
connection, a fontconfig scan and an EGL context — including `satl batch.satl` on
a headless server that draws nothing, where it is also a failed connect on every
run. **satl already tokenises the whole file before running a line**
(`structured-library.cpp`), so it can know whether any window word appears before
executing anything, and warm only then. Not a guess from the environment — a fact
from a pass that already happens. The REPL warms eagerly, since what gets typed
is unknowable.

## WIN-3 — the window words

None exist in 004 — 0 rows in `words/words_004.tsv` and in the generated
`words/words.tsv`. **003 HAD them and their numbers are GONE.**
`words/words_003.tsv` has seven (`1 6 15 satellite.variable.window` at :187,
`1 24 satellite.window` at :364, `1 24 1 satellite.window.new`,
`1 24 2 satellite.window.console`, … :369); they were removed on purpose
(`words/make_words.py:6`, "remove GUI commands") and the numbers were REASSIGNED —
`1 6 15` is now `satellite.variable.capsule`, `1 24` is `satellite.constructor`.
So minting takes the next free number, **`1 27` for `satellite.window` and `1 6 18`
for the type** (words_004.tsv already holds `1 25` feedback, `1 26` infinity,
`1 6 16` percentage, `1 6 17` infinity) — never 003's. To mint:
`satellite.variable.window` (the type — **the next free arm, which is 13 TODAY**.
The "arm 15" this file first claimed is a reservation, not a fact:
`satellite_object.hpp:156-157` are COMMENT lines for the float and hex, the live enum
ends at `infinity = 12, how_many_kinds = 13`, and `:143-145` rules that "arms take
their numbers in the order they are BUILT, not the order they were named". 15 is right
only if the float and hex reserved by SATELLITE_INFINITY.md Q26 are built first),
`satellite.window.new()`, `.close()`, `.focus()`, `.button()`. The author wants a
folder of folders, `satellite.window/`, one C++ file per object.

Rules that bite here: `words.tsv` is GENERATED from `words_004.tsv` and rows are
only APPENDED; every new header joins HEADERS in `make_support/040-sources.mk`;
`token_codes.hpp`/`word_codes.hpp` are regenerated by their scripts; a fixture
naming an unbuilt path turns red when the path is built.

**`.append()` places a button BY ITS CENTRE** in window coordinates — 400x300 is
the centre of an 800x600 window. Buttons are a fixed size, 12px font. GTK4 has no
absolute positioning in a box; this wants `GtkFixed`, and the centre-not-corner
rule means subtracting half the button's measured size at placement.

**Two constraints that live only in `hello/hello-static.c` and bind WIN-3:**

- **No `GtkApplication`, on purpose** (`hello-static.c:17-22`). GtkApplication is
  GApplication, which registers on the D-Bus session bus, and a bare machine may have
  none. `gtk_window_new()` needs none of it. If satl later wants GtkApplication for the
  launcher that is a separate decision with a separate cost — `satl-term/window.cpp`
  passes `G_APPLICATION_NON_UNIQUE` for a related reason.
- **`gtk_init_check()`, not `gtk_init()`** (`hello-static.c:155-158`): it returns 2
  instead of dying when there is no display. WIN-2's warm order says `gtk_init()`; for
  the headless case WIN-2 itself raises, the *check* form is the whole graceful path.

**A bug already paid for once, and WIN-2/WIN-3 wire exactly this signal:** a `destroy`
handler is called as `(window, user_data)`, so connecting `g_main_loop_quit` plainly
hands it the WINDOW where a `GMainLoop*` is expected. The first version of the
experiment opened the window, fired the timer and never quit. Use
`g_signal_connect_swapped` (`hello-static.c:196-199`).

**Decision the author owes:** he wrote `my_window.title(12px)` AND *"leave it
unchangeable at 11px"*. Which? Note this is a THREE-way conflict, not two:
`vendor/fonts/fetch.sh` and `.gitignore` both record the rule as *"every window title
is IBM Plex Mono, 11px, never bold or italic"*, and **only Regular is in git** — so a
12px bold anything is not merely undecided, it is unshippable without
`fetch.sh --family`.

## WIN-4 — `800x600` does not lex — **SETTLED 2026-09-20, see Part 2a**


`satl` reads `x600` as a hex literal (`xFFAA` is hex), so `800x600` is two numbers
side by side and is refused S110 (checked). Same for `400x300`. Options:
`new("title", 800, 600)`; a new NxM token; or a string `"800x600"`. **The author's
call** — it changes the syntax he wrote.

## WIN-5 — the cairo software renderer

`GSK_RENDERER=cairo` currently exits 1 with `Error 0 (Success) dispatching to
Wayland display`. It matters for a target with no GL driver at all, and
`gsk/gskrenderer.c:803` is `g_assert_not_reached()` if every renderer fails to
realize — so the fallback is load-bearing, not tidy.

## WIN-6 — linking GTK into satl — **the word question is ANSWERED, see Part 2a**


satl is built by **make**, not meson, so a make rule must name the archives.
`make_support/048-link.mk` holds only `LINK_ENV` and `LINK_STAMP`; the recipe that
actually links satl is **`make_support/050-build.mk:57-58`** (satl-term at :83-84,
with `WINDOW_LIBS` from `047-window.mk`). That is where the GTK archives have to go, and it the way `vendor/gtk/hello/build.sh` does — CFLAGS from
pkg-config, libraries gathered as FILES, `--start-group` because the graph has
cycles, and five archives excluded by name (`libmalloc-stats.a`,
`libcairo-trace.a`, `libcairo-fdr.a`, `libdemo.a`, `libintl.a`) because they
define `malloc`/`realloc`, interpose `cairo_*`, or collide with glibc's gettext.

**The unresolved tension, and there are THREE shapes, not two** (`vendor/README.md`
wrote all three down; an earlier draft of this milestone offered only the first two).
004 dlopens each word's library from `satellite-numbers/`. With GTK statically inside
satl, widget words as separate `.so` files would each need GTK's symbols back out of
satl. So: **(i)** link the widget words INTO satl; **(ii)** link satl with
`--export-dynamic` and keep them dlopened; or **(iii)** make the GUI **one `.so` with
GTK statically inside it** — one satl and one library, not one file. The
author's *"tiny C++ executables"* for widgets pulls against *"one executable"*.
**Decide before minting WIN-3, not after.**

Also live: 004 links dynamically on purpose so satl and its `.so` words share one
libstdc++. GTK is C and does not touch that argument; `-static` for the whole
binary would — and WIN-1's measurement says a fully static satl cannot draw
anyway.

## WIN-7 — the purity audit is CLEAN (corrected 2026-09-20)

**An earlier draft of this file said `libffi.so.8` leaks in from the system. It does
not**, and the correction is worth keeping because the method that found it is the one
to reuse. `ldd` prints libffi, but `ldd` prints the whole transitive closure.
`readelf -d vendor/gtk/hello/hello-dynamic` names exactly six NEEDED entries — `libm`,
`libresolv`, `libwayland-client`, `libwayland-egl`, `libc`, `ld-linux` — and libffi is
not among them. It appears because `readelf -d
/opt/amdgpu/lib64/libwayland-client.so.0` has `NEEDED libffi.so.8`, and
libwayland-client is the one library kept shared **on purpose** (Part 1). libffi did
build as a subproject: `build-static/subprojects/libffi/src/libffi.a` is on disk.

**Use `readelf -d`, not `ldd`, to ask what a binary itself requires.** To catch a
system library linked at BUILD time, grep the configure log for
`Run-time dependency ... found: YES`, which is meson saying *from the system* — that
is what caught libmount, libselinux, graphene and the X libraries.

## WIN-8 — the third-party notices

satellite is **MIT**; the stack is not one licence. LGPL-2.1-or-later (GTK, glib,
gdk-pixbuf), LGPL-2.0-or-later (pango), LGPL-2.1-or-MPL-1.1 (cairo), FTL-or-GPLv2
(freetype), MIT/BSD (harfbuzz, pixman, graphene, libepoxy, libxkbcommon, expat,
libffi, libjpeg-turbo, libtiff, fontconfig), libpng, zlib, BSD-3 (pcre2),
**Apache-2.0** (PCG, in 003 only — 004 has `satellite.random` numbered at `1 7`
and no implementation), **OFL-1.1** (IBM Plex Mono, now **vendored**; embedding it in the binary is
WIN-1's spill and is NOT built — `strings hello-static | grep -i plex` finds nothing,
and `satl-term/terminal.cpp:125` still asks fontconfig for
`"IBM Plex Mono,monospace 11"` with a system fallback).

**LGPL §6 is the one with teeth, and static linking is what gives it teeth:** the
recipient must be able to **relink against their own modified GTK**. Shipping
satellite's source plus `vendor/gtk/`'s four scripts satisfies it today. It
becomes a live constraint the moment `satellite_enterprise/` ships a static binary
*without* source.

**The OFL obligation is a BUILD STEP, not a courtesy.** `vendor/fonts/fetch.sh`:
*"Embedding a font in a binary is redistribution, and the OFL requires its notice to
travel along. Whatever satl ships, ships OFL.txt with it."* So WIN-1 must decide how —
spill `OFL.txt` beside the font, or answer it from `satl --licences`.

**Two choices the author owes** before `THIRD-PARTY-NOTICES.md` can be generated:
freetype (**FTL** recommended — GPLv2 would infect) and cairo (**LGPL-2.1**
recommended, same as GTK). FTL wants this line in the documentation: *"Portions of this software are copyright © `<year>` The FreeType Project
(www.freetype.org). All rights reserved."* — `<year>` is not decoration: `FTL.TXT:54`
says to replace it with the version actually vendored, and the line is meant to be
pasted verbatim.

## WIN-9 — force the satl-term console, or not — **THE AUTHOR'S, STILL UNANSWERED**

He asked it on 2026-09-19 and has not ruled. The recommendation given was: **do
not force it.** Static removes the "satl would not start where GTK is missing"
objection, but not the other two — a detached window still loses the program's
**exit status** and its **stdout**, and both are load-bearing here
(`satellite/machine/exit_status.hpp`; check.sh asserts 44 at line 1237; DESIGN §8
spends four bullets getting stdout right, one of them on pipes specifically,
`DESIGN.md:277-290`). 004 already removed 003's handover on purpose
(PLAN.md:147, and the branch is named `...-no-console-handover`).

Recommended shape: `satl` never opens a window on its own; the `.desktop` names
the window (`Exec=satl-term %f`, already true); `satl --window` for a person at a
console who wants one; and a console program calling `satellite.window.new()`
gets a window without satl becoming a terminal emulator — which is WIN-2's
on-demand warm. Forcing the window would give such a program **two** windows.

## WIN-10 — X11, and machines that are not Wayland

The build is `-Dx11-backend=false` on purpose: the X11 backend needs libX11,
libXext, libXi, libXcursor, libXdamage, libXfixes, libXinerama, libXrandr and
libXrender (`meson.build:520-532`), and
**none of them has a wrap**, so every one would link as a system `.so` and quietly
un-static the binary. libepoxy's own `glx_static` test proves the point by failing
the build: `attempted static link of dynamic object /usr/lib64/libX11.so`.

An X11 satl is a separate decision with its own vendoring, not a flag. Until then
satl draws on Wayland only.

## WIN-11 — a widget talks back — **BUILT 2026-09-21, see Part 2b**

`my_button.pressed(when_pressed)` says what a press runs; `my_button.press()` is
the program pressing it itself, and takes nothing (the author: *"It's just a
mouse click"*). Part 2a named this as the next real piece
and said the choice was *"the walker running a capsule on the desk's thread, or
a queue back to the interpreter's"*. **It is the queue**, because the walker is
no more thread-safe than GTK is: one `BytecodeRegistry`, one `MachineState`, one
global statement ring. Part 2b has the ruling, what it gives, the two open
questions it leaves the author, and the `gtk_init_check` D-Bus hang it found on
the way. Proved by clicking a real button three times, not reasoned about.

**A capsule now takes ARGUMENTS**, built the same day and the thing that makes a
press worth having: a press hands the capsule the piece that was pressed and the
window it is in, which is the only way it can close what it was pressed in. That
is a LANGUAGE milestone the window forced; PROGRESS.md carries its row.

---

# Part 3 — what was NOT done

- ~~**No window word exists.**~~ **DONE 2026-09-20 — see Part 2a.** Four words,
  two method tokens, arm 13, and satl links GTK when pkg-config finds it.
- **WIN-1's spill is designed and not built** — so a SHIPPED binary still depends
  on the target having xkeyboard-config and a font. It is not in the way of the
  next widget.
- ~~**No widget can talk back yet.**~~ **DONE 2026-09-21 — WIN-11, Part 2b.**
  A press queues the capsule's name and the INTERPRETER's thread walks it, proved
  by clicking a real button three times (`press-a-button.sh`). A press hands the
  capsule the piece that was pressed and the window it is in, so it can close
  what it was pressed in — which needed capsules to take ARGUMENTS at all, a
  language milestone built the same day (PROGRESS.md). Part 2b has the two
  questions it leaves open.
- **INF-2 was never reviewed by a fresh reader** (all four agents died on the
  account's session limit, 2026-09-18). Its evidence is `check_infinity.py`
  (98,184 cases, two mutants caught) and the suite. **INF-3** is next in
  SATELLITE_INFINITY.md whenever the GUI is not the priority.
- **`THIRD-PARTY-NOTICES.md` is not written** — waiting on WIN-8's two choices.
- **The full vendoring pass was not done.** `vendor/gtk/fetch-deps.sh` pins glib,
  pango, gdk-pixbuf, graphene and libxkbcommon to what this machine runs; the
  lower layer (zlib 1.2.11 from 2017, libpng 1.6.37, freetype 2.11.0, harfbuzz
  4.0.0) still comes from GTK's own wraps. Static linking means a distribution can
  never patch those under us, and the author's plan is to freeze for 3–5 years, so
  this matters more, not less.
- **libxkbcommon's checksum is weaker than the rest.** xkbcommon.org publishes no
  `.sha256sum` (all three spellings 404), so the hash in `fetch-deps.sh` was taken
  from our own download. It pins the file against later change; it does not prove
  the first download. Compare against the distribution's source package before
  shipping.
