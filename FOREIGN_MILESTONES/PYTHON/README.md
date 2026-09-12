# Python 3.12 → satellite, feature by feature

**The standard this folder is written against:** **Python 3.12** (October 2023),
with 3.13 features marked where they intrude. Python has no ISO standard; the
reference is the CPython language reference and standard library for that
version.

**Written 2026-09-12 against knowledge current to May 2026.** Anything added
after that is missing rather than excluded.

Python is the language most new satellite readers arrive from, and the one whose
habits break hardest, because the two disagree about *declaration*. Python
declares nothing; satellite declares everything, including the type.

**Rows 1–20 were the first pass; rows 21–323 were added on 2026-09-12** to take
the catalogue from the headline features to the whole language and the parts of
the standard library a program reaches for. **Every row has its own page.** Every
satellite fact in them was checked that day against `satl --words`,
`satellite.help`, or a probe program run through `satl`, and the page says so
where the answer surprised. The pages are generated from
[../tools/gen_python.py](../tools/gen_python.py); change the data there and
regenerate, rather than editing a page by hand.

**One file per feature.** `M16.md` is a position in a list of Python features,
not a satellite milestone. Where a feature needs work in satellite, the row names
the **satellite** milestone (M27–M45, in [../SATELLITE/](../SATELLITE/) and
[../CXX26/](../CXX26/)), and those numbers are real. **PYTHON/M33 is not
SATELLITE/M33.**

| answer | meaning |
|---|---|
| **says it** | satellite has this today; the page gives the line to type |
| **M<n>** | satellite cannot yet; that satellite milestone brings it |
| **never** | satellite will not have this, and the page says why |
| **partial** | satellite has part of it, and the page says which part |
| **open** | no milestone and no refusal yet: **a decision the author has not taken** |

**86 say it, 62 partial, 60 wait on a milestone, 51 never, 64 open.**

---

## Read these first

These are the rows where Python habits carry across **silently wrong**: no
refusal, just a different answer. Everything else in the catalogue either works
or is refused with a message naming the fix.

| row | the trap |
|---|---|
| [M90](M90.md) | **`//` is a comment.** `q = 7 // 2` is `q = 7`, and nothing reports it |
| [M165](M165.md) | **a list, map, string or number passed to a capsule is copied**, so `def fill(out): out.append(x)` changes nothing |
| [M198](M198.md) | **`b = a` copies a list**, and a spacesuit is shared: containers copy, spacesuits share |
| [M199](M199.md) | `grid[0].append(x)` is refused, and the fix takes a copy that must be written back |
| [M91](M91.md) | **`-7 % 3` is `-1`**, not Python's `2` |
| [M164](M164.md) | **`round(2.5)` is `3`**, not Python's `2` |
| [M305](M305.md) | **`satellite.time.sleep` takes milliseconds**, not seconds |
| [M209](M209.md) | `size()` counts bytes: `"héllo"` is 6 |
| [M312](M312.md) | **threads run in parallel with no GIL and no locks** |
| [M203](M203.md) | reference cycles are never freed; Python's collector frees them |
| [M16](M16.md) | there is no `self` |
| [M33](M33.md) | a `number` variable accepted a string, which may be a bug |
| [M116](M116.md) | a list subscripted with a string is a fuzzy *search*, not an error |

And three that are refused, loudly and well, but that a Python reader hits in the
first hour: **no truthiness** ([M98](M98.md)), **block scope**
([M34](M34.md)), and **exceptions cannot be caught** ([M134](M134.md) maps each
Python exception to its refusal code and the question to ask first).

---

## The catalogue

### The first twenty rows

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 1 | `print(x)` | 3.0 | says it | [M1](M1.md) |
| 2 | `def f():` | 1.0 | says it | [M2](M2.md) |
| 3 | `class C:` | 1.0 | says it | [M3](M3.md) |
| 4 | `import x` | 1.0 | partial | [M4](M4.md) |
| 5 | `if __name__ == "__main__":` | 1.0 | says it | [M5](M5.md) |
| 6 | `x = 5` (no declaration) | 1.0 | says it | [M6](M6.md) |
| 7 | `True` / `False` / `None` | 2.3 / 1.0 | says it | [M7](M7.md) |
| 8 | `elif` | 1.0 | says it | [M8](M8.md) |
| 9 | `for x in xs:` | 1.0 | partial | [M9](M9.md) |
| 10 | `and` / `or` / `not` | 1.0 | [M28](../SATELLITE/M28-logical-operators.md) | [M10](M10.md) |
| 11 | `break` / `continue` | 1.0 | [M27](../SATELLITE/M27-break-and-continue.md) | [M11](M11.md) |
| 12 | `lambda` | 1.0 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M12](M12.md) |
| 13 | `try` / `except` | 1.0 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M13](M13.md) |
| 14 | `match` statement | 3.10 | [M30](../SATELLITE/M30-switch-and-match.md) | [M14](M14.md) |
| 15 | list comprehensions | 2.0 | [M36](../SATELLITE/M36-ranges.md) | [M15](M15.md) |
| 16 | `self` | 1.0 | never | [M16](M16.md) |
| 17 | decorators | 2.4 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M17](M17.md) |
| 18 | generators and `yield` | 2.2 | never | [M18](M18.md) |
| 19 | f-strings | 3.6 | never | [M19](M19.md) |
| 20 | duck typing | 1.0 | never | [M20](M20.md) |

### Source text

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 21 | indentation as block structure | 1.0 | never | [M21](M21.md) |
| 22 | `#` comments | 1.0 | says it | [M22](M22.md) |
| 23 | docstrings | 1.0 | open | [M23](M23.md) |
| 24 | line continuation `\` and inside brackets | 1.0 | partial | [M24](M24.md) |
| 25 | semicolons and several statements on one line | 1.0 | never | [M25](M25.md) |
| 26 | `pass` | 1.0 | says it | [M26](M26.md) |
| 27 | `...` (Ellipsis) as a placeholder | 3.0 | never | [M27](M27.md) |
| 28 | Unicode identifiers | 3.0 | never | [M28](M28.md) |
| 29 | source encoding and `# -*- coding: -*-` | 2.3 | [M45](../CXX26/M45.md) | [M29](M29.md) |
| 30 | the shebang line `#!/usr/bin/env python3` | 1.0 | open | [M30](M30.md) |
| 31 | the interactive prompt (REPL) | 1.0 | says it | [M31](M31.md) |
| 32 | `python -c` and `python -m` | 1.0 | partial | [M32](M32.md) |

### Names, types and scope

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 33 | rebinding a name to a value of another type | 1.0 | open | [M33](M33.md) |
| 34 | block scope, not function scope | 1.0 | says it | [M34](M34.md) |
| 35 | the loop variable after the loop | 1.0 | says it | [M35](M35.md) |
| 36 | `global` | 1.0 | says it | [M36](M36.md) |
| 37 | `nonlocal` | 3.0 | never | [M37](M37.md) |
| 38 | `del name` | 1.0 | never | [M38](M38.md) |
| 39 | an unassigned name | 1.0 | says it | [M39](M39.md) |
| 40 | chained assignment `a = b = 0` | 1.0 | never | [M40](M40.md) |
| 41 | tuple unpacking and swapping `a, b = b, a` | 1.0 | [M35](../SATELLITE/M35-structured-bindings.md) | [M41](M41.md) |
| 42 | starred unpacking `first, *rest = xs` | 3.0 | [M35](../SATELLITE/M35-structured-bindings.md) | [M42](M42.md) |
| 43 | the walrus operator `:=` | 3.8 | open | [M43](M43.md) |
| 44 | constants by convention and `typing.Final` | 1.0 / 3.8 | partial | [M44](M44.md) |
| 45 | `is`, `is not` and `id()` | 1.0 | partial | [M45](M45.md) |
| 46 | `x is None` | 1.0 | says it | [M46](M46.md) |
| 47 | type hints on variables and parameters | 3.5 / 3.6 | partial | [M47](M47.md) |
| 48 | `mypy` and static type checking | 3.5 (PEP 484) | says it | [M48](M48.md) |
| 49 | `typing.Any` | 3.5 | says it | [M49](M49.md) |
| 50 | `Optional[T]` and `T \| None` | 3.5 / 3.10 | says it | [M50](M50.md) |
| 51 | `Union[A, B]` | 3.5 / 3.10 | partial | [M51](M51.md) |
| 52 | `type` statement and type aliases | 3.12 | open | [M52](M52.md) |
| 53 | generic classes and functions `class Box[T]`, `TypeVar` | 3.5 / 3.12 | [M31](../SATELLITE/M31-user-generics.md) | [M53](M53.md) |
| 54 | `ParamSpec`, `TypeVarTuple`, `Concatenate` | 3.10 / 3.11 | [M31](../SATELLITE/M31-user-generics.md) | [M54](M54.md) |
| 55 | `typing.Protocol` | 3.8 | [M31](../SATELLITE/M31-user-generics.md) | [M55](M55.md) |
| 56 | `Callable[[int], str]` | 3.5 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M56](M56.md) |
| 57 | `Literal`, `TypedDict`, `NewType` | 3.8 / 3.5 | partial | [M57](M57.md) |
| 58 | `Self`, `@override` and `@final` | 3.11 / 3.12 / 3.8 | partial | [M58](M58.md) |
| 59 | `@typing.overload` | 3.5 | [M39](../SATELLITE/M39-overloading-and-defaults.md) | [M59](M59.md) |
| 60 | `isinstance()`, `type()`, `issubclass()` | 1.0 / 2.2 | partial | [M60](M60.md) |
| 61 | `__annotations__`, `typing.get_type_hints` | 3.0 | [M41](../CXX26/M41.md) | [M61](M61.md) |

### Literals and built-in types

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 62 | `int`, arbitrary precision | 1.0 (unified 3.0) | says it | [M62](M62.md) |
| 63 | `float` | 1.0 | partial | [M63](M63.md) |
| 64 | `complex` and `1j` | 1.0 | never | [M64](M64.md) |
| 65 | `decimal.Decimal` | 2.4 | says it | [M65](M65.md) |
| 66 | `fractions.Fraction` | 2.6 | partial | [M66](M66.md) |
| 67 | `bool` is a subclass of `int` | 2.3 | never | [M67](M67.md) |
| 68 | `str` and its immutability | 1.0 | says it | [M68](M68.md) |
| 69 | single-quoted strings `'text'` | 1.0 | never | [M69](M69.md) |
| 70 | triple-quoted strings | 1.0 | open | [M70](M70.md) |
| 71 | raw strings `r"..."` | 1.0 | open | [M71](M71.md) |
| 72 | escape sequences `\n`, `\t`, `\u00e9` | 1.0 / 3.0 | says it | [M72](M72.md) |
| 73 | `bytes`, `b"..."` and `bytearray` | 3.0 | partial | [M73](M73.md) |
| 74 | `memoryview` and the buffer protocol | 2.7 | never | [M74](M74.md) |
| 75 | hex, binary and octal literals `0xff`, `0b1010`, `0o17` | 1.0 / 2.6 | partial | [M75](M75.md) |
| 76 | underscores in numbers `1_000_000` | 3.6 | open | [M76](M76.md) |
| 77 | scientific notation `1e3` | 1.0 | open | [M77](M77.md) |
| 78 | `int(s)`, `float(s)`, `str(n)` | 1.0 | says it | [M78](M78.md) |
| 79 | `hex()`, `bin()`, `oct()`, `int(s, base)` | 1.0 / 2.6 | partial | [M79](M79.md) |
| 80 | `ord()` and `chr()` | 1.0 | open | [M80](M80.md) |
| 81 | `list` | 1.0 | says it | [M81](M81.md) |
| 82 | list literals `[1, 2, 3]` | 1.0 | open | [M82](M82.md) |
| 83 | `tuple` | 1.0 | [M35](../SATELLITE/M35-structured-bindings.md) | [M83](M83.md) |
| 84 | `dict` | 1.0 | says it | [M84](M84.md) |
| 85 | dict literals `{"a": 1}` | 1.0 | open | [M85](M85.md) |
| 86 | `set` and `frozenset` | 2.4 | partial | [M86](M86.md) |
| 87 | `range()` | 1.0 | says it | [M87](M87.md) |
| 88 | `object` as the root of every class | 2.2 | never | [M88](M88.md) |

### Operators and expressions

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 89 | `/` true division | 3.0 | says it | [M89](M89.md) |
| 90 | `//` floor division | 2.2 | partial | [M90](M90.md) |
| 91 | `%` with negative operands | 1.0 | partial | [M91](M91.md) |
| 92 | `divmod()` | 1.0 | says it | [M92](M92.md) |
| 93 | `**` power | 1.0 | says it | [M93](M93.md) |
| 94 | `pow(base, exp, mod)` | 1.0 | partial | [M94](M94.md) |
| 95 | comparisons `== != < <= > >=` | 1.0 | says it | [M95](M95.md) |
| 96 | chained comparisons `1 < x < 3` | 1.0 | [M28](../SATELLITE/M28-logical-operators.md) | [M96](M96.md) |
| 97 | `and` / `or` answering an operand, `x or default` | 1.0 | never | [M97](M97.md) |
| 98 | truthiness `if items:` | 1.0 | never | [M98](M98.md) |
| 99 | `in` and `not in` | 1.0 | says it | [M99](M99.md) |
| 100 | the conditional expression `a if c else b` | 2.5 | [M37](../SATELLITE/M37-ternary.md) | [M100](M100.md) |
| 101 | bitwise `&`, `\|`, `^`, `~` | 1.0 | open | [M101](M101.md) |
| 102 | shifts `<<` and `>>` | 1.0 | says it | [M102](M102.md) |
| 103 | `@` matrix multiplication | 3.5 | never | [M103](M103.md) |
| 104 | augmented assignment `+=`, `-=`, `*=` | 2.0 | open | [M104](M104.md) |
| 105 | `+` joining a string and a number | 1.0 | says it | [M105](M105.md) |
| 106 | repetition `"-" * 40` and `[0] * n` | 1.0 | open | [M106](M106.md) |
| 107 | list concatenation `a + b` and `extend` | 1.0 | open | [M107](M107.md) |
| 108 | operator precedence and parentheses | 1.0 | says it | [M108](M108.md) |
| 109 | `eval()` and `exec()` | 1.0 | open | [M109](M109.md) |
| 110 | `compile()`, `ast`, `dis` | 2.2 / 2.5 | partial | [M110](M110.md) |

### Subscripts and slices

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 111 | negative indices `xs[-1]` | 1.0 | says it | [M111](M111.md) |
| 112 | slices `xs[a:b]`, `xs[a:]`, `xs[:b]` | 1.0 | says it | [M112](M112.md) |
| 113 | slice steps `xs[::2]`, `xs[::-1]` | 2.3 | partial | [M113](M113.md) |
| 114 | slice assignment and `del xs[a:b]` | 1.0 | open | [M114](M114.md) |
| 115 | `IndexError` | 1.0 | says it | [M115](M115.md) |
| 116 | subscripting a list with a string | — | says it | [M116](M116.md) |

### Control flow

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 117 | `while` | 1.0 | says it | [M117](M117.md) |
| 118 | `for i in range(len(xs))` | 1.0 | says it | [M118](M118.md) |
| 119 | `enumerate()` | 2.3 | says it | [M119](M119.md) |
| 120 | `zip()` | 2.0 | [M36](../SATELLITE/M36-ranges.md) | [M120](M120.md) |
| 121 | `reversed()` | 2.4 | says it | [M121](M121.md) |
| 122 | `for … else` and `while … else` | 1.0 | [M27](../SATELLITE/M27-break-and-continue.md) | [M122](M122.md) |
| 123 | `return` from inside a loop | 1.0 | says it | [M123](M123.md) |
| 124 | `match` with class and sequence patterns | 3.10 | [M30](../SATELLITE/M30-switch-and-match.md) | [M124](M124.md) |
| 125 | `match` guards `case x if x > 0` | 3.10 | [M30](../SATELLITE/M30-switch-and-match.md) | [M125](M125.md) |
| 126 | `match` mapping patterns and `case _` | 3.10 | [M30](../SATELLITE/M30-switch-and-match.md) | [M126](M126.md) |
| 127 | `assert` | 1.0 | [M42](../CXX26/M42.md) | [M127](M127.md) |
| 128 | `raise` | 1.0 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M128](M128.md) |
| 129 | `try … finally` | 1.0 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M129](M129.md) |
| 130 | `try … except … else` | 2.5 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M130](M130.md) |
| 131 | custom exception classes | 1.0 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M131](M131.md) |
| 132 | `raise … from` and exception chaining | 3.0 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M132](M132.md) |
| 133 | exception groups and `except*` | 3.11 | [M33](../SATELLITE/M33-catching-a-refusal.md) | [M133](M133.md) |
| 134 | the built-in exceptions and what each becomes | 1.0 | partial | [M134](M134.md) |
| 135 | tracebacks | 1.0 | says it | [M135](M135.md) |
| 136 | `warnings.warn` | 2.1 | open | [M136](M136.md) |
| 137 | `KeyboardInterrupt` | 1.0 | never | [M137](M137.md) |
| 138 | `with` statements and context managers | 2.5 | partial | [M138](M138.md) |
| 139 | `async def` and `await` | 3.5 | [M43](../CXX26/M43.md) | [M139](M139.md) |
| 140 | `async for`, `async with`, async generators | 3.5 / 3.6 | [M43](../CXX26/M43.md) | [M140](M140.md) |
| 141 | `asyncio` event loop, tasks and `gather` | 3.4 | [M43](../CXX26/M43.md) | [M141](M141.md) |
| 142 | `yield from` and generator delegation | 3.3 | never | [M142](M142.md) |
| 143 | generator expressions | 2.4 | [M36](../SATELLITE/M36-ranges.md) | [M143](M143.md) |
| 144 | dict and set comprehensions | 2.7 | [M36](../SATELLITE/M36-ranges.md) | [M144](M144.md) |
| 145 | the iterator protocol, `iter()`, `next()` | 2.2 | never | [M145](M145.md) |
| 146 | `itertools` | 2.3 | [M36](../SATELLITE/M36-ranges.md) | [M146](M146.md) |
| 147 | recursion and the recursion limit | 1.0 | partial | [M147](M147.md) |

### Functions

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 148 | default argument values | 1.0 | [M39](../SATELLITE/M39-overloading-and-defaults.md) | [M148](M148.md) |
| 149 | keyword arguments `f(name="x")` | 1.0 | [M39](../SATELLITE/M39-overloading-and-defaults.md) | [M149](M149.md) |
| 150 | keyword-only `*` and positional-only `/` parameters | 3.0 / 3.8 | [M39](../SATELLITE/M39-overloading-and-defaults.md) | [M150](M150.md) |
| 151 | `*args` | 1.0 | never | [M151](M151.md) |
| 152 | `**kwargs` | 1.0 | never | [M152](M152.md) |
| 153 | unpacking into a call `f(*xs)`, `f(**d)` | 2.0 | never | [M153](M153.md) |
| 154 | returning several values `return a, b` | 1.0 | [M35](../SATELLITE/M35-structured-bindings.md) | [M154](M154.md) |
| 155 | functions as values and passing a function | 1.0 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M155](M155.md) |
| 156 | closures | 2.2 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M156](M156.md) |
| 157 | nested functions | 1.0 | never | [M157](M157.md) |
| 158 | `functools.partial` | 2.5 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M158](M158.md) |
| 159 | `functools.lru_cache` and `cache` | 3.2 / 3.9 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M159](M159.md) |
| 160 | `map()`, `filter()`, `functools.reduce()` | 1.0 | [M36](../SATELLITE/M36-ranges.md) | [M160](M160.md) |
| 161 | `sorted()` and `list.sort(key=…, reverse=…)` | 2.4 | partial | [M161](M161.md) |
| 162 | `min()`, `max()`, `sum()` | 1.0 | says it | [M162](M162.md) |
| 163 | `any()` and `all()` | 2.5 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M163](M163.md) |
| 164 | `abs()` and `round()` | 1.0 | partial | [M164](M164.md) |
| 165 | argument passing: what a function can change | 1.0 | says it | [M165](M165.md) |
| 166 | the mutable default argument trap | 1.0 | never | [M166](M166.md) |
| 167 | a function with no `return` answers `None` | 1.0 | says it | [M167](M167.md) |
| 168 | `callable()` | 1.0 | never | [M168](M168.md) |
| 169 | `help()` and `dir()` | 1.0 | partial | [M169](M169.md) |

### Classes and objects

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 170 | `__init__` with parameters | 1.0 | partial | [M170](M170.md) |
| 171 | attributes added at run time `obj.extra = 1` | 1.0 | never | [M171](M171.md) |
| 172 | public attributes `obj.x` | 1.0 | never | [M172](M172.md) |
| 173 | class attributes | 1.0 | partial | [M173](M173.md) |
| 174 | `@property` | 2.2 | says it | [M174](M174.md) |
| 175 | `@staticmethod` | 2.2 | says it | [M175](M175.md) |
| 176 | `@classmethod` and alternative constructors | 2.2 | partial | [M176](M176.md) |
| 177 | single inheritance `class B(A):` | 1.0 | says it | [M177](M177.md) |
| 178 | `super()` | 2.2 | partial | [M178](M178.md) |
| 179 | overriding and calling through a base-typed name | 1.0 | partial | [M179](M179.md) |
| 180 | multiple inheritance and the MRO | 2.2 (C3: 2.3) | open | [M180](M180.md) |
| 181 | mixins | 2.2 | open | [M181](M181.md) |
| 182 | abstract base classes `abc.ABC`, `@abstractmethod` | 2.6 | [M31](../SATELLITE/M31-user-generics.md) | [M182](M182.md) |
| 183 | `__str__` and `__repr__` | 1.0 | partial | [M183](M183.md) |
| 184 | `__eq__`, `__lt__`, `__hash__` | 1.0 / 2.1 | [M38](../SATELLITE/M38-operator-overloading.md) | [M184](M184.md) |
| 185 | `__add__` and arithmetic dunders | 1.0 | [M38](../SATELLITE/M38-operator-overloading.md) | [M185](M185.md) |
| 186 | `__len__`, `__getitem__`, `__contains__`, `__iter__` | 1.0 / 2.2 | never | [M186](M186.md) |
| 187 | `__call__` | 1.0 | never | [M187](M187.md) |
| 188 | `__getattr__`, `__setattr__`, `__getattribute__` | 1.0 / 2.2 | never | [M188](M188.md) |
| 189 | descriptors `__get__`, `__set__` | 2.2 | never | [M189](M189.md) |
| 190 | `__slots__` | 2.2 | never | [M190](M190.md) |
| 191 | metaclasses, `__init_subclass__`, `__class_getitem__` | 2.2 / 3.6 / 3.7 | never | [M191](M191.md) |
| 192 | class decorators | 2.6 | [M32](../SATELLITE/M32-capsules-as-values.md) | [M192](M192.md) |
| 193 | `__del__` and finalisers | 1.0 | partial | [M193](M193.md) |
| 194 | `@dataclass` | 3.7 | partial | [M194](M194.md) |
| 195 | `collections.namedtuple` | 2.6 | [M35](../SATELLITE/M35-structured-bindings.md) | [M195](M195.md) |
| 196 | `enum.Enum` | 3.4 | open | [M196](M196.md) |
| 197 | private names `_x` and name mangling `__x` | 1.0 | says it | [M197](M197.md) |
| 198 | assignment shares a list: `b = a` | 1.0 | says it | [M198](M198.md) |
| 199 | changing a list inside a list `grid[0].append(x)` | 1.0 | partial | [M199](M199.md) |
| 200 | `copy.copy`, `copy.deepcopy`, `copy.replace` | 1.0 / 3.13 | partial | [M200](M200.md) |
| 201 | nested classes | 1.0 | open | [M201](M201.md) |
| 202 | `vars()`, `__dict__`, `getattr(obj, "name")`, `hasattr` | 1.0 | [M41](../CXX26/M41.md) | [M202](M202.md) |
| 203 | garbage collection of reference cycles | 2.0 | open | [M203](M203.md) |
| 204 | `weakref` | 2.1 | open | [M204](M204.md) |
| 205 | the `gc` module | 2.0 | never | [M205](M205.md) |

### Strings

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 206 | `str.format()` | 2.6 | never | [M206](M206.md) |
| 207 | `%` formatting | 1.0 | never | [M207](M207.md) |
| 208 | number formatting: `f"{x:.2f}"`, `{n:,}`, `{n:08}` | 2.6 / 3.6 | open | [M208](M208.md) |
| 209 | `len(s)` | 1.0 | says it | [M209](M209.md) |
| 210 | indexing and slicing a string | 1.0 | says it | [M210](M210.md) |
| 211 | `upper()`, `lower()` | 1.0 | says it | [M211](M211.md) |
| 212 | `strip()`, `lstrip()`, `rstrip()` | 1.0 | partial | [M212](M212.md) |
| 213 | `split()` | 1.0 | partial | [M213](M213.md) |
| 214 | `",".join(parts)` | 1.0 | says it | [M214](M214.md) |
| 215 | `replace()` | 1.0 | says it | [M215](M215.md) |
| 216 | `find()`, `index()`, `rfind()` | 1.0 | partial | [M216](M216.md) |
| 217 | `startswith()`, `endswith()`, `in` | 1.0 | says it | [M217](M217.md) |
| 218 | `count`, `isdigit`, `isalpha`, `isspace`, `title`, `capitalize`, `center`, `zfill`, `partition`, `splitlines`, `removeprefix` | 1.0 / 3.9 | open | [M218](M218.md) |
| 219 | `repr()` and quoting in displayed containers | 1.0 | open | [M219](M219.md) |
| 220 | `encode()`, `decode()` and codecs | 2.0 / 3.0 | [M45](../CXX26/M45.md) | [M220](M220.md) |
| 221 | the `re` module | 1.5 | open | [M221](M221.md) |
| 222 | `textwrap`, `string` constants, `difflib` | 2.3 / 1.0 / 2.1 | open | [M222](M222.md) |

### Lists, maps and sets in detail

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 223 | `list.insert(i, x)` | 1.0 | says it | [M223](M223.md) |
| 224 | `list.pop()` and `pop(0)` | 1.0 | says it | [M224](M224.md) |
| 225 | `list.remove(x)`, `del xs[i]` | 1.0 | says it | [M225](M225.md) |
| 226 | `list.index(x)` | 1.0 | says it | [M226](M226.md) |
| 227 | `list.count(x)` | 1.0 | partial | [M227](M227.md) |
| 228 | `clear()`, `copy()`, `reverse()` | 1.0 / 3.3 | says it | [M228](M228.md) |
| 229 | `dict.get(k, default)` | 1.0 | says it | [M229](M229.md) |
| 230 | `setdefault()` and `collections.defaultdict` | 2.0 / 2.5 | partial | [M230](M230.md) |
| 231 | `keys()`, `values()`, `items()` | 1.0 | says it | [M231](M231.md) |
| 232 | `del d[k]` and `dict.pop(k)` | 1.0 | says it | [M232](M232.md) |
| 233 | `update()` and `\|` merge | 1.0 / 3.9 | partial | [M233](M233.md) |
| 234 | map keys of other types | 1.0 | partial | [M234](M234.md) |
| 235 | `collections.Counter` | 2.7 | partial | [M235](M235.md) |
| 236 | `collections.OrderedDict` | 2.7 | says it | [M236](M236.md) |
| 237 | `collections.deque` | 2.4 | says it | [M237](M237.md) |
| 238 | `heapq` and `queue.PriorityQueue` | 2.3 | partial | [M238](M238.md) |
| 239 | `bisect` | 2.1 | partial | [M239](M239.md) |
| 240 | set operations `\| & - ^` | 2.4 | partial | [M240](M240.md) |
| 241 | the `array` module | 1.4 | never | [M241](M241.md) |

### Modules, packages and the program

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 242 | `from module import name` | 1.0 | open | [M242](M242.md) |
| 243 | `import module as alias` | 1.0 / 2.0 | open | [M243](M243.md) |
| 244 | writing a module in a second file | 1.0 | partial | [M244](M244.md) |
| 245 | packages, `__init__.py` and relative imports | 1.5 / 2.5 | open | [M245](M245.md) |
| 246 | `__all__` and underscore-private module names | 1.0 | open | [M246](M246.md) |
| 247 | `importlib` and importing by name at run time | 3.1 | never | [M247](M247.md) |
| 248 | `pip`, PyPI, virtual environments | 3.4 (ensurepip) / 3.3 (venv) | open | [M248](M248.md) |
| 249 | `__file__` and `__name__` | 1.0 | open | [M249](M249.md) |
| 250 | `sys.argv` | 1.0 | says it | [M250](M250.md) |
| 251 | `argparse` | 3.2 | says it | [M251](M251.md) |
| 252 | `sys.exit(code)` | 1.0 | partial | [M252](M252.md) |
| 253 | `input()` | 1.0 | says it | [M253](M253.md) |
| 254 | `sys.stderr` and `print(..., file=sys.stderr)` | 1.0 | open | [M254](M254.md) |
| 255 | `sys.version`, `platform` | 1.0 | says it | [M255](M255.md) |
| 256 | `sys.getrecursionlimit()`, `sys.float_info`, `sys.maxsize` | 2.0 | says it | [M256](M256.md) |
| 257 | `os.environ` | 1.0 | says it | [M257](M257.md) |
| 258 | `os.getcwd()`, `os.chdir()`, `os.listdir()` | 1.0 | says it | [M258](M258.md) |
| 259 | `os.path.exists()` and `pathlib.Path` | 1.0 / 3.4 | partial | [M259](M259.md) |
| 260 | `os.remove()`, `os.rmdir()` | 1.0 | says it | [M260](M260.md) |
| 261 | `os.mkdir()`, `os.rename()`, `shutil.copy()`, `shutil.rmtree()` | 1.0 / 2.3 | open | [M261](M261.md) |
| 262 | `glob` and `fnmatch` | 1.0 | partial | [M262](M262.md) |
| 263 | `tempfile` | 2.3 | open | [M263](M263.md) |
| 264 | `subprocess` and `os.system` | 2.4 / 1.0 | open | [M264](M264.md) |
| 265 | `multiprocessing` | 2.6 | open | [M265](M265.md) |
| 266 | `os.getpid()`, `os.cpu_count()`, `getpass.getuser()` | 1.0 / 3.4 | says it | [M266](M266.md) |
| 267 | process memory (`resource`, `tracemalloc`) | 2.0 / 3.4 | says it | [M267](M267.md) |
| 268 | `signal` and `atexit` | 1.0 / 2.0 | never | [M268](M268.md) |
| 269 | `logging` | 2.3 | partial | [M269](M269.md) |
| 270 | `unittest`, `pytest`, `doctest` | 2.1 | open | [M270](M270.md) |
| 271 | `pdb` and `breakpoint()` | 1.0 / 3.7 | open | [M271](M271.md) |
| 272 | `timeit`, `time.perf_counter`, `cProfile` | 2.3 / 3.3 | open | [M272](M272.md) |
| 273 | `__debug__` and `python -O` | 1.0 | never | [M273](M273.md) |
| 274 | `from __future__ import …` | 2.1 | never | [M274](M274.md) |
| 275 | `ctypes`, C extensions and `cffi` | 2.5 | never | [M275](M275.md) |
| 276 | `sys.getsizeof()` | 2.6 | never | [M276](M276.md) |
| 277 | `locals()`, `globals()`, `inspect` | 1.0 / 2.1 | [M41](../CXX26/M41.md) | [M277](M277.md) |
| 278 | `warnings.deprecated` and `@deprecated` | 3.13 | never | [M278](M278.md) |

### Files and data formats

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 279 | `open(path, mode)` | 1.0 | says it | [M279](M279.md) |
| 280 | a file that is not there | 1.0 | says it | [M280](M280.md) |
| 281 | `read()`, `readline()`, `write()` | 1.0 | says it | [M281](M281.md) |
| 282 | `for line in f:` | 1.0 | partial | [M282](M282.md) |
| 283 | binary mode, `seek()`, `tell()` | 1.0 | open | [M283](M283.md) |
| 284 | `json` | 2.6 | open | [M284](M284.md) |
| 285 | `csv` | 2.3 | partial | [M285](M285.md) |
| 286 | `tomllib`, `configparser`, `xml` | 3.11 / 1.5 / 2.0 | open | [M286](M286.md) |
| 287 | `pickle` and `shelve` | 1.0 | open | [M287](M287.md) |
| 288 | `sqlite3` | 2.5 | open | [M288](M288.md) |
| 289 | `zipfile`, `gzip`, `tarfile` | 1.6 / 1.5 / 2.3 | open | [M289](M289.md) |
| 290 | `hashlib`, `hmac`, `base64`, `uuid`, `secrets` | 2.5 / 2.2 / 1.0 / 2.5 / 3.6 | open | [M290](M290.md) |
| 291 | `io.StringIO` | 2.0 / 3.0 | never | [M291](M291.md) |
| 292 | `struct` | 1.4 | never | [M292](M292.md) |

### Numbers and maths

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 293 | `math.sqrt`, `floor`, `ceil`, `trunc` | 1.0 | says it | [M293](M293.md) |
| 294 | `math.sin`, `cos`, `log`, `exp` | 1.0 | open | [M294](M294.md) |
| 295 | `math.pi`, `math.e`, `math.inf`, `math.nan` | 1.0 / 3.5 | open | [M295](M295.md) |
| 296 | `math.gcd`, `lcm`, `factorial`, `comb`, `isqrt` | 3.5 / 3.9 / 2.6 / 3.8 | open | [M296](M296.md) |
| 297 | `math.isclose` | 3.5 | never | [M297](M297.md) |
| 298 | `statistics.mean`, `median` | 3.4 | partial | [M298](M298.md) |
| 299 | `random.randint`, `random.random`, `random.seed` | 1.0 | says it | [M299](M299.md) |
| 300 | `random.choice`, `shuffle`, `sample` | 1.0 / 2.3 | partial | [M300](M300.md) |
| 301 | `random.gauss` and other distributions | 1.0 | open | [M301](M301.md) |
| 302 | `int.bit_length()`, `int.bit_count()` | 2.7 / 3.10 | open | [M302](M302.md) |
| 303 | `float("inf")`, `float("nan")`, `math.isnan` | 2.6 | open | [M303](M303.md) |

### Time

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 304 | `time.time()` and `datetime.now()` | 1.0 / 2.3 | partial | [M304](M304.md) |
| 305 | `time.sleep(seconds)` | 1.0 | says it | [M305](M305.md) |
| 306 | `datetime` arithmetic and `timedelta` | 2.3 | partial | [M306](M306.md) |
| 307 | `strftime` and `strptime` | 2.3 | partial | [M307](M307.md) |
| 308 | `zoneinfo` and time zones | 3.9 | open | [M308](M308.md) |
| 309 | `time.monotonic()` | 3.3 | open | [M309](M309.md) |

### Concurrency

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 310 | `threading.Thread`, `start()`, `join()` | 1.5 | says it | [M310](M310.md) |
| 311 | getting a result back from a thread | 3.2 (`futures`) | says it | [M311](M311.md) |
| 312 | the GIL and free-threaded Python | 1.5 / 3.13 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M312](M312.md) |
| 313 | `threading.Lock` and `RLock` | 1.5 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M313](M313.md) |
| 314 | `Condition`, `Event`, `Semaphore`, `Barrier` | 1.5 / 3.2 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M314](M314.md) |
| 315 | `queue.Queue` | 2.3 | [M40](../SATELLITE/M40-locks-and-atomics.md) | [M315](M315.md) |
| 316 | `concurrent.futures` and thread pools | 3.2 | [M43](../CXX26/M43.md) | [M316](M316.md) |
| 317 | `threading.local` | 2.4 | open | [M317](M317.md) |
| 318 | daemon threads and `threading.Timer` | 1.5 / 2.0 | open | [M318](M318.md) |

### Networking, windows and the terminal

| # | Python feature | since | satellite | file |
|---|---|---|---|---|
| 319 | `socket` | 1.0 | open | [M319](M319.md) |
| 320 | `http.server`, `urllib.request`, `requests` | 1.0 / 3.0 | open | [M320](M320.md) |
| 321 | `email`, `smtplib`, `ftplib` | 1.0 / 2.2 | never | [M321](M321.md) |
| 322 | `tkinter` | 1.1 | open | [M322](M322.md) |
| 323 | `curses` and terminal size | 1.5 | partial | [M323](M323.md) |

---

## The milestones this folder asks for

| satellite milestone | Python rows |
|---|---|
| [M27](../SATELLITE/M27-break-and-continue.md) | [M11](M11.md), [M122](M122.md) |
| [M28](../SATELLITE/M28-logical-operators.md) | [M10](M10.md), [M96](M96.md) |
| [M30](../SATELLITE/M30-switch-and-match.md) | [M14](M14.md), [M124](M124.md), [M125](M125.md), [M126](M126.md) |
| [M31](../SATELLITE/M31-user-generics.md) | [M53](M53.md), [M54](M54.md), [M55](M55.md), [M182](M182.md) |
| [M32](../SATELLITE/M32-capsules-as-values.md) | [M12](M12.md), [M17](M17.md), [M56](M56.md), [M155](M155.md), [M156](M156.md), [M158](M158.md), [M159](M159.md), [M163](M163.md), [M192](M192.md) |
| [M33](../SATELLITE/M33-catching-a-refusal.md) | [M13](M13.md), [M128](M128.md), [M129](M129.md), [M130](M130.md), [M131](M131.md), [M132](M132.md), [M133](M133.md) |
| [M35](../SATELLITE/M35-structured-bindings.md) | [M41](M41.md), [M42](M42.md), [M83](M83.md), [M154](M154.md), [M195](M195.md) |
| [M36](../SATELLITE/M36-ranges.md) | [M15](M15.md), [M120](M120.md), [M143](M143.md), [M144](M144.md), [M146](M146.md), [M160](M160.md) |
| [M37](../SATELLITE/M37-ternary.md) | [M100](M100.md) |
| [M38](../SATELLITE/M38-operator-overloading.md) | [M184](M184.md), [M185](M185.md) |
| [M39](../SATELLITE/M39-overloading-and-defaults.md) | [M59](M59.md), [M148](M148.md), [M149](M149.md), [M150](M150.md) |
| [M40](../SATELLITE/M40-locks-and-atomics.md) | [M312](M312.md), [M313](M313.md), [M314](M314.md), [M315](M315.md) |
| [M41](../CXX26/M41.md) | [M61](M61.md), [M202](M202.md), [M277](M277.md) |
| [M42](../CXX26/M42.md) | [M127](M127.md) |
| [M43](../CXX26/M43.md) | [M139](M139.md), [M140](M140.md), [M141](M141.md), [M316](M316.md) |
| [M45](../CXX26/M45.md) | [M29](M29.md), [M220](M220.md) |

**M40 matters most to a Python reader**, and not as a convenience: Python code
that is thread-safe only because of the GIL is not thread-safe here (row
[M312](M312.md)).

**M33 is the largest mismatch in habit.** Python is written *easier to ask
forgiveness than permission*, and satellite is built for *look before you leap*:
every refusal a Python exception maps to names the question to ask first. The
exception is `"abc".to_number()`, which has no such question until `isdigit`
exists ([M78](M78.md), [M218](M218.md)).

---

## The open rows — to talk through, one at a time

These rows have no honest answer until the author decides something. Many share
a decision with an open CXX23 row, and the page says which.

- **Probably a bug, not a decision:** [M33](M33.md) (a number variable took a string)
- **Literal spellings:** [M30](M30.md) shebang, [M70](M70.md) triple quotes,
  [M71](M71.md) raw strings, [M76](M76.md) `1_000`, [M77](M77.md) `1e3` (which
  `to_number` already accepts), [M82](M82.md) list literals, [M85](M85.md) dict literals
- **Operators satellite lacks:** [M104](M104.md) `+=`, [M101](M101.md) bitwise,
  [M106](M106.md) `*` on strings and lists, [M107](M107.md) list `+`/`extend`,
  [M114](M114.md) slice assignment, [M43](M43.md) walrus
- **Types:** [M52](M52.md) aliases, [M196](M196.md) `enum` (the one most likely to
  become a milestone, alongside satellite's `switch`)
- **Objects:** [M180](M180.md) and [M181](M181.md) multiple inheritance,
  [M201](M201.md) nested classes, [M203](M203.md) and [M204](M204.md) cycles and weak
  references (DESIGN §12 names both fixes and chooses neither)
- **A second file and program namespaces:** [M242](M242.md), [M243](M243.md),
  [M245](M245.md), [M246](M246.md), [M248](M248.md), [M249](M249.md)
- **Text:** [M208](M208.md) number formatting (the real loss behind *no
  f-strings*), [M218](M218.md) string methods, [M219](M219.md) quoting in
  displayed lists, [M221](M221.md) regex, [M222](M222.md), [M80](M80.md) `ord`/`chr`,
  [M23](M23.md) docstrings
- **The outside world:** [M254](M254.md) stderr, [M261](M261.md) mkdir/rename/copy,
  [M263](M263.md), [M264](M264.md) subprocess, [M265](M265.md), [M283](M283.md) binary
  files, [M284](M284.md) JSON, [M286](M286.md)–[M290](M290.md)
- **Maths and time:** [M294](M294.md)–[M296](M296.md), [M301](M301.md)–[M303](M303.md),
  [M308](M308.md), [M309](M309.md), [M272](M272.md)
- **Tools:** [M270](M270.md) tests, [M271](M271.md) debugger, [M136](M136.md)
  warnings, [M109](M109.md) `eval`
- **Threads:** [M317](M317.md), [M318](M318.md)
- **Numbered in satellite, planned in PLAN.md, not built:** [M319](M319.md) sockets,
  [M320](M320.md) HTTP, [M322](M322.md) windows. These are open only in the sense
  that this folder makes no promise for them.

| row | feature |
|---|---|
| [M23](M23.md) | docstrings |
| [M30](M30.md) | the shebang line `#!/usr/bin/env python3` |
| [M33](M33.md) | rebinding a name to a value of another type |
| [M43](M43.md) | the walrus operator `:=` |
| [M52](M52.md) | `type` statement and type aliases |
| [M70](M70.md) | triple-quoted strings |
| [M71](M71.md) | raw strings `r"..."` |
| [M76](M76.md) | underscores in numbers `1_000_000` |
| [M77](M77.md) | scientific notation `1e3` |
| [M80](M80.md) | `ord()` and `chr()` |
| [M82](M82.md) | list literals `[1, 2, 3]` |
| [M85](M85.md) | dict literals `{"a": 1}` |
| [M101](M101.md) | bitwise `&`, `\|`, `^`, `~` |
| [M104](M104.md) | augmented assignment `+=`, `-=`, `*=` |
| [M106](M106.md) | repetition `"-" * 40` and `[0] * n` |
| [M107](M107.md) | list concatenation `a + b` and `extend` |
| [M109](M109.md) | `eval()` and `exec()` |
| [M114](M114.md) | slice assignment and `del xs[a:b]` |
| [M136](M136.md) | `warnings.warn` |
| [M180](M180.md) | multiple inheritance and the MRO |
| [M181](M181.md) | mixins |
| [M196](M196.md) | `enum.Enum` |
| [M201](M201.md) | nested classes |
| [M203](M203.md) | garbage collection of reference cycles |
| [M204](M204.md) | `weakref` |
| [M208](M208.md) | number formatting: `f"{x:.2f}"`, `{n:,}`, `{n:08}` |
| [M218](M218.md) | `count`, `isdigit`, `isalpha`, `isspace`, `title`, `capitalize`, `center`, `zfill`, `partition`, `splitlines`, `removeprefix` |
| [M219](M219.md) | `repr()` and quoting in displayed containers |
| [M221](M221.md) | the `re` module |
| [M222](M222.md) | `textwrap`, `string` constants, `difflib` |
| [M242](M242.md) | `from module import name` |
| [M243](M243.md) | `import module as alias` |
| [M245](M245.md) | packages, `__init__.py` and relative imports |
| [M246](M246.md) | `__all__` and underscore-private module names |
| [M248](M248.md) | `pip`, PyPI, virtual environments |
| [M249](M249.md) | `__file__` and `__name__` |
| [M254](M254.md) | `sys.stderr` and `print(..., file=sys.stderr)` |
| [M261](M261.md) | `os.mkdir()`, `os.rename()`, `shutil.copy()`, `shutil.rmtree()` |
| [M263](M263.md) | `tempfile` |
| [M264](M264.md) | `subprocess` and `os.system` |
| [M265](M265.md) | `multiprocessing` |
| [M270](M270.md) | `unittest`, `pytest`, `doctest` |
| [M271](M271.md) | `pdb` and `breakpoint()` |
| [M272](M272.md) | `timeit`, `time.perf_counter`, `cProfile` |
| [M283](M283.md) | binary mode, `seek()`, `tell()` |
| [M284](M284.md) | `json` |
| [M286](M286.md) | `tomllib`, `configparser`, `xml` |
| [M287](M287.md) | `pickle` and `shelve` |
| [M288](M288.md) | `sqlite3` |
| [M289](M289.md) | `zipfile`, `gzip`, `tarfile` |
| [M290](M290.md) | `hashlib`, `hmac`, `base64`, `uuid`, `secrets` |
| [M294](M294.md) | `math.sin`, `cos`, `log`, `exp` |
| [M295](M295.md) | `math.pi`, `math.e`, `math.inf`, `math.nan` |
| [M296](M296.md) | `math.gcd`, `lcm`, `factorial`, `comb`, `isqrt` |
| [M301](M301.md) | `random.gauss` and other distributions |
| [M302](M302.md) | `int.bit_length()`, `int.bit_count()` |
| [M303](M303.md) | `float("inf")`, `float("nan")`, `math.isnan` |
| [M308](M308.md) | `zoneinfo` and time zones |
| [M309](M309.md) | `time.monotonic()` |
| [M317](M317.md) | `threading.local` |
| [M318](M318.md) | daemon threads and `threading.Timer` |
| [M319](M319.md) | `socket` |
| [M320](M320.md) | `http.server`, `urllib.request`, `requests` |
| [M322](M322.md) | `tkinter` |
