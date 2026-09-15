# satellite

**satellite 004 revision 04** — the third satellite, and the one being built from
here on. Its version, revision and build number live in
`satellite/config/satellite_config.hpp`; `make` raises the build number every time
it builds.

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
make                     # the interpreter and every numbered library
./check.sh               # the runner's checks
build/satellite-004 --version
```

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
