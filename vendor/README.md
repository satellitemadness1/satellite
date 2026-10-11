# vendor/ — other people's source, so satl can carry what it needs

The author, 2026-09-19: *"let's create another folder, /satellite/vendor/gtk/* so that
we can statically build GTK into the program, that would be super cool to do that for
enterprise linux, because nobody has gtk installed on their machines, you know? We
_must_ statically build this application, and that way we can have a unified
executable"*.

**What is here:** `gtk/` — GTK **4.16.7**, the same version this machine runs
(`gtk4-4.16.7-4.el10`), fetched and checked by `gtk/fetch.sh` from download.gnome.org
against upstream's own sha256sum. The unpacked tree is 201 MB and **is not in git**:
the repository already keeps that rule for a tree with its own history elsewhere
(`.gitignore`, `old_versions/first_satellite/`). `fetch.sh` and this file are, so the
tree is two lines away on any machine.

**The licence.** satellite is MIT; GTK is LGPL-2.1-or-later. Linking it statically is
allowed and carries one obligation: whoever gets the binary must be able to relink it
against their own GTK. Shipping this folder's source and the build scripts satisfies
it — which is what vendoring is anyway.

---

## What was true when this folder was made (2026-09-19, checked, not assumed)

**GTK already builds a static library of itself.** `gtk/gtk-4.16.7/gtk/meson.build`:

    1116  libgtk_static = static_library('gtk', ...)
    1124  libgtk = shared_library('gtk-4', ..., link_whole: [libgtk_static, ...], install: true)

The shared library everyone installs is made *out of* the static one, and GTK's own
testsuite links `libgtk_static_dep`. So there is a real `.a` to link — it is simply
never installed, which is why a machine with `gtk4-devel` has only `.so` files
(checked: `/usr/lib64` has `libgtk-4.so`, `libglib-2.0.so`, `libvte-2.91-gtk4.so` and
no `.a` for any of them). **Building from source is what makes a static satl possible.**

**GTK ships meson wraps for its whole stack** (`gtk-4.16.7/subprojects/*.wrap`): glib,
cairo, pango, harfbuzz, fribidi, graphene, gdk-pixbuf, libepoxy, libpng, libjpeg-turbo,
libtiff. `meson setup --default-library=static --wrap-mode=forcefallback` builds all of
them from source. Two catches:

1. **They are `[wrap-git]`, not tarballs** — a build would clone gitlab.gnome.org, so it
   needs the network and is not reproducible on its own. Each one we depend on should be
   vendored here beside GTK, with its own `fetch.sh`, and a local `subprojects/<name>`
   directory (meson prefers a directory that is already there over a wrap).
2. **The pinned revisions are older than this machine's** — glib.wrap says 2.76.0 where
   AlmaLinux 10 runs 2.80.4. Pin what we vendor to what we test against.

## The four scripts, and the order they run in

    sh vendor/gtk/fetch.sh             GTK 4.16.7 itself, checked against upstream's sha256sum
    sh vendor/gtk/fetch-deps.sh        the stack it is built on, pinned to what THIS machine runs
    sh vendor/gtk/configure-static.sh  meson, every option carrying the failure that earned it
    sh vendor/gtk/build-static.sh      ninja, with the venv first on PATH (this matters, see below)

    sh vendor/gtk/hello/build.sh       link the experiment against the static archives
    sh vendor/gtk/hello/bare-machine.sh ./hello-static   run it where no GTK exists

None of what they produce is in git — tarballs, unpacked trees, generated `.wrap` files,
`build-*/`, `.mesonvenv/`. The scripts are, so the whole stack is four commands away.

## GTK'S OWN WRAP SET DOES NOT BUILD STATICALLY. Nine configure rounds, 2026-09-19.

This is the finding, and it is worth stating plainly before the list: the wraps GTK
ships contradict each other and cannot produce a static build as distributed. Every fix
below is in `fetch-deps.sh` or `configure-static.sh` with the error that earned it
quoted beside it, so that none of them is later deleted as "probably unnecessary".

| # | what broke | where the fix lives |
|---|---|---|
| 1 | `Recursive include of subprojects: cairo => fontconfig => freetype2 => harfbuzz => freetype2` | `-Dfreetype2:harfbuzz=disabled` |
| 2 | `pango.wrap` pins `revision = main`, a moving branch, which now needs glib ≥ 2.88 while `glib.wrap` pins 2.76.0 | pinned tarballs |
| 3 | eight `[wrap-redirect]` files name `glib/subprojects/*.wrap`, resolved before anything is extracted | glib unpacked as a *directory* |
| 4 | a `[wrap-file]` with no `source_url` is looked for in `packagefiles/`, not `packagecache/` | `source_url` on each wrap |
| 5 | `python is missing modules: packaging` | venv |
| 6 | glib silently links the system `libmount` and `libselinux` | `-Dglib:{libmount,selinux}=disabled` |
| 7 | `libdrm` required — but only for one header, and never linked | generated headers-only `.pc` |
| 8 | deleting `glib.wrap` handed the name `glib-2.0` to *cairo's* wrap, which pins 2.74.0 | wrap **and** directory |
| 9 | `graphene-gobject-1.0` resolved from `/usr/lib64` because a hand-written `[provide]` named only `graphene-1.0` | `[provide]` copied from upstream |
| 10 | `attempted static link of dynamic object /usr/lib64/libX11.so` (libepoxy's `glx_static` test) | `-Dlibepoxy:{x11=false,glx=no,tests=false}` |
| 11 | `multiple definition of '_nl_msg_cat_cntr'` — forcefallback takes glib's "last chance" proxy-libintl over glibc's own gettext | the wrap is **removed** |

**Two of these were the compiler, not the source.** `CC` here is clang 24.0.0 built from
LLVM trunk (`make_support/010-compiler.mk` picks the same one for satl, so this is
consistency, not a mistake), and it emits warnings no released compiler has. pango
**1.54.0 — the current release** — dies on `-Wunused-but-set-global` over ordinary
`G_DEFINE_TYPE` boilerplate. A newer library version cannot fix that and never will.

`-Dc_args=-Wno-error` DOES NOT WORK, and the reason is worth keeping. Meson puts user
`c_args` *before* a subproject's `add_project_arguments`, and clang resolves a warning by
the **last** flag that names it. Measured, compiling one file that sets an unused static:

    -Werror=unused-but-set-variable                                     FAILS
    -Werror=unused-but-set-variable  -Wno-error=unused-but-set-variable  OK   (after)
    -Wno-error=unused-but-set-variable  -Werror=unused-but-set-variable FAILS (before)
    -Wno-unused-but-set-global  -Werror=unused-but-set-variable         FAILS (before)

Note the second line: plain `-Wno-error` does **not** cancel a specific
`-Werror=<name>`, only a blanket `-Werror`. Every flag we can pass lands in the losing
slot, so `configure-static.sh` generates a compiler wrapper that appends `-w` *after*
the caller's arguments — the only position that wins. Real errors still fail (checked).
harfbuzz needs its own hatch, `-DHB_NO_PRAGMA_GCC_DIAGNOSTIC_ERROR`, because it promotes
~30 warnings to errors with `#pragma GCC diagnostic error` inside `hb.hh`, which no
command-line flag can reach.

**And one was the shebang.** glib's `gdbus-codegen` starts `#!/usr/bin/env python3`, and
`python3` on this machine is PyPy, which has no `packaging`. meson found the right
interpreter at configure time and said so; the shebang re-resolves through `PATH` when
ninja runs it. Configuring in one shell and building in another is enough to break the
build, 2235 targets in. That is why `build-static.sh` exists rather than a bare `ninja`.

## THE TRAP THAT WOULD HAVE PROVED THE OPPOSITE

`gtk4.pc` — and the `gtk4-uninstalled.pc` meson writes beside a build — both describe
`libgtk_dep`, which is the **shared** `libgtk-4.so` (`gtk-4.16.7/meson.build:883`).
Checked in a build configured `--default-library=static`:

    $ pkg-config --libs gtk4
    -L.../build-static/gtk -lgtk-4 ...

A binary linked from that line passes every casual check and is not static. The static
library is `libgtk_static` (`gtk/meson.build:1116`), it is never installed, it is named
in no `.pc`, and on disk it is `build-static/gtk/libgtk.a` — a **thin** archive of 557
members, so it references its objects by path and cannot be copied elsewhere.
`hello/build.sh` therefore takes CFLAGS from pkg-config and gathers the libraries as
files, which is also the shape satl needs: satl is built by make, not meson.

## WHAT WAS MEASURED, 2026-09-19 — it works, with a named residue

A GTK4 hello-world (`hello/hello-static.c`) built from this tree, run under `bwrap` in
an empty root containing nothing but the binary, the GL driver, the Wayland socket and
glibc:

    window presented
    clean exit
    --- exit status 0 ---

No GTK, no glib, no cairo, no pango, no GSettings schemas, no icon theme, no MIME
database, no gdk-pixbuf loaders, no GIO modules, no D-Bus. `hello/bare-machine.sh` is
that test; it binds only the libraries `ldd` names, individually, so "a machine with no
GTK" is not a machine that quietly still had GTK on it.

**A FULLY STATIC BINARY IS POSSIBLE AND IS NOT WHAT YOU WANT.** `-static` links clean —
`ldd` says *not a dynamic executable*, 21 MB stripped — and then **cannot render**. Not
for the reason everyone expects (static glibc vs `dlopen`); the real cause is duplicate
library state:

    #0  wl_list_insert ()          from /opt/amdgpu/lib64/libwayland-client.so.0
    #1  wl_proxy_create_wrapper () from /opt/amdgpu/lib64/libwayland-client.so.0
    #2  dri2_initialize_wayland () from /opt/amdgpu/lib64/libEGL_mesa.so.0
    #5  gdk_display_init_egl       at gdk/gdkdisplay.c:1836

Statically linked, libwayland-client exists **twice** in the process: our copy, and the
one the GPU driver dlopens. GDK makes its `wl_display` with ours and hands the pointer
to EGL, which passes it to its own copy, whose lists were never initialised for that
object. It SIGSEGVs in the dynamic build too, which is how we know it is not the glibc
hazard. **The rule: a library whose objects cross into a dlopened driver cannot be
static.** `fetch-deps.sh` therefore removes `wayland.wrap`.

So the shape that works is *everything except glibc, libwayland and the GL driver*:

| carried inside the binary | must exist on the target |
|---|---|
| GTK, GSK, GDK | glibc (every Linux has it) |
| glib, gobject, gio | libwayland-client, libwayland-egl |
| cairo, pango, harfbuzz, freetype, fontconfig, pixman | libEGL + the vendor driver |
| gdk-pixbuf + png/jpeg/gif/tiff loaders | **xkeyboard-config data** |
| graphene, libepoxy, **libxkbcommon** | **at least one font** |

`ldd` on the working binary, in full: `libm`, `libresolv`, `libc`, `ld-linux`,
`libwayland-client`, `libwayland-egl`, `libffi`. Nothing else. Every item in the right
column is present on any machine that can display a window at all.

**CORRECTION, measured against a claim made during the investigation: NO FONTS IS
FATAL, NOT DEGRADED.** The expectation was tofu. What actually happens, reproduced twice
each way on a bare root with xkeyboard-config present:

    no fonts,   run 1: exit 139      with fonts, run 1: exit 0
    no fonts,   run 2: exit 139      with fonts, run 2: exit 0

and before the crash, `GtkImage reported baselines of minimum -2147483648` (INT_MIN — a
failed measurement) followed by `g_object_ref: assertion 'G_IS_OBJECT (object)' failed`.
A font is not a nicety for this binary; it is a hard requirement, so satl must embed one
and register it (`gsk/gskrendernodeparser.c:1186` shows GTK's own way: spill to a temp
file, `FcConfigAppFontAddFile` on a private `FcConfigCreate()`, then
`pango_fc_font_map_set_config`).

## Still open

- **`GSK_RENDERER=cairo`**, the software fallback, has not been made to work: it exits 1
  with `Error 0 (Success) dispatching to Wayland display`. It matters for a target with
  no GL driver at all, and `gsk/gskrenderer.c:803` is `g_assert_not_reached()` if every
  renderer fails to realize, so the fallback is load-bearing rather than tidy.
- **The startup spill is designed but not built** — the xkb tree, the schema blob and the
  font all have to be embedded as a GResource and written to a writable directory before
  `gtk_init()`. Until then the binary depends on the target having xkeyboard-config.
- **`libffi.so.8` still leaks in** from the system although libffi is built as a
  subproject; harmless, but it means the audit is not yet clean.

## CODE LINKS IN; DATA DOES NOT. What a static binary still hunts for at run time.

Checked against this tree, not assumed.

**Fine, nothing to do** — theme CSS and 153 icons are inside GTK's own GResource
(`gtk/gtkcssprovider.c:1591`, `gtk/gtkicontheme.c:1796`); a missing
`/usr/lib64/gio/modules` is silent; **no D-Bus session bus is needed**; fontconfig
self-heals with no `/etc/fonts` (`fcinit.c:43`); GTK decodes its own PNGs through GDK,
not gdk-pixbuf, so no librsvg.

**Must be carried and spilled to a writable directory before `gtk_init()`:**

- **xkeyboard-config — the hard one.** `gdk/wayland/gdkkeymap-wayland.c:478-486` runs at
  seat creation and null-checks nothing, so a machine without this data gets a
  **SIGSEGV inside `gtk_init()`**, before any window exists and before anything is
  printed. libxkbcommon reads files and only files: there is no API to hand it bytes and
  no meson option that embeds them. Minimum closure for `evdev/pc105/us` is **34 files,
  348215 bytes**; a *partial* tree fails exactly like no tree. `hello/hello-static.c`
  probes for it before `gtk_init` so the failure is a printed line instead of a death.
- **GSettings schemas.** `g_settings_new()` on a missing schema calls `g_error()`, which
  is fatal and cannot be caught. Three unguarded sites in GTK, and one is the emoji
  chooser — which is in the **default right-click menu of every editable text widget**
  (`gtk/gtktext.c:6348`), so any satl window with a text field aborts on right-click.
  Four schemas, ~8 KB compiled; `GSETTINGS_SCHEMA_DIR` prepends, so it cannot regress a
  machine that has them.

**`-Dgdk-pixbuf:gio_sniffing=false` is what makes `builtin_loaders=all` actually work.**
Left at its default, gdk-pixbuf picks a loader only via `g_content_type_guess()` — the
system MIME database, through `XDG_DATA_DIRS` — so on a bare machine the loaders
compiled *into* the binary are unreachable. `shared-mime-info` has no wrap, so the build
resolves it from the system and succeeds in silence.

**The honest residue — what the target machine must still provide:** a Wayland
compositor socket; a writable temp directory; `libEGL.so.1` and the vendor driver chain
*if* hardware rendering is wanted; and **at least one font, which is FATAL to be
without, not degraded** — exit 139 twice, exit 0 twice with fonts bound, as the
CORRECTION above this paragraph already says. (This sentence used to say "tofu
(degraded, not fatal)", 55 lines below the measurement that disproves it. The
measurement wins. SATELLITE_WINDOW.md WIN-1 also records that the MECHANISM is not
understood: the experiment builds no GtkImage, so the icon path is a live suspect and
"embed a font" may not be the whole fix.)

- **A unified executable and 004's "every word is its own .so" pull against each other.**
  satl loads each word's library at start-up (`satellite-numbers/`), and a dlopened `.so`
  is the opposite of one file. Either the window words are linked INTO satl (and satl
  carries GTK even for programs that draw nothing), or the GUI lives in one `.so` that
  has GTK statically inside it (one satl, one library — not one file). This is the
  author's call and it is not made yet.
- **satl and its libraries must share one libstdc++** (`make_support/048-link.mk`), which
  is why 004 links dynamically today. GTK is C, so it does not touch that argument — but
  `-static` for the whole binary would, and that is the distinction to keep straight:
  *GTK inside satl* is not the same request as *a fully static satl*.
