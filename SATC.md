# satellite — the `.satc` file

**This file is permanent.** It specifies `.satc` — **sat**ellite **c**ompiled — the
on-disk form of a program after its language-owned words have become numbers.

Nothing here is built yet. The format is written down first because it **constrains
the numbering** (§3) and because it changes something PLAN.md had ruled out (§7),
and both are cheaper to settle now than after `words.def` lands.

Companions: [WORD_NUMBERS.md](WORD_NUMBERS.md) holds the numbers this file writes
down, [DESIGN.md](DESIGN.md) §4 explains them, [PLAN.md](PLAN.md) §2 says where the
work sits.

---

## 1. What it is

A `.satc` sits beside its source — `hello_world.satl` produces
`hello_world.satc` — and holds the same program with every language-owned path
replaced by its number. On the next run satl reads it instead of doing the walk
again.

**It is a cache, not a build product.** No user ever runs a compiler, waits for
one, or ships a `.satc` in place of source. Deleting every `.satc` on the machine
costs nothing but the walk. That is what keeps DESIGN §12's *"no JIT, and no
compile step the user ever runs"* true.

### 1.1 It is meant to be read

```
satc 1
words 003.01 c4f9a1e2
source hello_world.satl 1756304412 142

1.1.1                                    // satellite.include(satellite)

1.2 1.3(1.4.2<1.6.1> arguments)          // satellite.capsule satellite.main(…)
{
    1.5.1("Hello, World!")               // satellite.console.display
    1.15.1                               // satellite.return(satellite)
}
```

Segments are joined with `.`, so a path is one token: `1.5.1` is
`satellite.console.display`. Everything after `//` is a comment written for a
person, ignored on read, and **never trusted** — the numbers are the file.

Being readable is not decoration. A format nobody can read is a format nobody
checks, and this one encodes the meaning of a program in integers whose only
definition lives in another file. The comment column is how a person confirms that
`1.5.1` still says what they think it says.

---

## 2. The header

Three lines, and each answers a question that has exactly one wrong answer.

| line | says | on a mismatch |
|---|---|---|
| `satc <n>` | which version of *this format* | refuse, in plain words |
| `words <version> <digest>` | which numbering it was written against | ignore the file, walk the source |
| `source <name> <mtime> <size>` | which source it came from | ignore the file, walk the source |

**`words` is the load-bearing one.** A `.satc` is meaningless except against the
numbering that produced it, and DESIGN §4.3's whole warning is that a changed
numbering *silently changes what a program means*. The digest is over `words.def`,
so a numbering that has changed at all produces a different digest and every stale
`.satc` on the machine stops being read on the same instant.

Appending is legal and does not invalidate anything semantically — but it still
changes the digest, and that is the right trade: a `.satc` that is merely
out-of-date costs one walk to rebuild, and a `.satc` that is wrong costs a
program that does the wrong thing.

---

## 3. What may **not** become a number

This is the constraint that made the format worth specifying before M2.

**User-owned names are written as names.** A capsule the user wrote, a spacesuit
they defined, a global under `satellite.library.<name>` — all of these take *the
next number free when the name is first met* (WORD_NUMBERS.md §3), and that number
is not the same in the next run. §3 already states the rule this file has to obey:

> anything that writes one down must record the name and not the number

So in a `.satc`:

```
1.2 fact(1.6.4 n)          // satellite.capsule fact(satellite.variable.number n)
    ^^^ numbered            ^^^^ NOT numbered — a user name
```

`satellite.capsule` is `1.2` because the language owns it. `fact` stays `fact`
because the user owns it, and `n` stays `n` for the same reason. **This is DESIGN
§1's generating rule falling straight out of the file format** — a dotted path
rooted at `satellite` becomes numbers, a bare identifier does not.

Literals stay literal too: `"Hello, World!"` is a string in the source and a string
in the `.satc`. Numbering it would buy nothing and cost the readability §1.1 exists
for.

---

## 4. Reading comes before writing

The order matters and is easy to get backwards:

1. Look for `<source>.satc`.
2. If it exists, is well-formed, and all three header lines match — **use it, and
   do not walk the source.**
3. Otherwise walk the source as normal, and write a fresh `.satc` afterwards.

A missing, stale or unreadable `.satc` is **never an error**. It is a cache miss,
the program runs exactly as it would have, and the only cost is the walk that would
have happened anyway. A *malformed* one is different: it says something went wrong
that a person may want to know about, and DESIGN §9 asks for a plain-words note
rather than silence.

---

## 5. Writing happens on its own thread

The run does not wait for the write. The walk finishes, the program starts, and the
`.satc` is written behind it — so the first run of a program is never slower for
having produced one.

Two things that follow, and both are how a cache corrupts a machine if they are
skipped:

- **The write is atomic.** Write `<name>.satc.<pid>.tmp`, `fsync`, then `rename`
  onto the final name. `rename` within a directory is atomic, so a reader either
  sees the whole old file or the whole new one. A process killed halfway through a
  plain write leaves a truncated `.satc` that the next run would read as a program.
- **A failed write is silent and harmless.** A read-only directory, a full disk, a
  source tree owned by somebody else — none of these are the program's problem. The
  cache does not get written, nothing says anything, and the program runs. The one
  exception is §4's malformed file, which is a fact about the machine rather than a
  permission the user declined to give.

---

## 6. Open

- **Where does a `.satc` go when the source directory is not writable?** Beside the
  source is the readable answer and the one that makes `--dry-run`-style inspection
  obvious. A cache directory under `$HOME/.satl` always works and is invisible.
  Doing both means two places to look and a rule about which wins.
- **What is the digest over?** `words.def` alone is the honest answer if the file
  is the only input to the numbering. If the trie is built from more than that
  file, the digest has to cover all of it, or two different numberings can share
  one digest — which is the exact failure §2 exists to prevent.
- **Does a `.satc` survive `satellite.include` of a spaceship?** A program's
  meaning then depends on files it did not name in its own header, so either the
  header grows a line per included file or an included program invalidates the
  cache wholesale.
- **Is the number column stable enough to diff?** If two runs of the same source
  and the same numbering produce byte-identical output, a `.satc` becomes something
  a test can compare. That is worth having and it is a constraint on the writer,
  not a property that arrives for free.

---

## 7. What this changes elsewhere

**PLAN §2.3 lists `no serializable format` among the things closure compilation
deliberately does not have.** That is no longer true and the line is corrected
there: what satellite has no serialised form of is the *closure tree*, which stays
in memory and is rebuilt every run. `.satc` serialises the layer above it — the
program with its words numbered — which is the one thing that can be written down
without freezing an implementation.

The distinction is worth keeping sharp, because it is the difference between a
cache and a bytecode VM. **A `.satc` is a source file with the dictionary already
applied.** Nothing in it names a handler, an instruction, or an evaluation order.

*Companions: [WORD_NUMBERS.md](WORD_NUMBERS.md) — every number this file writes.
[DESIGN.md](DESIGN.md) — what the language is. [PLAN.md](PLAN.md) — how it gets
built. [LAYOUT.md](LAYOUT.md) — every file in the tree.*
