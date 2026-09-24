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

## T1 — threads that share nothing: **BUILT 2026-09-23**, `796f175`

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

**What T1 refuses, with S727 THREAD_CANNOT_SHARE_YET:**

- handing a thread an object of a spacesuit, a file or a window, even inside a list;
- running a spacesuit's capsule on a thread, because it runs on an object;
- a window word used on a thread. The window belongs to the main thread.

All of these would **share** something between two threads. Numbers, strings, lists and
the other values are copies. A list is copied the moment either side changes it.

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

## T2 — sharing, and the lock: **THE AUTHOR'S DESIGN, NOT BUILT**

This is how the design reached its present form on 2026-09-23. Each step was raced in
`time_test/locks/`.

1. The author asked for locks: *"Let's build locks into satellite.library instead of having
   a queue"*.
2. Then a bool on every object: *"if the bool is true, you cannot write to it"*.
   **Measured:** a bool checked and then set loses writes, 2,286,389 of 4,000,000. The same
   bool flipped in one step is right.
3. Then *"double literally everything"*: copy, change, swap in. **Measured:** right, and
   nobody waits, but every write copies.
4. Then *"try it with two bools"*. **Measured:** still wrong, 2,558,785 of 4,000,000.
5. Where it settled:

> *"we could just leave the entire library unlocked, so the programmer has to build their
> own, and we can offer a locking mechanism, let's offer an object.lock() that locks an
> object, and otherwise it's off, just use mutexes on all of the objects, but by default
> it's not on, so the programmer has to lock everything themselves! that beats the mutex!"*

**So T2 is:**

- Objects, lists, files and `satellite.library` values **may be shared** between threads.
  T1's S727 goes, for objects and files.
- **Nothing is locked unless the program asks.** `x.lock()` takes a mutex that belongs to
  `x`, and `x.unlock()` gives it back. It costs nothing on any object the program never
  locks.
- **`satellite.library` becomes writable at run time.** Today every write to it is refused
  with S250, because nothing is shared.

**OFFERED TO THE AUTHOR, NOT ANSWERED:**

1. **A forgotten lock can crash satl, not only lose a write.** Two threads appending to one
   list can leave the list's memory in two places at once. C++ calls that the
   programmer's problem.
   - **Proposed:** an optional check mode, off by default and so free, that names the line
     where two threads touched one object unlocked.
2. **A forgotten `.unlock()` would freeze every other thread.**
   - **Proposed:** a lock also lets go by itself when the capsule that took it ends, so
     `.unlock()` is optional.

## T3 — windows on threads: **NOT BUILT**

A window is the main thread's today. The desk's own thread and the walker meet in
`window_desk.hpp`, and one interpreter thread writes a piece. Letting a program's thread
build or change a window is a design of its own.
