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

**AND THERE IS A THIRD KIND OF TOKEN SINCE M19.6, WHICH IS `0#down`.** A file
holds three things now and not two: a path written as its number (`#1.5.1`), a
literal written as itself (`"down"`), and an **option** written as neither
(`0#down`). This section gains a row rather than losing one — an option is not a
literal, and the rule above is untouched:

| in the source | in a `.satc` | why |
|---|---|---|
| `satellite.console.display` | `#1.5.1` | a path, and §3.1 |
| `"Hello, World!"` | `"Hello, World!"` | a literal, and the paragraph above |
| `my_list.sort("down")` | `my_list.sort(0#down)` | an option, and §5.1 |

**The `0#` is the author's spelling and the derivation is one sentence of
theirs**: *"`#` stands for a number in a `.satc` file, so `0#` stands for an
option."* **It carries no quotes** — also the author, 2026-09-08 — because the
prefix has already said this is an option rather than a string, and `0#"down"`
would be the file saying the same thing twice in two notations that could
disagree. What keeps it unambiguous is a **letter** after the mark where a path
has a digit, and `src/satellite_cache/paths.hpp` carries both halves of the
test.

**It names the OPTION and not the ROW, and that was the choice.** `#1.4.2.5`
would have said the answer and lost what the program wrote; `0#down` says which
word was written, so the file can still be read as the program it came from.
What it costs is that the number is not in the file, and §5.1 is where that is
paid.

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

***M19.6 MOVED THE WRITE AND THIS SECTION IS WHERE THAT LANDS.*** Everything
below was true from 2026-08-30 until 2026-09-09 and is kept because it is the
argument the format was built on. What changed is the SCHEDULE and not the
reasoning: the writer now runs **after** resolve, so the things this section
calls "unknowable" are knowable, and §5.1's list has a sixth step. **The rule it
draws is still the rule** — a selector is still bare in the file (§3.1), because
knowing a type is not the same as having somewhere honest to put a number that
means one thing for the language and another for a user. Only the OPTION moved,
and only because an option is a word the program wrote.

PLAN §8 schedules `.satc` as **M4.5 — after M4's parser and before M7's resolve.**
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
3. Otherwise walk the source as normal, **resolve it**, and write a fresh
   `.satc` afterwards.

**"AND RESOLVE IT" IS M19.6 AND IT CLOSED A HOLE AS WELL AS OPENING A STEP.** A
`.satc` is now written only for a program that PARSED **and RESOLVED**, where
before it was written for anything that parsed. That is not tidiness: the file
records a decision taken about the program's types (§5.1 step 6), so writing one
for a program whose names did not check would cache a fold nothing verified. It
is the same rule the parse test already kept, one pass later — *a file that is
read back INSTEAD of its source must not carry anything the source would have
been refused for.*

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
just read**. `ast.hpp` reserves the side table indexed by node for M7 and
forbids M4 to put a mutable field on a node, so the honest M4.5 reader produces
the TREE and M7's resolve still numbers every path in it. That matches what PLAN
asks M4.5 for — read, check, write and refuse, and no more — and it means the
cache does not pay for itself until M7's resolve learns to skip a path the file
has already numbered. **That is M7's sentence to add, and it is written here so
that it is inherited rather than rediscovered.**

**M7 ADDED IT ON 2026-08-31, AND THE SOMEWHERE IS NOT THE TREE.** `ast.hpp` still
forbids a mutable field on a node and PLAN §2.2 still says why, so what
`unnumber()` records is a list of `cache::Mark` — **where each substitution's
words END in the text the parser is about to read**, and the `PathId` they stood
for. It is a fact about that text and it dies with it, which is the only place
this could go without the tree carrying it.

**`ends` AND NOT `starts`, AND THAT IS THE WHOLE OF WHY IT WORKS.** A chain's
root is the same token for `satellite.time.now()` and for the `.some_function()`
wrapped around it, so keying on where a substitution BEGAN would hand the outer
node the inner one's number. Every node that names a path is anchored at the
path's **last segment** — `ast.hpp`'s *"the token that NAMES the node"* — so the
end of the substituted words belongs to exactly one node, whichever node that
turns out to be. The argument list is never counted in it: a row may keep its
parentheses when the words are written back (`input()` is `1 5 2` and the number
says its own brackets) and those characters belong to no node's anchor.

**AND WHAT IT SAVES IS COUNTED RATHER THAN TIMED.** §4.1's table above is what
says why: on a 273-byte program the walk is far under what a shell loop can see.
`satl --resolve` prints the two counts and they are exact — **28 walked cold
against 14 walked and 14 taken warm** on `example/frames.satl`. MILESTONES/M7.md
§5, and §7 records that a count is also the only thing that could catch the skip
being deleted, because every number stays right without it: the walk is the
fallback and the fallback works.

---

## 5. Writing happens on its own thread

The run does not wait for the write. The walk finishes, the program starts, and the
`.satc` is written behind it — so the first run of a program is never slower for
having produced one.

**M19.6 DELAYED THE THREAD'S START BY ONE PASS AND IT MEASURED AS FREE.** The
write now happens after resolve rather than after the parse, which PLAN asked to
be measured rather than asserted. Taken 2026-09-09 on the same machine, same
build flags, min of 50 — the baseline is `89dc0c4`, the commit before the move:

| | before the move | after |
|---|---|---|
| `satl --version` (the floor) | 2.01 ms | 1.99 ms |
| `satl --satc hello_world.satl`, cold | 3.77 ms | 3.75 ms |
| `satl --resolve frames.satl`, cold | 3.90 ms | 3.90 ms |

**The first attempt at this table compared `satl` against `satl.haswell` and
reported a 0.17 ms regression that does not exist** — those two are built with
different `-march`, so what it measured was codegen. Recorded because the wrong
number was believable and the right one is not obviously different from it.

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

6. **Record which selectors folded an option.** `my_list.sort("down")` becomes
   `my_list.sort(0#down)`. **Added at M19.6**, and it is the only step that is
   not parse-tree work — which is why it could not exist until the write moved.

***THE FIVE STEPS ABOVE ARE STILL FIVE AND STILL PARSE-TREE WORK.*** The sixth
is separate rather than folded into the list because the sentence that opens
this section — *"if any of them needs resolve, the writer is in the wrong place
in the pipeline"* — is still the right test for those five. Step 6 is the one
that needs resolve, and the writer moved to meet it rather than the other way
round.

**A literal option WAS not folded here, and this is what changed on 2026-09-09.**
*(The paragraph this replaces is kept below, because it is the argument that had
to be answered rather than a mistake.)* WORD_NUMBERS.md §1.5 lets
`my_list.sort("down")` intern to a different `PathId` than `sort("up")`, and the
file now records that it did — as `0#down`, which §3 lists as a third kind of
token beside a number and a literal.

> **The paragraph that stood here until M19.6:** *"A literal option is not folded
> here. [...] that is a resolve-time decision, and §3's 'literals stay literal'
> governs the file. The `.satc` keeps `"down"` as a string. The runtime keeps the
> number. This is the one place the numbering deliberately says more than the
> file does, and it is not a contradiction as long as nobody tries to make the
> file say it."*

**WHAT ANSWERED IT WAS THE ORDER AND NOT THE TOKEN.** The objection above is
about `"down"` being a literal, and `0#down` is not one — so §3 gained a row and
lost nothing. What actually blocked the file from saying it was that the writer
took the PARSE TREE, and whether `"down"` is an option depends on the receiver's
declared TYPE, which is resolve's answer. M19.6 moved the write; the token is
what the move made writable.

**WHAT THE FILE STILL DOES NOT SAY IS THE NUMBER.** `0#down` names the option,
so `1 4 2 5` is not in the file and the resolver still looks the row up under
the receiver's type — **one lookup, where a cold walk takes a decision**:
`takes_options()` against words.def's sixth list, the sibling scan that collects
`down, up`, and M16's bare-word retry are all skipped. MILESTONES/M19.6.md §3 is
careful about that difference, because "the fold is written down" and "the fold
costs nothing" are two claims and only the first one is true.

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
