# ACCESS_PLAN.md — building values, reaching into them, and showing the way in

Started 2026-09-14 at `ad0a4a1`, satellite 003 revision 06. **The author's
order: brace initialization first, then lists within lists, then
`satellite.container.multiple`, then reaching into a multiple, then
`satellite.access`.** Each stage is a milestone below (A1–A5) with a program
that proves it. Anything found while building one is added to §2 before that
stage can land.

The goal, in one program the language cannot run today:

```satellite
satellite.container.list<satellite.container.list<satellite.variable.string>> my_lists = {{"str1", "str2"}, {"str3", "str4"}}
my_lists[0].append("str5")
my_lists[1][0] = "changed"
satellite.access(my_lists)
```

---

## 1. What runs today — measured, not remembered

Every line below was run through `./satl` built 2026-09-14 10:50 at `ad0a4a1`.

| program | result |
|---|---|
| `list<list<string>> x = satellite.container.list()` and `x.append(inner)` | **works** |
| `x[0]`, `x[0][1]`, `cube[0][0][0]` (three deep) | **works** — `[hello, world]`, `world`, `7` |
| `map<string, list<string>> m`, then `m["k"][0]` | **works** — `a` |
| `display(x)` | **works** — `[[hello, world], [goodbye]]` |
| `= {{"str1", "str2"}, {"str3", "str4"}}` | **S0231** at the first `{` — and then **six more errors**, because the parser loses the capsule's own braces (§2, F1) |
| `x[0].append("third")` | **S0720** — a method only folds through a declared name (WORD_NUMBERS §1.5) |
| `x[0][1].size()` | **S0720**, the same |
| `x[1][0] = "changed"` | **S0720** — an index write needs a name to write back to (DESIGN §6.4) |
| `x.append(first)`, then `first.append("y")` | `x` is unchanged — **a container is a value and is copied in.** This plan does not change that; a spacesuit is the reference type |
| an argument list continued onto the next line | **works** — so a nested literal can be written over several lines |
| `m.set(some_list, 1)` | **S0727** — a list is not a map key. §3's map literal depends on this refusal |

**`{ }` initialization existed in the FIRST satellite and was not carried
over.** `old_versions/first_satellite/plans/brace_literals.txt` is the design
and its "WHAT LANDED" section lists the files and twelve tests. A1 ports it.
Read it before starting A1; its five decisions are §3's starting point.

**WORD_NUMBERS §1.5 says "NO MILESTONE OWNS LOOSENING" the one hop.** A2 and A4
are that milestone for positions inside a declared container, and §1.5 gets a
paragraph saying so when A2 lands.

---

## 2. Defects found while checking

- **F1. One wrong character makes seven errors.** A `{` the expression parser
  does not accept puts the parser into panic, it unwinds to the top level, and
  every later line of the capsule reports S0204 ("a file holds
  satellite.include, satellite.capsule…"). Recovery should resume at the next
  line of the SAME body. Fixed in A1, with a test that one bad line gives one
  error.
- **F2. The "that looks like Python" hint misfires.** On
  `my_list_of_str[0].append("third")` it suggests `my_list.append(x)` — the
  spelling the program already used. `error_reporter/foreign.cpp` matches the
  line's TEXT: its guard skips a line containing `satellite.`, and a method call
  on a declared name contains none. So any error on a correct line like
  `names.append(y)` is answered as Python. Fixed in A2: the guard also skips a
  line whose receiver resolved to a declared name.

- **F3. An object holding one number costs ~937 bytes, and ~600 of them wait
  for a second thread.** `sizeof(SuitObject)` is 216, and 184 of that is
  threading: `std::recursive_mutex hold` (40) and `thread::Access` (144).
  `Access` holds a `std::deque`, and libstdc++'s deque allocates its map and
  a 512-byte block when it is CONSTRUCTED, empty or not. That's 216 + 16 +
  the field vector + ~600 for the deque + the 40-byte list slot, about 936
  bytes, against 937 measured. A program with no threads pays it on every
  object. It matters for QUAD's millions of objects, and A5's size column
  will show it. **Not this plan's to fix**: creating the queue only when a
  second thread first waits is a decision for THREAD.md.

---

## 3. The rules — v1's, and what changes in this tree

**R1. The literal takes its shape from where it lands** (v1 decision 1).
`{a, b}` stored into a `list<…>` is a list; into a `map<…>` a map; into a
`multiple<…>` a multiple. With nothing declared — `display({1, 2})` — it is a
list.

**R2. Only what was written as braces is reshaped** (v1 decision 2). A variable
that holds a list stays a list wherever it goes.

**R3. The declaration shapes the literal and does not check it — THIS TREE
DIFFERS FROM v1.** v1's `matches()` refused a literal whose values were the
wrong type. This tree has no store check anywhere: `satellite.variable.number n
= "abc"` is accepted and prints `abc` (DESIGN §8.7, "the declared type is a
dispatch key, not an enforced invariant"). A literal checking its contents would
be the one store in the language that did. **Decision D2.**

**R4. A map literal has v1's two spellings** (v1 decision 4):
`{"a", 1, "b", 2}` and `{{"a", 1}, {"b", 2}}`. The flat reading is tried first;
its keys come out as lists, S0727 refuses a list key, and that refusal selects
the paired reading. `{}` is an empty container of whatever was declared.

**R5. Nesting has no depth limit** (v1 decision 5, and DESIGN §7.5). Shaping,
reading, writing and `access` each walk with their own stack on the heap. No
recursion on the C++ stack, and no constant in a header.

**R6. A place is a name followed by positions.** `x`, `x[0]`, `x[0][1]`,
`m["k"][2]` all name somewhere a value can be written back to, because the
first word is a declared name (local, global or field). `f()[0]` names nowhere
and stays refused under DESIGN §6.4.

**R7. A position's type comes from the declaration.** `list<T>[i]` is `T`,
`map<K, V>[k]` is `V`, `multiple<T0, T1, …>[i]` is `Ti`. That is how a method
on `x[0]` finds its number at resolve, exactly as `x.append` does today.

---

## 4. Decisions only the author can make

Each has a recommendation, and the plan is written against it until answered.

- **D1. How many types does a `multiple` hold?** Recommended: **any number,
  one or more**, fixed by the declaration — `multiple<list<list<string>>,
  number, string>` is legal. No `append`; its size is its declaration.
- **D2. Does a literal check its contents against the declaration?**
  Recommended: **no**, R3 — the same as every other store in this tree.
- **D3. How does `display` print a multiple?** Lists print `[a, b]` and maps
  `{k: v}`. Recommended: **`(a, b)`** — `([str1, str2], 42)`.
- **D4. A method on a multiple's position that is not a literal** —
  `m[i].size()`, where `i` is a variable. The declaration cannot say which
  type `m[i]` is. Recommended: **choose the method at run time by what the
  value holds**, only on this path; every literal position still folds at
  resolve. The alternative is refusing and asking for a literal position.
- **D5. `satellite.access` on a million items.** Recommended: **print every
  row**, as `display` prints every value.
- **D6. `satellite.container.multiple` takes `1 4 5`**, the next free number
  under `satellite.container` (`1 4 3` is `arguments`, `1 4 4` is `result`).
  `satellite.access` takes **`1 26` — confirmed by the author 2026-09-14.**
- **D7. Shared memory in the size column.** Recommended: **count it once**
  in every total, and mark the repeat row `shared with <place>`. Counting it
  twice would make the top row larger than the memory the process really uses.
- **D8. Rounding the size column.** `satellite.system.memory`'s rows answer
  exactly and never round (`memory_methods.cpp`), so they give
  `1.0986328125 kb`. Recommended for the column: **one decimal place**,
  `1.1 kb`, and whole numbers of bytes below 1 kb.

---

## 5. The milestones

### A1 — brace initialization

**First step: the declared type travels as a tree.** `resolve::Info::type` is
one `PathId`, so `list<list<string>>` is recorded as `1 4 2` and nothing else.
The inner types exist only in the parse tree (`frame_->types` keeps the node).
R1, R7 and A5's empty-list rows all need them, so `Info` gains the declared
TYPE NODE beside the PathId, copied onto every Name exactly as `type` is now,
and a helper answers "the type of position i under this type node".

Then:

1. **Parser** (`parser_expressions.cpp`): `{` in operand position opens a
   literal frame, `open_bracket()` so it may span lines, `,` separates, `}`
   closes. A new `NodeKind::Brace` with a list of elements. `unparse` prints it
   back. F1's recovery fix lands here.
2. **Shaping** (a new `evaluator/brace_literals.cpp`): given a Brace node and a
   type node, emit ops that build a list, a map (R4) or — after A3 — a
   multiple, walking nested braces with its own stack (R5).
3. **Where it applies** — every place a value meets a declared type:
   - `T x = {…}` and `x = {…}`
   - `.append({…})`, `.insert(n, {…})`, `.set(k, {…})` — the element type from
     the receiver's declaration
   - a capsule argument, against the parameter's type
   - a spacesuit field's initialiser, and `return {…}` against `returns`

   v1 left the last three out; this tree takes them, because "do it for the
   user" is the rule.

**Done when** `example/braces.satl` builds a `list<list<string>>`, a
`map<string, list<number>>` in both R4 spellings, a
`list<map<string, number>>` via `.append({"a", 1})`, an empty `{}` of each, and
a literal passed to a capsule and returned from one — and displays each. Also
when `eval_test` pins R2 (a variable is not reshaped), one bad line gives one
error (F1), and a 100,000-deep nested literal builds without a crash (R5).

### A2 — lists within lists: methods and writes through positions

1. **Resolve**: an `Index` node takes its type from R7, so the existing
   selector fold in `call_target_done` reaches `x[0].append` and
   `x[0][1].size()` through the type NODE. This is the one-hop change; WORD_NUMBERS
   §1.5 gets a paragraph saying positions inside a declared container are now
   folded, and a call's answer still is not.
2. **Compile**: a place (R6) compiles to its root slot plus its position
   operands, all reduced to values first (DESIGN §6.5).
3. **Run** (`operations_subscript.cpp`): `op_place_store` and a mutating
   method on a place take the root out of storage — `19526c9`'s move —
   descend taking each child out, mutate the innermost in place when its
   handle is unshared, and put each level back on the way up. Its own stack
   (R5). A read-only method (`size`) skips the write-back.
4. F2's hint fix.

**Done when** `x[0].append("str5")`, `x[1][0] = "changed"`,
`x[0][1].size()`, `m["k"].append(3)` and `cube[0][0].append(9)` all run in a
local, a global and a spacesuit field. Also when `b = x` then `x[0].append(…)`
leaves `b` unchanged (the copy path still fires when shared), when 1,000,000
`grid[i % 100].append(i)` stays linear (measured against a flat append), and
when `f()[0].append(1)` is still refused.

### A3 — `satellite.container.multiple<T0, T1, …>`

1. `words.def` rows under `1 4 5` (D6): the bare shape `multiple()` and
   `size()`. WORD_NUMBERS §2.2, `help_lines/nodes.tsv`, the help entry and
   `help.def` move in ONE commit. The help bootstrap is circular: seed the new
   `help.def` rows, build, then `make nodes` and `python3 help_lines/gen.py`.
2. **A seventeenth Value arm**, `Mul = shared_ptr<const Multiple>`, appended at
   the end per `value.hpp`'s append-only rule. The 40-byte assert is re-run.
3. The parser's `type()` already reads `<T0, T1, …>` with any count, so the
   declaration needs no grammar.
4. A1's shaping learns the multiple: element i of the literal against `Ti`.
   A literal with the wrong number of elements is an error — the one count this
   plan does check, because a missing position has no value to hold. It needs a
   new code, not a type check.
5. `display` prints it per D3; `==` compares position by position.

**Done when** `multiple<list<list<string>>, number> pair = {{{"a", "b"},
{"c"}}, 42}` declares, displays as `([[a, b], [c]], 42)`, and round-trips
through a capsule argument and a `return`.

### A4 — reaching into a multiple

A2's machinery with R7's third row: `pair[0]` is the list of lists,
`pair[0][1][0]` is `"c"`, `pair[1]` is `42`. Methods fold at resolve for a
literal position; a computed position follows D4. Writes (`pair[1] = 43`,
`pair[0][0].append("z")`) go through A2's place op unchanged. A position out
of range is refused by name, with the declared count in the sentence.

**Done when** every one of those runs in `example/multiple.satl`, including a
multiple inside a list (`list<multiple<string, number>>`, then
`rows[3][1] = 7`) and a list inside a multiple inside a map.

### A5 — `satellite.access(place)` `1 26`

The layout the author chose on 2026-09-14: **every item on its own row, then a
list of methods after the whole table.**

```
satellite.access(my_lists)

name            | type                      | size   | value
----------------+---------------------------+--------+---------
my_lists        | satellite.container.list  | 1.1 kb | 2 items
my_lists[0]     | satellite.container.list  | 512 b  | 2 items
my_lists[0][0]  | satellite.variable.string | 112 b  | "str1"
my_lists[0][1]  | satellite.variable.string | 112 b  | "str2"
my_lists[1]     | satellite.container.list  | 512 b  | 2 items
my_lists[1][0]  | satellite.variable.string | 112 b  | "str3"
my_lists[1][1]  | satellite.variable.string | 112 b  | "str4"

methods
my_lists.method()  my_lists[x].method()  satellite.container.list
append(x)  size()  sort()  sort(direction)  sort_down()  ...every row, on one line

my_lists[x][x].method()  satellite.variable.string
size()  empty()  find(x)  contains(x)  substring(start, end)  ...every row, on one line
```

- **Rows** are the place, then everything inside it, in order: a list's items
  by position, a map's by key (`m["k"]`), a multiple's by position, a
  spacesuit's by field (`ship.fuel`). The argument may be a place, so
  `satellite.access(my_lists[1])` starts there.
- **Type** is the value's own arm. A slot holding nothing, or an empty
  container, shows the DECLARED type (A1's type node) and a row
  `my_lists[x]  satellite.container.list  (empty)`, so the shape shows before
  any value exists.
- **Size** *(the author, 2026-09-14)* is the memory the object uses, with
  everything inside it added in: a list is its own storage plus every item's
  size. The sizes in the mockup are illustrative; A5 measures them.
  - **Nothing is tracked.** `access` already visits every item, so each size
    is added up on that same pass, from the innermost items out. A running
    program that never calls `access` pays nothing. Keeping a running count
    instead would slow every `append`, `set` and string join in every program,
    for a number only `access` reads.
  - **It is real memory, not text length.** Measured 2026-09-14 at `03cadb7`,
    1,000,000 of each in a list: **~153 bytes per short string** (`"str123456"`)
    and **~42 per number**. A heap block counts what the allocator really
    handed over (`malloc_usable_size`), so the total matches what the process
    pays.
  - **C++'s `sizeof` cannot answer it on its own.** It is fixed when the
    program is compiled, so it measures a type's fixed part and never what the
    type points to. Compiled against this tree's `value.hpp` on 2026-09-14:
    `sizeof(satellite::Value)` is 40 for every value, `sizeof(satellite::List)`
    is 24 for a list of none or of a million, and a 1,000,000-character
    `std::string` is 32 bytes by `sizeof` while it really uses 1,000,033.
    `sizeof` supplies the fixed part (the 40), and the rest has to be asked of
    the value.
  - **So every value type answers two questions, and none of them stores
    anything.** `payload_bytes()` is the bytes it owns beyond its 40-byte slot.
    `payload_id()` is the address of that memory, so shared memory is counted
    once. **`Number` already has both** (`bignum_number.hpp`, used by
    `satl --number`), with the comment "shared storage is counted once". A5
    writes the same pair for the other fifteen arms:

    | arm | what it counts beyond the 40-byte slot |
    |---|---|
    | nothing, bool, runtime, time | 0 — they fit in the slot |
    | number | `Number::payload_bytes()`, which exists: 0 in the small form, 4 per limb once boxed |
    | string, float, binary, hex | the body behind the handle and its character or digit storage |
    | list | the body, then CAPACITY × 40 (not length), then every item's own size |
    | map | the body, entries capacity × 80, the key index's buckets and key strings, then every key and value |
    | file | the handle and its read buffer, plus zlib's state when the file is gzipped |
    | arguments | the object and its strings |
    | capsule | the packaged call and the arguments it holds |
    | thread | the handle and the answer it holds; its OS stack is `satellite.system.memory.this`'s to report, not this column's |
    | spacesuit | the object (216 bytes fixed, then its field vector) and every field's own size — **not its capsules**, below |

  - **A spacesuit object does not hold its capsules, so its size doesn't
    include them.** `suit_object.hpp`: an object is one pointer to a `Layout`
    that every object of that spacesuit shares, plus its fields, plus its
    threading hold. The capsules are compiled once into the program. Measured
    2026-09-14: 1,000,000 objects of a spacesuit with one number field cost
    **937 bytes each with a 1-line capsule, and 936 with a 3,000-line
    capsule.** `access` shows the capsules in its methods section, and
    capsules added to one object (POLY_PLAN.md, the author's next plan) will
    count in that object's size.
  - **A spacesuit can point back at itself**, which DESIGN §12's refcounting
    cannot free and S1501 warns about. The walk remembers every
    `payload_id()` it has visited. An object it meets a second time gets one
    row saying `shared with <place>`, and the walk does not go inside it again.
    That rule is what stops `access` from printing forever on a ring, and it's
    the same rule as D7's.
  - **A shared body is counted once in the total.** `b = a`, or appending one
    list twice, makes two handles to the same memory. Each row still shows its
    own size, but a row whose body already appeared says `shared with
    my_lists[0]`, and the rows above it don't add it again. **Decision D7.**
  - **Units are `units.hpp`'s**: `b`, `kb`, `mb`, `gb`, `tb`, each 1024 of the
    last, so 1024 b is `1 kb`. A row uses the largest unit its size reaches.
    **Decision D8** covers rounding.
- **Value**: strings quoted (`display` does not quote, so `"1"` and `1` look
  alike there), containers `N items`, nothing as `nothing`.
- **Methods**: one block per type met, headed by the access patterns that reach
  it, then every row under that type in `words.def` on ONE line that is never
  wrapped. Brackets and arguments come from the help line where the registry
  spells the name bare (`size`, `append`). A spacesuit's block lists its own
  methods.
- **Columns** are sized over all rows, so the walk runs twice: once to measure
  and once to print. Pipes and files get the same text with no style (the
  2026-09-12 decision).

**Done when** the top row's size for a 1,000,000-string list agrees with
`satellite.system.memory.main("b")` measured before and after building it,
within the allocator's page rounding. That check proves the size column. And
`example/access.satl` prints the table above for `my_lists`, and
also for a map of lists, a multiple, a spacesuit holding a list, an empty
declared list and a 100,000-deep nest (R5). The help line runs under
`verify.py`, and **the author has seen it in satl-term** — which is what they
asked for.

---

## 6. Every stage, every time

- `make test` over all fifteen suites, `help_lines/verify.py`, the binary's
  mtime checked against its sources (a make alias can report ok having
  compiled nothing).
- `satl --words <path>` before minting anything.
- One `make` at a time, checked quiet in its own tool call.
- Commit when a stage lands, and write its hash back into this file.
