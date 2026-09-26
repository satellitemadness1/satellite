*satellite design docs, §11 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§10](10-evaluator.md), On: [§12](12-deliberately-deferred.md).*

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
| M9 | `satellite.container.map` (§8.6) | `src/evaluator/maps.cpp`; tests in `eval_test.cpp`, map stress in `library_test.cpp` | O(1) lookup; insertion order stable across runs; two independently built maps compare equal; `library_test` still TSan-clean | **done** |
| M10 | including a spaceship (§16) | `loader.*`, `loader_test.cpp` | a capsule defined in one spaceship is callable from another; a diamond loads the shared spaceship once; `a -> b -> a` terminates; an error inside an included spaceship names *that* spaceship | **done** |

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
standing up another binary that would have linked the same objects to say the same thing.
M10 went the other way and earned `loader_test.cpp`, the fourteenth: every interesting
property of the loader is about the filesystem — where it looks, what it canonicalises, what
it declines to read twice — and none of that is reachable from `eval_test`.
