#!/usr/bin/env python3
# Generates FOREIGN_MILESTONES/PYTHON/M1.md onward, and PYTHON/README.md's index.
# Every satellite fact below was checked on 2026-09-12 against `satl --words`,
# `satellite.help`, or a probe program run through ./satl. Rows 1-20 keep the
# numbers the first-pass index gave them; everything after appends.
import os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))
OUT = os.path.join(HERE, "..", "PYTHON")
FIRST = 1

SAT = {
    "M27": "../SATELLITE/M27-break-and-continue.md",
    "M28": "../SATELLITE/M28-logical-operators.md",
    "M29": "../SATELLITE/M29-method-chaining.md",
    "M30": "../SATELLITE/M30-switch-and-match.md",
    "M31": "../SATELLITE/M31-user-generics.md",
    "M32": "../SATELLITE/M32-capsules-as-values.md",
    "M33": "../SATELLITE/M33-catching-a-refusal.md",
    "M34": "../SATELLITE/M34-compile-time-evaluation.md",
    "M35": "../SATELLITE/M35-structured-bindings.md",
    "M36": "../SATELLITE/M36-ranges.md",
    "M37": "../SATELLITE/M37-ternary.md",
    "M38": "../SATELLITE/M38-operator-overloading.md",
    "M39": "../SATELLITE/M39-overloading-and-defaults.md",
    "M40": "../SATELLITE/M40-locks-and-atomics.md",
    "M41": "../CXX26/M41.md",
    "M42": "../CXX26/M42.md",
    "M43": "../CXX26/M43.md",
    "M44": "../CXX26/M44.md",
    "M45": "../CXX26/M45.md",
}

def link(text):
    """Turn bare M27..M45 into links to the satellite milestone files."""
    return re.sub(r"(?<![\[(/])\bM(2[7-9]|3[0-9]|4[0-5])\b(?!\.md|\])",
                  lambda m: f"[{m.group(0)}]({SAT[m.group(0)]})", text)

SECTIONS = []  # (heading, [entries])

def section(name):
    SECTIONS.append((name, []))

def code(s):
    return s.strip("\n") if s else None

def E(feature, since, answer, py=None, sat=None, differs=""):
    SECTIONS[-1][1].append(dict(feature=feature, since=since, answer=answer,
                                py=code(py), sat=code(sat),
                                differs=(differs or "").strip()))

# ---------------------------------------------------------------------------
section("The first twenty rows")

E("`print(x)`", "3.0", "says it",
  r'''
print("total:", total)
print(a, b, sep=", ", end="")
''',
  r'''
satellite.console.display("total: " + total)
''',
  r"""
**`display` takes exactly one argument.** Checked 2026-09-12:
`satellite.console.display("a", "b")` is `S0722: display takes 1 argument and was
given 2`. Build the line first with `+`; a number joins onto a string without
`str()` (row @{`+` joining}@).

`display` always ends the line, and there is no `sep`, `end`, `file` or `flush`.
A list displays as `[x, y]` and a map as `{2: two, 1: one}`, with strings
**unquoted**, unlike Python's `repr` (row @{`repr()`}@).
""")

E("`def f():`", "1.0", "says it",
  r'''
def area(width, height):
    return width * height
''',
  r'''
satellite.capsule area(satellite.variable.number width, satellite.variable.number height) satellite.returns(satellite.variable.number)
{
    satellite.return(width * height)
}
''',
  r"""
**Every parameter names its type, and so does the answer.** A capsule that
answers nothing leaves out `satellite.returns` and ends with `satellite.return()`.

A capsule is declared at the top of a file, never inside another capsule
(`S0213`), so there are no nested functions (row @{nested functions}@). A capsule can
call one written further down the file.
""")

E("`class C:`", "1.0", "says it",
  r'''
class Counter:
    def __init__(self):
        self.count = 0
    def add(self, n):
        self.count += n
''',
  r'''
satellite.spacesuit counter()
{
    satellite.protected
    {
        satellite.variable.number count = 0
    }
    satellite.public
    {
        satellite.capsule add(satellite.variable.number by)
        {
            count = count + by
        }
    }
}
''',
  r"""
**Three differences that a Python reader hits on day one:**

- **Fields are declared up front**, with their types and starting values, in
  `satellite.protected`. They cannot be added later from a method.
- **A field is never reachable from outside with a dot.** `tally.count` is
  refused (`S0517`) even for a public field. Ask the object through a capsule.
- **There is no `self`.** Inside the suit, fields and the suit's own capsules
  are named bare (row @{=`self`}@).

Declaring a suit builds one: `counter tally` with no `=`.
""")

E("`import x`", "1.0", "partial",
  r'''
import math
from collections import deque
''',
  r'''
satellite.include(satellite)
''',
  r"""
**The one include that runs brings the whole language**, so there is nothing to
import one module at a time: `satellite.console`, `satellite.file` and the rest
are all in scope after the first line.

**Partial, because importing a program's own second file does not exist.** A
program is one file today. Including another `.satl` file is PLAN.md's build
milestone 25, which is the plan's numbering and not a promise from this folder.
See rows @{writing a module in a second file}@ and @{packages, `__init__.py`}@.
""")

E("`if __name__ == \"__main__\":`", "1.0", "says it",
  r'''
def main():
    ...

if __name__ == "__main__":
    main()
''',
  r'''
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.return(satellite)
}
''',
  r"""
**There is exactly one entry point and it is named.** Code at the top level of a
Python file runs when the file is imported *and* when it is run; a satellite
file's top level holds only declarations (`S0204` names what may be there), so
nothing runs until `satellite.main` does. The guard has nothing to guard.
""")

E("`x = 5` (no declaration)", "1.0", "says it",
  r'''
x = 5
name = "ada"
''',
  r'''
satellite.variable.number x = 5
satellite.variable.string name = "ada"
''',
  r"""
**A name is declared once, with a type, before it is used.** Checked
2026-09-12: assigning to a name nobody declared is `S0511: nothing called y is in
scope here`, and declaring a name twice is `S0242`.

**Scope is the block, not the function.** See row @{block scope}@: a name declared
inside an `if` is gone after the `}`, which is the opposite of Python.
""")

E("`True` / `False` / `None`", "2.3 / 1.0", "says it",
  r'''
ready = True
result = None
if result is None: ...
''',
  r'''
satellite.variable.bool ready = satellite.bool.true
satellite.variable.variant result
satellite.statement.if (result.holds("nothing")) { ... }
''',
  r"""
**`None` is not a value you assign; "nothing" is a state every declaration
starts in.** Checked 2026-09-12: a declared, unassigned variant answers
`holding()` with `nothing`, and `satellite.system.environment` of an unset
variable answers `nothing` rather than raising.

**`True` is not `1`.** `satellite.bool.true + 1` is `S0711` (row @{`bool` is a subclass}@).
""")

E("`elif`", "1.0", "says it",
  r'''
if x == 0:
    ...
elif x == 1:
    ...
else:
    ...
''',
  r'''
satellite.statement.if (x == 0) { ... }
satellite.statement.else satellite.statement.if (x == 1) { ... }
satellite.statement.else { ... }
''',
  r"""
**Checked 2026-09-12: an `else` followed directly by an `if` runs.** There is no
need to nest the `if` inside the `else`'s braces.

**This corrects the first version of this index**, whose row 8 said *nest an if
inside the else*, and `foreign.cpp`'s two `elif` rows say the same. That text is
out of date. The `foreign.cpp` half belongs to the interpreter session and is
recorded in [../CONTINUING.md](../CONTINUING.md) §3.
""")

E("`for x in xs:`", "1.0", "partial",
  r'''
for name in names:
    print(name)
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < names.size(); i = i + 1)
{
    satellite.variable.string name = names[i]
    satellite.console.display(name)
}
''',
  r"""
**Every walk over a container is an index loop plus a named element.** Checked
2026-09-12 in the CXX23 folder: `for (T x : l)` is refused, and Python's `for x
in xs` is the same missing loop.

**Partial, because the loop over elements is open.** It is
[CXX23 row 141](../CXX23/M141.md), which calls it the single row most likely to
matter to readers from any language. Two things make it cheaper than it looks:
satellite has two containers, and a list already knows its size.

The loop variable is scoped to the loop (row @{the loop variable after}@).
""")

E("`and` / `or` / `not`", "1.0", "M28",
  r'''
if x > 0 and not done:
''',
  r'''
satellite.statement.if (x > 0)
{
    satellite.statement.if (!done) { ... }
}
''',
  r"""
**`not` already exists, spelled `!`.** Checked 2026-09-12: `!ready` and
`!l.empty()` both work. The words `and`, `or` and `not` are all refused
(`S0201`), and so are `&&` and `||`. **`and` and `or` are M28.** Until then, nest
two ifs; for `or`, set a bool in each branch and test it.

The first version of this row sent all three to M28. `!` is not missing.
""")

E("`break` / `continue`", "1.0", "M27",
  r'''
for line in lines:
    if not line:
        continue
    if line == "END":
        break
''',
  r'''
// until M27: move the loop into its own capsule and return out of it
''',
  r"""
M27. **The cleanest workaround is `satellite.return` from inside the loop**, in
a capsule of its own; the CXX23 folder checked that a `return` inside a `for`
answers immediately ([CXX23 row 148](../CXX23/M148.md)). `continue` becomes an
`if` around the rest of the body.
""")

E("`lambda`", "1.0", "M32",
  r'''
names.sort(key=lambda n: n.lower())
''',
  None,
  r"""
M32. A capsule can be called and cannot be passed, so there is nothing for a
lambda to be. `list.sort_up(key)` already takes a key, which is the shape that
M32 generalises.
""")

E("`try` / `except`", "1.0", "M33",
  r'''
try:
    n = int(text)
except ValueError:
    n = 0
''',
  r'''
// until M33: ask first, then do it
satellite.statement.if (m.has(k)) { satellite.variable.number v = m.get(k) }
''',
  r"""
M33. **A refusal stops the run and cannot be caught.** Python's habit, *easier to
ask forgiveness than permission*, is the one that does not transfer: satellite is
built for *look before you leap*, and the refusals say which question to ask
first (`S0726` names `has(k)`, `S0716` and `S0728` name `contains(x)`, `S0729`
names `empty()`).

The one exception to asking first is `"abc".to_number()`, which refuses with
`S0610` and has no question to ask beforehand. See row @{`int(s)`}@.
""")

E("`match` statement", "3.10", "M30",
  r'''
match command:
    case "go": go()
    case "stop": stop()
    case _: unknown()
''',
  None,
  r"""
M30, and Python's `match` is **structural** pattern matching, which is more than
M30's `switch`. Rows @{`match` with class}@ to @{`match` mapping patterns}@ take the parts apart.
Today it is an `else if` chain.
""")

E("list comprehensions", "2.0", "M36",
  r'''
squares = [x * x for x in xs if x > 0]
''',
  r'''
satellite.container.list<satellite.variable.number> squares = satellite.container.list()
satellite.statement.for (satellite.variable.number i = 0; i < xs.size(); i = i + 1)
{
    satellite.variable.number x = xs[i]
    satellite.statement.if (x > 0) { squares.append(x * x) }
}
''',
  r"""
M36, which names list comprehensions among its foreign spellings and is eager,
which is what a comprehension is. M36 depends on M32, since a pipeline is made of
functions passed along.

**The syntax is still a decision.** M36's pipeline is method-shaped. Whether
satellite ever has the bracketed Python form is not recorded anywhere, and
square brackets already mean subscripts and slices.
""")

E("`self`", "1.0", "never",
  r'''
class Node:
    def attach(self, registry):
        registry.add(self)
''',
  None,
  r"""
**There is no `self` and no `this` anywhere in satellite.** Inside a spacesuit,
fields and capsules are named bare, so the everyday use of `self.` is simply
deleted. What cannot be written is **an object handing out a reference to
itself**.

The workaround is to *tell the object its own handle from outside*, once, by
whoever made it. `infinity_data_main.satl` does exactly this and explains the
price in a comment at the call site. [CXX23 row 160](../CXX23/M160.md) is the
C++ side of the same decision.

**Read this row first** if you are coming from Python. `self` is in every
method signature a Python programmer has ever written, and its absence changes
how objects refer to each other.
""")

E("decorators", "2.4", "M32",
  r'''
@lru_cache
def fib(n): ...
''',
  None,
  r"""
M32. A decorator is a function that takes a function and hands one back, so it
needs a capsule to be a value first. `foreign.cpp` answers `@` with the same
number. Class decorators and `@property` are separate rows
(@{class decorators}@, @{`@property`}@).
""")

E("generators and `yield`", "2.2", "never",
  r'''
def countdown(n):
    while n > 0:
        yield n
        n -= 1
''',
  r'''
satellite.capsule countdown(satellite.variable.number n) satellite.returns(satellite.container.list<satellite.variable.number>)
{
    satellite.container.list<satellite.variable.number> out = satellite.container.list()
    satellite.statement.while (n > 0)
    {
        out.append(n)
        n = n - 1
    }
    satellite.return(out)
}
''',
  r"""
**No generators are planned**, and `foreign.cpp` says so. Build the list and
answer it. A generator's other use, an endless stream consumed a piece at a time,
has no satellite spelling; M36 decides for eager pipelines and against laziness.
""")

E("f-strings", "3.6", "never",
  r'''
print(f"{name} has {count} items")
''',
  r'''
satellite.console.display(name + " has " + count + " items")
''',
  r"""
**There is no format string of any kind**: no f-strings, no `str.format`, no
`%` formatting (rows @{`str.format()`}@, @{`%` formatting}@). A line is built with
`+`, and a number joins onto a string directly.

**What is lost is formatting a number**, like `f"{price:.2f}"`, which is open
(row @{number formatting}@).
""")

E("duck typing", "1.0", "never",
  r'''
def total(things):
    return sum(t.price() for t in things)   # anything with price()
''',
  None,
  r"""
**Every declaration names its type**, so a capsule takes a `widget` and not
"whatever has `price()`". The satellite version of *anything with `price()`* is
an interface that says so, which is M31 plus the decision after it (row
@{abstract base classes}@, abstract base classes).

**But see row @{rebinding a name}@.** Checked 2026-09-12: a `number` variable accepted
a string assignment without a refusal, so the declared type is not yet enforced
everywhere it appears to be.
""")

# ---------------------------------------------------------------------------
section("Source text")

E("indentation as block structure", "1.0", "never",
  r'''
if ready:
    start()
    report()
''',
  r'''
satellite.statement.if (ready)
{
    start()
    report()
}
''',
  r"""
**Blocks are braces, and indentation means nothing to the parser.** A Python
reader's instinct to indent is still right for the reader, and it is how every
example in the tree is laid out.
""")

E("`#` comments", "1.0", "says it",
  r'''
# a note
x = 1  # trailing
''',
  r'''
// a note
satellite.variable.number x = 1   // trailing
''',
  r"""
Checked 2026-09-12: `# a comment` inside a capsule is `S0231`, and at the top of
a file it is `S0204`. **Only `//` comments exist.**

**That makes `//` the most dangerous two characters for a Python reader**, because
Python's floor division is a comment here. See row @{`//` floor division}@.
""")

E("docstrings", "1.0", "open",
  r'''
def area(r):
    """Area of a circle of radius r."""
''',
  r'''
// Area of a circle of radius r.
satellite.capsule area(satellite.variable.number r) satellite.returns(satellite.variable.number)
''',
  r"""
**A comment above the capsule is the only place for it.** `satellite.help(path)`
describes the language's own paths, from text written into the tree; nothing
reads a program's comments.

**Open: whether `satellite.help` should answer for a program's own capsules**,
and if so, where it reads the text from.
""")

E("line continuation `\\` and inside brackets", "1.0", "partial",
  r'''
total = first + \
        second
total = (first +
         second)
''',
  r'''
satellite.variable.number total = (first +
    second)
''',
  r"""
Checked 2026-09-12: **inside parentheses an expression may continue onto the
next line** (`(1 +` then `2)` printed `3`). **A backslash at the end of a line is
refused** (`S0231`).
""")

E("semicolons and several statements on one line", "1.0", "never",
  r'''
a = 1; b = 2
''',
  r'''
a = 1
b = 2
''',
  "A statement ends at the end of its line. Checked 2026-09-12: a trailing `;` is `S0203`, and the message says to put the second statement on its own line.")

E("`pass`", "1.0", "says it",
  r'''
if debug:
    pass
''',
  r'''
satellite.statement.if (debug)
{
}
''',
  "Checked 2026-09-12: an empty block is accepted. There is nothing to write inside it.")

E("`...` (Ellipsis) as a placeholder", "3.0", "never",
  r'''
def later(): ...
''',
  r'''
satellite.capsule later()
{
}
''',
  "An empty block is the placeholder. There is no Ellipsis object, and slices with `...` (a NumPy idiom) have nothing to index.")

E("Unicode identifiers", "3.0", "never",
  r'''
café = 1
π = 3.14159
''',
  r'''
satellite.variable.number cafe = 1
''',
  r"""
Checked 2026-09-12: `café` is refused, `S0203` at the `é`. Names are ASCII.
Strings may hold any bytes, including UTF-8 (row @{=`len(s)`}@).
""")

E("source encoding and `# -*- coding: -*-`", "2.3", "M45",
  r'''
# -*- coding: latin-1 -*-
''',
  None,
  "A satellite string is bytes with no declared encoding, and a source file is read the same way. Declaring what the bytes mean is M45.")

E("the shebang line `#!/usr/bin/env python3`", "1.0", "open",
  r'''
#!/usr/bin/env python3
''',
  None,
  r"""
**Checked 2026-09-12: refused.** A file whose first line is `#!/usr/bin/env satl`
stops with `S0204` at the `#`. A satellite program is run as `satl file.satl`.

**Open, and small.** Every scripting language skips a `#!` first line, and a
user who wants `./tool.satl` to run has no way to get it.
""")

E("the interactive prompt (REPL)", "1.0", "says it",
  r'''
$ python3
>>> 2 + 2
''',
  r'''
$ satl --repl
''',
  r"""
**`satl --repl` is the prompt.** Type satellite at it, `run <file>` to run one,
`exit` to leave. `satellite.system.persist(x)` turns off the prompt's memory of
declarations, which is on by default so that a name declared on one line is there
on the next.
""")

E("`python -c` and `python -m`", "1.0", "partial",
  r'''
python3 -c "print(2 ** 10)"
python3 -m http.server
''',
  r'''
satl --call tool.satl twice 21
satl --number 2 + 2
''',
  r"""
**`satl --call <file> <name> [n...]` runs one capsule and prints its answer**,
and `satl --number a op b` does exact arithmetic from the shell. There is no way
to run a line of satellite given on the command line, and no module to run with
`-m`, because a program is one file.
""")

# ---------------------------------------------------------------------------
section("Names, types and scope")

E("rebinding a name to a value of another type", "1.0", "open",
  r'''
x = 1
x = "one"      # fine in Python
''',
  r'''
satellite.variable.number x = 1
x = "one"      // checked: accepted
''',
  r"""
**Checked 2026-09-12: accepted.** A `satellite.variable.number` was assigned
`"abc"`, and `satellite.console.display(x)` printed `abc`. No refusal at check time
or at run time.

**Satellite says every declaration names its type** (row @{duck typing}@,
DESIGN §1's explicitness), and the other direction, a number assigned to a
string, is a documented conversion. A string assigned to a number is not.

**Open: is this a missing check or a decision?** A Python reader will not notice
it, which is exactly why it matters. Flagged for the interpreter session in
[../CONTINUING.md](../CONTINUING.md) §3.
""")

E("block scope, not function scope", "1.0", "says it",
  r'''
if found:
    where = i
print(where)       # fine in Python
''',
  r'''
satellite.variable.number where = 0
satellite.statement.if (found)
{
    where = i
}
satellite.console.display(where)
''',
  r"""
**A name lives until the end of the block it was declared in**, as in C++ and
Java, not until the end of the function, as in Python. Checked 2026-09-12: a
number declared inside an `if` is `S0511` on the line after the `}`.

Declare the name above the block and assign inside it. The refusal names the
cause, so this is a surprise once and not a bug.
""")

E("the loop variable after the loop", "1.0", "says it",
  r'''
for i in range(10):
    if hit(i): break
print(i)           # the last i, in Python
''',
  r'''
satellite.variable.number last = 0
satellite.statement.for (satellite.variable.number i = 0; i < 10; i = i + 1)
{
    last = i
}
''',
  "Checked 2026-09-12: `i` declared in a `for` header is `S0511` after the loop. Keep what you need in a name declared outside.")

E("`global`", "1.0", "says it",
  r'''
count = 0
def bump():
    global count
    count += 1
''',
  r'''
satellite.library.count = 0

satellite.capsule bump()
{
    satellite.library.count = satellite.library.count + 1
}
''',
  r"""
**A global is written with its whole path every time**, so every read and write
of shared state is visible at the line where it happens, and there is no
`global` declaration to forget. `satellite.library` is also shared by every
thread, with no lock (M40).
""")

E("`nonlocal`", "3.0", "never",
  r'''
def counter():
    n = 0
    def inc():
        nonlocal n
        n += 1
''',
  None,
  "There are no nested capsules (`S0213`), so there is no enclosing function scope to reach. A closure's captured state is M32's problem, and its page names it as the hard part.")

E("`del name`", "1.0", "never",
  r'''
del big_table
''',
  r'''
big_table.clear()
''',
  "A name lasts until its block ends and cannot be removed early. `clear()` empties a list, map, string or variant, and a spacesuit is freed when its last handle goes.")

E("an unassigned name", "1.0", "says it",
  r'''
x: int
print(x)          # NameError
''',
  r'''
satellite.variable.number x
satellite.console.display(x)     // nothing
''',
  "A declared name that was never assigned holds nothing and displays `nothing`. Python's `UnboundLocalError` has no satellite counterpart, and neither does reading garbage.")

E("chained assignment `a = b = 0`", "1.0", "never",
  r'''
a = b = 0
''',
  r'''
a = 0
b = 0
''',
  "Checked 2026-09-12: `S0203` at the second `=`. Assignment is a statement and not an expression, so it has no value to pass along.")

E("tuple unpacking and swapping `a, b = b, a`", "1.0", "M35",
  r'''
a, b = b, a
q, r = divmod(7, 2)
''',
  r'''
satellite.variable.number spare = a
a = b
b = spare
''',
  "Checked 2026-09-12: `a, b = b, a` is `S0203` at the comma. Taking a value apart in one line is M35, whose page names Python's `a, b =` among its spellings.")

E("starred unpacking `first, *rest = xs`", "3.0", "M35",
  r'''
first, *rest = items
''',
  r'''
satellite.variable.string first = items[0]
satellite.container.list<satellite.variable.string> rest = items[1:]
''',
  "The binding is M35, and **the slice already works**: checked 2026-09-12, `l[1:]` answered everything after the first element.")

E("the walrus operator `:=`", "3.8", "open",
  r'''
if (n := len(line)) > 80:
''',
  r'''
satellite.variable.number n = line.size()
satellite.statement.if (n > 80) { ... }
''',
  r"""
Checked 2026-09-12: an assignment inside a condition is `S0201`.

**Open, and it pairs with [CXX23 row 144](../CXX23/M144.md)** (an initialiser in
`if`). Both exist to take a declaration line away, and satellite puts every
declaration on its own line on purpose.
""")

E("constants by convention and `typing.Final`", "1.0 / 3.8", "partial",
  r'''
MAX_USERS: Final = 100
''',
  r'''
satellite.library.max_users = 100
''',
  "A library value from a literal is resolved at parse time, which is the useful part. **Nothing stops a later assignment to it.** There is no `const` (the CXX23 folder's row 8), and a constant computed from others is M34.")

E("`is`, `is not` and `id()`", "1.0", "partial",
  r'''
if a is b: ...
print(id(a))
''',
  r'''
satellite.statement.if (a == b) { ... }
''',
  r"""
Checked 2026-09-12: `is` is refused (`S0201`). **`==` compares values**, and it
works on lists (two equal lists answered `true`).

**Partial because identity cannot be asked.** Two spacesuit handles can refer to
one object (`b = a` shares), and nothing answers whether they do. `id()` has
nothing to answer with: there are no addresses. Comparing spacesuits at all is
M38.
""")

E("`x is None`", "1.0", "says it",
  r'''
if value is None:
''',
  r'''
satellite.statement.if (value.holds("nothing")) { ... }
''',
  "A variant answers `holds(\"nothing\")` and `holding()`. `value.holding() == \"nothing\"` is the same question.")

E("type hints on variables and parameters", "3.5 / 3.6", "partial",
  r'''
def greet(name: str) -> str:
count: int = 0
''',
  r'''
satellite.capsule greet(satellite.variable.string name) satellite.returns(satellite.variable.string)
satellite.variable.number count = 0
''',
  r"""
**In satellite the hints are not hints; they are required.** Every parameter,
answer and variable names its type. Passing a string where a capsule wants a
number is a check-time refusal.

**Partial, because of row @{rebinding a name}@:** plain assignment of a string to a
number variable was not refused.
""")

E("`mypy` and static type checking", "3.5 (PEP 484)", "says it",
  r'''
$ mypy app.py
''',
  r'''
$ satl --check app.satl
''',
  "`--check` runs every pass and reports everything wrong with a file without running it. It is part of the one tool, not a second program.")

E("`typing.Any`", "3.5", "says it",
  r'''
payload: Any = load()
''',
  r'''
satellite.variable.variant payload = 5
''',
  "A variant holds a value of any type and names it with `holding()`. **It does not forward methods**: checked 2026-09-12, `v.upper()` on a variant holding a string is `S0723`. Copy it into a typed name or use `held()`.")

E("`Optional[T]` and `T | None`", "3.5 / 3.10", "says it",
  r'''
def find(k) -> str | None:
''',
  r'''
satellite.capsule find(satellite.variable.string k) satellite.returns(satellite.variable.variant)
''',
  "A variant answers `holds(\"nothing\")`. What it does not say is *which* type it holds when it is not nothing; that is the next row.")

E("`Union[A, B]`", "3.5 / 3.10", "partial",
  r'''
Value = int | str
''',
  r'''
satellite.variable.variant value = 5
''',
  "A variant holds any type, not *only* A or B. A union restricted to named types, which a checker can hold a program to, is not a spelling.")

E("`type` statement and type aliases", "3.12", "open",
  r'''
type Grid = list[list[int]]
''',
  r'''
satellite.container.list<satellite.container.list<satellite.variable.number>> grid = satellite.container.list()
''',
  "Every type is spelled in full every time. **Open, and it is [CXX23 row 97](../CXX23/M97.md)**, which records the tension with DESIGN §1: a full path reads without looking anything up, and an alias is something to look up.")

E("generic classes and functions `class Box[T]`, `TypeVar`", "3.5 / 3.12", "M31",
  r'''
class Box[T]:
    def __init__(self, item: T): ...
def first[T](xs: list[T]) -> T: ...
''',
  None,
  "M31. The language's own containers are generic (`list<T>`, `map<K, V>`), and a program's spacesuits and capsules are not yet.")

E("`ParamSpec`, `TypeVarTuple`, `Concatenate`", "3.10 / 3.11", "M31",
  r'''
def logged[**P, R](f: Callable[P, R]) -> Callable[P, R]: ...
''',
  None,
  "These type decorators and variadic generics. They need M31 for the generics and M32 for the functions being typed.")

E("`typing.Protocol`", "3.8", "M31",
  r'''
class HasPrice(Protocol):
    def price(self) -> float: ...
''',
  None,
  "A structural interface. `foreign.cpp` sends Java's `interface` and Rust's `trait` to M31 and the decision after it, and this is the same question.")

E("`Callable[[int], str]`", "3.5", "M32",
  r'''
handler: Callable[[int], str]
''',
  None,
  "The type of a function value. `satellite.variable.capsule` is numbered and today holds only a call waiting to run on a thread; holding a capsule to call later is M32.")

E("`Literal`, `TypedDict`, `NewType`", "3.8 / 3.5", "partial",
  r'''
Mode = Literal["read", "write"]
class Point(TypedDict): x: int; y: int
UserId = NewType("UserId", int)
''',
  None,
  r"""
**`TypedDict` and `NewType` are a spacesuit**, a named type with named fields.
**`Literal` is an enumeration, which is open** ([CXX23 row 94](../CXX23/M94.md)).
The file modes are a live example: `satellite.file.open(path, "read")` takes a
word that nothing checks against a closed set before the run.
""")

E("`Self`, `@override` and `@final`", "3.11 / 3.12 / 3.8", "partial",
  r'''
class Derived(Base):
    @override
    def name(self) -> Self: ...
''',
  r'''
satellite.spacesuit derived(base)
{
    satellite.public
    {
        satellite.capsule name() satellite.returns(satellite.variable.string) { ... }
    }
}
''',
  r"""
Checked 2026-09-12: **a subclass capsule with the parent's name is the one
called** on the subclass (`derived` printed, not `base`). No `@override` is
written, and nothing checks that the name exists in the parent. `Self` needs
`self` (never), and `@final` has no dispatch to seal
([CXX23 row 178](../CXX23/M178.md)).
""")

E("`@typing.overload`", "3.5", "M39",
  r'''
@overload
def get(k: int) -> str: ...
@overload
def get(k: str) -> str: ...
''',
  None,
  "Two capsules with one name is M39.")

E("`isinstance()`, `type()`, `issubclass()`", "1.0 / 2.2", "partial",
  r'''
if isinstance(x, str): ...
print(type(x).__name__)
''',
  r'''
satellite.statement.if (x.holds("string")) { ... }
satellite.console.display(x.holding())
''',
  r"""
**A variant can say what it holds**: checked 2026-09-12, `holding()` answered
`number`, then `string`. **A spacesuit cannot be asked what it is**, and there is
no base handle to test against; that is open in
[CXX23 row 106](../CXX23/M106.md).
""")

E("`__annotations__`, `typing.get_type_hints`", "3.0", "M41",
  r'''
print(f.__annotations__)
''',
  None,
  "A program reading its own declarations is reflection, M41, which the CXX26 folder says satellite is better placed for than C++, because every word already has a number.")

# ---------------------------------------------------------------------------
section("Literals and built-in types")

E("`int`, arbitrary precision", "1.0 (unified 3.0)", "says it",
  r'''
print(2 ** 100)
''',
  r'''
satellite.variable.number two = 2
satellite.console.display(two.power(100))
''',
  r"""
**The one type Python and satellite agree on completely.** Both have exact
integers with no width. Checked 2026-09-12: `2.power(100)` answered
`1267650600228229401496703205376` exactly.

**It printed `1267650600228229401496703205376.0`, with a `.0`**, and `2.power(3)`
printed `8.0`, while `10 / 4` printed `2.5` and `7 % 3` printed `1`. A whole
number answered by `power` displays as a decimal. That is worth knowing, and
probably worth a look from the interpreter session.
""")

E("`float`", "1.0", "partial",
  r'''
x = 0.1 + 0.2
print(x == 0.3)     # False
''',
  r'''
satellite.variable.number x = 0.1 + 0.2
satellite.console.display(x == 0.3)     // true
''',
  r"""
**Satellite's `number` is exact decimal, so the classic float surprise does not
happen.** Checked 2026-09-12: `0.1 + 0.2 == 0.3` answered `true`, and `1 / 3`
goes to `satellite.library.system.division_digits` places. `sqrt(2)` printed 34
digits.

`satellite.variable.float` exists for when a float is wanted. Today it has only
conversions, which is why this row says partial.
""")

E("`complex` and `1j`", "1.0", "never",
  r'''
z = 3 + 4j
''',
  None,
  "Not a language type in satellite. A spacesuit with two numbers is the satellite version, and M38 would give it `+`. The CXX23 folder answers `std::complex` the same way.")

E("`decimal.Decimal`", "2.4", "says it",
  r'''
from decimal import Decimal, getcontext
getcontext().prec = 50
Decimal(1) / Decimal(3)
''',
  r'''
satellite.variable.number third = 1 / 3
''',
  "**This is what `satellite.variable.number` already is.** The precision of a division that does not end is `satellite.library.system.division_digits`, the same knob as `getcontext().prec`.")

E("`fractions.Fraction`", "2.6", "partial",
  r'''
Fraction(1, 3) + Fraction(1, 6)
''',
  r'''
satellite.variable.number sum = 1 / 3 + 1 / 6
''',
  "A number is exact while a division ends, and rounds to `division_digits` places when it does not. It does not keep `1/3` as a numerator and denominator, so a sum of thirds can differ in its last digit from the exact fraction.")

E("`bool` is a subclass of `int`", "2.3", "never",
  r'''
total = True + True    # 2
''',
  None,
  "Checked 2026-09-12: `satellite.bool.true + 1` is `S0711`: *`+` works on two numbers and this one is bool*. Counting trues is an `if` and a counter.")

E("`str` and its immutability", "1.0", "says it",
  r'''
s = "abc"
s.upper()        # s is unchanged
s += "d"         # a new string
''',
  r'''
satellite.variable.string s = "abc"
s.append("d")    // s is now abcd
''',
  r"""
**A satellite string can change in place.** Checked 2026-09-12: `s.append("d")`
changed `s`, while `s.upper()` answered `ABCD` and left `s` alone. So *methods
that answer something* behave like Python's, and `append` and `clear` are the
two that change the string.

**Passing a string to a capsule copies it.** A capsule that appended to its
parameter left the caller's string unchanged.
""")

E("single-quoted strings `'text'`", "1.0", "never",
  r'''
name = 'ada'
''',
  r'''
satellite.variable.string name = "ada"
''',
  "Checked 2026-09-12: `'hi'` is `S0231`. A string has one spelling, in double quotes, and there is no character type for single quotes to mean.")

E("triple-quoted strings", "1.0", "open",
  r'''
usage = """
satl tool
  --fast   go faster
"""
''',
  r'''
satellite.variable.string usage = "satl tool\n  --fast   go faster\n"
''',
  r'''
**Checked 2026-09-12: refused.** `"""hi"""` lexes as an empty string followed by a
second statement (`S0203`). A multi-line text is one string with `\n` in it, or
one `display` per line.

**Open, together with raw strings** (next row). Help text and usage messages are
where a program feels this.
''')

E("raw strings `r\"...\"`", "1.0", "open",
  r'''
pattern = r"\d+\.\d+"
''',
  None,
  "Escapes work (`\\n`, `\\t`, `\\\"` and `\\u00e9` were checked), so this only matters for text full of backslashes. **Open, and [CXX23 row 90](../CXX23/M90.md)** says it is not urgent until something like regex arrives.")

E("escape sequences `\\n`, `\\t`, `\\u00e9`", "1.0 / 3.0", "says it",
  r'''
print("a\tbé")
''',
  r'''
satellite.console.display("a\tbé")
''',
  r"""
Checked 2026-09-12: `\n`, `\t`, `\"` and `é` print as Python would.

**Satellite adds six live escapes** that Python does not have: `\home`, `\user`,
`\threads`, `\memtotal`, `\memused` and `\cwd` are answered when the string is
displayed, and `s.resolved()` answers them into a string a program can keep. A
Python string containing `\home` means something different here.
""")

E("`bytes`, `b\"...\"` and `bytearray`", "3.0", "partial",
  r'''
data = b"\x89PNG"
buf = bytearray(16)
''',
  r'''
satellite.variable.binary mask = b10001001
''',
  r"""
**`b` is already a prefix in satellite, and it means something else.** `b0101` is
a binary literal of type `satellite.variable.binary`. Checked 2026-09-12:
`b"abc"` is refused (`S0203`).

**Partial:** `binary` and `hex` carry bits with a width, and a string can hold
any bytes. There is no byte-sequence type and no binary file reading (row
@{binary mode}@).
""")

E("`memoryview` and the buffer protocol", "2.7", "never",
  r'''
view = memoryview(buf)[4:8]
''',
  None,
  "A view exists to avoid a copy, and satellite copies containers on assignment. The CXX23 folder answers `std::span` the same way.")

E("hex, binary and octal literals `0xff`, `0b1010`, `0o17`", "1.0 / 2.6", "partial",
  r'''
mask = 0xFF
bits = 0b1010
perms = 0o755
''',
  r'''
satellite.variable.hex mask = xFF
satellite.variable.binary bits = b1010
''',
  "**The prefix is a letter without the zero**, and the literal has its own type. Checked 2026-09-12: `0o17` is refused (`S0203`), and there is no octal type, which is why this row is partial.")

E("underscores in numbers `1_000_000`", "3.6", "open",
  r'''
n = 1_000_000
''',
  r'''
satellite.variable.number n = 1000000
''',
  "Checked 2026-09-12: `1_000` is `S0203`. **Open, and [CXX23 row 89](../CXX23/M89.md)**, which notes that `_` is what most languages chose and that arbitrary-precision numbers make long literals more common.")

E("scientific notation `1e3`", "1.0", "open",
  r'''
limit = 1e6
''',
  r'''
satellite.variable.string text = "1e6"
satellite.variable.number limit = text.to_number()
''',
  r"""
**Checked 2026-09-12: the language disagrees with itself.** `1e3` as a literal is
refused (`S0203`, *`e3` is a second statement*), yet `"1e3".to_number()` answered
`1000`, and `S0610`'s own text says a number is *digits, optionally a `.`, and
optionally an `e` and an exponent*.

**Open: the lexer should probably accept what `to_number` accepts.**
""")

E("`int(s)`, `float(s)`, `str(n)`", "1.0", "says it",
  r'''
n = int("42")
s = str(n)
''',
  r'''
satellite.variable.string text = "42"
satellite.variable.number n = text.to_number()
satellite.variable.string s = n.to_string()
''',
  r"""
One conversion serves integers and decimals. **A bad string refuses**: checked
2026-09-12, `"abc".to_number()` is `S0610`, which is Python's `ValueError`, and
it cannot be caught (M33). Unlike `has(k)` for maps, there is no question to ask
first, so a program reading numbers from a user has no way to avoid this
refusal. That is a real gap, recorded in row @{`try` / `except`}@.
""")

E("`hex()`, `bin()`, `oct()`, `int(s, base)`", "1.0 / 2.6", "partial",
  r'''
hex(255)    # '0xff'
bin(255)    # '0b11111111'
''',
  r'''
satellite.variable.number n = 255
satellite.console.display(n.hex())       // xFF
satellite.console.display(n.binary())    // b11111111
''',
  "Checked 2026-09-12: `n.hex()` and `n.binary()` answer satellite's own literal spellings. There is no octal, and no parsing a string in an arbitrary base.")

E("`ord()` and `chr()`", "1.0", "open",
  r'''
ord("A")   # 65
chr(65)    # 'A'
''',
  None,
  "**Not in `satl --words`.** There is no character type and no byte-to-number conversion on a string. Open, and it waits on M45 for what a *character* is beyond ASCII.")

E("`list`", "1.0", "says it",
  r'''
xs = []
xs.append(3)
''',
  r'''
satellite.container.list<satellite.variable.number> xs = satellite.container.list()
xs.append(3)
''',
  "One list, generic over its element type. A list of mixed types is `list<satellite.variable.variant>`: checked 2026-09-12, `1` and `\"two\"` in one list displayed `[1, two]`. **Assigning a list copies it**, which Python does not do (row @{assignment shares a list}@).")

E("list literals `[1, 2, 3]`", "1.0", "open",
  r'''
primes = [2, 3, 5, 7]
''',
  r'''
satellite.container.list<satellite.variable.number> primes = satellite.container.list()
primes.append(2)
primes.append(3)
primes.append(5)
primes.append(7)
''',
  "There is no list literal (`[1, 2, 3]` is `S0231`), so a table of constants costs a line per element. **Open, and [CXX23 row 117](../CXX23/M117.md)** calls it a strong candidate for a milestone.")

E("`tuple`", "1.0", "M35",
  r'''
point = (3, 4)
x, y = point
''',
  None,
  "A fixed group of values, usually taken apart at once. M35 names both halves: destructuring, and a capsule answering more than one value. Today it is a spacesuit with two fields.")

E("`dict`", "1.0", "says it",
  r'''
ages = {}
ages["ada"] = 36
''',
  r'''
satellite.container.map<satellite.variable.string, satellite.variable.number> ages = satellite.container.map()
ages["ada"] = 36
''',
  r"""
Checked 2026-09-12: `m["k"] = v` writes, `m["k"]` reads, and `m.set` / `m.get` are
the long spellings. **A map keeps insertion order**, as a Python 3.7 dict does:
keys set as `2` then `1` came back `[2, 1]`.

A missing key refuses with `S0726`, which is Python's `KeyError`. See row
@{`dict.get}@ for `get` with a default.
""")

E("dict literals `{\"a\": 1}`", "1.0", "open",
  r'''
limits = {"cpu": 4, "mb": 512}
''',
  r'''
satellite.container.map<satellite.variable.string, satellite.variable.number> limits = satellite.container.map()
limits["cpu"] = 4
limits["mb"] = 512
''',
  "Follows the list literal row. A spelling has to fit around `{ }` already meaning a block.")

E("`set` and `frozenset`", "2.4", "partial",
  r'''
seen = set()
if name not in seen:
    seen.add(name)
''',
  r'''
satellite.container.map<satellite.variable.string, satellite.variable.bool> seen = satellite.container.map()
satellite.statement.if (!seen.has(name)) { seen[name] = satellite.bool.true }
''',
  "A map with a throwaway value is a set, and `has(k)` is the membership test. Union, intersection and difference are loops (row @{set operations}@).")

E("`range()`", "1.0", "says it",
  r'''
for i in range(2, 20, 3):
''',
  r'''
satellite.statement.for (satellite.variable.number i = 2; i < 20; i = i + 3)
''',
  "The `for` header holds start, stop and step. There is no range *object* to store or pass.")

E("`object` as the root of every class", "2.2", "never",
  r'''
class Thing(object): ...
def keep(x: object): ...
''',
  r'''
satellite.variable.variant anything = 5
''',
  "A spacesuit with no superclass has none. \"Any value at all\" is a variant, and \"any spacesuit\" has no spelling until a base handle can refer to a derived suit ([CXX23 row 28](../CXX23/README.md)).")

# ---------------------------------------------------------------------------
section("Operators and expressions")

E("`/` true division", "3.0", "says it",
  r'''
7 / 2     # 3.5
1 / 0     # ZeroDivisionError
''',
  r'''
satellite.variable.number half = 7 / 2    // 3.5
''',
  "**The same as Python 3.** `7 / 2` is `3.5`. `1 / 3` is exact to `division_digits` places instead of a float. Division by zero is `S0601`, for `%` too (checked 2026-09-12).")

E("`//` floor division", "2.2", "partial",
  r'''
pages = total // per_page
''',
  r'''
satellite.variable.number ratio = total / per_page
satellite.variable.number pages = ratio.floor()
''',
  r"""
**THIS IS THE MOST DANGEROUS ROW IN THE PYTHON CATALOGUE.**

`//` starts a comment in satellite. Checked 2026-09-12:

    satellite.variable.number q = 7 // 2
    satellite.console.display(q)

**printed `7`, with no refusal and no warning.** Everything after `//` is a
comment, so the line is `q = 7`, and it is valid. `satl --check` has nothing to
report, and `foreign.cpp` never sees the line, because nothing went wrong.

Divide, name the answer, and take `.floor()` of it; the method has to go on a
name and not on the expression (M29). `.floor()` rounds toward minus infinity as
Python's `//` does (checked: `-3.5` floors to `-4`), and `.truncate()` rounds
toward zero.

**A check-time diagnostic is worth asking for**: a `//` that follows an operand
on the same line, with no space before it, is almost never a comment. Recorded
for the interpreter session in [../CONTINUING.md](../CONTINUING.md) §3.
""")

E("`%` with negative operands", "1.0", "partial",
  r'''
-7 % 3     # 2 in Python
7 % -3     # -2 in Python
''',
  r'''
satellite.variable.number a = -7
satellite.console.display(a % 3)      // -1
''',
  r"""
**The sign follows the dividend, as in C, not the divisor, as in Python.**
Checked 2026-09-12: `-7 % 3` is `-1` (Python says `2`), and `7 % -3` is `1`
(Python says `-2`). For non-negative operands the two agree.

This is silent. Code that wraps an index with `(i - 1) % n` works in Python for
`i = 0` and answers `-1` here, which then refuses as a list position (`S0725`).
""")

E("`divmod()`", "1.0", "says it",
  r'''
q, r = divmod(17, 5)
''',
  r'''
satellite.variable.number ratio = 17 / 5
satellite.variable.number q = ratio.floor()
satellite.variable.number r = 17 % 5
''',
  "Two lines, and a pair of answers is M35. Mind the previous row for negative numbers.")

E("`**` power", "1.0", "says it",
  r'''
area = r ** 2
''',
  r'''
satellite.variable.number area = r.power(2)
''',
  r"""
Checked 2026-09-12: `2 ** 10` is `S0231`. `power` is a method on the base and
takes **one** argument, the exponent (`x.power(a, b)` is `S0722`). The answer is
exact, and a whole-number answer currently displays with a trailing `.0` (row
@{`int`, arbitrary}@).
""")

E("`pow(base, exp, mod)`", "1.0", "partial",
  r'''
pow(7, 128, 13)
''',
  r'''
satellite.variable.number big = seven.power(128)
satellite.variable.number r = big % 13
''',
  "Exact, and slow for large exponents, since the whole power is built before the remainder is taken. There is no modular fast path.")

E("comparisons `== != < <= > >=`", "1.0", "says it",
  r'''
if a <= b: ...
if name < other: ...
''',
  r'''
satellite.statement.if (a <= b) { ... }
''',
  r"""
All six work on numbers. **On strings, only `==` and `!=` work.** Checked
2026-09-12: `"a" < "b"` is `S0712`, *string has no order*, while `list.sort()`
sorts strings. [CXX23 row 242](../CXX23/M242.md) records that contradiction as
open. Lists compare with `==`.
""")

E("chained comparisons `1 < x < 3`", "1.0", "M28",
  r'''
if 0 <= i < n: ...
''',
  r'''
satellite.statement.if (0 <= i)
{
    satellite.statement.if (i < n) { ... }
}
''',
  r"""
**Checked 2026-09-12: refused, with a misleading message.** `1 < x < 3` is
`S0712: < orders numbers, and bool has no order`, because it parses as
`(1 < x) < 3`. That is true about the parse and says nothing about what a Python
reader meant. It becomes `0 <= i && i < n` with M28.

A friendlier refusal, *satellite does not chain comparisons*, would be worth
asking the interpreter session for.
""")

E("`and` / `or` answering an operand, `x or default`", "1.0", "never",
  r'''
name = given or "anonymous"
''',
  r'''
satellite.variable.string name = given
satellite.statement.if (given.empty()) { name = "anonymous" }
''',
  "Python's `or` answers one of its operands, which only works because every value has a truth value. Satellite has no truthiness (next row), so M28's `||` will answer a bool.")

E("truthiness `if items:`", "1.0", "never",
  r'''
if items: ...
if not name: ...
if count: ...
''',
  r'''
satellite.statement.if (!items.empty()) { ... }
satellite.statement.if (name.empty()) { ... }
satellite.statement.if (count != 0) { ... }
''',
  r"""
**Satellite has no truthiness, and says so.** Checked 2026-09-12: a number, a
list or a string as a condition is `S0710: a condition is a satellite.variable.bool
and this one is number -- satellite has no truthiness`.

This is a deliberate refusal, and a good one: it is a check-time error rather
than a silent wrong branch, and the question it asks you to write (`empty()`,
`!= 0`) is the one you meant.
""")

E("`in` and `not in`", "1.0", "says it",
  r'''
if "bolt" in parts: ...
if key not in table: ...
if "err" in line: ...
''',
  r'''
satellite.statement.if (parts.contains("bolt")) { ... }
satellite.statement.if (!table.has(key)) { ... }
satellite.statement.if (line.contains("err")) { ... }
''',
  r"""
Checked 2026-09-12: `1 in l` is refused (`S0201`). Each container says it with a
method: **`contains(x)` on a list or a string, `has(k)` on a map.** A list's
`contains` is exact (`contains("app")` over `["apple"]` answered `false`).
""")

E("the conditional expression `a if c else b`", "2.5", "M37",
  r'''
label = "on" if enabled else "off"
''',
  r'''
satellite.variable.string label = "off"
satellite.statement.if (enabled) { label = "on" }
''',
  "M37, whose page names Python's spelling.")

E("bitwise `&`, `|`, `^`, `~`", "1.0", "open",
  r'''
flags = flags | READY
''',
  None,
  "Refused as operators, and no and/or/xor method is in the word tree. **Open, and [CXX23 row 128](../CXX23/M128.md)**, which suggests methods on `binary` and `hex` fit better than operators beside M28's `&&`.")

E("shifts `<<` and `>>`", "1.0", "says it",
  r'''
big = 1 << 10
''',
  r'''
satellite.variable.number one = 1
satellite.variable.number big = one.shift_left(10)
''',
  "`shift_left(n)` and `shift_right(n)` are number methods, and the operators are refused.")

E("`@` matrix multiplication", "3.5", "never",
  r'''
c = a @ b
''',
  None,
  "Satellite has no matrix type and no NumPy. `@` is not an operator and never will be one in the way Python's decorators use it either (M32 decides decorators).")

E("augmented assignment `+=`, `-=`, `*=`", "2.0", "open",
  r'''
total += x
''',
  r'''
total = total + x
''',
  "Refused (`S0231`). **Open, and [CXX23 row 126](../CXX23/M126.md)**, which calls it the most frequent verbosity in a satellite loop and cheap: a parser rewrite with no evaluator change. Python has no `++`, so this is the one row a Python reader misses.")

E("`+` joining a string and a number", "1.0", "says it",
  r'''
"total: " + str(n)     # Python needs str()
"total: " + n          # TypeError
''',
  r'''
satellite.variable.string line = "total: " + n
''',
  "**The Python `TypeError` does not happen here.** Checked 2026-09-12: `\"apples: \" + 5` and `5 + \" apples\"` both built a string. This is what keeps satellite's display lines short without format strings.")

E("repetition `\"-\" * 40` and `[0] * n`", "1.0", "open",
  r'''
print("-" * 40)
row = [0] * width
''',
  r'''
satellite.variable.string rule = ""
satellite.statement.for (satellite.variable.number i = 0; i < 40; i = i + 1)
{
    rule.append("-")
}
''',
  r"""
**Checked 2026-09-12: refused.** `"ab" * 3` is `S0711: * works on two numbers and
this one is string`.

**Open, together with list `+`** (next row). Both are *operators on something
that is not a number*, and M38 limits even user operators to `+`, `==` and `<`.
""")

E("list concatenation `a + b` and `extend`", "1.0", "open",
  r'''
both = first + second
first.extend(second)
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < second.size(); i = i + 1)
{
    first.append(second[i])
}
''',
  "Checked 2026-09-12: `a + a` on lists is `S0711`, and there is no `extend` in the word tree. **Open**: either a method (`extend`, which fits satellite's method style) or `+` on lists.")

E("operator precedence and parentheses", "1.0", "says it",
  r'''
x = (1 + 2) * 3
''',
  r'''
satellite.variable.number x = (1 + 2) * 3
''',
  "Checked in the CXX23 folder: prints `9`. Arithmetic precedence is the conventional one.")

E("`eval()` and `exec()`", "1.0", "open",
  r'''
result = eval("2 + 2")
exec(source)
''',
  None,
  r"""
**Nothing runs text as code.** `satellite.variable.expression` is numbered and has
nothing built behind it: checked 2026-09-12, `satellite.help` on it is `S1101`.
What it is meant to be is not something this folder can say, so do not assume it
is `eval`.

**Open.** The REPL (`satl --repl`) already reads satellite a line at a time,
which is the machinery `eval` would need.
""")

E("`compile()`, `ast`, `dis`", "2.2 / 2.5", "partial",
  r'''
import ast, dis
tree = ast.parse(source)
dis.dis(f)
''',
  r'''
$ satl --tokens app.satl
$ satl --unparse app.satl
$ satl --compile app.satl
''',
  "**The interpreter shows its own stages from the command line**: tokens, the parsed tree printed back as source, the resolved frames, and the closure tree the evaluator runs. A program cannot get any of them as a value (M41).")

# ---------------------------------------------------------------------------
section("Subscripts and slices")

E("negative indices `xs[-1]`", "1.0", "says it",
  r'''
last = xs[-1]
''',
  r'''
satellite.variable.number last = xs[-1]
''',
  "**Checked 2026-09-12: works, as in Python**, on lists and on strings. `l.last()` is the long spelling.")

E("slices `xs[a:b]`, `xs[a:]`, `xs[:b]`", "1.0", "says it",
  r'''
middle = xs[1:3]
tail = xs[1:]
head = s[:5]
''',
  r'''
satellite.container.list<satellite.variable.number> middle = xs[1:3]
satellite.container.list<satellite.variable.number> tail = xs[1:]
satellite.variable.string head = s[0:5]
''',
  r"""
**Checked 2026-09-12: slices work, on lists and on strings, with Python's
meaning.** `[10, 20, 30, 40][1:3]` is `[20, 30]`; `[1:]` and `[:1]` both work;
`"abcdef"[1:3]` is `"bc"`. **A slice clamps where an index refuses**, which is also
Python's rule: `[0:99]` answered the whole list, and `[5]` on a one-element list is
`S0725`.

Note that `substring(start, end)` takes an end, like a slice, and not a length.
""")

E("slice steps `xs[::2]`, `xs[::-1]`", "2.3", "partial",
  r'''
evens = xs[::2]
backwards = xs[::-1]
''',
  r'''
satellite.container.list<satellite.variable.number> backwards = xs
backwards.reverse()
''',
  "Checked 2026-09-12: a third slice part is `S0201`. **Reversing is `reverse()` on a copy**, which is the common use of `[::-1]`; any other step is an index loop.")

E("slice assignment and `del xs[a:b]`", "1.0", "open",
  r'''
xs[1:3] = [9, 9]
del xs[1:3]
''',
  None,
  r"""
**Checked 2026-09-12: refused with a clear reason**: `S0720: a slice names a run
of elements and not one place, so there is nothing for a single value to be
written into`. `l[i] = v` works for one position, and `remove_at(n)` removes one.

**Open.** The refusal says *does not run yet*, so the parse exists and the
meaning is undecided.
""")

E("`IndexError`", "1.0", "says it",
  r'''
xs[5]     # IndexError: list index out of range
''',
  r'''
satellite.statement.if (5 < xs.size()) { ... xs[5] ... }
''',
  "Checked 2026-09-12: `S0725: l was asked about position 5 and this list has 1 elements`. It cannot be caught (M33), so compare with `size()` first.")

E("subscripting a list with a string", "—", "says it",
  r'''
parts["olt"]     # TypeError in Python
''',
  r'''
satellite.system.threshold(6)
satellite.container.list<satellite.variable.string> found = parts["olt"]
''',
  r"""
**This is not an error in satellite; it is a search.** A string in square
brackets after a list answers the values that match it, and
`satellite.system.threshold` says how loose a match may be (`example/containers.satl`:
at 6, `"olt"` finds `"bolt"`). `parts.search(pattern)` is the rich form, a map per
hit with value, key, path and score.

**Worth knowing because a Python typo will not be caught**: a list subscripted
with a string that was meant to be a map key answers a (possibly empty) list.
""")

# ---------------------------------------------------------------------------
section("Control flow")

E("`while`", "1.0", "says it",
  r'''
while n < 3:
    n += 1
''',
  r'''
satellite.statement.while (n < 3)
{
    n = n + 1
}
''',
  "Checked 2026-09-12: printed `3`.")

E("`for i in range(len(xs))`", "1.0", "says it",
  r'''
for i in range(len(xs)):
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < xs.size(); i = i + 1)
''',
  "The one loop Python readers write that satellite spells directly. `len(x)` is `x.size()`, and `foreign.cpp` answers both `len(` and `range(`.")

E("`enumerate()`", "2.3", "says it",
  r'''
for i, name in enumerate(names):
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < names.size(); i = i + 1)
{
    satellite.variable.string name = names[i]
}
''',
  "The index loop is `enumerate`. The two-name binding is M35.")

E("`zip()`", "2.0", "M36",
  r'''
for name, age in zip(names, ages):
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < names.size(); i = i + 1)
{
    satellite.variable.string name = names[i]
    satellite.variable.number age = ages[i]
}
''',
  "M36 (which lists C++23's `views::zip`). One index over two lists works today; Python's `zip` stops at the shorter list, so check both sizes.")

E("`reversed()`", "2.4", "says it",
  r'''
for x in reversed(xs):
''',
  r'''
satellite.statement.for (satellite.variable.number i = xs.size() - 1; i >= 0; i = i - 1)
''',
  "Count down, or `reverse()` a copy.")

E("`for … else` and `while … else`", "1.0", "M27",
  r'''
for x in xs:
    if bad(x): break
else:
    print("all good")
''',
  None,
  "The `else` runs when the loop did **not** `break`, so it means nothing without `break` (M27). When M27 lands, whether satellite adopts the `else` form is a separate decision; a bool set before the `break` is the portable spelling.")

E("`return` from inside a loop", "1.0", "says it",
  r'''
for x in xs:
    if x == target:
        return True
''',
  r'''
satellite.statement.for (satellite.variable.number i = 0; i < xs.size(); i = i + 1)
{
    satellite.statement.if (xs[i] == target) { satellite.return(satellite.bool.true) }
}
''',
  "Checked in the CXX23 folder. **It is the workaround for `break`** until M27.")

E("`match` with class and sequence patterns", "3.10", "M30",
  r'''
match shape:
    case Point(x=0, y=0): ...
    case [first, *rest]: ...
''',
  None,
  "Destructuring inside a case needs M30 and M35 together. Class patterns also need a way to ask a suit what it is (row @{`isinstance()`}@).")

E("`match` guards `case x if x > 0`", "3.10", "M30",
  r'''
case n if n > 100:
''',
  None,
  "Part of M30's decision about what a case may test.")

E("`match` mapping patterns and `case _`", "3.10", "M30",
  r'''
case {"action": "go", "speed": s}: ...
case _: ...
''',
  None,
  "`case _` is M30's default. Mapping patterns ask a map for keys and bind the values, which is M30 plus M35.")

E("`assert`", "1.0", "M42",
  r'''
assert count >= 0, "count went negative"
''',
  None,
  "M42 (contracts). **Python removes `assert` under `-O`; satellite's would not**, since satellite has no build modes. The statement form is the smaller half and could come first.")

E("`raise`", "1.0", "M33",
  r'''
raise ValueError("bad port")
''',
  None,
  "**A program cannot refuse on purpose.** A refusal comes from the language, with a code, a caret and a call stack; there is no statement a program writes to stop itself with a message. `foreign.cpp` answers `raise ` with M33, and M42 covers the checked-condition form.")

E("`try … finally`", "1.0", "M33",
  r'''
f = open(path)
try:
    process(f)
finally:
    f.close()
''',
  r'''
satellite.variable.file f = satellite.file.open(path, "read")
process(f)
f.close()
''',
  "With no way to catch a refusal, there is also no cleanup that runs *because of* one: a refusal ends the whole run. `finally` belongs to M33's design.")

E("`try … except … else`", "2.5", "M33",
  r'''
try:
    n = int(text)
except ValueError:
    n = 0
else:
    log(n)
''',
  None,
  "The `else` branch runs when nothing was raised. It follows M33.")

E("custom exception classes", "1.0", "M33",
  r'''
class ConfigError(Exception): ...
''',
  None,
  "M33 is deliberately an *answer-or-refusal value* (C++23's `std::expected`) rather than a hierarchy caught by type. A Python reader should expect to inspect a refusal, not to catch it by class.")

E("`raise … from` and exception chaining", "3.0", "M33",
  r'''
raise ConfigError("bad file") from err
''',
  None,
  "Carrying a cause inside a refusal. Before M33 there is no refusal value to carry one.")

E("exception groups and `except*`", "3.11", "M33",
  r'''
except* (OSError, ValueError) as group:
''',
  None,
  "Several failures at once, usually from concurrent tasks. It needs M33 for the failures and M43 for the tasks.")

E("the built-in exceptions and what each becomes", "1.0", "partial",
  r'''
KeyError, IndexError, ZeroDivisionError, ValueError, FileNotFoundError
''',
  r'''
satellite.statement.if (m.has(k)) { ... }              // KeyError          -> S0726
satellite.statement.if (i < l.size()) { ... }          // IndexError        -> S0725
satellite.statement.if (d != 0) { ... }                // ZeroDivisionError -> S0601
satellite.statement.if (f.ok()) { ... }                // FileNotFoundError -> no refusal
''',
  r"""
**Every common Python exception has a satellite refusal, with a code and a
sentence naming the question to ask first.** Checked 2026-09-12:

| Python | satellite | ask first |
|---|---|---|
| `KeyError` | `S0726` | `has(k)` |
| `IndexError` | `S0725` | `size()` |
| `ZeroDivisionError` | `S0601` | `!= 0` |
| `ValueError` from `int()` | `S0610` | nothing (row @{`int(s)`}@) |
| `ValueError` from `list.index` / `remove` | `S0728` | `contains(x)` |
| `ValueError` from `max([])` | `S0729` | `empty()` |
| `str.find` returning `-1` | `S0716` | `contains(x)` |
| `FileNotFoundError` | **not a refusal** | `f.ok()`, then `f.error()` |
| `RecursionError` | not reached in 20 s | (row @{recursion and the recursion limit}@) |

**Two differ from Python in shape.** `s.find` refuses where Python answers `-1`.
Opening a missing file does not stop anything: `ok()` answered `false` and
`error()` answered `No such file or directory`, which is the pattern M33 wants to
generalise.

`satl --errors` lists every code and what it says.
""")

E("tracebacks", "1.0", "says it",
  r'''
Traceback (most recent call last):
  File "app.py", line 12, in load
''',
  r'''
satl: app.satl:12:31: error S0726: this map holds nothing under zz ...
   12 |     satellite.console.display(m["zz"])
      |                               ^^^^
       in load, called at line 30
       in satellite.main
''',
  "**Every refusal prints file, line, column, a caret under the exact expression, and the call stack.** A program cannot get the stack as a value.")

E("`warnings.warn`", "2.1", "open",
  r'''
warnings.warn("config is old", DeprecationWarning)
''',
  None,
  "The tree has a diagnostic severity that does not stop the run, and a program has no way to emit one. **Open**, alongside the stderr row @{`sys.stderr`}@.")

E("`KeyboardInterrupt`", "1.0", "never",
  r'''
try:
    run()
except KeyboardInterrupt:
    save()
''',
  None,
  "Ctrl-C belongs to satellite, which handles it for the program (the CXX23 folder's signals row, DESIGN §10.2). `satellite.time.sleep` says Ctrl-C still reaches a sleeping program.")

E("`with` statements and context managers", "2.5", "partial",
  r'''
with open(path) as f:
    data = f.read()
''',
  r'''
satellite.variable.file f = satellite.file.open(path, "read")
satellite.variable.string data = f.read_all()
f.close()
''',
  r"""
**Open it, use it, close it.** `foreign.cpp` answers `with ` this way: there is no
scope guard, and `close()` is explicit.

**Partial, because nothing closes on the way out of a block.** In practice a
refusal ends the whole run, so the failure case `with` protects in Python is the
end of the process here. Writing your own context manager (`__enter__`,
`__exit__`, `contextlib.contextmanager`) has no satellite counterpart.
""")

E("`async def` and `await`", "3.5", "M43",
  r'''
async def fetch(url):
    return await client.get(url)
''',
  None,
  "Composing concurrent work is M43, and M43 must not start before M40. Satellite's concurrency today is threads (row @{`threading.Thread`}@).")

E("`async for`, `async with`, async generators", "3.5 / 3.6", "M43",
  r'''
async with session:
    async for item in stream: ...
''',
  None,
  "Follows `async def`. The generator half is also row 18's *never*.")

E("`asyncio` event loop, tasks and `gather`", "3.4", "M43",
  r'''
results = await asyncio.gather(a(), b())
''',
  r'''
satellite.variable.thread ta = satellite.thread.new(a())
satellite.variable.thread tb = satellite.thread.new(b())
ta.start()
tb.start()
satellite.variable.number ra = ta.join()
satellite.variable.number rb = tb.join()
''',
  "**`gather` over two calls is two threads and two joins today**, and `join()` answers what the capsule returned. The event loop itself is M43.")

E("`yield from` and generator delegation", "3.3", "never",
  r'''
yield from inner()
''',
  None,
  "Follows row 18: no generators are planned.")

E("generator expressions", "2.4", "M36",
  r'''
total = sum(x * x for x in xs)
''',
  r'''
satellite.variable.number total = 0
satellite.statement.for (satellite.variable.number i = 0; i < xs.size(); i = i + 1)
{
    total = total + xs[i] * xs[i]
}
''',
  "M36, eagerly: a pipeline that builds its answer. The *lazy* half, never materialising the sequence, is what M36 decides against.")

E("dict and set comprehensions", "2.7", "M36",
  r'''
index = {name: i for i, name in enumerate(names)}
''',
  None,
  "The same as list comprehensions (row 15), producing a map.")

E("the iterator protocol, `iter()`, `next()`", "2.2", "never",
  r'''
it = iter(xs)
first = next(it)
''',
  r'''
satellite.variable.string first = xs.first()
''',
  "No iterators; index instead. The CXX23 folder's row 16 gives the same answer, and it is why a `for` over elements (row 9) is being asked for *without* a protocol behind it.")

E("`itertools`", "2.3", "M36",
  r'''
from itertools import chain, groupby, islice, product
''',
  None,
  "The finite tools (`chain`, `product`, `groupby`, `accumulate`) are M36 pipelines. The infinite ones (`count`, `cycle`, `repeat`) are lazy and are not planned.")

E("recursion and the recursion limit", "1.0", "partial",
  r'''
def fact(n):
    return 1 if n < 2 else n * fact(n - 1)
sys.setrecursionlimit(5000)
''',
  r'''
satellite.capsule fact(satellite.variable.number n) satellite.returns(satellite.variable.number)
{
    satellite.statement.if (n < 2) { satellite.return(1) }
    satellite.variable.number m = n - 1
    satellite.variable.number sub = fact(m)
    satellite.return(n * sub)
}
''',
  r"""
**Recursion works and is exact**: the CXX23 folder checked `fact(30)`.

**Partial, because runaway recursion did not stop.** Checked 2026-09-12: a capsule
that called itself with no base case was still running after 20 seconds, with no
output, and was killed. Python raises `RecursionError` at depth 1000.

**The reason is in `satl --limits`**: `max_depth` is listed as `unset`. The limit
exists (`satellite.library.system.max_depth`) and nothing sets it by default, so an
endless recursion runs until memory gives out. Set it when a program recurses on
input it did not choose.
""")

# ---------------------------------------------------------------------------
section("Functions")

E("default argument values", "1.0", "M39",
  r'''
def greet(name="you"):
''',
  None,
  "Checked 2026-09-12: `name = \"you\"` in a parameter list is `S0201`. M39, whose page names Python's default arguments. Until then, every argument is given every time.")

E("keyword arguments `f(name=\"x\")`", "1.0", "M39",
  r'''
connect(host="ada", port=8080)
''',
  None,
  "Checked 2026-09-12: `greet(name = \"x\")` is `S0201`. M39 names keyword arguments (Python) among its spellings.")

E("keyword-only `*` and positional-only `/` parameters", "3.0 / 3.8", "M39",
  r'''
def f(a, /, b, *, c): ...
''',
  None,
  "These only mean something once keyword arguments exist, so they follow M39.")

E("`*args`", "1.0", "never",
  r'''
def total(*values):
''',
  r'''
satellite.capsule total(satellite.container.list<satellite.variable.number> values) satellite.returns(satellite.variable.number)
{
    satellite.return(values.sum())
}
''',
  "**Pass a list.** A capsule's argument count is part of its identity (PLAN.md §8 on `satellite.network`: *the arity is the identity*). A type-safe variadic generic is M31's territory.")

E("`**kwargs`", "1.0", "never",
  r'''
def configure(**options):
''',
  r'''
satellite.capsule configure(satellite.container.map<satellite.variable.string, satellite.variable.string> options)
''',
  "**Pass a map.** Same reason as `*args`.")

E("unpacking into a call `f(*xs)`, `f(**d)`", "2.0", "never",
  r'''
point(*coords)
''',
  None,
  "Follows `*args`: satellite does not spread a container across parameters.")

E("returning several values `return a, b`", "1.0", "M35",
  r'''
def span(xs):
    return min(xs), max(xs)
''',
  None,
  "A capsule answers exactly one value. M35 names *a capsule answering more than one value*. Today the answer is a small spacesuit, or two capsules.")

E("functions as values and passing a function", "1.0", "M32",
  r'''
def apply(f, x):
    return f(x)
apply(len, "abc")
''',
  None,
  "M32.")

E("closures", "2.2", "M32",
  r'''
def make_adder(n):
    def add(x):
        return x + n
    return add
''',
  None,
  "M32, and its page says a capture that outlives its frame is the hard part.")

E("nested functions", "1.0", "never",
  r'''
def outer():
    def helper(): ...
''',
  None,
  "Checked 2026-09-12 in the CXX23 folder: `S0213: satellite.capsule is a declaration and goes at the top of a file, not inside a block`. Write the helper beside the capsule that uses it.")

E("`functools.partial`", "2.5", "M32",
  r'''
greet_ada = partial(greet, "ada")
''',
  None,
  "Partial application builds a callable value. M32.")

E("`functools.lru_cache` and `cache`", "3.2 / 3.9", "M32",
  r'''
@cache
def fib(n): ...
''',
  r'''
satellite.library.fib_seen = satellite.container.map()
''',
  "As a decorator it is M32. **The memo itself is a map today**: `has(n)`, else compute and `set`. `satellite.library` makes it outlive the call.")

E("`map()`, `filter()`, `functools.reduce()`", "1.0", "M36",
  r'''
lengths = list(map(len, words))
evens = list(filter(is_even, xs))
''',
  None,
  "M36, which depends on M32. For the most common `reduce`, a sum, `xs.sum()` exists (row @{`min()`, `max()`, `sum()`}@).")

E("`sorted()` and `list.sort(key=…, reverse=…)`", "2.4", "partial",
  r'''
xs.sort(reverse=True)
people.sort(key=lambda p: p.age)
''',
  r'''
xs.sort("down")
xs.sort_up(key)
''',
  r"""
`sort()`, `sort("down")`, `sort_up(key)` and `sort_down(key)` sort **in place**.
`satellite.help` says strings sort as text and numbers as numbers. **A key
function is M32**, and sorting spacesuits by their own `<` is M38. `sorted()`,
which answers a new list, is assignment (a copy) followed by `sort()`.
""")

E("`min()`, `max()`, `sum()`", "1.0", "says it",
  r'''
biggest = max(xs)
total = sum(xs)
''',
  r'''
satellite.variable.number biggest = xs.max()
satellite.variable.number total = xs.sum()
''',
  r"""
Methods on the list. Checked 2026-09-12: `sum()` of an empty list is `0`, and
`max()` of an empty list is `S0729`, the same as Python's `ValueError`. Two
numbers are `a.max(b)` and `a.min(b)`. A `key=` function is M32.
""")

E("`any()` and `all()`", "2.5", "M32",
  r'''
if any(x < 0 for x in xs):
''',
  r'''
satellite.statement.if (xs.contains(0)) { ... }
''',
  "They take a condition, so M32. **`contains(x)` is `any` for one exact value.**")

E("`abs()` and `round()`", "1.0", "partial",
  r'''
abs(-3)
round(2.5)       # 2  (to even)
round(3.14159, 2)
''',
  r'''
satellite.variable.number n = -3
satellite.console.display(n.abs())
satellite.variable.number h = 2.5
satellite.console.display(h.round())    // 3
''',
  r"""
**`round` disagrees with Python on halves.** Checked 2026-09-12: `2.5.round()` is
`3`. Satellite rounds a half **away from zero** (so `-2.5` is `-3`), and
`satellite.help` says that choice is written down on purpose. Python 3 rounds a half
**to even**: `round(2.5)` is `2`, `round(3.5)` is `4`.

**Partial, because there is no `ndigits`.** `round` takes no argument, so rounding
to two places is multiply, round, divide (row @{number formatting}@).
""")

E("argument passing: what a function can change", "1.0", "says it",
  r'''
def fill(out):
    out.append(1)      # the caller sees it
''',
  r'''
satellite.capsule fill(bag target)      // a spacesuit: the caller sees changes
''',
  r"""
**The rule is per type, and it is not Python's.** In Python every argument is a
shared reference, so a function that appends to a list changes the caller's list.
Checked 2026-09-12 in satellite:

- **a number** changed inside a capsule: caller unchanged
- **a string** appended to inside a capsule: caller unchanged
- **a list or a map**: caller unchanged ([CXX23 row 9](../CXX23/M9.md) measured
  five appends inside and zero outside, with no diagnostic)
- **a spacesuit**: the caller's object changes, because a suit is a handle

So **Python's `def fill(out): out.append(x)` silently does nothing** here. Answer
the list and assign it, or put it in a spacesuit and pass that. This is the
single most important Python row after `self` and `//`.
""")

E("the mutable default argument trap", "1.0", "never",
  r'''
def add(x, into=[]):   # one list shared by every call
''',
  None,
  "Cannot happen: there are no default arguments yet (M39), and a list is copied rather than shared. When M39 lands, a default that is a fresh value per call is the behaviour to keep.")

E("a function with no `return` answers `None`", "1.0", "says it",
  r'''
def log(msg):
    print(msg)
''',
  r'''
satellite.capsule log(satellite.variable.string msg)
{
    satellite.console.display(msg)
}
''',
  "A capsule with no `satellite.returns` answers nothing and cannot be used as a value. A capsule that does answer can be called as a bare statement and the answer is dropped silently ([CXX23 row 73](../CXX23/M73.md), open).")

E("`callable()`", "1.0", "never",
  r'''
if callable(handler):
''',
  None,
  "Nothing but a capsule can be called, and a capsule is not a value (M32).")

E("`help()` and `dir()`", "1.0", "partial",
  r'''
help(str.split)
dir(obj)
''',
  r'''
satellite.help(satellite.variable.string.split)
$ satl --words satellite.variable.string
''',
  "**The language's own paths are richly documented from inside a program and from the shell.** `satellite.help(path)` prints what one does with an example, and `satl --words <path>` lists its children. A program's own capsules and suits are not described by either (row @{docstrings}@).")

# ---------------------------------------------------------------------------
section("Classes and objects")

E("`__init__` with parameters", "1.0", "partial",
  r'''
class Point:
    def __init__(self, x, y): ...
p = Point(1, 2)
''',
  r'''
point p
p.set_x(1)
p.set_y(2)
''',
  r"""
**A spacesuit is declared and then set.** A parameter list on a spacesuit's name
is read as its superclass, so constructor arguments are not a spelling yet
(checked in the CXX23 folder: `S0201`). Fields start at their declared defaults.

**Partial and open**, and the same decision as CXX23 row 27 and
[row 115](../CXX23/M115.md).
""")

E("attributes added at run time `obj.extra = 1`", "1.0", "never",
  r'''
p.label = "origin"      # a new attribute, from outside
''',
  None,
  "A suit's fields are the ones declared in it. The object's shape is fixed by its declaration, which is also why `__slots__` has nothing to do (row @{`__slots__`}@).")

E("public attributes `obj.x`", "1.0", "never",
  r'''
print(p.x)
p.x = 5
''',
  r'''
satellite.variable.number x = p.get_x()
p.set_x(5)
''',
  "**A field is reached only through a capsule**, even a public one: `tally.count` is `S0517` (`example/spacesuits.satl`). A getter and a setter per field is the price, paid on purpose (DESIGN §12).")

E("class attributes", "1.0", "partial",
  r'''
class Counter:
    made = 0
''',
  r'''
satellite.library.counters_made = 0
''',
  "There is no per-class storage. A library value is per program, which is the same thing when one type needs it.")

E("`@property`", "2.2", "says it",
  r'''
@property
def area(self): return self.w * self.h
''',
  r'''
satellite.capsule area() satellite.returns(satellite.variable.number)
{
    satellite.return(w * h)
}
''',
  "A capsule is the only way to read a field, so every attribute is already a property, and it is called with `()`.")

E("`@staticmethod`", "2.2", "says it",
  r'''
class Maths:
    @staticmethod
    def twice(x): return 2 * x
''',
  r'''
satellite.capsule twice(satellite.variable.number x) satellite.returns(satellite.variable.number)
''',
  "A capsule at file scope. There is no reason to attach it to a type.")

E("`@classmethod` and alternative constructors", "2.2", "partial",
  r'''
@classmethod
def from_string(cls, s): ...
''',
  r'''
satellite.capsule point_from_string(satellite.variable.string s) satellite.returns(point)
{
    point p
    satellite.return(p)
}
''',
  "Checked 2026-09-12: **a file-scope capsule can build a spacesuit and answer it**, and the caller's handle works. That is the factory half of `@classmethod`. What is missing is `cls`, a class as a value, which would need M41.")

E("single inheritance `class B(A):`", "1.0", "says it",
  r'''
class LoudCounter(Counter):
''',
  r'''
satellite.spacesuit loud_counter(counter)
''',
  "The superclass goes in the parentheses. The subclass's capsules call the parent's by bare name (`example/spacesuits.satl`).")

E("`super()`", "2.2", "partial",
  r'''
def name(self):
    return "loud " + super().name()
''',
  None,
  "Parent capsules are called by bare name, so `super().add(x)` is `add(x)` when the child does not have its own `add`. **When the child overrides a name, reaching the parent's version is not a spelling** that this folder found.")

E("overriding and calling through a base-typed name", "1.0", "partial",
  r'''
def show(c: Counter): print(c.name())
show(LoudCounter())       # the subclass method runs
''',
  None,
  "Checked 2026-09-12: calling a capsule on a declared subclass runs the subclass's version. **Dispatch through a name declared as the base type** does not exist yet ([CXX23 row 28](../CXX23/README.md)), and that is most of what polymorphism is for.")

E("multiple inheritance and the MRO", "2.2 (C3: 2.3)", "open",
  r'''
class C(A, B):
''',
  None,
  "Checked in the CXX23 folder: two superclasses is `S0201`. **Open, and [CXX23 row 179](../CXX23/M179.md)**: no decision is recorded. Python's method resolution order is one worked answer to its hard question, two parents with one name.")

E("mixins", "2.2", "open",
  r'''
class Service(LoggingMixin, Base):
''',
  None,
  "Follows multiple inheritance.")

E("abstract base classes `abc.ABC`, `@abstractmethod`", "2.6", "M31",
  r'''
class Shape(ABC):
    @abstractmethod
    def area(self): ...
''',
  None,
  "An interface with no body, the same question as Java's `interface`, which `foreign.cpp` sends to M31 and the decision after it.")

E("`__str__` and `__repr__`", "1.0", "partial",
  r'''
def __repr__(self): return f"Point({self.x}, {self.y})"
''',
  r'''
satellite.capsule describe() satellite.returns(satellite.variable.string)
''',
  r"""
**A spacesuit already displays itself.** Checked 2026-09-12:
`satellite.console.display(p)` on a `point` with one field printed `<point x: 1>`,
which is a generated `__repr__`.

**Partial, because a suit cannot choose its own text.** A `describe()` capsule is
the custom version, called by name. DESIGN §12 defers a printer `to_string`, and
M38 bears on it.
""")

E("`__eq__`, `__lt__`, `__hash__`", "1.0 / 2.1", "M38",
  r'''
def __eq__(self, other): ...
def __lt__(self, other): ...
''',
  None,
  "M38 limits user operators to `+`, `==` and `<`. It is also what lets a suit be sorted and used as a map key.")

E("`__add__` and arithmetic dunders", "1.0", "M38",
  r'''
def __add__(self, other): ...
''',
  None,
  "M38, whose page names `__add__`. Only `+` is planned; `__mul__`, `__sub__` and the rest are not.")

E("`__len__`, `__getitem__`, `__contains__`, `__iter__`", "1.0 / 2.2", "never",
  r'''
def __getitem__(self, i): ...
def __len__(self): ...
''',
  r'''
satellite.capsule at(satellite.variable.number i) satellite.returns(satellite.variable.number)
satellite.capsule size() satellite.returns(satellite.variable.number)
''',
  "M38 excludes `[]` and `()` on purpose ([CXX23 row 175](../CXX23/M175.md)), and there is no iterator protocol. A suit answers `size()` and `at(i)` by name.")

E("`__call__`", "1.0", "never",
  r'''
def __call__(self, x): ...
''',
  None,
  "Excluded by M38. Passing behaviour around, which is why people write callable objects, is M32.")

E("`__getattr__`, `__setattr__`, `__getattribute__`", "1.0 / 2.2", "never",
  r'''
def __getattr__(self, name): ...
''',
  None,
  "A name on an object is resolved before the run and is refused when it does not exist. A suit whose names change at run time would give that up.")

E("descriptors `__get__`, `__set__`", "2.2", "never",
  r'''
class Typed:
    def __get__(self, obj, owner): ...
''',
  None,
  "Descriptors are how Python implements properties and methods. Satellite has capsules for both and no attribute lookup to hook.")

E("`__slots__`", "2.2", "never",
  r'''
__slots__ = ("x", "y")
''',
  None,
  "A spacesuit's fields are already fixed by its declaration, which is what `__slots__` exists to impose.")

E("metaclasses, `__init_subclass__`, `__class_getitem__`", "2.2 / 3.6 / 3.7", "never",
  r'''
class Meta(type): ...
class Model(metaclass=Meta): ...
''',
  None,
  "A class is not a value, so there is no class-of-a-class. What metaclasses are usually used for, registering or checking every subclass, is M41.")

E("class decorators", "2.6", "M32",
  r'''
@dataclass
class Point: ...
''',
  None,
  "A decorator on a class needs the class to be a value and the decorator to be a function value. M32 for the second, M41 for the first.")

E("`__del__` and finalisers", "1.0", "partial",
  r'''
def __del__(self): close_handle()
''',
  None,
  "A spacesuit is freed when its last handle goes, as a Python object is under reference counting. **Whether a suit can run code when that happens** is [CXX23 row 27](../CXX23/README.md)'s destructor question, marked partial there.")

E("`@dataclass`", "3.7", "partial",
  r'''
@dataclass
class Point:
    x: int = 0
    y: int = 0
''',
  r'''
satellite.spacesuit point()
{
    satellite.protected
    {
        satellite.variable.number x = 0
        satellite.variable.number y = 0
    }
}
''',
  r"""
**The field list is the same, and a spacesuit displays itself** (`<point x: 0 y:
0>`-style, row @{`__str__` and `__repr__`}@). What `@dataclass` generates and a suit does not:

- `__init__(x, y)`: constructor arguments are open (row @{`__init__` with parameters}@)
- `__eq__` and ordering: M38
- public field access: never (row @{public attributes}@), so a getter per field
""")

E("`collections.namedtuple`", "2.6", "M35",
  r'''
Point = namedtuple("Point", "x y")
x, y = Point(1, 2)
''',
  None,
  "A spacesuit covers the named fields. Taking one apart into names is M35.")

E("`enum.Enum`", "3.4", "open",
  r'''
class Colour(Enum):
    RED = 1
    GREEN = 2
''',
  r'''
satellite.library.red = 1
satellite.library.green = 2
''',
  "There is no enumeration. **Open, and [CXX23 row 94](../CXX23/M94.md)**, which calls it the open entry most likely to become a milestone, and the one that most changes M30.")

E("private names `_x` and name mangling `__x`", "1.0", "says it",
  r'''
class Account:
    def _audit(self): ...    # private by convention only
''',
  r'''
satellite.spacesuit account()
{
    satellite.protected
    {
        satellite.capsule audit() { ... }
    }
}
''',
  "**Satellite enforces what Python only suggests.** A capsule in `satellite.protected` is refused by name to everyone outside the suit and its subclasses (`S0516`, `example/spacesuits.satl`).")

E("assignment shares a list: `b = a`", "1.0", "says it",
  r'''
b = a
b.append(1)     # a changed too
''',
  r'''
satellite.container.list<satellite.variable.number> b = a
b.append(1)     // a is unchanged
''',
  r"""
**The opposite of Python, and silent.** Checked 2026-09-12 in the CXX23 folder:
`b = a` then `b.append(2)` left `a` at size 1. In Python, `b` and `a` are one list.

**A spacesuit is the other way round and matches Python**: `counter same = first`
makes a second name for the same object (`example/spacesuits.satl`). So the rule
to learn is *containers copy, spacesuits share*. Row @{argument passing}@ is the
same rule at a call.
""")

E("changing a list inside a list `grid[0].append(x)`", "1.0", "partial",
  r'''
grid[0].append(5)
''',
  r'''
satellite.container.list<satellite.variable.number> row = grid[0]
row.append(5)
grid[0] = row
''',
  r"""
**Checked 2026-09-12: refused**, `S0720: a method on this expression parses and
does not run yet ... name the receiver first` (M29's one hop). And because a list
copies (previous row), naming the receiver takes a *copy*, so the change has to be
written back with `grid[0] = row`. Forgetting the last line is silent.
""")

E("`copy.copy`, `copy.deepcopy`, `copy.replace`", "1.0 / 3.13", "partial",
  r'''
b = copy.deepcopy(a)
''',
  r'''
satellite.container.list<satellite.container.list<satellite.variable.number>> b = a
''',
  "**Assigning a container copies it**, nested containers included. **Copying a spacesuit has no spelling**: assignment shares it. A suit that needs copying writes a capsule that builds a new one and sets its fields.")

E("nested classes", "1.0", "open",
  r'''
class Tree:
    class Node: ...
''',
  None,
  "A spacesuit inside a spacesuit is `S0207`. **Open, and [CXX23 row 166](../CXX23/M166.md).**")

E("`vars()`, `__dict__`, `getattr(obj, \"name\")`, `hasattr`", "1.0", "M41",
  r'''
for k, v in vars(obj).items(): ...
getattr(obj, field_name)
''',
  None,
  "Looking a member up by a name held in a string is reflection. M41.")

E("garbage collection of reference cycles", "2.0", "open",
  r'''
a.partner = b
b.partner = a
del a, b          # the cycle collector frees both
''',
  None,
  r"""
**Python and satellite both count references, and only Python also collects
cycles.** CPython frees an object when its count reaches zero *and* runs a cycle
collector (`gc`) for the objects that point at each other. Satellite has the first
half and not the second: a ring of spacesuits that point at each other is never
freed (DESIGN §12). `infinity_data_main.satl` has one on purpose.

A Python programmer writes parent/child back-references without thinking about
this, because Python's collector makes it safe. **In satellite that pattern leaks
until the process ends.**

**Open, and it is the author's decision**: DESIGN §12 names a weak-reference field
as the cheap partial fix and a tracing collector as the complete one, and chooses
neither. [CXX23 row 182](../CXX23/M182.md) is the same decision.
""")

E("`weakref`", "2.1", "open",
  r'''
self.parent = weakref.ref(parent)
''',
  None,
  "The cheap half of the previous row's fix. Open, with it.")

E("the `gc` module", "2.0", "never",
  r'''
gc.collect()
gc.disable()
''',
  None,
  "There is no collector to run or turn off. Memory use is visible instead: `satellite.system.memory.this.used(unit)` answers this process's usage, which Python needs a third-party package for.")

# ---------------------------------------------------------------------------
section("Strings")

E("`str.format()`", "2.6", "never",
  r'''
"{} has {} items".format(name, n)
''',
  r'''
name + " has " + n + " items"
''',
  "No format strings, the same answer as f-strings (row 19).")

E("`%` formatting", "1.0", "never",
  r'''
"%s: %d" % (name, n)
''',
  r'''
name + ": " + n
''',
  "No format strings. In satellite `%` is only the remainder.")

E("number formatting: `f\"{x:.2f}\"`, `{n:,}`, `{n:08}`", "2.6 / 3.6", "open",
  r'''
print(f"{price:.2f}")
print(f"{count:,}")
''',
  None,
  r"""
**There is no way to display a number to a fixed number of places.** `round()`
takes no argument (row @{`abs()` and `round()`}@), `division_digits` is a program-wide
precision rather than a display width, and there is no padding or digit grouping.

**Open, and it is the real loss behind *no format strings*.** Joining text with `+`
covers almost everything else a format string does; money, tables and progress
percentages are what it does not cover.
""")

E("`len(s)`", "1.0", "says it",
  r'''
len("héllo")     # 5
''',
  r'''
satellite.variable.string s = "héllo"
satellite.console.display(s.size())    // 6
''',
  "`size()` counts **bytes**. Checked 2026-09-12: `\"héllo\".size()` is `6`, because `é` is two bytes in UTF-8. Python counts code points and says `5`. What a character is, is M45.")

E("indexing and slicing a string", "1.0", "says it",
  r'''
s[0], s[-1], s[1:3]
''',
  r'''
satellite.variable.string first = s[0]
satellite.variable.string tail = s[1:3]
''',
  "Checked 2026-09-12: `s[0]`, `s[-1]` and `s[1:3]` work, and `s.at(n)` is the long spelling. **Each position is a byte**, so slicing through a multi-byte character splits it (M45).")

E("`upper()`, `lower()`", "1.0", "says it",
  r'''
"héllo".upper()    # 'HÉLLO'
''',
  r'''
s.upper()          // HéLLO
''',
  "Checked 2026-09-12: `\"héllo\".upper()` is `HéLLO`. **Only ASCII letters change**, silently. Python's is Unicode-aware. M45.")

E("`strip()`, `lstrip()`, `rstrip()`", "1.0", "partial",
  r'''
line.strip()
''',
  r'''
line.trim()
''',
  "Checked 2026-09-12: `trim()` removes spaces from both ends. There is no one-sided trim and no argument naming the characters to strip.")

E("`split()`", "1.0", "partial",
  r'''
"a,,b".split(",")   # ['a', '', 'b']
"a  b".split()      # ['a', 'b']
''',
  r'''
satellite.container.list<satellite.variable.string> parts = line.split(",")
''',
  "Checked 2026-09-12: `split(\",\")` on `a,,b` answered three parts with an empty middle, as Python does. **`split()` with no separator is `S0722`**, so Python's split-on-any-whitespace has no spelling; `split(\" \")` keeps empty parts between repeated spaces.")

E("`\",\".join(parts)`", "1.0", "says it",
  r'''
", ".join(names)
", ".join([1, 2])    # TypeError
''',
  r'''
satellite.variable.string line = names.join(", ")
''',
  "**The method is on the list, not on the separator**, which is the usual order everywhere but Python. Checked 2026-09-12: a list of numbers joins too (`1-2`), where Python raises.")

E("`replace()`", "1.0", "says it",
  r'''
"a-b-c".replace("-", "+")
''',
  r'''
s.replace("-", "+")
''',
  "Checked 2026-09-12: every occurrence is replaced and `s` itself is unchanged, as in Python. There is no `count` argument.")

E("`find()`, `index()`, `rfind()`", "1.0", "partial",
  r'''
"abc".find("z")     # -1
''',
  r'''
satellite.statement.if (s.contains("z"))
{
    satellite.variable.number at = s.find("z")
}
''',
  "**`find` refuses when the text is absent**: checked 2026-09-12, `S0716`, with the message naming `contains(x)`. So satellite's `find` is Python's `index`, and Python's `-1` has no counterpart. There is no search from the right.")

E("`startswith()`, `endswith()`, `in`", "1.0", "says it",
  r'''
path.startswith("/")
"err" in line
''',
  r'''
path.starts_with("/")
line.contains("err")
''',
  "The same questions with underscores. Python's tuple argument (`startswith((\"a\", \"b\"))`) is two calls.")

E("`count`, `isdigit`, `isalpha`, `isspace`, `title`, `capitalize`, `center`, `zfill`, `partition`, `splitlines`, `removeprefix`", "1.0 / 3.9", "open",
  r'''
s.isdigit()
s.zfill(5)
s.removeprefix("v")
''',
  None,
  r"""
**Not in `satl --words`.** A string has `size`, `empty`, `find`, `contains`,
`substring`, `starts_with`, `ends_with`, `lower`, `upper`, `split`, `trim`,
`replace`, `to_number`, `append`, `clear` and `at`.

**Open, and `isdigit` is the one that matters most**, because it is the question
that would let a program avoid `to_number`'s uncatchable `S0610` (row
@{`int(s)`}@).
""")

E("`repr()` and quoting in displayed containers", "1.0", "open",
  r'''
print(["a", "", "b"])    # ['a', '', 'b']
''',
  r'''
satellite.console.display(parts)    // [a, , b]
''',
  "Checked 2026-09-12: strings inside a displayed list are unquoted, so `[a, , b]` is how a list with an empty string looks, and a string containing `, ` cannot be told from two strings. **Open: whether display should quote strings inside containers.**")

E("`encode()`, `decode()` and codecs", "2.0 / 3.0", "M45",
  r'''
data = text.encode("utf-8")
''',
  None,
  "A string is bytes with no declared encoding. Converting between encodings is M45.")

E("the `re` module", "1.5", "open",
  r'''
re.findall(r"\d+", text)
''',
  None,
  "**There is no regex.** `search(pattern)` on lists and maps is a *fuzzy* search tuned by `satellite.system.threshold`, not a regular expression. **Open, and [CXX23 row 240](../CXX23/M240.md).**")

E("`textwrap`, `string` constants, `difflib`", "2.3 / 1.0 / 2.1", "open",
  r'''
textwrap.fill(text, 72)
string.ascii_letters
''',
  r'''
satellite.variable.number width = satellite.console.width()
''',
  "None of these are in the word tree. `satellite.console.width` and `height` answer the terminal size a wrapper would need. Open, and low priority.")

# ---------------------------------------------------------------------------
section("Lists, maps and sets in detail")

E("`list.insert(i, x)`", "1.0", "says it",
  r'''
xs.insert(0, 9)
''',
  r'''
xs.insert(0, 9)
''',
  "The same call. Checked 2026-09-12.")

E("`list.pop()` and `pop(0)`", "1.0", "says it",
  r'''
top = stack.pop()
front = queue.pop(0)
''',
  r'''
satellite.variable.number top = stack.last()
stack.remove_last()
satellite.variable.number front = queue.first()
queue.remove_first()
''',
  "Two steps: read, then remove. Checked 2026-09-12: `first()`, `last()`, `remove_first()` and `remove_last()` all work (the last two are not listed by `satl --words` but run).")

E("`list.remove(x)`, `del xs[i]`", "1.0", "says it",
  r'''
xs.remove(1)
del xs[0]
''',
  r'''
xs.remove(1)
xs.remove_at(0)
''',
  "Checked 2026-09-12: `remove(x)` removes the **first** match, as Python's does, and removing a value that is not there is `S0728` (Python's `ValueError`).")

E("`list.index(x)`", "1.0", "says it",
  r'''
xs.index("nut")
''',
  r'''
xs.index_of("nut")
''',
  "Refuses when absent (`S0728`), as Python raises. Ask `contains(x)` first.")

E("`list.count(x)`", "1.0", "partial",
  r'''
xs.count(0)
''',
  r'''
satellite.variable.number zeros = 0
satellite.statement.for (satellite.variable.number i = 0; i < xs.size(); i = i + 1)
{
    satellite.statement.if (xs[i] == 0) { zeros = zeros + 1 }
}
''',
  "Not in the word tree; an index loop.")

E("`clear()`, `copy()`, `reverse()`", "1.0 / 3.3", "says it",
  r'''
xs.clear()
ys = xs.copy()
xs.reverse()
''',
  r'''
xs.clear()
satellite.container.list<satellite.variable.number> ys = xs
xs.reverse()
''',
  "`copy()` is plain assignment, because assignment copies.")

E("`dict.get(k, default)`", "1.0", "says it",
  r'''
port = config.get("port", 8080)
''',
  r'''
satellite.variable.number port = 8080
satellite.statement.if (config.has("port")) { port = config["port"] }
''',
  "`has(k)` then read. `get` with no default refuses on a missing key (`S0726`), which is Python's `d[k]` and not its `d.get(k)`.")

E("`setdefault()` and `collections.defaultdict`", "2.0 / 2.5", "partial",
  r'''
groups.setdefault(key, []).append(x)
''',
  r'''
satellite.container.list<satellite.variable.string> group = satellite.container.list()
satellite.statement.if (groups.has(key)) { group = groups[key] }
group.append(x)
groups[key] = group
''',
  "Read the list out (a copy), change it, write it back. The write-back is required, because the map holds its own copy.")

E("`keys()`, `values()`, `items()`", "1.0", "says it",
  r'''
for k, v in d.items():
''',
  r'''
satellite.container.list<satellite.variable.string> ks = d.keys()
satellite.statement.for (satellite.variable.number i = 0; i < ks.size(); i = i + 1)
{
    satellite.variable.string k = ks[i]
    satellite.variable.number v = d[k]
}
''',
  "`keys()` and `values()` answer **lists**, in insertion order, not live views. There is no `items()`; walking keys and reading each value is the form. The two-name binding is M35.")

E("`del d[k]` and `dict.pop(k)`", "1.0", "says it",
  r'''
del d["a"]
''',
  r'''
d.remove("a")
''',
  "Checked 2026-09-12: `remove(k)` took the key out and `size()` dropped by one.")

E("`update()` and `|` merge", "1.0 / 3.9", "partial",
  r'''
merged = defaults | overrides
''',
  r'''
satellite.container.list<satellite.variable.string> ks = overrides.keys()
satellite.statement.for (satellite.variable.number i = 0; i < ks.size(); i = i + 1)
{
    satellite.variable.string k = ks[i]
    merged[k] = overrides[k]
}
''',
  "A loop over the keys. There is no merge method and `|` is not an operator.")

E("map keys of other types", "1.0", "partial",
  r'''
grid[(x, y)] = cell
''',
  None,
  "**Strings and numbers work as keys today.** A tuple key needs M35 and a spacesuit key needs M38 (equality and hashing). A string key built with `+` (`x + \",\" + y`) is the workaround.")

E("`collections.Counter`", "2.7", "partial",
  r'''
counts = Counter(words)
''',
  r'''
satellite.variable.number seen = 0
satellite.statement.if (counts.has(w)) { seen = counts[w] }
counts[w] = seen + 1
''',
  "A map of numbers, incremented by hand. `most_common` is a sort over a list built from the keys.")

E("`collections.OrderedDict`", "2.7", "says it",
  r'''
od = OrderedDict()
''',
  r'''
satellite.container.map<satellite.variable.string, satellite.variable.number> od = satellite.container.map()
''',
  "A satellite map is already ordered by insertion. There is no `move_to_end`.")

E("`collections.deque`", "2.4", "says it",
  r'''
q = deque()
q.appendleft(x)
q.popleft()
''',
  r'''
q.insert(0, x)
q.remove_first()
''',
  "One list does both ends. The cost at the front has not been measured.")

E("`heapq` and `queue.PriorityQueue`", "2.3", "partial",
  r'''
heapq.heappush(h, x)
smallest = heapq.heappop(h)
''',
  r'''
h.append(x)
h.sort()
satellite.variable.number smallest = h.first()
h.remove_first()
''',
  "No heap: sort, then take the front. Correct, and slower per insert.")

E("`bisect`", "2.1", "partial",
  r'''
i = bisect.bisect_left(sorted_xs, x)
''',
  None,
  "`contains` and `index_of` answer membership without needing sorted input, and nothing uses sortedness to answer faster. Binary search is a hand-written loop.")

E("set operations `| & - ^`", "2.4", "partial",
  r'''
common = a & b
''',
  None,
  "With sets as maps of bools (row @{`set` and `frozenset`}@), intersection is a loop over one map's keys asking `has(k)` of the other.")

E("the `array` module", "1.4", "never",
  r'''
array.array("d", [1.0, 2.0])
''',
  r'''
satellite.container.list<satellite.variable.number> values = satellite.container.list()
''',
  "A typed compact array exists for memory layout, which a satellite program does not see. The list is already typed.")

# ---------------------------------------------------------------------------
section("Modules, packages and the program")

E("`from module import name`", "1.0", "open",
  r'''
from os.path import join
''',
  None,
  "**Every language path is written whole** (DESIGN §1), so importing a short name works against the rule. **Open, and it is [CXX23 row 68](../CXX23/M68.md)'s decision about program namespaces.**")

E("`import module as alias`", "1.0 / 2.0", "open",
  r'''
import numpy as np
''',
  None,
  "Follows the namespace decision ([CXX23 row 69](../CXX23/M69.md), namespace aliases).")

E("writing a module in a second file", "1.0", "partial",
  r'''
# helpers.py
def slug(s): ...
# app.py
from helpers import slug
''',
  None,
  "**A program is one file today.** `satellite.include` of another `.satl` file is PLAN.md's build milestone 25, the plan's own numbering and not a promise from this folder. Whether a file is loaded once per include is one of its open questions.")

E("packages, `__init__.py` and relative imports", "1.5 / 2.5", "open",
  r'''
from .models import User
''',
  None,
  "Follows a second file (previous row) and program namespaces. Nothing is decided.")

E("`__all__` and underscore-private module names", "1.0", "open",
  r'''
__all__ = ["public_api"]
''',
  None,
  "Hiding names from other files only matters once there are other files. [CXX23 row 70](../CXX23/M70.md) is the same question.")

E("`importlib` and importing by name at run time", "3.1", "never",
  r'''
mod = importlib.import_module(name)
''',
  None,
  "Every path is resolved before the program runs, and a refusal names a missing one at check time. Loading code chosen by a string at run time would give that up.")

E("`pip`, PyPI, virtual environments", "3.4 (ensurepip) / 3.3 (venv)", "open",
  r'''
pip install requests
python -m venv .venv
''',
  None,
  "**Satellite has no package manager and no third-party library format.** New capability arrives as numbered paths built into the interpreter, which is how every module in the language got there. Whether programs will ever share satellite code with each other depends on the second-file question first. Open.")

E("`__file__` and `__name__`", "1.0", "open",
  r'''
here = os.path.dirname(__file__)
''',
  r'''
satellite.variable.string cwd = satellite.directory.current()
''',
  "A program cannot ask which file it is. The working directory can be read. **Open, with [CXX23 row 61](../CXX23/M61.md)** (`__FILE__`, `source_location`).")

E("`sys.argv`", "1.0", "says it",
  r'''
import sys
if "--fast" in sys.argv: ...
''',
  r'''
satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> arguments)
{
    satellite.statement.if (arguments.contains("--fast")) { ... }
}
''',
  "`main` receives the arguments, and they answer `has(k)`, `get(k)`, `keys`, `first`, `last`, `contains(x)`, `length` and `count`. Checked 2026-09-12: **`arguments.size()` is refused** (`S0532`) and the refusal lists what is there.")

E("`argparse`", "3.2", "says it",
  r'''
parser.add_argument("--port", type=int)
args = parser.parse_args()
''',
  r'''
satellite.statement.if (arguments.has("port")) { ... arguments.get("port") ... }
''',
  "The arguments object parses options already (`has`, `get`, `keys`), so the common case needs no parser. Generated `--help` and type checking of values are not there.")

E("`sys.exit(code)`", "1.0", "partial",
  r'''
sys.exit(2)
''',
  r'''
satellite.return(satellite)
''',
  "`satl`'s exit code is decided by whether the run refused, not by a number the program chooses ([CXX23 row 1](../CXX23/M1.md)). **A program cannot exit with 2**, and it cannot stop early from deep inside a call chain.")

E("`input()`", "1.0", "says it",
  r'''
name = input("name: ")
age = int(input("age: "))
''',
  r'''
satellite.variable.string name = satellite.console.input("name: ")
satellite.variable.number age = satellite.console.input("age: ")
''',
  "`input(prompt)` answers the typed line, and assigning it to a number converts it (`example/advanced.satl`). `input(prompt, target)` writes straight into a variable.")

E("`sys.stderr` and `print(..., file=sys.stderr)`", "1.0", "open",
  r'''
print("warning", file=sys.stderr)
''',
  None,
  "**A program writes to one place.** Anyone using satellite in a pipeline needs this. **Open, and [CXX23 row 294](../CXX23/M294.md).**")

E("`sys.version`, `platform`", "1.0", "says it",
  r'''
sys.version
platform.system()
platform.machine()
platform.node()
''',
  r'''
satellite.library.main.arguments.interpreter.version
satellite.library.main.arguments.system.name
satellite.library.main.arguments.machine.architecture
satellite.library.main.arguments.system.hostname
''',
  "**Satellite answers far more of these than Python's standard library does**: kernel, distribution, CPU, cores, threads, page size, byte order, compiler, build flags, shell and terminal are all under `satellite.library.main.arguments`.")

E("`sys.getrecursionlimit()`, `sys.float_info`, `sys.maxsize`", "2.0", "says it",
  r'''
sys.getrecursionlimit()
sys.maxsize
''',
  r'''
satellite.library.system.max_depth
satellite.library.system.float_digits
''',
  "Numbers have no maximum. The limits a program can reach are library values, and `satl --limits` prints every one and where it came from.")

E("`os.environ`", "1.0", "says it",
  r'''
home = os.environ.get("HOME")
''',
  r'''
satellite.variable.string home = satellite.system.environment("HOME")
''',
  "Checked 2026-09-12: an unset variable answers `nothing`, not a refusal, which is `os.environ.get` and not `os.environ[...]`. Setting a variable is not in the word tree.")

E("`os.getcwd()`, `os.chdir()`, `os.listdir()`", "1.0", "says it",
  r'''
os.listdir(".")
''',
  r'''
satellite.container.list<satellite.variable.string> here = satellite.directory.list()
''',
  "`satellite.directory.current()`, `change(d)`, `exists(d)`, `list()` and `list(d)`.")

E("`os.path.exists()` and `pathlib.Path`", "1.0 / 3.4", "partial",
  r'''
Path("data") / "file.txt"
''',
  r'''
satellite.statement.if (satellite.file.exists(path)) { ... }
''',
  "`satellite.file.exists` and `satellite.directory.exists` answer the questions. There is no path type, and no joining, splitting or extension helpers: a path is a string built with `+`.")

E("`os.remove()`, `os.rmdir()`", "1.0", "says it",
  r'''
os.remove(p)
''',
  r'''
satellite.variable.bool gone = satellite.system.delete(p)
''',
  "Removes a file or an **empty** directory and answers whether it is gone. It is not `shutil.rmtree`.")

E("`os.mkdir()`, `os.rename()`, `shutil.copy()`, `shutil.rmtree()`", "1.0 / 2.3", "open",
  r'''
os.makedirs("out/logs", exist_ok=True)
''',
  None,
  "Not in `satl --words`. **Open, and [CXX23 row 299](../CXX23/M299.md).**")

E("`glob` and `fnmatch`", "1.0", "partial",
  r'''
glob.glob("*.txt")
''',
  r'''
satellite.container.list<satellite.variable.string> all = satellite.directory.list()
// keep the names where name.ends_with(".txt")
''',
  "List the directory and test each name. There is no wildcard matching.")

E("`tempfile`", "2.3", "open",
  r'''
with tempfile.TemporaryDirectory() as d:
''',
  None,
  "Not in the word tree. Open.")

E("`subprocess` and `os.system`", "2.4 / 1.0", "open",
  r'''
subprocess.run(["ls", "-l"])
''',
  None,
  "**There is no way to run another program.** **Open, and [CXX23 row 229](../CXX23/M229.md)**, which notes that a shell escape is the widest door a language can open.")

E("`multiprocessing`", "2.6", "open",
  r'''
with Pool(4) as p:
    p.map(work, items)
''',
  None,
  "Follows `subprocess`: no second process can be started. **Threads in satellite run in parallel**, so Python's reason for `multiprocessing`, the GIL, does not apply (row @{the GIL}@).")

E("`os.getpid()`, `os.cpu_count()`, `getpass.getuser()`", "1.0 / 3.4", "says it",
  r'''
os.getpid()
os.cpu_count()
''',
  r'''
satellite.library.main.arguments.process.id
satellite.library.main.arguments.machine.cores
satellite.library.main.arguments.machine.threads
satellite.library.main.arguments.username
''',
  "Machine and process facts. `cores` and `threads` are separate, which `os.cpu_count` does not give you.")

E("process memory (`resource`, `tracemalloc`)", "2.0 / 3.4", "says it",
  r'''
resource.getrusage(resource.RUSAGE_SELF).ru_maxrss
''',
  r'''
satellite.system.memory.this.used("mb")
satellite.system.memory.free("mb")
''',
  "**Satellite gives a program the numbers.** Free, used and total for the machine, swap and this process, in a unit you pass. There is no per-allocation tracing.")

E("`signal` and `atexit`", "1.0 / 2.0", "never",
  r'''
signal.signal(signal.SIGINT, handler)
atexit.register(cleanup)
''',
  None,
  "Ctrl-C belongs to satellite (DESIGN §10.2), and there is no hook that runs at exit. A refusal ends the run with its own report.")

E("`logging`", "2.3", "partial",
  r'''
logging.info("started")
''',
  r'''
satellite.console.display("started")
''',
  "`display` to the one output, or `write_line` to a log file. Levels, handlers and stderr are not there (row @{`sys.stderr`}@).")

E("`unittest`, `pytest`, `doctest`", "2.1", "open",
  r'''
def test_add():
    assert add(2, 2) == 4
''',
  r'''
$ satl --call app.satl add 2 2
''',
  "**No test framework in the word tree.** `satl --call` runs one capsule with arguments and prints its answer, which a shell script can compare, and the repository's own acceptance programs print `PASS`/`WRONG` lines (`example/threads.satl`). A program asserting conditions is M42. Open.")

E("`pdb` and `breakpoint()`", "1.0 / 3.7", "open",
  r'''
breakpoint()
''',
  None,
  "There is no debugger. A refusal's caret and call stack answer *where*; the CXX26 folder's debugging section records *watch a value without editing the loop* as the gap people actually hit. Open.")

E("`timeit`, `time.perf_counter`, `cProfile`", "2.3 / 3.3", "open",
  r'''
start = time.perf_counter()
''',
  None,
  "An instant can be read and displayed, and not yet subtracted (row @{`datetime` arithmetic}@). A monotonic clock for timing is open ([CXX23 row 304](../CXX23/M304.md)), and there is no profiler.")

E("`__debug__` and `python -O`", "1.0", "never",
  r'''
if __debug__: check()
''',
  None,
  "Satellite has no build modes, so there is one behaviour. M42 decides that checks are not removed.")

E("`from __future__ import …`", "2.1", "never",
  r'''
from __future__ import annotations
''',
  None,
  "A word's number is permanent and never withdrawn (WORD_NUMBERS.md §1.2), so there is no future spelling to opt into early.")

E("`ctypes`, C extensions and `cffi`", "2.5", "never",
  r'''
libc = ctypes.CDLL("libc.so.6")
''',
  None,
  "**There is no foreign function interface.** To reach C or C++, write it into `src/` behind a numbered path, the way every satellite module got there. [../ASM/README.md](../ASM/README.md) makes the same argument one level down.")

E("`sys.getsizeof()`", "2.6", "never",
  r'''
sys.getsizeof(xs)
''',
  r'''
satellite.system.memory.this.used("kb")
''',
  "A value's byte size is not visible (the CXX23 folder's `sizeof` row). The whole process's use is.")

E("`locals()`, `globals()`, `inspect`", "1.0 / 2.1", "M41",
  r'''
inspect.signature(f)
''',
  None,
  "Reflection. M41.")

E("`warnings.deprecated` and `@deprecated`", "3.13", "never",
  r'''
@deprecated("use g")
def f(): ...
''',
  None,
  "A language word is never withdrawn, and a program's capsules have no outside callers to warn until there are second files. The CXX23 folder answers `[[deprecated]]` the same way.")

# ---------------------------------------------------------------------------
section("Files and data formats")

E("`open(path, mode)`", "1.0", "says it",
  r'''
f = open("log.txt", "a")
''',
  r'''
satellite.variable.file f = satellite.file.open("log.txt", "append")
''',
  "**The mode is a word**: `\"read\"`, `\"write\"` (empties an existing file), `\"append\"` and `\"read_append\"`. `satellite.file.new(path)` makes a new file.")

E("a file that is not there", "1.0", "says it",
  r'''
try:
    f = open(path)
except FileNotFoundError: ...
''',
  r'''
satellite.variable.file f = satellite.file.open(path, "read")
satellite.statement.if (f.ok() == satellite.bool.false)
{
    satellite.console.display(f.error())
}
''',
  "**No exception and no refusal.** Checked 2026-09-12: opening `/nonexistent/zz` answered `ok()` `false` and `error()` `No such file or directory`. M33 names this as the pattern to generalise.")

E("`read()`, `readline()`, `write()`", "1.0", "says it",
  r'''
text = f.read()
line = f.readline()
f.write("no newline")
''',
  r'''
satellite.variable.string text = f.read_all()
satellite.variable.string line = f.read_line()
f.write("no newline")
f.write_line("with newline")
''',
  "`write` adds nothing; `write_line` adds the newline. Both answer whether the write worked.")

E("`for line in f:`", "1.0", "partial",
  r'''
for line in f:
    handle(line)
''',
  r'''
satellite.variable.string text = f.read_all()
satellite.container.list<satellite.variable.string> lines = text.split("\n")
''',
  "Read the whole file and split it. A loop calling `read_line` needs to know when the file has ended, and how `read_line` says so was not checked here.")

E("binary mode, `seek()`, `tell()`", "1.0", "open",
  r'''
with open(p, "rb") as f:
    f.seek(128)
''',
  None,
  "Files are text lines and whole reads. **Open, and [CXX23 row 296](../CXX23/M296.md)**, which pairs it with M44.")

E("`json`", "2.6", "open",
  r'''
data = json.load(f)
''',
  None,
  "**Not in the word tree.** A map of lists of maps can be *built* and displayed, and displayed maps are not JSON (strings unquoted). Open, and the data format a Python reader will reach for first.")

E("`csv`", "2.3", "partial",
  r'''
for row in csv.reader(f):
''',
  r'''
satellite.container.list<satellite.variable.string> cells = line.split(",")
''',
  "`split(\",\")` reads simple rows. Quoted fields containing commas are not handled.")

E("`tomllib`, `configparser`, `xml`", "3.11 / 1.5 / 2.0", "open",
  r'''
tomllib.load(f)
''',
  None,
  "None are in the word tree. Open.")

E("`pickle` and `shelve`", "1.0", "open",
  r'''
pickle.dump(state, f)
''',
  None,
  "**Not to be confused with `satellite.system.persist`**, which is the REPL's memory of declarations and not serialisation. There is no way to save a value and load it back except writing it out as text. Open.")

E("`sqlite3`", "2.5", "open",
  r'''
conn = sqlite3.connect("app.db")
''',
  None,
  "Not in the word tree. Open.")

E("`zipfile`, `gzip`, `tarfile`", "1.6 / 1.5 / 2.3", "open",
  r'''
with zipfile.ZipFile(p) as z:
''',
  None,
  "Not in the word tree, and it needs binary files first. Open.")

E("`hashlib`, `hmac`, `base64`, `uuid`, `secrets`", "2.5 / 2.2 / 1.0 / 2.5 / 3.6", "open",
  r'''
hashlib.sha256(data).hexdigest()
secrets.token_hex(16)
''',
  None,
  "None are in the word tree. `satellite.random` has three generator tiers (`fast`, `normal`, `ultra`); whether any is suitable for secrets was not checked, so do not assume `ultra` is `secrets`. Open.")

E("`io.StringIO`", "2.0 / 3.0", "never",
  r'''
buf = io.StringIO()
buf.write("x=")
''',
  r'''
satellite.variable.string buf = ""
buf.append("x=")
''',
  "A string can be appended to in place, which is what `StringIO` is used for.")

E("`struct`", "1.4", "never",
  r'''
struct.pack("<I", n)
''',
  r'''
satellite.variable.binary bits = n.binary()
''',
  "Byte layout is not visible. `binary` and `hex` are where bit-level values live.")

# ---------------------------------------------------------------------------
section("Numbers and maths")

E("`math.sqrt`, `floor`, `ceil`, `trunc`", "1.0", "says it",
  r'''
math.sqrt(2)
math.floor(-3.5)
''',
  r'''
satellite.variable.number two = 2
satellite.console.display(two.sqrt())
''',
  "Checked 2026-09-12: `sqrt(2)` printed 34 digits, `floor` of `-3.5` is `-4` and `truncate` is `-3`, matching Python's `floor` and `trunc`.")

E("`math.sin`, `cos`, `log`, `exp`", "1.0", "open",
  r'''
math.sin(x)
math.log(x)
''',
  None,
  "**Not in `satl --words`.** **Open, and [CXX23 row 277](../CXX23/M277.md)**, which names the design question: an exact number has no exact `sin(1)`.")

E("`math.pi`, `math.e`, `math.inf`, `math.nan`", "1.0 / 3.5", "open",
  r'''
2 * math.pi * r
''',
  None,
  "Goes with trigonometry. Satellite does not produce infinity (division by zero refuses for floats too), which [CXX23 row 288](../CXX23/M288.md) says should be written down as a guarantee.")

E("`math.gcd`, `lcm`, `factorial`, `comb`, `isqrt`", "3.5 / 3.9 / 2.6 / 3.8", "open",
  r'''
math.gcd(a, b)
math.factorial(30)
''',
  None,
  "Not in the word tree. A recursive `fact(30)` is exact today. **Open, and [CXX23 row 279](../CXX23/M279.md)**: small, and the arithmetic satellite numbers are best at.")

E("`math.isclose`", "3.5", "never",
  r'''
math.isclose(0.1 + 0.2, 0.3)
''',
  r'''
satellite.statement.if (0.1 + 0.2 == 0.3) { ... }
''',
  "Numbers are exact decimals, so `==` is the right test (checked: `true`). A result rounded to `division_digits` compares equal to the same rounding.")

E("`statistics.mean`, `median`", "3.4", "partial",
  r'''
statistics.mean(xs)
''',
  r'''
satellite.variable.number total = xs.sum()
satellite.variable.number mean = total / xs.size()
''',
  "Exact, with `sum()` and `size()`. A median is `sort()` and the middle.")

E("`random.randint`, `random.random`, `random.seed`", "1.0", "says it",
  r'''
random.seed(42)
random.randint(1, 6)
''',
  r'''
satellite.variable.number roll = satellite.random.fast(1, 6)
satellite.variable.number same = satellite.random.seeded(42)
''',
  "`fast(min, max)` draws in a range, and `seeded(seed)` repeats a sequence. **`fast`, `normal` and `ultra` are tiers of generator, not distributions**: `satellite.random.normal()` is not `random.gauss`.")

E("`random.choice`, `shuffle`, `sample`", "1.0 / 2.3", "partial",
  r'''
random.choice(names)
''',
  r'''
satellite.variable.number last = names.size() - 1
satellite.variable.number pick = satellite.random.fast(0, last)
satellite.variable.string name = names[pick]
''',
  "A random index. A shuffle is a hand-written swap loop.")

E("`random.gauss` and other distributions", "1.0", "open",
  r'''
random.gauss(0, 1)
''',
  None,
  "**Open, and [CXX23 row 291](../CXX23/M291.md)**, with its naming trap: `satellite.random.normal` is already a tier.")

E("`int.bit_length()`, `int.bit_count()`", "2.7 / 3.10", "open",
  r'''
n.bit_count()
''',
  None,
  "Not in the word tree. **Open, and [CXX23 row 282](../CXX23/M282.md)**, with the bitwise operators.")

E("`float(\"inf\")`, `float(\"nan\")`, `math.isnan`", "2.6", "open",
  r'''
best = float("inf")
''',
  r'''
satellite.variable.variant best
''',
  "There is no infinity to start a minimum search from. A variant holding nothing until the first value, or the list's first element, is the usual replacement. **Open, with [CXX23 row 288](../CXX23/M288.md).**")

# ---------------------------------------------------------------------------
section("Time")

E("`time.time()` and `datetime.now()`", "1.0 / 2.3", "partial",
  r'''
now = datetime.now(timezone.utc)
''',
  r'''
satellite.console.display(satellite.time.now())
''',
  "Checked 2026-09-12: `satellite.time.now()` displayed `2026-09-12T19:57:12.751454029Z`, ISO 8601 in UTC with nanoseconds. **An instant can be displayed and not yet taken apart or subtracted** (next rows).")

E("`time.sleep(seconds)`", "1.0", "says it",
  r'''
time.sleep(0.25)       # a quarter of a second
''',
  r'''
satellite.time.sleep(250)
''',
  "**MILLISECONDS, NOT SECONDS.** `satellite.help` (checked 2026-09-12): *stops the program for the number of milliseconds you give*. `time.sleep(2)` copied across waits two thousandths of a second. Ctrl-C still reaches a sleeping program.")

E("`datetime` arithmetic and `timedelta`", "2.3", "partial",
  r'''
elapsed = datetime.now() - start
''',
  None,
  "`satellite.variable.date` and `satellite.variable.duration` are numbered and unbuilt, and `satellite.time.new` answers `S1101` from `satellite.help`. Subtracting two times is PLAN.md's calendar milestone (milestone 29 in the plan's own numbering), not a promise from this folder.")

E("`strftime` and `strptime`", "2.3", "partial",
  r'''
now.strftime("%Y-%m-%d")
''',
  None,
  "Follows the calendar milestone above. Today the ISO 8601 display is the only format.")

E("`zoneinfo` and time zones", "3.9", "open",
  r'''
ZoneInfo("Europe/London")
''',
  None,
  "Time zones are not mentioned anywhere in satellite yet. Open.")

E("`time.monotonic()`", "3.3", "open",
  r'''
t0 = time.monotonic()
''',
  None,
  "DESIGN §13 settled that a time is `system_clock`, which can jump. **Open, and [CXX23 row 304](../CXX23/M304.md).**")

# ---------------------------------------------------------------------------
section("Concurrency")

E("`threading.Thread`, `start()`, `join()`", "1.5", "says it",
  r'''
t = threading.Thread(target=work, args=(x,))
t.start()
t.join()
''',
  r'''
satellite.variable.thread t = satellite.thread.new(work(x))
t.start()
t.join()
''',
  "**The call is written out whole**: `satellite.thread.new(work(x))` works out `x` on the current thread and holds the call until `start()` (`satellite.help` on `satellite.variable.capsule`). A thread is a reference type, so two names share one thread.")

E("getting a result back from a thread", "3.2 (`futures`)", "says it",
  r'''
with ThreadPoolExecutor() as ex:
    result = ex.submit(work, x).result()
''',
  r'''
satellite.variable.thread t = satellite.thread.new(work(x))
t.start()
satellite.variable.number result = t.join()
''',
  "**`join()` answers what the capsule returned** (`example/threads.satl` §5). A plain Python `Thread` cannot do this, and needs a future or a queue.")

E("the GIL and free-threaded Python", "1.5 / 3.13", "M40",
  r'''
# Python: the GIL lets one thread run Python code at a time
# (3.13 adds an experimental free-threaded build)
''',
  None,
  r"""
**Satellite threads run at the same time, with nothing like a GIL, and with no
locks.** `example/threads.satl` says it outright: *threads can interleave and lose
an increment. That is a data race.* `satellite.library` is one variable seen by
every thread.

A Python programmer's threaded code is usually safe *by accident*, because the
GIL serialises most single operations. **Code carried across unchanged is not
safe here**, and M40 says there is no mutex and no atomic anywhere under the
containers.

**M40 is the most serious milestone in this folder**, by its own account.
""")

E("`threading.Lock` and `RLock`", "1.5", "M40",
  r'''
with lock:
    count += 1
''',
  None,
  "M40, whose page names `threading.Lock`. Until then, give each thread its own data and combine the answers from `join()`, which is race-free.")

E("`Condition`, `Event`, `Semaphore`, `Barrier`", "1.5 / 3.2", "M40",
  r'''
done = threading.Event()
done.wait()
''',
  None,
  "Waiting on something needs a lock underneath. M40. `join()` is the one wait that exists.")

E("`queue.Queue`", "2.3", "M40",
  r'''
q = queue.Queue()
q.put(item)
''',
  None,
  "A thread-safe queue needs M40's lock, and the list is not safe to share across threads today.")

E("`concurrent.futures` and thread pools", "3.2", "M43",
  r'''
with ThreadPoolExecutor(8) as ex:
    results = list(ex.map(work, items))
''',
  None,
  "The interpreter keeps a worker pool of its own with no door to it from a program. Composing parallel work is M43, which must follow M40. N threads and N joins is the spelling today.")

E("`threading.local`", "2.4", "open",
  r'''
state = threading.local()
''',
  None,
  "Locals are per call and so per thread already; `satellite.library` is shared by all. Nothing sits between. **Open, and [CXX23 row 155](../CXX23/M155.md).**")

E("daemon threads and `threading.Timer`", "1.5 / 2.0", "open",
  r'''
threading.Timer(5.0, remind).start()
''',
  r'''
satellite.capsule remind_later()
{
    satellite.time.sleep(5000)
    remind()
}
''',
  "A timer is a thread whose capsule sleeps first. Whether a program can end while a thread it started is still running was not checked. Open.")

# ---------------------------------------------------------------------------
section("Networking, windows and the terminal")

E("`socket`", "1.0", "open",
  r'''
s = socket.create_connection(("host", 80))
''',
  None,
  "**`satellite.network` is numbered and not built**: checked 2026-09-12, `satellite.help` on `satellite.network.open` and `satellite.network.http` is `S1101`. It is planned in PLAN.md (milestone 27 in the plan's own numbering), which is not a promise from this folder.")

E("`http.server`, `urllib.request`, `requests`", "1.0 / 3.0", "open",
  r'''
urllib.request.urlopen("https://example.com").read()
''',
  None,
  "Follows `socket`. `satellite.network.http(port)` and `https(host, …)` are among the numbered, unbuilt paths.")

E("`email`, `smtplib`, `ftplib`", "1.0 / 2.2", "never",
  r'''
smtplib.SMTP("mail.example.com")
''',
  None,
  "Protocol libraries are libraries, not language features; the CXX26 folder answers `std::linalg` the same way. If `satellite.network` lands, they would be programs written on top of it.")

E("`tkinter`", "1.1", "open",
  r'''
root = tkinter.Tk()
''',
  None,
  "`satellite.window` and `satellite.variable.window` are numbered, and `satellite.help` on `satellite.window.new` is `S1101`. Open until built.")

E("`curses` and terminal size", "1.5", "partial",
  r'''
shutil.get_terminal_size()
''',
  r'''
satellite.variable.number cols = satellite.console.width()
satellite.variable.number rows = satellite.console.height()
''',
  "The terminal's size is answered. Cursor movement, colour and key-at-a-time input are not in the word tree.")

# ---------------------------------------------------------------------------

def render_page(n, e):
    since = e["since"]
    lines = [f"# PYTHON/M{n} — {e['feature']}", ""]
    if since == "—":
        lines.append("**Introduced:** not a Python feature; it is where a Python habit lands.")
    else:
        lines.append(f"**Introduced:** Python {since}. Present in Python 3.12.")
    lines.append(f"**Answer: {link(ANSWER_WORDS(e['answer']))}.**")
    lines.append("")
    if e["py"]:
        lines += ["## Python", "", "```python", e["py"], "```", ""]
    if e["sat"]:
        lines += ["## satellite", "", "```", e["sat"], "```", ""]
    if e["differs"]:
        head = "## What differs" if e["sat"] else "## Why"
        lines += [head, "", link(resolve(e["differs"])), ""]
    return "\n".join(lines).rstrip() + "\n"

def ANSWER_WORDS(a):
    return {
        "says it": "satellite says it",
        "partial": "partial — satellite has part of it",
        "never": "never",
        "open": "open — no milestone and no refusal yet; this needs a decision",
    }.get(a, f"satellite milestone {a}")

NUMBER = {}

def resolve(text):
    """@{key}@ -> [Mn](Mn.md), where key is a feature name, a unique prefix of one,
    or `=name` for an exact match."""
    def one(mo):
        key = mo.group(1)
        if key.startswith("="):
            hits = [f for f in NUMBER if f == key[1:]]
        else:
            hits = [f for f in NUMBER if f == key] or [f for f in NUMBER if f.startswith(key)]
        if len(hits) != 1:
            sys.exit(f"reference @{{{key}}}@ matches {len(hits)} rows: {hits}")
        n = NUMBER[hits[0]]
        return f"[M{n}](M{n}.md)"
    return re.sub(r"@\{(.+?)\}@", one, text)

def main():
    index_path = sys.argv[1] if len(sys.argv) > 1 else None
    k = FIRST
    for _, entries in SECTIONS:
        for e in entries:
            NUMBER[e["feature"]] = k
            k += 1
    n = FIRST
    index = []
    asks = {}
    opens = []
    for heading, entries in SECTIONS:
        rows = []
        for e in entries:
            path = os.path.join(OUT, f"M{n}.md")
            if os.path.exists(path):
                sys.exit(f"refusing to overwrite {path}")
            with open(path, "w") as f:
                f.write(render_page(n, e))
            cell = e['feature'].replace("|", "\\|")
            rows.append(f"| {n} | {cell} | {e['since']} | {link(e['answer'])} | [M{n}](M{n}.md) |")
            if re.match(r"M(\d+)$", e["answer"]):
                asks.setdefault(e["answer"], []).append(n)
            if e["answer"] == "open":
                opens.append((n, e["feature"]))
            n += 1
        index.append((heading, rows))
    out = []
    for heading, rows in index:
        out.append(f"### {heading}\n\n| # | Python feature | since | satellite | file |\n|---|---|---|---|---|\n")
        out.append("\n".join(rows) + "\n\n")
    out.append("ASKS\n")
    for k in sorted(asks, key=lambda s: int(s[1:])):
        out.append(f"| {link(k)} | {', '.join(f'[M{x}](M{x}.md)' for x in asks[k])} |\n")
    out.append("OPENS\n")
    for x, feat in opens:
        out.append(f"| [M{x}](M{x}.md) | {feat.replace('|', chr(92) + '|')} |\n")
    with open(index_path, "w") as f:
        f.write("".join(out))
    counts = {}
    for _, entries in SECTIONS:
        for e in entries:
            k = e["answer"] if not e["answer"].startswith("M") else "milestone"
            counts[k] = counts.get(k, 0) + 1
    print("wrote", FIRST, "to", n - 1, "=", n - FIRST, "pages;", counts)

main()
