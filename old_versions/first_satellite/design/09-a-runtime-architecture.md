*satellite design docs, §9, part 1 of 3. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§8 part 4](08-d-size-length-and-arguments.md), On: [§9 part 2](09-b-the-console-and-windows.md).*

---

## 9. Runtime architecture

### Keep the two processes

The parent is a GTK4 app owning one VTE terminal window; the child is `satl --repl` running
inside the terminal's PTY. The interpreter lives entirely in the child. (The child was
originally the same binary re-executed; the split below made it the sibling binary instead,
which changes nothing about the architecture.)

The decisive reason to keep it is one not previously written down: **the PTY gives you the
kernel tty line discipline for free** — echo, backspace, line editing, Ctrl-C, Ctrl-D,
SIGWINCH. Collapsing to one process means reimplementing all of it on top of
`vte_terminal_feed`. Secondarily it buys crash isolation and lets the watchdog's `_exit(2)`
kill the interpreter without killing the UI.

Cost: no shared address space, and `satellite.library` is a per-process singleton living in
the interpreter process only.

### Fix: crash isolation currently buys nothing

**Still outstanding.** `on_child_exited` (`window.cpp:54-57`, where the split moved it)
destroys the window unconditionally, and since it is
the only window, that quits the application. So an interpreter crash, an uncaught error, or
the watchdog's `_exit(2)` all take the terminal — **and the entire scrollback, including the
diagnostic just printed, vanishes instantly.**

Feed a message instead and offer to respawn:

```cpp
vte_terminal_feed(term, "\r\n[satellite exited: status N — press any key to restart]\r\n", -1);
```

About ten lines, and it is what converts "we have two processes" from a fact into a benefit.

### Fix: there is no way to run a program file — **done**

**Verified blocker, since fixed.** The single binary routed only `--repl`; anything else fell
through to `g_application_run` with the user's argv, so invoking it on a file printed
`GLib-GIO-CRITICAL: This application can not open files` and exited 1. **The hello world in
§2 had no way to be invoked at all.**

The fix was to route arguments *before* constructing the GtkApplication and never hand user
argv to GApplication. What the two binaries route today, in full:

| invocation | behavior |
|---|---|
| `satl` | REPL on stdin/stdout |
| `satl --repl` | the same, spelled out |
| `satl --run <file> [args]` | execute headless on stdout |
| `satl <file> [args]` | alias for `--run` |
| `satl --where` | print the resolved library directory and which tier answered |
| `satl-term` | GUI terminal, spawns `satl --repl` into its PTY |
| `satl-term <file> [args]` | GUI terminal spawning `satl --run <file> [args]` |

The split below is why `satl` alone is the REPL rather than the window: the GUI moved to its
own binary, so the interpreter no longer has a window to open and the `--window <file>` flag
this section originally proposed became `satl-term <file>` instead.

`satl-term` is the only one of the two that calls `g_application_run`, and it passes a
synthetic argv: `char *gtk_argv[] = { argv[0], nullptr };`

The headless `--run` path is also what makes the interpreter testable in the Makefile
without a display server.

The REPL needs the same door from the inside, because the GUI window has no shell behind
it: a file typed at the prompt is otherwise unrunnable without closing the window.

| prompt line | behavior |
|---|---|
| `run <file> [args]` | `interp::run_file` — same entry point as `satl --run` |
| `interpret <file> [args]` | alias |
| `--run <file> [args]` | alias, so the shell spelling works at the prompt |

`parse_run_command` (in `interp.*`, not `main.cpp`, so it is testable without gtk) splits
the line: quotes group a path containing spaces, and a **backslash is an ordinary
character** — `C:\home\a.satl` names the file the user typed. This is the `encode_raw` rule
of §3.3 again: a path is data, not source text. A verb with nothing after it prints usage
rather than reaching the parser, and a nonzero status is printed, since a REPL has no exit
status to carry a failure out to.

### The interpreter must not link the GUI

**Verified, and it was costing 90% of startup.** The one binary linked gtk4 and vte, so the
dynamic linker loaded **119 shared objects** — pango, harfbuzz, cairo, gdk-pixbuf and a
hundred more — before `main()` ran, on every invocation. `satl --run` touches none of
them.

```
the interpreter linked against gtk4                 25.9 ms
the same interpreter, not linked against gtk4        2.5 ms
a bare int main(){return 0;}                         2.2 ms
```

The interpreter's own share of hello world — read the file, lex, parse, resolve, walk, print
— is **0.3 ms**. The other 23.4 ms was the loader, for libraries the run never called into.

**Resolution: two binaries.** `satl` is the interpreter and links no GUI (6 shared objects,
five of them the C and C++ runtimes and the sixth the loader); `satl-term` is the window
(119). §9's two-process architecture is what makes the split nearly free — the window never
interpreted anything anyway, it spawns a child into the terminal's PTY and renders its bytes,
so the only change is that it spawns the sibling `satl` rather than itself.

What this is worth, against CPython, both timed whole-process with fork + execvp:

| | before the split | after |
|---|---|---|
| startup (74-line feature tour) | 1.3× slower than python3 | **5.8× faster** |
| peak RSS | 26.3 MB | 4.0 MB (python3: 9.9 MB) |

The "after" column is §14's table rather than a second measurement of the same thing: both
come from `make python`. This row read 7.6× against §14's 5.8× for one benchmark under one
name, which is what one number written in two places does. The "before" column cannot be
re-measured, because the binary it describes no longer exists.

Execution is unaffected and unflattered: see §14's note on where satellite actually sits.

### The commands are `satl` and `satl-term`, and the reason is not ours

The binaries were called `satellite` and `satellite-term` until packaging made that
impossible. **Ubuntu's archive already ships `satellite-gtk`, a GNSS data viewer, and it owns
`/usr/bin/satellite`.** Two packages that install the same path conflict, so shipping ours
under that name would need `Conflicts:`/`Replaces:` against an unrelated program — a Debian
Policy violation dressed up as a declaration, and one whose effect is that installing a
programming language removes a working GNSS tool from the machine.

What moved is exactly the command name and nothing else:

| | before | now |
|---|---|---|
| binaries | `satellite`, `satellite-term` | **`satl`, `satl-term`** |
| Debian packages | `satellite`, `satellite-term` | unchanged — both names are free |
| the language | satellite | unchanged |
| the reserved word | `satellite` | unchanged |
| source extension | `.satl` | unchanged |

The keyword is the point worth being explicit about: `satellite.capsule`,
`satellite.variable.number` and every other path in this document are untouched, because §1's
generating rule is about the language, and the command name is about a filesystem that
already had an occupant. A language whose surface syntax bent to accommodate a package
archive would be the wrong trade; renaming a binary costs a `make install` line.

### Finding the installed library

`satl` searches three places for its library directory, in order, and stops at the first that
exists:

1. `$SATELLITE_PATH`, which is the development override — a working tree is not an install.
2. A path relative to `/proc/self/exe`, which is what makes a tarball or a build tree
   relocatable: the binary finds its own siblings without being told where it lives.
3. The compiled-in `-DSATELLITE_LIB_DIR`, baked from the Makefile's `prefix`, which is what a
   distribution package gets.

`satl --where` prints the answer **and which of the three produced it**, because those are
different diagnoses: "found it next to the binary" and "fell through to the compiled-in
default" describe two different broken installs, and the path alone does not say which
happened. It exists to make an install falsifiable without running a program. §16's search
order for included files starts from this directory as its last tier.
