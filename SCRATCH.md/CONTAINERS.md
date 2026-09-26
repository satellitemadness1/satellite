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
