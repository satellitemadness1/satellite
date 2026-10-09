# MAP SORT — satellite.container.map's entries, and .sort("key") / .sort("value")

Written 2026-09-26, for the author and for **the session after `/clear`**. Read this whole file first.

## START HERE AFTER `/clear`

- **`.sort` IS BUILT (2026-09-27, commit `2eee0d4`, build 0124 installed).** His
  words that day, after `/clear`: *"I wanted 3 different functions, .sort() which is just an alias for
  sort by key, and .sort("value") which sorts by value"*. So on a map: `.sort()` = `.sort("key")`, and
  `.sort("value")`. The code is `satellite/satellite_object/satellite_map_sort.hpp` -- his four
  functions: `by_width`, `by_a_to_z_0_to_9`, and one template `sorted_by<half, width>` made twice,
  `sorted_by_key` and `sorted_by_value` (*"both "key" and "value" are the same thing"*).
- **What it answers (MY CHOICE, his to overrule):** the map, its entries in the new order, as a COPY --
  as a list's `.sort()` is -- so `display(m.sort("value"))` leaves m alone and `m = m.sort("value")`
  keeps the order. The key table is repaired, not rebuilt: `m["k"]` still finds every key after.
- **The order (his words, and MY reading where they stop):** width first, then a-z / 0-9.
  - two strings: shorter first, then his character table (a-z, A-Z, 0-9): `"bo", "zoe", "alice"`,
    `"file2"` before `"file10"`, `"al"` before `"Al"`.
  - two whole numbers: sign first (the width leaves the minus out), then width, then digits -- which is
    their worth: `-10, -5, 0, 9, 10, 100`.
  - anything else (float, fraction, percentage, bool, binary...): by worth, satl's own compare --
    because a float's width counts its decimals, width first would put 1.25 after 9.5. MY CHOICE.
  - a tie keeps the order it went in (stable).
  - no order between the kinds present -- a number key and a string key, two list values -- is REFUSED
    before sorting (S301, "entry 1 and entry 2 have no order between them"), never guessed.
- **Refused, MY CHOICES:** `m.sort().by_name()` / `.by_value()` on a map (ERRORS4 page 012: they
  ordered the keys and answered a list) -- the sentence points at `.sort("key")`, `.sort("value")` and
  `.keys.sort().by_name()`; `list.sort("key")` (a list is `.sort().by_name()`/`.by_value()`); any word
  but "key"/"value"; a quoted word is judged before the run, a worked-out one when it runs.
- **Speed, measured 2026-09-27 (best of 3, idle machine):** building a 1,000,000 number map and a
  200,000 string map is 2.8 s; `.sort()` of the million adds 0.83 s (0.65 s of it is the copy of the
  map, which `c = m; c[k] = v` costs too) and of the strings 0.3 s. Sorting positions into the map
  first cost 1.6 s -- the jump into the map on every comparison was the sort -- so each entry's width
  and whole-number value is laid out in a 32-byte record first (`satelliteMapSortPlace`).
- **Proved:** 5 random maps of ~2,100 numbers (negatives, one-limb, past 2^64, to 10^30) and ~450
  strings, sorted by key and by value, equal Python's numeric sort and (length, his table) sort;
  `./check.sh` rows under "A MAP PUT IN ORDER".
- **Still open, his:** `.reverse()` on a map still answers its KEYS as a list, reversed (the rest of
  ERRORS4 page 012), so `count.sort("value").reverse()` is the keys largest-first, not the map. Whether
  a map's `.reverse()` should answer the map reversed is his. And `m.sort("value")[1]` is the KEY 1, not
  the first entry -- a map's `[ ]` is a key; `.first` / `.last` are the first and last key.
- **The other open thread:** FAST_PRINTING.md -- step 6 waits on his RUNNING_PROGRAMS.md answers.

### THE STATE BEFORE .sort (kept as it was written, 2026-09-26)

- **Built (his order: "just build the satellite.container.multiple 2x with a width first, don't worry
  about sorting it at all just yet"):** each map entry is a key, a value and a width for each --
  commit `8237187`.
- **Also built the same evening:** S413 COUNTS_FROM_ONE, machine code 66 -- every position 0 in the
  language (commit `861c9a2`). His words: *"will requesting [0] of anything return an error? We need to
  build an error report that explains that satellite starts counting from 1 and not 0"*.

## WHAT IS BUILT (8237187)

`satellite/satellite_object/satellite_map_entry.hpp`:
- **`satelliteMapWidth : public satellite_number`** -- adds nothing (24 bytes); it adds, compares and
  prints as a number. Only map entries hold one, so no other number carries a width.
- **`satelliteMapEntry { key, value, key_width, value_width }`** -- key and value are each a
  `satelliteObject`, which IS a `satellite.container.multiple` (type_shape.hpp: *"multiple is a
  std::variant"*). `satelliteIndex::entries` (the map; map = index) is a vector of these, in the order
  they went in, beside its hash table from key to place.
- **A width:** a number's digits (the minus sign not counted); a string's characters (`"héllo 😀"` is 7);
  the characters any other one value displays as (a float, a bool, a fraction...), minus sign not counted;
  **0** for a list, a map, an object, a file, a window, a thread, nothing.
- **Kept at all times:** a key's width is measured once (a key never changes); a value's every time a
  value goes in -- `put_value` is the only way in (`entry_for_writing`, `file_under`). A write INSIDE a
  value (`m["a"][2] = 5`, `.append`) goes through `value_to_change_inside`, never changes the value's
  kind, and so never its width.
- **Proved:** a scratch harness through the real functions, 22 of 22; check.sh; the container sweep at
  depth 4 (5,412 programs, 0 wrong).
- **Cost, told to him:** 1,000,000 number and 200,000 string map writes, best of five: 1.52 -> 1.55 s
  (+2%), 20.6 -> 22.0 MB. He did not object and moved on.
- **My choices, his to overrule:** TWO widths per entry (he said *"the width of both the key and the
  value"*, and also *"a width for each entry"*); the name `satelliteMapEntry` (he said that first, then
  *"satelliteMap ... that is an individual entry"*).

## HIS WORDS, IN ORDER (2026-09-26)

On hearing a C++ map is sorted and satellite's is not: *"so we just need to provide .sort() on maps then"*.

Shown that `.sort().by_name()` / `.by_value()` already exist and on a map sort only the keys into a list
(ERRORS4 page 012), he gave the design:

> *"let's change the syntax a little bit, and let's program this now... let's have .sort accept a string
> or a string object, so we type sort("key" or "value") key returns a list with the smallest by key at
> position[0] and value returns the smallest value at position[0] and we sort by a - z and by size at the
> same time, so 10 is always bigger than 9,  it's 2 digits, so if the key is a number, we are sorting by
> two schemes at the same time -- 0 - 9 and the amount of digits at the same time, so
> satellite.container.map holds the width of both the key and the value at all times, in a
> satellite.variable.number object, I don't necessarily want our numbers to have to adjust a width for
> every single calculation, so we will have to modify our satellite.variable.number class and take a copy
> of it and have this copy only used for satellite.container.map to hold a width, we will call it a
> satelliteMapWidth, and it's superclass will just be satellite.variable.number, if that is possible"*

Asked how `.sort` should order, and what its list holds, he answered:

> *"let's spend a little bit of time and do it correctly, we will build all new stuff for our
> satellite.container.map, so just hold on for a second, each map entry is always two values, correct? A
> key and it's data, so we build a satelliteMapEntry class -- it has a satelliteMapWidth, and a
> std::variant of either a satelliteUserDefinedClass object, a satelliteNumber, a satelliteString, and it
> has dual std::variants, so we can actually build this out of a satellite.container.multiple -- the map
> is two things, one thing is a key, which, is just a satellite.container.multiple, and a value, which is
> also just a satellite.container.multiple, they just need to be kept together, given a width, and they
> have to at times be sorted -- so we need a total of 4 new functions just for map's .sort() function,
> one that sorts by width, one that sorts a - z and 0 - 9, and then it either does it by key or by value,
> which, both are just satellite.container.multiple's so we only need 2 new functions for sorting, one
> that sorts by "key" and one that sorts by "value", but both "key" and "value" are the same thing, the
> satellite.container.map is just two satellite.container.multiple's; can you get it to work that way?? I
> dunno, I am partially lost myself, so it doesn't matter if you need to ask for clarification or
> something, we need two satellite.container.multiple's stuck together and sorted see we are trying to do
> two things at the same time -- we are building our map and building the .sort on it at the same time,
> and we don't need to do that. Just make a satelliteMap class out of the satelliteMapWidth and a
> satelliteObject I think it's called, whatever we use as a class name for our objects, satelliteMap is
> simply two of them together -- that is an individual entry, so all that this is is over complicating
> something that is simply two satellite.container.multiple's stuck together with a width added on, and
> you already know how to stick two satellite.variable.number's together -- just look at the
> satellite.variable.float, it's just two satellite.variable.number's, this is no different, there's just
> a satelliteMapWidth added on this time, just like adding a satellite.variable.bool onto the float to
> keep the sign of the numbers, all that we are doing is adding satellite.container.multiple together, and
> adding a width, and after we do that LATER we can do .sort, once we have the satellite.container.map
> built out of 2x multiple and a width, .sort is just 4 functions with templates, one function to sort by
> "key" and one by "value", this sorting is just returning what value is inside of the "key" or inside of
> the "value", a - z and 0 - 9 and width together, just build the satellite.container.multiple 2x with a
> width first, don't worry about sorting it at all just yet"*

> *".sort alters our 2x satellite.container.multiple with a width ordering, your trying to do to many
> things at the same time, just build the map, it's just a satellite.container.multiple x2 and a width for
> each entry, so a satellite.container.map is just two multiples and a width, there's nothing more to it
> than that, you just need a class that has two registers -- one for the key and one for the value, and
> both key and value are a satellite.container.multiple"*

## OPEN BEFORE .sort WAS BUILT (2026-09-26) -- see START HERE for what each became

1. **What `.sort("key")` / `.sort("value")` answers.** His first words: *"key returns a list with the
   smallest by key at position[0] and value returns the smallest value at position[0]"*. His later words:
   *".sort alters our 2x satellite.container.multiple with a width ordering"* -- which reads as the map's
   own entries put in order (the map changed, or a sorted copy of it?). The three options put to him (he
   answered with the design above instead): a list of the pairs `{{"or": 1}, {"be": 2}}`; one half each
   (keys, or values); or the keys ordered by their values.
2. **Width first, then a-z / 0-9 -- for every kind?** Shown to him, run on build 0118: satl's own compare
   already orders `{10, 9, 100, 1}` as `{1, 9, 10, 100}`, `{-5, -10, 3}` as `{-10, -5, 3}`, `{10.5, 9.25}` as
   `{9.25, 10.5}` and `{"zoe", "alice", "bo"}` as `{"alice", "bo", "zoe"}`. Width first gives the same for
   whole positive numbers. What was shown to him where it differs: `10.5` before `9.25` (both 4 wide, and
   "1" sorts before "9"); `"bo", "zoe", "alice"` (shortest first, not a to z); and, counting the minus
   sign, `3, -5, -10`. The widths as BUILT leave the minus out (as a float's sign is its own bool), so -5
   and 3 are both 1 wide and something else must order them. He has not ruled on negatives, decimals or
   words.
3. **Positions:** his *"position[0]"* is satellite's `[1]` -- satellite counts from 1 (S413 now says so).
4. **The old spellings:** `.sort().by_name()` and `.sort().by_value()` on a map still sort only the keys
   into a list (ERRORS4 page 012). Keep, replace, or refuse them on a map once `.sort("key")` exists?
5. **"4 functions with templates"**: by width, by a-z / 0-9, each by key or by value -- *"both "key" and
   "value" are the same thing"*, so 2 sorting functions over a multiple.

## WHERE THINGS ARE

- `satellite/satellite_object/satellite_map_entry.hpp` -- the width and the entry.
- `satellite/satellite_object/satellite_index.hpp` -- the map: `entries`, `where`, `entry_for_writing`,
  `file_under`, `value_to_change_inside`, `value_at` (read only).
- `satellite/bytecode/container_calls.cpp` -- `.sort()` (`sort_token`, `by_name_token`, `by_value_token`)
  and `items_of`, which gives a map's KEYS: where `.sort("key")` / `.sort("value")` will go.
- `SCRATCH.md/ERRORS4/012-map-sort-and-reverse-use-keys.md` -- the defect this replaces.
