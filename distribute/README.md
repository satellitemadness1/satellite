# distribute/

Packages of satellite for machines that are **not** going to build it.

The two installers in the tree above --- `satellite_enterprise/install.sh` and
`satellite_debian/install.sh` --- compile the sources on the machine they are
run on. That is right for somebody who is going to change satellite, and it is
the wrong shape for somebody who is going to *use* it: it needs a compiler, the
pkg-config files for gtk4 and vte, and on Enterprise Linux the CRB repository
turned on before `vte291-gtk4-devel` can even be found.

What is here instead is a package of **programs that are already built**, and an
installer that copies them into place. Nothing is compiled on the target machine
and nothing is downloaded.

    make-package.sh            run HERE, on a build machine
    enterprise_satellite/      the package: AlmaLinux 10 / RHEL 10 family
    deb/                       empty, for the Debian family, not written yet

## Making a package

    ./make-package.sh

It builds the tree, checks the two properties that make a binary shippable,
fills `enterprise_satellite/` with the programs, the artwork and the examples,
and writes `satellite-<version>-almalinux10-x86_64.tar.xz` beside itself.

xz rather than gzip for a reason that is about delivery, not compression: the
site this gets uploaded to is WordPress, whose upload allowlist rejects
`.tar.gz`. It is also half the size — 2,129,664 bytes against 4,177,217 — because
most of the package is `satl` and `satl.haswell`, the same sources compiled
twice, and xz's dictionary window is large enough to see across the megabytes
between them.

`--no-build` packages the binaries that are in the tree root already, instead of
running `make` first.

## Installing one

Copy the tarball to the target machine, then:

    tar xf satellite-003r01-almalinux10-x86_64.tar.xz
    cd satellite-003r01-almalinux10-x86_64
    ./install-satellite.sh

That installs into `~/.local` and puts satellite in the applications grid.
`--system` puts it in `/usr/local` for every account, and says what to run as
root rather than escalating into it. `-n` prints every command and runs none of
them. `--uninstall` takes it all back out.

### The two things it does outside the prefix

Both are **on by default**, which reverses what `satellite_enterprise/install.sh`
and `satellite_debian/install.sh` do. That reversal is deliberate: their defaults
are for somebody who wants to know exactly what an installer touched, and these
are for somebody who wants satellite to work when it finishes.

* **`sudo dnf install`** for the packages `satl-term` links against, plus the
  four tools that make an installed icon visible at all — `hicolor-icon-theme`,
  `shared-mime-info`, `desktop-file-utils`, `gtk-update-icon-cache`.
  `035-dependencies.sh`. It is the only step that asks for a password. The
  package list and the exact command are printed before it runs, `sudo` asks for
  the password itself, and a failure there does not fail the install — the
  interpreter needs none of it. `--no-deps` declines.

* **A block in `~/.bashrc`** putting the install directory on `PATH`, so `satl`
  works in any terminal window. `075-shell-path.sh`. It is written between two
  markers, `--uninstall` removes exactly the lines between them, a reinstall
  rewrites rather than appends, and the original is kept once at
  `~/.bashrc.satellite-backup`. Skipped for a prefix already on the default
  `PATH`, like `/usr/local`. `--no-path` declines.

The launcher is also **rewritten as it is installed**, with an absolute `Exec=`.
The shipped `.desktop` carries a bare `Exec=satl-term`, and its own comments
explain why — the prefix is not known when that file is *written*. It is known
at install time. This matters on AlmaLinux specifically: measured 2026-09-08,
`systemd-path search-binaries-default` is
`/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin`, and nothing in `/etc/profile`
or `/etc/profile.d` adds `~/.local/bin` — the stock `~/.bashrc` does, and a
desktop shell launching a `.desktop` entry never reads `.bashrc`. So a bare
command name into a home prefix is a bet on how the session was started, and
`TryExec` means losing the bet *hides the entry*.

`READ-ME-FIRST.txt` inside the package is the same instructions written for
somebody who has not used a terminal before.

## Why this can ship a binary at all

Two facts, checked by `make-package.sh` on the way out and by the installer's
`050-payload.sh` on the way in:

1. **`satl` is fully static.** `ldd` answers *not a dynamic executable*. It maps
   no shared object, so there is no libstdc++, no libc, and no version of either
   for the target to match. See `make_support/048-static.mk`.

   The check is not a formality on this tree. `LD_RUN_PATH` is exported in the
   build environment and GNU ld silently turns it into an RPATH, so a
   *dynamically* linked satl comes out pointing at a hand-built GCC inside one
   developer's home directory --- and `ldd` on the build machine reports success,
   because there those paths exist. `make-package.sh` reads `readelf -d` and
   refuses a binary with an RPATH for exactly that reason.

2. **`satl-term` cannot be static and does not pretend to be.** GTK4 and VTE
   ship no static libraries and hardcode `shared_library()` in their own build
   files, so there is nothing to link even in principle. The window therefore
   uses the GTK4 and VTE on the target machine, which is why this package names
   one operating system instead of claiming to be portable: AlmaLinux 10.2 built
   it and AlmaLinux 10.2 runs it, against the same `vte291-gtk4` 0.78 that
   `ptyxis` --- the distribution's default terminal --- already pulls in.

## The artwork is copied and never edited

`make-package.sh` takes the launcher, the mime packet and eighteen PNGs from
`../satellite_enterprise/icons`, and `cmp`s every one of them after the copy.
Nothing is scaled, re-encoded or generated. The set is deliberately **mixed** ---
the 128, 256 and 512 application icons were re-exported on 2026-08-31 and the
smaller sizes were not --- so a script that helpfully filled in a missing size
from a larger one would replace chosen artwork with a guess about it.

That directory is a seam worth naming: the artwork belongs to the *program*, not
to Enterprise Linux, so the folder it currently sits in is a fact about which
installer was written first. `satellite_debian/install.sh` reaches across to the
same place for the same reason. If it ever moves, `artwork_src` in
`make-package.sh` is the one line that changes.
