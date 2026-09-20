# satellite-004 — SATELLITE_WINDOW

The GTK+ window: what is **proved**, what is **owed**, and what is still the
author's to decide. Written 2026-09-19/20, the same shape as SATELLITE_INFINITY.md
— one milestone each, `WIN-n`, because MILESTONES.md's numbering already carries
two M34s and two M35s and a GUI is a large enough subject to keep its own file.

**Read PROGRESS.md first** for what IS built. This file is the debt and the
evidence behind it.

The brief, in the author's words (2026-09-19):

> *"we are wiring in GTK+ so that satl and satl-term become a single application"*
> *"we **must** statically build this application, and that way we can have a
> unified executable... nobody has gtk installed on their machines"*
> *"after this we can build all of the different widgets in to satellite, as tiny
> C++ executables"*

    satellite.variable.window my_window = satellite.window.new("window_title", 800x600)
    my_window.append(satellite.window.button("text") <position>)

---

# Part 1 — what was MEASURED, so nobody measures it twice

Everything here was run, not reasoned about. Commit `efc0ddd`.

## A GTK4 binary that carries GTK works on a machine with no GTK

`vendor/gtk/hello/hello-static.c`, run under `bwrap` in an empty root holding
only the binary, the GL driver, the Wayland socket and glibc:

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
`libwayland-client`, `libwayland-egl`, `libffi`. Nothing else. Every item on the
right is present on any machine that can show a window at all.

## GTK's own wrap set does not build statically

Eleven defects, each fixed in `vendor/gtk/fetch-deps.sh` or
`configure-static.sh` with the error that earned it quoted beside it. The table is
in `vendor/README.md`. Two were the compiler rather than the source: `CC` here is
clang 24 from LLVM trunk, and pango **1.54.0, the current release**, dies on
`-Wunused-but-set-global` over ordinary `G_DEFINE_TYPE` boilerplate.

`-Dc_args=-Wno-error` does **not** fix that, and the measurement is worth keeping:

    -Werror=unused-but-set-variable                                     FAILS
    -Werror=unused-but-set-variable  -Wno-error=unused-but-set-variable  OK   (after)
    -Wno-error=unused-but-set-variable  -Werror=unused-but-set-variable FAILS (before)

Plain `-Wno-error` does not cancel a specific `-Werror=<name>`, only a blanket
`-Werror`; and meson puts user `c_args` BEFORE a subproject's own. Hence the
generated compiler wrapper that appends `-w` after the caller's arguments.

## The trap that would have proved the opposite

`pkg-config --libs gtk4` returns `-lgtk-4` — the **shared** library — in a build
configured `--default-library=static`. A binary linked from that line passes every
casual check and is not static. The static library is `libgtk_static`
(`gtk/meson.build:1116`), never installed, named in no `.pc`, and on disk it is a
**thin** archive of 557 members that references its objects by path.

---

# Part 2 — the milestones

## WIN-1 — the startup spill — **THE BLOCKER, nothing else matters first**

Code links in; **data does not**. Three things must be carried inside satl as a
GResource and written to a writable directory before `gtk_init()`:

1. **xkeyboard-config** — `gdk/wayland/gdkkeymap-wayland.c:478-486` runs at seat
   creation and null-checks nothing, so a machine without this data gets a
   **SIGSEGV inside `gtk_init()`**, before any window exists and before anything
   is printed. libxkbcommon reads files and only files: there is no API to hand it
   bytes and no meson option that embeds them. Minimum closure for
   `evdev/pc105/us` is **34 files, 348,215 bytes**, and a *partial* tree fails
   exactly like no tree. Reproduced independently here: the build compiles in
   `xkb-config-root=/nonexistent/satl-must-provide-xkb-data` on purpose, and the
   binary dies on a machine that HAS xkeyboard-config installed.
2. **A font.** Measured, and it corrects an earlier claim that this was cosmetic:
   **no font is FATAL, not degraded** — exit 139 twice, exit 0 twice with fonts
   present, after `GtkImage reported baselines of minimum -2147483648` and
   `g_object_ref: assertion 'G_IS_OBJECT (object)' failed`.
   `vendor/fonts/ibm-plex-mono/IBMPlexMono-Regular.ttf` is in git for this, 133 KB.
   GTK's own way to register bytes is `gsk/gskrendernodeparser.c:1186`: spill to a
   temp file, `FcConfigAppFontAddFile` on a private `FcConfigCreate()`, then
   `pango_fc_font_map_set_config`. There is no `FcConfigAppFontAddMemory`.
3. **GSettings schemas.** `g_settings_new()` on a missing schema calls
   `g_error()`, which is fatal and cannot be caught. Three unguarded sites in GTK,
   and one is the emoji chooser — **in the default right-click menu of every
   editable text widget** (`gtk/gtktext.c:6348`). A bare window is fine; any satl
   window with a text field aborts on right-click. Four schemas, ~8 KB compiled.
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
possible, and the API exists — checked in the vendored source:

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
call above must happen on the thread owning the `GMainContext`. So thread2 is:
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

None exist: 0 rows matching "window" in `words/words_004.tsv`. To mint:
`satellite.variable.window` (the type — **arm 15**; 13 is the float, 14 hex),
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

**Decision the author owes:** he wrote `my_window.title(12px)` AND *"leave it
unchangeable at 11px"*. Which?

## WIN-4 — `800x600` does not lex

`satl` reads `x600` as a hex literal (`xFFAA` is hex), so `800x600` is two numbers
side by side and is refused S110 (checked). Same for `400x300`. Options:
`new("title", 800, 600)`; a new NxM token; or a string `"800x600"`. **The author's
call** — it changes the syntax he wrote.

## WIN-5 — the cairo software renderer

`GSK_RENDERER=cairo` currently exits 1 with `Error 0 (Success) dispatching to
Wayland display`. It matters for a target with no GL driver at all, and
`gsk/gskrenderer.c:803` is `g_assert_not_reached()` if every renderer fails to
realize — so the fallback is load-bearing, not tidy.

## WIN-6 — linking GTK into satl, and the widget-`.so` question

satl is built by **make**, not meson (`make_support/048-link.mk`), so a make rule
must name the archives the way `vendor/gtk/hello/build.sh` does — CFLAGS from
pkg-config, libraries gathered as FILES, `--start-group` because the graph has
cycles, and four archives excluded by name (`libmalloc-stats.a`,
`libcairo-trace.a`, `libcairo-fdr.a`, `libdemo.a`, `libintl.a`) because they
define `malloc`/`realloc`, interpose `cairo_*`, or collide with glibc's gettext.

**The unresolved tension.** 004 dlopens each word's library from
`satellite-numbers/`. With GTK statically inside satl, widget words as separate
`.so` files would each need GTK's symbols back out of satl — which means linking
satl with `--export-dynamic`, or linking the widget words in directly. The
author's *"tiny C++ executables"* for widgets pulls against *"one executable"*.
**Decide before minting WIN-3, not after.**

Also live: 004 links dynamically on purpose so satl and its `.so` words share one
libstdc++. GTK is C and does not touch that argument; `-static` for the whole
binary would — and WIN-1's measurement says a fully static satl cannot draw
anyway.

## WIN-7 — finish the purity audit

`libffi.so.8` still comes from the system although libffi builds as a subproject.
Harmless, but it means the audit is not clean, and the audit is the only thing
standing between "static" and "mostly static". The method that found the others:
grep the configure log for `Run-time dependency ... found: YES`, which is meson
saying *from the system*.

## WIN-8 — the third-party notices

satellite is **MIT**; the stack is not one licence. LGPL-2.1-or-later (GTK, glib,
gdk-pixbuf), LGPL-2.0-or-later (pango), LGPL-2.1-or-MPL-1.1 (cairo), FTL-or-GPLv2
(freetype), MIT/BSD (harfbuzz, pixman, graphene, libepoxy, libxkbcommon, expat,
libffi, libjpeg-turbo, libtiff, fontconfig), libpng, zlib, BSD-3 (pcre2),
**Apache-2.0** (PCG, in 003 only — 004 has `satellite.random` numbered at `1 7`
and no implementation), **OFL-1.1** (IBM Plex Mono, now embedded).

**LGPL §6 is the one with teeth, and static linking is what gives it teeth:** the
recipient must be able to **relink against their own modified GTK**. Shipping
satellite's source plus `vendor/gtk/`'s four scripts satisfies it today. It
becomes a live constraint the moment `satellite_enterprise/` ships a static binary
*without* source.

**Two choices the author owes** before `THIRD-PARTY-NOTICES.md` can be generated:
freetype (**FTL** recommended — GPLv2 would infect) and cairo (**LGPL-2.1**
recommended, same as GTK). FTL wants this line in the documentation: *"Portions of
this software are copyright © The FreeType Project (www.freetype.org). All rights
reserved."*

## WIN-9 — force the satl-term console, or not — **THE AUTHOR'S, STILL UNANSWERED**

He asked it on 2026-09-19 and has not ruled. The recommendation given was: **do
not force it.** Static removes the "satl would not start where GTK is missing"
objection, but not the other two — a detached window still loses the program's
**exit status** and its **stdout**, and both are load-bearing here
(`machine/exit_status.hpp`; check.sh asserts 44 at line 1237; DESIGN §8 spends
four bullets getting pipes right). 004 already removed 003's handover on purpose
(PLAN.md:147, and the branch is named `...-no-console-handover`).

Recommended shape: `satl` never opens a window on its own; the `.desktop` names
the window (`Exec=satl-term %f`, already true); `satl --window` for a person at a
console who wants one; and a console program calling `satellite.window.new()`
gets a window without satl becoming a terminal emulator — which is WIN-2's
on-demand warm. Forcing the window would give such a program **two** windows.

## WIN-10 — X11, and machines that are not Wayland

The build is `-Dx11-backend=false` on purpose: the X11 backend needs libX11,
libXext, libXi, libXcursor, libXdamage, libXfixes, libXinerama and libXrandr, and
**none of them has a wrap**, so every one would link as a system `.so` and quietly
un-static the binary. libepoxy's own `glx_static` test proves the point by failing
the build: `attempted static link of dynamic object /usr/lib64/libX11.so`.

An X11 satl is a separate decision with its own vendoring, not a flag. Until then
satl draws on Wayland only.

---

# Part 3 — what was NOT done

- **No window word exists.** Nothing in `satellite/` links GTK yet. What exists is
  `vendor/gtk/` (the static stack, proved) and `vendor/fonts/` (the font).
- **WIN-1's spill is designed and not built** — so the binary still depends on the
  target having xkeyboard-config and a font.
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
