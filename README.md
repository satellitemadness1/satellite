# satellite

A programming language where **the path is the interface**.

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display("Hello, World!")

    satellite.return(satellite)
}
```

One invariant produces all of it:

> **A dotted path rooted at `satellite` names something the language owns.
> A bare identifier names something the user owns.**

So there is no flag soup, no bitmask of options nobody remembers the order of, and
no punctuation ceremony. `satellite.file.open("filename", "read_append")` reads
without a reference open, because the path segments *are* the documentation. The
tie-breaker at every fork is the same: **do absolutely everything for the user**,
and pay for it in performance rather than in their attention — but never do anything
behind their back. A refusal in plain words beats a guess.

---

## Where this is

This is the **second** satellite, and it is early — early enough that the honest
answer is a milestone number rather than a feature list. [PLAN.md §1](PLAN.md) says
where things stand and [PLAN.md §8](PLAN.md) says what lands when. `satl --version`
tells you what you actually have in front of you.

The first satellite is complete, works, and is fast. It lives at
`old_versions/first_satellite/` as a reference, not a dependency: nothing here
includes from it, and `satl` compiles with the folder absent.

## Building

```sh
make
```

On x86-64 that produces a baseline `satl`, a `satl.haswell` built for Haswell and
newer, and `satl-cpu-level`, the small program that says which one this machine can
execute. Everywhere else it produces one `satl`. [PLAN.md §4](PLAN.md) explains why
there are two builds and why the detector is compiled at the baseline.

## Installing

```sh
satellite_enterprise/install.sh
```

Written and tested on AlmaLinux, aimed at the RHEL family, and it says so and
carries on if it finds itself elsewhere. It **needs no root, ever** — everything it
writes is under `$HOME/.satl`, one directory to remember and one to delete. It
edits no file you own, exports no variable into a shell that is about to exit, and
`--uninstall` removes what it wrote by name.

`--dry-run` prints exactly what it would do, as a transcript you can paste. Run that
first. `--link` and `--desktop` are opt-in, and even when asked for they refuse
every path they do not already own.

## The documents

Seven files, each with one job, and none of them repeats another:

| | |
| --- | --- |
| [DESIGN.md](DESIGN.md) | **What the language is.** The generating rule, the syntax, the numbering, scope, the types, and what it deliberately refuses. Permanent. |
| [PLAN.md](PLAN.md) | **How it gets built.** Architecture, the build, the install, the milestones, and the measurement discipline. Permanent. |
| [WORD_NUMBERS.md](WORD_NUMBERS.md) | **The numbering.** Every number in the language, and the authority over all of them — DESIGN §4 explains the scheme, this holds the numbers. Permanent. |
| [SATC.md](SATC.md) | **The `.satc` file.** A program with its words already numbered, cached beside its source. Permanent. |
| [QUAD.md](QUAD.md) | **Why the language exists.** The program satellite has to be able to express, and what it is still missing to do it. Permanent. |
| [LAYOUT.md](LAYOUT.md) | **Every file in the tree**, and one line on what each is for. |
| [PLAN_ONE.md](PLAN_ONE.md) | The first draft plan. **Superseded** — read it for history or not at all. |

Cite DESIGN.md and PLAN.md **by section number, never by line.** Line numbers stop
meaning anything the first time a file is edited; `§4.2` is the whole address.

This README is an index and holds no fact of its own — the same shape as the
`Makefile` over `make_support/` and `install.sh` over `install_support/`. Every
number lives beside the decision it justifies, in the file that makes it, because
**prose may explain a number; it may never be the only place the number lives.**

## Licence

MIT (Expat) for satellite's own source. See [LICENSE](LICENSE), whose
third-party section covers the one component that is not satellite's:
[pcg/](pcg/) is pcg-cpp 0.98, **Apache-2.0**, three unmodified headers.
