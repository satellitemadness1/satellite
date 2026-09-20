# satellite-004 — SATELLITE_WINDOW

**WIN-3 IS BUILT AND A WINDOW APPEARS (2026-09-20).** `satellite.window.new(...)`
opens a real window with a real button in it, from a real `.satl` — run and
photographed, not reasoned about. Read **Part 2a** before anything else in this
file: it is what changed, and it moves WIN-1 off the critical path, answers WIN-6
and settles WIN-4. Parts 0 and 1 are still true and still worth not re-measuring.

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
- **More widgets than a button**, and a button that does something when pressed.
  There is no signal from a widget back into a satellite program yet — that is
  the next real piece, and it is bigger than it sounds: it means the walker
  running a capsule on the desk's thread, or a queue back to the interpreter's.
- **check.sh cannot prove a window appears.** Every window row runs headless on
  purpose, because a row that opened one would put it on the screen of whoever
  ran the suite. What is asserted is the shape; the appearing was proved by
  photograph.

---

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

---

# Part 3 — what was NOT done

- ~~**No window word exists.**~~ **DONE 2026-09-20 — see Part 2a.** Four words,
  two method tokens, arm 13, and satl links GTK when pkg-config finds it.
- **WIN-1's spill is designed and not built** — so a SHIPPED binary still depends
  on the target having xkeyboard-config and a font. It is not in the way of the
  next widget.
- **No widget can talk back yet.** A button is drawn and pressing it does
  nothing: there is no path from a GTK signal into a satellite capsule.
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
