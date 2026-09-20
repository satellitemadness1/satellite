# satellite-004 — GTK AND NO DEPENDENCIES

**One executable that needs nothing installed, built from a folder that needs
nothing downloaded.** Written 2026-09-20 at the author's asking, diverging from
SATELLITE_WINDOW.md's plan because the subject turned out to be its own.

Milestones are `DEP-n`. SATELLITE_WINDOW.md keeps `WIN-n` and is still the
record for the *window*; this file is the record for what the window **costs**.

The author, 2026-09-20, in his own words:

> *"we need no dependencies, if you need something, include a copy of it so it
> exists inside of this project folder, /vendor/library/something.whatever_extension,
> so that we can build a completely static executable"*

> *"we don't care how big the executable is — 100mb transfers across the
> internet in... 2-3 seconds"*, and the ceiling he then worked out:
> **5 × 60 × 40 = 12,000 MB.** *"12 gigabytes, we are okay"*.

---

# Part 0 — how to get back to where this was written

`make GTK=vendor` builds it. That is the whole recipe **on this machine**,
because vendor/gtk is already built here. Anywhere else, follow
SATELLITE_WINDOW.md Part 0 first (~1 hour, ~2.2 GB) — and **DEP-1 exists so that
stops being true.**

    make                 1.1 MB   GTK loaded from the machine at run time
    make GTK=vendor     87.3 MB   GTK compiled in; 21.0 MB stripped

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

## The eight it still needs, and why each

    libc  libm  libresolv  ld-linux  libwayland-client  libwayland-egl
    libstdc++  libgcc_s

`make_support/050-build.mk` holds this as `ALLOWED_NEEDED` and **fails the build
and deletes the binary** if `readelf -d` ever names anything else.

- **libwayland-client, libwayland-egl — CANNOT be static, ever.** Two copies in
  one process segfaults: GDK makes its `wl_display` with ours and EGL calls
  `wl_list_insert` in the copy the GPU driver dlopened, which never saw the
  object. Measured in *both* link modes, so it is not the static-glibc hazard.
- **libstdc++, libgcc_s — removable, and DEP-2 is how.** See below.
- **libc, libm, ld-linux** — every Linux.
- **libresolv — NOT yet understood.** It is in the proved `hello` binary too, so
  it comes from the GTK stack rather than from satl. **DEP-4 asks whether it can
  go**; nobody has looked.

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

# Part 2 — the milestones

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

## DEP-3 — prove it on a machine with nothing

`vendor/gtk/hello/bare-machine.sh` already does this for `hello`: `bwrap` into a
root holding only the binary, the GL driver, the Wayland socket and glibc,
binding libraries one at a time. **Point it at the real `satl`, and drop
`--with-xkb --with-fonts`** — carrying those is exactly what WIN-1 built.

It needs `satellite-numbers/` beside the binary (satl loads from
`/proc/self/exe`'s folder), which is itself an argument for DEP-2.

**The script binds `/opt/amdgpu/lib64` and a fixed list of AMD/mesa paths — on a
non-AMD machine it needs editing before it proves anything.**

## DEP-4 — the six that remain, and whether it is really six

- **libresolv: nobody has looked.** It is in `hello` too, so it is the GTK
  stack's. If it is gio's DNS resolver it may be configurable away. Worth an
  hour; it would make five.
- **libwayland-client, libwayland-egl: settled, cannot go.** Part 1 says why.
- **libc, libm, ld-linux: settled.** A fully static glibc binary links and
  cannot draw (measured, 21 MB, SIGSEGV in EGL).
- **Write down the floor once it is known**, so nobody reopens it every time.

## DEP-5 — satl-term, and the author's "single application"

His original brief was *"wiring in GTK+ so that satl and satl-term become a
single application"*. Today they are two binaries, and **satl-term cannot be
static at all** — GTK4 and VTE hardcode `shared_library()` upstream, so there is
no `.a` to link.

**So "single application" has to mean something other than one static binary
containing VTE.** Options, none chosen: satl grows the terminal widget itself;
satl-term stays dynamic and is the one thing that needs GTK installed; or VTE is
vendored and patched to build a static library. **The author's call.** And
whatever is decided, **satl-term is not removed** — he has ruled on that once
already: the GPU terminal next door does not replace it.

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
- **Non-AMD GPUs**: nothing here should care, but `bare-machine.sh` does
  (DEP-3).

---

# Part 3 — what is NOT done, stated plainly

- **DEP-1 is not started.** A fresh clone cannot build this at all.
- **`vendor/xkb/xkb-data` is staged from this machine** and is gitignored.
- **`fribidi` is pinned to `master`.**
- **NEEDED is eight, not six.** DEP-2 is the way to six and is unstarted.
- **`bare-machine.sh` has never been run against the real satl.**
- **No widget can talk back.** A button draws and pressing it reaches no
  satellite code — there is no path from a GTK signal into a capsule. That is
  WIN-work, not DEP-work, but it is the most visible thing still missing.
- **`THIRD-PARTY-NOTICES.md` is not written**, waiting on DEP-6's two rulings.
- **WIN-9 — force the satl-term console or not — asked 2026-09-19, still
  unanswered.** The recommendation was: do not.

---

# Part 4 — the order, and why

1. **DEP-7's version decision** — five minutes of the author's time, and it
   decides what DEP-1 freezes.
2. **DEP-1** — mechanical, ~85 MB, makes everything after it reproducible.
3. **DEP-3** — prove the current binary on a bare machine. Cheap, and it either
   validates WIN-1 or finds the next missing file.
4. **DEP-2** — the architectural one, and the only one that removes a dependency
   rather than vendoring it.
5. **DEP-4**, then DEP-6, DEP-8, DEP-5, DEP-9.

**DEP-1 is the first milestone**, and it is the one to do after `/clear`.
