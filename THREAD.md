# THREAD.md — making satellite's threads bulletproof

Started 2026-09-13. **The author's rule for this file: no known error in the
threading logic, or anything to do with threading, may be left standing.**
Every defect below has a milestone that removes it and a test that proves it
gone. Anything found later is added to §3 before the milestone it belongs to
can land. A defect is never left out because it is rare or awkward to reach.

Why now: `~/code/satl/dark_mechanicum` needs 150 "minor satellite" threads
feeding shared link objects, and satl corrupts its heap when threads share an
object. The dark_mechanicum session found it; this session reproduced it and
went looking for everything else.

---

## 1. What "bulletproof" means here

A threaded satellite program must never:

1. **crash** — no SIGSEGV, no SIGABRT, no heap corruption, whatever the timing;
2. **hang** — every `join()` and every program end returns;
3. **answer wrongly in silence** — no lost update, no spurious "holding
   nothing" refusal, no error blamed on the wrong capsule;
4. **drop an error** — a thread's refusal reaches a person on every entry point.

And the interpreter must be **clean under ThreadSanitizer** running every
program in `tests/thread_test/programs/`.

---

## 2. How it was found

The programs are in `tests/thread_test/programs/`, run against `satl 003
revision 01` at `4d0ede5`.

| program | what it does | result |
|---|---|---|
| `dark_mechanicum_link.satl` | 8 threads × 20,000 `call_feed` on ONE spacesuit (number field `+ 1`, list field `.append`) | SIGABRT 10 of 10: `corrupted size vs. prev_size`, `malloc(): corrupted top size`; twice instead S0713/S0714 "this one is nothing" at `pieces.append` |
| `shared_suit_fields.satl` | 4 threads × 20,000 `t.add()` on one spacesuit (number and string fields) | 3 of 3 died: `unaligned tcache chunk`, `double free`, SIGSEGV |
| `global_counter.satl` | 4 threads × 20,000 `satellite.library.n = satellite.library.n + 1` | exit 0, **35,690 / 36,101 / 36,333 of 80,000** |
| `nested_thread.satl` | a thread starts and joins its own thread | **hangs**, 3 of 3; gdb: inner thread spinning in `stopped_or_interrupted()` |
| `two_joiners.satl` | two threads `join()` the same thread | **hangs**, 3 of 3; gdb: both in `pthread_join`, 0 % CPU |
| `two_starters.satl` | two threads `start()` the same thread | S1402, correctly — but reported `in satellite.main`, not `in starter` |

Not a thread bug: dark_mechanicum saw five runs "exit 0" with stdout sent to
`/dev/null`. `/dev/null` sets off satl's window handover
(`programs/window_handover.cpp`), so those programs never ran in that terminal.

---

## 3. Every known defect

**Evidence:** *run* = reproduced by a program above. *read* = found by reading
the code, not yet reproduced; its milestone must reproduce it first (or show
it can't happen), then fix it.

### Crashes and memory corruption

| id | defect | where | evidence | fix in |
|---|---|---|---|---|
| **D1** | Spacesuit fields are a plain `std::vector<Value>` with no lock. Two threads writing a field write one 40-byte variant at once; a string or list handle is freed twice. | `satellite_spacesuit/suit_object.hpp:94`, `op_field_store`, `operations_dispatch.cpp:199,250` | run | T1 ✓ — per-object hold; TSan clean; 1,000 runs |
| **D2** | A mutating method on a field TAKES THE VALUE OUT (`fields[i] = nothing`) so `use_count()==1` allows in-place mutation (`19526c9`), then writes it back. Two threads can both take it out; one frees what the other mutates. A third reads the empty slot and gets S0713/S0714. | `operations_dispatch.cpp:187-201, 228-254` | run | T1 ✓ — hold kept from take-out to write-back |
| **D3** | The same take-out and write-back on a **global** is two separate lock grabs, so another thread sees `nothing` in between, and appends are lost. | `operations_dispatch.cpp:194-195, 252`; `evaluator/globals.hpp` | read: **not reachable from source** — every spelling of a method on a global is refused before running (S0521 `satellite.library.x.append`, S0204). Guarded anyway | T1 ✓ — `Globals::hold()` |
| **D4** | Two threads calling `start()` on one handle both pass the `started` check, then both assign `handle->worker`. Assigning to a joinable `std::thread` is `std::terminate`. | `satellite_thread/handlers.cpp:104-111`, `thread_handle.cpp:190` | read (S1402 won the race 3 of 3; not forced to terminate) | T1 ✓ — `started.exchange` |
| **D5** | `close_all()` joins a handle it swapped out of the registry while a child thread may be inside `wait()` on the same handle. Two joins on one `std::thread` are undefined behaviour, and `worker` is read and written from two threads with no lock. `pthread_kill` on a `pthread_t` another thread already joined is also undefined behaviour; the comment's "worst case is ESRCH" holds only before a join. | `thread_handle.cpp:121-132, 226-243, 245-277` | run: `closing_threads.satl` hangs 3 of 3 at 9cfd479 (with D8 in front of it) | T1 ✓ — one join via `claimed`/`reaped`; wake only before `ended` |
| **D6** | A thread started AFTER `close_all()` has swapped the registry is never closed. The run returns, `Compiled` and `Ast` are destroyed, and that thread keeps walking freed memory. | `thread_handle.cpp:251-255` | read: only reachable through a thread starting a thread, which D8 hung first. `closing_threads.satl` covers it | T1 ✓ — parent chain stops late starters; `close_all` loops until empty |
| **D7** | A **file handle** shared by threads: `read_at`, `buffer`, `buffer_at` and the gzip stream are one position across unsynchronised fields (the header says so: "Two threads reading one handle at M23 need a lock"). | `satellite_file/file_handle.hpp:22-31, 101-110` | run: `shared_file.satl` at 9cfd479 — SIGABRT (`std::out_of_range`) 1 of 3, and 42,440 / 40,087 lines of 40,000 with 80 / 72 torn | T1 ✓ — per-handle lock (Q4) + S1406 in satellite.log |

### Hangs

| id | defect | where | evidence | fix in |
|---|---|---|---|---|
| **D8** | A thread's own thread inherits `stopped_or_interrupted` as its "parent's hook", which is itself. At every statement boundary it calls itself forever (the tail call became a loop). **Any thread that starts a thread hangs.** | `thread_handle.cpp:39-44, 161-165`; `handlers.cpp:90` copies `m.policy()` | run + gdb | T1 ✓ — root hook fixed at `new`; 1,000 runs |
| **D9** | Two threads joining one thread: both reach `std::thread::join()`; the second waits forever. `joined` is a plain `bool` written after the join. | `thread_handle.cpp:226-230`, `handlers.cpp:139-145` | run + gdb | T1 ✓ — second join waits for `reaped`, gets S1404 warning + same answer (Q2) |

### Wrong answers and lost errors

| id | defect | where | evidence | fix in |
|---|---|---|---|---|
| **D10** | `satellite.library.n = satellite.library.n + 1` from several threads loses updates: the read and the write are separate lock grabs. This is DESIGN §7.1's "1585 wrong out of 1600", the thing globals were meant to prevent. | `globals.hpp` `read`/`write` | run: 36,333 of 80,000 | T2 ✓ — a statement keeps `satellite.library` on its thread's access list; 80,000 of 80,000 |
| **D11** | The same lost update on a spacesuit field (`count = count + 1`). T1's lock stops the crash but not this. | as D1 | run (hidden behind D1's crash) | T2 ✓ — a method call keeps its object on the access list; `shared_suit_fields` 80,000 of 80,000 |
| **D12** | A method's effects are not atomic on its object. `call_feed` doing `received + 1` then `pieces.append` can be seen half-done by another thread, which then reports `received != pieces.size()`. | spacesuit method dispatch | run at T1: `dark_mechanicum_link` received 57,217 / pieces 59,579 of 160,000 | T2 ✓ — 160,000 and 160,000 |
| **D13** | A thread's error re-raised at `join()` names the WRONG capsule: `Machine::refuse()` overwrites `problem.frames` with the joiner's call stack. | `machine.cpp:281`, `handlers.cpp:161` | run (`in satellite.main` for an error in `starter`) | T1 ✓ — now `in starter`, 1,000 runs |
| **D14** | `satl --evaluate` discards errors from threads nobody joined (`(void)thread::close_all()`), although it prints its own errors to stderr. A program that failed in a thread reports nothing. | `programs/evaluate_commands.cpp:141` | run: `call_thread_error.satl` under `--call` at 9cfd479 exits 0 and prints nothing | T1 ✓ — S0601 on stderr, exit 1 |
| **D15** | The wake signal SIGUSR2 (no `SA_RESTART`) interrupts any blocking call in a thread being closed. satellite's own `read`/`write` retry on EINTR, but vendored zlib's `gz_load` treats `read() == -1` as an error. A thread reading a `.gz` file when the program ends gets a spurious refusal, which `close_all()` then reports. | `thread_handle.cpp:80-105`, `vendor/zlib-develop/gzread.c:30-35`, `satellite_file/gzip.cpp:42` | read: **not reproduced** — `gzip_at_close.satl` 0 of 100 bad at 9cfd479 and after; read(2) of a regular file on a local filesystem is not interrupted by a signal on Linux. Reachable on NFS/FUSE | T1 ✓ — SIGUSR2 masked around `gzfread` |

### Found while building T1

| id | defect | where | evidence | fix in |
|---|---|---|---|---|
| **D18** | A thread joining a child that the closing run stopped re-raised the child's S0730 ("Ctrl-C arrived") as a refusal; `close_all()` reported it and the program exited 1 though nobody interrupted it. | `satellite_thread/handlers.cpp` `thread_join` | run: `closing_threads.satl` exit 1, 5 of 5, before the fix | T1 ✓ — the joiner stops Interrupted too |
| **D19** | Destroying a value nested deep recursed in C++ once per level. A list nested 1,000,000 deep: fine on the main walk, **SIGSEGV on a worker thread** (gdb: `_Sp_counted_ptr_inplace<List>::_M_dispose`), at 9cfd479 too. Same shape for maps, spacesuit chains, deferred calls, thread answers and the parent chain. | `satellite_value/value.hpp` | run: `satl deep.satl 1000000` exit 139 | T1 ✓ — `Burial`: children queued on the heap; 10,000,000 deep passes on both |

| **D20** | `close_all()` woke threads one at a time in launch order, waiting for each to end. A parent joining its sleeping child came first, could not end until the child did, and the child was never signalled: the program hung at exit for the length of the sleep (for ever on console input). Found by T1's fresh-reader review. | `thread_handle.cpp` `close_all`, `keep_waking` | run: `join_sleeping_child.satl` exit 124 (timeout) before the fix | T1 ✓ — every unended thread is woken together, then reaped |
| **D21** | A join that waits for itself: a thread joining its own handle, or A joining B while B joins A, waited for ever — also at program end, where one Ctrl-C could not reach it. `std::thread::join()` used to throw for the self case; the T1 wait did not. Found by the same review. | `thread_handle.cpp` `wait`, `handlers.cpp` `thread_join` | run: `join_itself.satl` exit 124 before the fix; `join_each_other.satl` | T1 ✓ — a join whose chain of waits leads back to the asker is refused, **S1407** |

| **D22** | *(T2, found by its review)* A method call or statement already under way when the program's first `start()` ran was never put on the access list: `c.run()` starting a worker that calls `c.bump()` raced run()'s own increments. | `machine.cpp` `enter`, `global()` | run: `started_inside_method.satl` 29,839 / 29,725 / 29,451 of 40,000 | T2 ✓ — a program containing `start()` is shared from its first line (`Compiled::starts_threads`); 40,000 of 40,000 |
| **D23** | *(T2, found by its review)* A mutating method on a global took T1's globals mutex and then waited for `satellite.library`'s access entry; the entry's owner then blocked on that mutex — a hang no wait check can see, because a mutex is not an edge. | `operations_dispatch.cpp` | read: needs a method on a global, which no source can spell today (see D3) | T2 ✓ — the entry is taken before the mutex |

### What is missing to prove any of this

| id | defect | fix in |
|---|---|---|
| **D16** | No sanitizer anywhere in the build: no TSan and no ASan target. The static build (`STATIC ?= full`) cannot take `-fsanitize=thread`, so a dynamic variant is needed. TSan works with `clang-current` here (checked 2026-09-13: it flagged a two-thread `x++` at once). | T3 |
| **D17** | No suite runs threaded satellite source. `tests/eval_test` covers the thread words one at a time; nothing runs two threads against one object, one global, one file or one handle. | T3 |

---

## 4. Decisions only the author can make

Each has a recommendation. **T2 cannot start until Q1 is answered.**

**Q1 — what a program can rely on when threads share something (D10–D12).**
**ANSWERED 2026-09-13 — (a), built as a per-thread ACCESS LIST.** The author's
design: every thread keeps a list of the shared things it is accessing; when
something is on another thread's list, a thread that wants it waits its turn,
"like a lock, but it's an access list kept per thread", at a small per-thread
CPU cost. An entry lasts **one method call** on a spacesuit (on when the method
starts, off when it returns) and **one statement** for globals. A thread
already holding an entry re-enters freely; two objects calling each other from
two threads take turns; a wait that would close a cycle through the lists is
refused with a new S14xx, never slept on. (Asked and decided against: holding
entries for a thread's whole life, which would run threads that share anything
one after another.) The options as they were written:

- **(a) A spacesuit's method call is atomic on its object** (monitor
  semantics): while `link.call_feed(e)` runs, no other thread is inside a
  method of `link`. Globals get the same guarantee per statement. *Recommended*:
  dark_mechanicum's `received + 1` and `pieces.append` stay consistent with no
  new words. **Its cost is deadlock:** thread A in `x.m()` calls `y.n()` while
  thread B in `y.n()` calls `x.m()`. Bulletproof means that cannot hang, so
  the interpreter must detect the cycle when a thread is about to wait and
  refuse with a new S14xx naming both objects. T2 builds that detection.
- **(b) Explicit words**, e.g. `satellite.thread.together(link) { ... }`. The
  interpreter guarantees only no-crash (T1). A program that forgets the word
  gets lost updates, which is a program bug rather than an interpreter bug.
- **(c) No sharing**: `thread.new` refuses an argument that reaches a
  spacesuit, file or thread, and threads talk through a new channel type. That
  is the safest, but it rules out dark_mechanicum's design.

**Q2 — a second `join()` from another thread (D9).** **ANSWERED 2026-09-13:**
the thread is done, so the second join is done too. It gives back the same
answer (or raises the same error) the first join did, the run keeps going, and
S1404 becomes a *warning*: printed when the run ends and appended to
`~/.satl/satellite.log`, where it stays until somebody clears it. This applies
to a second join in sequence as well.

**Q3 — `satellite.console.input` from several threads.** It is safe today (the
reader's queue is locked), and each line goes to whichever thread asked first.
v1's plan refused input from a worker. *Recommended:* allow it, documented, with
T3 testing that no line is lost or duplicated.

**Q4 — one file handle used by several threads (D7).** **ANSWERED 2026-09-13:**
use locks — each method call on a handle is whole — and write warning S1406 to
`~/.satl/satellite.log` (once per handle, not printed) so the sharing is on
record, and let the program run.

---

## 5. The milestones

### T1 — nothing crashes, nothing hangs

Fixes D1–D9 and D13–D15. No language change; every program that runs today
runs the same.

- **Spacesuit objects get a lock** (D1, D2). Field reads, writes and the
  take-out/write-back all happen under it. The in-place fast path stays, but
  only while the lock is held. **Rule from v1's A7:** a replaced value is
  destroyed AFTER the lock is released, because destroying a `Value` can run
  arbitrary destructors (a thread handle's, a file's).
- **Globals: take-out, method and write-back happen under one hold** (D3).
- **Thread handles** (D4, D5, D9): `started` via `exchange`; one mutex on the
  handle guarding `worker`, `joined`, `answer` and `problems`; exactly one
  `std::thread::join()` ever, with other waiters getting Q2's answer.
  `keep_waking` never signals a joined thread.
- **Nested threads** (D8): a child's `inherited` is the ROOT interrupt hook
  (Ctrl-C), never `stopped_or_interrupted`, and each child also sees its
  parent's `stop`, so closing a thread closes its children.
- **close_all is final** (D6): once closing begins, a new `start()` refuses
  (new S14xx or the existing Interrupted path — decide while building), and
  `close_all` loops until the registry is empty.
- **Files** (D7): Q4's lock.
- **Errors** (D13, D14): `refuse()` keeps a diagnostic's frames when it
  already has them; `--evaluate` prints abandoned thread errors to stderr.
- **Wake** (D15): reproduce with a large `.gz`, then either retry EINTR inside
  the gzip read path or have the wake skip threads inside a file read.
- **Measure, don't assume:** a worker's stack under `ulimit -s unlimited` is
  glibc's 2 MB. The walker keeps its own stack, but check that no C++
  recursion (deep value destruction, rendering) reaches that cliff first.

**Status (2026-09-13):** built, `make test` 14/14. New reproduction programs:
`shared_file.satl` (D7), `closing_threads.satl` (D5, D6, D18),
`call_thread_error.satl` (D14, run with `--call ... later`),
`join_sleeping_child.satl` (D20), `join_itself.satl` and `join_each_other.satl`
(D21, both end in S1407 by design),
`gzip_at_close.satl` (D15; needs `thread_gzip_test.txt.gz` in the current
directory). A dynamic `-fsanitize=thread -O1 -g` satl, built in a scratch copy of
the tree, reported **0 warnings** on `dark_mechanicum_link`, `shared_suit_fields`,
`nested_thread`, `two_joiners`, `two_starters` and `global_counter`, and 0 on
`join_sleeping_child`, `join_itself`, `join_each_other`, `closing_threads` and
`shared_file` after D20/D21. **1,000 runs each, all passed:** `nested_thread`,
`two_joiners`, `two_starters`, `closing_threads`, `shared_file`,
`global_counter`, `shared_suit_fields`, `join_sleeping_child`, `join_itself`,
`join_each_other` on the final binary, and `dark_mechanicum_link` on the binary
before D20/D21 (whose change is to join and close, which it does not reach).
`two_starters` passes by ending in S1402 named `in starter`; the two `join_`
cycle programs pass by ending in S1407. `make test` 14/14,
`help_lines/verify.py` 301/301. The counts in `global_counter`,
`shared_suit_fields` and `dark_mechanicum_link` are still wrong — T2.

**Done when:** every program in §2 runs 1,000 times with exit 0 (hangs are
killed by `timeout`, and a timeout is a failure), and the two spacesuit
programs also finish under the TSan build from T3's first step, which T1 may
pull forward. `global_counter.satl` may still print a wrong count; that is T2.

### T2 — nothing is silently wrong

Fixes D10–D12 by building Q1's answer.

- If Q1 is (a): a method call holds its object for the whole call, a statement
  touching globals holds them for the statement, and **a wait that would close
  a cycle is refused**, never slept on.
- `example/threads.satl`'s "touched" and "marks" checks become exact counts
  under 8 threads.

**Status (2026-09-13):** built as the author's access list —
`satellite_thread/access_list.hpp`. An `Access` on every spacesuit object and
one on `Globals`; `Machine::enter` puts a method's receiver on the thread's list
and `unwind` takes it off; the first global touch in a statement puts
`satellite.library` on until the statement's boundary (or the frame's return);
a run that ends any other way gives everything back. Joins and access waits are
one graph, so S1407 and the new **S1408** both refuse a wait that would never
end — `two_object_deadlock.satl` ends in S1408, every time.
`example/threads.satl` §4 now asserts exactly 16. **Verified:** 1,000 runs each
of `started_inside_method`, `global_counter` (80,000), `shared_suit_fields`
(80,000), `dark_mechanicum_link` (160,000 / 160,000), `two_object_deadlock`
(S1408), `join_sleeping_child`, `join_itself`, `join_each_other`,
`nested_thread`, `two_joiners`, `two_starters`, `closing_threads`,
`shared_file` — all passed; TSan 0 warnings on all but `shared_file` (unchanged
by T2, clean at T1); `make test` 14/14; `help_lines/verify.py` 301/301. A
two-reader review (concurrency, semantics) with a skeptic per finding found
D22 and D23 and the S1408 sentence, all fixed before this commit.

**What "one statement" means, written down.** The globals are held from a
statement's first touch until that statement ends **in the same capsule**. A read
in one capsule and a write in another are two statements: in
`satellite.library.n = next()`, where `next()` reads `n` in its own statement, an
update can still be lost. Holding through calls would make `run_everything()` in
`satellite.main` hold the globals for the whole program, so the unit stays the
statement; a program that needs a larger unit puts the read and write in one
statement or in one spacesuit method. Joining a thread while holding what that
thread needs is a real deadlock and is refused (S1407/S1408).

**Measured (load ~8, best of 3), T1 → T2:** single-threaded 160,000 method calls
0.18 s → 0.18 s; 8 threads each on its own object 0.04 s → 0.04 s; 8 threads on
one shared object 12.84 s (with lost updates, which forced copies) → 1.02 s exact,
about 6 µs per contended call.

**Done when:** `global_counter.satl` prints `80000 of 80000` and
`dark_mechanicum_link.satl` prints `received 160000` and `pieces 160000`,
1,000 runs each. A two-object deadlock program ends with the new S14xx instead
of hanging.

### T3 — proven, and written down

Fixes D16, D17, and closes the file.

- **`make tsan`**: a dynamic `-fsanitize=thread -O1 -g` build of `satl` (and
  `console_test`, per v1's §7.2), run over every program in
  `tests/thread_test/programs/`. **v1's gotcha, verified there:** two binaries
  in one `$(if ...)` run line make the second an argument to the first, so it
  never runs and the suite passes. One run line per binary.
- **`tests/thread_test`**: the fifteenth suite in `make test`. Each §2 program
  × 200 runs, plus a test for every D-number and every refusal, and Q3's input
  test.
- **DESIGN §10.5 rewritten** with what a program can rely on (Q1–Q4), and the
  header notes in `thread_handle.hpp` / `globals.hpp` corrected where they
  claim safety this file disproved.
- **A last adversarial read** of `satellite_thread/`, `globals.hpp`, the
  dispatch path and `satellite_spacesuit/` by a fresh reader. Anything it finds
  goes into §3 and is fixed before T3 lands.
- **dark_mechanicum's real plan runs**: 150 minor threads feeding links, its
  PLAN.md §9 item 1 re-enabled.

**Done when:** `make test` (15 suites) and `make tsan` are green, §3 has no row
without a fix commit beside it, and the dark_mechanicum session confirms its
program runs with shared links.

---

## 6. Checked and clean — don't re-investigate

- **Random generators**: `thread_local` sources (`satellite_random/tiers.cpp:27,99`).
- **The console**: whole units under one mutex; `end=` is one unit since `4d0ede5`.
- **Console lifetime (v1's A9)**: every entry point calls `close_all()` before
  the console shuts down (`run_command.cpp:212-214`, `session.cpp:312,392`).
- **`read`/`write` on files** retry EINTR (`file_reading.cpp:95`,
  `file_methods.cpp:123`). Only zlib's path does not (D15).
- **Arguments object, limits holder**: written once before any thread exists.
- **Lists and maps not held by a shared slot**: a thread's arguments are copied
  handles, so a local list's `use_count()` is at least 2 while the `Deferred`
  lives and the copy path runs.
- **v1's A1–A3, A5, B1–B5**: they don't apply to v2's design (one Machine per
  thread, an arena shared read-only, no per-evaluator output buffer).

## 7. Open, and not known to be threading

- The ~37 GB held by `satl --repl` after a program ended (2026-09-12). It isn't
  known to involve threads. If T1–T3 show it does, it becomes a D-number.
