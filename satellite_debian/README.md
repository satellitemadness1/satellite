# `satellite_debian/` — the Debian family build

Build the four binaries out of tree, into `build/`:

```
cd satellite_debian
make
```

```
build/satl              the interpreter, x86-64 baseline
build/satl.haswell      the same sources at -march=x86-64-v3
build/satl-cpu-level    which of the two this machine can run
build/satl-term         the window
```

## What to install first

```
sudo apt install build-essential pkg-config libgtk-4-dev libvte-2.91-gtk4-dev
```

`build-essential` carries `g++`, `make` and `libc6-dev` — and with them
`libstdc++.a` and `libc.a`, so `STATIC=1` and `STATIC=full` both work with no
further packages. That is the one real difference from
`satellite_enterprise/`, where the two static libraries are separate packages
and one of them lives in a repository that is off by default.

`libvte-2.91-gtk4-dev` is the only one `satl` itself does not need. Without it
`make` builds three binaries and says so; `make satl-term` fails and names the
package.

## Which releases

**Debian 13 (trixie) and Ubuntu 24.04 or newer.** One directory covers both
because every name this build depends on is spelled identically on each: the
packages above, the `vte-2.91-gtk4` pkg-config module, `c++` meaning `g++`, and
where the compiler keeps its static libraries.

`satl-term` is what sets the floor. vte gained its GTK4 build at 0.70, and
Ubuntu 22.04 ships 0.68 — there is no `libvte-2.91-gtk4-dev` to install there,
so on jammy the other three build and the window cannot. That is not a special
case in this build; it is the ordinary no-gtk4 path.

Measured on Debian 13 with g++ 14.2.0 and vte 0.80.1, 2026-08-28, and checked
against packages.ubuntu.com for noble (0.76), questing (0.80) and resolute
(0.84).

## Knobs

| | |
|---|---|
| `make OPT=-O2` | the one flag knob, as in the root build |
| `make STATIC=1` | link the C++ runtime in; libc stays dynamic |
| `make STATIC=full` | no dynamic dependencies at all, except in `satl-term` |
| `make -s static-available` | `full`, `1` or `no` for this machine |
| `make clean` | remove what this build made, and nothing the root build made |

`STATIC` exists because a binary that finds `libstdc++.so.6` in somebody's home
directory is a binary only that account can start. `satl-term` is never fully
static under either setting — it links gtk4 and vte, which `dlopen` their own
modules. `make_support/040-static.mk` carries the argument.

## Installing

```
./install.sh                 into $HOME/.satl, and symlink ~/.local/bin/satl
./install.sh --desktop       also publish the icons and the .satl file type
sudo ./install.sh --system   into /usr/local, for every account on the machine
./install.sh --uninstall     remove everything it installed, by name
./install.sh -n              print the commands and run none of them
./install.sh --help          every option
```

It builds first, runs `satl-cpu-level` to ask this CPU which interpreter it can
execute, and installs that one **under the name `satl`** — so the build makes
four files and the install is three programs. `satl.haswell` is not a fourth
program; it is `satl` compiled a second time, and the installed binary is its
own record: `satl --version` prints the flags its objects were compiled with.

`--static` is on by default and reaches `STATIC=full` on a stock machine of this
family, because `libstdc++.a` ships in `g++` and `libc.a` in `libc6-dev` and
`build-essential` depends on both. The result has no dynamic dependencies at
all. `satl-term` is never fully static — gtk and vte `dlopen` their own modules
— but its C++ runtime is linked in like the rest.

For `--desktop` and `--system` you also want:

```
sudo apt install gtk-update-icon-cache desktop-file-utils shared-mime-info \
                 hicolor-icon-theme
```

`gtk-update-icon-cache` is a package name as well as a command on this family,
and it is **not** part of `libgtk-4-bin`. `hicolor-icon-theme` owns the
`index.theme` without which GTK does not look inside an icon directory at all —
which is how a prefix can hold eighteen correct PNGs and a `.satl` file still
shows a blank page. `075-system.sh` is entirely about that file.

The installer never calls `sudo`, never edits `.profile`, `.bashrc` or any file
you own, and never asks you to export a variable. `--system` checks that the
prefix is writable and prints the command to run rather than escalating.
Nothing it did not create is ever overwritten: a path holding somebody else's
file is refused and listed at the end.

### It is the enterprise installer's sibling, not a copy that drifted

Every decision about layouts, symlinks, ownership, dry runs and never calling
sudo is made the same way as in `../satellite_enterprise/install.sh`, because
those were argued once and the arguments do not change at a distribution
boundary. Three things genuinely differ:

| | |
|---|---|
| where the binaries come from | `build/`, not beside the Makefile — it runs `make -C satellite_debian` |
| the package names | apt, and shorter: one `build-essential` where EL needs two packages and an off-by-default repository |
| the icon cache tool | Debian ships `gtk-update-icon-cache` **and** `gtk4-update-icon-cache`; `070-desktop.sh` takes whichever is there rather than hardcoding one, because a missing tool means the icons install, the cache is never rebuilt, and nothing reports a problem |

### The artwork is shared, not copied

`060-install-tree.sh` reads the launcher, the mime packet and the eighteen PNGs
from `../satellite_enterprise/icons`. That is a seam and it is deliberate: the
artwork belongs to the **program**, not to Enterprise Linux, and the folder it
sits in is a fact about which installer was written first. Twenty binary files
copied into a second directory are twenty files that can drift, and a `.satl`
icon that differs depending on which installer ran is exactly the silent
divergence this project refuses everywhere else. One variable, `artwork=`, is
the single line that changes if the files ever move somewhere shared.

## How it relates to the tree above

This directory is a build **of** the tree above it, not a copy of one. It reads
`../src`, `../pcg` and `../make_support`, and it writes only under `build/`.
Both builds can sit in one checkout: the root build puts `main.o` beside
`main.cpp`, and if this one did too they would be the same file compiled with
different flags, taking turns to be wrong.

The source lists are **inherited and never copied**.
`make_support/020-inherited.mk` includes the six parent fragments that declare
no targets — 010, 020, 030, 040, 045 and 047 — and then redirects all of them
with one line, `SRC := $(ROOT)/src`, which works because the parent's directory
variables are recursively expanded. So a source file added to the language
reaches this build with no edit here at all.

The five fragments the parent uses to declare *rules* are not included, because
they would put objects beside sources. This directory writes its own, numbered
to say which parent fragment each one answers:

| here | answers | |
|---|---|---|
| `010-tree.mk` | — | where the tree is, and the default goal |
| `020-inherited.mk` | — | the six includes and the one override |
| `030-output.mk` | — | the source-to-object map, and where things land |
| `040-static.mk` | `048-static.mk` | same three settings, apt in place of dnf |
| `050-build.mk` | `050-build.mk` | the default goal and the four links |
| `060-compile.mk` | `060-compile.mk` | how a `.cpp` becomes a `.o` |
| `070-clean.mk` | `070-clean.mk` | removing what this build made |

The installer is split the same way, into nine `install_support/` fragments
sourced in numbered order — `010-defaults` through `080-report` — mirroring
`../satellite_enterprise/install_support/` name for name.

There is no `065-tests.mk` here. The root build's `make test` is the suite, and
a second copy of it running the same sources against the same compiler would
prove nothing this one does not.

## Why the binaries may live in `build/` together

`satl` finds `satl-term`, and `satl-term` finds `satl`, by reading
`/proc/self/exe` and looking in their own directory — deliberately, in both
directions, refusing to trust `argv[0]` or `PATH` so that one install cannot
spawn another's interpreter. Four binaries in one directory is exactly the
arrangement they were written for. See `../src/programs/window_handover.cpp`
and `../src/programs/terminal.cpp`.
