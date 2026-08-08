# satellite — language design and implementation plan

Status: runtime substrate, lexer, tree, parser, resolver and evaluator all landed,
including §6's frames and capsule calls. §14 adds spacesuits (classes) with
inheritance, access control and constructors, and `satellite.variable.time` with
`satellite.time.now()`. §8.1's `Number` migration is **done**: `double` is gone
from the variant and `satellite.variable.number` is an exact arbitrary-precision
decimal. §8.3's `file` is done, so a satellite program can now read and write
one, which is what §16 was waiting on. The interpreter and the GUI terminal are
two binaries, `satl` and `satl-term` (§9). Still outstanding: the window (M7),
and §16's include mechanism, which is designed and unbuilt.
Last updated: 2026-08-07

This document fixes the syntax, names the decisions still open, and lays out the build
order. Everything marked **verified** was checked by compiling and running code against
the real source in this repo, not reasoned about on paper.

---

## 1. The generating rule

One invariant produces all satellite naming:

> **A dotted path rooted at `satellite` names something the language owns.
> A bare identifier names something the user owns.**

Every other naming rule follows from it:

- Types are language-owned, so they are prefixed: `satellite.variable.time`.
- Variable names are user-chosen, so they are bare: `my_time`.
- Method names are selectors interpreted relative to what precedes them, so they are
  bare: `my_time.some_function()`.
- `satellite.main` is prefixed because the *runtime* chooses and calls that name, not
  the user. A capsule the user writes is bare: `fact(3)`.

The earlier phrasing — "everything the language provides has `satellite` in front of it" —
is not quite the real rule, because `some_function` and `now` are language-provided and
bare. The invariant above is the version that holds everywhere.

`satellite` is the **only reserved word in the language**. Nothing else needs reserving:
`variable`, `library`, `time`, `file`, `list` are special only in the second position of a
satellite-rooted path, so a user may freely name a variable `time`. Declaring anything
named `satellite` is a hard error, checked at the binding site.

---

## 2. Hello world

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> argz)
{
    satellite.console.display("hello, world!")

    satellite.return(satellite)
}
```

- **capsule** = function.
- **`satellite`** as a value is the *singleton runtime object*, not a zero sentinel. That
  is why `satellite.include(satellite)` and `satellite.return(satellite)` can use the same
  word to mean sensible things: include the runtime, return the runtime (i.e. success).
- **`satellite.console.display`** is ordinary stdout. The child process writes bytes to the
  PTY and VTE renders them. Implementation is `decode()` + `fwrite` + `fflush` — the flush
  is required because glibc line-buffers stdout on a tty. Nothing else is involved: no GTK,
  no IPC. **Verified**: "hello, world!" round-trips through `encode`/`decode` byte-identically.
- **`satellite.include`** is ceremony in v1: parsed, validated, ignored. Say so honestly in
  the docs rather than implying that including a file does anything. §16 designs what it
  will mean and is not implemented.

---

## 3. Lexical structure

The lexer walks `SatString`, so the code table in `satellite_string.hpp` *is* the language's
alphabet. Five rules are load-bearing and each fixes a verified defect in the original plan.

### 3.1 Underscore is an identifier character

**Verified blocker.** `_` is punctuation code 74 in the table (`satellite_string.cpp:9`,
index 11 of `PUNCT`). Under the naive rule, `encode("my_time")` = `13 25 74 20 9 13 5` and
`my_time` lexes as **three tokens**. Every example in this document breaks. The slicing
example `list_name[some_number_start:some_number_end]` degrades from 6 tokens to 13.

```cpp
constexpr SatChar SAT_UNDERSCORE = SAT_PUNCT_BASE + 11;   // 74
bool word_start(SatChar c) { return (c >= SAT_A && c < SAT_DIGIT_0) || c == SAT_UNDERSCORE; }
bool word_cont (SatChar c) { return (c >= SAT_A && c < SAT_PUNCT_BASE) || c == SAT_UNDERSCORE; }
```

Add the named constant to `satellite_string.hpp` next to the existing enum rather than a
bare `74`.

### 3.2 Whitespace lives in the raw area

**Verified.** Space, tab, newline have no code-table entry, so they arrive as
`SAT_RAW_BASE + byte`. A lexer that calls `isspace()` on a `SatChar` or compares against
`' '` skips nothing and emits one Error token per space.

```cpp
bool is_space(SatChar c) {
    return c == SAT_RAW_BASE + ' '  || c == SAT_RAW_BASE + '\t'
        || c == SAT_RAW_BASE + '\r' || c == SAT_RAW_BASE + '\n';
}
```

### 3.3 Lex with `encode_raw`, not `encode`

**Verified.** `encode()` expands backslash escapes *everywhere*, so it corrupts source text
before the lexer sees it. Add `SatString encode_raw(const std::string &)` — the same loop
minus the escape branch, about 10 lines. Lex with `encode_raw()`; apply escape-expanding
`encode()` only to the *body* of a string literal when building the String token. The
existing `encode()` and its passing tests stay untouched.

### 3.4 String literals need real escapes

**Verified.** The escape table is only `{home,user,threads,memtotal,memused,cwd,void}`.
There is no `\n`, `\t`, `\\` or `\"`, so `"a\nb"` decodes to the two literal characters
`\n`, and `"C:\home\x"` silently becomes `C:/home/madness\x`.

Add `\n \t \r \\ \"` mapping to `SAT_RAW_BASE + byte`, which round-trips unchanged.
Match escape names longest-first, and check `\\` *before* the named escapes so `"C:\\home"`
is writable at all.

### 3.5 `<` and `>` are always single-character tokens

satellite has no `<<` or `>>` operator, ever. This is why nested generics need no special
handling: `list<list<string>>` lexes as two independent `>` tokens and `parse_type`'s
recursion consumes one per level. **The C++98 maximal-munch bug cannot occur.** Make it
written policy rather than an accident of the operator list. Bit shifts, if ever needed,
are `satellite.number.shift_left(n)`, consistent with §1.

The greedy two-character operators are exactly `== <= >= !=`. `>=` is the only one that can
collide with a generic close; add a ten-line `close_generic()` guard now.

### 3.6 Other lexer rules

- **Never fold a sign into a Number.** `-1` is always `Punct(-) Number(1)`. Folding would
  turn `a-1` into `Word(a) Number(-1)` and break subtraction. Unary minus is an expression rule.
- A `.` joins a Number only when the token started with a digit *and* a digit follows, so
  `3.14` is one Number but `main.x` is `Word . Word`.
- Tokens carry `start`/`end` SatChar offsets **and a `line`**. All three are load-bearing —
  offsets for error spans, `line` for the postfix-`[` rule in §7.
- The lexer never throws. It emits an `Error` token carrying a position.

---

## 4. The reservation rule (what makes generics parse)

> **`satellite.variable.*` and `satellite.container.*` are type namespaces.
> A path in either namespace is never a value expression.**

This rule is not optional. Without it the design has a genuine ambiguity, **verified with a
prototype lexer**: the parameter

```
satellite.container.list<satellite.variable.string> argz
```

produces a token stream *identical* to the chained comparison
`((satellite.container.list < satellite.variable.string) > argz)`. That is exactly the
C++98 `a<b>c` problem, and no amount of lookahead resolves it, because the distinguishing
fact ("is this path a type or a value?") is semantic, not syntactic.

With the rule, type context and expression context are disjoint by construction:
`parse_type` is entered only where a type is grammatically required, so `<` inside it is
always a generic opener; `parse_expression` never calls `parse_type`, so `<` there is always
less-than. `at_type()` becomes a 3-token test and the grammar is **LL(3)**.

Corollary: user-defined generics must not use bare names, since a bare name can be a value.
Defer user generics.

### A nice consequence

Because a type is always syntactically distinguishable by its `satellite.` prefix, satellite
**cannot have C++'s "most vexing parse."** The declaration-vs-expression ambiguity that has
plagued C and C++ for forty years arises precisely because a type name and a variable name
are both bare identifiers. Here they never are. This is a real, underappreciated strength of
the prefix rule.

---

## 5. Grammar (v1)

```ebnf
program        := { top_level }
top_level      := include_decl | capsule_decl | global_decl

include_decl   := "satellite" "." "include" "(" expression ")"

capsule_decl   := "satellite" "." "capsule" capsule_name
                  "(" [ param_list ] ")" [ returns_clause ] block
capsule_name   := IDENT                        // user capsule, bare
                | "satellite" "." IDENT        // reserved; currently only `main`
returns_clause := "satellite" "." "returns" "(" type ")"
param_list     := param { "," param }
param          := type IDENT                   // prefixed type, bare name

type           := "satellite" "." type_space "." IDENT [ "<" type { "," type } ">" ]
                | "satellite"                  // the singleton runtime type
type_space     := "variable" | "container"

block          := "{" { statement } "}"
statement      := var_decl | assign | expr_stmt | return_stmt | block
                | if_stmt | while_stmt | for_stmt
var_decl       := type IDENT [ "=" expression ]
return_stmt    := "satellite" "." "return" "(" [ expression ] ")"

stmt_kw        := "satellite" "." "statement" "."
if_stmt        := stmt_kw "if" "(" expression ")" block
                  [ stmt_kw "else" ( block | if_stmt ) ]
while_stmt     := stmt_kw "while" "(" expression ")" block
for_stmt       := stmt_kw "for" "(" [ var_decl | assign ] ";" [ expression ] ";"
                  [ assign ] ")" block

expression     := ... precedence climbing ...
postfix        := primary { "." IDENT | "(" [ args ] ")" | "[" subscript "]" }
subscript      := expression | [ expression ] ":" [ expression ]
primary        := NUMBER | STRING | "satellite" | IDENT | "(" expression ")"
```

### Statement dispatch is on segment 1, not on shape

**Verified.** The signal that a type path ended and a name began is *two adjacent Word
tokens with nothing between them*. Whitespace is load-bearing without being a token:
`satellite.variable.timemy_time` glues into a single `Word(timemy_time)`, while spaces
around the dots are free — `satellite . variable . time my_time` is byte-identical.

But a purely **structural** rule ("dotted path followed by a bare Word = declaration") is
not safe, and the collision comes from §1's own unifying rule. **Verified**:

```
satellite.control.return my_time
  -> Word(satellite) Punct(.) Word(control) Punct(.) Word(return) Word(my_time)
satellite.variable.time   my_time
  -> Word(satellite) Punct(.) Word(variable) Punct(.) Word(time)   Word(my_time)
```

Identical in shape. A structural rule silently declares a variable named `my_time` of type
`satellite.control.return`. Every statement form shaped `satellite.<x>.<y> <operand>`
collides this way. So does `main.x foo = 5`, which a structural rule would also read as a
declaration, and no amount of lookahead fixes either.

**Dispatch on segment 1 instead** — one string compare, still LL(1):

| `path[1]` | meaning |
|---|---|
| `variable`, `container` | type path — a following Word is a declaration |
| `library` | variable path |
| `statement` | statement form: `if`, `else`, `while`, `for` |
| `include`, `capsule`, `spacesuit`, `return`, `returns`, `protected`, `public` | declaration and statement forms with their own parse rules (§14 added the last three) |
| anything else | module; needs `(` — but do not make that absolute, or module constants like `satellite.math.pi` become errors (§8.4's `satellite.bool.true` is the first one to exist) |

The last two rows are one rule, not two: a segment-1 word either has a parse rule of its own
or it does not, and the ones that do are exactly the forms in §5's grammar. `library` is the
odd one, dispatched by the evaluator rather than the parser, because §5's own rule below —
keep the parser resolution-free — says a variable path is a flat Member chain until eval time.

This also makes the "no keyword enum" goal *honest* rather than aspirational: the keywords
become strings in a parser dispatch table instead of an enum in the lexer, which is exactly
the intent. The lexer stays a pure `std::string -> std::vector<Token>` with no feedback edge
from a symbol table — no C-style "lexer hack," no `TryParseDeclarator`, and none of the
reclassification machinery Java needs an entire spec chapter (JLS 6.5) for.

An uninitialized declaration (`satellite.variable.file my_file`) ends in two adjacent Words,
a shape no expression can produce, so it stays unambiguous. Parameter lists extend cleanly
because commas give hard boundaries.

### The postfix loop

**Verified defect in the original sketch**: `parse_primary` then `while (peek == '.')`
cannot parse `foo().bar()` — it fails on the first token after the primary. The loop must
cover all three postfix forms over one node variable, each *replacing* the node with a wrapper:

```cpp
for (;;) {
    if (at_punct(".")) { node = member(node, ident()); continue; }
    if (at_punct("(")) { node = call(node, args());    continue; }
    if (at_punct("[")) { node = index(node, subscript()); continue; }
    return node;
}
```

This single loop is what makes chaining uniform. `Member` and `Call` are peers, so
`satellite.time.now().some_function()` and `my_list[0].f()[1:2]` both fall out with no
extra rules.

**Keep the parser resolution-free.** Do *not* teach it that `satellite.library.<fn>.<var>`
is exactly four segments — that encodes the standard library's shape into the grammar and
forces a new parse rule per namespace. Emit a flat Member/Call/Index chain and resolve at
eval time, left to right, letting each object answer for its own members.

The rule that distinguishes a call from a path is **one token**: after a dotted path, if the
next token is `(`, it is a call — the last segment is the method name, everything before it
is the receiver.

---

## 6. Scope model — the most important decision

**Verified blocker.** `Library` keys are literally `<function_name>.<var_name>`
(`library.cpp:44-48`), and `intern()` returns the same `shared_ptr<Variable>` for that key
forever (`library.cpp:22-34`). So a capsule has **exactly one slot per local for the whole
program**, with no per-call storage.

**Recursion collapses.** Simulated against the real `library.cpp`, a recursive `fact` built
on `Library::set/get` returns **1 for every input** — 1, 2, 3, 5, 10 all yield 1, because
the base case's write to `fact.n` is the last one standing. The failure is
evaluation-order-dependent: caching `n` in a C++ temporary before the recursive call makes
the same source return 6 correctly, which is the worst possible property for a bug.

**Concurrency is worse, and it is the part that would not be seen coming.** Eight threads
running a capsule with *no recursion and no shared state* produced **1585 wrong results out
of 1600**. The write lock does not help: `satellite.library` provides *atomicity* (no torn
value) but not *isolation* (private storage), and locals need isolation.

This cannot be reframed as deliberate "capsule-static" semantics, because parameters are
locals too — `argz` would be a program-wide static.

### The fix: slot-indexed call frames

`satellite.library` is reserved for **shared/global state**. Every capsule call gets its own
frame; locals resolve to integer slot indices **statically, before execution** — in a
separate `resolve()` pass (`env.hpp`), not in the parser.

The original wording here said "at parse time", which reads as a contradiction of §5's
"keep the parser resolution-free" and is not one: what matters is that a name is resolved
*once, ahead of the tree walk* rather than looked up on every read. Putting it in the parser
would not work anyway — **a capsule may call one defined further down the file, and mutual
recursion (`is_even`/`is_odd`) is unresolvable in single-pass recursive descent.** The
resolver therefore runs in three passes: capsule names first, then top-level statements,
then each body. Two properties fall out: the parser's purity survives untouched, and a
resolver bug reports `unknown variable in capsule: x` *before* anything executes instead of
producing a wrong value somewhere inside the walk.

```cpp
struct Frame {
    std::vector<ValuePtr> slots;   // no mutex, no atomic: reachable from one thread
};
```

Three things make this the clear choice:

1. **`library.hpp` / `library.cpp` need zero logic changes** — one doc comment and a
   `function_name` → `namespace_name` rename. Nothing stress-tested is discarded. The
   Library is not being demoted; it is being pointed at the data it was designed for.
   Its properties (permanent identity, cross-thread sharing, lock-free reads, atomic
   read-modify-write) are exactly what globals need and exactly the opposite of what
   locals need.
2. **Frames are faster.** 200k × `fact(15)`: **160 ms with frame slots vs 1131 ms through
   the Library — 7.1× faster**, and the only correct option. Correctness here is not a tax.
3. **It does not violate "no bytecode VM."** The tree stays the tree; a variable-reference
   node just holds an integer instead of a string.

### Rejected alternatives

- **Key mangling (`main#3.x`)** — looks cheapest, is catastrophic. `intern()` copies and
  rehashes the entire directory per new key (`library.cpp:30`), which is quadratic:
  1000 keys 54.7 ms, 4000 keys 2.6 s, **16000 keys 70.5 seconds**. There is also no
  `erase()` anywhere in the Library, so frame keys leak permanently.
- **Declaring recursion unsupported** — does not fix the concurrency failure at all
  (1585/1600 wrong with zero recursion), is not statically enforceable once lists can hold
  capsule references, and the runtime already recurses: `to_string` in `value.hpp:36-58`
  walks nested lists, and `library_test.cpp:28-31` exercises it deliberately.

Add a configurable `satellite.library.system.max_depth` (default ~10000, read exactly like
`system.min_free_mb`) so runaway recursion raises a satellite stack-overflow error instead
of segfaulting the C++ stack.

**Capsule definitions live in a separate `CapsuleTable`, not the Library.** A bare capsule
name cannot even form a legal Library key — **verified**: `set_path("fact")` is rejected,
because `normalize_path` (`library.cpp:68-76`) requires a dot on both sides. Capsules are
written once at parse time and never reassigned; they need none of the Library's machinery.

---

## 7. Method calls, indexing, slicing

### Methods are sugar — verified

```
my_list.append(x)   ===   satellite.container.list.append(my_list, x)
```

A prototype implementing exactly this over **one** table landed **16000/16000 concurrent
appends from 8 threads with no lost write**. Receiver methods and module functions share one
table, so methods cost almost nothing once module functions exist.

Three qualifications:

1. The module path is **not derivable** from the type name (`satellite.container.list` has
   two segments, `satellite.time` has one). Use an explicit variant-index → module-path
   table, not string derivation.
2. Constructors (`satellite.time.now`) live in the same table and must carry a
   "does the first parameter bind the receiver" tag, or `my_file.new()` degrades into a
   confusing arity error.
3. `nil` has no module, and there are **two** non-dispatchable states that need different
   messages: an undeclared variable (`Library::get()` returns `nullptr`, `library.cpp:65`)
   and a declared variable holding `std::monostate` (returns a live pointer).

### Mutation goes through `Library::update()`

For a mutating method on a shared variable, `Library::update()` is exactly the right
primitive and already exists. But one invariant must be written above it:

> **No satellite code runs inside `update()`.**

**Verified blocker otherwise.** `std::mutex` is not recursive, so re-entering `update()` on
the same variable **deadlocks** — and receiver syntax makes this reachable from ordinary
user code (`my_list.append(my_list.size())`). Fully reduce arguments to Values *before*
entering `update()`:

```cpp
std::vector<Value> argv = eval_args(call.args, err);   // all eval() happens HERE
if (!err.empty()) return;
lib.update(ns, name, [&](const Value *cur) { return apply_method(cur, argv); });
```

A mutating method also needs a receiver that names a storage slot. Reject
`foo().append(x)` — there is nowhere to write back.

### Indexing and slicing

```
list_name[i]        single item
list_name[a:b]      half-open range
list_name[:b]  list_name[a:]  list_name[:]
```

- **Half-open (exclusive end).** `len(l[a:b]) == b - a` with no `+1`, and it maps directly
  onto `List(begin+a, begin+b)` — the half-open iterator pair C++ already gives you.
  `l[:k]` and `l[k:]` partition exactly, and an empty slice `l[3:3]` is expressible.
  Rename the docs from `some_number_start:some_number_end` to `start:stop` — "end" reads as
  inclusive and will set the wrong expectation for every reader.
- **Out-of-range index is an error; out-of-range slice clamps.** `hi < lo` yields empty.
- **Negative indices** Python-style.
- **Slicing copies pointers only.** **Verified**: a child's `use_count` goes 2 → 3 with
  pointer identity preserved. Indexing is O(1) with no copy at all. Skip a view/span type —
  it would force every `get_if<List>` site to handle two representations and would pin whole
  backing lists. Do take `if (lo == 0 && hi == len) return base_ptr;`.
- **Postfix `[` must be on the same line as what it subscripts.** With no statement
  terminators, `display(x)` followed by a line starting `[1,2,3]` parses as
  `display(x)[1,2,3]` — the JavaScript ASI bug. One comparison using the token's `line`
  field removes it permanently. Decide now; retrofitting is breaking.
- **String indexing selects satellite characters, not display characters.** **Verified**:
  `encode("hi\home!")` is 4 SatChars decoding to the 16-character `hi/home/madness!`, so
  `s[2:3]` is one unit that displays as `/home/madness`. `SAT_VOID` occupies an index and
  displays as nothing. This is the right rule — it is the only O(1), stable one — but it is
  unique to satellite and must be specified explicitly or it will be reported as a bug forever.
  Note honestly that string slicing *does* copy characters, unlike list slicing.

### Generic element types are enforced at runtime

Check at **insertion and literal construction only** — not on every assignment, and skip it
for slices, which preserve element type by construction. One recursive
`bool matches(const TypeNode&, const Value&)`, roughly 15 lines, no change to the Value
variant. Store the declared `TypeNode` alongside each binding, not inside the Value.

The argument is timing, not purity: once user code exists that relies on no checking, adding
checking is a breaking change. R2 is a gift here — every variable has an explicit declared
type, so no inference is needed.

---

## 8. Types

Each type is a C++ struct with named fields, exposed through accessor methods.

| satellite type | C++ representation | notes |
|---|---|---|
| `satellite.variable.bool` | `bool` | exists; written `satellite.bool.true` / `.false`, see §8.4 |
| `satellite.variable.number` | `Number` (arbitrary precision) | **replaces `double`** — see §8.1 |
| `satellite.variable.string` | `SatString` | exists; 32-bit code table |
| `satellite.container.list<T>` | `std::vector<ValuePtr>` | exists; children shared by pointer |
| `satellite.variable.time` | `struct Time { int64_t ns; }` | absolute instant, UTC |
| `satellite.variable.file` | `shared_ptr<FileHandle>` | reference type |
| `satellite.variable.window` | `shared_ptr<WindowHandle>` | reference type |
| `satellite` | the runtime singleton | `satellite.return(satellite)` |

### 8.1 Numbers — one type, arbitrary precision

"Technically infinite" should mean **exact arbitrary-precision decimal** (bignum significand
× 10^exp), named `satellite.variable.number`. Not rationals (denominators grow without bound
and collide with the memory watchdog), not integer-only (`1/3` → 0 is unacceptable), not double.

**`double` must leave the variant.** **Verified by doing the migration on a copy of the
project**: replacing `double` with a 32-byte
`Number { long long sig; int exp; shared_ptr<const BigInt> big; }` leaves
`sizeof(ValueBase)` unchanged at 40 bytes, and the migrated `library_test` passes 160,000
concurrent increments TSan-clean. Only four source files plus the Makefile change.

Three traps, each found only by compiling:

1. After removing `double`, `Value v = 3.14` (`main.cpp:41`, and again at `main.cpp:94`)
   does **not** fail to compile — it silently truncates to `3`, with zero warnings under
   the project's actual `-Wall -Wextra -O2`.
2. The obvious guard `Number(double) = delete;` then breaks `Number(42)`, because deleted
   overloads still participate in resolution. What works is a template constructor
   constrained to integral types **plus** deleted float overloads.
3. "Keep both `double` and `Number`" is worse than it looks: `Value v = 4096.0` silently
   selects `double` while `Value v = 42` is a hard compile error. That would split
   `satellite.library` into two numeric representations and make the watchdog quietly ignore
   any user retuning of `min_free_mb`.

Implement with **base-10^9 limbs**, not base 2^32 — decimal I/O and 10^k scaling become
trivial, which is most of a decimal type's work. Roughly 400 lines for integer-only
`+ - * / %` and decimal I/O; ~800 for full decimal.

**Staging:** ship v1 with the `Number` *type name and semantics* even if the first
implementation is int64-backed, so adding full precision later is invisible to programs
already written.

### 8.1.2 What landed, and the three traps in the flesh

**Done**, in `bignum.hpp` / `bignum.cpp`. `sizeof(ValueBase)` is unchanged at 40 bytes and
the whole suite passes, including `library_test` under TSan.

The representation is §8.1's, exactly: `long long sig; int exp; shared_ptr<const BigInt>
big`, 32 bytes, with a **null `big` as the fast path**. That split is what keeps the
migration cheap at run time — a loop counter, an index and every small literal live entirely
in the `long long`, so `i = i + 1` is an add and an overflow check with no allocation and no
atomic. Measured on the §14 benchmark: **2.79 s against the double version's 2.79 s**, which
is to say the exactness is free at this workload.

`BigInt` is base 10^9 little-endian limbs, per §8.1's reasoning: this backs a decimal type,
so decimal I/O and scaling by 10^k are most of the work, and both are limb shifts in base
10^9 against full base conversions in base 2^32. Division is long division one **decimal**
digit at a time rather than Knuth's algorithm D in base 10^9 — the inner loop runs at most
nine times, and getting nine comparisons right is a different proposition from getting
base-10^9 quotient estimation and its correction step right.

**All three traps fired.** They were predicted on paper and every one of them turned up as a
real compile error during the migration:

1. `Value v = 4096.0` in `main.cpp` — the memory watchdog's `min_free_mb` default. Under the
   double version this compiled clean and silently truncated. It is now a compile error.
2. The constrained template plus deleted float overloads is what makes that error appear
   while `Number(42)` still works. `bool` is excluded from the template so `Value v = true`
   still selects the variant's `bool`.
3. `system.cpp`'s watchdog read `get_if<double>(min_free_mb)` — precisely the site §8.1
   warned would "quietly ignore any user retuning" if both representations coexisted.

**What is and is not exact.** Addition, subtraction and multiplication are exact with no
bound on digits, and so is division that terminates — which includes every division by a
power of ten, so a count of nanoseconds becomes an exact number of seconds. Division that
does not terminate is rounded to `satellite.library.system.division_digits` significant
figures, default 34 (decimal128's precision). That is the only rounding anywhere in the
language, and it is unavoidable rather than chosen.

### 8.1.1 How a number renders

**The rule is "print the value", never "print N significant digits."** A whole number
prints whole; a fraction prints the shortest digit string that reads back as the same
number.

This is phrased as a rule about *values* on purpose, and that is what let §8.1's migration
happen underneath it: the `double` became an arbitrary-precision decimal, "print the value"
went on meaning the same thing, and programs written before it kept printing what they
printed. A rule phrased in significant digits would not have survived. The *implementation*
this section used to name did not survive it, and where it went belongs here rather than
only in a code comment. It was `std::to_chars` in `std::chars_format::fixed`, in
`value.cpp`; there is no `to_chars` anywhere in the tree today. Rendering is
`Number::to_string` in `bignum.cpp`, because how a number renders is a property of the type
rather than of the printer — which is the reason `value.cpp` no longer answers the question
at all.

**The notation boundary was restated, and it moved.** It was `[1e-6, 1e21)`, on the grounds
that outside it digits stop being informative and `1e308` in fixed notation is 309
characters. That was true of a `double`, where a 33-digit integer had no 33 informative
digits to give. An exact decimal has them, so `bignum.cpp` rewrote the boundary in the terms
the reasoning had always really been about:

> switch to scientific when the padding zeros would outnumber the information, never when
> the information itself is long.

The thresholds are 20 trailing and 5 leading zeros, which are the old bounds restated — 20
trailing zeros is `1e20`, 5 leading is `1e-6` — so every value that printed fixed before
still does. What changed is what happens above them. `1e308` is one significant digit and
308 zeros and stays scientific; `30!` is 26 significant digits and 7 zeros and prints whole:

```
265252859812191058636308480000000
```

That value is 2.65e32, comfortably outside `[1e-6, 1e21)`, so the old rule would have
rendered it `2.6525285981219105863630848e+32` — hiding the exact answer the type went to
the trouble of computing, which is the one thing an exact type must not do.

**Verified defect this fixes.** The original `snprintf(buf, sizeof buf, "%g", d)` printed
six significant digits, so `123456789` displayed as `1.23457e+08` and `2000000 + 1` as
`2e+06` — wrong values, through the only output path the language has, since
`satellite.console.display`, the REPL echo, `.to_string()` and every error message that
quotes a number all arrive at the same function. Note that neither obvious alternative
works: `%.17g` round-trips but renders `0.1` as `0.10000000000000001`, and `to_chars`'
own `general` mode picks whichever notation is *shorter*, so it returns `1.23456789e+08`
for the same number. `fixed` was the one that was exact and readable at once, which is where
the `double` era settled; `Number::to_string` inherits the requirement rather than the
function.

Exactness is shown rather than hidden: `0.1 + 0.2` renders `0.3`, because §8.1's decimal
migration made `0.3` the value that is actually there. Before it the same expression
rendered `0.30000000000000004`, which was equally the value that was actually there, in a
type that could not hold the other one. Neither answer was ever a rounding rule bolted onto
the printer, which is the point: the printer never decided anything.

### 8.2 Time

**Verified**: time cannot be a `double`. Epoch nanoseconds now is 1785988800000000000,
needing 61 bits; a double's 53-bit mantissa gives 198 ns resolution at the current epoch, so
`satellite.time.now()` called twice in quick succession can return the identical value.

`struct Time { int64_t ns; }` — 8 bytes, does not grow the variant (SatString's 32 bytes
already dominate). **Absolute instant only** in v1: nanoseconds since the Unix epoch, UTC,
no timezone stored, no instant/duration mode flag. `a.minus(b)` returns a `number` of
nanoseconds. Adding `satellite.variable.duration` later only adds overloads.

### 8.3 File and window are reference types

The immutable-Value contract **survives**, but only because it was never a claim about the
resources a Value names. The `shared_ptr` member never changes after construction, so the
Value node's bytes are immutable and the lock-free snapshot protocol is completely
unaffected. What is mutable is the OS file description behind it.

State the split explicitly rather than hedging:

> satellite has **value semantics for values** and **reference semantics for the external
> resources that `file` and `window` name**. Copying a file value copies the handle, not the file.

True value semantics is not achievable: `dup()` shares the file offset, so independent
offsets require re-`open()` by path, which fails for pipes, sockets and unlinked files.

Ship an explicit `my_file.close()` returning a status — a destructor cannot report that
`close()` failed with ENOSPC/EIO, and buffered writes commit at close. After close, other
snapshots see a closed handle and get a clean language-level error; never silently reopen.
Guard fd state with `std::atomic<int>`. RAII in the destructor stays as the backstop.

**Require accessor methods (`my_window.height()`); do not add bare field access.** R4
specified method calls, and consistency matters more than the saved parentheses here.

### 8.3.1 What landed for `file`

**Done.** `satellite.file.open(path, mode)` returns a `satellite.variable.file`, and the
value answers `.ok()`, `.path()`, `.error()`, `.read()`, `.write(s)` and `.close()`. The
handle is the `shared_ptr<FileHandle>` §8.3 specified, with `std::atomic<int>` guarding the
descriptor, an explicit `close()` reporting its own status, and RAII in the destructor as
the backstop. The window is not built, so M7 is what remains of this section.

Two decisions §8.3 did not have to make, both forced by the first line of code that opens
a file:

- **`open` is the only constructor**, and a declaration cannot make one. Opening needs a
  path and somewhere to report failure, and `satellite.variable.file f` offers neither, so
  it is `nil` — **verified** — for the same reason §14 makes a spacesuit-typed field `nil`:
  a reference may name nothing.
- **A failed open is a value, not an error.** The handle comes back holding `errno` and the
  caller asks `.ok()`. Failing at the call site instead would make "does this file exist"
  unanswerable without killing the program that asked, which is the one question a script
  most often has.

Modes are the strings `"read"`, `"write"` and `"append"` rather than flags, because the
language has no enum and wants none: the three words say at the call site what a bitmask
never does.

**§10's two Library fixes are no longer hypothetical.** They were filed as harmless "before
resources land", and a file is a resource: a top-level `satellite.variable.file` declaration
writes the handle into `satellite.library`, and reassigning that variable now destroys the
old handle — `close(fd)`, which can block for seconds on NFS or FUSE — while the write lock
is held. Neither fix has been made.

### 8.4 Writing a `bool`

`satellite.variable.bool` was in the type table from the beginning with **no way to write
either of its two values**, which made the type unreachable from source: a bool could only
arrive as the result of a comparison. `satellite.bool.true` and `satellite.bool.false` are
those two values, and they are module constants — §5's dispatch table already warned against
requiring a `(` after a module path "or module constants like `satellite.math.pi` become
errors", and these are the first ones to exist.

The spelling costs no parser rule, no lexer keyword and no second reserved word: segment-1
dispatch sends `bool` down the module path like any other name.

It cannot be `satellite.variable.bool.true`, however much that reads like the type it
belongs to, because §4 reserves `satellite.variable.*` as a **type** namespace in which no
path is ever a value expression, and that reservation is exactly what keeps `<` unambiguous.
`bool` as a module segment and `bool` as a type name are different things, and keeping them
apart is the point. The obvious alternative — bare `true` and `false` — would be the
language's second and third reserved words, and §1 has exactly one.

---

## 9. Runtime architecture

### Keep the two processes

The parent is a GTK4 app owning one VTE terminal window; the child is `satl --repl` running
inside the terminal's PTY. The interpreter lives entirely in the child. (The child was
originally the same binary re-executed; the split below made it the sibling binary instead,
which changes nothing about the architecture.)

The decisive reason to keep it is one not previously written down: **the PTY gives you the
kernel tty line discipline for free** — echo, backspace, line editing, Ctrl-C, Ctrl-D,
SIGWINCH. Collapsing to one process means reimplementing all of it on top of
`vte_terminal_feed`. Secondarily it buys crash isolation and lets the watchdog's `_exit(2)`
kill the interpreter without killing the UI.

Cost: no shared address space, and `satellite.library` is a per-process singleton living in
the interpreter process only.

### Fix: crash isolation currently buys nothing

**Still outstanding.** `on_child_exited` (`window.cpp:54-57`, where the split moved it)
destroys the window unconditionally, and since it is
the only window, that quits the application. So an interpreter crash, an uncaught error, or
the watchdog's `_exit(2)` all take the terminal — **and the entire scrollback, including the
diagnostic just printed, vanishes instantly.**

Feed a message instead and offer to respawn:

```cpp
vte_terminal_feed(term, "\r\n[satellite exited: status N — press any key to restart]\r\n", -1);
```

About ten lines, and it is what converts "we have two processes" from a fact into a benefit.

### Fix: there is no way to run a program file — **done**

**Verified blocker, since fixed.** The single binary routed only `--repl`; anything else fell
through to `g_application_run` with the user's argv, so invoking it on a file printed
`GLib-GIO-CRITICAL: This application can not open files` and exited 1. **The hello world in
§2 had no way to be invoked at all.**

The fix was to route arguments *before* constructing the GtkApplication and never hand user
argv to GApplication. What the two binaries route today, in full:

| invocation | behavior |
|---|---|
| `satl` | REPL on stdin/stdout |
| `satl --repl` | the same, spelled out |
| `satl --run <file> [args]` | execute headless on stdout |
| `satl <file> [args]` | alias for `--run` |
| `satl --where` | print the resolved library directory and which tier answered |
| `satl-term` | GUI terminal, spawns `satl --repl` into its PTY |
| `satl-term <file> [args]` | GUI terminal spawning `satl --run <file> [args]` |

The split below is why `satl` alone is the REPL rather than the window: the GUI moved to its
own binary, so the interpreter no longer has a window to open and the `--window <file>` flag
this section originally proposed became `satl-term <file>` instead.

`satl-term` is the only one of the two that calls `g_application_run`, and it passes a
synthetic argv: `char *gtk_argv[] = { argv[0], nullptr };`

The headless `--run` path is also what makes the interpreter testable in the Makefile
without a display server.

The REPL needs the same door from the inside, because the GUI window has no shell behind
it: a file typed at the prompt is otherwise unrunnable without closing the window.

| prompt line | behavior |
|---|---|
| `run <file> [args]` | `interp::run_file` — same entry point as `satl --run` |
| `interpret <file> [args]` | alias |
| `--run <file> [args]` | alias, so the shell spelling works at the prompt |

`parse_run_command` (in `interp.*`, not `main.cpp`, so it is testable without gtk) splits
the line: quotes group a path containing spaces, and a **backslash is an ordinary
character** — `C:\home\a.satl` names the file the user typed. This is the `encode_raw` rule
of §3.3 again: a path is data, not source text. A verb with nothing after it prints usage
rather than reaching the parser, and a nonzero status is printed, since a REPL has no exit
status to carry a failure out to.

### The interpreter must not link the GUI

**Verified, and it was costing 90% of startup.** The one binary linked gtk4 and vte, so the
dynamic linker loaded **119 shared objects** — pango, harfbuzz, cairo, gdk-pixbuf and a
hundred more — before `main()` ran, on every invocation. `satl --run` touches none of
them.

```
the interpreter linked against gtk4                 25.9 ms
the same interpreter, not linked against gtk4        2.5 ms
a bare int main(){return 0;}                         2.2 ms
```

The interpreter's own share of hello world — read the file, lex, parse, resolve, walk, print
— is **0.3 ms**. The other 23.4 ms was the loader, for libraries the run never called into.

**Resolution: two binaries.** `satl` is the interpreter and links no GUI (6 shared objects,
five of them the C and C++ runtimes and the sixth the loader); `satl-term` is the window
(119). §9's two-process architecture is what makes the split nearly free — the window never
interpreted anything anyway, it spawns a child into the terminal's PTY and renders its bytes,
so the only change is that it spawns the sibling `satl` rather than itself.

What this is worth, against CPython, both timed whole-process with fork + execvp:

| | before the split | after |
|---|---|---|
| startup (74-line feature tour) | 1.3× slower than python3 | **5.8× faster** |
| peak RSS | 26.3 MB | 4.0 MB (python3: 9.9 MB) |

The "after" column is §14's table rather than a second measurement of the same thing: both
come from `make python`. This row read 7.6× against §14's 5.8× for one benchmark under one
name, which is what one number written in two places does. The "before" column cannot be
re-measured, because the binary it describes no longer exists.

Execution is unaffected and unflattered: see §14's note on where satellite actually sits.

### The commands are `satl` and `satl-term`, and the reason is not ours

The binaries were called `satellite` and `satellite-term` until packaging made that
impossible. **Ubuntu's archive already ships `satellite-gtk`, a GNSS data viewer, and it owns
`/usr/bin/satellite`.** Two packages that install the same path conflict, so shipping ours
under that name would need `Conflicts:`/`Replaces:` against an unrelated program — a Debian
Policy violation dressed up as a declaration, and one whose effect is that installing a
programming language removes a working GNSS tool from the machine.

What moved is exactly the command name and nothing else:

| | before | now |
|---|---|---|
| binaries | `satellite`, `satellite-term` | **`satl`, `satl-term`** |
| Debian packages | `satellite`, `satellite-term` | unchanged — both names are free |
| the language | satellite | unchanged |
| the reserved word | `satellite` | unchanged |
| source extension | `.satl` | unchanged |

The keyword is the point worth being explicit about: `satellite.capsule`,
`satellite.variable.number` and every other path in this document are untouched, because §1's
generating rule is about the language, and the command name is about a filesystem that
already had an occupant. A language whose surface syntax bent to accommodate a package
archive would be the wrong trade; renaming a binary costs a `make install` line.

### Finding the installed library

`satl` searches three places for its library directory, in order, and stops at the first that
exists:

1. `$SATELLITE_PATH`, which is the development override — a working tree is not an install.
2. A path relative to `/proc/self/exe`, which is what makes a tarball or a build tree
   relocatable: the binary finds its own siblings without being told where it lives.
3. The compiled-in `-DSATELLITE_LIB_DIR`, baked from the Makefile's `prefix`, which is what a
   distribution package gets.

`satl --where` prints the answer **and which of the three produced it**, because those are
different diagnoses: "found it next to the binary" and "fell through to the compiled-in
default" describe two different broken installs, and the path alone does not say which
happened. It exists to make an install falsifiable without running a program. §16's search
order for included files starts from this directory as its last tier.

### Windows

**The plan that stood here assumed one binary, and the split above deleted its foundation.**
It had the interpreter call `gtk_init_check()` on its own real main thread in `--repl`/`--run`
mode, run `g_main_loop_run()` there, put the tree walk on a worker thread and marshal window
operations back with `g_idle_add` — on the stated grounds that this cost "zero build changes:
the binary already links gtk4." That clause is what expired. `satl` links six shared objects
and not one of them is GTK, so the `gtk_init_check()` call site the plan is written around
does not exist, and creating it means paying back the 23.4 ms this section has just finished
removing.

**§16's native modules is the live plan for M7**, and it inverts this one: GTK arrives in a
shared object that the runtime `dlopen`s when a program executes
`satellite.include(satellite.window)`, so a program that never includes it never loads it.
That keeps the 23.4 ms instead of spending it, which is why M7 is now filed behind §16 rather
than in front of it. §16 has the mechanism. What survives here are two rules that outlive the
change, because neither was ever about where GTK is linked.

1. **Whatever opens a satellite program's windows must not construct a `GtkApplication`.**
   Registering the app id `org.satellite.terminal` is how a process says "I am the terminal",
   and whichever way the race falls the interpreter loses: it either becomes a remote whose
   `activate` makes some other process build *another* terminal and spawn *another* child — a
   recursive spawn loop, not a window — or it becomes the process that answers for the id. A
   `dlopen`ed shim wants bare `gtk_init_check()` + `gtk_window_new()` for precisely the reason
   the child was going to.

   `satl-term` itself passes `G_APPLICATION_NON_UNIQUE`, so it is not single-instance
   across invocations. Single-instance is wrong for it: the file to run is parsed into the
   *remote's* globals and `activate` runs in the *primary*, which reads its own empty copy,
   so `satl-term prog.satl` with a window already open opened a second REPL and dropped
   the file. One process per invocation also hands the interpreter the directory the command
   was typed in, which is what makes a relative path resolve.
2. **A main loop needs a main thread that is not blocked, and the REPL's is.** `satl` reads
   its prompt with `getchar()` (`main.cpp:171`), so nothing is pumping a GMainContext and a
   window created from that thread would never be drawn. Neither this section nor §16 has
   settled what M7 does about that, and it is the first thing the shim will run into.

---

## 10. Evaluator

Tree-walking, per R5. No bytecode VM. Three findings carry hard measurements.

### Split `Expr` from `Value`

The `value.hpp` comment proposes folding AST node kinds into the same variant. **Drop it,
and keep the spirit.** Measured: adding the AST kinds grows every runtime `Value` from
**40 to 96 bytes** — a 1M-number list goes 38 MB → 91 MB. Under R2 there is also no
`satellite.variable.<something>` for "call node", so the type checker would need arms
meaning "unreachable".

Use a separate `Expr` variant that deliberately mirrors `Value`'s idiom. Recover full
homoiconicity later via **one** new alternative rather than fifteen:

```cpp
struct Quoted { ProgramPtr prog; const Expr *node; };   // satellite.variable.expression
```

That is the whole of the `satellite_expression` idea, at zero cost today. A prototype of the
split runs hello world end to end.

### `satellite.return` is a status enum, not an exception

**Measured: 1537 ns per return as a C++ exception vs 8.5 ns for a `Flow` enum — 181×.**
At one return per call that is seconds of pure unwinding in any recursive program. Reserve
exceptions for genuine errors.

### Caret positions must never come from `decode()`

**Verified**: `decode()` is not injective (`\home` is 1 SatChar, 5 source bytes, 13 display
bytes) and **not even stable** — `decode({SAT_CWD})` returned 97 bytes, then 1 byte after a
`chdir`, for the same SatChar. Produce a `byte_of[]` side table during `encode_raw` mapping
each SatChar index back to its source byte offset, and drive carets from that.

### Fix `to_string` before adding any type — **done**

**Verified blocker, since fixed.** `to_string` tested four alternatives with `get_if` then fell
through to an unconditional `return decode(std::get<SatString>(v));`. Adding a `Time`
alternative compiled with **zero warnings** under the project's own flags and then threw
`std::get: wrong index for variant` on first print. The REPL's `:get` and `:vars` both call
it, so the first `:get main.my_time` would have crashed the interpreter.
v1 adds four to six alternatives, so this would have fired four to six times.

`to_string` is now `std::visit` over an exhaustive overload set — one `operator()` per
alternative, **no generic `auto` fallback**, since a fallback silently reintroduces the bug.
It was the highest-value twenty lines in the codebase, and it paid: `Time`, `Number`,
`FilePtr` and `ObjectPtr` all arrived afterwards, and each one was a compile error in
`ValuePrinter` rather than an archaeological dig through a crash.

### Two more Library fixes, filed "before resources land" — resources have landed

**Neither is done, and §8.3.1's `file` is what makes them live.** They were cheap when every
Value was a number, a string, a bool or a list; a `satellite.variable.file` in
`satellite.library` is a real descriptor closed under a real lock.

- **No Value destructor may run while a write lock is held.** **Verified**: both
  `set_key` (`library.cpp:36-42`) and `update` (`library.cpp:50-59`) destroy the previous
  Value under the lock. Harmless for a `double`; with a file it means `close(fd)` under the
  lock, which can block for seconds on NFS/FUSE; with a window it means GTK teardown from a
  non-GTK thread, which is undefined behavior. In `update`, declare `ValuePtr doomed;`
  *before* `var`, scope the lock inward, and end with `doomed = std::move(current);`. In
  `set_key`, use `exchange()` and let the old pointer die after the lock.
- **Tighten `normalize_path`.** It silently accepts >2-segment paths and fabricates a
  garbage variable rather than erroring. After stripping the prefix, require
  `key.find('.') == key.rfind('.')`. Three lines, and worth doing now while the only caller
  is the REPL.

### The `main.x` collision

`normalize_path` makes the `satellite.library.` prefix optional, so `main.x` and
`satellite.library.main.x` are literally the same variable — **verified: identical pointer**.
But §7 needs `main.x` to mean "receiver `main`, member `x`". Two rules claim one token stream.

**Resolution:** the bare two-segment form is illegal in program source. Source text requires
the full `satellite.library.main.x`. Keep `normalize_path`'s prefix-optional behavior scoped
to the REPL `:get`/`:set` debug commands, which `main.cpp:61-62` already documents as
temporary. That is where they still are, and they are still the only callers.
Once the parser exists, stop feeding `get_path`/`set_path` raw dotted strings from user code
entirely — the evaluator should walk the resolved chain and call the typed
`Library::get/set/update` with two already-separated segments.

---

## 11. Build order

Do the **prerequisite fixes** first — they are cheap now and expensive later:

- `to_string` → `std::visit` (§10)
- argv routing + `--run` (§9)
- `encode_raw` + string escapes (§3.3, §3.4)
- destructor-outside-lock + `normalize_path` (§10)
- `on_child_exited` keeps the window (§9)

Then extract `std::string eval_line(const std::string &)` into `interp.cpp`, so every later
test drives the full pipeline **without linking gtk4/vte**.

### M0 — "the spine"

```
satellite.variable.number x = 1
x                    -> 1
x.plus(1)            -> 2
:vars                -> satellite.library.main.x = 1
```

That last line is the point: it proves the evaluator wrote into the real Library rather than
a side table, exercising lexer → parser → AST → eval → Library → to_string → REPL.

**Use only the types that map 1:1 onto existing variant alternatives** — `number`, `string`,
`bool`, `list`. Do *not* start from the §2 examples: `time` and `file` have no representation
yet, so `satellite.time.now()` has nothing to return.

### Milestones

| # | goal | new files | done when | status |
|---|---|---|---|---|
| M1 | lexer | `lexer.*`, `lexer_test.cpp` | `my_time` is one Word; spaces produce no Error tokens; `>>` is two tokens | **done** |
| M2a | syntax tree | `ast.*`, `ast_test.cpp` | every form in §5 unparses to canonical source | **done** |
| M2b | parser | `parser.*`, `parser_test.cpp` | `unparse(parse(src)) == src` for every form in §5 | **done** |
| M0 | the spine above | `eval.*`, `interp.*`, `eval_test.cpp` | `x.plus(1)` prints 2 in the REPL | **done** |
| M3 | frames + capsules | `env.*`, `env_test.cpp` | recursive `fact(10)` = 3628800; 8 threads × 200 calls all correct | **done** |
| M4 | generics, indexing, slicing | — | half-open bounds, clamping, pointer-sharing verified by `use_count` | **done** |
| M8 | spacesuits (§14) | `spacesuit_test.cpp` | `my_object.my_func()` mutates a field; override reached from an inherited method; `sizeof(ValueBase)` still 40 | **done** |
| M5 | `Number` migration | `bignum.*`, `bignum_test.cpp` | `double` gone from the variant; `library_test` still TSan-clean | **done** |
| M6 | `file` (+ `time` **done**, §8.2) | none — `FileHandle` in `value.*`, tests in `eval_test.cpp` | `satellite.file.open` round-trips a file through `.write`/`.read`/`.close`; a failed open answers `.ok()` instead of raising | **done** |
| M7 | window — §16's first native module | a `dlopen`ed shim | `satellite.include(satellite.window)` opens a window **and** `ldd satl` still lists six objects | |

M8 is numbered after M7 and listed before M5 because it landed out of order: it
needs frames (M3) and nothing else, and `satellite.variable.time` came with it
because a timer written in satellite is what measures §14's benchmark.

M6's `handle.*` was never written. `FileHandle` is eleven lines of struct and went
next to the variant it is an alternative of, in `value.hpp`/`value.cpp`, with the
`open` and method dispatch in `eval.cpp` beside every other module function. A
file did not earn a translation unit, and splitting one out would have separated
the handle from the `to_string` overload that has to know about it — which is
§10's whole argument for `std::visit`, one level down. The original done-when for
M6 (`--run hello.satl` prints hello, world!) tested §9's argv routing rather than
files at all, and was already satisfied before M6 started; the row above states
what actually had to work.

The lexer and tree came before the spine because both are testable without an
evaluator, and the tree is what the spine's `eval` walks.

Of the prerequisite fixes above, four are done: `encode_raw` and the string
escapes (which the lexer needed), `to_string` → `std::visit`, which M0 needed
because the REPL echoes every value through it, and **argv routing + `--run`**,
which the §9 table now implements in full. The §2 hello world runs:

```
$ satl --run hello.satl 'C:\home'
hello, world!
```

Still outstanding:

- **destructor-outside-lock + `normalize_path`** (§10) — this was the cheap one,
  and it stopped being hypothetical when M6 landed. It was harmless while values
  were numbers, strings, bools and lists; a `file` in `satellite.library` means
  `close(fd)` under a write lock today, and M7's window will mean GTK teardown
  from a non-GTK thread.
- **`on_child_exited` keeps the window** (§9).

### The entry point is not a general capsule call

`satellite.main` is invoked by the runtime with its parameter bound into the
`main` namespace, which looks like the capsule-static model §6 rejects and is
not. Both failures §6 measured need something the entry point cannot have: the
recursion collapse needs a second activation of the same capsule, and the
1585/1600 concurrency failure needs two threads inside one capsule.
`satellite.main` runs once, on one thread, before any user capsule can run at
all — so with exactly one activation, binding the parameter is observationally
identical to giving it a frame. M3 replaces it with a real frame and no
satellite source changes. General capsule calls remain an error until then.

**`argz` is built with `encode_raw`, never `encode`.** An argument arrives
already escaped by the shell, so expanding backslash names a second time
rewrites it — verified: `encode()` turns `--path=C:\home` into
`--path=C:/home/madness`, silently. This is §3.3's defect in its second home.

Each milestone ends in a standalone PASS/FAIL binary in the style of
`satellite_string_test.cpp`, with its own Makefile rule and an entry in `test:` and `clean:`
— except where a milestone adds no new file to test, which is M4 and M6. Both extended the
evaluator rather than adding a component, so both extended `eval_test.cpp` rather than
standing up an eleventh binary that would have linked the same objects to say the same thing.

---

## 12. Deliberately deferred

Everything below is still deferred, and two things that were absent from the language this
morning are deliberately not on it: **file I/O**, which is §8.3.1 and M6, and **a way to write
a `bool`**, which is §8.4. Neither was ever listed here — file I/O was tracked as a milestone,
and the bool literals were tracked nowhere at all, which is how `satellite.variable.bool` sat
in §8's table from the first draft with no way to name either of its two values. A deferral
list is only useful if the things missing from the language are on it.

- **User-defined generics** — a bare name can be a value, which reopens §4's ambiguity.
- **Including a file** — `satellite.include` is still parsed, validated and ignored, exactly
  as §2 says. It is no longer deferred for want of a design: §16 is the design, and its
  prerequisite (file I/O) has landed. What blocks it now is that nobody has written it, and
  that the **word** for the thing being included is undecided — see §16's first paragraph
  before using any name for it.
- **Durations** — `time` is an absolute instant only; `a.minus(b)` returns a number of nanoseconds.
- **Bare field access** (`my_window.height`, `my_object.my_str`) — accessor methods only.
  §14 keeps this: a spacesuit field is reachable from inside the spacesuit and nowhere else,
  which is what makes `satellite.protected` a statement about the language rather than a
  comment.
- **`<<` / `>>` operators** — permanently, per §3.5.
- **A static type checker** — types are checked at runtime, at declaration and insertion.
- **Garbage collection** — **this claim no longer holds and §14 is what broke it.** The
  original reasoning was that `shared_ptr` refcounting is sufficient because cycles cannot be
  constructed: every Value was immutable, so no Value could be made to point at something
  that points back. A spacesuit instance is mutable, so two objects can now name each other
  and neither is ever freed. This is a real leak, it is the price of reference semantics
  (§14), and it is not fixable by being careful — a cycle is a shape a correct program can
  want. Revisit when object graphs get large enough to matter; the cheap partial answer is a
  weak-reference field, the complete one is a tracing collector over the object table.
- **Passing constructor arguments to a superclass** — a `super(...)` form. Today every
  constructor above the most derived one must take none, which resolve() reports (§14).
- **User-defined operators and a spacesuit `to_string` the printer consults** — an instance
  prints as `<my_class_name>`, and a spacesuit that wants more says so with a method that
  the caller calls.

---

## 13. Decisions, and the one still open

This section held three genuine forks that needed an answer before M0. **M0 shipped, and each
was answered the way it was recommended here.** They are recorded rather than deleted, because
a design document that quietly loses the questions it once had cannot be trusted about the
ones it still has.

1. **Return type syntax** — optional `satellite.returns(TYPE)` after the parameter list,
   defaulting to the `satellite` type, which leaves hello world byte-identical. It is in §5's
   grammar and in `parse_signature`, and a constructor is the one member forbidden to declare
   one, since what a constructor produces is the object (§14).
2. **User capsules are bare** — `fact(3)`, with `satellite.main` the reserved exception, which
   is §1's invariant read straight.
3. **Statements are newline-terminated**, so §7's same-line rule for postfix `[` is load
   bearing rather than a nicety, and §14 reuses it for the bare spacesuit type.

The one genuinely open question in this document is not here, because it is not about syntax:
**§16's name for a unit of includable code.**

### Resolved

**Control flow** is `satellite.statement.if(...)`, `satellite.statement.while(...)` and
`satellite.statement.for(...)`, each followed by a brace-delimited block. `statement` joins
`variable`, `container` and `library` as a segment-1 dispatch key, so §1's rule holds with no
exceptions anywhere in the language.

**Classes are `satellite.spacesuit`**, with `satellite.protected` / `satellite.public` blocks
and a bare-name type. See §14.

`for` is three-part and C-shaped —
`for(satellite.variable.number i = 0; i < 10; i = i + 1)` — which introduces no syntax the
language did not already have: `;` is an ordinary punctuation code, the init clause is the
same `VarDecl` a declaration statement produces, and the step clause is the same `Assign`.
All three parts are optional, so `for(;;)` is the infinite loop.

**`satellite.container.list` stays.** `variable` and `container` are both type namespaces
and §4's reservation rule covers both. Two type namespaces cost the parser one extra string
comparison in `at_type()`, and `container` earns it by saying something `variable` cannot:
this type holds other things and takes a generic parameter. A scalar and an aggregate
reading differently at a glance is the point, not an inconsistency.

---

## 14. Spacesuits (classes)

```satellite
satellite.spacesuit my_class_name(superclass)
{
    satellite.protected
    {
        satellite.variable.string my_str = "some_str"
    }
    satellite.public
    {
        my_class_name(satellite.variable.string input_str)
        {
            my_str = input_str
        }

        satellite.capsule my_func(satellite.variable.string s)
        {
            my_str = s
        }
    }
}

my_class_name my_object_of_class("some_data")
my_object_of_class.my_func("another_str")
```

- **`spacesuit`** = class. It is what a capsule's contents travel in, it is worn by one
  occupant at a time, and it is the thing with an inside and an outside — which is the whole
  of `satellite.protected` and `satellite.public`.
- The parenthesised name is the **superclass**, not a parameter list. A spacesuit is
  constructed by declaring a variable of its type, so it has nowhere to take arguments and
  needs no signature; the parens are free to mean the one thing a class declaration does need
  to say. Empty parens mean no superclass.
- **Every member sits in an access block.** Requiring that rather than defaulting an
  unannotated member keeps one canonical spelling, so `unparse` has one form to emit and a
  reader never has to remember which way the default goes.

### The type is bare, and that does not reopen §4

`my_class_name my_object` uses a **bare name in type position** — the first type in the
language that is not `satellite.`-rooted. §1 requires it: the user names the class, so the
class is named bare.

§4's ambiguity is created by `<`, not by bareness: `satellite.container.list<satellite.variable.string> argz`
and the chained comparison `((list < string) > argz)` have identical token streams. A
spacesuit **takes no generic arguments**, so no `<` can follow it in type position and that
token stream cannot be produced. §12's deferral of user-defined generics is what keeps this
true, and it stays deferred for exactly this reason.

What makes the bare type *parse* is §5's own observation, reused: **two adjacent Word tokens
are a shape no expression can produce.** `at_type()` gains one arm — `Word Word` on the same
line — and the grammar stays LL(2). §5's collision is untouched, because `main.x foo` and
`satellite.control.return my_time` both have a `Punct` between the path and the name, so
neither reaches the new arm. The same-line requirement is the same one an ordinary
declaration already carries: without it, a bare name at the end of one line silently swallows
the name that starts the next.

### Objects are a reference type — and §8.3 already decided that

The immutable-Value contract **survives**, for the reason §8.3 gave for `file` and `window`:
it was never a claim about what a Value *names*. The `ObjectPtr` inside the Value never
changes after construction, so the Value node's bytes stay immutable and the lock-free
snapshot protocol is completely unaffected. What is mutable is the object.

Value semantics was rejected because it does not survive the first method. `my_object.my_func()`
mutating a field would have to write back through the receiver's storage slot the way
`.append()` does, and that works only while the receiver *names* a slot — an object inside a
list, or passed as an argument, has nowhere to write back to. Reference semantics is what
"class" means everywhere else, and it is the only reading under which the example above does
what it looks like it does.

**The cost is stated in §12 and is real: objects are the first values that can close a
`shared_ptr` cycle, and a cycle leaks.** That was previously impossible and the deferral of
garbage collection rested on its being impossible.

Equality is identity, and needs no code: the variant's own `operator==` compares the
`ObjectPtr`s. Two instances with equal fields are two instances — only the spacesuit knows
which of its fields are part of what it means to be equal, so only the spacesuit can say.

### Fields are slots, methods are one lookup — §6's trade, one level down

Both tables are **flattened at resolve time**: a superclass's fields and methods are copied
into the subclass. Nothing walks a superclass chain at run time.

- **Fields** are indexed, never searched, exactly as a Frame's slots are. A superclass's
  fields are laid down **first**, so an inherited method — resolved against its own suit's
  indices — finds the same fields at the same indices in every descendant's instance. That
  prefix property *is* inheritance at the field level, and it makes it cost nothing at the
  access.
- **Methods** are one hash lookup on the **object's** suit, not the caller's. That is what
  makes an override visible to the inherited method that calls it, with no vtable and no
  chain walk.

`Name::slot` grew from three cases to five. The non-negative half is still a frame index, so
the additions are negative: `SLOT_METHOD` and `SLOT_SUIT` are sentinels, and a field carries
its own index by counting down from `SLOT_FIELD` — one int on every `Name` rather than two.

Inside a method the scope chain is: **local → field → method of this suit → capsule →
spacesuit.** A local shadows a field, and an unknown name is a resolve-time error exactly as
it is inside a capsule (§6), for a second reason as well: a name that fell through to a
global would make a typo'd field read a variable elsewhere in the program.

### Concurrency: satellite.library's protocol, one level down

An object reachable from a global is reachable from every thread that can see the global, so
a Frame's freedom from locks (§6 — reachable from exactly one thread) does not transfer. Each
field is a `std::atomic<ValuePtr>` and the object carries one `std::mutex`:

- a **read** is a lock-free atomic load, so a reader racing a writer sees the old value or
  the new one, never a torn one — `Library::get()`'s contract;
- a **write** takes the lock and publishes with one atomic store, so a plain assignment
  cannot interleave with a mutator's read-modify-write — `Library::set()`/`update()`'s.

§7's rule holds unchanged: **no satellite code runs inside the lock.** Arguments are fully
reduced before it is taken, so `my_field.append(my_field.length())` cannot deadlock.

`sizeof(ValueBase)` is **unchanged at 40 bytes** — an `ObjectPtr` is 16 and `SatString`'s 32
still dominates — so nothing about §8.1's migration budget moves.

### Construction

A declaration **is** the construction site; there is no `new`.

- `my_class_name x` builds one. `my_class_name x("data")` is **sugar**, desugared in the
  parser to `my_class_name x = my_class_name("data")`, so the resolver's arity check, the
  type check and the evaluator all see one form instead of two.
- A **field** of spacesuit type is `nil`, not a fresh instance. A variable is not part of any
  object's layout so there is no regress; a field is, and `my_class next` would otherwise
  describe an infinitely deep object. Every language with reference-typed fields makes them
  null by default for this reason. `matches()` therefore accepts `nil` for a spacesuit type:
  a reference may name nothing.
- A **constructor** is spelled with the spacesuit's own name and no `satellite.capsule` in
  front of it, because it is the one member never called by name. It declares no
  `satellite.returns` — what it produces is the object. One token of lookahead finds it: a
  field is `type name` (two Words) and a method starts with `satellite.capsule`, so a bare
  Word followed by `(` is neither.
- **Every field initialiser runs before any constructor**, superclass fields first, then the
  constructor chain from the superclass down. This is deliberately *not* C++'s interleaving
  of base construction with derived field initialisation: running every initialiser first
  means a constructor sees the whole object already at its declared values, which is the
  property the interleaved order famously fails to give.
- Only the **most derived** constructor is handed the site's arguments, so every earlier one
  must take none — checked by resolve(), reported once per spacesuit rather than once per
  construction. §12 defers the `super(...)` form that would lift this.

Access applies to constructors too: a `satellite.protected` constructor means only the suit
and its descendants may build one, which is how an abstract base is spelled. A protected
**method** is reachable from code inside a *related* spacesuit — the receiver's own, an
ancestor, or a descendant — judged on the suits rather than on the method, because after an
override the method found belongs to a subclass the caller may never have heard of.

### One lifetime problem the feature creates

`resolve()` records pointers into the Program, and an Object records a pointer into the
ResolveResult. Everything the evaluator produced used to die with the run; **an instance is
the first value that can outlive it**, because a top-level declaration writes it into the
process-global `satellite.library` and the REPL reads it back on the next line. One line is
enough to reach it:

```
satellite.spacesuit c() { satellite.public { } } c x
x            -> reads x's spacesuit, already destroyed
```

`interp.cpp` retains the image of any program that declared a spacesuit. That is a retention
rather than a leak — it is exactly what the live objects point at — but a blunt one. The
sharp version is the `ProgramPtr` `ast.hpp` already anticipates in its note about
`Quoted{ProgramPtr, const Expr *}`: an Object holding `shared_ptr<const Program>` keeps alive
only what is reachable.

### Measured against compiled C++

`example/class_bench.satl` and `example/class_bench.cpp` do the same work in the same shape —
one million constructions, one million prints, one million string transfers into one object.
Both time themselves from inside, both write to the same place. clang 24 `-O2`, x86-64:

| phase | C++ | satellite | ratio |
|---|---|---|---|
| construct 1M objects | 0.0029 s | 1.145 s | **395×** |
| 1M print statements | 0.0705 s | 0.795 s | **11×** |
| 1M string transfers | 0.0037 s | 0.853 s | **231×** |
| total | 0.0772 s | 2.794 s | **36×** |

(Timed with `satellite.variable.time` from inside the satellite program, in exact seconds —
§8.1 makes nanoseconds-to-seconds a division by a power of ten and therefore exact. The
figures are unchanged by §8.1.2's migration: the double version totalled 2.788 s.)

Read the *shape*, not the total. The print phase is 11× because both sides spend most of it
in the same place — formatting and buffering bytes — and the interpreter's overhead is
amortised against real work. The other two are 200–400× because the work per iteration is
tiny, so the tree walk is nearly all of the cost: a construction is `make_shared<Object>`
plus an atomic array plus a Frame plus an Activation, against C++'s three SSO strings and no
allocation at all.

Two things about the C++ side keep the comparison honest rather than flattering:
`sync_with_stdio(false)` and no `std::endl`, because satellite buffers all output into one
string and writes it once; and a `volatile` sink for the constructed object, because without
it clang deletes the entire construction loop and the measurement becomes a true fact about
the optimiser and a useless one about the work.

The total ratio is the one to plan against, and 36× is where an unoptimised tree walker
belongs. The costs are not mysterious and none of them is the design: a `ValuePtr` allocation
per intermediate value, a `Frame` vector per activation, and `std::visit`/`get_if` chains per
node. NaN-boxing or an arena for `Value` attack the first; neither requires a bytecode VM to
start paying off.

### Against CPython

Both are interpreters, so there is no build step on either side and nothing to argue about:
`example/py_compare/` times the whole process with fork + execvp, verifies the two programs
produce byte-identical output before timing either, and reports the wall clock. CPython's VM
boot is inside every number — 18.9 ms of it, measured separately as `python3 -c pass`.

| | python3 | satellite | |
|---|---|---|---|
| startup (74-line feature tour) | 0.0219 s | 0.0038 s | **satellite 5.8× faster** |
| execution (300k objects, prints, calls) | 0.194 s | 0.795 s | satellite 4.1× slower |
| peak RSS | 9.9 MB | 4.0 MB | |

The split is the honest summary of what a tree walker is. **Startup**: satellite's front end
costs 0.3 ms over a bare process, against CPython's 18.9 ms of VM boot and library imports,
so anything small or run-once is satellite's by a wide margin. **Execution**: CPython
compiles to bytecode once and then runs a tight dispatch loop over a flat instruction array,
while satellite re-walks the tree — a `get_if` chain per node, a `shared_ptr` allocation per
intermediate value with an *atomic* refcount, and a `std::vector` per activation. Four times
is what that costs, and the ratio is stable across an arithmetic loop (3.0×), capsule calls
(3.6×) and the object benchmark (4.1×), which is what says it is the walk itself rather than
any one operation.

None of the fixes for that require abandoning the tree: interning small numbers, a
non-atomic refcount for values that never cross a thread, and a frame arena are each worth
more than they cost and none of them is a bytecode VM. A bytecode VM is what closes the
rest, and §10's decision to stay a tree walker was made when nothing had been measured.

### The compiler's own time is part of running a C++ program

The table above measures the compiled program, which is not the same as measuring C++. Before
clang's output can run, clang has to produce it, and that step is missing from every number
above. `example/cxx_compare/` adds it: it shells out to a makefile, builds from clean, times
the build, times the run, and times the satellite program that does the same work — all from
one command, `make compare`. Mean of three, clang 24 `-O2`, x86-64:

| program | C++ compile | C++ run | C++ total | satellite | first run |
|---|---|---|---|---|---|
| hello world | 0.714 s | 0.002 s | 0.716 s | 0.002 s | **satellite 332× faster** |
| the 1M benchmark | 2.419 s | 0.076 s | 2.495 s | 2.840 s | satellite 1.14× slower |

**36× becomes 1.14× the moment the compiler is counted**, and for a small program the sign
flips entirely.

**That is not a speed measurement, and it must not be read as one.** The compile is a
CONSTANT — about 2.5 s whatever the program does — while satellite's cost scales with the
work. So the ratio is not a property of the two languages; it is the point at which a fixed
2.5 s crosses a line with a 36× slope, and the 1M benchmark happens to land near that
crossing by coincidence. Vary only the loop count and the whole apparent result moves:

| rounds | C++ compile | C++ run | C++ total | satellite | ratio |
|---|---|---|---|---|---|
| 100,000 | 2.481 s | 0.015 s | 2.496 s | 0.316 s | **0.13×** (satellite 8× faster) |
| 1,000,000 | 2.488 s | 0.083 s | 2.571 s | 2.919 s | 1.14× |
| 4,000,000 | 2.559 s | 0.300 s | 2.859 s | 11.768 s | 4.12× |

Two things collapse it back to 36×: more work, and running the program a second time — the
compile is paid once, the tree walk every time. **satellite executes ~36× slower than
compiled C++, and that is the number that describes the language.** The compile-inclusive
figure describes a WORKFLOW: small program, or run once.

Neither number is more honest than the other; they answer different questions, which is why
the harness reports the break-even instead of picking one:

> C++ pays for the compiler once and satellite pays for the tree walk every run, so the
> question is how many runs it takes for the compile to pay for itself.

The benchmark breaks even after **0.88 runs** — under one, so C++ is ahead there from the
very first run. Hello world does not break even at all, and the harness's figure for it must
not be quoted: satellite runs it in about 0.0021 s and the compiled C++ binary in about
0.0021 s, so the per-run penalty the break-even divides into is the gap between two process
startups, and that gap is noise. Three consecutive `make compare` invocations of the same two
binaries put it at 12,670 runs, then 6,653 runs, then "never — satellite runs faster than the
compiled program too", the sign having flipped. What the row really reports is that once hello
world is built the two cost the same, and every remaining thing C++ charges for it is the
compiler.

That is the shape of the trade, and it is the
same shape every interpreter has: satellite wins where the program is small or run once, and
loses wherever the work is large enough that the per-node cost dominates. Making the second
case competitive is what the optimisation list above is for. Making the first case good was
free, and it is most of what a script is.

---

## 15. Bootstrapping

The goal is not self-hosting for its own sake. It is to **delete the C++**, and the shape of
the answer follows from one fact: an interpreter written in satellite needs a satellite
interpreter to run it, so a native host survives forever. A **compiler** written in satellite
does not.

```
1. write satc (satellite -> C) in satellite; run it under the C++ implementation
2. run satc, still interpreted, on its OWN source -> satc.c -> a native binary
3. have that binary compile its own source again
   -> byte-identical output means a fixpoint, and the C++ compiler can go
```

This is how C, Go and Rust each did it; Rust's first compiler was OCaml.

**Bytecode is the one target that would prevent this**, because bytecode needs a native VM to
execute it. Emitting C source — or eventually machine code — leaves nothing of ours behind.
§10's "no bytecode, the tree stays the tree" is therefore reinforced by the bootstrap plan
rather than challenged by it. C source is the right first target for a second reason:
satellite strings can already hold arbitrary bytes (`SAT_RAW_BASE`), but emitting text needs
none of that, and a C compiler is an optimising backend nobody has to write.

### The boundary: what stays C, and why it must

| | lines | |
|---|---|---|
| **runtime — stays C** | 1,986 | `bignum`, `satellite_string`, `library`, `value`, `system` |
| **compiler — becomes satellite** | 6,061 | `lexer`, `ast`, `parser`, `env`, `eval`, `interp`, `main` |

(Headers and sources both, counted against the current tree. M6 added 166 lines to the
runtime floor and 220 to the compiler; `file` is the one item on the "cannot be written in
satellite" list that was still hypothetical when this table was first drawn.)

Each runtime piece is there because it **cannot** be written in satellite, not because nobody
got round to it:

- **bignum** — satellite's one number type *is* arbitrary-precision decimal, so implementing
  it in satellite needs something underneath it to build limbs from, and there is nothing
  underneath it. Circular.
- **satellite_string** — same argument, one level over.
- **value** — the variant every satellite value is. Circular.
- **library** — atomics, mutexes and a lock-free publish protocol, none of which satellite
  can spell.
- **system**, **file** — syscalls.

The alternative was to add a machine-word primitive visible only to runtime authors, so the
bignum could be written in satellite and the C count go to zero. **Rejected**: it puts a type
in the language that exists purely for implementation reasons, and a language with one number
type is worth more than a language with a smaller runtime. The nice consequence of the split
is that one runtime serves both consumers — the compiler runs on it, and so does every
program the compiler emits.

### Stages, and what each needs

| stage | needs | status |
|---|---|---|
| 0 — lexer in satellite | nothing | **done**, `example/bootstrap/lexer.satl` |
| 1 — parser + unparse, holding `unparse(parse(src)) == src` | nothing | |
| 2 — resolver + C code generation | a map, or accept O(n) symbol lookup | |
| 3 — self-compile to a fixpoint | §16's include mechanism; file I/O, the other prerequisite, is **done** (§8.3.1) | |

Stage 0 was written before planning the rest, on purpose: it is cheaper to find out what the
language cannot say than to predict it. What it found, in order of how much it hurt:

1. **File I/O — fixed.** Source arrived through `argz` because nothing could read a file, and
   **`satellite.include` needed it too**, since including a file means reading one, so both
   were blocked behind the same feature. §8.3 designed it and M6 built it (§8.3.1). A
   satellite program can now open the source it is asked to compile, which is what stage 1
   needs and stage 0 had to work around. Of the two things stage 3 was waiting on, this was
   the one that had a design; the other still does not have a name.
2. **`break` and `continue`.** Five loops, five `going` flags, and every exit condition has to
   be reconstructed by the reader rather than read.
3. **`&&` and `\|\|`.** §3.5's two-character operators are exactly `== <= >= !=`, so the
   logical pair has never been part of the language, and `.and()`/`.or()` chains read
   backwards: `(c >= "a").and(c <= "z").or(is_digit(c))`.
4. **A map.** Only `list` exists, so every symbol table is a linear scan — correct, but O(n)
   inside a tree walker is what decides whether self-compilation takes seconds or minutes.
5. **String to number.** A ten-way `if` chain stands in for `"3".to_number()`.

What stage 0 did **not** need is the more useful half of the result: spacesuits carry AST
nodes well, `satellite.container.list<token>` works, virtual dispatch substitutes cleanly for
pattern matching, and the exact decimals make the lexer *easier* than the C++ one — there is
no `strtod`, no precision question, and `n = n * 10 + digit` is exact by construction.

---

## 16. Including a file

### The name of the thing is undecided, and this section does not settle it

**Read this before quoting any word from this section as satellite terminology.** satellite
has no word yet for *a unit of includable code*. This matters more here than it would
elsewhere: §1 makes naming the generating rule of the whole language, so a word that ends up
in `satellite.<word>` paths is a permanent surface, not a label.

So this section does not name the thing: it says "an included file" wherever the missing word
would go. **"Module" below is not the missing word**, and must not be renamed along with it —
every occurrence of it is §5's and §7's settled sense of a namespace of language-owned
functions, which is what `satellite.time`, `satellite.file` and `satellite.window` are. That
collision is why `module` is a rejected candidate rather than the leading one, and it cuts
both ways: an instruction to treat every "module" in this section as a placeholder would
rename `satellite.window`'s namespace and the native-module mechanism along with the unit.

Three candidates have been rejected, each for a reason that is about satellite rather than
taste:

| candidate | rejected because |
|---|---|
| `module` | it is Python's word, and borrowing it imports every expectation Python attaches to it — packages, `__init__`, dotted import paths — none of which this section's flat namespace provides. It is also **already in use in this document for something else**: §5's dispatch table and §7 call a namespace of language-owned functions a module, which is what `satellite.time` and `satellite.file` are. One word for a namespace and for a file is one word too few. |
| `include` | it is already the **verb**. `satellite.include(x)` says what is done; the noun cannot be the same word, or `include an include` is the best the documentation can do. |
| `library` | `satellite.library` is the global variable registry (§6) **and** one of the keys in §5's segment-1 dispatch table. The word is spoken for twice over, and reusing it would make `satellite.library` mean two unrelated things at the same position in a path. |

**The standing proposal is `payload`** — what a capsule carries, which is the same metaphor
`capsule` and `spacesuit` already run on. It is a proposal and **has not been accepted**. Until
the user decides, nothing should be written that hardcodes any of these words into a path, a
message, a filename or a test.

### Status: designed, not built

`satellite.include` has had its syntax since §2 and none of its meaning: the parser accepts
any expression and the evaluator skips the node (`eval.cpp`, where `Include` items are
walked past deliberately). **Everything below this line is design.** §15 put it behind file
I/O, because including a file means reading one, and **file I/O has now landed** (§8.3.1), so
the prerequisite is gone and what remains is the writing — and the name.

§1 decides all three spellings without any new rule:

| form | means |
|---|---|
| `satellite.include(satellite)` | the runtime. Ceremony, and it stays ceremony (§2). |
| `satellite.include(satellite.window)` | a **language-owned** module, possibly backed by a native `.so` |
| `satellite.include(my_parser)` | a **user-owned** satellite source file |

A bare name is user-owned, so it names the user's file; a satellite-rooted path is
language-owned, so it names ours. Nothing had to be invented.

**Not `satellite.gtk`.** GTK is somebody else's library, and a name that leaks it into the
language surface is wrong the day the implementation changes — every program in existence
would say a word that is no longer true. §8.3 already named the type
`satellite.variable.window`, so the module is `satellite.window` and GTK stays behind the
shim where it is an implementation detail.

### Where it happens: a load phase between parse and resolve

The includer's tree is parsed, its `Include` items are walked, each named file is loaded and
parsed recursively, and every declaration is **merged into one Program**. Only then does
resolve() run, over the merged whole.

This is the decision that makes everything else cheap. resolve() already handles forward
references and mutual recursion across a whole program (§6's collect-then-walk), so a capsule
in one file calling a capsule in another needs no new machinery at all — it is the same
problem resolve() was already built for. The evaluator changes not at all.

- **Search order**: the directory of the *including* file, then `$SATELLITE_PATH`, then the
  installed library directory — which is `library_path()`'s three tiers (§9's "Finding the
  installed library") with the including file's own directory in front. Including-file-first is what lets a
  project's own files find each other with no configuration, which is the case that has to be
  frictionless.
- **Included once**, keyed by canonicalised path. Without it, `a` including both `b` and `c`
  where both include `d` is a duplicate-capsule error rather than a working program.
- **Cycles need no separate check.** Include-once makes `a -> b -> a` terminate on its own:
  by the time `b` asks for `a`, `a` is already loading and is skipped. This is what C's
  include guards do, and it is a feature rather than an error — two files that genuinely need
  each other's declarations are what resolve()'s forward references are for.
- **Flat namespace**, and a name defined twice across files is an error naming *both* files.
  §1 makes a user's capsule bare, and qualifying a name by the file it came from would need a
  second naming rule for no benefit until the library is large. Revisit when it is.
- **Top-level statements in an included file run**, in include order, before the includer's
  own. That is the included file's body, and a file that sets up globals needs one.

### Spans have to name a file, and it costs nothing

Today `format_error` takes one source string, because a program was one file. The moment a
program spans files, a runtime error inside an included capsule prints line N against the
**wrong file's text** — a language whose errors lie about where they are is unusable, and for
a self-hosting compiler it is fatal.

So `Span` gains a file id and the interpreter gains a `SourceMap` holding one text per loaded
file. The id is free:

```
Span today:                3 x size_t                  = 24 bytes
Span with a file id:  2 x size_t + 2 x uint32          = 24 bytes
```

A line number does not need 64 bits and neither does a file id, so `Expr` stays 96 bytes and
`Stmt` stays 200. The `SourceMap` is internal C++ bookkeeping — a vector of texts indexed by
that id — and is not a language feature. It is emphatically **not**
`satellite.container.map`, which remains unbuilt and is not needed for any of this.

### Native modules, and what they buy

A language-owned module may be backed by a shared object that the runtime `dlopen`s and which
registers `satellite.<name>.*` functions. `satellite.window` is the first, and the payoff is
larger than the window:

- GTK loads **only** when a program includes it. `satl --run hello.satl` keeps its 2.5ms
  startup (§9) and never pays the 23.4ms.
- `satl-term` — 188 lines of C++ — can then be written *in satellite*, because a
  terminal host is just a program that opens a window. It appears in neither column of §15's
  boundary table, which is the tell: it is C++ that no "cannot be written in satellite"
  argument defends, and it is exactly the kind of deletion the bootstrap plan exists for.

The mistake to avoid is binding GTK's API surface. §8.3 already settled the shape: a window is
a `shared_ptr<WindowHandle>` reached through accessor methods, so satellite sees a handful of
functions and the shim behind them is small. Exposing GTK wholesale through `dlsym` would be
thousands of lines of binding for a language whose whole naming rule is that it owns its own
surface.
