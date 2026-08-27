# satellite — the layout

Every file in this tree, and one line on what each is for.

**This is a map, not a declaration.** Nothing here is load-bearing: what actually
gets compiled is declared in `make_support/040-sources.mk`, and what actually gets
installed is declared in `satellite_enterprise/install_support/060-install-tree.sh`.
Those two are the authority and this file is a reader's index of the tree. Keeping
a second list that *decided* anything is how a tree rots — the day someone adds a
file to one list and not the other, one of them is silently wrong.

So: when a file is added, add a line here too, and know that forgetting costs a
reader a minute rather than costing a build a file.

Companions: [DESIGN.md](DESIGN.md) is what the language is, [PLAN.md](PLAN.md) is
how it gets built, [WORD_NUMBERS.md](WORD_NUMBERS.md) holds every number in it, and
[SATC.md](SATC.md) specifies the `.satc` file those numbers get written to.

---

## The root

| file | what it is |
| --- | --- |
| [README.md](README.md) | The front door: what satellite is, hello world, where the state of things is written down. An index, and deliberately holds no fact of its own. |
| [DESIGN.md](DESIGN.md) | The language: the generating rule, syntax, the numbering, scope, types, and what it refuses. Permanent. |
| [PLAN.md](PLAN.md) | The work: architecture, the build, the install, milestones, measurement discipline. Permanent. |
| [WORD_NUMBERS.md](WORD_NUMBERS.md) | **The numbering**, and the authority over every number in the language. DESIGN §4 explains it; `words.def` transcribes it; when they disagree this file is right. Permanent. |
| [SATC.md](SATC.md) | The `.satc` file format: a program with its language-owned words replaced by their numbers, cached beside its source. Specified before it is built, because it constrains the numbering. Permanent. |
| [QUAD.md](QUAD.md) | The goal: `quad_infinity` must be expressible in satellite, what that program needs, and the four things the language has not settled that it needs. Permanent. |
| [LAYOUT.md](LAYOUT.md) | This file. |
| [PLAN_ONE.md](PLAN_ONE.md) | The first draft plan, **superseded** by the two above and deletable as soon as nothing cites it. |
| [Makefile](Makefile) | An index. Includes the eight fragments under `make_support/` in numbered order and does nothing else. |
| [LICENSE](LICENSE) | MIT (Expat). |
| [.gitignore](.gitignore) | Build output, and the deliberate exclusion of `old_versions/` from this repository's history. |

## `src/` — the interpreter

One folder per module, named for the job the module does rather than for the
abbreviation its files use. Every unit spells its includes from the top of `src/`
— `"system_facts/version.hpp"` and never `"../version.hpp"`.

| file | what it is |
| --- | --- |
| [src/programs/main.cpp](src/programs/main.cpp) | `satl` itself: reads the command line, answers `--version` / `--help`, and reports honestly that running a file lands at M8. |
| [src/programs/opening.cpp](src/programs/opening.cpp) | The banner and the usage text — the words, kept in a `.cpp` because they change every milestone. |
| [src/programs/opening.hpp](src/programs/opening.hpp) | Declarations for the above, plus the exit-status enum so two arms cannot disagree about what a failure is worth. |
| [src/programs/cpu_level.cpp](src/programs/cpu_level.cpp) | `satl-cpu-level`: prints `haswell` or `baseline`. Compiled at the baseline on purpose — it runs before anything is known about the machine. |
| [src/system_facts/version.hpp](src/system_facts/version.hpp) | The two version numbers and what a build records about itself, including both the compiler make invoked and the one that answered. |

Two directories exist and are **empty**, holding names for work that has not
started: `src/satellite_number/` and `src/satellite_string/`. `src/satellite_words/`
is where M2's trie will go and does not exist yet.

## `make_support/` — the build

Numbered because the order is load-bearing in two places: 010 before 020, which both
fragments say at their own top, and 045 before 050, which 045 says at its own top.

| file | what it is |
| --- | --- |
| [make_support/010-compiler.mk](make_support/010-compiler.mk) | Which compiler, and `OPT` as the one flag knob. Explains why `CXXFLAGS` is not one. |
| [make_support/020-version.mk](make_support/020-version.mk) | The two version numbers, the build stamp, and `version_defs` — a function, because satl is built twice and the two must not describe themselves identically. |
| [make_support/030-directories.mk](make_support/030-directories.mk) | One variable per module directory, so a directory that moves is one edit. |
| [make_support/040-sources.mk](make_support/040-sources.mk) | **What gets compiled and linked**, named one by one rather than wildcarded. Also holds the measured startup numbers. |
| [make_support/045-microarchitecture.mk](make_support/045-microarchitecture.mk) | The two microarchitecture builds: the `-march=x86-64-v3` flags, the `-dumpmachine` test that decides whether there are two, and the object lists. |
| [make_support/050-build.mk](make_support/050-build.mk) | The default goal and the three link rules. The first fragment that declares a target. |
| [make_support/060-compile.mk](make_support/060-compile.mk) | How a `.cpp` becomes a `.o`, for both variants, plus the two flag stamps that catch a changed command line. |
| [make_support/070-clean.mk](make_support/070-clean.mk) | Removing what a build made, named one by one rather than by deleting a directory. |

## `satellite_enterprise/` — the Enterprise Linux install

| file | what it is |
| --- | --- |
| [satellite_enterprise/install.sh](satellite_enterprise/install.sh) | An index. Locates the tree, then sources the eight fragments below in order. |

### `install_support/`

A sourced fragment runs as it is read, so the order below *is* the script.

| file | what it is |
| --- | --- |
| [010-defaults.sh](satellite_enterprise/install_support/010-defaults.sh) | The root (`$HOME/.satl`) and the flags, with the measurement behind `--link` and `--desktop` defaulting off. |
| [020-saying-things.sh](satellite_enterprise/install_support/020-saying-things.sh) | `die`, `usage`, and the `run`/`show`/`quoted` trio that makes `--dry-run` an honest, pasteable transcript. |
| [030-arguments.sh](satellite_enterprise/install_support/030-arguments.sh) | The command line, and the refusal to accept a root that belongs to something else. |
| [040-machine.sh](satellite_enterprise/install_support/040-machine.sh) | Reads `/etc/os-release` without sourcing it, notes a non-EL system, and checks there is a compiler and a make. |
| [050-building.sh](satellite_enterprise/install_support/050-building.sh) | Builds, then runs `satl-cpu-level` and picks the variant to install. |
| [060-install-tree.sh](satellite_enterprise/install_support/060-install-tree.sh) | **The one declaration of what gets installed**, read by both the install and the uninstall. |
| [070-desktop.sh](satellite_enterprise/install_support/070-desktop.sh) | The optional symlinks under `~/.local`, and the ownership check that refuses to overwrite another install's files. |
| [080-report.sh](satellite_enterprise/install_support/080-report.sh) | What happened, verification by running the installed binary, and what the word `satl` actually gets you. |

### `icons/` — installed

Layout mirrors the install destination exactly, so installing is a copy and not a
translation.

| file | what it is |
| --- | --- |
| [application-x-satellite.xml](satellite_enterprise/icons/application-x-satellite.xml) | The `.satl` mime packet. **Read its comments before changing anything about icons or the mime type** — each records something found the hard way. |
| [org.satellite.terminal.desktop](satellite_enterprise/icons/org.satellite.terminal.desktop) | The launcher for `satl-term`. **Not installed until M11** builds the binary it names. |
| [org.satellite.terminal.svg](satellite_enterprise/icons/org.satellite.terminal.svg) | A complete scalable icon that is **deliberately never installed** — shipping it alongside the PNGs makes which one a shell draws unpredictable. |

And the pixel artwork, two files at each of nine sizes:

| path | what it is |
| --- | --- |
| `icons/hicolor/<size>/apps/org.satellite.terminal.png` | The application icon, at 16, 22, 24, 32, 48, 64, 128, 256 and 512. |
| `icons/hicolor/<size>/mimetypes/application-x-satellite.png` | The icon drawn on a `.satl` file, at the same nine sizes. |

Eighteen files. The two basenames are the two names a desktop looks up, and neither
may drift: `org.satellite.terminal` matches the GApplication id and the `.desktop`
filename, and `application-x-satellite` is the mime type with `/` replaced by `-`.

### `icon_artwork/` — source, never installed

The user's own exported work, copied byte-for-byte from the first satellite.
Nothing here is installed and nothing here should be re-encoded.

| file | what it is |
| --- | --- |
| `for_redo/satellite-icon.xcf` | The editable GIMP source for the satellite icon. |
| `satellite-icon.avif` | The icon as exported to AVIF. |
| `5829875.png` | The original photograph the satellite icon was cut from. |
| `satellite_icon_{64,128,256,512}.png` | The satellite icon at four sizes. |
| `scaled/satellite-icon-{64,128,256,512}.png` | A scaled set of the same. |
| `satl-app-icon/satl-icon-{64,128,256,512}.png` | The app-icon variant at four sizes. |
| `file_icon/352-3528073_piece-paper-frames-illustrations-piece-of-paper-icon.jpg` | The stock sheet-of-paper image the file icon was built from. |
| `file_icon/file-icon.png`, `file_icon/file-icon-512.png` | Intermediate file-icon work. |
| `file_icon_final/final_file_icon_{64,128,256,512}.png` | The finished file icon at four sizes. |

## `SCRATCH.md/` — the things that do not last forever

A folder, and the `.md` in its name is deliberate: it sorts beside the four
permanent documents it is the opposite of. **Nothing in it decides anything**, and
every file in it is written to be deleted. The test for whether something belongs
here rather than in DESIGN, PLAN, LAYOUT or WORD_NUMBERS is whether it *stops being
true when the work it describes is finished.*

Its own [README.md](SCRATCH.md/README.md) lists what is in it and the condition for
deleting each one, so this table does not repeat them. `plans/` used to hold the
author's first note; that note has been converted into the permanent documents and
the file deleted, and its conversion is recorded in
[SCRATCH.md/FIRST_NOTE.md](SCRATCH.md/FIRST_NOTE.md) until nothing needs it.

## `old_versions/`

| path | what it is |
| --- | --- |
| `old_versions/first_satellite/` | The complete first satellite — 644 files, its own build, its own `design/`. A **reference**, not a dependency: nothing here includes from it and `satl` compiles with the folder absent. Deliberately outside this repository's history. |

## Build output

None of this is source, all of it is gitignored, and `make clean` removes it by
name.

| path | what it is |
| --- | --- |
| `satl` | The interpreter, baseline build — runs on any x86-64. |
| `satl.haswell` | The interpreter, `-march=x86-64-v3 -mtune=haswell`. Built only on x86-64. |
| `satl-cpu-level` | The detector the installer runs to choose between the two. |
| `src/*/*.o` | Objects. `main.o` and `main.haswell.o` are the same source compiled against the two instruction sets. |
| `.cxxflags-stamp` | The exact compiler and flags the baseline objects were built with, so a changed command line forces a rebuild. |
| `.cxxflags-stamp-haswell` | The same for the haswell objects — a second file, because one could only ever describe one of the two. |

## Installed, outside the tree

| path | what it is |
| --- | --- |
| `$HOME/.satl/satl` | Whichever build this machine can run. `satl --version` prints the flags it was compiled with, so it is its own record of which one. |
| `$HOME/.satl/share/mime/packages/application-x-satellite.xml` | The `.satl` file type. |
| `$HOME/.satl/share/icons/hicolor/…` | The eighteen icons. |

Twenty files. `satellite_enterprise/install.sh --uninstall` removes them by name.
