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

**And there are no limits.** No depth a program may not reach, no ceiling on
recursion, no constant in a header deciding how big a thing you are allowed to
write — the bound is memory, the way a list's length is. *(Decided 2026-08-31.
[DESIGN.md](DESIGN.md) §7.5 is the rule and [PLAN.md](PLAN.md) §2.5 is how it
gets built. satl raises its own stack to 8 GiB at startup, which is not the same
thing as having no limit and puts every depth a person could reach out of reach;
`SCRATCH.md/NO_LIMITS.md` is the honest half — what was measured, what was built,
and what is still owed.)*

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

`make test` runs the suites — one per module that has one, each a binary beside its
own sources. `make startup` re-measures what `satl` costs to start against an empty
program linked the same way, and diffs it against what it cost last time; PLAN §9
asks for that every milestone, and `make_support/startup.rows` is the table it
answers against. Neither is part of `make`, and `make_support/065-tests.mk` says
why: a test binary is not something an install ships, and adding one to the default
goal would change what "the build" means for a number compared across milestones.

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

**Seven files at the root**, each with one job, and none of them repeats another:

| | |
| --- | --- |
| [DESIGN.md](DESIGN.md) | **What the language is.** The generating rule, the syntax, the numbering, scope, the types, and what it deliberately refuses. Permanent. |
| [PLAN.md](PLAN.md) | **How it gets built.** Architecture, the build, the install, the milestones, and the measurement discipline. Permanent. |
| [WORD_NUMBERS.md](WORD_NUMBERS.md) | **The numbering.** Every number in the language, and the authority over all of them — DESIGN §4 explains the scheme, this holds the numbers. Permanent. |
| [SATC.md](SATC.md) | **The `.satc` file.** A program with its words already numbered, cached under `$HOME/.satl/cache`. Built at M4.5. Permanent. |
| [QUAD.md](QUAD.md) | **Why the language exists.** The program satellite has to be able to express, and what it is still missing to do it. Permanent. |
| [LAYOUT.md](LAYOUT.md) | **Every file in the tree**, and one line on what each is for. |
| [PLAN_ONE.md](PLAN_ONE.md) | The first draft plan. **Superseded** — read it for history or not at all. |

**And three directories that hold documents of their own**, which this index left
out until 2026-08-31 — so a reader who wanted to know what a landed milestone
actually did had no route to it from the front page:

| | |
| --- | --- |
| [MILESTONES/](MILESTONES/) | **What each milestone did, and the commit that did it.** One file per milestone that has been built, written when it landed. These are *reviews and not plans*: PLAN §8 says what a milestone will be, and a file here says what it turned out to be — what was decided on the way, what building it found, what it left open, and which mutations prove its test. [MILESTONES/README.md](MILESTONES/README.md) is the index and the dates. |
| [FORMAT/](FORMAT/) | **How the code is written.** [CXX.md](FORMAT/CXX.md) is the house style, the comment culture, how the build and a test are edited, and the X-macro registry mechanism. Permanent, and not a design document. |
| [SCRATCH.md/](SCRATCH.md/) | **A folder, not a file**, and the `.md` in its name is deliberate — it sorts next to the documents it is the opposite of. Everything in it is temporary and meant to be deleted; nothing in it decides anything. [Its README](SCRATCH.md/README.md) says what is in there now and what deletes each one. |

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
