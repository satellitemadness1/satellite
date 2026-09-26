*satellite design docs, §9, part 2 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§9 part 1](09-a-runtime-architecture.md), On: [§9 part 3](09-c-the-prompt-and-ctrl-c.md).*

---

## 9. Runtime architecture — the console and windows

*Continues [§9 part 1](09-a-runtime-architecture.md).*

### The console is a queue with a printer thread

**Verified defect, since fixed.** `satellite.console.display` was `output_ += text` into a
plain `std::string` member of the Evaluator, and `interp.cpp` copied the whole thing out and
printed it once, after the program had already returned. Two things followed, and the second
is the one that was never noticed:

1. **Nothing appeared until the program exited.** Verified: a program that printed a line,
   spun for several seconds, then printed another emitted both lines 2 ms apart, at the end.
   A long-running program was indistinguishable from a hung one.
2. **Every byte a program ever displayed was resident at once**, and then copied. Verified on
   300,000 lines: peak RSS 36.9 MB, of which ~20 MB was the buffer.

Both are the same fact — the buffer had no consumer. It has one now: a `Console` (console.hpp)
owning a `std::vector<std::string>`, a mutex, two condition variables and a printer thread.
A producer takes the lock only long enough to move one string in; the printer swaps the whole
vector out, writes each line to `std::cout`, and frees it. Same 300,000 lines: **peak RSS
4.4 MB, an 8.4× reduction, with byte-identical output.**

**One element per line, and that is the load-bearing choice.** A shared `std::string` under
`+=` from two threads does not merely race, it interleaves *mid-line* and shreds both. One
whole line per vector element cannot, whatever order the elements arrive in — so this is the
first piece of a concurrency story the rest of which is not written yet, and the one piece of
it that had to exist before any of the others. `console_test`
proves it with eight threads writing 250 lines each: all 2,000 arrive, none torn, and each
thread's own lines stay in order even though the threads interleave. It is clean under
ThreadSanitizer.

**The printer does not `erase(begin())`**, which is the obvious reading of "print it, then
delete it". Erasing the front shifts every remaining element on every line, so a program's
whole output costs O(lines²) — two million moves for 2,000 lines of forge narration. It swaps
the vector out under the lock instead and empties its private copy, freeing each string the
instant it has been written. Same structure, same "gone once printed", and producers get the
lock back immediately rather than queueing behind a terminal.

**What it costs, measured rather than assumed.** 300,000 lines, mean of 5, same program on
both binaries, output byte-identical: **0.4534 s → 0.6645 s, 47% slower.** The cause is not
where it was first looked for, and both wrong guesses are recorded because each was cheap to
check and each was wrong:

- *Not the condition-variable wake.* Signalling only on the empty-to-non-empty transition
  halves the cost of `write()` itself (20,000 queued writes, 2.63 ms → 1.78 ms) and moves the
  program's wall clock not at all.
- *Not the flush.* `strace -c` counts 25,468 `write` calls for 300,000 lines — the printer
  averages twelve lines a batch — totalling 21 ms of syscall.
- *Not `std::cout` being synchronised with stdio.* 300,000 insertions cost 10.8 ms synchronised
  and 8.0 ms not, against a 211 ms gap.

What is left is **one heap-allocated string per line, allocated on the walking thread and
freed on the printer's.** Isolated, the two mechanisms are 30 ms and 84 ms when the producer
races ahead, and 263 ms and 347 ms when it is slowed to the interpreter's own pace — so the
penalty grows the more the two threads run in lockstep, which is the allocator arena and the
cache line changing hands per line rather than per batch. The probe reproduces about 40% of
the interpreter's gap; the remainder is the walk's own allocation load contending with the
same arena, and that has not been separately measured.

**The 47% is not paid down, and it should be judged against what it buys**: output that
appears while a program runs, and memory that no longer scales with how much a program has
printed. The fix if it is ever wanted is producer-side batching — accumulate into a
thread-local buffer and hand over a chunk per N lines or per N milliseconds, trading a bounded
latency for one allocation per chunk instead of one per line. It is deliberately not done
here, because a latency bound is a language-visible promise and this section is not the place
to invent one.

**`satl --run` and the prompt both get a Console; a test does not.** `run_program` and
`run_source` each take one or null, and null keeps the accumulating behaviour — which is not a
fallback but a requirement for a test, which reads the text back rather than watching it
arrive. The run drains the Console before returning, which is why `drain()` is a separate call
and not folded into the destructor: §10 promises a runtime error is reported "after whatever
output preceded it", and that is only true if the output has actually left before the caller
prints the report.

The prompt's Console was added with the pace below, and it is owned by the **session** rather
than by a line. That is the whole of why it exists there: an Evaluator is rebuilt for every
line typed and for every `run <file>`, so a setting kept on one could not survive the line
that set it, while the printer thread outlives the entire prompt. A pace set at the prompt is
therefore still in force for the next line, and for a file that line runs. `eval_line()` then
returns the error report alone, the displayed text having already gone to stdout — which is
the same arrangement `satl --run` has always used, and satl-term inherits it unchanged because
satl-term spawns `satl` into a PTY and renders its bytes rather than interpreting anything.

### `satellite.console.display(100ms)` sets the printer's pace

**The surface.** A number followed by `ms` is a duration literal, and `100ms` and `100 ms` are
the same literal — the lexer needs no rule for either, because a word may not start with a
digit, so a number token ends at the `m` whether or not a space follows it and the parser sees
`Number` then `Word(ms)` in both cases. Nothing that parsed before parses differently: a number
followed by a word was a syntax error in every position the grammar has.

    satellite.console.display(100ms)      the printer pauses 100 ms between lines
    satellite.console.display(0ms)        full speed again
    satellite.console.display(value)      unchanged: display the value

**The pause is taken by the printer thread, and that is the point.** The program is not
waiting: the walk carries on building the next line while the previous one is still being
spaced out on the terminal, and the queue between the two absorbs the difference. A pause taken
by the code that called `display` would stop the program instead — which is the one thing the
console's whole design exists to avoid. The cost is stated rather than discovered: the queue is
the buffer the pause is drawn against, so a program that displays faster than the pace lets
lines leave holds the difference in memory.

**A duration is not a value.** §8.2's decision stands — there is no `satellite.variable.duration`
— and this does not reopen it. A duration *value* would first have to answer `.plus`, `.size`,
its own type name and its identity as a map key before it earned a slot in the `Value` variant;
a *literal* needs none of that. So `100ms` is an `Expr` alternative and never a `Value`, it is
legal in exactly one position — the argument of `satellite.console.display` — and everywhere
else it is an error that names the form that works:

    satellite.variable.number x = 100ms

    a duration is not a value: 100ms is a length of time, and
    satellite.console.display(100ms) — which sets the pause the printer takes between
    two displayed lines — is the only place the language asks for one

That position is matched on the argument **expression**, before the arguments are evaluated,
because there is no value for a duration to be reduced to. It costs one check at the top of
the call path and buys a rule with one meaning: `display` of a value displays it, `display` of
a length of time sets the pace.

**The pace travels with the line, not with the printer.** Sampling it in the printer was the
first implementation and it was wrong — verified: a program that paces five lines and then
sets `0ms` queues all six calls in microseconds, so the printer finds the zero already in place
and prints in 2 ms the five lines it was asked to space out. The pace in effect when a line was
*displayed* is stamped onto it in `write()` and obeyed when it is printed, which costs eight
bytes per queued line and makes the timing a property of program order rather than of a race.
For the same reason it is read per line rather than per batch: which lines shared a printer
wake-up is an efficiency of the console and must never be visible in the timing a program
asked for.

**`n` lines get `n-1` pauses.** The pause goes before a line and never before the first one the
printer writes: a pause after the last line would only delay the program's exit, and one before
the first would delay output that has nothing to be spaced from. A paced printer also flushes
per line instead of per batch, because a pause is only a pause if what came before it is
already on the screen — buffered, which is what happens the moment stdout is a pipe, every
sleep would happen invisibly and the whole output would still arrive in one burst.

**Shutdown honours the pace.** `drain()` and the destructor print what the queue still holds at
the pace it was written with, so the run outlives the walk by whatever is owed. The alternative
— dropping the pace at exit — would make a program that displays five lines and stops show all
five at once, which is the exact case the pace was set for.

**In the format**, a duration operand is kind 7 (format.def). Kind 1 would carry the integer
perfectly well and is exactly why the new kind exists: a decoder reading `P_DISPLAY` with a
kind-1 operand could no longer tell `display(100)`, which prints a number, from `display(100ms)`,
which prints nothing and sets a pace. A second path cannot separate them either — both spell
`satellite.console.display` and paths are keyed by their name code — so the operand is the only
place the difference can live.

### Windows

**The plan that stood here assumed one binary, and the split above deleted its foundation.**
It had the interpreter call `gtk_init_check()` on its own real main thread in `--repl`/`--run`
mode, run `g_main_loop_run()` there, put the tree walk on a worker thread and marshal window
operations back with `g_idle_add` — on the stated grounds that this cost "zero build changes:
the binary already links gtk4." That clause is what expired. `satl` links six shared objects
and not one of them is GTK, so the `gtk_init_check()` call site the plan is written around
does not exist, and creating it means paying back the 23.4 ms this section has just finished
removing.

**§16's native modules is the live plan for M7**, and it inverts this one: GTK arrives in a
shared object that the runtime `dlopen`s when a program executes
`satellite.include(satellite.window)`, so a program that never includes it never loads it.
That keeps the 23.4 ms instead of spending it, which is why M7 is now filed behind §16 rather
than in front of it. §16 has the mechanism. What survives here are two rules that outlive the
change, because neither was ever about where GTK is linked.

1. **Whatever opens a satellite program's windows must not construct a `GtkApplication`.**
   Registering the app id `org.satellite.terminal` is how a process says "I am the terminal",
   and whichever way the race falls the interpreter loses: it either becomes a remote whose
   `activate` makes some other process build *another* terminal and spawn *another* child — a
   recursive spawn loop, not a window — or it becomes the process that answers for the id. A
   `dlopen`ed shim wants bare `gtk_init_check()` + `gtk_window_new()` for precisely the reason
   the child was going to.

   `satl-term` itself passes `G_APPLICATION_NON_UNIQUE`, so it is not single-instance
   across invocations. Single-instance is wrong for it: the file to run is parsed into the
   *remote's* globals and `activate` runs in the *primary*, which reads its own empty copy,
   so `satl-term prog.satl` with a window already open opened a second REPL and dropped
   the file. One process per invocation also hands the interpreter the directory the command
   was typed in, which is what makes a relative path resolve.
2. **A main loop needs a main thread that is not blocked, and the REPL's is.** `satl` reads
   its prompt with `getchar()` (`main.cpp:171`), so nothing is pumping a GMainContext and a
   window created from that thread would never be drawn. Neither this section nor §16 has
   settled what M7 does about that, and it is the first thing the shim will run into.
