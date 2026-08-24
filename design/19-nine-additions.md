*satellite design docs, §19 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§18](18-satellite-random.md).*

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

### 19.5 `satellite.console.input`, and the only out parameter in the language

1 site, and it is the program's entire reason for existing: it prompts for the
name of a target, searches its library for an exact match, and forges what it
finds. `satellite.console` had `display` and nothing else, so with no input the
program could only ever forge something hardcoded.

Three shapes, and the first two are the ones to prefer:

```
satellite.console.input()                       the line
satellite.console.input(prompt)                 the line, after prompting
satellite.console.input(prompt, target)         writes into target
```

**The two-argument form is the only out parameter in the language and is
deliberately not the beginning of a general facility**: no user capsule can
declare one, because nothing in a capsule's parameter list can say "this one is
written back". It exists because it is the shape a program reaches for, and
because the value form alone would make `satellite.console.input("", answer)` a
mystery rather than a mistake. It is written in terms of the value form, so one
place prompts, one place drains and one place reads. The **place** is checked
before the prompt is printed — a target that cannot be written to is a mistake in
the program, and discovering it after the user has typed an answer would throw
that answer away.

**The drain is the part that is not obvious.** Output goes through §9's printer
thread, so a prompt is *queued* rather than printed, and a read that did not wait
for it would block on an apparently empty terminal with the prompt sitting behind
it. `Console::drain()` — which §9 already provides so that `interp.cpp` can print
an error report after a program's output without the two interleaving — is
exactly the barrier needed, and this is its second caller.

End of input is **loud**. An empty line and no line at all are different answers
— the first is somebody pressing return, the second is nobody being there — and
a program that cannot tell them apart loops forever on a closed stdin.

### 19.6 Named arguments as grammar, and `end=` as the only one understood

```
satellite.console.display("[SATELLITE_VIEW_FORGE]>>", end="")
```

Two absences were tangled here, and only one of them blocked the program: there
is no keyword-argument syntax anywhere in the language, and there was no
unnewlined print. A prompt whose cursor lands on the line below it is ugly and
survivable; a prompt that cannot be written at all is not.

`NamedArg` is **grammar for one named argument and not the start of keyword
arguments.** It parses anywhere and is accepted in exactly one place — the same
shape `DurationLit` already has — and the reason to give it a node at all is the
error: an unrecognised `name=` now reaches one message that states the whole rule,
instead of a parse error complaining about a missing `)`. A bare word followed by
a single `=` cannot be anything else in an argument list, because assignment is a
statement in this language and never an expression, so this steals no form; `==`
is one token, so a comparison is not caught by it.

**The ending is a value, not a flag.** `end=""` is a prompt, `end=" "` puts two
displays on one line, and `end="\n"` is the default spelled out. A boolean
`newline=false` could not express the middle one. It emits **one** piece, for the
reason §9 gives about `display`: the unit handed to the Console is the unit
another thread cannot tear in half, and a prompt should not arrive split around
some other thread's output.

### 19.7 Bare `TRUE` and `FALSE`, without a reserved word

35 sites. `satellite.variable.bool target_list_has_unset = TRUE`.

§8.4 closed with an objection to exactly this: "the obvious alternative — bare
`true` and `false` — would be the language's second and third reserved words, and
§1 has exactly one." **That objection was right, and it is about reserved words
rather than about the spelling.** `satellite.bool.true` and
`satellite.bool.false` remain canonical and remain the only spelling this
document recommends.

The resolution is **where** the two words are recognised. They are answered in
`Resolver::resolve_name` only after a local, a field, a method, a capsule and a
spacesuit have all been asked and said no, and then at run time only after
`satellite.library` has been asked too. So §1 holds exactly as written: a bare
identifier still names something the user owns, and a variable, field, capsule or
spacesuit called `TRUE` still wins the word. **Verified** — a capsule declaring
`satellite.variable.number TRUE = 5` returns 5. Nothing is reserved, and the
words are not taken away from anybody.

Doing this in the **lexer** is the obvious implementation and is the one that
would have broken §1, by taking both words from the user everywhere and for good.

### 19.8 Two spellings accepted, and one silent bug closed

**`satellite.statement.else()`** — 24 sites. An `else` takes no condition, so an
empty pair of parentheses carries nothing and the parser was right to be
surprised. But the program writes parens on every other statement form it uses
and reached for them here by symmetry, which is a mistake a reader makes once per
program rather than once per career. An empty pair is now accepted and dropped;
`else(x)` is still an error, because a condition on an `else` is a
misunderstanding rather than a spelling.

**`\'` inside a double-quoted string** — this one was not a blocker at all, and
that is what makes it the most interesting entry here. §3.4 gave the string lexer
`\n \t \r \\ \"` and left every other backslash to pass through untouched, so
`"DOESN\'T"` printed with the backslash still in it. No error, no refusal, and
therefore invisible: once the nine blockers above were fixed and the program
actually ran, **501 lines of its output were wrong**. An apostrophe needs no
escaping inside double quotes and there are no single-quoted strings for it to be
escaping from, so the program was being over-careful rather than wrong, and was
punished for it.

`\'` is now a redundant spelling of `'`. Rejecting unknown escapes outright was
the alternative and was rejected: `encode()` cannot tell a mistake from a `\d` or
`\s` that somebody put in a string on purpose, so making unknown escapes an error
would break working programs to catch this one. **Verified**: 501 stray
backslashes before, 0 after.

### 19.9 What did not change, and what is still open

Recorded because a nine-item list invites the conclusion that the language was
half-built, and the opposite is what the evidence says. Every one of these was
doubted, probed and found already working: spacesuits with `satellite.protected`
and `satellite.public` blocks, fields, methods and constructors; a bare instance
declaration running the no-argument constructor; a protected capsule calling a
sibling by bare name; a spacesuit name as a parameter type;
`satellite.container.list<user_spacesuit>`; a free capsule handing back a list of
spacesuits **with no `satellite.returns` clause** (all 25 omit it and satl infers
it); `object_list[i].call_get_name_str()`; a list element passed straight into a
call; `string + number` with the number coerced; `satellite.main` with no
explicit `satellite.return`. The object model is the part of this program that
satellite already ran.

Still open:

1. **`string + bool` is refused.** `"flag=" + b` gives
   `+ does not apply to flag= and true`, while `string + number` coerces and
   works. 0 sites today — the program writes the words TRUE and FALSE into its
   literals by hand — but its whole style is to narrate the value of every
   variable it touches, and the moment somebody narrates a bool the way they
   narrate a number, this fails. Whether the asymmetry is intended is worth
   settling deliberately rather than at the first site.
2. **Keyword arguments in general** are not decided, and §19.6 deliberately does
   not decide them. `end` is one name understood in one place.
3. **A bare nested block `{ }`** that opens a scope was one of §19.3's three
   options and is still not in the language. It is the conventional answer and
   would make the rebind unnecessary for programs that wanted the C++ shape
   explicitly. Nothing now depends on it.
