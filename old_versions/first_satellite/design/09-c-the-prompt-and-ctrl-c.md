*satellite design docs, §9, part 3 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§9 part 2](09-b-the-console-and-windows.md), On: [§10](10-evaluator.md).*

---

## 9. Runtime architecture — the prompt, and Ctrl-C

*Continues [§9 part 2](09-b-the-console-and-windows.md).*

### 9.4 What the prompt could not do

Two things, and they had one cause. `run_repl()` read a line with
`while ((c = getchar()) != EOF && c != '\n')`, and nothing anywhere in the tree
called `sigaction`. So:

- **The up arrow was three literal characters.** A terminal sends `ESC [ A`,
  and a cooked-mode read hands those bytes to the line like any others, so the
  line became `^[[A` and the previous entry was unreachable. There was no
  history to reach it in either.
- **Ctrl-C killed the session.** With no handler installed, SIGINT's default
  action terminated `satl` — at the prompt, mid-program, and half way through a
  multi-line capsule alike. A prompt that cannot be interrupted without being
  destroyed is a prompt where an infinite loop costs you the session.

### 9.5 The split: a byte at the prompt, a signal during a run

The fix for both is the same mechanism read two ways, and the division is the
whole design:

| where | terminal mode | what Ctrl-C is | what it does |
| --- | --- | --- | --- |
| typing at the prompt | **raw** (`ISIG` off) | the byte `0x03` | cancels the line; the session lives |
| a program is running | **cooked** (`ISIG` on) | a real `SIGINT` | stops the walk at the next statement |

Reading an arrow key requires raw mode, and raw mode is exactly what turns the
INTR character into an ordinary byte. That is not a cost to be worked around —
it is the answer to what Ctrl-C should mean at a prompt, which is *cancel this
line*, not *end this session*. And because raw mode is scoped to the read of
one line, a program the prompt then runs gets the cooked terminal back: its own
`satellite.console.input()` keeps the kernel's echo and backspace, and its
Ctrl-C is a real signal.

**That scoping is what limits the damage §9 part 1 warned about.** The argument
for keeping two processes is that "the PTY gives you the kernel tty line
discipline for free — echo, backspace, line editing, Ctrl-C", and that
collapsing to one process would mean reimplementing all of it. `console_input/`
*is* that reimplementation, and it is paid for deliberately and narrowly: it
runs while a prompt line is being typed and at no other moment.

### 9.6 `console_input/`, and why only one of its files touches a terminal

The module is console_output's other half and named for it. Five files, and the
split is a testing decision rather than a tidiness one:

| file | what it is |
| --- | --- |
| `keys.*` | bytes → keys. A pure state machine. |
| `editor.*` | keys → a line and a cursor. Pure. |
| `history.*` | the previous entries, and the file they live in. |
| `render.*` | drawing a line that may not fit on one row. |
| `line_reader.cpp` | the read loop — **the only tty in the module**. |

An arrow key is three bytes that arrive one `read()` at a time, and a Ctrl-arrow
is six. The only way to test that decoding is to be able to hand it bytes from a
test, so everything that can be a function of bytes is one. `console_input_test`
drives all four of the pure files with no terminal existing anywhere.

Three things the pure half settled that are easy to get subtly wrong:

- **An escape sequence occupies no columns.** The prompt is bold-on, a username,
  bold-off — eight bytes the terminal consumes and never draws. A width that
  counted them would put the cursor eight columns right of where the user is
  typing, on every redraw. `display_width` walks the CSI grammar rather than
  guessing a length.
- **Movement is by character, never by byte.** A left arrow over a two-byte
  character has to move two bytes or it lands inside one, and a backspace has to
  delete both.
- **The half-typed line is saved on the way out of it.** Type half a line, press
  Up to check something, press Down to come back — and without this the work is
  gone. It is saved once, on the *first* Up, or the second press would overwrite
  it with the entry the first recalled. This is the one history bug every reader
  notices and nobody can describe.

**Wrapping is the whole of `render.cpp`.** A line shorter than the terminal is a
carriage return, a write and an erase-to-end-of-line. Once prompt + line exceeds
the width the terminal has wrapped it across rows, a carriage return reaches only
the last one, and redrawing needs to know how many rows the previous draw used
and which of them the cursor was left on. The method is linenoise's: walk down to
the last row, clear each row on the way back up, redraw from the top, walk back
down. Row by row rather than one erase-to-end-of-display, so the redraw cannot
wipe what the terminal is showing below the prompt.

One edge case cannot be reasoned about from the arithmetic. Writing into the last
column does **not** move the cursor to the next row — terminals leave it in a
phantom column and wrap only when the next character arrives — so a cursor at the
end of a line that exactly fills its rows is drawn at the right edge of the row
above where it belongs. The newline is emitted by hand to force the wrap that has
not happened yet.

**What is not handled, and is recorded rather than hidden:** a CJK ideograph and
most emoji are two columns wide and are counted here as one, so a line containing
them redraws with the cursor one column left per character. The fix is a
`wcwidth` table and the language has no business carrying one yet.

### 9.7 The interrupt is a flag, checked where the depth guard is

`SIGINT` sets a lock-free atomic and does nothing else, because nothing else is
safe in a handler. The evaluator reads it at the top of `exec()`, beside the
stack-depth guard and for the same reason: a statement boundary is the one point
every walk passes through often and cheaply. It reports through `fail()`, so it
unwinds exactly as a stack overflow does and the run ends with a report naming
the line the walk was on when the key was pressed.

**Every loop the language has reaches there.** `while` and `for` both `exec()`
their body once per iteration and an empty body is still a Block statement, so
`satellite.statement.for (;;) { }` is caught as surely as one that does work.

**What is not caught is a single statement that takes a long time inside itself**
— a division carried to thirty-four digits, a `satellite.random.ultra` spin —
because there is no boundary inside one to check at. That is what the second
Ctrl-C is for: it means the first did not land, and the handler leaves with
`_exit(130)`.

130 is 128 + SIGINT, what every shell reports for a program stopped this way, and
an interrupted run exits with it however far the walk got. "Somebody stopped
this" and "this went wrong" are different facts, and a script that runs a
satellite program is entitled to tell them apart.

Both presses say so out loud, on **stderr**:

```
SATELLITE: CTRL+C RECEIVED: QUITTING
SATELLITE: CTRL+C RECEIVED AGAIN: QUITTING NOW
```

stderr rather than stdout, because a run whose stdout is piped or redirected —
possibly being read in another terminal entirely — would otherwise swallow the
one line explaining why it stopped, and a program that stops without saying why
looks like a crash. The **prompt** prints neither, and that is not an oversight:
at a prompt nothing is quitting, the terminal shows `^C` where the cursor was,
and a message announcing a quit would be a false statement about what just
happened.

### 9.8 The printer is the only thread that can notice in time

An interrupted run stops **pacing**. `satellite.console.display(100ms)` is a
presentation choice made by a program that is still running, and this one is
not — so a program that queued ten thousand lines at 100 ms a line still owes
seventeen minutes of terminal at the moment the key is pressed, and a drain that
honoured it would make the interrupt look like it did nothing. **Measured**:
forty lines at 300 ms, interrupted after 1.5 s, held the terminal for the full
11.7 s. With the pace abandoned it is 1.51 s, and every line still arrives — the
output was produced, so it is owed; only the spacing is dropped.

`pace(0)` cannot do it, because §9 part 2 makes the pace travel *with* each
queued line and setting a new one changes nothing about the lines already
stamped.

Nor can the check live in the run: a paced program queues its lines far faster
than they print, so by the time somebody presses Ctrl-C the walk has usually
finished and the run is already inside `drain()` — there is no other code left
running to check anything. **The printer thread is the only thread that can
notice**, which is the whole reason console_output knows what an interrupt is,
and it waits in 20 ms slices rather than one sleep, because a signal handler may
store into an atomic and nothing more. It certainly may not notify a condition
variable, so the only way the flag can be seen is for that thread to wake up and
look at it.

### 9.9 A terminal left raw is the worst failure here

There is no echo, so the user cannot see what they type; there is no line
discipline, so Ctrl-C does not work; and the fix is to type `reset` into a screen
that shows them nothing. So the restore is on **four** paths rather than one: the
`RawMode` destructor, an `atexit` handler, `restore_terminal()` by hand, and an
emergency hook that both `_exit(2)` callers run first — the SIGINT escalation
above, and the memory watchdog, neither of which runs a destructor or an atexit
handler by design.

The hook is a bare function pointer in an atomic rather than a `std::function`,
because the signal handler calls it: a load and an indirect call are signal-safe
and a `std::function`'s storage is not. `system_facts` deliberately does not
learn what a terminal is — `console_input` registers the restore, and this side
only knows there is something to run.

### 9.10 History, and not doing it behind their back

The previous entries are kept in `$HOME/.satl_history`, one line per entry,
capped at a thousand. A history that dies with the session is half a feature and
every prompt worth using remembers across runs — but writing a file into
somebody's home directory because they pressed the up arrow is exactly the kind
of thing this language does not do quietly. So the file is real, `:history`
prints its path along with the list, and `$SATL_HISTORY` names it: a path moves
it, and `none` (or empty) keeps the whole session in memory and writes nothing.
A machine with no `$HOME` gets memory-only too, rather than a dotfile in whatever
directory `satl` happened to start in.

A line is remembered **before** anything decides what it means, so a syntax
error, an abandoned block's lines and a `:` command are all recallable. A history
that only kept what worked would be missing exactly the lines somebody wants back
in order to fix them.
