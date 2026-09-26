*satellite design docs, §8, part 4 of 4. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§8 part 3](08-c-bool-characters-and-maps.md), On: [§9 part 1](09-a-runtime-architecture.md).*

---

## 8. Types — size, length and arguments

*Continues [§8 part 3](08-c-bool-characters-and-maps.md).*

### 8.7 `.size()` is bytes, `.length()` is items

Two questions had been sharing one word. "How many characters is this string" and "how much
memory does this string cost" are different questions with different answers, and until now the
language could only ask the first. `.length()` keeps that meaning everywhere it already had it,
and `.size()` is the second question, on every receiver that has methods at all.

    "abc".length()     3      characters
    "abc".size()       46     bytes

The gap between 3 and 46 is the whole point of having both: a value is a node plus its content,
and a program deciding whether to hold a million of something needs the second number.

**A number answers neither `length` the way a container does.** `.length()` on a
satellite.variable.number is an **error**, and the error names `.digits()` and `.size()`:

    satellite.variable.number has no method length — length() counts the items in a
    container and a number holds none; use .digits() for its decimal digits, or
    .size() for its bytes

Giving `length` a second meaning on numbers was the cheaper change and the wrong one. §17's
registry had already settled the principle when it minted `has` as a new word rather than
reusing `contains`: a word means one thing, and dispatch on the receiver is for types that
answer the *same* question differently, not for types that answer different questions. So
`digits` is its own word (86), and it counts the digits the value is **written** with, from its
most significant down to its least: `100` has three, `1230` has four, `12.5` has three, and `0`
has one because "0" is a digit. It is deliberately *not* the significand's count. Storage
normalizes `100` to `1e2` because trailing zeros are representation and not value — that is what
keeps `2.50` and `2.5` printing the same — but the number a program asks about still has three
digits, and a `.digits()` that answered `1` was reporting the representation instead of the
number. Leading zeros are padding rather than digits, so `0.001` has one.

#### The model: three numbers, and what is deliberately left out

A value costs a **node** of 40 bytes, plus a 16-byte **handle** for every slot that points at
another value, plus its **content**:

| receiver | content beyond the node |
| --- | --- |
| `bool` | none — it lives in the node |
| `number` | none in the small form; 4 bytes per limb once the magnitude is boxed |
| `string` | 2 bytes per character (§8.5) |
| `time` | none — eight bytes of nanoseconds, in the node |
| `file` | the path's bytes; the file description is the kernel's, not the program's (§8.3) |
| `list` | a handle per element, plus each element |
| `map` | two handles per entry, plus each key and value, plus the canonical key bytes and one slot each in the side index (§8.6) |
| spacesuit instance | a handle per field, plus each field |

What the model does **not** count is the allocator: `make_shared` control blocks, `std::vector`
capacity beyond its size, hash-table bucket arrays, `malloc`'s rounding. Those are real bytes,
and leaving them out is the deliberate half of the design. They are properties of the C++
runtime a build happened to use rather than of the program's data, and the packages are
`Architecture: any` — a `.size()` that moved with the pointer width would make any program that
prints one unportable and any test that asserts one unfalsifiable on half the build matrix. So
the three constants are the 64-bit layout **frozen as the definition**, the same move
`sizeof(Value) == 40` already makes, and a `static_assert` in value.hpp keeps them from drifting
from the layout they were derived from.

The consequence worth stating: **`.size()` is the language's model of memory, not the
allocator's.** It is exact about what the value model owns and silent about what sits underneath
it. A process that wants its real RSS should ask the operating system.

#### Shared storage counts once, and a cycle terminates

The walk carries a set of the storage it has already counted, and that one mechanism is load
bearing twice.

**Shared storage counts once.** A list holding the same sublist twice holds one sublist, and
`b = a` shares one buffer. The useful number is the bytes that would be freed if this value went
away, and that is not the sum of the parts:

    inner.size()       152
    outer.size()       224     two handles onto one 152-byte inner, not two inners

**A cycle terminates.** §12 records the price of making objects a reference type in one
sentence — an object *can* close a shared_ptr cycle, which no previous value could. `to_string()`
sidesteps that by refusing to print an object's fields at all; `.size()` has to walk them. An
honest and slightly startling consequence, from the run that verified it: a fresh `node` whose
`next` is nil costs **96**, and after `a.link(a)` it costs **56** — the nil was a real 40-byte
node, and the self-reference is storage already counted. The number is right; "smaller after you
add a link" is just what counting each distinct piece once looks like from the outside.

Depth is **not** capped, and that matches the precedent rather than overlooking it: `ValuePrinter`
recurses over nested lists with no cap either, so a structure deep enough to break the size walk
already breaks every print of the same value. The cycle — the one shape that would never
terminate at any depth — is handled.

#### Where it sits in the dispatch, and the one place it yields

`.size()` is tested **above** the per-type tables in `call_method`, not copied into each of them.
That is what keeps the answer consistent as the language grows: the walk in value.cpp is an
exhaustive `std::visit` with no `auto` fallback, so a tenth alternative in `ValueBase` fails to
compile until `.size()` has an answer for it, and no one has to remember to add a row.

The exception is a spacesuit instance, and it falls out of §14's existing rule rather than being
a new one. An instance answers for its own members **before** any built-in table is consulted, so
a spacesuit that already defines `size()` keeps its own and `.size()` never reaches the model
above. Adding this to the language therefore cannot change what any program that compiles today
does — the same guarantee that already let a spacesuit name a method `length` or `to_string`.

`nil` is unchanged: it has no methods, so it has no `.size()` either.

---

### 8.8 The arguments object

`satellite.main`'s parameter is not a `list<string>`. It is an **arguments
object**, which *satisfies* `satellite.container.list<satellite.variable.string>`
so that §2's signature does not move, and which carries a name for every element
plus about thirty more elements the command line never had.

```satellite
satellite.capsule satellite.main(
    satellite.container.list<satellite.variable.string> arguments)
{
    satellite.console.display(arguments.cxx_compiler)
    satellite.return(satellite)
}
```

**The parameter's name is the user's.** `arguments`, `args`, `argz`,
`argument`, `argumentz`, `arg` — or `banana`. It is an ordinary frame slot and
resolve() binds it by position, so the language never reads the spelling. §1 is
why: a bare identifier names something the user owns, and a feature that fired
on six blessed spellings would be the language reaching into that. The six are
what the documentation uses; `banana` is in `eval_test` on purpose, because it
is what proves they are documentation and not a gate. **Verified.**

**`.length()` and a numeric index are the command line and nothing else.** This
is the decision the rest of the type is arranged around. Every program that
takes arguments writes

```satellite
satellite.statement.for (satellite.variable.number i = 1;
                         i < arguments.length(); i = i + 1) { ... }
```

and if `.length()` counted the machine entries, that loop would walk off the
user's arguments and start reading the kernel release as though it had been
typed. So `.length()`, `[i]`, `.first()`, `.last()` and `.contains()` all run
over the command-line half, and answer exactly what they answered before this
type existed. `.count()` is the total for anyone who wants it.

**Everything else is reached by name**, in three spellings that are one lookup:

| spelling | when |
| --- | --- |
| `a.cxx_compiler` | the name is known when the program is written |
| `a["cxx_compiler"]` | the name is computed |
| `a.get("cxx_compiler")` | the same, as a call |
| `a.has("cxx_compiler")` | ask without failing |
| `a.names()` | every name, in order, as a `list<string>` |

The bare selector is **the one place in the language where a bare name after a
dot is not a method call**, and it is a narrowing of §12's refusal by exactly
one case rather than a lifting of it. That refusal lives in `eval_member`
(`expr.cpp`), not in the parser — `a.cxx_compiler` already parsed before this
existed and merely died on evaluation — so this cost no grammar change, no new
token, and nothing in §4's reservation rule. Every other value still gets the
old message: `"abc".missing` is still refused, and `eval_test` pins that.

A method name wins over an entry name, and no entry is named after a method.
That is not an assertion — `eval_test` walks `.names()` and fails if any of the
ten method names appears, so adding an entry called `length` breaks the build
rather than silently shadowing.

**An absent name is an error**, never `""`, and the message lists every name
that does exist — the same call §8.6 makes for a missing map key, and for the
same reason: a typo that returns the empty string is a bug that surfaces three
capsules away. A fact the machine will not give up is *present* and says so in
words — `unrecorded` when the build never passed it, `unavailable` when the
system declined — so `.has()` can tell "no such name" from "no such answer
here".

**Printing it is its own form.** A list is `[a, b]` and a map is `{k: v}`; an
arguments object is one entry per line, the name padded, then the value. There
are about thirty entries and one of them is a compiler version string, so a
single line is unusable in the only place it is ever printed.

**What is in it**, in display order: the command line (`program`,
`argument_1..n`, `argument_count`); where the program is (`current_directory`,
`home_directory`, `username`, `hostname`); the system (`operating_system`,
`kernel_release`, `kernel_version`, `architecture`, `distribution`,
`distribution_id`, `distribution_version`); what built the interpreter
(`cxx_compiler`, `cxx_compiler_version`, `cxx_standard`, `cxx_flags`,
`make_version`, `standard_library`, `c_library`, `built`); the interpreter
(`interpreter`, `interpreter_version`, `library_path`, `library_path_source`);
the process (`process_id`, `parent_process_id`, `thread_count`, `page_size`,
`pointer_bits`, `byte_order`); and the session (`shell`, `terminal`,
`language`).

`$PATH` is deliberately absent: unbounded, and the likeliest of all of them to
hold something private in an object people paste into bug reports.

**In the value model** it is `ArgsRef` at variant index **10**, appended for the
reason `MapRef` is 8 and `BitsRef` is 9 — `help_for()` and `module_of()` switch
on raw variant indices, so inserting renumbers every alternative after it. A
`shared_ptr` is 16 bytes, so `sizeof(Value)` stays 40 and §8.1's `static_assert`
keeps holding. Built then frozen, exactly as a `MapBody` is, with the name index
built eagerly — a lazily built one would be a data race nothing in the suite
would catch.

**In §17** it costs two words, 109 `count` and 110 `names`, and not thirty-six:
the bare selector **lowers to `.get(name)`**, so an entry name is an operand and
never a frozen id. Spending a permanent registry id on `kernel_release` would be
the registry recording a fact about somebody's machine, which is not what it is
for.

`satellite.help(arguments)` is the whole surface, and it covers passing
arguments generally as well — parameters are positional, a parameter is a frame
slot, a spacesuit passed to a capsule is a reference while a number is a value,
and `satellite.returns(T)` is parsed and not enforced. It answers to twelve
spellings: the six names above, each bare or quoted, each optionally prefixed
`system ` or `system.`. The two with a space are quoted-only, because a bare
`system args` is two tokens and no change to §3 is worth making it one.

A program cannot construct one. There is no `satellite.variable.arguments` to
declare, and the object is read-only; `satellite.library` is where a program
records something of its own. Reading the environment in general is a
`satellite.system.environment(name)` that does not exist yet — the four session
variables above are named entries, not a mechanism.
