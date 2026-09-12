# satellite — error handling, and how the interpreter explains itself

**This file is a proposal, not a record.** It says what satellite's refusals should
be able to tell you, and why the present ones cannot.
[DESIGN.md](DESIGN.md) says what the language is and [PLAN.md](PLAN.md) says how it
gets built; this is a third thing — how the implementation answers the question
*"why did you say no?"* — and it exists because on 2026-09-12 it could not.

**Status, 2026-09-12.** Nothing in §4 is built. §1 is a bug that happened, §2 is
its diagnosis, and the rest is the work. The one part already done is the fix in
§1.3, which repaired the instance and not the class.

**Cite this file by section number**, the rule DESIGN.md's own preamble sets, and
for the same reason: line numbers die the moment the file is edited.

---

## 1. The bug this file is a receipt for

### 1.1 What happened

A program used `satellite.thread.new`. Run from a file, it worked. Typed at the
prompt, one line apart, the same call answered:

```
satl: <prompt>:1:51: error S0721: `new` is a path satellite has a number for
                     and nothing behind yet -- a later milestone
```

The author read that, believed it, and went looking for the milestone that would
build threads. Threads had been built three weeks earlier, at M23. The milestone
was not missing. The *row in this arm's handler table* was missing, because M23
called `thread::install_handlers()` from `run_command.cpp` and from nowhere else.

### 1.2 Why the message was the worst possible shape

It was not wrong. `eval::Handlers::table()`, in that process, genuinely had nothing
behind `1 23 1`, and the message described that faithfully.

**It was truthful about the implementation and false about the language**, and it
resolved that gap by blaming the language. So it sent a reader who trusted it to
the one place the answer was not — and the more carefully they read it, the further
away they went. A refusal that is merely unhelpful wastes a minute; a refusal that
is *confidently misdirecting* costs an afternoon, and this one did.

> **A REFUSAL THAT NAMES A CAUSE MUST NAME THE RIGHT ONE, OR NAME NONE.**
> Guessing at a cause is worse than reporting only the symptom, because the guess
> is what the reader acts on.

### 1.3 What was done, and what it did not do

`thread::install_handlers()` was added to `src/satellite_prompt/prompt.cpp` and to
`src/programs/evaluate_commands.cpp`. That repaired the instance.

It repaired nothing about the class. There are three hand-maintained install lists;
nothing compares them; and the next word that lands in one arm will produce exactly
this message again. §4.3 is the part that makes it stop.

---

## 2. Three things "not available" can mean

When a path does not answer, satellite is capable of exactly one of these being
true, and today it can only say one of them:

| # | What is true | What satellite says |
|---|---|---|
| 1 | The language has no such word | `S0521` — correct, and good |
| 2 | The word is numbered; nothing is built anywhere | `S0721` — correct |
| 3 | The word is numbered, **built**, and this arm did not install it | `S0721` — **wrong** |

Case 3 is a *build-and-wiring* fact wearing case 2's clothes. They need different
words because they need different actions from the reader: case 2 means wait or
implement; case 3 means one line in one file, today.

**The interpreter already knows which it is.** `eval::Handlers::table()` knows the
row is absent. The other arms' install lists know the handler exists. Nothing
currently puts those two facts in the same room — that is the entire defect, and
§4.1 is putting them in the same room.

### 2.1 This rule is already in the tree, applied somewhere else

None of the above is a new principle. `errors.def` makes exactly this argument for
`satellite.container.arguments`, in the comment above `S0731`:

> a miss cannot answer `nothing`, because two of the object's rows are a later
> milestone's and refuse, so **"absent" and "present and not answering yet" must
> never be one answer**.

That sentence is the whole of §2, written for the arguments object and never
carried to the handler table — where the same two states exist, and *are* one
answer. So this file is not proposing a rule. **It is proposing that a rule the
tree already holds be applied where it currently is not.**

---

## 3. The rule this file is arguing for

> **A REFUSAL MUST NAME WHAT WOULD HAVE MADE IT NOT HAPPEN.**

Not what went wrong — what would have gone right. For each of the three cases:

1. *"there is no such word"* → the nearest words that exist
2. *"nothing is built"* → the milestone that will build it
3. *"not installed here"* → **which arm has it, and which file installs it**

A message that cannot say any of these should say the symptom and stop, rather than
reach for the most likely-sounding cause. That is the §1.2 rule stated as a duty.

---

## 4. The proposal

Four pieces. §4.1 and §4.2 make refusals honest; §4.3 makes this class of bug
impossible to commit; §4.4 makes a whole run reportable in one artifact.

### 4.1 A refusal carries its provenance

Today `m.refuse(text)` takes a finished sentence. The caller has already decided
what the cause was, at the point where it knows least.

Instead, a refusal about a path should carry **facts**, and the renderer should
compose the sentence from them:

```
struct Refusal {
    words::PathId  path;        // the number -- 1 23 1
    Arm            arm;         // Run, Prompt, Call, Check
    RowState       row;         // Absent, Installed, InstalledElsewhere
    const char    *milestone;   // what the row says, when there is a row
    const char    *installer;   // "satellite_thread/handlers.cpp", when known
};
```

`RowState::InstalledElsewhere` is the whole point. It is the state that exists in
the world and has no name in the code today, which is precisely why it got
rendered as `Absent`.

With it, §1.1's message becomes:

```
satl: <prompt>:1:51: error S0734: `satellite.thread.new` (1 23 1) is built, and
      this arm did not install it.
      the prompt installs 9 handler groups; `--run` installs 11.
      thread::install_handlers() is called from programs/run_command.cpp
      and not from satellite_prompt/prompt.cpp.
      this is a wiring gap and not a milestone -- M23 built it.
```

**That message ends the investigation instead of starting a wrong one.**

### 4.2 Split S0721

`S0721` keeps case 2 — numbered, nothing built, here is the milestone. It is a
good error and it is not the problem.

A **new** code takes case 3, with the shape above. Two codes because they are two
different things to *do*, and `satl --errors` is a list a person reads to learn
what can go wrong.

**Which number, checked rather than assumed.** `S0722` is taken (`takes {2} and was
given {3}`); the evaluator block currently ends at `S0733`, and `S0732` is an
undocumented gap that should be left alone in case it was reserved. `errors.def`
requires the numbers to **ascend in the file** — that is its replacement for
words.def's "a number is a position" — so the honest mint is **`S0734`, appended**.
Confirm against `satl --errors` before writing it: the first draft of this section
asserted `S0722` was free, and it was not.

### 4.3 The arms audit — the clever part, and the cheap one

**Every path in this language is a number.** That is DESIGN §4's whole idea, and it
has a consequence nobody has collected yet: *the set of rows an arm installs is
computable data, so two arms can be diffed mechanically.*

So: build the handler table for every arm, and compare.

```
satl --arms                    # what each arm installs, and what differs
satl --arms --check            # exit non-zero if anything differs undeclared
```

The differences that are **on purpose** get declared once, as data, beside the
arms rather than in prose:

```cpp
// --call runs one capsule with no printer behind it, so these are absent by
// decision and not by accident. evaluate_commands.cpp argues both at length.
constexpr ArmException kCallExceptions[] = {
    { Arm::Call, Group::Console, "no printer is started under --call" },
    { Arm::Call, Group::Help,    "help's whole answer is printed"     },
};
```

Then `make test` fails the day an arm gains a group its siblings lack without a
line saying why. **The bug in §1 becomes unrepresentable**, and the reasoning that
currently lives only in comments becomes a thing the build enforces.

This is the piece to build first. It is perhaps 150 lines, it needs no change to
any error message, and it would have caught M23's omission the day it was made.

> **PROSE MAY EXPLAIN A DIFFERENCE. IT MAY NEVER BE THE ONLY PLACE THE DIFFERENCE
> LIVES.** — WORD_NUMBERS.md's rule about numbers, which is the same rule.

### 4.4 `--report` — everything, in one artifact

The question "what is wrong" is usually answered by a dozen small facts nobody
thinks to gather. `satl --report <file>` gathers them:

```
satl --report infinity_data_main.satl > report.txt
```

and writes, in one file: the version and build stamp; every machine fact
`--limits` knows and where each value came from; the arms table from §4.3;
`--resolve`'s output including every path that resolved and to what number; the
cache's state and whether it was read or rebuilt; and, if the program was run,
the refusal with its full §4.1 provenance.

**It is a bug report a person can paste without being asked six questions first.**
Everything in it is already computed somewhere in this tree; none of it is
currently collectable in one command.

### 4.5 `--why`, once §4.1 exists

With provenance on refusals, the prompt can answer a follow-up:

```
madness: satellite.thread.new(work())
satl: error S0734: ... this arm did not install it.
madness: why
satl: 1 23 1 -- satellite.thread.new
      numbered   words.def:826, minted at M23
      built      satellite_thread/handlers.cpp:184, arity 1, deferred arg 0
      installed  run_command (yes), prompt (NO), call (NO), check (n/a)
      you are in prompt.
```

This is not a new subsystem. It is §4.1's struct, printed.

---

## 5. What this costs

**Nothing at run time.** A `Refusal` is built only on the path where a program is
already stopping; the arms table is built by a test and by `--arms`, not by a run.
`--report` is a command, not a mode.

**One new error code**, which is a permanent commitment. `errors.def`'s own
preamble is explicit that an error code is "what a PERSON LOOKS UP" and so may be
assigned out of order and may leave gaps — but it is spent the moment it ships.

**The install lists become data instead of code**, which is the only structural
change here and is the one worth having anyway.

---

## 6. What this does not change

- **No refusal becomes a warning.** Everything that stops the run still stops it.
  This is about what the stop *says*.
- **No message gets longer for the common case.** Case 1 and case 2 render exactly
  as they do today; only case 3 gains a sentence, and only because it currently
  renders as a different case entirely.
- **The numbering is untouched.** §4.3 reads the numbers; it does not mint any.

---

## 7. The order to build it

1. **§4.3, the arms audit.** Smallest, catches the live class of bug, needs no
   message changes. A day's work and `make test` gets a new guarantee.
2. **§4.1, provenance on refusals.** The struct and the renderer. Nothing user-
   visible changes until step 3 uses it.
3. **§4.2, the new code.** One code, one render path, one row in `--errors`.
4. **§4.4, `--report`.** Assembly of things that already exist.
5. **§4.5, `--why`.** Only worth doing once 1–3 are in; it is their output.

Steps 1 and 2 are independent and can be done in either order. Nothing after
step 2 is worth starting before it, because 3, 4 and 5 are all renderings of the
struct it introduces.

---

## 8. The general lesson, for messages this file does not cover

The §1 bug was not really about threads. It was about an error message that knew
two facts, could only report one, and **invented a relationship between them**.
That shape recurs wherever the implementation can be wrong in a way the language
cannot:

- a cache that is stale reporting as a program that is different
- a handler present but with the wrong arity reporting as a handler absent
- a path that resolved against an older `.satc` reporting as a path that moved

Each of those is the same defect: **a true statement about this binary, phrased as
a statement about satellite.** When writing any refusal, the test is §3's — name
what would have made it not happen, and if you cannot, say only what you saw.

---

## 9. When the line is another language

**Everybody arrives from somewhere else.** The first thing a new reader types is
the language they already know, and `print("hi")` gets them *"expected a
declaration"* — which is true, and is an answer to a question they did not ask.
They know what they wanted. They do not know how to say it here.

`src/error_reporter/foreign.cpp` is a table of the distinctive spellings of C,
C++, Python, Java, Rust and assembly, and what satellite says instead. It hangs
off `render()` under the "did you mean" line, because it is the same kind of
help one step wider: `did you mean console?` is about one misspelled word, this
is about a reader fluent in another language.

```
$ satl --check f.satl
       that looks like Rust. satellite spells it:
           satellite.console.display(x)
           there is no format string -- build the line with `+`
```

### 9.1 It has to be quiet, and the guard is one line

This runs on **every rendered diagnostic**, so a matcher that fires on ordinary
satellite would put noise under every error in the language. The guard is that a
line containing `satellite.` is never matched — an author who wrote the word is
already here, and telling them how to spell what they just spelled is noise.

**Measured 2026-09-12:** zero matches across all of `example/` and across
`infinity_data_main.satl` (2,474 lines), and the `did you mean` path unchanged.

### 9.2 Three answers, and the third is the honest one

| answer | when | example |
|---|---|---|
| the satellite line | satellite has it | `println!` → `satellite.console.display(x)` |
| a milestone number | satellite does not yet | `&&` → M28 |
| **"does not have it"** | and never will | `mov eax, 1` → no inline assembly, none planned |

**The third is a real answer and not a failure.** Saying outright that satellite
has no inline assembly respects the reader more than an invented number they
would wait for forever.

---

## 10. A milestone named in a message is a promise

This is §1.2's rule one step earlier. S0721 blamed "a later milestone" for work
M23 had already done; a foreign-syntax row saying "M28 brings it" is the same
sentence said deliberately, and it is only allowed to be true.

> **A MILESTONE NAMED IN AN ERROR MESSAGE MUST BE DEFINED BEFORE THE MESSAGE
> SHIPS.** The reader is being told to wait. They are entitled to know what for,
> and to read it without asking anybody.

So every number `foreign.cpp` names is defined in
[FOREIGN_MILESTONES/SATELLITE/](FOREIGN_MILESTONES/SATELLITE/), one file each,
saying what the foreign languages spell, what satellite makes you write instead,
what building it would cost, and what it must not break.

| milestone | brings | why it is named |
|---|---|---|
| [M27](FOREIGN_MILESTONES/SATELLITE/M27-break-and-continue.md) | `break`, `continue` | every loop that stops early |
| [M28](FOREIGN_MILESTONES/SATELLITE/M28-logical-operators.md) | `&&`, `\|\|`, `not` | every two-part condition |
| [M29](FOREIGN_MILESTONES/SATELLITE/M29-method-chaining.md) | more than one hop | `x.round().to_string()` is S0720 |
| [M30](FOREIGN_MILESTONES/SATELLITE/M30-switch-and-match.md) | `switch` | any multi-way branch |
| [M31](FOREIGN_MILESTONES/SATELLITE/M31-user-generics.md) | user generics | a container of your own |
| [M32](FOREIGN_MILESTONES/SATELLITE/M32-capsules-as-values.md) | a capsule as a value | callbacks, sort keys |
| [M33](FOREIGN_MILESTONES/SATELLITE/M33-catching-a-refusal.md) | catching a refusal | `try`, `catch`, `std::expected` |

**M27, M28 and M29 were all hit in one afternoon** writing one real program, and
the workarounds are in that program's comments at the call site. They are not
speculative.

[FOREIGN_MILESTONES/CXX23/](FOREIGN_MILESTONES/CXX23/) is the other half: a
catalogue of every C++23 feature and which of the three answers it gets. Its
numbers are catalogue positions, not promises, and must never appear in a
message.
