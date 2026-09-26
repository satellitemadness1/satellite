*satellite design docs, §19, part 1 of 2. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§18](18-satellite-random.md), On: [§19 part 2](19-b-input-arguments-and-booleans.md).*

---

## 19. Nine additions, and the program that asked for them

Status: **built**, 2026-08-24.

Every section above this one was written from the inside out — what the language
ought to be, then what to build. This one was written from the outside in. A real
program was pointed at `satl` and the interpreter's refusals were collected in
order, and the gap list that came back (`plans/missing.txt`, 2026-08-23) is the
only design input this section has.

The program is `view_forge`: 40 spaceships, 13,273 lines, a console application
that loads a library of encyclopedia objects out of a declaration tree and forges
one of them onto the screen. It is worth being precise about why it is good
evidence. It was not written to exercise satellite, and nobody consulted this
document while writing it — so where it disagrees with the language, it is
reporting what a satellite program looks like when someone just writes one. It
also asks for **nothing new about what satellite is**: §15's bootstrapping
stages and §11's milestones are untouched by everything below. Every entry is
surface syntax. That is the finding, and it is a better one than a longer list
would have been: the object model, the container model, the call convention and
the scope model all survived contact with a program of this size unchanged.

METHOD, and it is the same one `plans/missing.txt` used: every claim below was
checked by running the interpreter. Where a count appears it came from `grep`
over the tree on 2026-08-24.

The program now runs end to end — **verified**: 0 errors, 582,810 lines of
output, 501 objects loaded and forged, 43 types. Before these nine changes it
died on line 3.

### 19.1 A quoted `satellite.include` path

72 of the program's 112 include lines, across 38 of its 40 files.

```
satellite.include("object/forge_object.satl")
satellite.include("../helper.satl")
```

§16 settled include *semantics* — include-once by canonical path, cycles
terminating, bodies before the includer — and none of that changed. What was
missing was only a way to **name a spaceship that is not a sibling file**. The
bare form does not rescue it: the loader looks in the includer's own directory
and nowhere below it, and `view_forge` is a five-level tree in which every
include crosses at least one directory boundary and some climb three.

**§1 is completed by this, not weakened.** The quotes are the whole of the
distinction. A bare word is still a name the user owns and still goes through
`search_paths()`; a satellite-rooted path is still ours; and **a string literal
was never a name at all**, so it can carry a location without any name ever
having to mean a place. C draws exactly this line with `<>` against `""`. The
rejected alternative was a dotted subdirectory spelling
(`satellite.include(object.forge_object)`), which fits the naming rule more
snugly but has no answer for `..` — and upward traversal is not an edge case
here, it is how the declaration files reach the shared object definitions.

Three decisions inside it:

- **Resolved relative to the including spaceship, never to the working
  directory.** This is what the form turns on. A project is a tree of files that
  include each other by their positions in that tree, and those positions do not
  move when someone runs the program from one directory up. Resolving against the
  cwd would make `satl view_forge/main.satl` and `cd view_forge && satl main.satl`
  two different programs, and the failure would be a not-found error naming a
  file that is plainly there.
- **`..` needs no code.** The kernel walks it and `canonical()` collapses it for
  the include-once key, so a file reached as `../helper.satl` from one spaceship
  and as `object/helper.satl` from another is **one** spaceship. **Verified** —
  were they two, every capsule in it would be reported as already defined.
- **The extension may be omitted**, so `"object/helper"` and
  `"object/helper.satl"` are one include rather than two spellings of it.

The parser needed no change at all: it already parsed a string literal in that
position, and only `classify()` in `loader.cpp` refused it.

### 19.2 The list literal

2,774 sites, 2,767 of them as a call argument. This single missing spelling
accounted for more failing lines than every other entry combined.

```
satellite.container.list<satellite.variable.string> location = {"BERLIN", "GERMANY"}
local_forge_object.call_set_links({86, 87, 123, 134})
```

§8.6's containers could be **built** before this — declare empty, then
`.append()` — but not **written**, which made a list of four strings five
statements. This was a genuine hole rather than a refusal: lists already existed,
already nested, and were already passed around as values; there was simply no way
to write one down. It is also the only way the declaration tree could reasonably
be written, since the alternative is a named temporary and four `.append()` calls
before every setter.

`ListLit` is appended to `ExprBase` for the reason `DurationLit` was (§10's note
on variant indices), and a `vector` is 24 bytes so `sizeof(Expr)` does not move.

**The element type is deliberately not part of the literal.** The type belongs to
the variable or parameter the literal is handed to, not to the literal, which is
the rule every other expression already follows. Empty is spelled `{}` and is the
one literal whose element type cannot be inferred from its contents — which is
the argument, not an exception to it.

That is safe rather than lax, and it needed no new code: §8's "check at insertion
and literal construction" already runs, because a declaration checks its
initialiser against the declared type and `matches` recurses into the element
type. `satellite.container.list<satellite.variable.number> l = {1, "no"}` is
refused at the declaration — **verified**. §8 wrote that rule before there was a
literal to apply it to; this is the literal, and the rule was waiting. Elements evaluate **left to right**, fixed
rather than left to the loop, because an element can be a call that displays
something and the order is observable.

### 19.3 A redeclaration rebinds — and the fresh slot is the whole argument

724 declarations of two names, **687 of them redeclarations in one scope**.

```
forge_object local_forge_object
local_forge_object.call_set_object_id(124)
object_list.append(local_forge_object)

forge_object local_forge_object          // and again, 74 more times
```

The program uses a redeclaration the way C++ uses a fresh block scope: build one
object, put it in the list, forget the name, build the next. §6 refused it.
Refusing to shadow a live name in one scope is defensible in the abstract and
indefensible at 687 sites, and of the three ways out — rebind, add a bare nested
block, or make the program number its temporaries — rebinding is the one the
program already assumes.

**THE FRESH SLOT IS THE CORRECTNESS ARGUMENT, and reusing the old one would have
been the natural implementation and silently wrong.** §14 makes a spacesuit a
**reference type**, so `.append()` stores a handle rather than a copy. Rebind to
a new slot and the previous instance is untouched — it is still whatever the list
is holding — so *n* declarations through one name leave *n* **distinct** objects.
Write the new instance over the old slot and every handle in the list points at
one piece of storage, and a library of 501 objects reads back as 501 copies of
the last one. **With no error anywhere.** That is the failure mode this paragraph
exists to have prevented: it is not a crash, it is a program that runs to
completion and prints the wrong encyclopedia.

**Verified** two ways: `env_test` pins `slot_count == 2` for two declarations of
one name and pins that the read after both sees the *newer* binding; and the real
program's 501 forged objects carry 498 distinct names.

This also costs nothing structurally, because §6 already promised it —
"slots are never reused across scopes" — so a fresh slot per declaration is the
grain of the existing design rather than a new rule. The declared **type** of the
second binding may differ from the first, and that follows rather than being a
separate decision: the name now denotes a different slot, and a slot's type is
its own.

### 19.4 Assigning to a list element

5 sites. `location_str[i] = location_input[i]`.

Reading through an index already worked, including with a variable index and a
negative one; only the store side was absent, because the assignment path wants a
storage slot and an index expression names none.

A list is **built, then frozen** (§8's value model), so this is a
copy-and-replace and not an in-place poke: the transform builds a new `List` with
one element changed and `update_through_slot` publishes it, under whatever lock
that slot needs. Every other list mutation already works this way — `.append()`
is the same three lines — so this adds a **spelling, not a mechanism**, and
inherits the §14 write protocol instead of inventing a second one.

The index rules are deliberately identical to the read side (§7): a negative
index counts from the end, and **out of range is an error and never a grow**. An
index assignment that extended the list would make `l[5] = x` on an empty list a
way to create four nil elements nobody asked for. A **map** still refuses, and
still says `.set(key, value)` — §8.6 gave it that, and a subscript store would be
a second way to spell one thing.

What it costs: O(n) per assignment, which is the honest price of the
immutability contract. Filling n slots by index is O(n²); `.append()` in a loop
is O(n) amortised and remains the way to build a list.
