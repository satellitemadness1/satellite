# THREADS — a satellite program's own threads, in 004

The author's syntax, settled for 003 on 2026-08-27 and asked for again for 004 on
2026-09-23 (*"I meant let's build satellite.variable.thread my_thread =
satellite.thread.new(capsule_name(args))"*):

```
satellite.variable.thread my_thread = satellite.thread.new(capsule_name(args))
my_thread.start()
my_thread.join()        -- my_thread.wait() is its second name
my_thread.stop()
```

The author, the same day: *"we need my_thread.stop(), my_thread.start(), and
my_thread.join()"*, then *"but we can also have my_thread.wait() as another name for
.join()"*.

003 built threads as its M23 and hardened them in its THREAD.md, T1 and T2. That is
`old_versions/second_satellite/`. Everything below that comes from 003 says so.

## T1 — threads: **BUILT 2026-09-23**, `796f175`; sharing opened by T2

The code is in [thread_calls.hpp](satellite/bytecode/thread_calls.hpp) and `.cpp`, with the
value in [satellite_thread.hpp](satellite/satellite_variable_thread/satellite_thread.hpp)
(arm 18 of satelliteObject).

**What each method does:**

| Written | What it does | Where it comes from |
|---|---|---|
| `satellite.thread.new(f(x))` | Works out `x` now, on this thread, and does not enter `f`. | 003 M23 §2.2 |
| `.start()` | Runs `f` on a fresh OS thread and answers at once. It answers the thread, so methods can follow it. A second `.start()` is **S722**. | 003 |
| `.join()`, `.wait()` | Waits, then answers what `f` handed back, or nothing when it handed nothing back. Before `.start()` it is **S723**. | 003; `wait` is the author's |
| A second `.join()` | Answers the same value again, and **S724** is said once as a notice. But an answer that is or holds an object, a file or a window goes to the **first** join only, and a second is **S727**, because it would share it. | 003 Q2, the author; the review |
| `.stop()` | Asks the thread to stop at its next statement. It never ends a thread in the middle of a statement. `.join()` then answers nothing. | the author |
| `t == u` | True when both are the same thread. There is no `<` between threads. | 003 M23 §6 |
| `display(t)` | Shows the thread, for example `(thread count_alone, running)`. It can also say not started, stopped, failed or finished. | |

**When something goes wrong on a thread:**

- **A refusal on a thread** is reported where it happened, with its own line and caret.
  `.join()` then stops the joiner with the same code and prints nothing more.
- **A thread that fails and is never joined** still fails the run with its code.
- **At the end, "whenever the program reaches `satellite.return(satellite)`, close
  everything"** (the author, 2026-09-12). Every thread still running is asked to stop and
  is waited for. So a thread started on the last line may never run at all, as in 003.

**Chosen on 003's reasoning, and the author can reverse any of them:**

- **A fresh OS thread for every `start()`, not a warm one from the pool.** A pool of N
  threads deadlocks when N+1 program threads wait for each other (003 M23 §2.4). The 1024
  warm threads keep doing satl's own work.
- **Each thread gets a 128 MB stack**, which is satl's floor: about 40,000 capsules deep.
  The main thread gets the machine's whole share.
- **Output is locked one line at a time,** and so are reports. The lock is taken only once
  a program has started a thread, so a program without threads pays nothing
  (`machine/console_lock.hpp`).

**What is refused, with S727 THREAD_CANNOT_SHARE_YET:** a window given to a thread, and a
window word used on one. A window belongs to the main thread, which draws it (T3).
**Everything else is shared as it is written:**

- an object of a spacesuit handed to a thread is the **same** object on both;
- so is a file, and an object answered by `join()`;
- a spacesuit's capsule can run on a thread, as `satellite.thread.new(obj.call_x())`, or by
  its bare name inside the spacesuit's own capsules.

Numbers, strings and lists are values: each thread gets its own copy, and a list is copied
the moment either side changes it.

**Tested:**

- The author's own 003 program `thread_test.satl` runs unchanged apart from `main`'s
  parameter type, and prints `HELLO, WORLD!`.
- `tests/threads.satl` passes 003's proof: 8 threads × 200 activations, **1600 of 1600
  right**.
- Twelve more `tests/threads_*.satl` cover each refusal, both kinds of failure, the close at
  the end, and four threads printing 500 lines each with every line whole.
- There are 15 rows for this in check.sh, two of them added after the fresh review.

**The fresh reader's findings, 2026-09-23.** It built the change in its own copy and ran
probe programs. Everything it confirmed is fixed:

1. **`join()` handed an object to every joiner.** A thread's answer is copied to each
   joiner, so an object made on a thread and joined by two threads was shared. With two
   threads writing a string field, satl aborted in 3 of 3 runs. It now goes to the first
   join only (above).
2. **A loop with an empty body could never be stopped.** The stop check stood after the
   `}` test. It comes before it now.
3. **`input()`'s prompt was written unlocked.** Another thread's lines were lost. The
   prompt is written under the lock now, but the wait for a line is not.
4. **`--debug`'s state lines were written unlocked** from every thread's
   `satellite.return`. They are locked now.
5. **A spurious S724 appeared at the end of a run.** The close marked threads as joined,
   and a program thread inside `join()` then saw that. The close no longer marks them.
6. **A checker cache was filled at run time by `new`.** The value it filled was never
   read, so the call is gone.
7. **Threads started by a window press were closed only by the guard.** Their failures
   were lost. They are closed after the window pump now.
8. **A refused `pthread_create` could hang another joiner.** The thread is now marked
   ended with S725, never set back to "not started".
9. **The out-of-memory parachute could be freed twice.** It is an atomic pointer now.

**Known and not yet handled:**

- **Two threads writing `config.ini` at once** through an arguments row with `access` can
  mix the file: both write the same `config.ini.writing` before the rename. (The review.)
- **Only a thread waiting for itself is caught.** Two threads waiting for each other would
  hang, as in 003 before its S1407. In T1 this cannot happen yet, because a thread's
  handle is fixed when `new` runs, so no thread can hold its own. It becomes possible with
  T2's sharing.
- **A thread blocked in `satellite.console.input` stops only when its line comes.** 003's
  Q3, input from several threads, is still unanswered.
- **The `statements` and `word_counts` debugging features keep one table for the whole
  process**, so with threads their counts can mix.
- **`satellite.directory.change` moves the whole process,** every thread with it.

## T2 — sharing, and the author's lock: **BUILT 2026-09-23** `ebfce50`

T2 was first written up here as "the author's design, not built", after T1 had been
built to share nothing. The author, the same day:

> *"this was supposed to be done like completely differently than how you built it, we
> were locking objects, at the users discretion, so if the user decides to put a lock on
> something, that doesn't necessarily turn the lock on, it only turns the lock on when
> something goes to write to that object"*

> *"we were leaving the locks off, entirely, unless the user turns the lock that's on, on
> that object, then it only locks if that object is being written to"*

And on `.unlock()`: *"there really is no .unlock() needed though ... but how you have it
is good enough"*.

**What is built** (`satellite_object/object_lock.hpp`):

| Written | What it does |
|---|---|
| (nothing) | Every object and every file has a lock, and it is **off**. Nothing is ever locked for it, and threads share it freely. |
| `obj.lock()`, `f.lock()` | Turns the lock **on**. That locks nothing by itself. |
| a statement that **writes** a locked object | Holds the lock for that one statement, so `total = total + 1` from four threads loses nothing. Writing means: it assigns one of the object's fields, calls a method on one (`items.append(x)`), or calls a capsule. |
| a statement that only **reads** it | Waits only while a write is happening, and never for another read. Without that, reading a list while another thread appends to it can crash satl. |
| `if`, `while`, `for` | Hold the lock for their **condition** only, never for their body. |
| any method on a locked **file** | Holds it for that call. |
| `obj.unlock()`, `f.unlock()` | Turns it off again. |

A thread already holding an object's lock never waits for itself: a locked statement that
calls another capsule of the same object runs its statements without taking the lock again.

**Measured** in `tests/threads_lock.satl`, and run for comparison without `.lock()`. You can
run both yourself:

| Four threads, one object | Without `.lock()` | With `.lock()` |
|---|---|---|
| 5,000 adds each to one number field | **12,528**, 12,691, 12,179 | **20,000**, every run |
| 1,000 appends each to one list field | not run: it can crash satl | **4,000** |

Two threads appending 500 lines each to one locked file gave **1,000 lines** in three runs
of three.

**Still open:**

- **`satellite.library` is not yet writable at run time.** It is still refused with S250,
  so nothing there can be shared or locked yet.
- **Two threads each holding one object's lock while waiting for the other's** — a locked
  statement on A calling into B, while another on B calls into A — wait forever. 003
  caught that circle as S1408. 004 does not yet.
- **OFFERED, NOT ANSWERED:** an optional check mode, off by default and so free, that would
  name the line where two threads wrote one object with its lock off.

## T3 — windows on threads: **NOT BUILT**

A window is the main thread's today. The desk's own thread and the walker meet in
`window_desk.hpp`, and one interpreter thread writes a piece. Letting a program's thread
build or change a window is a design of its own.
