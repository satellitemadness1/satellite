# FOREIGN_MILESTONES — what other languages have that satellite does not yet

**What this folder is.** `src/error_reporter/foreign.cpp` recognises the common
spellings of C++, C, Python, Java, Rust and assembly, and answers with the
satellite line that does the same thing. Sometimes there is no such line. When
that happens the table names a milestone, and **this folder is where that
milestone is defined** — because a number in an error message is a promise, and
a promise with nothing behind it is worse than saying "no".

**The rule these files exist to enforce:**

> **A MILESTONE NAMED IN AN ERROR MESSAGE MUST BE DEFINED BEFORE THE MESSAGE
> SHIPS.** The reader is being told to wait. They are entitled to know what they
> are waiting for, and to read it without asking anybody.

That is [ERROR_HANDLING.md](../ERROR_HANDLING.md) §1.2's rule applied one step
earlier: a refusal that names a cause must name a real one, and "a later
milestone" is a cause. M23 had already landed when S0721 blamed it.

---

## The three answers a foreign construct can get

A row in `foreign.cpp` ends in exactly one of these, and only the second one
lands a file in this folder.

| Answer | Looks like | Example |
|---|---|---|
| **satellite says it** | the line to type | `println!` → `satellite.console.display(x)` |
| **not yet** | a milestone number | `&&` → M28 |
| **never** | said outright | `mov eax, 1` → no inline assembly, none planned |

**"Never" is a real answer and is not a failure.** Saying "satellite does not
have inline assembly and none is planned" respects the reader more than an
invented M-number they would wait for forever.

---

## The milestones

| # | What it brings | Hit by | Foreign spellings |
|---|---|---|---|
| [M27](SATELLITE/M27-break-and-continue.md) | `break`, `continue` | every loop that stops early | C/C++/Java/Rust/Python all |
| [M28](SATELLITE/M28-logical-operators.md) | `&&`, `\|\|`, `not` | every two-part condition | `&&`, `and`, `\|\|`, `or` |
| [M29](SATELLITE/M29-method-chaining.md) | more than one hop | `x.round().to_string()` | universal |
| [M30](SATELLITE/M30-switch-and-match.md) | `switch` / `match` | any multi-way branch | `switch`, `match` |
| [M31](SATELLITE/M31-user-generics.md) | generics a program writes | containers of your own | `template<`, `<T>` |
| [M32](SATELLITE/M32-capsules-as-values.md) | a capsule as a value | callbacks, sorting by key | `lambda`, `\|x\|`, `[](){}` |
| [M33](SATELLITE/M33-catching-a-refusal.md) | `try` / `catch` | recovering instead of stopping | `try`, `catch`, `throw` |

**M27, M28 and M29 are the ones that bite first**, and all three were hit inside
a single afternoon writing one real program (`infinity_data_main.satl`,
2026-09-12). They are not speculative: the workarounds in that file's comments
are what their absence costs, in the author's own words at the call site.

**M29 is no longer the worst of the three.** It was, while a chained call passed
every static check and failed only at run time — seven minutes into a program
whose build phase runs that long. S0720 became a check-time diagnostic later the
same day, so the fault is now reported before anything runs. M27 and M28 are
the two that are still silent until you hit them, and they should be scheduled
first for that reason. See [SATELLITE/M29](SATELLITE/M29-method-chaining.md).

---

## The folders

| folder | what is in it |
|---|---|
| [CXX23/](CXX23/) | every C++23 (ISO/IEC 14882:2024) feature, one file each |
| PYTHON/ | *to be written* |
| JAVA/ | *to be written* |
| RUST/ | *to be written* |
| ASM/ | *to be written* |
| [SATELLITE/](SATELLITE/) | the satellite milestones the language folders ask for |

**The language folders catalogue; SATELLITE/ is the work.** A number in
`CXX23/M20.md` is a position in a list of C++ features. A number in
`SATELLITE/M28-...` is a promise satellite has made in an error message. Only
the second kind may appear in `foreign.cpp`.

---

## Writing a new one

One file, named `M<number>-<what-it-brings>.md`, holding:

1. **What the foreign languages spell**, in each of the five
2. **What satellite makes you write instead** — the actual workaround, from real
   code where possible
3. **What it would cost to build** — where in the tree, and what it touches
4. **What it must not break** — the DESIGN section it has to stay inside

Then add the row to the table above, and to `foreign.cpp`. `tests/foreign_test`
checks that every milestone named in the table has a file here, so a number
invented in an error message fails the build rather than reaching a reader.

---

## What is deliberately not here

Constructs with **no plan**, which `foreign.cpp` answers outright rather than
with a number:

- **inline assembly** — satellite is interpreted and has no machine layer to
  reach. `satellite.bits` is the lowest it goes.
- **direct syscalls** — `satellite.system` and `satellite.file` are the ways out.
- **`goto`** — no.
- **pointers, `new`, `delete`** — not missing; *replaced*. A spacesuit is a
  reference type (DESIGN §7.4), so there is nothing to take the address of and
  nothing to free by hand. This is an answer, not a gap.
