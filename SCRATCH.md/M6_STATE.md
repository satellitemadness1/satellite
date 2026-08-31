# M6 — where it got to, 2026-08-30

**CONDITION MET 2026-08-31 — deletable now.** All three of the items below are
done, and each one is recorded where it belongs rather than here:
[MILESTONES/M6.md](../MILESTONES/M6.md) §9 is the account of doing them,
`make_support/040-sources.mk` carries the startup table, and this note's hash is
in M6.md's first line. **Doing them found four things** — a 34× startup
regression, a five-milestone-stale installer banner, a header that said
`EXIT_FINE` where the process exits 130, and a comment heading inverted against
its own code — all in M6.md §9 and in the files that said them. Kept only until
somebody has read it once, the standing [WORD_SURFACE.md](WORD_SURFACE.md) has.

**A dated working record, not a review.** [MILESTONES/M6.md](../MILESTONES/M6.md)
is the review and is complete. This file exists because the session that built M6
was stopped before the last three items below were done, and a milestone that is
finished in the code and unfinished in the ledger is exactly the failure
`MILESTONES/README.md` was written to catch.

## State: the work is done, built and green

- `make` and `make test` pass under **clang** (`~/opt/clang-current`) and **g++**,
  six suites, and `make STATIC=full` links and runs.
- Every clause of PLAN §8's M6 done-when is met; MILESTONES/M6.md §2 is the
  clause-by-clause table, §3 the three blockers it had to take, §4 what it found,
  §5 the finding worth reading, §6 what is left open, §7 the thirteen mutations.

## What was NOT done, and where each one landed — all three closed 2026-08-31

1. **`MILESTONES/M6.md` opens `(`PENDING`)` where a commit hash goes.** Nothing is
   committed yet — the whole of M6 is an uncommitted working tree. When it lands,
   the follow-up commit replaces `PENDING` with the hash, which is the shape
   `MILESTONES/README.md` now names.

   **DONE.** M6 landed 2026-08-31 and a follow-up commit wrote the hash into
   M6.md's first line — the shape `MILESTONES/README.md` describes, used for the
   first time.
2. **Startup has not been re-measured against PLAN §4.3's floor.** §9 says it is
   re-measured every milestone, and M6 is the first one that adds work to *every*
   run: a thread pool (~20 µs on the main thread, §4.5.1.2), a watchdog thread,
   and a `stat` for `satellite_config.ini`. `make_support/040-sources.mk` carries
   the M3 table to compare against — static, best of five runs of 200:
   `satl --version` 0.624 ms, `satl --words` 0.766 ms, bare static `main` 0.606 ms.
   **Both sides must be static**, which is the mistake that table records.

   **DONE, and it is the finding of the whole exercise.** `satl --version` is
   **1.172 ms** against a bare static 0.552 ms, so satl's own share went from
   0.018 ms to 0.620 ms — 34×. Two thirds of that is `facts::physical_cores()`,
   which reads 48 sysfs files on every run so that one row of `satl --limits` can
   print `12`; most of the rest is the two threads. The pool is **not** the ~20 µs
   PLAN §4.5.1.2 claims, and that is not a fault in the pool — ~20 µs is
   main-thread time and §4.3's floor is wall clock per invocation. Table in
   `040-sources.mk`, account in M6.md §9.1, corrections in PLAN §4.3 and §4.5.1.2,
   open item in M6.md §6.8.
3. **`satl --limits` and `satl --watchdog` have not been run from the INSTALLED
   binary.** PLAN §9's last rule — *"running the installed binary is what proves
   an install"* — and M6's done-when says §9 means the installed one. The install
   is `satellite_enterprise/install.sh`; `satl` on this machine is
   `~/.local/bin/satl -> ~/.satl/satl`.

   **DONE.** Installed, then run from outside the tree with `SATL_NO_WINDOW`
   unset — eight runs, tabulated in M6.md §9.2. The one that mattered is a config
   **beside the installed binary**: `found_config_path()` reads
   `dirname(/proc/self/exe)`, so that path exists only in an install and had never
   been executed. Both watchdog checks fire and exit 4. It also found the
   installer's closing banner still saying "this build is milestone 2".

## Files M6 added

    src/machine_limits/    limits.hpp limits.cpp config_internal.hpp config.cpp
                           pool.hpp pool.cpp watchdog.hpp watchdog.cpp
                           dump.hpp dump.cpp
    src/system_facts/      facts.hpp memory_facts.cpp host_facts.cpp stack_facts.cpp
    src/programs/          limits_command.hpp limits_command.cpp
    tests/limits_test/     limits_test.{hpp,cpp} reading.cpp examples.cpp
                           facts.cpp pool.cpp
    example/               satellite_config.ini broken_config.ini
    MILESTONES/            M6.md

## Files M6 changed

    src/error_reporter/errors.def      the S08xx block, ten rows, and the
                                       paragraph answering its own S04xx question
    src/error_reporter/report.hpp      as_text for (unsigned) long long
    src/programs/opening.hpp           EXIT_LIMIT = 4; EXIT_MALFORMED widened
    src/programs/opening.cpp           --limits, --watchdog, the config paragraph,
                                       exit status 4 in the closing lines
    src/programs/main.cpp              the two arms, start_limits(), the --words
                                       pool note, --limits/--watchdog in
                                       only_prints_and_exits
    make_support/010-compiler.mk       -pthread in CXXFLAGS
    make_support/030-directories.mk    LIMITS, limits_test in TESTNAMES
    make_support/040-sources.mk        twelve sources, seven headers
    make_support/065-tests.mk          the limits_test rule, run list, alias
    tests/reporter_test/codes.cpp      46 rows; the reserved-block check now names
                                       S05-S07 rather than counting up to 4
    PLAN.md                            §1, §4.5 heading, §4.5.1.2, §4.5.3, §4.5.4,
                                       §8's M6 paragraph
    LAYOUT.md                          src/, example/, tests/ rows; eleven fragments
    FORMAT/CXX.md                      eleven fragments
    MILESTONES/README.md               the M6 row, and the PENDING convention

And on 2026-08-31, closing the three items above:

    make_support/040-sources.mk        the M6 startup table and its attribution
    src/machine_limits/limits.cpp      the inverted F_OK heading
    src/machine_limits/watchdog.hpp    it does not return EXIT_FINE; it does not
                                       return
    satellite_enterprise/install_support/080-report.sh
                                       the closing banner, four milestones stale
    PLAN.md                            §4.3 and §4.5.1.2, both corrected by the
                                       measurement
    MILESTONES/M6.md                   §5.1, §6.5, §6.8, §6.9 and the whole of §9
    MILESTONES/README.md               the hash, and the PENDING paragraph in the
                                       past tense

## Two things a reader should not miss

**PLAN's M6 done-when is corrected in two places and both corrections are in the
plan itself**, not only in the review: the watchdog exits **4** and not 2 (2 is
`EXIT_USAGE` in this tree, and a script testing for it would report a memory
ceiling as a typo), and `satl --words` walks single-threaded for a reason that is
**not** the ~170 floor — 254 nodes is *over* 170, and the floor is about interning
a source, which that command does not do.

**Eight of thirteen mutations went through the suite as first written.** Four are
closed (MILESTONES/M6.md §7.2) and three stand with reasons (§7.3). Building one
of them found a real defect: `std::thread`'s constructor throws when
`pthread_create` fails, on a detached thread with no handler above it, so satl
would have died silently at startup on a machine with a low `ulimit -u`.
