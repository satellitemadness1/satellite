# satellite-004 — GTK AND NO DEPENDENCIES

**One executable that needs nothing installed, built from a folder that needs
nothing downloaded.** Written 2026-09-20 at the author's asking, diverging from
SATELLITE_WINDOW.md's plan because the subject turned out to be its own.

**TWO FAMILIES OF MILESTONE LIVE HERE**, one per half of the title.
`DEP-n` is *no dependencies* — what the window **costs**. `GTK-n` is *GTK* —
**every widget**, and how one is added (Part 2G, written 2026-09-21 at the
author's asking). SATELLITE_WINDOW.md keeps `WIN-n` and is still the record for
the **window itself**: the one GTK thread, the carried data, the press queue.

**Part 00 is the chart**: which milestone calls which vendor library, and which
of the twenty-four vendored projects no satellite word has ever reached.

The author, 2026-09-20, in his own words:

> *"we need no dependencies, if you need something, include a copy of it so it
> exists inside of this project folder, /vendor/library/something.whatever_extension,
> so that we can build a completely static executable"*

> *"we don't care how big the executable is — 100mb transfers across the
> internet in... 2-3 seconds"*, and the ceiling he then worked out:
> **5 × 60 × 40 = 12,000 MB.** *"12 gigabytes, we are okay"*.

---

# Part 00 — THE CHART: which milestone calls which vendor library

Written 2026-09-21, the author asking for it in these words: *"we need a chart of
what milestones call which vendor library, at the top of that document"*.

**TWO KINDS OF CALLING, AND THE CHART KEEPS THEM APART**, because they cost
completely different things:

- **CALLS** — satellite's own C++ names a function in that library. Changing the
  library can break satellite's own source.
- **reaches** — GTK does it underneath, because satellite asked GTK for
  something. Nothing of ours names it; it is in the binary because the thing
  above it needs it.

Today satellite's own code calls **six** of the twenty-four projects by name:
gtk, glib, gobject, gio — gio for the GResource that WIN-1 carries, for
GTK-11's `GAsyncResult`, and since GTK-12 for a menu's `GMenu` and
`GSimpleActionGroup`, which is the one milestone that is gio before it is gtk —
and, since GTK-15 on 2026-09-22, **cairo and pango**: cairo for a canvas's
lines, boxes and circles and for the PNG `.save` writes, pango (pangocairo) for
the words on it. Everything else in the list below is reached. **That is the
number the widget milestones move**, and the last table says by how much.

## The stack, as it is actually built

**29 projects, 37 archives** (24 and 33 until 2026-09-22, when VTE and the
four it needs went in; 43 files end in `.a` if libpng's two names, pixman's
three SIMD halves and the rest of the manifest's extras are each counted), `vendor/stage/BUILD_MANIFEST.txt` is the authority and
is written by the build rather than by hand.

    gtk 4.24.0        glib 2.90.0       pango 1.58.2      cairo 1.18.4
    harfbuzz 14.4.0   freetype 2.14.3   fontconfig 2.18.3 fribidi 1.0.17
    gdk-pixbuf 2.44.8 libpng 1.6.58     libjpeg-turbo 3.2.0  libtiff 4.7.2
    zlib 1.3.2        pixman 0.46.4     graphene 1.10.8   libepoxy 1.5.10
    libxkbcommon 1.13.2   xkeyboard-config 2.48   expat 2.8.4   pcre2 10.48
    libffi 3.8.0      wayland-protocols 1.49   gperf 3.3   meson 1.12.0
    vte 0.84.1        lz4 1.10.0        simdutf 8.2.0     fmt 12.1.0    fast_float 8.1.0

`xkeyboard-config`, `wayland-protocols`, `gperf`, `meson` and `fast_float` link
nothing — the first two are **data and XML**, the next two are **build tools**,
the last is **headers only**. Twenty-four projects produce the archives.

## Table A — every milestone, and what it calls

`✔` is built. `◑` is part-built and says which part. `—` is not.

| | milestone | satellite's own code CALLS | reached underneath it |
|---|---|---|---|
| ✔ | **WIN-1** the carried data | glib, gio (GResource), zlib (the blob is compressed) | fontconfig, freetype, libxkbcommon + xkeyboard-config **data** |
| ✔ | **WIN-2** the one GTK thread | gtk (`gtk_init_check`), glib (`GMainContext`, `GMainLoop`) | gdk, gdk-wayland, gsk, graphene, libepoxy, libxkbcommon |
| ✔ | **WIN-3** window and button | gtk | pango + pangocairo + pangoft2, harfbuzz, fribidi, freetype, fontconfig, expat, cairo, pixman |
| ✔ | **WIN-11** a press | gtk, gobject (`g_signal_connect`, `g_signal_emit_by_name`) | libffi — the closure marshaller is libffi's |
| ✔ | **GTK-1** a label | gtk | the whole pango stack, as a button's label already does |
| ✔ | **GTK-2** a person types | gtk | pango |
| ✔ | **GTK-3** on and off | gtk (and `gtk_check_button_set_group` for the radio, 2026-09-22) | — |
| ✔ | **GTK-4** a number chosen | gtk | pango (the number is drawn as text) |
| ✔ | **GTK-5** a list to choose from | gtk, gobject (`GtkStringList` is a GListModel) | — |
| ✔ | **GTK-6** a picture | gtk, **gdk-pixbuf** | **libpng, libjpeg-turbo, libtiff**, zlib, gtk_svg |
| ✔ | **GTK-7** rows, columns, a grid | gtk | — |
| ✔ | **GTK-8** the window itself | gtk, gdk | gdk-wayland |
| ✔ | **GTK-9** every piece talks back | gtk, gobject | libffi |
| ✔ | **GTK-10** the look | gtk (`GtkCssProvider`), gtk_css | pango, fontconfig, freetype |
| ✔ | **GTK-11** asking a person | gtk (`GtkAlertDialog`, `GtkFileDialog`), gio (`GAsyncResult`, `GFile` since the file dialog, 2026-09-22) — and the portal is turned off before any of it (Q-WIN-11a) | — |
| ✔ | **GTK-12** a menu | **gio** (`GMenu`, `GSimpleAction`, `GSimpleActionGroup`, and since 2026-09-22 `g_menu_append_section` for a separator and a submenu link for a menu inside a menu), gtk (`GtkPopoverMenuBar`, `gtk_widget_insert_action_group`) | — |
| ✔ | **GTK-13** time | **glib alone** (`g_timeout_add`) — no gtk call at all | — |
| ✔ | **GTK-14** the keyboard and the mouse | gtk | **libxkbcommon + xkeyboard-config**, this time for satellite and not for GTK |
| ✔ | **GTK-15** a canvas | gtk (`GtkDrawingArea`), **cairo directly** (a line, a box, a circle, an arc and a slice, an outline, a pen width, an image surface, `cairo_surface_write_to_png`), **pango directly** (`pango_cairo_show_layout`) | pixman, freetype, **libpng** for `.save` |
| ✔ | **GTK-16** more than one screenful | gtk (`GtkNotebook` since 2026-09-22) | — |
| ✔ | **GTK-17** `satellite.console` is a window — **BUILT 2026-09-22**, and `satl --console` with it | **VTE** (`VteTerminal`, `VtePty`), gtk, pango | lz4, simdutf, freetype, harfbuzz, fribidi |
| — | **GTK-18** `satellite.terminal` is a bash prompt | VTE, glib (`g_spawn`) | — |
| ✔ | **DEP-1** every source in the folder — and **PROVED ON A FRESH CLONE WITH NO NETWORK, 2026-09-22**: 24 of 24 built in 216 s, `make` linked the same eight, check.sh green | **all 24** | — |
| — | **DEP-2** the word libraries link in | none — it is a link shape, not a call | — |
| ✔ | **DEP-3** prove it on a bare machine — **PROVED 2026-09-22**: in an empty root, a window, two capsules, the carried font in a PNG byte-identical to this machine's; and again with **no GPU driver at all** | none | all of them, which is the point |
| ✔ | **DEP-4** the eight that remain — **MEASURED 2026-09-22: SEVEN is the floor.** libresolv bound nothing and was a `-lresolv` on the link line; gone | none — libwayland is the **machine's** | — |
| — | **DEP-5** one application | VTE | — |
| — | **DEP-6** the notices | the licence of every one of the 24 | — |
| — | **DEP-7** the lower layer's versions | zlib, libpng, freetype, harfbuzz | — |
| — | **DEP-8** the distribute package | none | — |
| — | **DEP-9** other machines | the nine X libraries, **none of them vendored** | — |

## Table B — every vendored project, and the first milestone that needs it

The column that matters is the last one. **Ten of the twenty archive-producing
projects are in satl today and no satellite word has ever reached them.**

| project | archives | what it is there for | first milestone that CALLS it |
|---|---|---|---|
| **gtk** | libgtk, libgdk, libgdk-wayland, libgsk, libgtk_css, libgtk_svg | every widget | WIN-2 ✔ |
| **glib** | libglib-2.0, libgobject-2.0, libgio-2.0, libgmodule-2.0, libgthread-2.0 | the object system, the main loop, the resource | WIN-1 ✔ |
| **pango** | libpango-1.0, libpangocairo-1.0, libpangoft2-1.0 | laying text out | reached at WIN-3; **CALLED at GTK-15 ✔** — `pango_cairo_show_layout` draws the words on a canvas |
| **cairo** | libcairo, libcairo-gobject | drawing | reached at WIN-3; **CALLED at GTK-15 ✔** (2026-09-22) |
| **pixman** | libpixman-1 (+ mmx, sse2, ssse3) | cairo's rasteriser | reached only |
| **harfbuzz** | libharfbuzz, libharfbuzz-subset | shaping a run of characters into glyphs | reached only |
| **freetype** | libfreetype | a glyph out of a `.ttf` | reached only |
| **fontconfig** | libfontconfig | finding IBM Plex Mono in the spill | reached at WIN-1 |
| **fribidi** | libfribidi | right-to-left text | reached only |
| **expat** | libexpat | fontconfig's XML parser, on our own `fonts.conf` | reached at WIN-1 |
| **gdk-pixbuf** | libgdk_pixbuf-2.0 + 13 loaders | a picture off the disk | GTK-6 ✔ |
| **libpng** | libpng16 | `.png` | GTK-6 ✔ — proved with a real 64×48 PNG; and GTK-15's `.save` **writes** one |
| **libjpeg-turbo** | libjpeg | `.jpg` | GTK-6 ✔ — proved with a real JPEG |
| **libtiff** | libtiff | `.tif` | GTK-6 — same word, not yet proved with a `.tif` |
| **zlib** | libz | the compressed GResource; png | WIN-1 ✔ |
| **graphene** | libgraphene-1.0 | the render node maths | reached only |
| **libepoxy** | libepoxy | GL entry points for GSK | reached only |
| **libxkbcommon** | libxkbcommon | a Wayland keymap into keysyms | reached at WIN-2; **CALLED at GTK-14 ✔** |
| **xkeyboard-config** | **data, 293 files** | what libxkbcommon reads | carried since WIN-1 ✔ |
| **libffi** | libffi | gobject's generic closure marshaller | reached at WIN-11 |
| **pcre2** | libpcre2-8 | glib's `GRegex` | **nothing here has ever needed one** |
| **wayland-protocols** | **XML, build time** | the protocol gdk-wayland is generated from | DEP-1 ✔ |
| **gperf** | **a build tool** | fontconfig's perfect hashes | DEP-1 ✔ |
| **meson** | **a build tool** | builds the other 23 | DEP-1 ✔ |
| *(also linked)* | libgirepository-2.0, libcairo-script-interpreter | introspection; a cairo trace replayer | **dead weight — nothing calls either** |
| **VTE** | libvte-2.91-gtk4 — **vendored 2026-09-22**, static by a one-word journalled patch | a terminal in a window | GTK-17 ✔ — `satellite.console.new` and `satl --console` link it since the same evening; NEEDED stayed seven |
| **lz4** | liblz4 | VTE's scrollback compression | GTK-17 |
| **simdutf** | libsimdutf | VTE's UTF-8 validation and transcoding | GTK-17 |
| **fmt** | libfmt (installed; VTE compiles it header-only) | VTE's formatting | GTK-17 |
| **fast_float** | **headers only**, our own `.pc` | VTE's number parsing | GTK-17 |

## What the chart is FOR, and the three things it already says

1. ~~**`vendor/new/` is missing one tarball.**~~ **CLOSED 2026-09-22.** It was
   VTE, and VTE needed four more (lz4, simdutf, fmt, fast_float): five tarballs
   in `vendor/new/`, five recipes, one patch, 45 seconds to build, and a program
   linked from the archive that spawned a shell in a terminal on a headless
   compositor with satl's seven NEEDED. **GTK-17 was built the same evening**;
   GTK-18 can start; the one question the vendoring raised is Q-VTE-1, under
   GTK-17, still the author's and reset off the screen meanwhile.
2. **libpng, libjpeg-turbo, libtiff and gdk-pixbuf are 2.6 MB of satl that no
   satellite program can reach.** GTK-6 is what earns them. Until it lands they
   are carried for GTK's icon loading and nothing else.
3. **pcre2, libgirepository and libcairo-script-interpreter are called by
   nobody at all**, and no milestone below ever will. They are in the link
   because the archive sweep in `047-window.mk` takes every `.a` it finds.
   **Not a bug** — `--start-group` drops what nothing references — but worth
   saying once so nobody goes looking for the word that uses them.

---

# Part 0 — how to get back to where this was written

**REWRITTEN 2026-09-21: DEP-1 landed, and this section's whole premise is gone.**
It used to say "that is the whole recipe *on this machine*, because vendor/gtk is
already built here", and pointed anywhere else at an hour and 2.2 GB. Two commands
now, on any machine:

    /usr/bin/python3 vendor/build_stack.py      24 projects into vendor/stage, ~3 min
    make                                        satl, with GTK carried inside it

`vendor/gtk-old` — the GTK 4.16.7 tree that made the first proof — **has been
deleted** (3.0 GB reclaimed, 2026-09-21). Its seven scripts are still in git at
`vendor/gtk-old/*.sh` and `vendor/gtk-old/hello/`, including the `bare-machine.sh`
that DEP-3 grew out of — its successor, for the real satl, is
`satellite/satellite_variable_window/prove-bare-machine.sh` (2026-09-22); only the
build output, the source trees and the tarballs went.
Every measurement taken from it is in Part 1 and does not need it back.

    make                53 MB   GTK compiled in            <- the default since 2026-09-21
    make GTK=system    1.1 MB   GTK loaded from the machine at run time

Both write `build/satl`, and the link prints which kind it made. The author's
`satl` alias runs `build/satellite-004 -> satl`, so **whichever was built last is
what he gets** — that cost a round trip on 2026-09-20 and the printed line is the
fix.

To see a window without touching the author's desktop, use a headless
compositor. **`env -u WAYLAND_DISPLAY` IS NOT ENOUGH** — libwayland falls back to
`$XDG_RUNTIME_DIR/wayland-0` and reaches his real session. A window opened on his
screen this way once.

    mutter --headless --virtual-monitor 1280x800 --wayland-display=satlwin &
    env -u DISPLAY WAYLAND_DISPLAY=satlwin XDG_RUNTIME_DIR=/run/user/1000 \
        ./build/satl examples/window.satl
    # genuinely headless, for the refusal path:
    env -u DISPLAY -u WAYLAND_DISPLAY -u XAUTHORITY XDG_RUNTIME_DIR=<empty dir> \
        ./build/satl examples/window.satl        # S730 NO_DISPLAY, exit 50

Kill mutter **by PID**. `pkill -f` matches the pattern inside your own heredoc
and kills the shell running it (exit 144, and the rest of the command silently
never runs).

---

# Part 1 — what is MEASURED, so nobody measures it twice

All of it run on 2026-09-20, commits `94c33dd`, `9ce5d73`, `1f36e95`.

## It works today

`make GTK=vendor` produces a satl that opens a window on a real Wayland
compositor with **no GTK, glib, gio, cairo, pango, harfbuzz, gdk-pixbuf,
graphene or vulkan loaded from the machine**. 136 gio symbols are compiled in.
check.sh **351 passed, 0 failed, twice**. A rebuild is **5.67 seconds** — GTK
itself was built once, on 2026-09-19, and is not rebuilt.

## The eight it still needs, and why each — **SEVEN since 2026-09-22**

    libc  libm  ld-linux  libwayland-client  libwayland-egl
    libstdc++  libgcc_s              (libresolv was the eighth, and bound nothing: DEP-4)

`make_support/050-build.mk` holds this as `ALLOWED_NEEDED` and **fails the build
and deletes the binary** if `readelf -d` ever names anything else.

- **libwayland-client, libwayland-egl — CANNOT be static, ever.** Two copies in
  one process segfaults: GDK makes its `wl_display` with ours and EGL calls
  `wl_list_insert` in the copy the GPU driver dlopened, which never saw the
  object. Measured in *both* link modes, so it is not the static-glibc hazard.
- **libstdc++, libgcc_s — removable, and DEP-2 is how.** See below.
- **libc, libm, ld-linux** — every Linux.
- **libresolv — GONE 2026-09-22 (DEP-4).** It was `-lresolv` on the link line
  and nothing more: satl bound no symbol to it (glibc 2.34 folded the resolver
  into `libc.so.6`, below satl's floor), `readelf -V` had no version-needs for
  it, and gio never asked for it. A fresh reader caught the first write-up
  calling it gio's. The flag is dropped and NEEDED is seven.

## The finding that cost the six-library target

`-static-libstdc++ -static-libgcc` **does** reach six. It links, and 68 test
programs come out byte-identical. Then check.sh's `/dev/full` row answers **0
where it must answer 2**.

Every library in `satellite-numbers/` has `NEEDED libstdc++.so.6`. satl carrying
its own copy gives the process **two `std::cout`**: `satellite.console.display`
writes through the shared one, satl checks the error state of its own, and a
**failed write is reported as a run that succeeded**.

`make_support/048-link.mk` predicted this in prose before any of it was built:
*"satl and every library in build/satellite-numbers/ must share one libstdc++, or
each has its own std::cout (DESIGN §3.4)"*.

**IT GENERALISES SATELLITE_WINDOW.md PART 1's RULE.** That read *"a library whose
objects cross into a dlopened DRIVER cannot be static"*. The boundary is not a
driver — it is **any dlopen**, and satl dlopens 62 of its own words:

> **A LIBRARY WHOSE STATE IS SHARED ACROSS A DLOPEN BOUNDARY CANNOT BE STATIC.**

**And the weak test is the lesson.** "68 programs give byte-identical output"
felt conclusive and exercised no failure path at all. One row that writes to a
full device caught it.

## Uninstalling the system GTK is not the way, and would not work

The author proposed it. Measured before declining:

- `dnf remove gtk4` takes **38 packages**, including `gnome-shell` and `mutter` —
  the session he is working in.
- **`glib2` cannot be removed at all**: 261 installed packages require
  `libglib-2.0.so.0`, NetworkManager among them. And glib2 *is* gio.
- So a satl built on such a machine would still link the system's gio, cairo,
  pango, fontconfig and schemas and **pass** — a false green, which is the exact
  failure mode being engineered away.

**What replaces it:** `PKG_CONFIG_PATH=vendor/gtk/build-static/meson-uninstalled`
returns **zero `-I/usr` paths**, so the build never consults the system; and the
`readelf -d` gate makes a regression fail the build on every machine and after
every `dnf update`, instead of once, here.

**Use `readelf -d`, never `ldd`** — ldd prints the whole transitive closure, which
is what made libffi look like a leak until WIN-7 corrected it.

## Distributions ship no static archives, so building from source is the only way

Zero `.a` files in **gtk4-devel, glib2-devel, cairo-devel, pango-devel,
gdk-pixbuf2-devel, harfbuzz-devel** between them, and no distro packages a
`*-static` for any of them. `dnf install` gives `.so` only, and a shared object
cannot be statically linked — the objects that made it are gone. This is not a
preference; there is no other path.

## What a fresh clone actually gets

    git ls-files          2116 files
    git ls-files vendor/    12 files   (4 scripts, 1 font, README)

`.gitignore:49` excludes `/vendor/*/*.tar.*`. **Not one dependency source is
committed**, and `configure-static.sh` reaches **three hosts** at configure time:
Meson's wrapdb, `gitlab.freedesktop.org`, `github.com`.

**`fribidi.wrap` has `revision = master`** — not a version, not even a tag. Two
builds a week apart link different code. The commits this machine's
`build-static` was actually made from, captured before they could drift:

    cairo      3909090108bb2db55330e3eb148aebe664735363   2023-09-23  (tag 1.18.0)
    harfbuzz   8d1b000a3edc90c12267b836b4ef3f81c0e53edc   2022-03-01  (tag 4.0.0)
    libepoxy   2df68f811fc1a5f0a6d372ecdb887333ad3f540f   2022-12-18  (full SHA, correct)
    fribidi    c928e4c77549de59e057f72bbbec1e38ce8b43bd   2026-08-27  (off master)

## Sizes, so the budget is never the question

| | |
|---|---|
| author's ceiling | **12,000 MB** |
| satl, GTK carried, unstripped | 87.3 MB — **0.7%** |
| satl, stripped | 21.0 MB |
| the six tarballs we fetch | 33 MB |
| meson packagecache | 26 MB |
| all 63 built archives | 117 MB |
| `vendor/xkb/xkb-data` | 2.8 MB |

**DO NOT STRIP.** 87 MB is 2.2 s at the author's 40 MB/s and 140 s on a slow
5 Mbps line — inside his 5 minutes either way. Stripping saves nothing that
matters and costs `?? ()` in every backtrace. Space is not a constraint on this
project, and proposing to save it reads as not having listened.

## `libgtk.a` is a THIN archive, and converts

557 members referenced **by path** into `build-static/`, so it cannot be copied or
shipped as-is. `ar crs real.a $(ar t gtk/libgtk.a)` from inside `build-static/`
produces a real 49 MB archive. Tested.

---

# Part 2 — the DEP milestones: what the window COSTS

## DEP-1 — every source inside the project folder — **DO THIS FIRST**

The author's rule, literally. A fresh clone must build `make GTK=vendor` with
**no network at all**.

**What goes in**, under `vendor/` per his spelling
(`/vendor/library/something.whatever_extension`):

1. The six tarballs already fetched (33 MB) — stop `.gitignore`-ing them.
2. Meson's `packagecache` (26 MB, 19 files) — expat, libjpeg-turbo, libpng,
   libtiff, libxml2, pcre2, zlib and their patch zips.
3. **Release tarballs for the four wrap-git subprojects** — cairo, harfbuzz,
   libepoxy, fribidi — pinned to the commits in Part 1, and their `.wrap` files
   rewritten from `wrap-git` to `wrap-file` so meson never clones.
4. `vendor/xkb/xkb-data` (2.8 MB) — staged today by `vendor/xkb/fetch.sh` from
   **this machine's** `/usr/share/X11/xkb`, which is provenance nobody should
   accept. Prefer the upstream xkeyboard-config 2.41 tarball with its sha256.

**`fribidi` is the one that must not wait.** Everything else is at a tag or a
SHA; fribidi is whatever `master` was that morning.

**Verify it, do not assume it:** configure and build in a sandbox with no
network (`unshare -rn`, or bwrap with no `--share-net`). A wrap that still
reaches out will simply fail there, which is the only honest test.

**VERIFIED 2026-09-22, exactly that way.** A fresh `git clone` of `3613af0` into
a scratch folder, every step inside `bwrap --unshare-net` (uid unchanged, so
`tar` never tries to chown), the sandbox first proving the network was gone
(`connect` to 1.1.1.1: *Network is unreachable*):

    /usr/bin/python3 vendor/build_stack.py    unpacked 26 files from vendor/new/, 24 of 24 ok, 216 s
    make -j16                                 satl in 15 s, "carries GTK -- needs only" the eight of that
                                              morning (seven since DEP-4, later the same day);
                                              62 word libraries built; satl-term too, because THIS
                                              machine has vte -- 047-window.mk skips it where it does not
    ./check.sh                                525 passed, 0 failed -- 526 here; the one row it did not
                                              run is the `skip` for old_versions/second_satellite/satl,
                                              a built 003 binary that is gitignored

The clone's `BUILD_MANIFEST.txt` names the same 24 versions and the same 26
hashes as this checkout's; the archives differ in size by a few hundred bytes
each, which is the longer build path the members carry. **1.7 GB on disk, built.**
So the gap Part 3 carried since 2026-09-21 — *"a fresh clone still cannot run
it"* — was closed by `1e5a960` four minutes after it was written, and is now
measured closed rather than believed closed.

**Cost:** ~85 MB committed once, 0.7% of the ceiling. It is also what makes the
author's 3–5 year freeze possible, and what LGPL §6 needs anyway (DEP-6).

## DEP-2 — the word libraries link IN, and NEEDED becomes six

This is the one that removes a dependency rather than vendoring it, and it is
**the author's call because it changes DESIGN §3.4.**

Today satl dlopens 62 `.so` files from `satellite-numbers/`, each with `NEEDED
libstdc++.so.6`. That is why `-static-libstdc++` cannot be used (Part 1). Link
them in and there is one libstdc++, it can be static, and NEEDED is six.

**What it costs, honestly:**

- DESIGN §3.4 and `satellite-numbers/build_libraries.py` are built around one
  `.so` per word — *"the name of the method will only exist in the directory
  structure of the files"*. That idea survives; the **loading** changes.
- `call_number.hpp` promises a name is never looked up while a program runs. A
  linked-in table keeps that promise more easily, not less.
- The author wanted widgets as *"tiny C++ executables"*. SATELLITE_WINDOW.md
  WIN-6 already ruled the word that answers a **handle** cannot be a `.so`. This
  extends that to all of them.
- **What is LOST: a word can no longer be added without relinking satl.** Today a
  new `.so` dropped into the folder is picked up. That is a real property and the
  author should say whether he wants it.

**A middle option exists** and should be measured before choosing: keep the
dlopen and give satl `-Wl,--export-dynamic` so the words bind to satl's copy.
Likely fragile — each `.so` still has its own `DT_NEEDED`, so the loader maps
libstdc++ anyway and interposition order decides. **Measure it with the
`/dev/full` row, which is the one that catches this.**

## DEP-3 — prove it on a machine with nothing — **PROVED 2026-09-22**

`vendor/gtk-old/hello/bare-machine.sh` did this for `hello` on 2026-09-19, with
`--with-xkb --with-fonts` bound in because WIN-1 had not been built. The plan
here said: point it at the real satl and drop those two. That is
**`satellite/satellite_variable_window/prove-bare-machine.sh`**, run by hand like
the other two compositor proofs, thirty seconds, four stages on one headless
mutter of its own (`satlbare`, so it can run beside `prove-canvas-tabs-menus.sh`
in another session):

    XDG_RUNTIME_DIR=/run/user/1000 sh satellite/satellite_variable_window/prove-bare-machine.sh
    KEEP_WORK=1 ...     keeps every stage's output and binds.txt, the list of what was bound

**THE ROOT IS EMPTY, and what goes into it is named one path at a time** so the
list IS the dependency list. Forty-four paths on this machine, every one printed:
eight for satl's own runtime — the seven NEEDED, resolved as a machine with no
`LD_LIBRARY_PATH` resolves them, plus `libffi.so.8`, which is libwayland-client's
own NEEDED — and thirty-six for the GPU driver, found the way the loader finds it
rather than by a list of AMD paths: glvnd's `egl_vendor.d` names the vendor
library by SONAME, `ldconfig` turns that into a path, `ldd` gives its closure,
and mesa's gallium/dri neighbours and `drirc.d` come with it. Plus `/dev/dri`,
`/sys`, `/etc/ld.so.cache`, and the Wayland socket alone in a fresh tmpfs
runtime directory, which is also where WIN-1's spill lands (5.3 MB — the byte
count is in the folder's name). **Nothing from `/usr/share` but the driver's
three data folders (`drirc.d`, `glvnd`, `libdrm`, and only in the driver
stage), no `/etc/fonts`, no xkb data, no schemas, no icon theme, no
`/etc/passwd`, no session bus, no accessibility bus, no network
(`--unshare-net`), and no `vendor/stage/`** — whose paths are compiled into
fontconfig, gdk-pixbuf, glib and GTK and exist on no other machine.

**The program** opens a 640×480 window, appends a label, a button and a canvas,
writes *"carried IBM Plex Mono"* on the canvas in that face at 18px, saves it to
`bare.png`, presses its own button (`.press()`, WIN-11), and the capsule reads
the window's title and closes it; the `.closed` capsule runs; exit 0. The same
program runs first **outside** any sandbox on the same compositor, and its PNG
is the reference.

**MEASURED, all four stages, first on `3613af0`'s binary and again on the
seven-NEEDED relink DEP-4 made the same afternoon:**

| stage | root | result |
|---|---|---|
| outside | this machine as it is | exit 0, six lines |
| **driver** | empty + the 44 paths | **exit 0, six lines, `bare.png` byte-identical to the reference**, EGL found through the driver, `GskGLRenderer` |
| **no_driver** | empty + the 8 runtime paths, no `/dev/dri`, no `/sys` | **exit 0, six lines, `bare.png` identical**; GTK: *"Not using GL: libEGL not available"*, `GskCairoRenderer` |
| no_display | the same, no socket | **exit 50, S730 NO_DISPLAY** — a refusal, not a crash |

**WHAT IT FOUND, beyond "it works":**

1. **The GPU driver is not on satl's floor.** With no EGL at all GTK falls back
   to its cairo renderer over `wl_shm`, and the program, its output and its PNG
   are the same. A machine whose graphics stack is missing costs the GL
   renderer and nothing else. So the floor is the eight runtime paths.
2. **The distribution's libstdc++ is enough, and the version floor is now a
   number.** satl asks for `GLIBCXX_3.4.32` — one symbol,
   `std::ios_base_library_init`, which gcc 13's `<iostream>` emits — and
   `GLIBC_2.38` — the `__isoc23_strtol` family, `strlcat`, `fmod`, which clang 24
   picks off glibc 2.39's headers. AlmaLinux 10 ships `GLIBCXX_3.4.33` and glibc
   2.39, so the distro's `/lib64` runs it. **AlmaLinux 9 cannot** (glibc 2.34,
   gcc 11's `GLIBCXX_3.4.29`): satl as built is an EL10-class binary. check.sh
   asserts both numbers; the day a change raises either, the row fails and
   somebody decides. And the proof runs `ldd` with `LD_LIBRARY_PATH` unset and
   **stops if any library resolves into `$HOME`** — with it set, this shell binds
   `~/opt/gcc-17`'s libstdc++ and would have called a compiler's runtime the
   machine's.
3. **One warning on stderr, and it is the accessibility layer asking for a
   bus.** `gtk/a11y/gtkatspicontext.c:1877` — *"Unable to acquire session
   bus"*: with no session bus the AT-SPI context cannot ask `org.a11y.Bus`
   where the accessibility bus is, GLib tries to autolaunch one and cannot
   *"without a machine-id"*, naming `vendor/stage/var/lib/dbus/machine-id` —
   glib's compiled-in localstatedir, a path inside this checkout printed on
   somebody else's machine — and GTK carries on. Every systemd Linux has
   `/etc/machine-id`, so the wording there differs and the outcome does not.
   **So Q-WIN-11c has its numbers: a bus with no registry costs three
   `Gtk-CRITICAL`s, no bus at all costs one warning, a real desktop costs
   nothing.** The reference stage prints none because `env -u
   DBUS_SESSION_BUS_ADDRESS` does not make a run bus-less — GLib falls back to
   the real `$XDG_RUNTIME_DIR/bus` (press-a-button.sh's trap 2) and that stage
   reached the user's real bus and registry, measured in its trace.
4. **What satl reaches for on a machine that has things, traced outside the
   sandbox, none of it needed:** `/usr/share/icons/Adwaita` for the three
   window-decoration icons (absent, GTK uses its own), `vendor/stage`'s
   `loaders.cache`, `gio/modules`, `immodules`, `media` and fontconfig's
   `conf.avail` (absent, nothing changes — the loaders are built in), the
   session bus, the accessibility bus, `/etc/passwd`, `/usr/share/locale`.
5. **Two libffi in one process, measured working.** satl carries libffi as
   `libffi.a` for gobject's closures; the machine's `libwayland-client.so.0`
   brings `/lib64/libffi.so.8`. No shared state between them, and the same
   shape as the machine's `libz.so.1` beside satl's static zlib.
6. **Every path the old script hardcoded is gone — for a stock mesa behind
   glvnd.** On such a machine the vendor library is `/usr/lib64/libEGL_mesa.so.0`
   and the same derivation binds its closure out of `/usr/lib64` one file at a
   time, never the folder, so DEP-9's *"on a non-AMD machine it needs editing"*
   no longer applies there. What it does not cover: a driver that dlopens
   libraries no `ldd` names (NVIDIA's `libnvidia-*`) is not in the closure —
   that stage then finds no EGL and the report **fails loudly** rather than
   passing driverless; an absolute `library_path` in the vendor file is
   honoured.

What it does not prove: that the window's *pixels* were right in the cairo
stage — the PNG is cairo's image surface, drawn by satl's own display list, not
the compositor's frame. That the window was mapped, the capsules ran and GTK
named its renderer is what is asserted.

## DEP-4 — the eight that remain, and whether it is really six — **MEASURED 2026-09-22: it is SEVEN, and the floor is written down**

- **libresolv: LOOKED AT, AND IT WAS NEVER A DEPENDENCY.** The first write-up
  of this bullet, the same afternoon, said it was gio's: `nm` over the staged
  archives shows `libgio-2.0.a(gthreadedresolver.c.o)` naming `res_nquery`,
  `dn_expand`, `__res_ninit` and `__res_nclose`, `gresolver.c.o` naming the
  threaded resolver from `g_resolver_get_default`, and GDBus naming that — all
  true, and beside the point. **A fresh reader refuted the conclusion with one
  command:** `readelf -V build/satl` lists version-needs for libm, libstdc++,
  libgcc_s, libc and ld-linux and **nothing for libresolv** — the linker took no
  symbol from it. glibc 2.34 moved all four resolver symbols into `libc.so.6`
  (`res_nquery@@GLIBC_2.34`, `dn_expand@@GLIBC_2.34`); satl's floor is 2.38, so
  on no machine satl can run does libresolv supply anything, and
  `LD_DEBUG=bindings` shows every one binding to libc. And gio never asked:
  `gio/meson.build:52` adds `-lresolv` only where `res_query` does **not** link
  plainly, the vendored build's meson log says it did, and `gio-2.0.pc` has no
  `-lresolv`. **It was NEEDED because `047-window.mk` said `-lresolv`**, and a
  `-l` with no `--as-needed` is a NEEDED entry whether or not anything binds.
  The flag is dropped, `ALLOWED_NEEDED` is seven, `make` prints seven, and the
  bare-machine proof binds seven. (`-lpthread -lrt` on the same line are the
  same shape and produce no entry — glibc 2.34+ has no separate `.so` for
  either; they stay, harmless.)
- **libwayland-client, libwayland-egl: settled, cannot go.** Part 1 says why.
  libwayland-client's own NEEDED brings the machine's `libffi.so.8` in beside
  satl's static libffi — two in one process, no shared state, measured working
  in DEP-3's empty root.
- **libc, libm, ld-linux: settled.** A fully static glibc binary links and
  cannot draw (measured, 21 MB, SIGSEGV in EGL).
- **libstdc++, libgcc_s: DEP-2's**, decided and deferred by the author.

**THE FLOOR, written down once so nobody reopens it — SEVEN:**

    ld-linux-x86-64.so.2  libc.so.6  libm.so.6                      glibc, every Linux
    libstdc++.so.6  libgcc_s.so.1                                   the distribution's; DEP-2 removes them
    libwayland-client.so.0  libwayland-egl.so.1  (+ libffi.so.8)    the machine's, cannot be carried

**And what "measured" has to mean here, learned the same day:** the archive
chain was real and the conclusion was still wrong, because the question was
never *who references the symbol* but *who defines it at run time*. `readelf -V`
answers that in one line, and it is the check to make before any NEEDED entry
is called a dependency.

and a **version floor: glibc ≥ 2.38 and libstdc++ ≥ `GLIBCXX_3.4.32` (gcc 13)**
— AlmaLinux 10 yes, AlmaLinux 9 no, measured symbol by symbol in DEP-3 and
asserted by check.sh. The GPU driver is **not** on the floor: without one GTK
draws with cairo (DEP-3).

## DEP-5 — satl-term, and the author's "single application"

His original brief was *"wiring in GTK+ so that satl and satl-term become a
single application"*. Today they are two binaries, and satl-term as built
still links the machine's VTE and GTK. ~~**satl-term cannot be static at all**
— GTK4 and VTE hardcode `shared_library()` upstream, so there is no `.a` to
link.~~ **Both halves of that are gone**: GTK's archives have been linked out
of its build tree since 2026-09-20, and **VTE builds a static archive since
2026-09-22** with a one-word journalled patch (GTK-17).

**So "single application" is now a choice and not a wall.** The third option
below is built; none is chosen: satl grows the terminal widget itself
(GTK-17/18); satl-term stays dynamic and is the one thing that needs GTK
installed; or satl-term links the vendored archives the way satl does. **The
author's call.** And whatever is decided, **satl-term is not removed** — he
has ruled on that once already: the GPU terminal next door does not replace
it.

## DEP-6 — LGPL §6, which static linking is what gives teeth

satellite is MIT. GTK, glib and gdk-pixbuf are LGPL-2.1-or-later; pango
LGPL-2.0-or-later; cairo LGPL-2.1-or-MPL-1.1; freetype FTL-or-GPLv2.

**Static linking obliges us to let a recipient relink against their own modified
GLib.** Shipping satellite's source plus `vendor/` satisfies it — *which DEP-1
also happens to deliver*. It becomes a live constraint the moment
`satellite_enterprise/` ships a binary **without** source, and then we need to
ship the `.o`/`.a` set too.

**Still the author's, from WIN-8:** freetype FTL-vs-GPLv2 (FTL recommended;
GPLv2 would infect) and cairo LGPL-vs-MPL (LGPL recommended). FTL also wants a
verbatim line in the documentation naming the year of the version vendored.
`THIRD-PARTY-NOTICES.md` cannot be generated until both are answered.

**The OFL is a build step, not a courtesy** — IBM Plex Mono is embedded in the
binary now, so `OFL.txt` ships with it. It is already inside the GResource;
decide whether `satl --licences` prints it.

## DEP-7 — the lower layer is OLD, and freezing makes that worse

`fetch-deps.sh` pins glib, pango, gdk-pixbuf, graphene and libxkbcommon to what
this machine runs. Everything below still comes from GTK's own wraps:

    zlib 1.2.11 (2017)   libpng 1.6.37   freetype 2.11.0   harfbuzz 4.0.0 (2022)

Static linking means **a distribution can never patch these under us**, and the
author's plan is to freeze for 3–5 years. zlib 1.2.11 in particular predates
CVE-2018-25032 and CVE-2022-37434.

**DEP-1 freezes whatever is there today, so decide BEFORE DEP-1 whether to bump
the lower layer** — it is much cheaper to choose versions once than to re-vendor.

## DEP-8 — the distribute package, now that a binary is 87 MB

`satellite_distribute` ships prebuilt. With no dependencies it gets *simpler* —
no `dnf install` line at all — but it is 87 MB instead of 1 MB, and WordPress
rejects `.gz` (ship `.xz`). The install layouts and the sudo/.bashrc rules are
already settled; only the size and the "what must be installed first" section
change.

## DEP-9 — other machines

- **X11 is off** (`-Dx11-backend=false`): nine X libraries, none with a wrap, so
  every one would link as a system `.so` and quietly un-static the binary.
  libepoxy's own `glx_static` test fails the build proving it. **satl draws on
  Wayland only** until someone vendors them. (WIN-10.)
- **Windows and macOS**: see the porting notes; VTE makes satl-term Linux-only
  regardless.
- **Non-AMD GPUs**: nothing here should care, and since 2026-09-22 the
  bare-machine proof does not either for a stock mesa behind glvnd — it finds
  the driver through glvnd's vendor file — and it runs without one at all
  (DEP-3). A driver whose pieces no `ldd` names (NVIDIA) fails that stage
  loudly.

---

# Part 2G — the GTK milestones: every widget, and how one is added

**`DEP-n` is the "NO DEPENDENCIES" half of this file's title. `GTK-n` is the
"GTK" half**, and it was missing until 2026-09-21. The author:

> *"every widget needs to be added somehow, some widgets you can add by
> yourself, most of them you can add by yourself, we especially need
> satellite.console to be a window with libvte and satellite.terminal to be a
> bash prompt"*

SATELLITE_WINDOW.md's `WIN-n` is the record for **the window itself** — the one
GTK thread, the carried data, the press queue, the purity audit. `GTK-n` is the
record for **what a program can put in one**. Four `WIN-n` are built and they
gave satellite exactly **two** pieces: a window and a button.

## GTK-0 — THE RECIPE: the seven places one widget touches

This is not a milestone, it is the thing every milestone below repeats. It was
worked out by reading what `button` actually cost, and it is written here so the
tenth widget costs an hour rather than a morning.

**A WIDGET IS NOT A NEW TYPE.** `satellite_window.hpp` settled this on
2026-09-20 and nothing since has wanted it back: *"A WINDOW AND A BUTTON ARE ONE
TYPE, not two arms"*. Every piece is `satellite.variable.window` and carries a
`Piece` saying which it is. Twenty widgets is twenty `Piece` values and **one**
arm of the object model, one `Value` branch, one entry in every switch over
`Kind`. Two arms would have been two entries saying the same thing twice; twenty
would be unmaintainable and the author would be right to refuse it.

**THE SEVEN PLACES**, in the order to touch them:

| | file | what goes in |
|---|---|---|
| 1 | `words/words_004.tsv` | one row: `1 27 n<TAB>satellite.window.thing(what)` |
| 2 | — | `python3 words/make_words.py` then `python3 satellite/bytecode/make_word_codes.py`. **Never edit `word_codes.hpp`.** |
| 3 | `REGISTRY.satellite` | a row per NEW method, next free code; then `python3 satellite/bytecode/make_token_codes.py` |
| 4 | `satellite_window.hpp` | a `Piece` value, and the factory's declaration |
| 5 | `satellite_window.cpp` *(or its successor — see below)* | the factory, **every GTK call inside `on_the_desk()`** |
| 6 | `bytecode/window_calls.cpp` — **six files since 2026-09-22**, see below | the table row and the `call_window_word` branch in `window_calls.cpp`; `window_method_arity` in `window_shapes.cpp`; a bare read in `window_questions.cpp`; a doing in `window_methods.cpp` — **and every function's `#else` half must still answer the same shapes, in the same file** |
| 7 | `check.sh` | at least: the word is refused with the wrong argument count *before anything runs*; the word's numbers are in `words.tsv`; the method is refused on a piece that does not have it |

**THE NUMBER IS FROZEN THE DAY THE ROW LANDS.** `make_words.py` takes the next
free number under `1 27` and stops rather than write a gap or a clash, so the
order the milestones are BUILT in is the order the numbers are handed out. The
spellings below are the intention, not a reservation — a widget not yet built
has no number.

**AND THE FILES HAVE TO SPLIT FIRST.** `satellite_window.cpp` is 274 lines
against the author's *"try to build for 300 lines"*, and GTK-1 alone would pass
it. The split, decided once here so no milestone re-decides it:

    satellite_window.cpp    the window: new, close, focus, title, append, and the run
    window_pieces.cpp       every factory -- button, label, text box, ... (GTK-1)
    window_asks.cpp         reading a piece back: .text, .value, .on, .chosen (GTK-2)
    window_answers.cpp      the signals beyond `clicked` (GTK-9)

`window_calls.cpp` (350 lines) splits the same way when GTK-2 lands, into the
words and the methods.

**IT DID NOT, AND IT WAS 1278 LINES WHEN IT WAS SPLIT ON 2026-09-22** — after
the radio and the canvas's leftovers landed, at the seams above and two more
the file had grown (`window_readers.hpp` carries the roll):

    window_calls.cpp        the WORDS: kWords, the one table, and call_window_word
    window_shapes.cpp       what each METHOD takes -- the checker's questions
    window_questions.cpp    a piece ASKED something: every method read bare
    window_methods.cpp      a piece TOLD to do something: call_window_method
    window_run.cpp          events off the desk's queue into capsules
    window_readers.cpp      a Value into the C++ the desk wants

The same day `window_menu.cpp` (384) became the model and `window_menu_bar.cpp`,
and `window_canvas.cpp` (442) became the desk's replay and `window_strokes.cpp`,
each with a small internal header. **The rule the old file kept — both halves
of the `#if` in one place — is kept per function now**: a function's `#else`
stub sits in the file with its body, never in another. The one thing the
split changed in shape is `answer_a_question()`: the bare reads that were the
first half of a 590-line `call_window_method` answer through it, and a
refusal made there is a refusal made in the caller, because `context.code`
says so. Proved by `make`, check.sh and the compositor proof, all unchanged in
what they assert.

**WHAT A WIDGET COSTS IN THE BINARY IS NOTHING.** Every archive is already
linked, every widget class is already in `libgtk.a`, and `--start-group` drops
what nothing references — so the first program to say `satellite.window.label`
pulls in `gtklabel.o` and no library at all. **The exception is GTK-6**, which
turns on four projects nothing has reached yet, and **GTK-17**, which needs a
library that is not in the folder.

---

## GTK-1 — a label, and the split that has to happen first — **BUILT 2026-09-21**

    satellite.variable.window a_line = satellite.window.label("a line of text")
    my_window.append(a_line, 400, 100)

`gtk_label_new()`. The cheapest possible second widget: it draws and it answers
nobody, so it proves the **shape** of GTK-0 without also arguing about signals
or about reading a value back.

**What it forces, and every one of these is paid once for all twenty:**

- `piece_name()` is `piece == window ? "a window" : "a button"` today. It
  becomes a table, and every refusal that says *"only a window can be closed"*
  keeps working for pieces that did not exist when it was written.
- The **file split** of GTK-0. `window_pieces.cpp` starts here.
- `.text` — a new method token. A label's words, read with no brackets and
  written with them, the pair `.title` already is on a window.
- **`.append` stops being about buttons.** Its refusal names what can go in a
  window; that sentence has to be generated from the piece table, not typed.

**Reversible, and the one worth naming:** a label is not focusable and cannot be
pressed, so `label.pressed(c)` is refused — *"only a button is pressed"* already
says it. That refusal gets re-read at GTK-9.

**AS BUILT, and every one of the four above was paid:**

| where | what |
|---|---|
| `words/words_004.tsv` | `1 27 3  satellite.window.label(text)` |
| `REGISTRY.satellite` | `text_token` `0x0B2B` |
| `satellite_window.hpp` | `Piece` grew `label` and `how_many_pieces`; `kPieceNames` with a `static_assert` sized by the enum |
| `window_pieces.cpp` | **new** — every piece that goes INSIDE a window, and `window_piece_of_text` makes all of them |
| `window_calls.cpp` | `kWords`, the one table; `window_methods_are()` (in `window_shapes.cpp` since the 2026-09-22 split); `the_words_that_make_a_piece()` |
| `program_check.cpp` | its hand-typed method list deleted, asked of `window_calls.hpp` instead |
| `satellite_object.cpp` | a piece displays as its own name — `(label "hello")` — out of `kPieceNames` |

**TWO GUARDS THE COMPILER KEEPS**, and they are the reason GTK-2 onward is an
hour each: a `Piece` added without a row in `kPieceNames` fails the
`static_assert`, and one added without a widget in `a_widget_for` warns, because
that switch names every enumerator and has no `default`.

**Proved by running it**, not by reasoning: on a compositor of its own, an
800x600 window with the label appended at its centre, `.text` read back,
`.text("...")` written and read again, a button pressed, the window closed,
exit 0. check.sh is **391 passed, 0 failed** — nine rows more than WIN-11 left.

## GTK-2 — a person types: a text box and a text area — **BUILT 2026-09-21**

    satellite.variable.window a_box = satellite.window.text_box("")
    satellite.variable.window many  = satellite.window.text_area("")
    ...
    satellite.console.display(a_box.text)

`gtk_entry_new()` (one line) and `gtk_text_view_new()` (many). **Two words and
not one**, because a person typing one line and a person typing a page are
different things and GTK makes them different widgets; one word with a flag
would be satellite lying about a distinction it cannot hide.

**THIS IS THE FIRST PIECE WHOSE VALUE A PROGRAM READS BACK, AND IT CROSSES THE
THREAD LINE THE OTHER WAY.** Everything satellite reads today it already holds:
`.title` is a `std::string` on our side of the desk. What a person typed is
**GTK's**, and getting it means going to the desk *and waiting for an answer* —
`on_the_desk()` already waits, so the mechanism is there, but nothing has used
it to bring a value back yet. That is the whole of this milestone's risk and it
is small.

- `.text` reads what is in it and writes what is in it.
- An empty text box answers `""` and not a refusal. A person who typed nothing
  typed nothing.
- **A text area's text is a `GtkTextBuffer`**, not a widget property: two
  iterators and `gtk_text_buffer_get_text`. The `Piece` is what tells them apart.

**AS BUILT** — `text_box` is `1 27 4`, `text_area` is `1 27 5`, and `.text` grew
a second half: it was a look at the handle and is now an ask of the desk.

**WHAT RUNNING IT FOUND, AND IT WAS NOT SMALL.** The first shape rescued what a
person had typed when the window went away, so that `.text` could answer after a
close. It printed **five `Gtk-CRITICAL` assertion failures** and answered
nothing, and reading GTK's own source says why it can never work:

    gtk_window_dispose    unparents the child BEFORE chaining to the dispose
                          that emits the window's `destroy` -- so by the time
                          the desk hears about it, every piece is already gone
    gtk_entry_dispose     clears priv->text, and gtk_text_view_dispose calls
                          gtk_text_view_set_buffer(view, NULL) -- both BEFORE
                          they chain to the dispose that emits the PIECE's
                          `destroy`, so the piece's own signal is no better

**There is no moment in a GTK teardown at which a widget's words can still be
asked for.** So the rule is:

- a closed **button or label** answers — its words were always satellite's;
- a closed **text box or text area** is **refused**, with `S505 WINDOW_IS_CLOSED`
  and a sentence naming when to read it. Answering whatever satellite last
  happened to write would be an answer that is wrong and does not say so.

**AND THAT CHOICE IS WHAT KEEPS THIS MODULE RACE-FREE.** The rescue would have
had the desk writing a `std::string` that the interpreter reads — undefined
behaviour, and a real step down from the pointer and two bools that are the only
other fields the two threads share. `text` now has exactly one writer and it is
the interpreter: `window_text_of` reads the widget inside an `on_the_desk()`
lambda that has **finished** before the assignment happens.

**One size was invented and it is written down as such:** a `GtkTextView` in a
`GtkFixed` measures almost nothing, and `.append` places by the measured size, so
a text area would be a few pixels a person cannot find. It asks for 300×150.
GTK-8's `.resize` is how a program says otherwise.

## GTK-3 — on and off: a checkbox, a switch, and a group that agrees — **BUILT 2026-09-21; THE RADIO 2026-09-22, as the recommendation**

    satellite.variable.window agree = satellite.window.checkbox("I agree")
    ...
    satellite.console.display(agree.on)

`gtk_check_button_new_with_label()` and `gtk_switch_new()`. `.on` reads it and
`.on(satellite.true)` sets it.

**The radio is the interesting one and it is deferred inside this milestone.**
GTK4 makes a radio by giving one check button another as its *group*
(`gtk_check_button_set_group`), so a radio is not a widget — it is **two pieces
that know about each other**, and satellite has no spelling for that yet. Two
shapes, and **this one is the author's**:

    satellite.window.checkbox("yes").group(other_checkbox)     a method on a piece
    satellite.window.one_of(a_list_of_text)                    one word, many buttons

The second is nicer to write and makes a word that answers **many** pieces,
which nothing in satellite does. The first is a smaller change. **Not decided**,
and the checkbox and the switch were built without it.

### THE RADIO, BUILT 2026-09-22 AS THE RECOMMENDATION

On the author's *"we are almost done with GTK stuff"*, with the question kept
in the table below as still reversible. `one_of` is `1 27 22`, made from a
list exactly as a choice is:

    satellite.variable.window size = satellite.window.one_of({"small", "medium", "large"})
    satellite.console.display(size.chosen)      small -- the first is ticked from the start
    size.chosen("large")
    size.changed(when_picked)                   a person ticking one

**ONE WORD THAT DRAWS MANY, AND ONE PIECE.** GTK4 makes a radio by giving a
check button another as its group (`gtk_check_button_set_group`), so a one-of
is a column of check buttons, each after the first grouped to the first, and
what a program holds is the column. One piece and not a list of pieces,
because the question a program asks a radio is one question — which one — and
`.chosen` already asks it of a choice and a set of tabs. `.chosen("purple")`
on a one-of of three sizes is refused, as it is on a choice. `.changed` is
`toggled` on every button in it: a pick fires it twice, once for the button
going off and once for the one going on, and only the one going ON queues a
change. The first draft queued both and leaned on the queue collapsing the
pair — and the interpreter can take the first off before the second is on,
which would have run a capsule twice for one pick. Asking the button is one
call and no race.

**THE FIRST IS TICKED FROM THE START.** GTK4 would leave none ticked; a choice
shows its first item from the start, and a one-of that showed nothing picked
would have `.chosen` answer `""` for a control that looks as though it has an
answer. A person cannot un-pick a radio anyway.

**WHAT CHANGES IF THE AUTHOR RULES FOR `.group(other_checkbox)`:** the word
row and the `one_of` piece go, and `.group` is one method token on a checkbox
calling `gtk_check_button_set_group` — but then there is nothing to ask
`.chosen` of, and a program reads each `.on` in turn, which is the reason the
recommendation was this one.

**AND IT FOUND A DEFECT THREE DAYS OLD, by being run on a machine with no
screen.** A valid choice — `satellite.window.choice({"red", "green"})` — with
no display exited 13 `LINE_NOT_UNDERSTOOD` over a sentence saying there was no
display to draw on: the items branch of `call_window_word` marked EVERY
failure as the program's. Only an empty list is, and the test is the very
thing the factory checks now — as it already was for a range and a size — and
check.sh has the row. The two refusals also told a person to write
`satellite.container.list("red", "green")`, a word with no library; they say
`{"red", "green"}` now.

**PROVED ON A COMPOSITOR:** three words; `.chosen` reading `small` from the
start; `.chosen("large")` and reading it back; the group measuring; and a REAL
POINTER CLICK on the middle button running `when_picked` with `.chosen`
answering `medium` — and the capsule closed the window, exit 0.

**AS BUILT** — `checkbox` is `1 27 6`, `switch` is `1 27 7`, `.on` is `0x0B2C`,
read bare and written with brackets. A piece that is neither on nor off is
**refused by name** rather than answered `false`: a label has no such question.

**AND IT FOUND A LANGUAGE GAP THAT IS THE AUTHOR'S.** **satellite has no `true`
and no `false` to type.** A bool comes out of a comparison or out of `.ok`, and
nothing else makes one — `satellite.console.display(true)` is *"this name was
used and no satellite.variable line ever declared it"*. So `c.on(1 < 2)` would
be how a checkbox is turned on, which is not a sentence anybody should write.
**What was done instead:** `.on` takes a number, 0 for off and anything else for
on, which is `text_of`'s own documented rule pointing the other way — *a number
where text is expected is its digits* — rather than an invention. **A `true` and
a `false` literal is a LANGUAGE milestone and not a window one.**

**A WORD THAT TAKES NOTHING IS TWO ROWS IN `words.tsv`, and that was learned by
running it.** With only `satellite.window.switch()` registered,
`satellite.window.switch("on")` matched no word at all and was refused as **"no
capsule named switch"** — which tells a person nothing.
`satellite.window.nosuchword("on")` says exactly the same thing, which is what
proved it was the unregistered NAME and not the switch. The fix is
`satellite.infinity`'s own shape: `1 27 7` is the name and `1 27 7 0` is the
call. **Every zero-argument word after this one needs both rows.**

`satellite_window.cpp`'s pieces split a second time here, at 297 lines:
`window_pieces.cpp` MAKES a piece and `window_asks.cpp` ASKS one. The line is
`make` against `ask` — a piece is made once and asked for ever, and the asking
half is the half that crosses to the desk and brings a value back.

## GTK-4 — a number a person chooses: a slider, a number box, a progress bar — **BUILT 2026-09-21**

    satellite.variable.window how_much = satellite.window.slider(0, 100)
    satellite.variable.window getting_on = satellite.window.progress()
    getting_on.value(50%)

`gtk_scale_new_with_range()`, `gtk_spin_button_new_with_range()`,
`gtk_progress_bar_new()`.

**A PROGRESS BAR TAKES A PERCENTAGE, AND SATELLITE ALREADY HAS ONE.**
`satellite.variable.percentage` is `1 6 16` and was the author's own addition on
2026-09-17. GTK's `gtk_progress_bar_set_fraction` wants 0.0 to 1.0, which is a
percentage with the sign filed off — so the language's own type is exactly
right here and a program should never write `0.5` to mean half. `.value` on a
progress bar reads and writes a **percentage**; `.value` on a slider reads and
writes a **number**. Same method token, and the `Piece` decides — which is the
same rule `.text` follows on a label and a text box.

**GTK's ranges are `double` and satellite's numbers are not.** A slider from 0
to 100 is exact; a slider from 0 to 3 asked for a third is not. The honest rule,
and it is written here so nobody discovers it in a program: **a slider answers
whole numbers**, and a slider that needs fractions waits for the float arm
(arm 14, not built).

**AS BUILT** — `slider` `1 27 8`, `number_box` `1 27 9`, `progress` `1 27 10`
(and `1 27 10 0` for its call), `.value` `0x0B2D`.

**`spinner` WAS RENAMED `number_box`.** "Spinner" is GTK's word and it names two
different widgets in GTK's own docs — the little turning circle and the number
entry with arrows. `satellite.window.number_box(1, 12)` says which it is.

**THE PERCENTAGE IS EXACT AND THAT IS NOT LUCK.** A percentage is held as itself
times 10^32, so the whole of it is 10^34; the desk speaks **millionths**, so one
millionth of the whole is exactly **10^28**. Both directions are one multiply or
one divide and **nothing rounds on satellite's side** — `p.value(12.5%)` reads
back as `12.5%`. The millionths exist to fence off GTK's `double`, which is the
only thing in the chain that cannot be exact.

**The kind is the piece's, and the wrong one is refused rather than converted.**
`p.value(50)` on a progress bar is refused and told to write `50%`, because a
bare 50 could mean 50% or half of one and a guess between them is an answer that
is wrong and does not say so.

### Two defects it found, and one of them was three days old

1. **A slider on a machine with no screen exited 13 while printing "there is no
   display to draw on".** The refusal asked whether the WORD takes numbers
   instead of whether the NUMBERS were wrong. A code and a sentence disagreeing
   about what happened is the worst kind of report this project can print.
2. **A word whose only row takes two arguments fell out of the lexer entirely.**
   `shaped_word_code` had two answers — a row with exactly as many parameters as
   the call, and **the one-parameter row** as a fallback —
   so `satellite.window.slider(100)` matched neither and was refused as **"no
   capsule named slider"**: a word that exists, told it does not.
   **`satellite.window.new("a title", 800)` had the same hole since WIN-3** and
   nobody had seen it, because the check.sh row that tested it only ever asserted
   the exit code, and *"no capsule named new"* and *"takes a title, a width and a
   height"* are both 13. A third fallback — the single row carrying that name,
   whatever its shape — fixes both and cannot take a call away from a real word.

## GTK-5 — a list to choose from — **BUILT 2026-09-21** (the dropdown; the list box is GTK-16's)

    satellite.variable.list colours = satellite.container.list("red", "green")
    satellite.variable.window pick = satellite.window.choice(colours)
    ...
    satellite.console.display(pick.chosen)

`gtk_drop_down_new_from_strings()` and `gtk_list_box_new()`.

**THE FIRST WIDGET MADE OUT OF A SATELLITE CONTAINER RATHER THAN OUT OF TEXT**,
and that is the whole of its difficulty: a `GtkStringList` is a `GListModel`
which is a gobject, and it has to be built from satellite's list on the desk's
thread while the list itself lives on the interpreter's. **Copy it.** A live
view of a satellite list into a GTK model is a second ownership story across a
thread boundary, and this project already has one of those.

`.chosen` reads back the **text**, not the index — a program that wanted the
index can `.index_of` it in the list it already has.

**AS BUILT** — `choice` is `1 27 11`, `.chosen` is `0x0B2E`, and the list IS
copied: `satellite.window.choice(colours)` is a choice of what the list **said**,
and changing the list afterwards changes nothing on the screen.

**THE LIST BOX MOVED TO GTK-16 and that is a better cut.** A `GtkListBox` holds
*pieces*, not words — it is a container, and every container depends on GTK-7
having taken `.append` off coordinates. A dropdown holds words and depends on
nothing.

**`.chosen("purple")` ON A CHOICE OF RED AND GREEN IS REFUSED**, because
`gtk_drop_down_set_selected` on a position that is not there simply picks
nothing. A program that named an item the choice does not offer has said
something untrue about itself and should hear so. **Nothing picked reads as
`""`** and is not a refusal: a choice a person has not touched is an ordinary
state of a choice.

**AND IT BROKE THE GENERIC REFUSAL, WHICH IS WORTH KNOWING.** Every doing-method
could once fail for exactly one reason — the window had gone — so the tail of
`call_window_method` reported `S505 WINDOW_IS_CLOSED` for all of them.
`.chosen("purple")` fails with the window **wide open**, and S505 under a
sentence reading *"there is no purple to choose here"* is a code and a sentence
disagreeing about what went wrong. The tail now asks the widget.

## GTK-6 — a picture, and the four projects it switches on — **BUILT 2026-09-21**

    my_window.append(satellite.window.picture("logo.png"), 400, 200)

`gtk_picture_new_for_filename()` (which scales) or `gtk_image_new_from_file()`
(which does not).

**THIS IS THE MILESTONE THE CHART EXISTS TO POINT AT.** gdk-pixbuf with its
thirteen loaders, libpng, libjpeg-turbo and libtiff are ~2.6 MB of satl right
now, carried since the first vendored build, and **no satellite program can
reach a line of it**. GTK-6 is what earns them.

- **A missing file must not take the run down.** `gtk_picture_new_for_filename`
  on a path that is not there gives a widget that draws nothing and says nothing.
  satellite refuses at the word instead, the way `satellite.file` does, and for
  the same reason: an answer that is wrong and does not say so is the one thing
  this project will not ship.
- **A path is a path, and `satellite.file` already has the rules for one.** This
  milestone must not invent a second set.
- SVG is `libgtk_svg.a` in 4.24 and comes free with the same word.

**AS BUILT** — `picture` is `1 27 15`, and `.path` reads which file it shows and
writes a different one. **`.text` on a picture is refused and sent to `.path`**,
the same pairing a window has with `.title`: a path is not words on a piece, it
is where a piece got what it draws.

**`gdk_texture_new_from_filename()` AND NOT `gtk_picture_new_for_filename()`**,
and that is the whole of the first bullet above made real. The second takes a
path that is not there, hands back a widget that draws nothing, and **says
nothing at all** — a person would see an empty space where their logo should be
and have no way to find out why. The first has a `GError`, and **GLib's own
message is better than anything written here**: it names the file, and it knows
a missing file from one that is there and is not a picture.

**AND THE MACHINE CODE IS A FILE'S.** `satellite.variable.file` already has the
scale, so a missing picture answers `39 file_not_found` and a file that is not a
picture answers `42 file_unreadable`. `satl_line_not_understood` would have said
the LINE was wrong, and the line is fine.

**A FAILED `.path` LEAVES THE OLD PICTURE AND THE OLD ANSWER.** Writing the new
path before checking would have left a piece saying it shows a file it does not.

**Proved with real files**, written by GdkPixbuf on this machine: a 64×48 PNG
loaded and shown, then swapped for a JPEG — **libpng and libjpeg-turbo both
reached from a satellite program for the first time.** libtiff is the same word
and the same code path and has not been proved with a `.tif`.

## GTK-7 — putting a piece somewhere other than by coordinate — **BUILT 2026-09-21**

    satellite.variable.window a_row = satellite.window.row()
    a_row.append(satellite.window.button("one"))
    a_row.append(satellite.window.button("two"))
    my_window.append(a_row, 400, 300)

`gtk_box_new()` and `gtk_grid_new()`.

**IT CHANGES `.append`, WHICH IS A LANGUAGE-VISIBLE CHANGE AND THE FIRST ONE
HERE.** A window holds a `GtkFixed` and `.append(piece, x, y)` places by the
piece's **centre** (WIN-3, and it is the author's spelling). A row has no
coordinates at all. So:

    .append(piece, across, down)    into a window   -- three, as today
    .append(piece)                  into a row, a column or a grid
    .append(piece, across, down)    into a grid     -- a cell, not a pixel

Three arities for one method, told apart by the receiver's `Piece`. The checker
already asks `window_method_arity()` **before the program runs**, and that
function takes only the method — so **it has to learn the receiver too**, or
arity checking for `.append` moves back to the walker and a program prints a
line before stopping. That regression is exactly the one `7480119` fixed for
window methods generally, so it must not be reintroduced here.

**AS BUILT, and the arity question above has an answer that neither shape
offered.** **The checker cannot know the receiver — ever.** A
`satellite.variable.window` name may hold a window or a row, and which it holds
is not decided until the line that makes it **runs**. So:

- the checker accepts **one argument or three**, and nothing else — `.append(a, b)`
  is still refused before a line runs;
- `window_append()` names the wrong one at the moment it knows, **with the piece
  it actually got**: *"a row puts its pieces one after another, so .append takes
  just the piece"*.

Nothing moved back to the walker that the checker could have caught. `1 27 12`
row, `1 27 13` column, `1 27 14` grid, all taking nothing.

**A GRID'S CELLS COUNT FROM 1**, because that is how satellite counts a file's
lines (the author: *"all line numbers start at 1"*). GTK counts from 0 and the
one subtraction lives in `window_append` so that no program ever has to know it.

### Nesting broke two things that already worked, and both were found by running it

1. **A press inside a row handed its capsule the ROW.** `inside_of` names the
   *immediate* parent, which was the window for every piece that had ever
   existed — so `when_pressed(the_piece, its_window)` would have received a row
   where it declared a window, and `its_window.close()` would have answered
   *"only a window can be closed"*: a refusal about a line that is right.
   `the_window_holding()` walks the rest of the way, and **everything that wants
   "the window this happened in" must go through it**.
2. **A teardown let go of only the window's direct children.** GTK frees the
   whole tree with the window, so a button inside a row would have kept a
   `GtkWidget *` that had been freed — and **it would have read as a live button
   right up until something touched it**, which is the exact failure that loop
   was written to prevent in the first place.

**And one hang was designed out rather than found.** `.append` refuses to put a
piece inside something it already holds, because that is the only way the
`inside_of` chain could be made to loop — and a loop there is a hang **inside a
press**, which is the one place a hang looks exactly like satl locking up
(Q-WIN-11a's whole subject). `the_window_holding()` caps its walk anyway, so a
defect in that refusal is a refusal rather than a hang.

## GTK-8 — the window itself is more than a rectangle — **BUILT 2026-09-21** (no icon; see below)

    my_window.resize(1024, 768)     my_window.wide      my_window.tall
    my_window.fullscreen()          my_window.icon("logo.png")

`gtk_window_set_default_size`, `gtk_widget_get_width/height`,
`gtk_window_fullscreen/unfullscreen`, `gtk_window_set_icon_name`.

`.resize` is a method token that **already exists** — `infinity.resize(n)`,
`0x0B25`. A second receiver for one token is the shape `.append` is already in
(a file, a list, and now a row), and it is right: one name, one meaning, many
kinds of thing.

**`.wide` AND `.tall` ANSWER WHAT IS ON THE SCREEN AND NOT WHAT WAS ASKED FOR.**
A compositor may not have given the window the size it wanted — tiling ones
routinely do not — and a window that reports its wish rather than its size is
the failure mode this project keeps naming. Before it is mapped there is no
answer; it reports the asked-for size and **says so in the documentation**.

**AS BUILT** — `.resize(wide, tall)`, `.width`, `.height`, `.fullscreen`.

**`.wide` AND `.tall` WERE SPELLED `.width` AND `.height`**, and not by
preference: **`wide_token` already exists** at `0x9C40` and is the marker for a
32-bit character in a payload (D3.1). Two things called `wide` in one registry
is exactly the confusion the registry exists to prevent.

**`.resize` IS A TOKEN THAT ALREADY EXISTED** — `infinity.resize(n)`, `0x0B25`.
One name, one meaning, many kinds of thing: the shape `.append` has had since a
file and a list shared it.

**`.width` AND `.height` ARE EVERY PIECE'S, NOT JUST A WINDOW'S**, and what they
answer when nothing is on a screen yet was chosen rather than defaulted:

1. the real size, if a compositor has given it one;
2. for a **window**, the size it asked for;
3. for anything else, its **measured natural size** — which is the very number
   `.append` uses to centre it.

**Never 0 for a piece that exists**, because 0 is an answer a program would act
on and it would be acting on nothing. Measured: a button in no window at all
answers 105 × 34.

**AND THE COMPOSITOR DECLINING IS VISIBLE IN THE PROOF.** `w.resize(1024, 768)`
followed by `w.width` answered **800** under headless mutter — the request was
made and not granted, and `.width` reported what *is*.

**THERE IS NO `.icon`, AND THAT IS A FINDING RATHER THAN AN OMISSION.** GTK4 has
no per-window icon from a file: `gtk_window_set_icon_name` takes a **theme
name**, and a Wayland compositor takes a window's icon from the `.desktop` file
it matches by app id. A word that took a path and quietly did nothing is the
answer that is wrong and does not say so, so none was minted — and check.sh has
a row asserting none exists, so nobody adds one by accident.

## GTK-9 — every piece talks back, not just a button — **BUILT 2026-09-21**

    a_box.typed(when_typed)          a person typed in it
    how_much.changed(when_changed)   a slider moved, a checkbox turned
    pick.chosen(when_chosen)         a different thing was picked
    my_window.closed(when_closed)    the window went away

**The machinery is done and this milestone is mostly a table.** WIN-11 built the
press queue, the interpreter-thread ruling, the capsule-name checking and the
argument binding; the only thing `clicked` has that `changed` does not is a
line connecting it. What is genuinely new is one question, and **it is the
author's**:

> **Does a capsule get the NEW VALUE as an argument?**

Today a press hands the capsule what the capsule **declares**: nothing, the
piece, or the piece and its window (`7480119`). A slider that moved wants to
hand over **the number**, and a text box that was typed in wants **the text**.
Three shapes, none chosen:

1. **It does not.** The capsule takes the piece and asks it — `the_piece.value`.
   Costs nothing, works today, and is one more line in every capsule.
2. **A third declared parameter**, typed to match the piece. Reads best; means
   the checker has to know what a slider's capsule may declare, which is a
   per-widget rule in a place that has none today.
3. **A general `.was` on the piece**, filled in for the duration of the capsule.
   Cheap, and it is a global by another name, which this language does not have.

**The recommendation is (1)**, because the piece is already handed over and
`the_piece.value` is one short line. It is written here rather than decided.

**And one refusal gets re-read here.** `only a button is pressed` is right for
`.press()`; `.pressed()` on a checkbox is a reasonable thing to want, and GTK
gives a check button a `toggled` rather than a `clicked`. The rule that survives
is **a piece answers the signals it has**, and the table says which.

**AS BUILT — TWO METHODS, NOT FOUR.** `.typed`, `.changed` and `.chosen` above
were three names for one question, and one question gets one method:

    a_piece.changed(when_changed)    typed in, moved, ticked, picked in
    my_window.closed(when_closed)    the window went away, whoever closed it

`changed` is `0x0B2F`, `closed` is `0x0B30`. **A button is refused and sent to
`.pressed`** — a button is not changed, it is pressed. **A label, a picture, a
row and a progress bar are refused by name**, because nothing a *person* does
changes any of them and a capsule wired to one would simply never run — the
quietest possible way for a program to be wrong.

**EXTENDING ONE PREDICATE EXTENDED THE WHOLE CHECKER.** Every rule WIN-11 wrote
for `.pressed` — the name is read as written and not as text, only one name may
stand there, the capsule must exist, it may declare at most the piece and its
window — now holds for all three, and **not one line of the checker changed**.
`window_method_takes_a_capsule_name()` is the whole of it.

**`APress` BECAME `AnEvent`, AND THAT IS A RENAME AND NOT A REDEFINITION.** A
button being pressed was the only thing that could reach satellite code, so the
name was the truth. Five things arrive on that queue now, and a struct saying
"press" for all five would have been a comment that lies.

**A DRAG IS ONE CHANGE; THREE CLICKS ARE THREE PRESSES.** A slider dragged
across the screen emits `value-changed` dozens of times, and the capsule runs
*after* the program's own lines and reads the value that is there **then** — so
consecutive identical changes collapse into one. Collapsing loses nothing a
capsule could have observed. **A press never collapses**: three clicks are three
things a person did, and `press-a-button.sh` counts them.

**THE AUTHOR'S QUESTION ABOVE IS STILL HIS, AND (1) IS WHAT WAS BUILT.** A
changed capsule is handed what a press is handed — nothing, the piece, or the
piece and its window — and **not the new value**. `the_piece.text` is one short
line and it reads the live piece.

**THE FOOTGUN, WRITTEN DOWN RATHER THAN FENCED OFF:** a `.changed` capsule that
writes to its own piece runs again, for ever. That is the program's own
`while(true)` and the language has no limits, so it is not fenced — the collapse
keeps the queue from GROWING, which is the part that would have looked like a
leak rather than a loop.

## GTK-10 — the look: colour, a font, a size — **BUILT 2026-09-21** (the DEFAULT font is still the author's)

    my_window.font("IBM Plex Mono", 12)
    a_line.colour("#00ff88")

`GtkCssProvider` and `gtk_style_context_add_provider_for_display`. GTK4 has no
per-widget colour setter at all — **everything is CSS** — so this milestone is
really "satellite generates a stylesheet", and that is a bigger idea than it
looks.

**WIN-3'S FONT RULING IS STILL OWED AND LANDS HERE.** IBM Plex Mono is already
inside the binary (WIN-1 put the whole family in the GResource) and **no font is
set at all today** — the window uses whatever theme GTK found. 11px or 12px is
the author's, asked on 2026-09-19 and unanswered. A satl that carries a font and
does not use it is carrying it for nothing.

**The trap, written down before anybody hits it:** a CSS provider added to the
display styles **every** window, and a provider added per widget needs the
widget realized. The desk owns both, so both are `on_the_desk()` work, and
neither is a reason to hold a second lock.

**AS BUILT** — `.colour("#00ff88")` (and `.color`, two spellings one meaning),
`.background("#222228")`, `.font("IBM Plex Mono", 12)`. Any piece, **including a
window**: GTK styles a `GtkWindow` like anything else.

**The trap above is exactly what happened, and the class is the answer.** Each
piece that is dressed gets a css class nobody else has — `satl-1`, `satl-7` —
and its provider is scoped to that class, so a rule added to the display reaches
only the piece that asked. The provider is **kept and reloaded**, because a
provider holds **one** stylesheet: setting `.colour` after `.font` has to say the
font again or it would take it away.

**NOT `gtk_widget_get_style_context()`.** GTK deprecated it in 4.10 and **4.24
has removed the header** — `gtkstylecontext.h` is not in the tarball. The class
and the display are the way that is left, and it is the better one anyway.

### The finding: a dropped rule says nothing at all

`gtk_css_provider_load_from_string()` **answers nothing**. A rule it cannot read
is dropped, the piece stays exactly as it was, and the program carries on
believing it asked for something — and a stylesheet is the easiest place in this
whole module to produce that, because `.colour("orange juice")` gets through any
character filter worth writing and means nothing to GTK.

**The `parsing-error` signal is the only way to hear about it**, so it is
connected and a rule GTK will not read is a **refusal**:

    a.colour could not be done -- GTK could not read "notacolour" as a colour,
    so nothing was changed

**And a refused colour puts the old one back**, so a piece is never left wearing
a rule with a hole in it. *That refusal is also what proves the good ones
parsed* — the working program produces no error, which now means something.

### The font is carried AND findable, measured rather than assumed

`fc-match` through **satl's own spilled fontconfig**, not the machine's:

    FONTCONFIG_FILE=$SPILL/fonts.conf fc-match "IBM Plex Mono"
    IBMPlexMono-Regular.ttf: "IBM Plex Mono" "Regular"

So `.font("IBM Plex Mono", 12)` genuinely uses the family WIN-1 put in the
binary. **And `fc-match "NoSuchFamilyAtAll"` answers the same file**, which is
the asymmetry worth knowing: **a bad colour is refused and a bad font family
cannot be**, because fontconfig always answers something.

**THE DEFAULT IS STILL UNSET AND STILL THE AUTHOR'S.** WIN-3 asked 11px or 12px
on 2026-09-19 and it is unanswered, so no font is set for a window that does not
ask. What changed is that a program *can* now ask, and the font it asks for is
the one in the binary.

## GTK-11 — asking a person something: a message, a question, a file — **MESSAGE AND QUESTION BUILT 2026-09-21; THE FILE DIALOG 2026-09-22, ON THE AUTHOR'S RULING OF Q-WIN-11a: "defend"**

    satellite.window.message("saved")
    satellite.window.ask("delete it?", when_answered)
    satellite.window.choose_a_file(when_chosen)

`GtkAlertDialog` and `GtkFileDialog`, both new in 4.10 and both **asynchronous**:
the answer arrives in a `GAsyncReadyCallback` on the desk's thread, which is the
press queue again with a different producer.

**AND IT IS WHERE SATELLITE HAS TO DECIDE WHETHER A PROGRAM CAN WAIT.** Every
window word today answers at once. A question does not have an answer until a
person gives one, and there are only two honest shapes:

    satellite.window.ask("delete it?", when_answered)     a capsule, like a press
    satellite.variable.string a = my_window.ask("...")    the program STOPS here

The second blocks the interpreter's thread while the desk runs — which is safe,
because they are different threads and the desk is the one drawing — but it is
the first time a satellite line waits for a person, and `satellite.console.input`
is the only precedent. **The author's**; the recommendation is the capsule,
because it is the shape a press already taught.

**AS BUILT, AS THE CAPSULE** — `my_window.message("saved")`,
`my_window.ask(when_answered, "delete it?")`, and `my_window.answer` reading
`"yes"`, `"no"` or `""`. **The other shape stays open**; nothing here forecloses
it.

**THE CAPSULE'S NAME COMES FIRST, AND THAT IS NOW A LANGUAGE RULE.**
`.ask(when_answered, "delete it?")` reads less like English than the other way
round and is spelled this way because `.pressed`, `.changed`, `.closed`,
`.every` and `.key` all are: the checker looks for a capsule's name at the
**first** argument and `expression.cpp` reads a name there instead of working
out a value. **One rule a person can hold in their head beats one line that
reads slightly better.** Written the other way round it is refused before a line
runs.

**Proved on a compositor**: a message shown, a question asked, and a **real
Return keypress** through mutter's RemoteDesktop answering it — the capsule ran
with `its_window.answer` reading `"yes"`.

**Two things GTK would have got wrong quietly, both fixed here:**

- **`gtk_alert_dialog_new` TAKES A PRINTF FORMAT.** A person's own text
  containing a `%` would be read as a conversion and GTK would walk off the end
  of an argument list with nothing in it — **a crash a program could cause by
  displaying a percentage.** It is `"%s"` and the text as an argument.
- **DISMISSED IS NOT A FAILURE.** Closing a question without choosing is a thing
  a person is entitled to do, and GTK reports it as a `GError`. It becomes `""`,
  not a refusal of a program that did nothing wrong.

### THE FILE DIALOG IS NOT BUILT, AND THAT IS Q-WIN-11a

`GtkFileDialog` can go out to **xdg-desktop-portal**, and a wedged portal is
exactly the question the author has open: the synchronous D-Bus call that
**hangs satl for ever with nothing printed**. Building a word that can reach it
before he has ruled would be shipping the hang. check.sh asserts no
`gtk_file_dialog_` call exists, so nobody adds one without answering it.

**THE FACTS BEHIND Q-WIN-11a, read from the vendored 4.24.0 source on
2026-09-22 so the ruling can be made on them** — a fresh reader, spot-checked
line by line:

- **`GTK_USE_PORTAL` does not exist in GTK 4.24** — zero hits in `gdk/` and
  `gtk/` — so SATELLITE_WINDOW.md's *"`GTK_USE_PORTAL=0` before `gtk_init`
  would take the hang away"* was wrong, and is corrected there.
- **The only unbounded wait on the path is `gdksettings-wayland.c:477`**: a
  synchronous `ReadAll` on `org.freedesktop.portal.Settings` with timeout
  `G_MAXINT`, reached from `gdk_display_open_default` inside `gtk_init_check`.
  The two probes before it (`environment_has_portals` at `gdk.c:466`,
  `check_portal_interface` at `gdk.c:525`) are synchronous too, but capped at
  25 seconds each. **Which stage held satl for an afternoon on 2026-09-21 is
  NOT settled**: the gdb trace was caught inside `check_portal_interface`, the
  capped one, and nobody waited to see whether it moved on. What is settled is
  what turns the whole path off.
- **What turns it off, and turns the file chooser's portal off with it:**
  `gtk_disable_portals()` — public since 4.18, `gtkmain.h:87`, called before
  `gtk_init` — or `GDK_DEBUG=no-portals`. Both short-circuit
  `gdk_display_should_use_portal` (`gdk.c:623` and `:629`), which is also the
  gate the file chooser asks (`gtkfilechoosernativeportal.c:487`).
  `gtk_disable_portal_interfaces()` can name one interface and leave the rest.
- **The message and the question never reach the portal**: `gtkalertdialog.c`
  has no portal and no D-Bus in it. What GTK-11 built cannot hang this way.
- `env -u DBUS_SESSION_BUS_ADDRESS` in the proof scripts does not remove the
  bus: GLib falls back to `$XDG_RUNTIME_DIR/bus`, the user's real one, whose
  portal answers. The scripts work because they swap buses, not because satl
  runs without one.
- **The cost of disabling, which is the other half of the ruling:** no portal
  settings — dark mode, the font and the theme come through that interface,
  and GTK falls back to gsettings — and no portal file chooser inside a
  sandbox, so a flatpak satl would get the in-process dialog, which cannot see
  outside its sandbox.

The ruling stays the author's: defend and lose the portal, or leave it and say
so in a refusal. Nothing of it is built.

### Q-WIN-11a DECIDED 2026-09-22, AND THE FILE DIALOG BUILT THE SAME DAY

The author, asked what Q-WIN-11a was and given the two answers and their
costs, ruled in one word: *"defend"*. Then: *"can you do that now?"*

**THE DEFENCE IS ONE CALL BEFORE THE DISPLAY IS OPENED.** `window_desk.cpp`
calls `gtk_disable_portals()` before `gtk_init_check` — the same bit as
`GDK_DEBUG=no-portals`, read inside `gtk_init_check` at `gdk_pre_parse`, so it
has to come first — and on a system GTK older than 4.18 (`make GTK=system`)
appends `no-portals` to whatever `GDK_DEBUG` already said. satl never makes
the synchronous D-Bus call again. **The cost, taken with the word:** the
desktop's dark mode, font and theme no longer arrive through the portal and
GTK falls back to gsettings, whose schemas satl already spills (WIN-1); and
inside a sandbox the file chooser is GTK's own in satl's process, which
cannot see out of the sandbox. GTK's own note on the call says *"apps must
not call it"*; satl is a language's runtime shipped to machines it has never
seen, and a runtime that can hang before its first line is what was ruled
against.

**MEASURED, NOT ASSUMED:** the proof script's `file` stage starts satl ON the
bus `dbus-run-session` made — the one where the portal is activatable and
never finishes starting, the trap that cost an afternoon on 2026-09-21 —
without the `env -u DBUS_SESSION_BUS_ADDRESS` every other stage carries. It
opened its window, opened the chooser, and exited 0. Before the defence that
run sat in `gtk_init_check` for ever.

**THE FILE DIALOG, AS THE CAPSULE**, the shape the message and the question
already taught:

    my_window.choose_a_file(when_chosen)        the NAME first, as every capsule-naming method
    ... in the capsule:  its_window.answer      the path they chose, or "" if they closed it

`choose_a_file` is `0x0B49`. `GtkFileDialog`'s `open`, asynchronous, and with
the portal off it is GTK's own `GtkFileChooserDialog` in this process; the
callback turns the `GFile` into a path and it travels on the event as a
question's answer does — it IS an answer, and `.answer` reads it. Dismissed is
`""` and not a refusal, for the question's reason. The capsule's name is its
own field on the window and not `when_answered`, because a question and a
chooser can both be open over one window and the later one's capsule must not
run for the earlier one's answer. A save dialog and a folder dialog are the
same shape, one token and one GTK call each, and are not built: the plan
named one word.

**PROVED ON A COMPOSITOR WITH REAL KEYS:** the chooser up, a warm-up key,
`/` to open its location entry, the rest of a path typed a character at a
time, Return — and `when_chosen` ran with `its_window.answer` reading exactly
that path, and closed the window, exit 0.

**TWO THINGS THE TRANSCRIPT SHOWED, one fixed and one the author's:**

- The chooser adds what a person chose to the desktop's recent-files list
  under the application's name, and warned on stderr that satl had none.
  `g_set_application_name("satellite")` before init; the list says
  *satellite* now.
- **On that same hostile bus GTK printed three `Gtk-CRITICAL`s — *"Unable to
  register the application ... org.a11y.atspi.Registry"* — and carried on.**
  The accessibility bus is the same shape as the portal: a session-bus service
  GTK reaches for at init. It did not hang, it was noisy, and on a real desktop
  the registry exists and it is silent. It is NOT defended: `GTK_A11Y=none`
  would silence it and would also turn off every screen reader for every satl
  window, which is not a cost to take without asking. **Q-WIN-11c, the
  author's:** should satl silence the accessibility bus the way it silenced
  the portal?

## GTK-12 — a menu, and the only milestone that is gio and not gtk — **BUILT 2026-09-21**

    satellite.variable.window m = satellite.window.menu()
    m.append("Open", when_open)
    my_window.menu(m)

`GMenu`, `GSimpleAction` and `GActionMap` are **gio**, not gtk. Nothing else in
this list reaches gio for anything but WIN-1's resource, so this is the row in
the chart that only a menu fills.

**IT IS THE ONE PLACE `GtkApplication` WOULD NORMALLY BE, AND SATELLITE HAS
NONE.** `satellite_window.cpp` refuses `GtkApplication` on purpose and says why:
it is a `GApplication`, it registers on the D-Bus session bus, and a machine
satl ships to may have none — *and a wedged portal hangs `gtk_init_check` for
ever with nothing printed* (Q-WIN-11a). Actions therefore go on the **window**
(`gtk_widget_insert_action_group`), never on an application. That is settled by
the same evidence that settled `gtk_window_new()`.

satl-term already has a menu (`satl-term/menu.cpp`, 003's, ported). **Read it
before writing this one** — it is the same GMenu and the same actions.

**AS BUILT** — three lines, and two of them are not the three above:

    satellite.variable.window file = satellite.window.menu("File")
    file.item(when_open, "Open")
    my_window.menu(file)

`menu` is `1 27 19`, **one row and not two**; `menu` is `0x0B3D` and `item` is
`0x0B3E`. A second `my_window.menu(edit)` goes **beside** the first on the same
bar. What an item's capsule is handed is what a press is handed: nothing, the
**menu**, or the menu and its window.

**A MENU IS MADE FROM ITS HEADING, AND THAT IS GTK'S RULING BEFORE IT IS
OURS.** `gtkpopovermenubar.c`'s `tracker_insert` puts an item on the bar *only
when it has a submenu*, and an item without one is dropped with nothing said —
the doc string is *"The model should only contain submenus as toplevel
elements."* So `satellite.window.menu()` with nothing on the bar would have
been a menu that is nowhere, which is the answer this project does not ship. A
frame is the same shape already: the one holder whose word takes its words.

**`.item` AND NOT `.add`, AND NOT `.append`.** `.append` growing a third shape
for "a capsule and a name" is the point at which one method stops being one
method — GTK-16 says exactly that of a tab. `.add` reads better and **is `+`
already** (`s.add("x")` joins, `n.add(2)` sums), and the checker's capsule-name
rule is receiver-blind *on purpose* (`names_in_statement` walks every
statement, because a chain's second method has no declared receiver) — so
making `.add` name a capsule would have made every `x.add(...)` in the language
a capsule. `.item` is its own token. **The name comes first**, as in every
method that names one; the other way round is refused before a line runs.

**A MENU IS THE ONE PIECE THAT IS NOT A WIDGET.** Its `widget` holds the
`GMenu` — the model of its items — and a `GSimpleActionGroup` beside it, made
with the menu so that it can be built in full **before it has a window**, which
is the ordinary order to write those lines in. `satellite_window.hpp`'s
`is_drawn()` is the one question that says so, and **five** places ask it and
refuse a menu by name: measuring it, dressing it, watching it for a click,
putting it in a fixed or a row, and reading its words off a widget. What that
buys is **one bar for a whole window** — GTK's theme draws a rule under a bar,
so two bars side by side would show the seam, and F10 opens only the first bar
it finds — with the keyboard walking across it, which was **measured**: F10,
Right, Down, Return picked Edit's item from a bar that began on File.

**THE WINDOW'S CHILD IS NOW A VERTICAL BOX** with the `GtkFixed` as its
expanding child, because a `GtkWindow` holds exactly one child and a bar has to
go somewhere. With no menu the box holds only the fixed, and nothing measures
or places differently — check.sh's 475 rows and press-a-button.sh's
coordinates are the evidence.

**TEARDOWN GIVES BACK WHAT GTK NEVER TOOK.** A widget's piece owns nothing
after the window goes — GTK freed it. A menu's piece owns two references, and
`let_go_of_every_piece` returns them. A menu never put on a window is leaked at
exit exactly as a button never appended is; that rule did not change.

**Proved on a compositor**, by both routes a person has: a real pointer click on
File and then on Open ran `when_open` with `the_menu.text` reading `"File"` and
`its_window.title` reading the window's; a real F10 opened the bar and the arrow
keys walked it; an item picked after the menu was already on the bar ran; an
item whose capsule closed the window ended the run, exit 0; and `m.text("Edit")`
changed a heading already on the bar in place. Two windows open at once, one
closed with the other still up, also ran clean — the desk is one thread and
holds as many windows as a program opens.

**Three things found by running it, all fixed here:**

- **`satellite.window.menu()` ran the lines above it before refusing.** `menu`
  is the first word whose last segment is also a method's name: with no `menu()`
  row the lexer answered nothing for the shaped call, the shortening then read
  `satellite.window` and `.menu` as the *method*, and the wrong count was refused
  at run time with "there is no value here to work with". An empty call on a
  word that takes something now lexes as that word with 0 arguments and the
  checker refuses it by name before a line runs — `frame()` and `label()` too,
  which used to be **"no capsule named frame"**, a word that exists told it does
  not.
- **`m.width` on a menu exited 51 `WINDOW_IS_CLOSED`** under a sentence about
  a menu having no size — the code/sentence mismatch GTK-5 already fixed once
  for `.chosen`. It is `types_do_not_meet` unless the piece is actually closed.
- **A flat menu is not a menu bar.** The first design had `menu()` take
  nothing and its items sit on the bar directly; GTK's own source says a
  top-level item with no submenu is silently dropped. The heading is not a style
  choice.

**A MENU INSIDE A MENU AND A SEPARATOR — BUILT 2026-09-22, as the
recommendation, and still reversible.** The author's word was *"do the next
one, or do 3 even"*; the question row below is kept and marked, not removed.
`.menu` on a MENU puts the second menu under the first, as an item with an
arrow, and its heading is the word on that item — `satellite.window.menu("Recent")`
already carries it, so the piece carries its own name, as a tab's does now
(GTK-16). `.separator()` is `0x0B43`.

    satellite.variable.window file = satellite.window.menu("File")
    satellite.variable.window recent = satellite.window.menu("Recent")
    recent.item(when_one, "one.satl")
    file.item(when_open, "Open")
    file.separator()
    file.menu(recent)
    my_window.menu(file)

**A MENU'S MODEL HOLDS SECTIONS AND NOTHING ELSE NOW.** A `GMenu` has no
separator item; it draws a line between one section and the next, and that is
the only line it has. So a menu is made with one section, `.separator()` opens
the next, and items go into the last one. A menu that never says `.separator()`
has one section and looks exactly as it did. **A separator with nothing above
it is refused** — first, or two in a row — because a section with no items
draws no line, and a person asked for one. An item's action is named by a
counter across every menu now, not by the model's count: the model's count is
the number of sections, and two items either side of a line would have shared
a name.

**THE ACTIONS OF EVERY MENU INSIDE A MENU GO ON THE WINDOW WITH ITS PARENT'S**,
recursively, under each menu's own prefix and still with no `GtkApplication` —
at once if the parent is already on a window, and when the parent gets there
if not. The teardown walks `pieces` itself now rather than asking
`holds_pieces()`: a menu holds the menus inside it and refuses `.append`, and
the old question would have walked past a submenu and left it open with its
window gone.

**Refused by name:** a menu into itself; a menu into a menu it already holds
(the loop `.append` already refuses, for the same walk); a separator first or
twice. `.title` on a menu is sent to `.text`, which is the word it has.

**PROVED ON A COMPOSITOR BY REAL KEYS:** File with Open, Save, a separator and
Recent under it; More under Recent with `deep.satl` in it; `recent.text("Recently")`
renamed the submenu in place. F10, Down, Down, Down, Right, Right, Return picked
`deep.satl` — the capsule was handed the menu it was on (`the_menu.text` read
`More`) and the window (`its_window.title` read `menus`), and closed it, exit 0.

## GTK-13 — time: a capsule every so often — **BUILT 2026-09-21**

    satellite.window.every(1000, when_a_second_passes)

`g_timeout_add()` — **glib, with no gtk call at all**, the only milestone here
that touches neither a widget nor a window.

It is the **second producer for the press queue**, and the first proof that the
queue is a general thing rather than a button's private arrangement. Everything
WIN-11 ruled applies unchanged: it runs on the interpreter's thread, it waits
its turn behind whatever is running, and it is drained after the last window
closed.

**The one new question is what a tick does when its capsule takes longer than
its interval**, and the answer must be written into the word: **ticks do not
queue up**. A tick that arrives while the previous one is still running is
dropped, because the alternative is a program that falls further behind for
ever and looks like a leak.

**AS BUILT** — `my_window.every(when_it_ticks, 1000)`, `every` is `0x0B37`.

**IT IS A WINDOW'S METHOD AND NOT A WORD, and that is the whole design.** A word
would have had **no owner and no way to be stopped**; a window has a lifetime
already, so the clock simply stops when the window does. A second `.every`
replaces the first: one window, one clock.

**"TICKS DO NOT QUEUE UP" IS GTK-9'S COLLAPSE**, not a second mechanism. A tick
arriving while the previous one is still waiting its turn is dropped, exactly as
a dragged slider's repeated changes are.

**THE CAPSULE'S NAME COMES FIRST** — `.every(when_it_ticks, 1000)` — because
that is where the checker looks for a name and where `expression.cpp` reads one
instead of working out a value. **What may follow the name is now the method's
business**, asked of `window_calls.hpp`: `.pressed` takes a name and nothing
else, `.every` takes a name and then how often. Writing it the other way round
is refused before a line runs.

**A TICK OF 0 IS REFUSED.** It is not a rhythm, it is a busy loop with a capsule
in it — glib would run it as fast as the main loop turns and the queue would
fill faster than the interpreter could drain it.

**MEASURED ON A COMPOSITOR:** `w.every(when_it_ticks, 120)` ran the capsule **49
times in six seconds** — 50 is what 120 ms gives. And a second program whose
button closed the window **ended, exit 0**, rather than ticking for ever.

**The clock stops with the window, and it stops FIRST.** The source holds a raw
pointer into the `satellite_window`, so a tick firing between the close and the
desk letting go would queue a capsule for a window that is already gone — the
one way this module could reach freed memory.

## GTK-14 — the keyboard and the mouse, and where 2.8 MB finally earns itself — **BUILT 2026-09-21**

    my_window.key(when_a_key)        a_row.clicked(when_clicked)

`GtkEventControllerKey`, `GtkGestureClick`, `GtkEventControllerMotion` — GTK4
has no `key-press-event` on a widget; everything is a controller you add.

**libxkbcommon AND xkeyboard-config'S 293 FILES HAVE BEEN CARRIED SINCE WIN-1
FOR GTK'S SAKE**, because `gdkkeymap-wayland.c` SIGSEGVs without them before a
window exists. GTK-14 is the first time **satellite** asks what key was pressed.

**Keysyms are the hard part and it is a language question.** GTK answers a
`guint keyval` — `GDK_KEY_Escape` is `0xff1b`. A satellite program must not see
a number. The shapes are `.key` answering the **character** for a printable key
and a **name** for the rest (`"escape"`, `"up"`), which is what every scripting
language settles on, and is the recommendation. Whether modifiers are separate
is the author's.

**AS BUILT, AND THE RECOMMENDATION IS WHAT WAS BUILT.** `.key(a_capsule)` on a
window, `.key` read bare for the last one; `.clicked(a_capsule)` on anything
that is not a button. `key` is `0x0B38`, `clicked` is `0x0B39`.

**PROVED BY PRESSING REAL KEYS** on a headless mutter, through its own
`RemoteDesktop.NotifyKeyboardKeysym` — the route `press-a-button.sh` already
uses for the pointer:

    sent  a  a  c  d  7            answered  a  c  d  7
    sent  B  Escape Up Return F1   answered  shift_l  B  escape  up  return  f1

**`shift_l` is right**: mutter synthesises a Shift press to type a capital, and
a modifier **is** a key press — so modifiers are not separate, they are keys,
and that answers the author's question above by demonstration. **The first key
of a remote-desktop session is swallowed** — `a a c d 7` answers `a c d 7` —
and that is mutter settling, not satl: the second `a` arrives.

**`.key` READ BARE ANSWERS THE LAST KEY, NOT THE CAPSULE'S NAME**, and it is the
only one of the capsule-naming methods that does. Which key was pressed is the
thing a program wants; a capsule it wrote itself is not.

**WHAT A KEY SAID TRAVELS ON THE EVENT, NOT ON THE PIECE**, and that is the same
single-writer discipline GTK-2 settled. `AnEvent` gained a `said`; the desk
fills it in; **the INTERPRETER copies it onto the piece** as it takes the event
off the queue, just before running the capsule. Written by the desk and read by
a capsule it would have been a `std::string` with two threads on it.

**THE HANDLER ANSWERS `FALSE`, so the key goes on to whatever wanted it.** `TRUE`
would mean a program watching for Escape had silently made every text box in its
window unusable — which is the quiet kind of wrongness this project refuses.

**A button asked for `.clicked` is sent to `.pressed`**: a button already has a
word for being clicked.

## GTK-15 — a canvas: satellite draws it itself — **BUILT 2026-09-22, as the display list**

    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.draws(when_drawing)

`gtk_drawing_area_set_draw_func()`, and the callback is handed a `cairo_t *`.

**THE FIRST TIME SATELLITE'S OWN CODE CALLS CAIRO.** Cairo has been linked since
the first vendored build and every call into it so far has been GTK's.

**And it is the one milestone where the press queue is the WRONG answer.** A
draw function must return having drawn; queueing it to the interpreter's thread
and waiting would mean the desk blocks on the interpreter while the interpreter
may be waiting on the desk — **a deadlock, and the design already has every
piece needed to build one.** So either:

1. the draw capsule runs on the **desk's** thread, which needs the walker to be
   re-entrant and is a language decision, not a window one; or
2. a canvas is **not** a capsule at all — a program draws into it with
   `c.line(...)`, `c.box(...)` and satellite keeps the display list, which the
   draw function then replays with no satellite code running at all.

**(2) is the recommendation and it is not a compromise** — it is faster, it
cannot deadlock, and it is the only one of the two that works before the walker
is re-entrant. **The author's**, and it should be decided before anything is
built, because the two share no code.

**AS BUILT 2026-09-22, AND THE RECOMMENDATION IS WHAT WAS BUILT** — on the
author's *"do the next one, or do 3 even"*, with the question kept below as
still reversible. `canvas` is `1 27 20`, made from its size; `line` is
`0x0B3F`, `box` `0x0B40`, `circle` `0x0B41`, `write` `0x0B42`; `.clear()` and
`.save("picture.png")` reuse a file's `clear` and `save`, which mean the same
thing on a canvas.

    satellite.variable.window c = satellite.window.canvas(400, 300)
    c.colour("#40c8ff")
    c.line(0, 0, 399, 299)                  one pixel wide, on the pixel it names
    c.box(20, 20, 80, 50)                   filled, from its top-left corner
    c.circle(100, 200, 30)                  filled, around its centre
    c.font("IBM Plex Mono", 18)
    c.write(150, 40, "hello from satellite")
    c.save("before.png")
    c.clicked(when_clicked)                 the capsule is handed the canvas

**A CANVAS IS A DISPLAY LIST.** Every one of those appends an `AStroke` to the
piece and asks GTK to redraw; the function `gtk_drawing_area_set_draw_func`
was given replays the list and waits on nobody. No satellite code runs on the
desk, and the deadlock this milestone was ordered last to avoid cannot be built
out of it. The draw capsule stays the author's to ask for — nothing here
forecloses it, and it needs the walker to be re-entrant first.

**`.colour` AND `.font` ARE A PEN.** They change what is drawn after them and
nothing drawn before: the colour and the font are copied onto each stroke at
the moment it is made. With no `.colour` a canvas draws in the theme's own
foreground, which is right in a dark theme and in a light one. `.background`
is CSS, as on every piece, and GTK paints it under the list; `.save` paints the
same colour first so the file looks like the window, and nothing said is
nothing painted — the PNG is transparent there.

**THE PEN IS THE CANVAS'S OWN STRING AND NOT THE WIDGET'S COMPUTED STYLE, and
the picture is what found that.** The first draft read `gtk_widget_get_color`,
and a canvas told `.colour` twice before it was in a window drew the second
batch in the FIRST colour — the saved PNG showed a blue circle where a yellow
one was asked for. Adding the css class invalidates the node; reloading the
provider does not reach a widget that has no root. `a_colour` parsed with
`gdk_rgba_parse` at the stroke is the truth of what was asked for, and the
theme's colour is read off the widget only when nothing was asked.

**`.save` IS THE FIRST TIME satl WRITES A PICTURE** — cairo's own PNG writer,
through the libpng GTK-6 earned. The same replay draws the file and the
screen, so the two cannot differ. It is also how this milestone was proved: the
PNGs below were written by satl on a headless compositor and read back by eye,
which no other window milestone could offer.

**PROVED ON A COMPOSITOR:** a 400×300 canvas in an 800×600 window; `.width` and
`.height` answered 400 and 300; a REAL POINTER CLICK on the canvas ran its
`.clicked` capsule, which was handed the canvas and its window, drew a red
circle and red words on it AFTER it was on the screen, saved `after.png`, and
closed the window — exit 0. `before.png` showed the two blue diagonals, the
blue box, the yellow circle and the words in IBM Plex Mono 18; `after.png`
showed those and the red.

**cairo AND pango ARE CALLED BY NAME NOW** — Part 00 moved from four projects
to six. What is called is small and named in full: `cairo_move_to`, `_line_to`,
`_stroke`, `_rectangle`, `_arc`, `_fill`, `_set_source_rgba`, `_set_line_width`,
an image surface and `cairo_surface_write_to_png`; `pango_cairo_show_layout`
and a font description, on a layout GTK made from the widget. A line is drawn
at the half-pixel so it lands on the pixel it names rather than smearing over
two.

**A NEGATIVE WIDTH, HEIGHT OR RADIUS IS REFUSED where it is written.** A line
from a point to itself and a box of no size draw nothing, which is the honest
answer to what they are, and are let through — a program plotting data has
equal neighbours all the time. `.text` on a canvas is refused and sent to
`.write`; `.changed` is refused as on every piece nothing a person does can
change.

**OPEN, and the author's:** where a click on a canvas LANDED. `.clicked` runs
the capsule and says nothing about the point, and a drawing program wants it;
the shape would be what `.key` already does — the desk carries it on the event
and the interpreter copies it onto the piece — and no spelling has been
chosen. An outline (an unfilled box or circle), a line's width and an arc are
the same kind of question: cheap, and not decided.

### THE FOUR LEFTOVERS, BUILT 2026-09-22 AS THE RECOMMENDATION

On the author's *"we are almost done with GTK stuff"*, and each stays in the
table below as still reversible. `across` is `0x0B44`, `down` `0x0B45`,
`outline` `0x0B46`, `thickness` `0x0B47` and `arc` `0x0B48`.

    c.clicked(when_clicked)
    ... and in the capsule:  the_canvas.across    the_canvas.down
    c.thickness(4)                             the pen, from here on
    c.outline(1)                               boxes, circles and arcs as edges
    c.box(300, 20, 80, 50)                     a four-pixel outline
    c.arc(200, 150, 60, 0, 270)                three quarters, clockwise from three o'clock
    c.outline(0)
    c.arc(200, 150, 40, 270, 360)              a filled slice, twelve to three

**WHERE A CLICK LANDED TRAVELS ON THE EVENT**, exactly as `.key` does: the desk
writes the point GTK hands it — in the piece's own pixels, which on a canvas
are the pixels `.line` draws in — and the INTERPRETER copies it onto the piece
as it takes the event off the queue, so the two numbers have one writer.
`.across` and `.down` are questions, read bare or bracketed, and they answer 0
and 0 until a click has happened, as `.key` is `""` until a key has — a click
is noticed only on a piece told `.clicked(a_capsule)`, and a press released
off the piece lands outside it, negative or past `.width`, which is the truth
of where it was let go. They are every clickable piece's, not only a
canvas's; **a button and a menu are refused by name** (a fresh reader found
them answering 0), because neither is ever clicked — a button is pressed and
a menu's items are picked — so 0 from them would be *never*, not *not yet*.
**The spelling was not chosen in the plan and is chosen here:** `across` and
`down` are the words every canvas method already uses for its two coordinates,
so a person reads `.across` beside `.line(from_across, ...)` and learns no
second vocabulary. If the author wants `.x` and `.y`, or one `.clicked_at`, it
is two registry rows and the one branch in `answer_a_question`
(`bytecode/window_questions.cpp`) that answers them.

**AN OUTLINE AND A LINE'S WIDTH ARE THE PEN**, as `.colour` and `.font` are:
`.thickness(n)` and `.outline(1)` change what is drawn after them and nothing
before, because both are copied onto each stroke as it is made. `.outline`
takes 1 or 0, which is `.on`'s stopgap for the `true` satellite still cannot
spell. An even thickness gets no half-pixel: a one-pixel line wants the half
so that it lands on the pixel it names, and a two-pixel line already sits
evenly across the boundary. A thickness of 0 is refused — a line nobody can
see — and so is one past a thousand. `.thickness` and `.outline` read bare
answer what the pen is now.

**AN ARC IS PART OF A CIRCLE, CLOCKWISE, IN WHOLE DEGREES FROM THREE O'CLOCK.**
That is cairo's own convention, and on a screen whose `down` grows downward it
is a clock's. Filled it is a SLICE from the centre — what a pie chart and a
clock face want; outlined it is the curve alone — what a drawing wants. **The
angles are positions on the face**: 370 is 10 and -90 is 270 — **and cairo
does not read them so**, which a fresh reader caught in the first draft's
sentence: cairo reads `to - from` as a sweep, so 0 to 370 handed straight
through is a full turn and ten degrees more, a whole disc where a sliver was
asked for. Both angles are brought onto the face before the stroke, and an
arc from an angle round to itself is the whole circle — `arc(.., 0, 0)` and
`arc(.., 0, 360)` both draw one, and no sweep a program can write draws
nothing.

**AND DRAWING ONE FROM -90 FOUND A DEFECT TWO DAYS OLD.** The reader every
place, range and angle goes through (`place_of`, now in
`bytecode/window_readers.cpp`) borrowed the number fast path's `fits_a_count`,
which refuses every NEGATIVE number because a count is never negative — so
since 2026-09-20 `satellite.window.slider(-50, 50)`, the very example written
in that reader's own comment, was refused as *"further than any screen
reaches"*, and nobody had written a negative one. It reads the magnitude
whatever the sign now, and check.sh has the row.

**PROVED ON A COMPOSITOR, AND READ BACK BY EYE:** `before.png` shows a
four-pixel magenta outlined box, circle and three-quarter arc with its gap in
the top-right quarter, and a filled green slice in that same quarter; a REAL
POINTER CLICK at the canvas's centre ran `when_clicked` with `.across`
answering 200 and `.down` 148 — the two pixels are the compositor rounding a
relative pointer move, and the proof script judges within five — and the pen
read back 1 and false after `.thickness(1)` and `.outline(0)`.

## GTK-16 — more than one screenful: scroll, tabs, panes, a frame — **BUILT: SCROLL, FRAME AND SPLIT 2026-09-21; TABS 2026-09-22, as the recommendation**

`gtk_scrolled_window_new()`, `gtk_notebook_new()`, `gtk_paned_new()`,
`gtk_frame_new()`. Each holds other pieces, so **every one of them depends on
GTK-7** — a container that is not the window's `GtkFixed`.

Cheap after GTK-7 and worth naming separately only because `satl-term` already
has tabs (`satl-term/tabs.cpp`) and that file is where to start.

**AS BUILT** — `scroll` `1 27 16`, `frame(title)` `1 27 17`, `split` `1 27 18`.
A frame is the one holder with words of its own, drawn on its edge, so `.text`
reads and writes them.

**THEY HOLD A FIXED NUMBER AND THE ONE TOO MANY IS REFUSED.** A scroll and a
frame hold one; a split holds two, first on the left. **Refused rather than
ignored**, because `gtk_scrolled_window_set_child` on a scroll that already has
one **silently drops the first** — the piece is still a piece, the program still
holds it, and it is simply not on the screen any more and nothing said so.

### TABS ARE NOT BUILT, and it is a language question rather than a missing afternoon

**A tab needs a NAME, and `.append` has no shape for "a piece and a name".**
`.append` already means one thing for a row (the piece) and another for a window
and a grid (the piece and where it goes); a third meaning is the point at which
one method stops being one method. The shapes, none chosen:

    a_tabs.append(the_piece, "Open files")     a third arity for .append
    a_tabs.add(the_piece, "Open files")        a different method for it
    the_piece.title("Open files")              the PIECE carries its own name

**The third is the interesting one** — it makes a tab's name a property of the
piece rather than of the adding, which is how `.title` already works on a
window. **Not decided**, and check.sh has a row asserting no `tabs` word exists,
so nobody adds one without answering this.

### TABS BUILT 2026-09-22, as the recommendation: the piece carries its name

On the author's *"do the next one, or do 3 even"*; the question stays in the
table below as still reversible. `tabs` is `1 27 21` — two rows, it takes
nothing. `gtk_notebook_new()`.

    satellite.variable.window t = satellite.window.tabs()
    satellite.variable.window first = satellite.window.label("the first page")
    first.title("First")
    t.append(first)
    t.chosen("First")                    which tab is in front, read and written
    t.changed(when_switched)             a person clicking a tab

**`.title` IS EVERY PIECE'S NOW.** On a window it is the frame's words, as it
always was; on any other piece it is the name on its tab. One method because it
is one idea — what this thing is called where it is held — and it is how a tab
got its name without `.append` growing a third shape. A piece already in a set
of tabs is relabelled in place; one not in any simply remembers, for when it is
appended. A menu is sent to `.text`, the word it already has.

**AN UNNAMED PIECE IS REFUSED AT THE TABS**, with the line to write, rather than
given a blank tab a person cannot tell from the next: *"a tab is named by its
piece's title, and a label has none yet -- give it one first:
the_piece.title("Open files")"*. The same answer a menu with no heading gets.

**`.chosen` AND `.changed` ARE A CHOICE'S, REUSED**, because it is the same
question with the same answer: which one is picked, as text. `.chosen` reads
the name on the tab in front and `.chosen("Second")` brings that tab forward; a
name that is not there is refused, because `gtk_notebook_set_current_page` past
the end goes to the last page and says nothing. `.changed` is `notify::page`.

**THE BAR IS ALWAYS SHOWN**, unlike satl-term's, which hides it with one page: a
program that made tabs wants to see tabs.

**PROVED ON A COMPOSITOR:** two pages named First and Second; `.chosen` read
First; `.chosen("Second")`, and it read Second; `first.title("Alpha")` renamed
the tab while it was in the set; the set measured 302 by 189; a REAL POINTER
CLICK on the Alpha tab ran `when_switched` with `.chosen` answering `"Alpha"`,
and the capsule closed the window — exit 0.

## GTK-17 — `satellite.console` IS A WINDOW, with libvte — **BUILT 2026-09-22: the console a program makes, and the console satl launches**

> *"we especially need satellite.console to be a window with libvte"*

~~**AND VTE IS NOT IN THE FOLDER.**~~ **VENDORED 2026-09-22, at the author's
"go for it".** The DEP half is done, and it cost more than one tarball:

- **DEP-1 extended by five.** `vte-0.84.1.tar.xz` (the newest stable; hash
  matched against the `.sha256sum` upstream publishes), and the four VTE 0.84
  requires that the stage did not have: **lz4** 1.10.0 (scrollback
  compression, required since 0.74), **simdutf** 8.2.0 (UTF-8, required since
  0.80), **fmt** 12.1.0 (compiled header-only; the archive is installed and
  nothing links it) and **fast_float** 8.1.0 (headers only; it installs no
  `.pc`, so the recipe writes one — our own file beside upstream's). VTE ships
  the last three as bundled subprojects with wrapdb `meson.build`s dropped in;
  those are an edit somebody else maintains, `--wrap-mode=nofallback` refuses
  them, and each is built from its own release tarball with its own build
  system at the version VTE's own `.wrap` files pin. Recipes in
  `vendor/build_stack_recipes.py`; 45 seconds for the five, VTE 27 of them.
- **The static archive is one word.** `src/meson.build:490`
  `shared_library(` → `library(`, so `--default-library=static` is honoured:
  `vendor/edit_journal/vte/001-static-library.patch`, and **`build_stack.py`
  now applies a project's journalled patches the moment its tarball is
  unpacked** — the first patch in the journal, and the "command instead of an
  act of memory" README_FIRST promised. `libvte-2.91-gtk4.a` is 3.6 MB.
- **PROVED FROM THE ARCHIVE**, not assumed: a 60-line C program (a window, a
  `VteTerminal`, `/bin/sh -c 'printf ...; exit 7'` spawned in it) linked
  against `libvte-2.91-gtk4.a` and the whole static stack, **NEEDED exactly
  satl's seven**, run on a headless mutter with no session bus: the shell's
  line came back through `vte_terminal_get_text_format`, `child-exited`
  reported 7, exit 0. `vte_get_features()` says `+BIDI -GNUTLS -ICU -SYSTEMD`.
  It SIGSEGVed first, for WIN-1's reason exactly — the vendored GTK looks for
  xkb data at `/nonexistent` unless told — and ran once pointed at the staged
  xkeyboard-config, which is what satl's spill will do around it.
- **What the three optional dependencies were for, read from the source, and
  why two are off without a question:**
  - **systemd** (`src/systemd.cc`): after spawning the shell, VTE asks the
    session bus to put it in a transient systemd scope of its own — a cgroup
    per terminal, so the shell's tree is its own unit. Without it the child
    stays in satl's cgroup. `satl-term/child.cpp` never asks for a scope, and
    a bare machine has no bus (DEP-3). **Off.**
  - **icu**: legacy charsets for `vte_terminal_set_encoding`. satellite is
    UTF-8. **Off.**
  - **gnutls** (`src/vtestream-file.h`): VTE keeps scrollback in memory and
    spills what overflows to an unlinked temp file; with gnutls each block is
    AES-256-GCM encrypted with a key that lives only in the process, so the
    file on disk is useless to anyone who reads `/proc/<pid>/fd` or recovers
    the blocks later. **Without it, VTE feeds a red line into every new
    terminal** — `src/vte.cc`, `Terminal::Terminal()`: *"WARNING: GnuTLS not
    enabled; data will be written to disk unencrypted!"* — the proof above
    shows it as the terminal's first line. And upstream has **deprecated the
    option** in 0.84, which reads as "gnutls will be required". **Off for
    now, and that is Q-VTE-1, the author's:** (a) vendor gnutls, which brings
    nettle and gmp — three more autotools projects, all LGPL, an hour, and
    what the next VTE will demand anyway; (b) patch the warning out, an edit
    that grows on every upgrade; (c) live with the line. **Recommendation:
    (a).** It is what *"include a copy of it"* means, and nothing on the
    machine is asked for. Nothing in satl links VTE yet, so nothing shows the
    line today.

The author, the same afternoon, on what this opens: *"this will finally allow
satl to start a satellite.console.new, and we can incorporate code written in
satellite into the interpreter"*. That spelling — `satellite.console.new` — is
reading (2) below, a console a program starts by name, spelled under
`satellite.console` rather than `satellite.window`; and the second half is
his to say more about before anyone builds toward it.

**AND IT COLLIDES WITH WIN-9, WHICH IS STILL THE AUTHOR'S.** `satellite.console`
is `1 5` and is **today's stdout**: `.display`, `.input`, `.typed`, `.width`,
`.height`, `.clear`, `.home`. A `satellite.console` that is a VTE window is
**003's console handover**, which 004 removed on purpose — the branch this is
being written on is literally named `milestones-install-and-no-console-handover`.
What a handover costs was measured and is not small: a detached window loses the
program's **exit status** and its **stdout**, and check.sh asserts 44 on one and
DESIGN §8 spends four bullets on the other.

**So there are two readings of the author's sentence and they are very
different**, and he has to say which:

1. **`satellite.console.display` starts drawing into a VTE window** instead of
   writing to the terminal it was run from. That is the handover, and WIN-9's
   recommendation was *do not*.
2. **A new word makes a terminal-shaped piece** —
   `satellite.window.console(80, 24)` — that a program **appends into a window
   like any other piece**, and `satellite.console.display` keeps writing to
   stdout exactly as it does. **Nothing is lost and nothing is forced.**

**(2) is the recommendation, and 003 wrote it that way**: `make_words.py` still
carries 003's removed row `satellite.window.console.new(title, width, height)`
— the console was a **window word**, not a takeover of `satellite.console`.
Reading (2) also makes DEP-5's *"single application"* true without deleting
anything: satl can draw the terminal satl-term draws.

### BUILT 2026-09-22, as reading (2), in the author's spelling — and both halves are one widget

The author, that afternoon: *"I think we are ready to build satellite.console
and the console that satl launches"*. Both, and they turned out to be one thing.

**A CONSOLE IS A PTY, AND THAT IS THE WHOLE DESIGN.**
`satellite.console.new("a title", 800, 600)` — **`1 5 10`**, the next number
free under `satellite.console`, and the first word 004 has put under it (003
ended at `1 5 9`) — answers a window whose whole inside is a
`VteTerminal` with a `VtePty` of its own. satl holds the pty's OTHER end, the
slave: `.display("words")` is one `write()` to it, and `.typed(when_typed)` is
the desk reading a whole line off it the moment the kernel's line discipline
says one is finished. The kernel did the echo, the backspaces and the editing
— as it has for every terminal since before GTK existed — and satl
re-implements none of it. **`satellite.console.display` keeps writing to
stdout, untouched.**

    satellite.variable.window c = satellite.console.new("a console", 800, 600)
    c.display("type a line and press Enter")     one write() to the pty; ends the line
    c.typed(when_typed)                          the capsule a finished line runs
    ... in the capsule:  the_console.typed       that line, as .key is the last key
    c.clear()   c.home()   c.columns   c.rows
    c.colour("#000000")  c.background("#90D5FF")  c.font("IBM Plex Mono", 12)   VTE's own setters

**A console is a window everywhere a window is one.** `is_a_window()` in
`satellite_window.hpp`, and every `piece == window` in the folder that meant
"the thing with a frame" now asks it, so `.close()`, `.title`, `.resize`,
`.fullscreen`, `.closed`, `.every`, `.key`, `.message`, `.ask` and `.menu` all
work on a console with no second branch anywhere; check.sh asserts no
`piece != window` refusal is left and names the three `== window` that remain.
`.append` is refused by name — a console holds nothing but its terminal — and
`.text` is sent to `.display`. Five method
tokens, `0x0B4A`–`0x0B4E`: `display`, `typed`, `home`, `columns`, `rows`;
`.clear()` is the canvas's token, answered by the receiver. The window's own
`frame_new` was split out (`window_frame.hpp`) so the two frames share one
GtkWindow-and-column, and the console's `destroy` handler is connected
**before** the desk's, so its pty is let go of while the handle is whole.

**THE CONSOLE satl LAUNCHES: `satl --console [file] [words...]`.** The same
window, and then satl's own stdin, stdout and stderr are `dup2`'d onto its
pty — **in THIS process**. No exec, no fork, no satl-term: the exit status is
satl's own, and stdout is the pty, which is the window. That is WIN-9's two
objections to a handover answered by construction, and it was measured rather
than argued: on a compositor, `satl --console fail.satl` exited **22** through
the window, the refused line's own code. `satl --console` alone is the prompt
in a console. `console_launch.cpp`.

- **Explicit, never forced.** satl with no terminal and no flag prints where it
  was pointed, as it has since 004 removed the handover on purpose (PLAN
  M0.5). Whether satl should ever open a console **on its own** — 003's six
  reasons — stays WIN-9, the author's; this is the shape WIN-9 recommended in
  the meantime. When he wants the launcher to start satl itself (DEP-5's
  *"single application"*), it is one line in the `.desktop`: `Exec=satl
  --console %f`. **satl-term is untouched.**
- **satl-term's end-of-run policy, ported.** A file that finished closes the
  console at once; a run that STOPPED holds it with `[satl] stopped on machine
  code 22 (division_by_zero) -- press any key to close` as its last line,
  written to satl's own stderr so it queues behind whatever the program
  printed last; the prompt holds the same way when it ends. Any key but a
  modifier closes it. A stopped run's other windows are taken down first, as
  they are with no console.
- **The controlling terminal, as far as it goes.** `setsid()` then
  `TIOCSCTTY`. From a launcher — measured on this desktop: nautilus and ptyxis
  are their own session leaders, `pid == pgid == sid` — the pty becomes satl's
  controlling terminal and Ctrl-C is SIGINT by the kernel's hand. From a
  shell, satl already leads a process group, `setsid` fails, and a key
  controller on the window in the capture phase sends SIGINT itself when the
  pty is cooked and lets the byte through when the prompt has it raw — which
  is exactly what the line discipline would have done. Closing the console
  hangs up on the interpreter (SIGHUP), as closing any terminal does: the
  kernel's when it can, satl's own when it cannot — and never when satl closed
  it itself, which ignores SIGHUP first, or the clean exit code would be lost
  to the hangup the master's closing raises.
- **Q-VTE-1's stopgap.** Without gnutls VTE feeds its red warning into every
  terminal at construction. The terminal is reset once, before anything of
  ours is on it, so the line never shows. One call to remove when gnutls is
  in. **The question is still the author's.**
- 120 by 48 cells — satl-term's, the author's ask of 2026-09-12 — by
  satl-term's own fit-on-first-frame; satl-term's black on light blue and IBM
  Plex Mono 11, the font satl carries.
- **NEEDED is still seven.** VTE, lz4 and simdutf were already in the link
  group; the console cost the build one include path. 56 MB.
- **A satl without VTE still builds** (`SATELLITE_HAS_CONSOLE`, asked
  separately from the window): every console word lexes, checks, and refuses
  by name with the package to install — 047's oldest rule, one library further
  down. Both halves compiled and `nm`'d on their own.

**PROVED ON A COMPOSITOR** — `prove-console.sh`, four stages, all green on the
first run: the prompt in a console, typed at with real keys — the typed line
wrote a file, `exit` ended the session, one key closed the hold, exit 0, and
**nothing** landed on satl's original stdout; a program that stopped held its
console and exited 22 through the window; a program that finished closed its
console itself with no key pressed; and a program's own console, typed `abc`
into, ran `when_typed` with `.typed` answering `abc` and `.columns`/`.rows`
answering 77 and 17. stdin was `/dev/null` throughout, which is what a launcher
gives.

**DECIDED HERE AS RECOMMENDATIONS, ALL REVERSIBLE:**

- **`.typed` is a capsule, not a wait.** GTK-11's open shape — may a satellite
  line wait for a person — is kept open; a blocking `c.input()` is the author's
  to ask for, and the pty is already there to read it from.
- **`--console` is explicit.** Forcing it is WIN-9's, still his.
- **`.display` takes text or a number.** A list or a file is refused as `.text`
  would refuse it; the whole `satellite.console.display` set is one reader away.
- **No `--hold`.** satl-term's flag; a run that stopped already holds.

**NOT BUILT:** GTK-18 (`satellite.terminal`, a shell in a console) is the next
word over — `vte_terminal_spawn_async` into this same widget, with the pty VTE
makes for the child instead of the one satl holds an end of.

### What a fresh reader found the same evening, and all of it was real

- **The desk could deadlock on satl's own console, and it printed nothing.**
  The pty's master is drained by VTE on the desk's thread and by nobody else,
  and a pty holds about twelve kilobytes. A program printing faster than the
  terminal draws fills it and the interpreter waits for room, which is plain
  flow control; then the desk prints one line to stderr — a Gtk-CRITICAL, a
  Gtk-WARNING, GDK_DEBUG's chatter — and blocks on the same buffer, which only
  it can empty. Reproduced on the binary with `GDK_DEBUG=frames` and twelve
  megabytes of output: the interpreter in `write(1)`, the desk in
  `file_tty_write`, for ever. **The fix is where the desk's words go**: the
  desk prints through the C streams (GLib's writer is `fputs(stderr)`, GDK's
  is `vfprintf(stderr)`), the interpreter through `std::cout` and `std::cerr`,
  which write to the descriptors; so the descriptors move to the pty and
  `stdout` and `stderr` are reassigned to dups of what satl was started with —
  a shell, or a launcher's journal, where a GTK application's warnings go
  anyway. `prove-console.sh`'s `loud` stage is that reproduction, green.
- **Ctrl-D at the start of a line stopped `.typed` for good.** Canonical mode
  answers a `read()` of nothing for VEOF and the reader took that as the pty
  gone. It stays now; nothing typed delivers nothing; and a mid-line Ctrl-D,
  which hands over half a line with no newline, waits for the rest of itself.
- **A frame could be appended into a row.** `a_row.append(a_window)` reached
  GTK, which only asks whether a widget has a parent, and a toplevel has
  none. The hole was there for a window before a console existed; refused by
  name now.
- **The reader could be armed on a stale fd.** `.typed`'s parcel captured the
  fd number on the interpreter; a person closing the console between the
  check and the parcel would have had a watch on whatever the program opened
  next. The parcel asks the desk's own view now, and the slave is closed only
  with the handle, never by the desk.
- **Ctrl-Shift-C closed the hold** it promised to survive; it copies now, and
  Ctrl-Insert is left to VTE.
- **The no-VTE stubs blamed the build for a button.** `a_button.display(...)`
  on a satl without VTE said "built without a console"; the kind is refused
  first now, on every build, and a size of 0 is the program's on every build.
- **A closed button asked `.columns` said "is not a console" under
  window_is_closed.** Closed is asked first now, so the sentence is the one
  the code means.
- Two proof stages passed without the hold happening (the closing key landed
  on nothing either way); they record liveness before the key now, and the
  stopped stage's original stdout must be empty. And the record had called
  `1 5 10` "the first word a milestone has put under a family from 003", which
  sixteen earlier rows of words_004.tsv contradict; it is the first under
  `satellite.console`.

## GTK-18 — `satellite.terminal` is a bash prompt — **the author's, named 2026-09-21**

> *"satellite.terminal to be a bash prompt"*

`satellite.terminal` does not exist as a word; it would be **`1 28`**, the next
free number under `satellite`, and it is the first new top-level word since
`satellite.window` took `1 27`.

    satellite.variable.window t = satellite.terminal.new(80, 24)
    my_window.append(t, 400, 300)

`vte_terminal_new()` and `vte_terminal_spawn_async()` with `$SHELL`. **Every
line of this exists already** in `satl-term/terminal.cpp` and
`satl-term/child.cpp`, ported from 003 — including what a held tab says when the
child dies and how a machine code is turned into a sentence. **That code is the
starting point, not a reference.**

**What is genuinely new is that the prompt is a REAL SHELL**, which is a
capability satellite has not had: a program that opens one has handed a person a
shell with the program's own privileges. That is not a reason not to build it —
it is a reason for the word to be as plainly named as it is, and for
`satellite.terminal` never to be something a program does by accident.

**GTK-17 must land first.** A terminal is a VTE widget in a window, and GTK-17
is the milestone that gets VTE into the folder and into a static archive.

---

## What is DECIDED here, and what is the AUTHOR'S

**Decided, and the milestones below rest on them:**

- A widget is a `Piece`, never a new arm (`satellite_window.hpp`, 2026-09-20).
- Every GTK call is inside `on_the_desk()` (WIN-2).
- A capsule runs on the interpreter's thread, always (WIN-11) — and **GTK-15 is
  the one place that rule does not fit**, which is why GTK-15 asked. Built as a
  display list, no capsule draws, and the rule holds unbroken.
- Arity is checked **before the program runs**, for every window method
  (`7480119`). GTK-7 must not undo it.
- The word numbers are frozen when the row lands, in build order.
- **satl turns the desktop portal off before it opens a display** (Q-WIN-11a,
  the author, 2026-09-22: *"defend"*). It never waits on a bus it did not ask
  for, and the file dialog is GTK's own chooser in satl's process.

**The author's, and none of them are decided:**

| | question | recommendation |
|---|---|---|
| ~~GTK-3~~ | ~~how a radio group is spelled~~ | **BUILT as the recommendation, 2026-09-22** — `satellite.window.one_of({"small", "large"})`: one word, one piece, many buttons, asked `.chosen`. Still reversible. |
| ~~GTK-16~~ | ~~how a TAB gets its name~~ | **BUILT as the recommendation, 2026-09-22** — the piece carries it, `the_piece.title("Open files")`. Still reversible. |
| GTK-3 | **a `true` and a `false` to type** — a LANGUAGE milestone | there should be one; `c.on(1)` is the stopgap |
| ~~GTK-9~~ | ~~does a capsule get the new value?~~ | **BUILT as (1)** — it gets the piece and asks it. Still reversible. |
| ~~GTK-11~~ | ~~may a satellite line wait for a person?~~ | **BUILT as the capsule.** The waiting shape stays open. |
| ~~GTK-11~~ | ~~**Q-WIN-11a blocks the file dialog** — may a word reach the portal?~~ | **DECIDED by the author 2026-09-22, in one word: *"defend"*.** `gtk_disable_portals()` before `gtk_init_check`; the file dialog built the same day as `my_window.choose_a_file(when_chosen)`, its path in `.answer`. |
| GTK-11 | **Q-WIN-11c: the accessibility bus** — GTK prints three criticals on a bus with no a11y registry and carries on; silence it as the portal was, or leave screen readers on? | leave it on; it did not hang |
| ~~GTK-14~~ | ~~how a key is spelled to a program~~ | **BUILT as the recommendation, 2026-09-21** — the character, or a lower-case name for the rest; modifiers are keys, shown by pressing them. Still reversible. |
| ~~GTK-15~~ | ~~a draw capsule, or a display list~~ | **BUILT as the display list, 2026-09-22.** A draw capsule stays the author's to ask for; it needs a re-entrant walker first. |
| ~~GTK-12~~ | ~~a menu inside a menu, and a line between groups of items~~ | **BUILT 2026-09-22**: `file.menu(recent)` — the piece carries its heading — and `file.separator()`. Still reversible. |
| ~~GTK-15~~ | ~~where a click on a canvas LANDED; an outline, a line's width, an arc~~ | **BUILT as the recommendation, 2026-09-22** — `.across` and `.down` carried on the event as `.key` is; `.outline(1)` and `.thickness(n)` are the pen; `.arc(...)` clockwise from three o'clock. Still reversible — and `across`/`down` is the one spelling chosen here rather than recommended. |
| ~~GTK-17~~ | ~~does `satellite.console` become a window, or does a window get a console?~~ | **BUILT as the recommendation, 2026-09-22** — a window gets a console, spelled as he spelled it: `satellite.console.new(title, width, height)`; `satellite.console.display` keeps stdout; and `satl --console` is the console satl launches, in one process. Still reversible. |
| GTK-17 | **`.typed` is a capsule** — may a console line WAIT for a person (`c.input()`)? GTK-11's question, met again | a capsule; the pty is there to read from when he wants the wait |
| GTK-17 | **`--console` is explicit** — should satl open a console on its own with no terminal, 003's six reasons? This is WIN-9 | do not force it; the `.desktop` can say `satl --console %f` |
| GTK-17 | **Q-VTE-1: gnutls** — VTE without it prints a red warning into every new terminal and upstream has deprecated the option; vendor gnutls + nettle + gmp, patch the line out, or live with it? **Meanwhile the terminal is reset once at birth and the line never shows.** | vendor the three |
| GTK-10 | the window font: 11px or 12px | asked 2026-09-19, still open |

---

# Part 3 — what is NOT done, stated plainly

- **DEP-1 IS DONE** (2026-09-21). `vendor/build_stack.py` builds all 24 projects
  from the frozen tarballs in `vendor/new/` into `vendor/stage` in 172 seconds, and
  `make` links them. ~~One gap remains and it is named below.~~
- ~~**A FRESH CLONE STILL CANNOT RUN IT**: `vendor/new/*.tar.*` is committed but
  nothing UNPACKS it, and `vendor/<project>/` is gitignored. Until there is an
  unpack step the build works only where the trees already exist.~~ **STALE
  WHEN THE CHART WAS WRITTEN, AND PROVED CLOSED 2026-09-22.** `1e5a960`
  (2026-09-21 16:36, four minutes after this bullet) gave `build_stack.py` an
  `unpack()` step and `--unpack`; the bullet survived that evening's rewrite.
  The test DEP-1 itself asked for — a fresh clone, no network — was then never
  run until 2026-09-22: 24 of 24, `make`, check.sh green. DEP-1 has the numbers.
- **`vendor/xkb/xkb-data` is GONE** — it was xkeyboard-config 2.41 copied out of
  this machine's `/usr/share/X11/xkb`. satl now carries xkeyboard-config 2.48 built
  from the frozen tarball: 293 files rather than 254, `geometry/` included.
- **`fribidi` is no longer pinned to `master`** — it is the 1.0.17 release tarball,
  like every other dependency.
- **NEEDED is seven, not six** — eight until 2026-09-22, when DEP-4 found libresolv
  bound nothing. DEP-2 is the way to six. The author ruled on
  2026-09-21: *"eventually we will build all the satellite-number's into the satl
  interpreter as well, but until we do, they are left separate"* — so DEP-2 is
  DECIDED AND DEFERRED, not open.
- ~~**`bare-machine.sh` has never been run against the real satl.**~~ **DONE
  2026-09-22 — DEP-3.** `prove-bare-machine.sh` runs the real satl in an empty
  root, with and without a GPU driver, and it draws; the floor is DEP-4's.
- ~~**No widget can talk back.**~~ **DONE 2026-09-21 — WIN-11.**
  `my_button.pressed(when_pressed)` runs a capsule on the interpreter's thread,
  and `my_button.press()` is the program pressing it itself.
- **THERE ARE TWENTY-TWO WIDGETS** as of 2026-09-22: a window, a button, a
  label, a text box, a text area, a checkbox, a switch, a slider, a number box,
  a progress bar, a choice, a row, a column, a grid, a picture, a scroll, a
  frame, a split, a menu, a canvas, a set of tabs and a one-of.
  ~~No picture, no row, no menu, nothing that talks back but a button.~~ **Part 2G
  is the eighteen milestones**, GTK-0 is the recipe each one repeats, and
  **GTK-1 to GTK-16 are built, GTK-11 in full since the author ruled on
  Q-WIN-11a on 2026-09-22; and that day GTK-3's radio and GTK-15's four
  leftovers, each as its written recommendation** — the first paid GTK-0's bill, the second proved a
  value can be read back out of GTK at all, the third found that **satellite has
  no `true` to type**, and the fourth found a **three-day-old hole in the
  lexer** that had been refusing `satellite.window.new("a title", 800)` as a
  capsule nobody wrote.
- ~~**VTE IS NOT IN `vendor/new/`.**~~ **VENDORED 2026-09-22** with the four
  it needs, static by a one-word patch, proved from the archive — **and linked
  and called since the same evening: GTK-17 is built**, `satellite.console.new`
  and `satl --console`, proved on a compositor. GTK-18 can start. What is not
  done: Q-VTE-1 (gnutls) is the author's, and its warning is reset off the
  screen meanwhile.
- ~~**2.6 MB OF satl IS UNREACHABLE.**~~ **EARNED 2026-09-21 by GTK-6.**
  gdk-pixbuf, libpng and libjpeg-turbo are reached by `satellite.window.picture`
  and were proved with a real PNG and a real JPEG. libtiff is the same word and
  has not been proved with a `.tif`. **pcre2, libgirepository and
  libcairo-script-interpreter are still called by nobody** and no milestone
  below ever will — Part 00's Table B is the list.
- **`THIRD-PARTY-NOTICES.md` is not written**, waiting on DEP-6's two rulings.
- **WIN-9 — force the satl-term console or not — asked 2026-09-19, still
  unanswered.** The recommendation was: do not — and since 2026-09-22 the
  explicit shape it recommended exists, `satl --console`, in one process, with
  the exit status kept (GTK-17). Forcing it stays his.

---

# Part 4 — the order, and why

**DEP-1 IS DONE, so the order the rest are in has changed.** The author's
instruction on 2026-09-21 was *"only build all of the gtk milestones"*, and the
GTK family is now what this file is for.

## The GTK order, and why it is this one

1. **GTK-1 — a label.** Not because a label is important, but because it is the
   cheapest widget that is not a button, so it pays GTK-0's whole bill once:
   the piece table, the file split, `.text`, and a `.append` refusal that names
   pieces it has never heard of.
2. **GTK-2 — a person types.** The first value read back OUT of GTK, which is
   the only genuinely new mechanism in the first six.
3. **GTK-3, GTK-4, GTK-5** — on/off, a number, a list. Each is one factory and
   one method once GTK-2's reading-back works. Cheap, and they are most of what
   a person means by "a form". GTK-3's radio came last of the family, on
   2026-09-22, as `one_of` — once `.chosen` existed to ask it.
4. **GTK-7 — rows and columns**, before GTK-6. A picture placed by coordinate is
   fine; a picture in a row is what anybody actually wants, and GTK-7 changes
   `.append`'s arity, so it should change it while there are five widgets rather
   than fifteen.
5. **GTK-6 — a picture.** Four vendored projects stop being dead weight.
6. **GTK-9 — every piece talks back.** Mostly a table once GTK-3/4/5 exist, and
   it is what makes the first five worth having.
7. **GTK-8, GTK-10, GTK-16** — the window's own shape, the look (**and the
   window font ruling, owed since 2026-09-19**), and the containers that hold
   more than a screenful.
8. **GTK-13, GTK-14, GTK-11, GTK-12** — time, the keyboard, dialogs, a menu.
   Each brings one new question and they are in order of how small the question
   is.
9. **GTK-15 — a canvas.** Last of the GTK family, because its ruling — a draw
   capsule or a display list — is the only one that can deadlock the design.
   **Built 2026-09-22 as the display list**, with GTK-16's tabs and GTK-12's
   submenu and separator the same day, each as its written recommendation —
   and its own four leftovers (where a click landed, an outline, a width, an
   arc) later that day, the same way.
10. **GTK-17, then GTK-18** — the author's two. They are last **not** because
    they matter least but because they are the only ones with a DEP half: VTE
    has to be vendored, and patched to build a static archive, before a line of
    either can be written. **That half is done (2026-09-22) — and GTK-17 was
    built the same evening**, both halves: `satellite.console.new` and `satl
    --console`, proved on a compositor. GTK-18 is the next word over.

## The DEP order, for what is left of it

**Three of the four were done on 2026-09-22, in this order, the author having
said the GTK family was finished enough to *"move on to another .md"*:**

1. ~~**DEP-3** — prove the current binary on a bare machine.~~ **PROVED.** It
   validated WIN-1 — nothing was missing — and found the floor instead: the
   distro's libstdc++ suffices, EL10 yes, EL9 no; and the GPU driver is not on
   it at all.
2. ~~**DEP-1's remaining gap** — a fresh clone still cannot unpack `vendor/new/`.~~
   **It could since `1e5a960`; PROVED with no network.**
3. ~~**DEP-4**~~ **MEASURED — seven is the floor; libresolv, the eighth, bound
   nothing and is off the link line.** Then **DEP-6, DEP-8, DEP-5, DEP-9**, which is what is
   left: the notices (two rulings of the author's first), the distribute package
   at 54 MB, satl-term's *"single application"* (the author's), and X11.
4. **DEP-2 is DECIDED AND DEFERRED** and is not on this list.

**WHAT IS LEFT IN THE GTK FAMILY IS THE AUTHOR'S** (2026-09-22, evening):
GTK-18 (GTK-17 is built and VTE is in), a `true` and a `false` to type, the
window font, Q-VTE-1 (gnutls), and Q-WIN-11c (the accessibility bus). Q-WIN-11a was ruled that
evening — *"defend"* — and the file dialog built on it, so GTK-1 to GTK-16 are
all built. Every recommendation the plan wrote has been built and every one
of its question rows is still reversible. The DEP order above is what followed,
the same evening: DEP-3 proved, DEP-1 proved on a fresh clone, DEP-4 measured.
