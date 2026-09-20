# licenses/

**One folder per project, one `license.txt` in each, copied VERBATIM.**

    licenses/<project>/license.txt

The texts are byte-for-byte copies out of each vendored source tree. **Nothing is
reformatted, wrapped or tidied** -- a licence text that has been edited is no longer
the licence, and several of these require the notice be reproduced exactly.

Generated 2026-09-20 from `vendor/`, at the same time as DEP-1.

---

## Why there are twenty-five of these and not two

satellite is MIT. That is the licence for **the language**, and it is the one for the
website's licence page.

It is not the only licence **in the binary**. `make GTK=vendor` compiles twenty-four
other projects in, and "the GTK licence" is not one thing -- GTK is twenty-three
separate upstream projects with twenty-three sets of copyright holders. pango and GTK
are under the older GNU **Library** GPL; glib and gdk-pixbuf under the LGPL; zlib,
libpng, pcre2, libtiff, libjpeg-turbo and fontconfig each wrote their own.

**Static linking is what makes this matter.** A shared library is merely used; one
compiled in is distributed. LGPL-2.1 section 6 and the Library GPL's equivalent oblige
us to let a recipient relink against their own modified copy -- which shipping
satellite's source plus `vendor/` satisfies, and which is one reason DEP-1 committed
every source tarball into the project folder.

## Where it is said

| | |
|---|---|
| `satl --version` | four lines naming the licence families and pointing here |
| `satl --help` | the same four lines |
| the startup banner | **nothing, on purpose** -- it prints on every run of every program, and a notice nobody reads satisfies nothing |
| `licenses/` | the full texts |

## Three elections, made 2026-09-20

Two projects do not have a licence -- they offer a **choice**, and a choice has to be
made and recorded or the page is ambiguous.

**freetype -- ELECTED: the FreeType Licence (FTL).** Its `LICENSE.TXT` says: *"you must
choose one of the two licenses described below"*, the alternative being GPL-2.0. FTL is
the permissive one; GPLv2 would reach into satellite itself. `licenses/freetype/license.txt`
is `docs/FTL.TXT`, not the chooser page.

> The FTL asks for credit **in the documentation**: a sentence naming FreeType and the
> year of the version used. That obligation is not yet discharged and is the one open
> item in this folder.

**cairo -- ELECTED: LGPL-2.1.** Its `COPYING` offers *"either the GNU Lesser General
Public License (LGPL) version 2.1 or the Mozilla Public License (MPL) version 1.1"*.
LGPL-2.1 matches what glib and GTK already place on us, so electing it adds no new
obligation. `licenses/cairo/license.txt` is `COPYING-LGPL-2.1`.

> Note the LGPL side is **2.1-only** -- no "or any later version" appears in any `src/`
> header. And **MPL-1.1 is arguably the better election** for a statically linked MIT
> product: MPL 1.1 section 3.7 ("Larger Work") is friendlier to combination than
> LGPL-2.1's relink clause. It is close to moot in practice, because glib, GTK, pango
> and gdk-pixbuf put the LGPL relink obligation on us regardless of what cairo is --
> which is why LGPL-2.1 was chosen. **Reversible if you would rather narrow the set of
> files under the relink obligation.**

**glib -- ELECTED: LGPL-2.1-or-later.** Found by the audit, not by reading `COPYING`.
`gio/xdgmime/*` carries a second choice: its SPDX header reads exactly
`LGPL-2.1-or-later or AFL-2.0`. **It is linked** -- `gio/meson.build:445-446` does
`subdir('xdgmime')` and `internal_deps += [xdgmime_lib]`, and `libxdgmime.a` is on
satl's link line today. Electing LGPL-2.1-or-later adds nothing, since the rest of glib
is already LGPL-2.1-or-later.

## The list, and where each text came from

| project | version | text taken from |
|---|---|---|
| satellite | 004 | `LICENSE` (MIT) |
| cairo | 1.18.4 | `COPYING-LGPL-2.1` **(elected)** |
| expat | 2.8.4 | `COPYING` |
| fontconfig | 2.18.3 | `COPYING` |
| freetype | 2.14.3 | `docs/FTL.TXT` **(elected)** |
| fribidi | 1.0.17 | `COPYING` |
| gdk-pixbuf | 2.44.8 | `COPYING` |
| glib | 2.90.0 | `COPYING` |
| gperf | 3.3 | `COPYING` |
| graphene | 1.10.8 | `LICENSE.txt` |
| gtk | 4.24.0 | `COPYING` |
| harfbuzz | 14.4.0 | `COPYING` |
| ibm-plex-mono | -- | `OFL.txt` |
| libepoxy | 1.5.10 | `COPYING` |
| libffi | 3.8.0 | `LICENSE` |
| libjpeg-turbo | 3.2.0 | `LICENSE.md` |
| libpng | 1.6.58 | `LICENSE` |
| libtiff | 4.7.2 | `LICENSE.md` |
| libxkbcommon | 1.13.2 | `LICENSE` |
| pango | 1.58.2 | `COPYING` |
| pcre2 | 10.48 | `LICENCE.md` |
| pixman | 0.46.4 | `COPYING` |
| wayland-protocols | 1.49 | `COPYING` |
| xkeyboard-config | 2.48 | `COPYING` |
| zlib | 1.3.2 | `LICENSE` |

## Almost every tree is MIXED, and the mixing reaches the binary

A component is not "one licence". A twelve-agent audit on 2026-09-20 read every tree
file by file and raised **56 corrections** to a first pass that had read only the
top-level `COPYING`. The ones that matter are things **compiled into satl** that a
top-level read does not see:

| in the binary | licence | found in |
|---|---|---|
| `gtk/gtktextbtree.c` + 11 siblings | **Tcl/Tk** licence, from the Tk port | core GTK, named in `gtk/meson.build` |
| `gtk/inspector/*` (14 files) | **MIT** | part of libgtk, not optional |
| `gtk/roaring/roaring.c` | embeds **BSD-3-Clause** (`isadetection`, from pytorch) | clause 2 requires the notice in **binary** distributions |
| `pango/pango-script.c` | **ICU/IBM** permissive, attribution reaches binaries | below its LGPL header |
| `pango/json/*` (4 files) | Library GPL, © Benjamin Otte, imported from GTK | |
| `glib gio/xdgmime/*` | **LGPL-2.1-or-later OR AFL-2.0** | a choice -- elected above |
| `glib subprojects/gvdb` | LGPL-2.1-or-later, © Codethink Limited | linked into libgio |
| `glib gutils.c` | embedded **MIT** block, © Red Hat (xdg-user-dir-lookup) | |
| `expat lib/siphash.h` | **CC0-1.0** | `#include`d unconditionally by `xmlparse.c` |
| `harfbuzz src/hb-ucd.cc` | **ISC** -- src/ is not uniform | |
| `freetype src/dlg/*` | **Boost-1.0** | bundled logger |
| `libxkbcommon src/utils-checked-arithmetic.h` | **ISC**, omitted from upstream's own LICENSE | fixed below |

**Three files here were incomplete and have been repaired:**

- `libepoxy/license.txt` -- libepoxy is MIT **AND Apache-2.0**, and Apache-2.0 section
  4(a) obliges us to hand recipients a copy of the License. **libepoxy's tree ships
  none.** The Apache text is now appended, clearly marked as our addition.
- `libxkbcommon/license.txt` -- upstream's `LICENSE` assembles five notice blocks and
  omits the ISC grant above. Appended.
- `unicode/license.txt` -- **new.** fontconfig, harfbuzz and fribidi all compile Unicode
  Character Database derivatives in, and not one of the three ships the Unicode licence
  text; they cite a URL. Fetched from `unicode.org/license.txt`.

**A correction to an earlier claim:** `libffi src/dlmalloc.c` is **not** compiled on
this platform. Its `#include` sits behind `FFI_MMAP_EXEC_WRIT`, which `configure.ac`
sets only for Apple, the BSDs, Solaris and Android -- not Linux/glibc.

**And `SATELLITE_WINDOW.md:557-560` is wrong** about three of four components and would
poison anything drafted from it: libjpeg-turbo is not "MIT/BSD" but IJG AND Zlib for
what we link. Fix it when that file is next touched.

## Two things this folder does not yet settle

**gperf is a mixed tree.** Its top-level `COPYING` is the GNU **GPL**, which covers the
gperf *program*; its `lib/` is gnulib under **LGPL-2.1** (*"version 2.1 of the License"*).
The old meson-subproject build linked `subprojects/gperf/lib/libgp.a` into satl, which
is worth knowing about. In the bottom-up build gperf is a **build-time code generator**
for fontconfig -- it emits a hash function and contributes nothing to the binary -- so
the question should disappear. **Confirm it with `utility/linking` once the new stack
links**, rather than assuming.

**meson and pcg-cpp are in `vendor/` but not in the binary.** meson (Apache-2.0) is a
build tool; pcg-cpp is not part of the GTK stack. Neither is linked, so neither carries
a distribution obligation, and neither has a folder here. If pcg-cpp ever ends up inside
satl, it needs one.
