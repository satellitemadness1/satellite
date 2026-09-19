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

## What is still unproven, and must be before the design is trusted

- **A static GTK program needs its DATA as well as its code**: GSettings schemas
  (`org.gtk.gtk4.Settings.*`), the Adwaita icon theme, gdk-pixbuf loaders and GIO
  modules are found through `XDG_DATA_DIRS` and `dlopen` at run time, not linked. GTK's
  own CSS and icons are already inside it as GResource; the schemas are not. **The
  experiment that settles it:** build one static GTK4 hello-world from this tree and run
  it with `env -i` on a machine with no GTK installed. Until that runs, "unified
  executable" is a plan, not a fact.
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
