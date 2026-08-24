# Building satellite on Enterprise Linux

Tested on AlmaLinux 10. The same steps work on RHEL 10, Rocky 10 and any
rebuild of the same sources; CentOS Stream 10 differs only in that CRB is
enabled by a different command, noted below.

## The short version

```sh
git clone git@github.com:satellitemadness1/satellite.git
cd satellite
c++ -std=c++20 -Wall -Wextra -O2 -o enterprise_satellite_builder build_and_install.cpp
./enterprise_satellite_builder
```

That builds the driver, and the driver runs the four steps in the order they
have to happen in: uninstall, clean, build, install. `--help` prints them and
`--dry-run` prints them without running any.

If you would rather type the four commands, they are exactly:

```sh
./install.sh --uninstall
make clean
make
./install.sh
```

Both `Makefile` and `install.sh` are indexes. The build is sixteen fragments
under `make_support/` and the installer is nine under `install_support/`, each
one a single subject and none of them over 200 lines. You still type `make` and
`./install.sh`; both folders travel with the tree, and the download folder that
`make bundle` writes carries `install_support/` beside the installer for the
same reason it already carries `install_tree/`.

## What you need first

Two binaries come out of this tree and they have different dependencies.
`satl`, the interpreter, needs a C++20 compiler and nothing else — it links
six shared objects and five of those are the C and C++ runtimes. `satl-term`,
the GTK terminal, needs GTK 4 and VTE.

```sh
sudo dnf install gcc-c++ make git
```

That is enough for `make satl`. For the terminal as well:

```sh
sudo dnf --enablerepo=crb install gtk4-devel vte291-gtk4-devel
```

**`vte291-gtk4-devel` lives in CRB**, which is disabled by default on
Enterprise Linux. That is the one package people get stuck on. On CentOS
Stream 10 the repository is called `crb` too but is enabled with
`sudo dnf config-manager --set-enabled crb` rather than a per-command flag.

You do not have to install them. `make` is tolerant of a missing GTK: it
builds `satl`, tells you what `satl-term` would have needed, and installs what
it built. A machine with no display has no use for the terminal anyway.

## Where it installs, and why you probably do not want sudo

The prefix follows **who runs make**, and both `install.sh` and the Makefile
use the same rule:

| you run it as | prefix | on PATH already? |
| --- | --- | --- |
| an ordinary user | `$HOME/.local` | yes, on any current distribution |
| root (`sudo`) | `/usr/local` | yes |

So `./enterprise_satellite_builder` with no `sudo` installs to your home
directory, needs no privileges, and is the right answer on a machine you share
or do not administer. `sudo ./enterprise_satellite_builder` installs system
wide.

Do not mix them. If you install as your user and later install as root, the
root copy wins on PATH and the two can disagree about which version is
running. `satl --version` says which one you have and when it was built:

```
satl 002 revision 01
  built     2026-08-24 08:24:31 UTC
  compiler  Clang 24.0.0git (...)
  invoked   /home/madness/opt/clang-24-2/bin/clang++
  flags     -std=c++20 -Wall -Wextra -O3
```

`satl --where` says which of three places it resolved its library directory
from, which is the question worth asking when a program cannot find its
library.

## Choosing a compiler

The default is whatever `c++` is on your PATH, which on Enterprise Linux 10 is
GCC. The Makefile prefers a clang at `$HOME/opt/clang-24-2/bin` if one is
there, and falls back silently if it is not.

That choice, and every other knob in this build, lives in `make_support/`. The
`Makefile` at the top of the tree is an index: it names sixteen fragments and
includes them in the order they are numbered, and each one is a single subject
under 200 lines. The compiler is `make_support/020-compiler.mk`; the index
lists the rest. You still type `make`, and nothing about the commands it runs
changed when the file was split.

To force one:

```sh
make CXX=g++
make CXX=/usr/bin/clang++
```

**Do not set `CXXFLAGS` to change the optimisation level.** A command-line
`make CXXFLAGS=-O3` replaces that variable whole, taking `-std=c++20` with it,
and the build then dies on `std::atomic<std::shared_ptr<T>>` being a C++20
feature — twenty lines of `static_assert` away from anything that names the
real cause. The knob is `OPT`:

```sh
make OPT=-O2
make OPT="-O3 -march=native"
```

`-march=native` is deliberately not a default, because the Debian packages
built from this same Makefile are `Architecture: any` and have to run on
machines that are not yours.

Changing either `OPT` or `CXX` rebuilds the whole tree. That is on purpose:
`make` invalidates a target when a prerequisite changes and flags are not a
prerequisite of anything, so a stamp file carries them instead. Without it a
tree built at `-O2` would quietly stay at `-O2` except for whatever happened
to be touched since — a mixed binary that no output distinguishes from a clean
one.

## Checking it worked

```sh
make test
```

Sixteen test binaries, each printing one `PASS` line naming what it proved.
One of them is a ThreadSanitizer build of the global registry, and it needs a
compiler that ships `compiler-rt`; the Makefile probes for one by linking an
empty program and falls back to `/usr/bin/clang++` or `g++` if `$(CXX)` cannot
do it. `TSAN=0 make test` skips it.

Then:

```sh
satl --run example/hello_world.satl
satl --run example/full_test.satl
```

The second prints one line per language feature that works and exits 0.

## Uninstalling

```sh
./install.sh --uninstall
```

Add `--prefix /usr/local` (with `sudo`) to remove a system-wide install. It
takes out the binaries, the library directory, the icons and the mime type.

## If something goes wrong

**`vte-2.91-gtk4` not found by pkg-config** — CRB is not enabled. See above.

**The build succeeds but `satl` is not found** — `$HOME/.local/bin` is not on
your PATH. Log out and back in, or add it.

**`satl` runs but cannot find its library** — run `satl --where`. It prints
the directory *and which of the three tiers answered*: the `SATELLITE_PATH`
override, a path relative to the binary, or the compiled-in default. Those are
three different broken installs and the path alone does not say which.

**An old version keeps running** — you have two installs. `command -v satl`
says which one is winning; `satl --version` says what it is.
