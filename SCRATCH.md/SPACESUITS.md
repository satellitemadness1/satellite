# Spacesuits (classes): where they stand, 2026-09-27

The author asked what state classes are in, and whether anything is still needed for complete
class support. This page is the answer, tested on **satl 004 revision 08 build 0124**, the
installed binary (`~/.satl/satl`, byte-identical to `build/satl`).

**How it was tested.** `tests/spacesuits.satl` was run first. It passes, with all 17 lines as
check.sh expects. The ten spacesuit pages of ERRORS4 were re-run next, and all ten are still
there. Then five testers each took one area and between them wrote and ran **685 new programs**:
making objects, capsules on objects, inheritance, nesting and files, and objects meeting the rest
of the language. One fresh reader re-ran every one of the 67 problems they reported, and checked
each against the help pages, MILESTONES, POLYMORPH and every error list. That left **28
confirmed**, 28 duplicates, 6 already known, 4 by design and 1 not reproduced. I re-ran the seven
that matter most myself, and each output below is from those runs.

Every program, its output and its verdict: `SPACESUITS/result.json`. The testers' programs
(814 of them, no bytecode): `~/Documents/satl/spacesuit-probes-2026-09-27/`.

---

## 2026-10-02: `object.pointer()` and `object.reference()` are BUILT

**The author**, asked whether there was any point to having both: *"we could make .pointer() be
something that points at the object, and .reference() something that is a copy of the object, so
the object can be emptied and the reference is still there, so the reference never goes empty,
it's an exact copy, and the pointer() is something that just points at the object, doesn't keep
the object living, if there's nothing else, will you code this now?"* It is 2026-09-12's ruling
for 003 (old_versions/second_satellite/MILESTONES/M26.md §2.1, "it answers a weak reference"),
built at last, and the new half beside it.

    box p = a.pointer()       points at a's object, and does not keep it living
    box r = a.reference()     an exact copy of a's object, its own from then on
    p.ok()                    false once nothing else holds the object; true of an object itself

**What runs** (`tests/pointer.satl`, `reference_tree.satl`, `pointer_own_capsule.satl`,
`pointer_in_a_list.satl` and four refusals, 11 rows in check.sh, and an example on `satellite.help(spacesuit)`):
- a pointer is its object's spacesuit -- `box p` -- and every capsule of a box is called through
  it, on the object, which is held for the call; a list, a field or a capsule's parameter of a
  box takes one, and so does a thread's capsule;
- once nothing else holds the object -- no name, no field, no item of a list -- it is gone, and
  every pointer at it is empty: `a = other`, or the end of the capsule an object was made in;
- a capsule called through an empty pointer stops with **S260 OBJECT_IS_GONE** (machine code 76);
- a reference copies the object and every object it holds, through fields, lists and maps, once
  each -- an object held twice is one copy held twice, and an object that holds itself holds its
  copy. A pointer inside it points at the copy of its object when the copy made one, so a copied
  tree's children point at the copied parent. Emptying or changing the original never reaches it.

**The leak this ends, measured** on build 0142, 200,000 objects each linked to itself: a strong
link (`n.call_set(n)`, the author's `local_this` shape) peaks at **95.8 MB** -- not one is ever
freed; a link made with `.pointer()` at **18.2 MB**; no link at all, 17.8 MB. So a back-link made
with `.pointer()` is the way out of the cycle in "Missing" below, ahead of a cycle collector.

**What it costs:** nothing a clock separates. Best of five, alternating, outputs byte-identical,
0140 against 0142: 1,000,000 turns of two capsule calls 3.51 s against 3.51 s; 1,000,000 objects
handed to a capsule 3.13 against 3.13; a plain loop 2.80 against 2.82. Counted by callgrind, a turn
of two capsule calls was 24,642.7 instructions on 0140 and 24,733.6 on 0142 (+0.37%); build 0143
tests the new words only when they can be the one, and is 24,726.8 (+0.34%) -- the rest is in
one_operand's and call_method's layout, because a plain loop that runs none of the new code moved
the same way, 10,125.1 to 10,148.1 a turn (+0.23%). The compiler lays the binary out again; the
new words do no work on a call that is not theirs.

**Files:** `satellite/satellite_object/satellite_pointer.hpp` (the value, arm 20 of
satelliteObject: a std::weak_ptr and the spacesuit's layout, 32 bytes inside the 136 a value
already is), `satellite_object/object_copy.{hpp,cpp}` (the copy: a list of work, never the copy
calling itself, so a chain of a million objects copies like any other),
`bytecode/pointer_calls.{hpp,cpp}` (the three methods), `capsule_calls.cpp` (a pointer's capsules
run on its object), `type_shape.cpp` (a box name takes a box pointer), `program_check.cpp`, and
REGISTRY rows 0x0B6E and 0x0B6F.

**My choices, his to overrule** (each is one place):
1. **An empty pointer stops the program when it is used** (S260), and `p.ok()` asks first. The
   other choice is a pointer that quietly does nothing -- the "empty value" question below.
2. **Every object answers `ok()`**, true for an object itself: a name declared `box` may hold an
   object or a pointer, and nothing before the run can say which.
3. **The language's words come first**, as `.lock()`'s do: a spacesuit's own capsule named
   `pointer`, `reference` or `ok` is still called by its bare name inside the spacesuit, and
   `x.pointer()` from anywhere is the language's. Four of his programs (the_death_korps, terran,
   force_energy, infinity_data) declare a `pointer()` answering their strong `local_this`, so
   their `x.pointer()` lines now get a real pointer. None of the four runs on 004 either way:
   `#000000` colours and `satellite.system.memory.main` are 003's, and stop build 0140 the same.
4. **A reference copies all the way down**, not one level: "an exact copy", and one level would
   leave the copy sharing what the original goes on changing.
5. **What a reference does not copy, as `=` does not:** a list or a map that holds no object (it
   is a value already, shared until one side writes), a file, a window, a thread, a program.
6. **The copy's `.lock()` is on or off as the original's**, and each original is read under its
   lock when that is on, as a statement reading it would be.
7. **They are an object's only:** on a list, a number or any other kind, `.pointer()` and
   `.reference()` are refused as "an object's" (S110), before the run where the checker can see
   it -- a list is copied by `b = a` already.
8. A pointer's `.pointer()` is the pointer itself; `display(p)` refuses as `display(a)` does.

**Not done, and his:** `.ok()` on a field never given an object still refuses ("has no value
yet") -- answering false there would close the "test whether an object field is empty" row
below; `this`; `==` on two objects, so on two pointers too (S301 still).

---

## The short answer

**Classes work.** A normal object-oriented program can be written today:
- objects, constructors and fields;
- protected and public sections;
- one spacesuit extending another, with overriding and dynamic dispatch;
- spacesuits inside spacesuits, spacesuits from other files and inside namespaces;
- objects in lists, maps and multiples, and objects on threads.

Most mistakes are refused before anything runs, with the right sentence.

**What is not complete** falls in three groups:
1. **One dangerous bug.** A constructor that makes an object of its own spacesuit is not
   refused. It takes memory at about 2 GB a second until the machine runs out.
2. **Eleven things class systems usually have that satellite cannot write yet.** Most important
   are a way to test whether an object field is empty, `==` on two objects, a way for an object
   to hand itself on (`this`), and calling the supertype's version of a replaced capsule. Each
   of these is the author's decision.
3. **Refusals that come late or say the wrong thing.** The main one: a value of the wrong type
   given to an object name or a parameter is caught only when its line runs, after earlier lines
   have printed.

Also still the author's, from before: a built type as a supertype (M35), a spacesuit declared
inside a capsule (POLYMORPH M1), and making an object inside a line, `Type(args)`.

---

## What works (confirmed by running it)

- **Making objects:**
  - no constructor, a zero-argument one, or one taking each of the nine built types, a list, a
    map or another object;
  - field initialisers of every type;
  - each object gets its own list and map fields;
  - each loop iteration makes a fresh object;
  - an object made in a capsule and handed back outlives it;
  - declaring a name again makes a new object, and the old one lives on;
  - 200,000 objects made in a loop peak at 21 MB, freed as they go.
- **References:**
  - `b = a`, parameters and list items share the object;
  - containers are copied, and the objects inside them are shared;
  - a list compares objects by identity, in `.contains`, `.index_of`, `.remove` and `list == list`.
- **Capsules on objects:**
  - protected and public sections enforced before anything runs (S230, in eight positions);
  - bare calls inside;
  - answers of every type, including objects;
  - chaining 60 deep;
  - calls inside conditions, for headers, joins, list literals and map values;
  - left-to-right order;
  - tail self-calls in constant memory, and mutual recursion;
  - `break` and `continue` inside a method.
  - More than 100 built-in method words (size, add, sort, join, ...) work as capsule names; the
    two known exceptions are ERRORS4 004 and 005.
- **Inheritance:**
  - chains four deep, with every constructor running supertype first and arguments reaching the
    nearest constructor;
  - dynamic dispatch through a supertype-typed name, parameter, list, map, field and thread;
  - the template-method pattern: a supertype's bare call runs the override, even during
    construction;
  - shadowed fields as two slots, as documented;
  - override rules checked;
  - a subtype-only capsule through a supertype name refused before running;
  - a downcast checked when it runs;
  - extending across files, into nested spacesuits and into namespaces.
- **Nesting and files:**
  - spacesuits nested three deep;
  - `file.namespace.outer.inner.inmost` as a type, a supertype, a parameter or a list type;
  - same-named spacesuits in two files told apart;
  - spacesuits declared below their use.
- **Data structures:**
  - a 200,000-node linked list;
  - a doubly-linked list;
  - a binary search tree with recursive insert and walk;
  - an n-ary tree;
  - a graph with a breadth-first walk;
  - a 50,000-deep composite;
  - two spacesuits whose fields name each other.
- **With the rest of the language:**
  - objects in `list<T>`, `map<string, T>`, `multiple`, and in nested containers;
  - `satellite.access` on objects and on containers of them;
  - threads on objects;
  - `obj.lock()` giving exact counts with two threads;
  - an object holding a file.
- `satellite.class` works everywhere `satellite.spacesuit` does.

---

## Broken: confirmed, not in any list before today

### 1. A constructor that makes its own spacesuit runs away with memory (HIGH)

```satellite
satellite.include(satellite)

satellite.spacesuit node()
{
    satellite.constructor()
    {
        node made
    }
}

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    node n
    satellite.return(satellite)
}
```

It prints `before`, then nothing. satl's memory, sampled every half second:
0.98, 2.0, 3.1, 4.1, 5.2 and 6.2 GB at 3 s, when I killed it. It would fill this machine's
61 GB in about half a minute. The same happens through a subtype (a supertype's constructor
making a `sub`) and through two spacesuits whose constructors make each other.

The checker already refuses the **field** form of this ring (`node next()`, check.sh row `ring`),
and that refusal's own advice leads here: *"declare next with nothing after its name, and give it
an object in satellite.constructor"*. `rings_of_making` in `suit_reach.cpp` follows fields made
with brackets and supertypes, but not constructor bodies.

**What the fix can refuse.** A constructor that makes its own spacesuit **only inside an if**
is a real, finite recursion, for example building a tree n levels deep. So the certain ring is
the unconditional one: a declaration at the constructor's top level. Growing memory while it
runs follows the author's rule that recursion has no limit. The missing thing is the check,
never a depth bound.

### 2. A wrong type given to an object name or a parameter is caught only when it runs (MEDIUM)

```satellite
satellite.spacesuit one()
{
}

satellite.spacesuit two()
{
}

satellite.capsule satellite.main()
{
    satellite.console.display("before")
    one a
    two b = a
    satellite.return(satellite)
}
```

It prints `before`, then `S301 (run): b was declared two, and it holds an object of the
spacesuit one`, exit 27. Both types are on the lines above, and the checker already resolves
spacesuit types when it judges members. The same happens for:
- a wrong spacesuit handed to a parameter;
- a literal given to an object name;
- an object put into a built-type name;
- **a literal handed to any capsule's parameter, constructors included**: `num a("hello")`
  where the constructor takes a number.

The last case is the half of ERRORS2 #8 that ERRORS3 marks FIXED. 0da03d9's own message says
it judged only a declaration's or an assignment's lone literal, so parameters were never covered.

### 3. An override that never answers is caught only when it runs (MEDIUM)

`base.doubled()` answers `val() * 2`. `sub(base)` replaces `val()` with one that hands nothing
back. Calling `s.doubled()` prints `before`, then S240 at run. `s.val()` called directly is
refused before anything runs. The override check compares access and parameters, but not
whether the replacement answers where the capsule it replaces is used for its answer.

### 4. Two recursive shapes are not tail calls (MEDIUM; the author decides)

- **`satellite.return(count_to(i + 1, n))`** on the last line is never collapsed.
  100,000 deep took 904 MB (my run). Only the flat form, a self-call statement then
  `satellite.return()`, runs in constant memory. So a recursive capsule that **answers a value**
  can never be constant-memory. The ruling says *"only as the last line ... and do the tail call
  thing"*, and this is the last line.
- **`next[1].sum_rest(acc + value)`** as a method's last line, the same capsule on **another
  object**, is not treated as a call to itself. It costs about 10 KB a node: 452 MB for a
  40,000-node list. `program_walk.cpp` says *"Only a call to ITSELF"*, so this may be intended.
  It decides whether a recursive walk down a linked list is constant-memory. A while loop is.

### 5. Wording and a few refusals that say something untrue (LOW)

- `satellite.access(b)` on a name declared `base` that holds a subtype lists the subtype's extra
  capsules. Writing the listed line is then refused (C-02).
- A capsule written outside every section is said to be *"inside the satellite.protected
  part"*. The file has no such part, and the help does not say loose members are protected
  (B-12).
- S230 for a supertype's protected capsule says *"only base's own capsules can reach it"*.
  A subtype's capsules reach it bare (C-12).
- An object declared in an if that did not run, then used after the block, is refused as
  *"no capsule named get"*, though the spacesuit has `get`; the object was never made (A-07).
- An inner spacesuit reading the outer's field is told the field has *"no satellite.variable
  line declaring it"*. It is declared, in the outer. Refusing it is right (D-09).
- When an inner spacesuit shares a top-level one's name, S301 gives two different types one
  name (D-10).
- A protected inner spacesuit named from outside is filed as S201. Its paragraph says no line
  declared it, beside a sentence saying where it is declared (D-11).
- A bare spacesuit name found in two included files gets a hint naming only one of them (D-12).
- `satellite.namespace` inside a spacesuit is described as a field with an unbuilt type (D-14).
- Outside classes:
  - a map declared `= {}` is refused only when it runs (C-13);
  - `satellite.system.memory.this.used`, numbered and not built, is refused when it runs with
    *"there is no value here to work with"*, not S210 (E-11).

---

## Still present from ERRORS4 (all ten re-run on 0124)

| page | what happens |
|---|---|
| 004 | capsules `color()` and `colour()` in one spacesuit: `s.color()` answers **"blue"**, colour's. Also `o.contain()` runs `has()` when only `contains` is declared |
| 005 | a capsule named `lock()` never runs: the built-in lock takes its place, silently |
| 042 | a capsule called on an empty field, or on a method's number answer, gets the false *"math sign spacing"* reason |
| 043 | display, join and `.sort().by_name()` of objects whose spacesuit declares `to_string()`: *"an object has no to_string capsule"* |
| 044 | a member reached through a map item is judged only at run |
| 049 | `x.lock()`'s S210 hint says it is a file's, though objects have it too |
| 071 | `display(obj)` is refused only when its line runs |
| 072 | `counter.bump()` on the spacesuit's own name: *"no satellite.variable line declaring it"* |
| 073 | `things[1].v = 5` gets an unrelated S110 |
| 074 | `other.cents` inside money's own capsule gets a sentence that does not fit |

---

## Missing: most class systems have these, and satellite has no word for them (the author decides)

Ordered by how much each one stops a normal program being written. Each row was checked against
`words/words.tsv` and `satellite.help/`: no word exists for it today.

| what | today | workaround that runs |
|---|---|---|
| **test whether an object field is empty**, or set one back to empty | `node n = a.get_next()` on an empty field: S301 *"holds nothing"*, at run; every spelling of a test is refused | a bool beside the link, or a `list<node>` field holding 0 or 1 items |
| **`a == b` on two objects** | S301 *"no scenario for that pair"*, at run, **even `a == a`**, though `{a} == {a}` and `.contains` already compare objects by identity | a list of one, or an id field |
| **an object handing itself on** (`this` / `self`) | the help says *"there is no this"*, which settles naming inside; nothing lets an object register itself, return itself for chaining, or set a back-link | the caller sets both halves of the link, which is exactly the cycle that leaks (below) |
| **calling the supertype's version of a replaced capsule** (`super.speak()`) | `animal.speak()` inside `dog.speak()`: S201 | the supertype keeps the body in a second, differently named protected capsule |
| **a type test before a downcast** (`is it an eclipse?`) | a wrong downcast stops the program; `variant.holds` is numbered for variants only | an overridden `kind()` capsule |
| **sorting objects** by a key or by a capsule's answer | `.sort().by_value()`, `.max` and `.min` refuse objects, honestly | a hand-written sort |
| class-level (static) fields or capsules | no word, and no file-level variables | one shared object handed to every constructor |
| abstract capsules or interfaces | a placeholder whose answer is used must answer | `satellite.return(0)` as the placeholder |
| copying an object | **BUILT 2026-10-02: `object.reference()`**, an exact copy (top of this page) | -- |
| a destructor | no word | none |
| a supertype's nested spacesuits, by bare name in a subtype | a protected one cannot be named from a subtype at all | a protected factory capsule |

**The reference-cycle leak is by design** (old_versions/second_satellite/MILESTONES/M26.md §2.1:
refcount now, `.pointer()` as the weak reference, a cycle collector later). 200,000 self-linked
objects peaked at 71 MB, where unlinked ones stay flat at 21 MB. It matters more because of the
first three rows: with no empty value and no `this`, a back-link can only be broken by pointing
the field at a spare object. The 004 help page does not mention the leak. **SINCE 2026-10-02 a
back-link made with `.pointer()` does not keep its object living** -- 95.8 MB against 18.2 MB for
200,000 self-linked objects (top of this page).

**Also by design, and worth one line in the help:**
- one constructor per spacesuit;
- a constructor is public wherever it is written, even inside `satellite.protected`;
- a supertype whose constructor takes arguments cannot be handed them by a subtype with its own
  constructor (M35's `satellite.supertype.<name>` is the planned answer).

The help's list of refusals names none of the three.

## Still the author's, from before

- **M35:** a built type as a supertype, `satellite.spacesuit a(satellite.variable.string)`,
  and `satellite.supertype.<name>`.
- **POLYMORPH M1 D1/D2:** a spacesuit declared inside a capsule.
- **`Type(args)` inside a line:** today an object is made only by a declaration line.
- ~~**`.pointer()`** as a weak reference is owed (M26).~~ **BUILT 2026-10-02**, with `.reference()` (top of this page).
- **New questions from today:**
  - does `satellite.return(self(...))` count as the tail call?
  - does the same capsule on another object?
  - should `==` on two objects mean "the same object"?
  - does satellite want an empty value for object fields, and a `this` for handing an object on?

## Not reproduced

A tester measured a method call as 0.7 µs slower than a top-level capsule call, at load 13.8.
On an idle machine the fresh reader measured it about 0.2 µs **faster**. A field initialiser costs
about 0.45 µs each time an object is made: about one statement, not a defect.
