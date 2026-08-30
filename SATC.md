# satellite — the `.satc` file

**This file is permanent.** It specifies `.satc` — **sat**ellite **c**ompiled — the
on-disk form of a program after its language-owned words have become numbers.

**Built at M4.5 on 2026-08-30** — [MILESTONES/M4.5.md](MILESTONES/M4.5.md) is the
record and `src/satellite_cache/` is the module. The format was written down
first because it **constrains the numbering** (§3) and because it changes
something PLAN.md had ruled out (§7), and both were cheaper to settle before
`words.def` landed than after. Three things below are marked as corrected by the
build: §1's location, §1.1's spelling of a path, and the first bullet of §6.

Companions: [WORD_NUMBERS.md](WORD_NUMBERS.md) holds the numbers this file writes
down, [DESIGN.md](DESIGN.md) §4 explains them, [PLAN.md](PLAN.md) §2 says where the
work sits.

---

## 1. What it is

A `.satc` holds the same program as its source with every language-owned path
replaced by its number. On the next run satl reads it instead of doing the walk
again.

**Every `.satc` on a machine lives in `$HOME/.satl/cache`.** *(Decided
2026-08-30, when the writer was built; this paragraph used to say a `.satc` sits
beside its source and §6's first bullet asked the question.)* It is named
`<stem>.<16 hex digits of the source's absolute path>.satc`, so two programs
called `hello_world.satl` in two directories are two files. One directory rather
than two, because "beside the source" cannot be the only answer — a program run
out of `/usr/share` or a read-only checkout would never get a cache at all — and
because §6's own wording rules out doing both: *"two places to look and a rule
about which wins."* What one directory costs is a name collision, and §2's
`source` line is what makes one harmless rather than the digest making one
unlikely. `src/satellite_cache/file.cpp` carries the argument in full.

**It is a cache, not a build product.** No user ever runs a compiler, waits for
one, or ships a `.satc` in place of source. Deleting every `.satc` on the machine
costs nothing but the walk. That is what keeps DESIGN §12's *"no JIT, and no
compile step the user ever runs"* true.

### 1.1 It is meant to be read

This is `satl --satc example/hello_world.satl`, exactly:

```
satc 1
words 003.01 5d2cb10461e38766
source hello_world.satl 1787935778 273

#1.1.1                                   // satellite.include(satellite)

#1.2 #1.3(#1.4.2<#1.6.1> arguments)      // satellite.capsule satellite.main satellite.container.list satellite.variable.string
{
    #1.5.1("Hello, World!")              // satellite.console.display
    #1.15.1                              // satellite.return(satellite)
}
```

Segments are joined with `.`, so a path is one token: `#1.5.1` is
`satellite.console.display`. Everything after `//` is a comment written for a
person, ignored on read, and **never trusted** — the numbers are the file.

### 1.1.1 The `#` is part of the format

*(Added 2026-08-30. This example used to write a path as bare digits, and
writing the thing found that it could not.)*

`satellite.console` is `1 5`, which closes up to `1.5`, which is also the float
one-and-a-half. Both are legal tokens in the same file, and
`example/super_advanced.satl` already puts a float and a path within three lines
of each other. It happens to be decidable today, because all 24 two-segment paths
are namespaces and a namespace is never a value — but that is a property of the
numbering that nothing enforces, and WORD_NUMBERS §3 lets a numbering grow.

`#` settles it in one character, and it is `#` rather than anything else because
**no satellite program can contain one**: the lexer gives it back as `Punct(#)`
and the parser has no rule that accepts it. So a `#` in a file is proof that the
file is a `.satc` and not a source, which is the property worth having a marker
for at all.

Being readable is not decoration. A format nobody can read is a format nobody
checks, and this one encodes the meaning of a program in integers whose only
definition lives in another file. The comment column is how a person confirms that
`#1.5.1` still says what they think it says.

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
numbering *silently changes what a program means*. The digest is over **the
numbering** — §6's second bullet is where that was settled and why it is not over
`words.def`'s bytes — so a numbering that has changed at all produces a different
digest and every stale `.satc` on the machine stops being read on the same
instant.

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
#1.2 fact(#1.6.4 n)        // satellite.capsule fact(satellite.variable.number n)
 ^^^ numbered               ^^^^ NOT numbered — a user name
```

`satellite.capsule` is `#1.2` because the language owns it. `fact` stays `fact`
because the user owns it, and `n` stays `n` for the same reason. **This is DESIGN
§1's generating rule falling straight out of the file format** — a dotted path
rooted at `satellite` becomes numbers, a bare identifier does not.

Literals stay literal too: `"Hello, World!"` is a string in the source and a string
in the `.satc`. Numbering it would buy nothing and cost the readability §1.1 exists
for.

### 3.1 Paths become numbers. Selectors do not.

The rule in §3 is about *ownership* — the user's names stay names. There is a
second line running the other way, through words the **language** owns, and it
decides more of the file than the first one does.

A **path** is rooted at `satellite` and resolves with no context. Wherever
`satellite.console.display` appears it is `#1.5.1`. Substituting it is sound
anywhere in the file, which is what makes the whole format a substitution.

A **selector** is a bare word after a receiver — `sort` in `my_list.sort()`. It is
language-owned and it does have a number (`satellite.container.list.sort` is
`1 4 2 3`), but that number is reachable only *through the receiver's type*.

So a `.satc` keeps the sugar exactly as written:

```
my_list.sort()                 // stays. `sort` is a selector.
#1.5.1("Hello, World!")        // becomes a number. `display` is in a path.
```

**What it must never do is flip the sugar into its dispatch form.**
`satellite.container.list.sort(my_list)` is what DESIGN §6.4 defines as the
dispatch-table key — "the (type node, method name) key... with the receiver written
out as the first argument" — and §6.4 says no program may write it. Writing it here
would make the file name a handler, which §7 is the sentence forbidding.

### 3.2 The pipeline decides this, not taste

§3.1 is not a readability preference. It is forced, and the proof is where the
writer runs.

PLAN §8 schedules `.satc` as **M4.5 — after M4's parser and before M6's resolve.**
DESIGN §6.3 keeps the parser resolution-free on purpose: it "emits a flat
Member/Call/Index chain and resolves it afterwards, letting each object answer for
its own members." So at the instant the file is written, nothing has yet decided
that `my_list` is a list, and the identity of `sort` is **unknowable** — not
awkward to record, unavailable.

That is also why the file records no evaluation order and §7's claim survives: a
`.satc` holds a **tree**, and a tree's evaluation order is decided by the evaluator
on every run. There is no order in the file to get wrong.

WORD_NUMBERS.md §1.5 holds the other half of this — why a selector is still worth
numbering even though the number never appears here. Dispatch is
`handlers[path_id]`, one array index, and it is reached through resolve rather than
through the file.

---

## 4. Reading comes before writing

The order matters and is easy to get backwards:

1. Look for this source's `.satc` in `$HOME/.satl/cache` (§1).
2. If it exists, is well-formed, and all three header lines match — **use it, and
   do not walk the source.**
3. Otherwise walk the source as normal, and write a fresh `.satc` afterwards.

`src/satellite_cache/read.cpp` is steps 1 and 2, `save.cpp` is step 3, and
`src/programs/cache_command.cpp` is the order — which is the whole of
`satl --satc <file>`, the milestone's consumer.

**The numbers become words again and the ordinary parser reads them.** A `.satc`
already *is* a satellite program — one with its language-owned words spelled as
numbers — so the reader substitutes `satellite.console.display` back in for
`#1.5.1` and hands the text to the lexer and the parser a source goes through.
The alternative was a second grammar for `.satc`, which would be a second place
the language is defined and would drift from the first. What that buys is the
strongest check the format can have, and it is what
`tests/satc_test/reading.cpp` asserts on every acceptance program: write a
program, read the file back, write *that*, and the two files are identical.

A missing, stale or unreadable `.satc` is **never an error**. It is a cache miss,
the program runs exactly as it would have, and the only cost is the walk that would
have happened anyway. A *malformed* one is different: it says something went wrong
that a person may want to know about, and DESIGN §9 asks for a plain-words note
rather than silence. The note satl writes for one, in full:

    satl: /home/x/.satl/cache/hello_world.4b3528cea7f4dcc8.satc names `#1.99.1`,
    which this satl's numbering does not have. It was ignored and the source was
    read instead, so nothing is wrong with your program; deleting the file is
    safe, because a `.satc` is only a cache.

**A stale file says nothing, and that is as much a requirement as the note is.**
Appending a word to the language moves the digest, which invalidates every
cached program on the machine at once (§2); a note on each would be a hundred
lines of output about a cache doing exactly its job.

### 4.1 What a cache hit saves, today, is the walk and not the numbering

*(Written 2026-08-30, when the reader was built, because it is the kind of thing
that is otherwise discovered by whoever wonders why the cache is not faster.)*

§1 says satl "reads it instead of doing the walk again", and the reader does
exactly that — but the tree it hands back has **nowhere to put the `PathId`s it
just read**. `ast.hpp` reserves the side table indexed by node for M6 and
forbids M4 to put a mutable field on a node, so the honest M4.5 reader produces
the TREE and M6's resolve still numbers every path in it. That matches what PLAN
asks M4.5 for — read, check, write and refuse, and no more — and it means the
cache does not pay for itself until M6's resolve learns to skip a path the file
has already numbered. **That is M6's sentence to add, and it is written here so
that it is inherited rather than rediscovered.**

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

**The thread is joined and not detached, which this section did not say and had
to be decided when it was built.** "The run does not wait for the write" is about
where the wait goes, not about whether there is one: `satl hello_world.satl`
finishes in under a millisecond, so a *detached* writer is a thread the process
exits out from under — the first run pays for the walk, writes nothing, and the
second run pays for it again, forever. Joining puts the wait after the program
instead of in front of it, which is what this section actually asks for.

### 5.1 The transformation order

Five steps, and the first four are all parse-tree work. If any of them needs
resolve, the writer is in the wrong place in the pipeline (§3.2) and the bug is
there rather than here.

1. **Collapse aliases.** `hexadecimal` → `hex`'s node, `.range(a, b)` → the
   `(min, max)` node, all six spellings of `arguments` → one node
   (WORD_NUMBERS.md §2.3). First, because an alias has no number of its own to
   write.
2. **Classify every dotted chain** — a path rooted at `satellite`, a selector after
   a receiver, or a user name. Everything downstream depends on this and only the
   first branch continues.
3. **Absorb language-owned arguments.** `include(satellite)` → `1.1.1`. After (2),
   because it depends on the argument being word 1 rather than a user value.
4. **Slot by arity.** `input()` at `1 5 2` against `input(prompt)` at `1 5 3`.
   Counting, not resolving.
5. **Walk the trie and emit.**

Three things stay untouched through all five: user names (§3), literals (§3), and
selectors (§3.1).

**A literal option is not folded here.** WORD_NUMBERS.md §1.5 lets
`my_list.sort("down")` intern to a different `PathId` than `sort("up")` — but that
is a resolve-time decision, and §3's "literals stay literal" governs the file. The
`.satc` keeps `"down"` as a string. The runtime keeps the number. This is the one
place the numbering deliberately says more than the file does, and it is not a
contradiction as long as nobody tries to make the file say it.

### 5.2 The writer must emit in source order

This is the one ordering the file genuinely has to preserve, and it is not
evaluation order.

WORD_NUMBERS.md §3: a user capsule or spacesuit takes the next number free under
its parent, "allocated when the name is first met." *First met* is an order. A
program run from source numbers `fact` and `helper` in the order they appear; the
same program read back from a `.satc` re-allocates them, and it must arrive at the
same answer.

So the writer emits declarations in the order the source declared them. It does not
sort, group, or hoist. This is a constraint on the writer rather than a property
that arrives for free — the same one §6 is reaching for when it asks whether the
number column is stable enough to diff, and satisfying it answers both.

---

## 6. Open

- ~~**Where does a `.satc` go when the source directory is not writable?**~~
  **Answered 2026-08-30, when the writer was built.** `$HOME/.satl/cache`, and
  only there. §1 above carries the answer and the argument; `src/satellite_cache/file.cpp`
  carries the long form, including why the file name has a digest of the
  source's absolute path in it and why the `source` header line is still checked
  after that digest has already made a collision unlikely.
- ~~**What is the digest over?**~~ **Answered 2026-08-28, when M2 was built.** It
  is over **the numbering** — every node's parent, number, kind and text in file
  order, then every alias — and not over `words.def`'s bytes. Three reasons, in
  the order they bite, and `src/satellite_words/words_digest.hpp` is where they
  live: every input to the numbering is in that one file, so this bullet's own
  worry does not arise and the function is the one place to change if it ever
  does; hashing the bytes would move the digest when a **comment** moved, and
  `words.def` is more comment than data, so every cached `.satc` on the machine
  would be invalidated to record that a sentence was rewritten; and a `constexpr`
  digest needs no build step, where a `sha256sum` in the Makefile would leave a
  hand-compiled translation unit with no digest at all — and a `.satc` written by
  a binary whose digest defaulted to "unrecorded" is worse than no cache.

  What §2 asks for is unchanged and still holds: appending a word, moving a row,
  renaming a spelling or adding an alias all move it. `satl --words` prints it.
- **Does a `.satc` survive `satellite.include` of a spaceship?** A program's
  meaning then depends on files it did not name in its own header, so either the
  header grows a line per included file or an included program invalidates the
  cache wholesale.
- ~~**Is the number column stable enough to diff?**~~ **Answered by §5.2**, which
  the writer has to satisfy anyway for user-name allocation to survive a cache hit.
  Emit in source order and byte-identical output falls out, so a `.satc` becomes
  something a test can compare.

- **A `.satc` is lossy, and nothing yet says so where a reader would look.** §5.1's
  alias collapse means the file cannot say whether the source wrote `hex` or
  `hexadecimal`, or which of six spellings of `arguments` was used. Harmless for a
  cache — the program means the same thing either way — but it does mean a `.satc`
  is never a substitute for its source, and §1.1 invites exactly that mistake by
  making the file pleasant to read.

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
