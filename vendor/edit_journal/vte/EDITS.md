# EDITS -- vendor/vte/

**Project:** vte-0.84.1
**From:** `vendor/new/vte-0.84.1.tar.xz` -- unpacked to `vendor/vte/vte-0.84.1/` on 2026-09-22
**Hash:** recorded in `vendor/new/SHA256SUMS`, first tier: compared against the
`.sha256sum` upstream publishes beside the tarball on download.gnome.org, and matched.

---

## 2026-09-22 -- src/meson.build:490, `shared_library(` becomes `library(`

**What:** one word. `libvte_gtk4 = shared_library(` is now `libvte_gtk4 = library(`,
so the build honours `--default-library=static` and produces
`libvte-2.91-gtk4.a`. Nothing else in the tree changes; the gtk3 rule at :413 is
left as it is because gtk3 is off. The change is `001-static-library.patch`
beside this file, and `vendor/build_stack.py` applies it the moment the tarball
is unpacked -- a fresh clone gets it without anyone remembering.

**Why:** upstream hardcodes `shared_library()` and installs no static archive at
all, which GTK_AND_NO_DEPENDENCIES.md had recorded (DEP-5, GTK-17: *"VTE
hardcodes shared_library() upstream ... so a vendored VTE has to be patched to
build an .a, or satellite.console's window is the one thing that un-statics
satl"*). satl links every library statically. There is no option for this;
the rule in README_FIRST.md was checked -- prefer an option to an edit -- and
there is none to prefer.

**Upstream:** not reported. `library()` would be a reasonable change upstream,
but they ship no static build on purpose: `-Bsymbolic-functions` and the
soversion are shared-only concerns, and `library()` simply ignores them when
static.

**On upgrade:** needed again until upstream changes it. The line number moves;
the word does not. If `library(` is already there, delete the patch and this
entry, and the file goes back to saying "No edits".

---

## What is NOT an edit here, so nobody journals it later

The options passed at configure time -- `-Dgnutls=false -D_systemd=false
-Dicu=false -Dgir=false -Dvapi=false -Ddocs=false -Dapp=false -Dglade=false
-Dterminfo=false -Dgtk3=false` -- change nothing on disk. The three bundled
subprojects in `subprojects/` (fmt, simdutf, fast_float, each with a wrapdb
`meson.build` dropped in) are NOT used: `--wrap-mode=nofallback` refuses them
and each is built from its own release tarball into the stage instead. And
`vte.cc`'s red *"GnuTLS not enabled"* line, fed into every new terminal when
gnutls is off, is deliberately NOT patched out: that is the author's question
(Q-VTE-1), and a patch there would be an edit that grows on every upgrade.
