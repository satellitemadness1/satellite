# CONTAINERS — any combination, and `satellite.access(x)` showing how to reach into it

The author, 2026-09-25, late:

> *"for any combination of containers, lists, maps, multiples, for any combination of them -- we
> give the user a command -- accessing that particular complex object, so when we type
> satellite.access(complex_list_of_maps_object) it gives the correct syntax to access that
> object, no matter what that object is, I don't even know if satellite supports every single
> container combination yet, that is step 1 of this plan, first we have to accept any sort of
> crazy combination"*

He also settled the approach: *"incorrect ways could be valid ways and we just assumed something
in the wrong way, so we really can't provide any of this, we have to go by the user's
complicated container declarations though, we can assume the declaration of the container is
correct"*. So satl doesn't guess what a wrong line meant. It works from what the
**declaration** says, and shows the right way to reach in.

---

## WHERE IT STANDS, 2026-09-26: steps 1 and 2 are built

**Step 1.** `satellite.container.map` declares, at any depth -- his
`experiments/crazy_container_object.satl` declaration now runs. `utility/check_container_shapes.py`
writes every shape to a depth (list, map, index, multiple over number/string/float/bool): each
declared with a literal, shown back exactly, read and written at its deepest item, filled from
empty, and a wrong kind refused in the literal and in the deepest write. **Depth 4: 5,412
programs, 0 wrong** (check.sh runs depth 2). What the sweep and the spacesuit probes found, and
what was built for it:

- `satellite.container.map` was refused as a type. It is now the same container as index
  (`is_an_index_word`, type_shape.hpp); a refusal says the word written. `satellite.container.map()`
  is an empty one.
- **A map had no literal**, so a list of maps took a line per key. `{"zoe": 30, "al": 4}` now
  makes one -- the form satl always printed a map in; a key twice keeps its first place and its
  last value (Python's rule).
- **A write through a `multiple` was never checked below it**: `list<multiple<string,
  list<number>>> x = {{1}}` then `x[1][1] = satellite.bool.true` went in. 108 such at depth 3.
  The walk now goes on through the type the value is held as (`arm_holding`); `.append` too.
  **Still open:** when TWO of a multiple's types are of one kind -- `multiple<list<number>,
  list<string>>` -- neither is chosen, and below it a write is unchecked, as before. Choosing
  refused legal writes (an empty list fits both), and re-checking the whole value after each
  write made 20,000 appends take 7.8 s (the review). Doing it right needs a write that can be
  taken back when the result fits no type.
- **A `multiple<string, shelf>` holding an object could not call its capsules** (refused before the
  run). It can now, when one spacesuit is among its types; its container methods stay a
  container's (`multiple<list, shelf> e` still appends).
- Spacesuits in lists, maps of lists and lists of lists, and maps/lists as fields, all worked.

His `.append({{"hello"}})` on that declaration is refused, rightly by his rule (the declaration
is correct): an item of `crazy_container` is a list of MAPS, and `{"hello"}` is not a map.
`satellite.access(crazy_container)` shows `crazy_container.append({{"key": {{"text"}}}})`.

**Step 2.** `satellite.access(name)` is word `1 31 1` (`satellite.access` is `1 31`), in
`satellite/bytecode/access_calls.cpp`. The sweep also runs, as code, **every line access
prints** for every shape -- 1,360 programs at depth 4, all run. So what it shows is the
correct syntax, checked, not only described.

**My choices, his to overrule** (the three open questions below were his; these are answers
I picked so it could be built, not rulings):
- The layout: a header (`rows is a list of maps (string -> list of numbers), 2 items`), the
  value, one reach line per level, one fill line per level with a literal of the right shape,
  then `.size` and `.keys`. Positions are n, m, p, q...; keys "key", "key2"...
- **It does both**: a line that is nothing but `satellite.access(x)` prints; anywhere else it
  answers the same text as a string.
- **It works at the prompt**, on anything the session declared.
- A spacesuit's object shows its fields with their values, marked "reached only from inside
  its own capsules", and its public capsules as the lines.
- The `access` switch is not asked: it governs the last-known STORE (a name's last value after
  the program stops), and that store is still not built.
- `{}` given to a map name is still an empty LIST, and refused; `satellite.container.map()` or a
  bare declaration makes an empty map. Python's `{}` is a dict; say if you want that.

**Found and not fixed (not this work):** a `{` never closed on its line swallows the next
statement through the multi-line join, and the refusal names that statement's word
("satellite.return has no library built for it yet") -- a list does the same. And the run-time
type refusal is still one line, not a report (ERRORS2 #7).

**Step 3 is not started.**

---

## Step 1 — accept any combination (probed tonight, BUILD 0086)

| declaration | today |
|---|---|
| `list<list<number>>`, filled `{{1, 2}, {3}}`, read `g[1][2]` | works (2) |
| `list<index<string, number>>`, `rows[1]["a"]` | works |
| `index<string, list<number>>`, `m["xs"][3]` | works |
| `index<string, index<string, number>>`, `mm["outer"]["k"]` | works |
| `list<list<index<string, number>>>` | declares (size 0) |
| `list<list<list<string>>>` `{{{"a"}}}`, `cube[1][1][1]` | works |
| `multiple` `{1, "two", {3, 4}}`, `mx[3][2]` | works |
| `list<multiple>` | declares |
| **`satellite.container.map<string, number>`** | **refused**: "not a type a name can have" -- the map is spelled `satellite.container.index` today, though `satellite.help` has a `satellite.container.map` topic |

**Still to do for step 1:**
- Make `satellite.container.map` a second spelling of `index` (one row of `words/aliases.tsv`),
  or ask him which name he wants.
- A generated test that writes EVERY combination up to depth 3 or 4 -- list, map/index, multiple,
  with number/string/float/bool leaves -- declares each, fills it with a literal, reads its
  deepest item back and writes one item, and records which fail. Everything that fails is fixed.
- The same for a spacesuit inside a container, and a container inside a spacesuit's field.

## Step 2 — `satellite.access(x)` shows how to reach into x

`satellite.help(satellite.access)` already says `satellite.access(object_name)` "is meant to
give you the type and value" of a name, and only its switch (`access` in config.ini) is built.
His new ask makes it also say **how to reach every level**, from the declaration. For

```
satellite.container.list<satellite.container.index<satellite.variable.string, satellite.container.list<satellite.variable.number>>> rows
```

`satellite.console.display(satellite.access(rows))` would show something like:

```
rows is a list of indexes (string -> list of numbers), 3 items
  rows[n]                 one index, n counting from 1 (1 to 3)
  rows[n]["key"]          one list of numbers, "key" a string
  rows[n]["key"][m]       one number, m counting from 1
  rows.size               how many indexes
  rows[n].keys            the keys of one index
```

...worked out from the shape the checker already reads (`type_shape.hpp`). Every level's reach
and every method that level's type has comes from a table that already exists
(`container_arity`, `index_refuses`).

**Open for him:** the exact wording and layout of what it shows; whether it answers text (so a
program can print or keep it) or prints straight away; and whether it also appears on its own
at the prompt, when he types `satellite.access(rows)` there.

## Step 3 — refusals that say the right way

Once step 2 exists, a wrong reach into a declared container can show step 2's line for that
level. For example, `rows["a"]` on a list: "rows is a list -- reach it with rows[n], n counting
from 1". That follows his rule: satl knows the declaration, so it can say what IS right, never
what the person "must have meant".
