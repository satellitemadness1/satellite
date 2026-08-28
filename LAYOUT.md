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
| [QUAD.md](QUAD.md) | The goal: `quad_infinity` must be expressible in satellite, what that program needs, and — after reading its source on 2026-08-27 — the one thing the language has not settled that it needs. Permanent. |
| [LAYOUT.md](LAYOUT.md) | This file. |
| [PLAN_ONE.md](PLAN_ONE.md) | The first draft plan, **superseded** by the two above and deletable as soon as nothing cites it. |
| [Makefile](Makefile) | An index. Includes the ten fragments under `make_support/` in numbered order and does nothing else. |
| [LICENSE](LICENSE) | MIT (Expat) for satellite's own source, plus a third-party section for `pcg/`, which is Apache-2.0. It also records that no built binary currently contains any of it. |
| [pcg/](pcg/) | The only third-party code in the tree: three pcg-cpp 0.98 headers, its licence, and a README recording what was cut, why `-isystem`, and why a 512-bit variant was refused. |
| [.gitignore](.gitignore) | Build output, and the deliberate exclusion of `old_versions/` from this repository's history. |

## `FORMAT/` — how the code is written

| file | what it is |
| --- | --- |
| [FORMAT/CXX.md](FORMAT/CXX.md) | Everything needed to write C++ in this tree: the house style, the comment culture, how the build is edited to add a module, how a test is built, and the X-macro registry mechanism M2 ports from the first satellite. Its §9 listed the ten things that were **not** decided anywhere and blocked M2; all ten were settled when M2 was written and §9 now records each answer and where it lives. Permanent. |

The directory is separate from the root because these are documents about *writing the
code*, not about the language or the plan. The four permanent documents at the root
answer "what is satellite"; this one answers "what does a file here look like."

## `src/` — the interpreter

One folder per module, named for the job the module does rather than for the
abbreviation its files use. Every unit spells its includes from the top of `src/`
— `"system_facts/version.hpp"` and never `"../version.hpp"`.

| file | what it is |
| --- | --- |
| [src/programs/main.cpp](src/programs/main.cpp) | `satl` itself: reads the command line, answers `--version` / `--help`, and reports honestly that running a file lands at M8. |
| [src/programs/opening.cpp](src/programs/opening.cpp) | The banner and the usage text — the words, kept in a `.cpp` because they change every milestone. |
| [src/programs/window_handover.hpp](src/programs/window_handover.hpp) · [.cpp](src/programs/window_handover.cpp) | Started with no console, `satl` hands itself to `satl-term` (DESIGN §10.4). The test is a **controlling terminal**, not `isatty(stdout)` — the obvious version opens a window instead of feeding a pipe. Six named refusals. |
| [src/programs/opening.hpp](src/programs/opening.hpp) | Declarations for the above, plus the exit-status enum so two arms cannot disagree about what a failure is worth. |
| [src/programs/cpu_level.cpp](src/programs/cpu_level.cpp) | `satl-cpu-level`: prints `haswell` or `baseline`. Compiled at the baseline on purpose — it runs before anything is known about the machine. |
| [src/programs/window.cpp](src/programs/window.cpp) | `satl-term`: the command line, the `GtkApplication`, and the window. Its title and size are the same string and two numbers `satellite.window.console.new` takes. |
| [src/programs/terminal.cpp](src/programs/terminal.cpp) | The VTE widget and the `satl` it spawns into a PTY. Holds the exit policy — the clean-exit arm is M11.A's and M11.B deletes it. |
| [src/programs/terminal.hpp](src/programs/terminal.hpp) | One door to the above. Split from `window.cpp` by subject, not by line count. |
| [src/system_facts/version.hpp](src/system_facts/version.hpp) | The two version numbers and what a build records about itself, including both the compiler make invoked and the one that answered. |
| [src/satellite_random/random.hpp](src/satellite_random/random.hpp) | `satellite.random`: the three tiers, the `Bits32` seam, `MAX_RANDOM_DIGITS`, and the spin. Names no PCG type, so nothing above it includes an Apache-2.0 header. |
| [src/satellite_random/random.cpp](src/satellite_random/random.cpp) | The only translation unit that names a PCG entity. Compiled by `make` and **linked into nothing** — 040-sources.mk says why. |
| [src/satellite_words/words.def](src/satellite_words/words.def) | **The numbering, as data** — 254 nodes and 9 aliases, a transcription of WORD_NUMBERS §2.2 and nothing else. A node's number is its **position** among its parent's children and is not a column. The one file exempt from PLAN §3's line ceiling. |
| [src/satellite_words/words.hpp](src/satellite_words/words.hpp) | The umbrella: one door over the seven parts below, in an order that compiles. Include this and you get everything. |
| [src/satellite_words/words_nodes.hpp](src/satellite_words/words_nodes.hpp) | `NodeId`, `PathId`, the node table, the aliases, and how a row's text splits into a word and a call shape. |
| [src/satellite_words/words_numbers.hpp](src/satellite_words/words_numbers.hpp) | The numbers, computed from file order at compile time, plus `number_text` and `path_text` — which is what reproduces §2.2's path column character for character. |
| [src/satellite_words/words_spellings.hpp](src/satellite_words/words_spellings.hpp) | **The spelling interner** — many nodes, one string (DESIGN §4.4) — and every node's children as a first/next pair, because a node's children are not contiguous in a depth-first file. |
| [src/satellite_words/words_walk.hpp](src/satellite_words/words_walk.hpp) | `walk()`: a path read against the trie. A failed walk returns which segment failed and **which node it failed under**, which is what DESIGN §4.6's "did you mean" needs and what M5 will be built from. |
| [src/satellite_words/words_invariants.hpp](src/satellite_words/words_invariants.hpp) | The `static_assert`s, in a header so every consumer inherits them. Says which of PLAN M2's four properties are enforced here and which the encoding already made unrepresentable. |
| [src/satellite_words/words_digest.hpp](src/satellite_words/words_digest.hpp) | The numbering's identity as one 64-bit number, for a `.satc` header. Over the **numbering** and not the file's bytes — SATC §6 asked and this answers. |
| [src/satellite_words/words_runtime.hpp](src/satellite_words/words_runtime.hpp) | The user's half: the live child counter, and names numbered as they are met. PLAN §8.1's second table, and the half no `static_assert` can reach. |
| [src/satellite_words/dump.hpp](src/satellite_words/dump.hpp) · [dump.cpp](src/satellite_words/dump.cpp) | `satl --words`. The **registry's consumer, in the milestone that wrote it** — which the first satellite did not have for three commits, and four defects accumulated in that window. The only part of the module that prints. |

Two directories exist and are **empty**, holding names for work that has not
started: `src/satellite_number/` and `src/satellite_string/`.
`src/satellite_random/` is built and is the one module in the tree with **no
consumer** — it landed ahead of any milestone that calls it, the way `satl-term`
did. Everything under `satellite_words/` except `dump.cpp` is `constexpr` data and
pure functions over it, so a future `.satc` reader or disassembler can read the
numbering without linking anything.

## `example/` — the acceptance programs

Not samples. **Each of these is what a milestone means by done**, which is why
PLAN §8 and DESIGN §3 cite them by path rather than describing them in prose.

| file | what it is the done-when for |
| --- | --- |
| [example/hello_world.satl](example/hello_world.satl) | **M8.B**, and DESIGN §3 is a byte-for-byte copy of it. Declares `satellite.container.list<satellite.variable.string> arguments` again as of 2026-08-28; the program never reads it, so what it needs is one empty list — **which is M10's, so M8 runs after M10** and PLAN §8's opening carries the reordering. Its `//` comments are specified in DESIGN §5.6. |
| [example/advanced.satl](example/advanced.satl) | The console milestone that **does not exist** — `input(prompt)` `1 5 3` and `input(prompt, target)` `1 5 4`. Also uses `+` on strings, specified nowhere. |
| [example/thread_test.satl](example/thread_test.satl) | **M12**, and it cannot be M12's done-when yet. `.start()` `1 6 13 1` and `.join()` `1 6 13 2` **were numbered on 2026-08-28** and this row said otherwise until M2 transcribed them; what is still missing is that `satellite.thread.new(f(x))` needs the deferred call `1 6 16`, which no milestone owns. SCRATCH.md/THREADS.md. |
| [example/super_advanced.satl](example/super_advanced.satl) | **M9.5**, and it is the float's *exact* half — `+` is DESIGN §8.6's class 1, which never rounds, so it runs before the rounding rule is chosen. |

**None of them runs.** M2 landed on 2026-08-28 and **M3, the lexer, is the
milestone in progress**; nothing executes until **M8.A**, which is where a program
first runs at all. These are written first on purpose, because a milestone whose
done-when is a program somebody can read is one that cannot be argued about
afterwards.

## `tests/` — the suite

**New at M2**, and until then FORMAT/CXX.md §6's opening sentence was literally
true: *"there is no test infrastructure in this tree -- not a target, not a
directory, not a harness."* The first satellite's was ported rather than
reinvented, cut down to what one test needs.

`TESTNAMES` in `030-directories.mk` is the single place a test is declared to
exist; the source lists, the binaries, the run list and what `clean` removes are
all derived from it. The first satellite hand-copied that list, added two tests
to the run list and to neither build list, and `make test` ran binaries nobody
had built — reporting PASS from stale objects.

| file | what it is |
| --- | --- |
| [tests/words_test/words_test.cpp](tests/words_test/words_test.cpp) | The harness — three functions and a counter, no framework — and `main`. |
| [tests/words_test/words_test.hpp](tests/words_test/words_test.hpp) | The harness declarations and the three section prototypes. A *dependency*, which is why 065-tests.mk wildcards headers separately from sources. |
| [tests/words_test/authority.cpp](tests/words_test/authority.cpp) | **Opens WORD_NUMBERS.md and walks all 222 rows of §2.2**, plus §2.3's nine spellings. The one check no `static_assert` can make: a row left out of `words.def` does not leave a hole, it silently renumbers every sibling after it, and both files stay internally consistent. |
| [tests/words_test/walking.cpp](tests/words_test/walking.cpp) | PLAN M2's two worked examples by name, the interner in both directions, and what a failed walk says. |
| [tests/words_test/runtime.cpp](tests/words_test/runtime.cpp) | The live child counter and user names — PLAN §8.1's half, and the only check it gets until M4 calls it. |

## `make_support/` — the build

Numbered because the order is load-bearing in three places: 010 before 020, which
both fragments say at their own top, and 045 and 047 before 050, which each says at
its own top.

| file | what it is |
| --- | --- |
| [make_support/010-compiler.mk](make_support/010-compiler.mk) | Which compiler, and `OPT` as the one flag knob. Explains why `CXXFLAGS` is not one. |
| [make_support/020-version.mk](make_support/020-version.mk) | The two version numbers, the build stamp, and `version_defs` — a function, because satl is built twice and the two must not describe themselves identically. |
| [make_support/030-directories.mk](make_support/030-directories.mk) | One variable per module directory, so a directory that moves is one edit. |
| [make_support/040-sources.mk](make_support/040-sources.mk) | **What gets compiled and linked**, named one by one rather than wildcarded. Also holds the measured startup numbers. |
| [make_support/045-microarchitecture.mk](make_support/045-microarchitecture.mk) | The two microarchitecture builds: the `-march=x86-64-v3` flags, the `-dumpmachine` test that decides whether there are two, and the object lists. |
| [make_support/047-window.mk](make_support/047-window.mk) | Whether this machine can build `satl-term`: the `pkg-config vte-2.91-gtk4` probe, the flags it yields, and the window's two sources. |
| [make_support/050-build.mk](make_support/050-build.mk) | The default goal and the four link rules. The first fragment that declares a target. |
| [make_support/060-compile.mk](make_support/060-compile.mk) | How a `.cpp` becomes a `.o`, for both variants, plus the two flag stamps that catch a changed command line. |
| [make_support/065-tests.mk](make_support/065-tests.mk) | **New at M2.** The `test` target and the per-test source wildcards, all derived from `TESTNAMES`. Read before 070 so `clean` can name `$(TESTBINS)`. |
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
| [050-building.sh](satellite_enterprise/install_support/050-building.sh) | Builds, then runs `satl-cpu-level` to pick the variant to install, and looks for `satl-term` to decide whether the window installs. |
| [060-install-tree.sh](satellite_enterprise/install_support/060-install-tree.sh) | **The one declaration of what gets installed**, read by both the install and the uninstall: three programs — `satl`, `satl-cpu-level`, `satl-term` — the launcher, the mime packet and the artwork. |
| [070-desktop.sh](satellite_enterprise/install_support/070-desktop.sh) | The optional symlinks under `~/.local` — `satl`, `satl-term`, the launcher, the mime packet, the icons — and the ownership check that refuses to overwrite another install's files. |
| [080-report.sh](satellite_enterprise/install_support/080-report.sh) | What happened, verification by running the installed `satl` **and `satl-term`** — both answer `--version` without a display — and what the word `satl` actually gets you. |

### `icons/` — installed

Layout mirrors the install destination exactly, so installing is a copy and not a
translation.

| file | what it is |
| --- | --- |
| [application-x-satellite.xml](satellite_enterprise/icons/application-x-satellite.xml) | The `.satl` mime packet. **Read its comments before changing anything about icons or the mime type** — each records something found the hard way. |
| [org.satellite.terminal.desktop](satellite_enterprise/icons/org.satellite.terminal.desktop) | The launcher for `satl-term`. **Installed since 2026-08-28**, conditional on the binary it names having been built — M11.A built that on 2026-08-27 and `060-install-tree.sh` now carries the row. |
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
| `satl-term` | The GTK4 + VTE window (M11.A). Built once at the baseline, and only where `pkg-config` finds `vte-2.91-gtk4`; `make` skips it with a note elsewhere. |
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
