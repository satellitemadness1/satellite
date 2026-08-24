*satellite design docs, §14 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§13](13-decisions.md), On: [§15](15-bootstrapping.md).*

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
