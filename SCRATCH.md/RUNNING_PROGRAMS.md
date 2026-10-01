# RUNNING PROGRAMS — satellite running `clang++` and anything else, and a fast std::system on posix_spawn

## 2026-10-01: HIS DESIGN, AND WHAT IS BUILT

**Read this section first.** Everything below the line after it is the 2026-09-26 plan, kept as
it was written; its open questions are answered here where he answered them.

### Why (his words, 2026-10-01)

> *"we were working on implementing something similar to cstdlib's std::system to run a command on
> the machine and the reason we were doing that is to make the scripts in /~/Documents in
> satellite"*

> *"there's running a program on a machine, and running a bash commnand are two different things"*

What those scripts need beyond running a program -- 491 program starts across the eight of them,
310 of them chores a language does itself, and what satellite 0125 can and cannot do -- is in
`~/Documents/satl/running-programs-2026-10-01/` (inventory.json, the probes, the review).

### His design, in his words, in order

> *"satellite.variable.program my_program = {"/dir/program", "arg1", "arg2"} // or a list of str
> could optionally be put here, or just the name of the program in quotes like this:
> satellite.variable.program run_program = "/dir/some_program"*
> *my_program.start() my_program.ok() my_program.error() my_program.end() // for build.code
> my_program.pass("some_str") // or a list, or my_program.pass({"one", "two"})*
> *and the output will be displayed unless my_program.start("hide") is called*
> *and when all that is complete with posix_spawn, we do: satellite.variable.bash my_command =
> "mkdir /home/madness/code/satl" my_command.start()"*

> *"instead of just .end() have alias .code() .return() .exit()"*, then *"oh wait, I forgot .end()
> should shut the program down, with alias .shutdown()"*, then *"and .code() .return() (not
> .exit(), that sounds like .quit()) will be for the return"*

Asked whether start() waits: *"just make .join() .end() and .code() wait for the program to
finish, all 3 do the same thing, and return the error code, and have .start() return to satl
while the output from the program is displayed, so it runs in another window, forcing the user to
use both .start() and .join() together"*. Asked which window: *"satl's own console"*. Asked what
.shutdown() does: *"let's have .shutdown() kill the process, and same with .end() instead of .end()
waiting like join, .end() .exit() and .quit() and shutdown() all do the same thing"*. Asked what
pass() gives: *"typed input while the program is running"*. With no display: *"Use satl's own
output"*.

So, as built:

| | |
|---|---|
| `satellite.variable.program p = {"/dir/program", "arg1"}`, a list of strings, or `= "/dir/program"` | what it runs; one string is the program alone, never split |
| `p.start()` | starts it and answers at once; its output is shown in satl's console, between satl's lines |
| `p.start("hide")` | the same, its output and its errors thrown away (STEP 2, built 0129) |
| `p.ok()`, `p.error()` | whether it could start, and why not |
| `p.join()`, `p.code()`, `p.return()` | wait for it to end, and answer its exit code (STEP 1, built 0128) |
| `p.end()`, `p.exit()`, `p.quit()`, `p.shutdown()` | kill it -- it and everything under it, SIGTERM then SIGKILL five seconds later -- and answer its exit code (STEP 3, built 0131) |
| `p.pass("text")`, `p.pass({"one", "two"})` | typed into it while it runs, a line each; join() and end() close the input (STEP 4, built 0132) |
| `satellite.variable.bash c = "mkdir ..."` | a line bash reads -- its pipes, its `>` and its `*` work -- with every method above (STEP 5, built 0134) |

### The seven open questions of 2026-09-26, where they stand

1. **What "a fast std::system call" is** -- ANSWERED: a satellite type, satellite.variable.program,
   and satellite.variable.bash for a bash line ("two different things").
2. **The satellite spelling** -- ANSWERED, above.
3. **What comes back** -- ANSWERED for the code: join(), code() and return(). The output is shown, or
   thrown away with "hide"; KEEPING it as a string was not asked for and is not built.
4. **Whether satl waits** -- ANSWERED: start() answers at once, and join() waits.
5. **How the output meets satl's lines** -- through the printing satellite's ring, as satl's own
   words go, whole line by whole line (display/printing_satellite.hpp's display_bytes).
6. **Keyboard input** -- ANSWERED: pass(), "typed input while the program is running" (STEP 4).
7. **Ctrl-C** -- STILL OPEN, his. As built, a program is in satl's own process group, so Ctrl-C
   reaches both, as it does a bash script and the programs it runs.

### What starts the program: posix_spawn's own clone, with one more step

His word was posix_spawn. glibc's posix_spawn is clone(CLONE_VM | CLONE_VFORK) and a list of steps
before exec, and that list has no step to give a child back the stack satl was given -- the rule in
machine/stack_share.hpp. Raced 2026-10-01 from a process shaped like satl
(RUNNING_PROGRAMS/spawn_race.cpp; run it yourself, the command is at its top):

| | per run | the child's stack limit |
|---|---|---|
| posix_spawn | 565 us | 1,986,560 KiB -- satl's widened 1.94 GiB |
| fork + the hook (what prompt_run.cpp does) | 8,481 us | 8,192 KiB |
| clone + the hook (what is built, program_spawn.cpp) | 594 us | 8,192 KiB |

### My choices, his to overrule

- a program that cannot start is not a refusal: ok() is false, error() says why, and join() answers
  what a shell answers -- 127 for no such program, 126 for one that may not be run;
- a program ended by a signal answers 128 + the signal, as a shell does, and error() names it;
  error() also says "it ended with code N" when the code was not 0, and "" when there is nothing;
- every method needs its brackets, as a thread's do (p.ok is refused, p.ok() is right);
- ok(), error() and join() before any start() are S741; start() while it still runs is S740, and once
  it has ended start() runs it again;
- "forcing the user to use both": a program started and never joined is stopped when the run ends
  (SIGTERM, then SIGKILL five seconds later) and the run fails, S742, placed at that start() -- not
  for one that could not start, and not when the run is already failing for its own reason;
- its output and its errors are one stream, in the order it wrote them; whole lines go to the screen
  as they come, and a line with no end yet (a prompt) is shown once the program has been quiet 50 ms;
- what it is handed: input /dev/null (until pass(), STEP 4), satl's terminal as its controlling
  terminal (so sudo asks there), every other descriptor closed, signals back to their defaults.

And for a bash line (STEP 5), also mine:

- it runs as `bash -c -- "the line"`, bash found on the PATH as a program's name is. The `--` is
  glibc's own system()'s: without it, a line that begins with `-` is read as bash's options
  (`bash -c '-n'` exits 2; `bash -c -- '-n'` says "-n: command not found" and answers 127);
- bash runs as a script does: not a login shell and not interactive, so no .bashrc and no aliases
  -- what a `#!/bin/bash` script is given, and what std::system's sh is given;
- ok() is whether bash started. A command in the line that is not found is bash's to say, on the
  console, and the line answers bash's 127, as in a terminal;
- an empty line is refused, as an empty program name is;
- a bash line given to a program name, or a program to a bash name, is allowed and stays what it
  is -- both are programs, so a capsule taking `satellite.variable.program` takes both;
- displayed, it reads as its line: `(bash echo one two three | wc -w, ended with 0)`.

### What is not built, and is his

- Ctrl-C (question 7 above);
- what a LIST given to a bash name means -- refused for now, as either of two things it could be:
  the lines of a script, `{"cd /tmp", "make"}`, or a line and the words bash hands it as `$1`, `$2`
  -- `{"cp \"$1\" \"$2\"", "my file", "backup"}`, which is the safe way to pass a file name with
  spaces into a bash line;
- keeping a program's output as a string;
- a working directory or environment for one run;
- whether a program started at the satl prompt and never joined should be reported: it is stopped
  when the prompt ends (structured-library.cpp, since step 1's review), quietly -- no S742, since a
  session is not a run that a missing join() fails. (This line said "not stopped" until 2026-10-01,
  STEP 5; that was written before the review's fix.)

---


Written 2026-09-26 for **the session after `/clear`**. Read this whole file first. Its companion
is now `SCRATCH.md/FAST_PRINTING.md`, whose step 6 builds this file's work. (It was
`SCRATCH.md/DISPLAY_THREADS.md`, where the first display attempt stopped.)

**Nothing here is built.** The day produced answers, two small programs that demonstrate them,
and one decision about output that led to the display threads. Those were built and then pulled
out.

The programs are in `SCRATCH.md/RUNNING_PROGRAMS/`:
- `run_program_demo.cpp`: posix_spawnp, a raw pidfd and poll(), running clang++
- `system_vs_spawn.cpp`: std::system against posix_spawnp, timed

---

## His words, in order

> *"claude, this is the SATELLITE programming language, and we need to build into the language a
> way to run programs... from the output to everything, we need to build the whole thing.. so it
> will use cstdlib and std::system to start out, or is there a better way to run a program than
> cstdlib and std::system?? Just answer that before we continue..."*

> *"is there a better option that posix spawn even?"*

> *"so does this require glibc to be installed on the machine, or can I compile everything into
> the satellite interpreter??"*

> *"and i'm talking about running like, "clang++ my_file.cpp -o my_program", not another satl
> interpreter...."*

> *"hmmm, just show the output live, but satl may be doing other things while this is going on,
> so they will compete for the printer thread..."*

That led to DISPLAY_THREADS.md. After it was pulled out:

> *"I learned alot about.. std::system anyway, we can still use that knowledge to build a fast
> std::system call using posix_spawn"* ... *"during this context"*

**So the next step, in his words, is a fast std::system call built on posix_spawn.** Since then
FAST_PRINTING.md orders the work: its step 5 first, then this as its step 6. And his words later that
day: *"this is a pre-requisite for running a pre-version of quad ai that another ai cooked up, I dunno,
we do need to run std::system, or a command like it anyways"*.

---

## What was established

### std::system, and why it is slow and limited

**Measured**, `system_vs_spawn.cpp`, 2,000 runs of `/bin/true` (the absolute times ran while
check.sh was running, so read the ratio):

| | per run |
|---|---|
| `std::system("/bin/true")` | **1,788 µs** |
| `posix_spawnp("true")` + `waitpid` | **664 µs** |
| the same, with the calling process holding 2 GiB, like a big satl | 1,845 µs against 696 µs |

That is **2.7× faster without the shell**, and memory held makes no difference.

**Why, traced with `strace -f` on this machine.** glibc's `system()` already uses `posix_spawn`:
`clone3` with `CLONE_VM|CLONE_VFORK`, and no page tables copied. What it spawns is `/bin/sh -c -- "<command>"`.
`/bin/sh` here is **bash**, which then `execve`s `/bin/true` in its own place, with no second fork.
So the whole difference is starting bash for every command. **A fast std::system is one that
spawns the program itself and skips the shell.**

std::system's other limits:
- **The output can't be captured.** It goes wherever satl's goes.
- **It returns a wait status, not the exit code.** An exit of 1 comes back as 256.
  `speed_test/speed_test.cpp` in 003 already notes this.
- **Everything goes through the shell.** A file name holding a space, a quote, `;` or `$` is read
  again by the shell. That is a quoting bug and an injection door.
- **It blocks until the program ends.** There is no process id, so no stop, no timeout, and no
  running it while satl does other things.
- **It ignores SIGINT and SIGQUIT in the whole of satl while it waits.** That is process-wide, and
  satellite has threads.
- **`popen` is no better:** output one way only, and still the shell.

### Better than std::system, and whether anything beats posix_spawn

- **`posix_spawnp` + pipes + `poll()` + `waitpid`:**
  - the program's arguments go as a list, with no shell
  - the output and the errors come back on separate pipes
  - the real exit code comes back, or the signal that killed it
  - there is a pid to stop it, time it out, or run it in the background
- **`pidfd_spawnp`** (glibc 2.39) is the same call, handing back a **pidfd**: a file descriptor that
  stands for the process.
  - It goes in the same `poll()` as the output pipes, and becomes readable when the program ends.
  - `pidfd_send_signal` can never hit a stranger that was given a reused pid.
  - Its family adds `posix_spawnattr_setcgroup_np` (start the program inside a cgroup, to cap its
    memory or CPU), `posix_spawn_file_actions_addchdir_np` and `..._addclosefrom_np`.
  - This machine has it: `pidfd_spawnp@@GLIBC_2.39` is in `/lib64/libc.so.6`.
- **Nothing starts a program faster.** posix_spawn and pidfd_spawn are the same code in glibc.
  What a start costs is mostly `exec` and the program's own library loading.
- **Not better:**
  - `fork`+`exec`: slower from a big process, and risky with threads
  - `vfork` or `clone3` by hand: what glibc already does, with more ways to crash
  - io_uring: this machine's headers have `IORING_OP_WAITID` but nothing that starts a process
  - Boost.Process: it wraps the same calls, and would have to be vendored

### glibc: installed everywhere, but which version

- **glibc is on every machine satl runs on.** It is the C library the whole desktop runs on.
- **004 cannot compile it in:**
  - the word libraries are `.so` files that satl loads, and they share satl's C library
  - satl's console reaches the graphics driver through GTK and Wayland, and the system loads that
    driver built against the system's glibc
  - 003 could be fully static (`make STATIC=full`); 004 is dynamic on purpose (`make_support/048-link.mk`)
- **What matters is the version.** The installed satl already needs **GLIBC_2.38**
  (`__isoc23_strtol`, `__isoc23_sscanf`, from building on glibc 2.39 headers), measured with
  `objdump -T ~/.satl/satl`.
  - It **already does not start** on AlmaLinux/RHEL 9 (2.34), Ubuntu 22.04 (2.35) or Debian 12 (2.36).
  - It runs on AlmaLinux 10 (2.39, which includes his mother's machine), Ubuntu 24.04, Fedora 40+
    and Debian 13.
  - `pidfd_spawnp` would move the floor to 2.39.
- **The recommendation, as it last stood: `posix_spawnp` + `syscall(SYS_pidfd_open, pid, 0)`.**
  - posix_spawnp is in every glibc, and a raw `pidfd_open` needs only Linux 5.3 (2019), no glibc
    version at all.
  - For satl's own child it does exactly what pidfd_spawnp does: an unreaped child's pid cannot be
    reused.
  - It stays true as long as nothing else in satl reaps children: no `SIGCHLD` set to `SIG_IGN`, and
    no `waitpid(-1)`.
- **For older machines:** build on the oldest one to be supported (an AlmaLinux 9 container), not
  static. glibc works forwards. Not started; his call.
- **Windows** would need `CreateProcess` behind the same satellite words.

### The demonstration (`run_program_demo.cpp`)

posix_spawnp, a raw `pidfd_open`, and one `poll()` over the output pipe, the error pipe and the
pidfd. No shell.

| ran | came back |
|---|---|
| `clang++ broken.cpp -o broken` | clang's error, 139 bytes, as a string; **exit code 1** |
| `clang++ "my file.cpp" -o "my program"` | nothing printed, **exit code 0**; the spaces needed no quoting |
| `./my program` | `hello from my program`, **exit code 7** (its own `return 7`) |
| `clang+++ x.cpp` | `could not start clang+++: No such file or directory`, known before anything ran |

### What differs from typing the same command in a terminal

1. **Colours go.** clang++, gcc, ls and grep turn colour off when their output is a pipe. The fixes:
   - `-fcolor-diagnostics` for clang
   - a pseudo-terminal, which is general, but mixes output and errors unless errors get their own pipe
2. **Output can arrive late.** A program writing into a pipe usually saves up its output and sends
   it a few KB at a time. A pseudo-terminal fixes that too.
3. **The PATH is satl's.** Started from the Satellite button, satl has the desktop's PATH, not
   `.bashrc`'s. `/usr/bin/clang++` is found; `~/opt/gcc-17/bin` and `~/.local/bin` are not.
4. **No shell means no shell features.** `*.cpp`, `>`, `|` and `&&` would reach the program
   literally. The fix is a second form that runs a line through `/bin/sh -c`, with the same spawn,
   so the output and exit code still come back. That form costs bash's start every time, per the
   measurement above.

**What was recommended, never built:**
- pipes by default
- two forms: *a program and a list of arguments* (no shell) and *a shell line*
- a pseudo-terminal later, for interactive or colourful programs

### Where satl already starts programs

- **`satellite/satl/prompt_run.cpp:108-145`:** `fork` + `execv` + `waitpid`, the prompt running a
  satl file.
  - It flushes `std::cout` and `std::cerr` first and ignores SIGINT while it waits.
  - Its child writes `std::cerr` if `execv` fails.
- **`std::system` in race and test harnesses, not the language:**
  - `satellite/satl/directory_cases.cpp`
  - `time_test/satellite.library/main.cpp`
  - `experiments/racing_003/race.cpp`
  - `haswell_test/racing.cpp`

  These are the obvious first users of a fast std::system.

---

## Open questions — NOT decided, his to rule

1. **What "a fast std::system call" is:**
   - a C++ function inside satl, replacing std::system in satl and its harnesses
   - a satellite word
   - or both

   And whether it goes **through a shell at all**: the 2.7× is exactly the shell.
2. **The satellite spelling** of running a program, and of its two forms (program + arguments, and
   a shell line).
3. **What comes back to the program:**
   - the exit code (surely)
   - the output as a string: he said *"just show the output live"*, and did not say kept as well
   - the errors separately
4. **Whether the program waits for it, or satl carries on while it runs.** He said *"satl may be
   doing other things while this is going on"*.
5. **How the live output meets satl's own lines.** SUPERSEDED 2026-09-26: build 0113 has a printer
   thread (`satellite/display/`), and his drawing puts program output between the printing satellite
   and the display thread -- FAST_PRINTING.md step 6 is that design, and the console_lock option below
   is dropped. What follows is as it was written. His concern was that they *"will compete for
   the printer thread"*. **004 has no printer thread**: a line is written by whichever thread
   displays it, holding one lock once a thread exists (`satellite/machine/console_lock.hpp`). With
   the display threads pulled out, this is open again. One option was put to him before the
   three-thread design:
   - a watcher thread reads the program's pipes
   - it writes each whole line holding that same lock
   - it sets `a_thread_was_started()` first, so every line is locked

   That is the one-line-at-a-time rule 004 already has for threads.
6. **Keyboard input for the running program:**
   - none (`/dev/null`), which is the safe default
   - or satl passes typed lines on
7. **Ctrl-C:**
   - should it stop the running program too? It does by default, being in satl's process group.
   - `POSIX_SPAWN_SETPGROUP` would keep Ctrl-C from reaching it.
