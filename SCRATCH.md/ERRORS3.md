# ERRORS3 — what the author's testing found, 2026-09-26

The author tested the installed satl (`2119b0f`, BUILD 0100) from other windows, with AI
testers running programs past each stop point, while the website was being published. This
file holds everything they reported that no list had yet, **with where each lives in the code
and what it is related to**, so whoever fixes one starts from the cause and not the symptom.
ERRORS2.md still holds the older list; its Part 1b (the first finds of this same session)
points here.

**Status words:** FIXED (with the commit), IN PROGRESS (a builder is on it), TO FIX, HIS TO RULE.

**Two builder workflows are running as this is written**, each in its own worktree, to be
merged when done: **M16** (the string methods: `.size .contains .split .replace .trim ...`) on
branch `m16-string-methods`, and **ERRORS2 Part 2** (#7 run-time type refusals as full
reports, an unclosed `{` blaming the next line, #10 `n.power`, #9 003 spellings, #12 help
examples in check.sh, #11 multi-line carets) on branch `errors-2026-09-26`. Nothing below
overlaps them unless it says so.

---

## FIXED today (in the main checkout, not installed yet -- he was testing the installed satl)

| # | what he found | fix | commit |
|---|---|---|---|
| 1 | `m.has("a")`, `m.get("a")`, `m.set("a", 1)` refused on a map, though the map help page names all three. His own programs call them 70 times (003's spelling). | `.has` is contains's third spelling (REGISTRY.satellite; the arguments' `.has(k)` works too); `.get(k)`/`.set(k, v)` read beside the arguments' rows in expression.cpp and go through `index_into`/`write_through_index`, the same two functions as `m[k]` and `m[k] = v`. | 5b8cdf3 |
| 2 | `satellite.variable.string empty` then `display(empty)` refused: it held nothing. | A bare string starts as `""` (program_walk.cpp `empty_container_for`), as a bare list starts empty. | 5b8cdf3 |
| 3 | S501's paragraph says "the file has no line" when a list or a map ran out; S301's says "this operator" under an append or a `[ ] =` that did not fit. | Both paragraphs say what they mean for every case (machine/s_codes.hpp); codes and names unchanged. | 5b8cdf3 |
| -- | (ERRORS2 #8) a wrong-kind literal (`number n = "abc"`) refused only when its line ran. | Refused before anything runs, cell by cell from the walker's own table. | 0da03d9 |

**After these are installed**, the website's error boxes that print S301/S501 paragraphs are
stale: run `refresh_outputs.py` and publish (SCRATCH.md/PUBLISH.md).

---

## HIS TO RULE

### 4. Block scope: a name declared inside an if is still readable after its `}`

```
satellite.statement.if (1 == 1)
{
    satellite.variable.number y = 5
}
satellite.console.display(y)     // 5 -- y outlived its block
```

**Why it happens:** a body's variables are ONE table per capsule (`VariableTable`, value.hpp),
and an if/while body is walked in that same table -- nothing ends at a block's `}`. A for
loop's own number IS erased when the loop ends (MILESTONES M20.A), so blocks and for-numbers
already disagree. No document rules on block scope (searched).

**The two ways:** C++ (and 003-style braces): a block's names end at its `}`, and reading `y`
after it is refused BEFORE the run as "not declared here". Python: names live on to the end of
the capsule. **5 below depends on the answer.**

### 5. A declaration inside an if that did not run gets a false message

```
satellite.statement.if (1 == 2)
{
    satellite.variable.number y = 5
}
satellite.console.display(y)
```
The pre-run check accepts reading `y` (it saw the declaration), then the run says **"y has no
satellite.variable line declaring it"** -- though there is one. If blocks keep their names
(Python's way), this should say "y has no value yet -- the line that gives it one did not
run". If blocks end their names (C++'s way), the check refuses the read before the run and
this cannot happen. Cause: program_check.cpp's `declared` map is one per body, and the walker
declares only what it runs (program_walk.cpp `run_assignment`).

### 6. What a bare number holds

A bare string starts as `""` now (fix 2). A bare `satellite.variable.number n` still holds
nothing, and reading it is refused (see 11). 0 would be C's global rule and not C++'s local one;
Python has no bare declaration. His call.

---

## TO FIX, most harmful first

### 7. Ctrl-C cannot stop a loop typed at the prompt -- THE WORST ONE

```
satl --repl
satellite.variable.number n = 0
satellite.statement.while (n > -1)
n = n + 1
}
<Ctrl-C>
```
Nothing happens; one core stays at 100%; the prompt never returns, and lines typed after are
echoed and never run. The same loop through `interpret loop.satl` stops on Ctrl-C and the
prompt comes back, and `satl loop.satl` quits on Ctrl-C.

**Cause:** at the prompt, Ctrl-C runs session.cpp's `on_interrupt`, which only sets
`asked_to_stop` -- the flag `stop_flag()` (machine/stop_flag.hpp) hands to a library. Only the
directory listing and satellite.info read it (info_calls.cpp, expression.cpp's directory call).
The walker's while and for loops never do, so a typed loop is never told. `interpret` runs its
file in a CHILD process (prompt_run.cpp: "Ctrl-C belongs to the program while it runs"), which
is why that path stops. A SECOND Ctrl-C `_exit`s the whole session (on_interrupt's `presses`),
so the only way out today is to leave satl.

**Fix:** the walker's loops (and the statement loop) read `stop_flag()` once a pass and stop the
line with machine code 130 `interrupted`, and the prompt returns. `stop_flag()` is null outside
a session, so a file run pays one pointer test a pass (measure it on his race).

### 8. Building a string in a loop is quadratic

`s = s + "ab"` in a for loop: 0.34 s for 20,000 passes, 1.30 s for 40,000, 5.66 s for 80,000 --
4x each time the count doubles. The same counts of `list.append(i)`: 0.08, 0.09, 0.14 s;
`map[i] = i`: 0.09, 0.13, 0.23 s. (His single runs; time it with `time satl p.satl`.)

**Cause:** `str_add_str` (satellite_object/str_add_str.cpp) makes a NEW string of both sides
every time, so each pass copies everything built so far. A list does not: `.append` writes into
the variable's own list when nothing else holds it (satellite_list.hpp's `use_count() == 1`
story, the quadratic append 003 shipped for months). CPython special-cases exactly this line
(`s = s + t` appends in place when `s` is the only reference). **Fix:** in `run_assignment`,
when the value is `name + <string>` and the name holds the only reference to its string, append
in place. ERROR.md records the number `to_text` as quadratic (#29) but not strings. Relevant to
QUAD AI (memory: string throughput is a priority).

### 9. `[n]` works only straight after a name

Refused at run time:
```
make_list()[2]
m.keys[2]
l.sort().by_value()[1]
{10, 20}[2]
```
while `name_of(3).upper()` and `grid[2][1]` work. Putting the value in a name first works around it.

**Cause:** expression.cpp's `maybe_a_method` -- what runs after any value (a call's answer, a
literal, a method's answer) -- follows a `.method` but not a `[`. Indexing is handled only in the
name path of `one_operand` ("`f[n]` -- LINE n OF A FILE, and `a[n]` -- ITEM n OF A LIST").
**Fix:** a `[` after any value reads through `index_into`, the same function the name path
uses. Writing through a call's answer (`f()[1] = 2`) stays refused: there is nothing to change.

### 10. The catch-all refusal blames spaces that are there

"check that every math sign has a space on both sides" is shown for `#7`, for `3/-4`, and for
`a < b || c < d` -- every sign already has its spaces. All three fail HALFWAY through the run,
after earlier lines printed.

**Cause:** call_word in expression.cpp: when the evaluator stops at a code it cannot use and the
line has not ended, this is the one sentence. It should name what it stopped at: `#` is not a
character satellite uses; `3/-4` is a fraction's `/` touching (`fraction_token`) followed by a
minus; `||` is not built yet. And the checker does not judge operators, which is why they fail
mid-run -- an unknown operator code in a statement can be refused before the run.

**`&&` and `||`:** inside an if they get a different message calling them **"undecided"**
(program_walk.cpp:80), and token_codes.hpp still marks `and_token`/`or_token` "QUESTION". **He
ruled them on 2026-09-22** (`&&` `||` `!` for logic; `&` `|` `~` `<<` `>>` for bits; `.and()`
`.or()` `.xor()`; build after the type merge). So the texts are stale, and `&&`/`||` themselves
are owed -- `!` works. Building them is short-circuit evaluation in `evaluate_at`'s precedence
table, below the comparisons.

### 11. A name with no value gets four different messages

- `display(x)` says S210 NOT_BUILT_YET and exits 14.
- `x + 1` says "+ was given nothing and a number".
- `display()` with nothing in it passes the pre-run check, then fails at run time as
  NOT_BUILT_YET; `display("a", "b")` is refused before anything runs.
- A method on the same name already says the right thing: **"x has no value yet -- give it one
  with ="** (slot_through_index / write_through_index in expression.cpp).

**Fix:** one sentence wherever a name holding nothing is READ (one_operand's name path, before
the value is used), with the same words as the method path; and the checker refuses `display()`
with no argument, as it refuses two.

### 12. More explanations that describe the wrong thing (the family of fix 3)

- A list's `.remove(7)` when 7 is not in it: **S420 TEXT_NOT_FOUND**, "the text looked for is not
  in this string" (s_codes.hpp:226).
- `l[-1]`: **S410 NOT_A_POSITION**, "a position or a line number below zero. Lines count from 1"
  (s_codes.hpp:207).
- At the prompt, `1 / 0`'s report ends "satl exits with this", but the prompt carries on. The
  report writer should say "the line stops" when a session runs it.

Each is one paragraph (or one flag to the report writer); keep the S-codes and names.

---

## CHECKED AND WORKING (his testers' list, kept as the record of what was covered)

- Arithmetic: big numbers, negative `/` and `%`, `^` with negative powers, every zero divisor.
- Other number types: floats, percentages, fractions, binary and hex.
- Strings: accented letters, emoji, escapes, and strings holding `,` `)` `}` `:` `//` `/*`.
- Program structure: includes, namespaces, library values, errors named in the included file,
  recursion 100,000 deep.
- Spacesuits: inheritance, a linked chain, a stack, objects in lists and maps, returned from
  capsules, made in a loop.
- Containers: nested lists and maps. `satellite.access`.
- The prompt: blocks, else, braces in strings, `interpret` of failing and missing files.
- The pre-run checks cannot be fooled by `satellite.return(satellite)` in a string or a comment.

**Next:** his testers' finds become check.sh rows as each is fixed, and a family (like 12's
paragraphs, or 9's "after any value") becomes a generated test, as check_container_shapes.py
is for containers.
