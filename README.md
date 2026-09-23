# satellite

**satellite 004 revision 08** — the third satellite, and the one being built from
here on. Its version, revision and build number live in
`satellite/config/satellite_config.hpp`; `make` raises the build number every time
it builds, and a new revision restarts the count at 1.

Revision 06 is the one that carries its own world: `make GTK=vendor` compiles GTK
and its whole stack *into* satl, so the binary needs nothing installed to open a
window. Every source it is built from is committed under `vendor/`, frozen at one
version — see [GTK_AND_NO_DEPENDENCIES.md](GTK_AND_NO_DEPENDENCIES.md). `satl
--license` shows all 26 licences that come with that.

Revision 07 is the one where a file is a namespace: every included file is reached
by its own name (`other.greet()`), `satellite.namespace name { }` (second spelling
`satellite.space`) makes a namespace inside one, and a program's own capsules are
its own -- an included file can no longer answer for them or replace its
`satellite.main`.

Revision 08 is the one where objects arrive and 003's list comes across:
`satellite.spacesuit` objects made by declaring them, with their sections, their
`satellite.constructor`, a spacesuit inside a spacesuit and one extending another;
`satellite.library.x = <value>` written once at a file's top and read by every
capsule, changed by nothing; and `satellite.container.list()` with `.sum`, `.max`,
`.min`, `.join(separator)` and `.reserve(n)` beside the lists of lists 004 already
had (`grid[x][y]`).

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")

    satellite.return(satellite)
}
```

The language is the same one: **a dotted path rooted at `satellite` names
something the language owns, and a bare identifier names something the user
owns.** The tie-breaker at every fork is still **do absolutely everything for the
user** — and never behind their back.

What changes in 004 is how it runs.

## What satellite 004 is being built to be

- **As fast as compiled C++.** The bar is within ×1.05 of the same work written in
  C++. Every word of the language — `satellite.console.display`,
  `satellite.variable.string.find(x)` — is a small compiled C++ library of *likely
  scenarios*, named by the word's number (`1.5.1.so`) and loaded once at start-up,
  so running a command is one call into code that is already in memory.
- **Numbers instead of names while it runs.** A program becomes a `.satc` (every
  word as its numbers), a `.satb` (the same, marked with where work can run in
  parallel) and a `.sati` (strings as 32-bit characters).
- **Built for many threads.** A pool of threads starts with the interpreter; some
  look for independent work — loops whose iterations do not touch each other,
  files, anything that waits — while the rest run it, and a program's output stays
  in order.
- **No built-in limits.** Every maximum — threads, memory, digits — is a value you
  set, used as a ceiling on what the machine actually offers.
- **Strings of every language.** A satellite string is 32 bits a character.

## Where it is today

Revision 02 is early. What runs:

- a prototype runner for `satellite.console.display`;
- the word table: every word numbered, 364 of them;
- `satellite.variable.string` as 32-bit characters, with its 23 words each built
  as its own library and checked answer-for-answer against satellite 003.

`satellite_number` is next, then spacesuits (satellite's classes). See
[PROGRESS.md](PROGRESS.md) for exactly what is built and checked,
[PLAN.md](PLAN.md) for the order of work, [DESIGN.md](DESIGN.md) for the standards
and every measurement behind them, and [ERROR.md](ERROR.md) for every known error.

```
make                                  # build/satl and every numbered library
make test                             # check.sh and the string checks
build/satl examples/hello_world.satl  # run a program; build/satl --help lists every way
build/satl --console examples/hello_world.satl   # the same, in a console window of satl's own
sh satellite_enterprise/install.sh --root <folder>   # satl and its libraries, into <folder>
```

**004's interpreter is `satl`** (`build/satellite-004` is a link to it). It is
not installed anywhere by `make`, and where 004 installs is not decided yet, so
the installer takes only a `--root` and refuses 003's `~/.satl` and `/usr/local`.
The word `satl` on a PATH is still satellite 003's.

## The earlier satellites

Nothing earlier is thrown away. It is finished, and kept exactly as it was:

- **[`old_versions/second_satellite/`](old_versions/second_satellite/)** —
  **satellite 003 revision 07**, the second satellite: a complete interpreter
  (`satl`), its prompt and `satl-term`, and its installers. Also the tag
  `satellite-003-revision-07` and the branch `archive/satellite-003-revision-07`.
- **[`old_versions/first_satellite/`](old_versions/first_satellite/)** — the first
  satellite (002).

## License

MIT — see [LICENSE](LICENSE).
