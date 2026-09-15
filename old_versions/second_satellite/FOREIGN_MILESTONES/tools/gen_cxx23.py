#!/usr/bin/env python3
# Generates FOREIGN_MILESTONES/CXX23/M57.md onward, and the index rows for them.
# Every satellite fact below was checked on 2026-09-12 against `satl --words`,
# `satellite.help`, or a probe program run through ./satl.
import os, re, sys

OUT = "/home/madness/code/cxx/satellite/FOREIGN_MILESTONES/CXX23"
FIRST = 57

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

def E(feature, std, answer, cxx=None, sat=None, differs=""):
    SECTIONS[-1][1].append(dict(feature=feature, std=std, answer=answer,
                                cxx=cxx, sat=sat, differs=(differs or "").strip()))

# ---------------------------------------------------------------------------
section("The preprocessor and program structure")

E("`#define` for a constant", "C++98", "says it",
  "#define SPAN 1000",
  "satellite.library.span = 1000",
  """
**It is a value with a path, not text substitution.** `--resolve` prints a
library value as known "at parse time", which is the useful half of a `#define`
constant. A calculation over two of them is not folded the same way yet; that is
M34.
""")

E("function-like macros", "C++98", "never",
  "#define SQUARE(x) ((x) * (x))",
  "satellite.capsule square(satellite.variable.number x) satellite.returns(satellite.variable.number)\n{\n    satellite.return(x * x)\n}",
  """
**There is no preprocessor, so nothing rewrites source text before it is
parsed.** Write a capsule. It has a type, it evaluates its argument once, and a
refusal inside it has a line number, none of which a macro gives you.
""")

E("`#if`, `#ifdef`, conditional compilation", "C++98", "never",
  "#ifdef __linux__\n    use_epoll();\n#endif",
  "satellite.variable.string os = satellite.library.main.arguments.system.name\nsatellite.statement.if (os == \"Linux\") { ... }",
  """
**One program, and it asks the machine at run time.**
`satellite.library.main.arguments` answers the machine, the system, the build and
the session, so the question an `#ifdef` answers at compile time is an `if` here.
Code that would not parse on some other platform does not exist, because there is
no platform-specific syntax.
""")

E("include guards, `#pragma once`", "C++98", "never",
  "#pragma once",
  None,
  """
**There is nothing to guard.** `satellite.include(satellite)` is the one include
that runs today, and it brings the whole language. Including a second `.satl`
file is PLAN.md's M25, a build milestone in the plan's own numbering and not one
of this folder's promises. Whether a file is loaded once or once per include is
one of that milestone's open questions.
""")

E("`__FILE__`, `__LINE__`, `std::source_location`", "C++98 / C++20", "open",
  "log(std::source_location::current().line());",
  None,
  """
**A refusal already carries file, line, caret and call stack**, so the most
common use (saying where something went wrong) happens without asking. What is
missing is a program asking *where am I* for its own logging. Nothing is
promised. The nearest milestone is M41 (reflection), but that is about the
language's numbering and not about source positions.

**Open: this needs a decision before it gets a number.**
""")

E("`static_assert`", "C++11", "M34",
  "static_assert(sizeof(int) == 4);",
  None,
  """
A check that runs before the program does. Satellite has no compile-time
evaluation to run it in, and that is M34. `--check` already runs every pass and
reports without running, which is where such a check would print.
""")

E("`__has_include`", "C++17", "never",
  "#if __has_include(<optional>)",
  None,
  """
**There is one include and it is always there.** The question underneath, *is
this feature available here?*, is M41's `installed()`, asked of the language's
own numbering rather than of a header search path.
""")

E("`#error` and `#warning`", "C++98 / C++23", "never",
  "#error \"this platform is not supported\"",
  None,
  """
There is no preprocessor to stop. To stop and say why when a condition is not
met, the milestone is M42 (contracts), which states the condition in the
program and has the refusal quote it.
""")

E("`extern \"C\"` and linkage", "C++98", "never",
  "extern \"C\" int legacy(int);",
  None,
  """
**There is no linker.** A satellite program is walked by the interpreter as
numbered operations, and there is no symbol table for C code to join. To reach C
or C++, write it into `src/` behind a numbered path. That is how every module in
the language got there, and [../ASM/README.md](../ASM/README.md) makes the same
argument one level down.
""")

E("translation units and the one-definition rule", "C++98", "partial",
  "// a.cpp and b.cpp both define f() -- a link error, or worse",
  None,
  """
**The one-definition rule is enforced, and by name.** Declaring `f` twice is
refused at check time:

    error S0242: f is already the name of something in this program, and a name is declared once

**What is partial is "more than one file"**: a program is one file today, and
`satellite.include` of another is PLAN.md's M25.
""")

E("`inline` functions and `inline` variables", "C++98 / C++17", "never",
  "inline int twice(int x) { return 2 * x; }",
  None,
  """
`inline` in C++ is about linkage: letting one definition appear in many
translation units. There are no headers to copy definitions into and no linker
to reconcile them, so there is nothing for the keyword to say.
""")

E("`using namespace`, using-declarations", "C++98", "open",
  "using namespace std;\nusing std::vector;",
  None,
  """
**Every language path is written out whole**, which is DESIGN §1:
`satellite.container.list` means the same thing in every file. Importing names
works against that rule, so even with program namespaces the likely answer for
the *language's* paths is still no.

**Namespaces a program declares do not exist yet, and nothing promises them.**
The language's own paths are dotted (`satellite.container.list`), but a program
cannot put its capsules and spacesuits under a name of its own. Every name it
declares sits at the top of the one file. Catalogue row [M4](M4.md) says
"says it — every path is dotted", which is true only of the language's paths.

**Open, waiting on one decision: does satellite get program namespaces?** If
it does, this entry follows from that decision. If it does not, it becomes
*never*.
""")

E("namespace aliases", "C++98", "open",
  "namespace fs = std::filesystem;",
  None,
  """
**Namespaces a program declares do not exist yet, and nothing promises them.**
The language's own paths are dotted (`satellite.container.list`), but a program
cannot put its capsules and spacesuits under a name of its own. Every name it
declares sits at the top of the one file. Catalogue row [M4](M4.md) says
"says it — every path is dotted", which is true only of the language's paths.

**Open, waiting on one decision: does satellite get program namespaces?** If
it does, this entry follows from that decision. If it does not, it becomes
*never*.
""")

E("anonymous namespaces, `static` internal linkage", "C++98", "open",
  "namespace { int helper() { return 1; } }",
  None,
  """
Today nothing a file declares is visible to another file, because a program is
one file. Hiding names becomes a real question once PLAN.md's M25 (another file)
and program namespaces both exist.

**Namespaces a program declares do not exist yet, and nothing promises them.**
The language's own paths are dotted (`satellite.container.list`), but a program
cannot put its capsules and spacesuits under a name of its own. Every name it
declares sits at the top of the one file. Catalogue row [M4](M4.md) says
"says it — every path is dotted", which is true only of the language's paths.

**Open, waiting on one decision: does satellite get program namespaces?** If
it does, this entry follows from that decision. If it does not, it becomes
*never*.
""")

E("inline namespaces", "C++11", "open",
  "inline namespace v2 { void f(); }",
  None,
  """
Inline namespaces version a library's ABI. The language already versions its
own surface through the numbering: a numbered path is never renumbered or reused
(WORD_NUMBERS.md §1.2). For a program's own code this would only matter after
program namespaces exist.

**Namespaces a program declares do not exist yet, and nothing promises them.**
The language's own paths are dotted (`satellite.container.list`), but a program
cannot put its capsules and spacesuits under a name of its own. Every name it
declares sits at the top of the one file. Catalogue row [M4](M4.md) says
"says it — every path is dotted", which is true only of the language's paths.

**Open, waiting on one decision: does satellite get program namespaces?** If
it does, this entry follows from that decision. If it does not, it becomes
*never*.
""")

E("nested namespace definitions `a::b::c`", "C++17", "open",
  "namespace app::net::detail { }",
  None,
  """
**Namespaces a program declares do not exist yet, and nothing promises them.**
The language's own paths are dotted (`satellite.container.list`), but a program
cannot put its capsules and spacesuits under a name of its own. Every name it
declares sits at the top of the one file. Catalogue row [M4](M4.md) says
"says it — every path is dotted", which is true only of the language's paths.

**Open, waiting on one decision: does satellite get program namespaces?** If
it does, this entry follows from that decision. If it does not, it becomes
*never*.
""")

# ---------------------------------------------------------------------------
section("Attributes")

E("`[[nodiscard]]`", "C++17", "open",
  "[[nodiscard]] bool save();",
  None,
  """
**Checked 2026-09-12: a discarded answer is silent.** A capsule that
`satellite.returns(satellite.variable.number)` can be called as a bare statement,
and nothing reports that the answer was dropped.

That matters because of M33's own rule: *ignoring the error half must be harder
than handling it*. `f.ok()` is exactly the answer somebody forgets to read.

**Open: whether a dropped answer should be a diagnostic, for every capsule or
only for some.** The tree now has a severity that does not stop the run, so this
would not have to be a refusal.
""")

E("`[[noreturn]]`", "C++11", "never",
  "[[noreturn]] void fail();",
  None,
  """
The only thing in satellite that does not return is a refusal, and the
interpreter already knows where those happen. There is nothing for a program to
declare.
""")

E("`[[deprecated]]`", "C++14", "never",
  "[[deprecated(\"use g\")]] void f();",
  None,
  """
A language word is never withdrawn: its number is permanent (WORD_NUMBERS.md
§1.2). A program's own capsules live in one file with no outside callers to warn.
If multi-file programs arrive (PLAN.md's M25), this question comes back with them.
""")

E("`[[fallthrough]]`", "C++17", "never",
  "case 1: a(); [[fallthrough]];",
  None,
  "M30 decides that satellite's `switch` has no fallthrough, so there is nothing to mark.")

E("`[[maybe_unused]]`", "C++17", "never",
  "[[maybe_unused]] int debug_only = 0;",
  None,
  """
**Checked 2026-09-12: an unused variable is not reported**, so there is no
warning to silence. If one is ever added, this entry should be revisited.
""")

E("`[[likely]]`, `[[unlikely]]`", "C++20", "never",
  "if (x) [[likely]] { }",
  None,
  """
These are hints about machine-code branch layout. Satellite walks numbered
operations in an evaluator and never emits machine code for a branch, which is
the same reason [../ASM/](../ASM/) says no.
""")

E("`[[no_unique_address]]`", "C++20", "never",
  "struct S { [[no_unique_address]] Empty e; int x; };",
  None,
  "Object layout is not visible to a satellite program, so there is nothing to optimise from inside one.")

E("`[[assume]]`", "C++23", "never",
  "[[assume(n > 0)]];",
  None,
  """
`[[assume]]` gives the optimiser permission to act as if something is true,
with undefined behaviour if it is not. Satellite has no undefined behaviour to
allow. **The satellite version of stating a condition is to have it checked**,
and that is M42.
""")

# ---------------------------------------------------------------------------
section("Types and literals")

E("`char` and character literals `'a'`", "C++98", "partial",
  "char c = 'a';\nchar first = s[0];",
  "satellite.variable.string c = \"a\"\nsatellite.variable.string first = s.at(0)",
  """
**There is no character type, and `'a'` does not lex.** Checked 2026-09-12:
`'a'` is `S0231`. A character is a one-byte string, and `s.at(n)` answers one.
It is a *byte*, not a character. See M45 for what that means outside ASCII.
""")

E("`wchar_t`, `char16_t`, `char32_t`", "C++98 / C++11", "M45",
  "std::u32string s = U\"wide\";",
  None,
  """
Satellite has one string type and it stores bytes with no declared encoding.
Wide and fixed-width code-unit types are a decision about encoding, and that
decision is M45.
""")

E("`char8_t` and `u8\"\"` literals", "C++20", "M45",
  "const char8_t *s = u8\"text\";",
  None,
  "Same as the wide types: the encoding of a satellite string is not declared yet, and M45 is that decision.")

E("the integer types: `short`, `int`, `long`, `unsigned`", "C++98", "says it",
  "unsigned long total = 0;",
  "satellite.variable.number total = 0",
  """
**One number type, arbitrary precision, no width and no sign flavour.** Checked
2026-09-12: a recursive factorial of 30 printed
`265252859812191058636308480000000` exactly. A C++ reader choosing between
`int` and `long` has no choice to make here.
""")

E("`long long` and fixed-width `<cstdint>` types", "C++11", "never",
  "std::int32_t x = 0;\nstd::uint64_t mask = 0;",
  "satellite.variable.binary mask = b0000000000000000",
  """
Numbers have no width. **When the width is the point** (a mask, a register, a
packet field), `satellite.variable.binary` and `satellite.variable.hex` carry
one and answer `.width()`.
""")

E("`float`, `double`, `long double`", "C++98", "partial",
  "double ratio = 7.0 / 2.0;",
  "satellite.variable.number ratio = 7 / 2        // 3.5, exactly\nsatellite.variable.float f = 3.25",
  """
**Most C++ uses of `double` want `satellite.variable.number`, which is exact.**
Checked 2026-09-12: `7 / 2` is `3.5` and `1 / 3` prints 34 threes, to
`satellite.library.system.division_digits`.

`satellite.variable.float` exists for when you want a float. Today it has only
conversions (`to_string`, `string`, `number`, `binary`, `hex`), which is why this
entry says partial.
""")

E("integer overflow and wraparound", "C++98", "never",
  "int x = INT_MAX; ++x;  // undefined",
  None,
  """
Numbers are arbitrary precision (`satellite_number/limbs.cpp`), so there is
nothing to overflow. The C++26 folder makes the same argument about saturating
arithmetic.
""")

E("hex `0xFF` and binary `0b1010` literals", "C++98 / C++14", "says it",
  "unsigned mask = 0xFF;\nunsigned bits = 0b1010;",
  "satellite.variable.hex mask = xFF\nsatellite.variable.binary bits = b1010",
  """
**The prefix is a letter without the zero.** Checked 2026-09-12: `xFF` and
`b0101` lex as literals, and `0xFF` is refused. The literal also has its own
type rather than being an integer. `mask.to_number()` answers `255`.
""")

E("digit separators `1'000'000`", "C++14", "open",
  "long n = 1'000'000;",
  "satellite.variable.number n = 1000000",
  """
**Checked 2026-09-12: refused.** `1'000` is `S0203`, with `'` read as a second
statement. Satellite numbers are arbitrary precision, so long literals are more
common here than in C++, and a separator would help more.

**Open: whether to have one, and which character.** `_` is what most other
languages chose.
""")

E("raw string literals `R\"(...)\"`", "C++11", "open",
  "auto re = R\"(\\d+\\.\\d+)\";",
  None,
  """
**Checked 2026-09-12: refused.** `R"(raw)"` lexes as a word followed by a
string. Escapes do work (`\\n`, `\\t` and `\\"` print as expected), so this is
only about text that is full of backslashes or quotes.

**Open, and not urgent until something like regex (below) makes backslashes
common.**
""")

E("escape sequences `\\n`, `\\t`, `\\\"`", "C++98", "says it",
  "std::cout << \"a\\tb\\n\";",
  "satellite.console.display(\"a\\tb\")",
  "Checked 2026-09-12: `\\t`, `\\n` and `\\\"` inside a string literal print as C++ would print them.")

E("user-defined literals `10_km`, standard literals `\"s\"s`, `1s`", "C++11 / C++14", "never",
  "auto d = 10_km;\nauto t = 250ms;",
  None,
  """
A literal's type is decided by the language (a number, a string, `b…`, `x…`),
and a program cannot add a spelling. For units, name the variable: `delay_ms`.
`satellite.time.sleep(n)` takes milliseconds and says so in `satellite.help`.
""")

E("`bool`, `true`, `false`", "C++98", "says it",
  "bool ready = (a == b);",
  "satellite.variable.bool ready = a == b",
  "Checked 2026-09-12: a comparison answers a `bool`, and `satellite.bool.true` and `.false` are the literals.")

E("`enum`", "C++98", "open",
  "enum Colour { Red, Green, Blue };",
  "satellite.library.red = 0\nsatellite.library.green = 1\nsatellite.library.blue = 2",
  """
**There is no enumeration.** The workaround is numbered library constants,
which have none of an enum's guarantees: nothing groups them, nothing stops a
`colour` variable holding 7, and nothing checks that a chain of ifs covers every
case.

**Open, and it pairs with M30.** `switch` gets most of its value from checking
cases against a closed set, and satellite has no closed sets to check against.
Among the open entries, this is the one most likely to become a milestone.
""")

E("`enum class`", "C++11", "open",
  "enum class Mode { Read, Write };",
  None,
  "Follows the `enum` entry above. If satellite gets enumerations, the scoped kind is the only kind worth having: every path is already dotted, so `mode.read` is the natural spelling.")

E("`using enum`, `std::to_underlying`", "C++20 / C++23", "open",
  "using enum Mode;\nint n = std::to_underlying(Mode::Read);",
  None,
  "Both depend on `enum` (above). `using enum` also runs against DESIGN §1's rule that paths are written in full.")

E("`typedef` and `using` type aliases", "C++98 / C++11", "open",
  "using Names = std::vector<std::string>;",
  "satellite.container.list<satellite.variable.string> names = satellite.container.list()",
  """
**Every type is spelled in full every time**, and a map of lists means a
declaration longer than most of the lines that use it.

**Open, and it is a real tension with DESIGN §1.** Full paths make every line
readable without looking anything up, and an alias is by definition something to
look up. One option that keeps §1: an alias as a *spacesuit*, a named wrapper
that the reader can find by its declaration.
""")

E("`union`", "C++98", "never",
  "union Value { int i; double d; };",
  "satellite.variable.variant value = 5",
  """
**`satellite.variable.variant` is the safe version, and it knows what it holds.**
Checked 2026-09-12: `holding()` answered `number`, and after `value = "text"`
it answered `string`. Reading the wrong member of a union is undefined behaviour
in C++. Satellite has no way to write that bug.
""")

E("bit-fields", "C++98", "never",
  "struct Flags { unsigned ready : 1; unsigned mode : 3; };",
  None,
  "Memory layout is not visible to a program. For bit-level values, `satellite.variable.binary` has a width and converts with `.to_number()` and `.to_hex()`.")

E("`sizeof`", "C++98", "never",
  "std::size_t n = sizeof(Thing);",
  "satellite.variable.number n = things.size()\nsatellite.variable.number bits = satellite.library.main.arguments.machine.pointer_bits",
  """
A value's byte size is not visible to a program, because none of its code
depends on layout. What people usually want from `sizeof` is somewhere else: a
container's element count (`.size()`), or the machine's word size (above).
""")

E("`alignof`, `alignas`", "C++11", "never",
  "alignas(64) struct Cache { };",
  None,
  "Alignment is layout, and layout belongs to the interpreter.")

E("`decltype`, `decltype(auto)`", "C++11 / C++14", "never",
  "decltype(a + b) sum = a + b;",
  None,
  "Every declaration names its type, the same decision as [M7](M7.md) (`auto`). There is no inference for `decltype` to ask.")

E("return type deduction `auto f()`", "C++14", "never",
  "auto twice(int x) { return 2 * x; }",
  "satellite.capsule twice(satellite.variable.number x) satellite.returns(satellite.variable.number)",
  "A capsule that answers something says what with `satellite.returns(type)`. The same no-inference rule as [M7](M7.md).")

E("trailing return types `auto f() -> int`", "C++11", "says it",
  "auto area(double r) -> double;",
  "satellite.capsule area(satellite.variable.number r) satellite.returns(satellite.variable.number)",
  """
**This is satellite's only spelling.** The return type goes after the
parameters, which is where C++11 moved it to make return types readable. A C++
reader who prefers trailing return types already writes the satellite shape.
""")

E("`static_cast` and numeric/string conversions", "C++98", "says it",
  "int n = std::stoi(s);\nstd::string t = std::to_string(n);\ndouble d = static_cast<double>(n);",
  "satellite.variable.number n = s.to_number()\nsatellite.variable.string t = n.to_string()\nsatellite.variable.float d = n",
  """
Checked 2026-09-12: all three work. `satellite.variable.string s = n` also
converts a number on assignment, and `+` joins a number onto a string. Those are
the two implicit conversions to know about. Methods on a call's *answer* still
refuse (`s.to_number().round()` is S0720, see M29).
""")

E("`dynamic_cast` and `typeid`", "C++98", "open",
  "if (auto *d = dynamic_cast<Derived *>(base)) { }",
  "satellite.variable.string kind = value.holding()",
  """
**A variant can name what it holds.** `holding()` answers `number` or
`string`. **A spacesuit cannot be asked what it is**, and there is no base
handle to downcast from yet: [M28](M28.md) records that inheritance exists but
dispatch through a base handle does not.

**Open, and it depends on that.** Until a handle can refer to a derived suit
through its base type, a cast has nothing to do.
""")

E("`reinterpret_cast`", "C++98", "never",
  "auto *p = reinterpret_cast<char *>(&x);",
  None,
  "There are no addresses and no representations to reinterpret. The legitimate uses (viewing the bits of a value) are `.binary` and `.hex`, which exist on number and float.")

E("`const_cast`", "C++98", "never",
  "const_cast<T &>(x) = y;",
  None,
  "There is no `const`, the same decision as [M8](M8.md), so there is nothing to cast away.")

E("implicit conversions and narrowing", "C++98 / C++11", "partial",
  "int n = 3.9;      // 3, silently\nint m{3.9};       // error since C++11",
  None,
  """
**Satellite has no narrowing to worry about**: a number holds 3.9 exactly. What
C++ readers should know is where it *does* convert without being asked, checked
2026-09-12:

- a number assigned to a string becomes its text
- `+` with a string on one side joins a number onto it
- a number assigned to a float becomes a float

**And where it refuses instead.** `<` on two strings is `S0712`: *string has no
order*. Yet `list.sort()` on a list of strings sorts them (`b,a` became `a,b`).
That is worth knowing, and worth a decision, because the two disagree about
whether strings are ordered.
""")

E("`volatile`", "C++98", "never",
  "volatile int *reg = (volatile int *)0x40000000;",
  None,
  "`volatile` is for memory that hardware changes behind the compiler's back. Satellite has neither the memory access nor the optimiser for it to matter.")

E("the `mutable` keyword", "C++98", "never",
  "mutable int cache_hits;",
  None,
  "Every name is mutable already, because DESIGN §12 decided against `const`. `mutable` only means something as an exception to `const`.")

E("`nullptr` and `NULL`", "C++98 / C++11", "says it",
  "Thing *p = nullptr;\nif (p == nullptr) { }",
  "satellite.variable.variant v\nsatellite.statement.if (v.holding() == \"nothing\") { }",
  """
**There are no pointers, so there is no null pointer.** "Nothing" is a state
every type has (DESIGN §8.7). Checked 2026-09-12: displaying a declared,
unassigned number prints `nothing`. For a value that is sometimes absent, use a
variant and ask it, as [M10](M10.md) (`std::optional`) does.
""")

E("uninitialised variables", "C++98", "says it",
  "int n;            // indeterminate\nstd::cout << n;   // undefined",
  "satellite.variable.number n\nsatellite.console.display(n)     // nothing",
  """
**A declaration with no value holds nothing, not garbage.** Checked 2026-09-12.
The C++ bug, reading memory nobody wrote, is not expressible.
""")

E("`std::byte`", "C++17", "says it",
  "std::byte b{0x2A};",
  "satellite.variable.hex b = x2A",
  "`satellite.variable.binary` and `satellite.variable.hex` are the bit-level types, and they convert between each other and to numbers.")

E("aggregate initialisation `Point p{1, 2}`", "C++98 / C++11", "open",
  "Point p{1, 2};",
  "point p\np.call_set_x(1)\np.call_set_y(2)",
  """
**A spacesuit is declared and then set.** Checked 2026-09-12: a parameter list
on a spacesuit declaration is read as a superclass (`S0201: expected ')' to close
the superclass`), so constructor arguments are not a spelling yet. Every field
gets its declared default, and a `call_set_` verb changes it.

**Open, together with [M27](M27.md)**, the constructors entry, which is
partial for the same reason.
""")

E("designated initialisers `.x = 1`", "C++20", "open",
  "Point p{.x = 1, .y = 2};",
  None,
  "Follows aggregate initialisation above. Naming each field at the construction site is the readable form of that feature, and it fits satellite's explicit style.")

E("brace initialisation and `std::initializer_list`", "C++11", "open",
  "std::vector<int> v{1, 2, 3};",
  "satellite.container.list<satellite.variable.number> v = satellite.container.list()\nv.append(1)\nv.append(2)\nv.append(3)",
  """
**There is no list literal.** Checked 2026-09-12: `[1, 2, 3]` is `S0231`. So
a list of known contents costs a line per element, and a table of constants
costs a capsule full of appends.

**Open, and a strong candidate for a milestone.** `l[i]` already uses square
brackets for subscripting, so a literal spelling has to fit around that (M35
raises the same collision).
""")

E("`std::any`", "C++17", "says it",
  "std::any a = 5;\na = std::string(\"text\");",
  "satellite.variable.variant a = 5\na = \"text\"\nsatellite.console.display(a.holding())   // string",
  """
Checked 2026-09-12: a variant took a number and then a string, and named each.
The difference from `std::any` is that nothing has to be `any_cast` back out,
because the variant is used as what it holds.
""")

E("`std::pair`, `std::tuple`, `std::tie`, `std::apply`", "C++98 / C++11 / C++17", "M35",
  "auto [q, r] = std::pair{7 / 2, 7 % 2};",
  None,
  """
A capsule answers exactly one value, so a pair is a spacesuit with two `call_`
verbs today. M35 describes both halves: destructuring, and a capsule answering
more than one value, which needs a tuple type.
""")

E("`std::array` and C arrays", "C++98 / C++11", "says it",
  "std::array<int, 4> a{};",
  "satellite.container.list<satellite.variable.number> a = satellite.container.list()",
  """
There is one list and its size is not part of its type. `a[i]` reads and
assigns (checked 2026-09-12: `l[0] = 9` then `l[0]` printed `9`). What you lose
is the compile-time length.
""")

E("multidimensional arrays and `std::mdspan`", "C++98 / C++23", "says it",
  "std::vector<std::vector<int>> grid;",
  "satellite.container.list<satellite.container.list<satellite.variable.number>> grid = satellite.container.list()",
  """
Checked 2026-09-12: a list of lists declares, appends and sizes correctly.
`std::mdspan` is a *view* over flat storage, and satellite has no views (see
`std::span`).

**Watch the copy.** A list is copied on assignment and when passed to a capsule,
so `row = grid[i]` then `row.append(x)` does not change `grid`. See [M9](M9.md).
""")

E("`std::span`", "C++20", "M36",
  "void total(std::span<const int> values);",
  None,
  """
**A span exists to avoid a copy, and satellite copies.** Checked 2026-09-12:
`b = a` then `b.append(2)` left `a` at size 1. A capsule taking a list therefore
takes its own list. M36 decides against lazy views, so a span-like non-owning
view is not planned. To share one list, wrap it in a spacesuit ([M9](M9.md)).
""")

E("`std::string_view`", "C++17", "says it",
  "void greet(std::string_view name);",
  "satellite.capsule greet(satellite.variable.string name)",
  "There is one string type. The problem `string_view` solves, choosing between `const char *` and `const std::string &`, does not exist here.")

# ---------------------------------------------------------------------------
section("Operators and expressions")

E("arithmetic `+ - * /`", "C++98", "says it",
  "int q = 7 / 2;     // 3",
  "satellite.variable.number q = 7 / 2      // 3.5",
  """
**`/` is exact division, not integer division.** Checked 2026-09-12: `7 / 2`
is `3.5`. This is the most likely surprise for a C++ reader. For C++'s answer,
take `.truncate()` of a named result (the one-hop rule, M29, means it cannot go
on the expression). Dividing by zero is refused with `S0601`, for floats too.
""")

E("`%`", "C++98", "says it",
  "int r = 7 % 3;",
  "satellite.variable.number r = 7 % 3",
  "Checked 2026-09-12: `7 % 3` prints `1`. `satellite.variable.number.modulus(a, b)` also exists as a method.")

E("compound assignment `+=`, `-=`, `*=`, `/=`", "C++98", "open",
  "total += x;",
  "total = total + x",
  """
**Checked 2026-09-12: refused.** `x += 2` is `S0231: expected an expression …
and found Punct(=)`. It is the most frequent verbosity in a satellite loop, and
it takes no new concept to add.

**Open, and cheap.** A parser rewrite to `x = x + 2` with no evaluator change.
The only design question is whether all of them come together, including the
bitwise ones, which need bitwise operators first (below).
""")

E("increment and decrement `++`, `--`", "C++98", "open",
  "for (int i = 0; i < n; ++i)",
  "satellite.statement.for (satellite.variable.number i = 0; i < n; i = i + 1)",
  """
**Checked 2026-09-12: refused**, `S0231` on the second `+`. Every loop header
in the example programs spells out `i = i + 1`.

**Open.** If satellite adds it, the safe choice is a *statement* only
(`i++` on its own line, never inside an expression). That avoids C++'s
pre/post-increment and sequencing traps, and it matches satellite's one
statement per line.
""")

E("bitwise `&`, `|`, `^`, `~`", "C++98", "open",
  "flags = flags | READY;",
  None,
  """
**Checked 2026-09-12: refused as operators** (`6 & 3` is `S0203`), and today's
`satl --words` lists no and/or/xor/not method on `number`, `binary` or `hex`.
Shifts do exist (next entry).

**Open.** `binary` and `hex` are where a program does bit work, so methods
there fit the existing style better than operators. Note that `&&` and `||` are
M28, and a `&` operator would have to coexist with them.
""")

E("shifts `<<` and `>>`", "C++98", "says it",
  "unsigned big = one << 10;",
  "satellite.variable.number big = one.shift_left(10)",
  "`shift_left(n)` and `shift_right(n)` are number methods. The operator is refused (`1 << 3` is `S0231`), and in satellite `<<` will never mean stream output either.")

E("comparison `==`, `!=`, `<`, `<=`, `>`, `>=`", "C++98 / C++20", "says it",
  "if (a != b && a >= c)",
  "satellite.statement.if (a != b) { ... }",
  """
All six work on numbers. **On strings, only `==` and `!=` work**, and `<` is
refused with `S0712` (*string has no order*). A spacesuit cannot be compared at
all until M38. Combining two comparisons is M28.
""")

E("logical not `!`", "C++98", "says it",
  "if (!ready) { }",
  "satellite.statement.if (!ready) { ... }",
  """
**Checked 2026-09-12: `!` works.** With `ready` false, `!ready` took the branch.

**This corrects the catalogue.** Row 20 lists `!` together with `&&` and `||`
under M28. `&&` and `||` are still missing; `!` is not. M28's own page talks
about adding `not`, so it should say that `!` already exists.
""")

E("unary minus", "C++98", "says it",
  "int x = -7;",
  "satellite.variable.number x = -7",
  "Checked 2026-09-12: `-7` works. (`example/scalars.satl` writes `0 - 7`, which predates this and is not needed.)")

E("the comma operator", "C++98", "never",
  "for (i = 0, j = n; i < j; ++i, --j)",
  None,
  "A statement ends at the end of its line, and a comma is not an operator. For a two-counter loop, keep the second counter in the body.")

E("`sizeof...` on a parameter pack", "C++11", "M31",
  "sizeof...(Args)",
  None,
  "Parameter packs are [M37](M37.md) (C++ packs), which is M31. There is nothing to count until a program can write a variadic capsule.")

E("order of evaluation and undefined behaviour", "C++98", "never",
  "i = i++ + ++i;   // undefined before C++17",
  None,
  """
Satellite has no undefined behaviour: an operation either has an answer or is a
refusal with a line number (division by zero is `S0601`, a missing map key is
`S0726`). The classic sequencing bugs also need `++`, which does not exist (see
above).
""")

E("pointer arithmetic", "C++98", "never",
  "*(p + 3) = 0;",
  "values[3] = 0",
  "No pointers ([M9](M9.md)). A subscript on a list is the whole of it.")

E("member access `.`, `->`, `.*`, `->*`", "C++98", "says it",
  "p->method();\n(obj.*member)();",
  "p.call_method()",
  "A spacesuit is a reference already, so access is always a dot. Pointers to members are `never`, and the capsule-as-value half of them is M32.")

E("scope resolution `::`", "C++98", "partial",
  "std::chrono::steady_clock::now()\napp::net::connect();",
  "satellite.time.now()",
  """
**For the language's own paths, `.` does what `::` does**, and `foreign.cpp`
answers `::` with `.`. **For a program's own names, there is nothing to qualify
yet**: a program cannot declare a namespace (see the namespace entries above and
catalogue row [M4](M4.md)).
""")

E("parentheses and precedence", "C++98", "says it",
  "int x = (1 + 2) * 3;",
  "satellite.variable.number x = (1 + 2) * 3",
  "Checked 2026-09-12: prints `9`.")

E("string concatenation with `+`", "C++98", "says it",
  "std::string line = \"x is \" + std::to_string(x);",
  "satellite.variable.string line = \"x is \" + x",
  "A number joins directly, with no `to_string`. This is what keeps most display lines short despite the one-hop rule ([M3](M3.md)).")

# ---------------------------------------------------------------------------
section("Statements")

E("range-based `for`", "C++11", "open",
  "for (const auto &name : names) { }",
  "satellite.statement.for (satellite.variable.number i = 0; i < names.size(); i = i + 1)\n{\n    satellite.variable.string name = names[i]\n}",
  """
**Checked 2026-09-12: refused.** `for (T x : l)` is `S0201: expected ';'`.
Every walk over a container is an index loop plus a named element.

**Open, and probably the highest-value entry in this section.** It is the
loop Python, Java and Rust readers all expect, and it needs no iterator
protocol: satellite has two containers, so a `for` over a list and a `for` over
a map's keys covers nearly all of it. [M16](M16.md) ("never — index instead")
refuses *iterators*, and this should not be read as reversing that.
""")

E("`do … while`", "C++98", "open",
  "do { line = read(); } while (!line.empty());",
  None,
  """
**Checked 2026-09-12: refused**, `S0212: no statement is spelled do --
satellite.statement has if, else, while and for`. The workaround is a
`while (satellite.bool.true)` loop, which needs `break` (M27), or running the
body once before the loop.

**Open.** Small, and it would take the next free number under
`satellite.statement`, which is currently wanted by M27 (twice) and M30 as well.
""")

E("`else if`", "C++98", "says it",
  "if (x == 0) { } else if (x == 1) { }",
  "satellite.statement.if (x == 0) { ... }\nsatellite.statement.else satellite.statement.if (x == 1) { ... }",
  """
**Checked 2026-09-12: this runs.** An `else` followed directly by an `if`
works without nesting the `if` in braces.

**This corrects `foreign.cpp`**, whose `elif` row says *"there is no `else if`
-- nest the if inside the else"*. That note is out of date. Flagged for the
session working on the interpreter rather than edited here.
""")

E("an initialiser in `if` and `switch`", "C++17", "open",
  "if (auto f = open(path); f.ok()) { }",
  None,
  """
`satellite.statement.for` has an init part and `if` does not, so a name used
only by one `if` is declared before it and stays in scope afterwards.

**Open, and low priority.** Satellite already puts every declaration on its own
line, and this feature exists to take one away.
""")

E("a declaration in the `for` header", "C++98", "says it",
  "for (int i = 0; i < 5; ++i)",
  "satellite.statement.for (satellite.variable.number i = 0; i < 5; i = i + 1)",
  "`example/scalars.satl` does exactly this.")

E("block scope and shadowing", "C++98", "says it",
  "int y = 1;\nif (y == 1) { int y = 2; }\n// y is 1",
  None,
  """
Checked 2026-09-12: a bare `{ }` block is accepted, and a `y` declared inside an
`if` block did not change the outer `y`, which still printed `1`. That is C++'s
behaviour. Satellite does not warn about the shadowing either.
""")

E("`return;` from a `void` function", "C++98", "says it",
  "void f() { return; }",
  "satellite.capsule f()\n{\n    satellite.return()\n}",
  "`satellite.return()` is the no-value shape. `satellite.return(satellite)` is the one `main` uses, and `satellite.return(value)` answers something.")

E("`return` out of a loop", "C++98", "says it",
  "for (int i = 0; i < 10; ++i) if (i == 3) return i;",
  "satellite.statement.for (satellite.variable.number i = 0; i < 10; i = i + 1)\n{\n    satellite.statement.if (i == 3) { satellite.return(i) }\n}",
  """
Checked 2026-09-12: this answered `3`. **It is the cleanest workaround for
M27** until `break` exists: move the loop into its own capsule and `return`
from it.
""")

E("semicolons and several statements on one line", "C++98", "never",
  "a = 1; b = 2;",
  "a = 1\nb = 2",
  "A statement ends at the end of its line. A trailing `;` is refused (`S0203`), and the message says to put the second statement on its own line.")

E("several declarators `int a = 1, b = 2;`", "C++98", "never",
  "int a = 1, b = 2;",
  "satellite.variable.number a = 1\nsatellite.variable.number b = 2",
  "One declaration per line. `S0203` on the comma.")

E("block comments `/* … */`", "C++98", "open",
  "/* several\n   lines */",
  "// several\n// lines",
  """
**Checked 2026-09-12: refused**, `S0231` at the `/`. Only `//` comments exist.

**Open.** `foreign.cpp`'s guard would need a matching rule, because it skips
lines starting with `//` and a `/*` line would get advice.
""")

E("`if consteval`", "C++23", "M34",
  "if consteval { } else { }",
  None,
  "There is no compile-time evaluation to test for. M34, alongside [M24](M24.md) (`if constexpr`).")

# ---------------------------------------------------------------------------
section("Declarations and lifetime")

E("global variables", "C++98", "says it",
  "int counter = 5;\nint main() { ++counter; }",
  "satellite.library.counter = 5\n\nsatellite.capsule satellite.main()\n{\n    satellite.library.counter = satellite.library.counter + 1\n}",
  """
Checked 2026-09-12: printed `6`. **A global is always written with its full
path**, so every use of shared state is visible where it happens. The
library's thread behaviour is covered in M40, and the folder currently
contradicts itself about it. That is worth settling there, not here.
""")

E("`static` local variables", "C++98", "says it",
  "int next_id() { static int id = 0; return ++id; }",
  "satellite.library.next_id = 0",
  "There are no per-capsule statics. A value that outlives one call is a `satellite.library` value, visible to the whole file.")

E("`thread_local`", "C++11", "open",
  "thread_local int scratch = 0;",
  None,
  """
Each call gets its own frame (DESIGN §7.2), so locals are already per-thread.
`satellite.library` is shared by every thread. **Nothing sits in between**, and
per-thread state that lasts across calls has to be passed around in a spacesuit.

**Open, and it belongs next to M40.**
""")

E("local functions and local classes", "C++98 / C++11", "never",
  "void f() { struct Local { }; auto g = [] { }; }",
  None,
  """
Checked 2026-09-12: `S0213: satellite.capsule is a declaration and goes at the
top of a file, not inside a block`. Declarations live at file scope. The
lambda half of this entry is M32.
""")

E("forward declarations and prototypes", "C++98", "says it",
  "int helper(int);\nint main() { return helper(1); }\nint helper(int x) { return x; }",
  None,
  "**Nothing is declared before use.** Resolution sees every capsule name and then every spacesuit name before it resolves bodies, so a capsule can call one written further down the file.")

E("headers and source files", "C++98", "never",
  "// thing.hpp declares, thing.cpp defines",
  None,
  "A capsule is declared and defined in one place. More than one file is PLAN.md's M25, and satellite has no separate declaration file for it to split into.")

E("recursion", "C++98", "says it",
  "long fact(long n) { return n < 2 ? 1 : n * fact(n - 1); }",
  "satellite.capsule fact(satellite.variable.number n) satellite.returns(satellite.variable.number)\n{\n    satellite.statement.if (n < 2) { satellite.return(1) }\n    satellite.variable.number m = n - 1\n    satellite.variable.number sub = fact(m)\n    satellite.return(n * sub)\n}",
  """
Checked 2026-09-12: `fact(30)` printed exactly. The call depth is bounded by
`satellite.library.system.max_depth`, which a refusal names, instead of by a
stack overflow.
""")

# ---------------------------------------------------------------------------
section("Classes, in detail")

E("`this`", "C++98", "never",
  "return *this;\nregistry.add(this);",
  None,
  """
**There is no `this`**, and a member capsule cannot name the object it
belongs to. An object that must hand out its own handle is told it once, from
outside, by whoever made it. `infinity_data_main.satl` does this and explains
the cost at the call site. [M9](M9.md) and [M34](M34.md) (deducing this) say
the same.
""")

E("member functions calling each other", "C++98", "says it",
  "void Suit::call_erase() { erase(); }",
  "satellite.capsule call_erase()\n{\n    erase_binary_data()\n}",
  "Inside a spacesuit, a capsule calls another by its bare name, and reads fields by bare name too. `example/class_test.satl` does both.")

E("`static` data members", "C++98", "partial",
  "struct Counter { static int made; };",
  "satellite.library.counters_made = 0",
  "There is no per-type storage. A `satellite.library` value is per program, which is the same thing when a file has one type that needs it, and a naming convention when it has several.")

E("`static` member functions", "C++98", "says it",
  "struct Maths { static int twice(int); };",
  "satellite.capsule twice(satellite.variable.number x) satellite.returns(satellite.variable.number)",
  "A capsule at file scope is what a static member function is. Satellite has no reason to attach it to a type.")

E("`const` member functions", "C++98", "never",
  "int size() const;",
  None,
  "No `const`, the same decision as [M8](M8.md). A `call_` verb that does not change the object simply does not change it.")

E("`friend`", "C++98", "never",
  "friend class Inspector;",
  None,
  "A spacesuit's protected section is reachable only from inside the suit and its subclasses. There is no mechanism for granting an outsider access. The C++26 folder says the same about variadic friends.")

E("nested classes", "C++98", "open",
  "class Outer { class Inner { }; };",
  None,
  """
**Checked 2026-09-12: refused.** A `satellite.spacesuit` inside a protected
section is `S0207: expected a field or a satellite.capsule`.

**Open.** It is a real use (a node type private to its list), and it collides
with "declarations go at the top of a file" (see local classes).
""")

E("`explicit` constructors", "C++98 / C++11", "never",
  "explicit Meters(double);",
  None,
  "Nothing converts to a spacesuit implicitly, so there is no conversion to forbid.")

E("delegating constructors", "C++11", "open",
  "Point() : Point(0, 0) { }",
  None,
  "Follows [M27](M27.md) (constructors, partial). A spacesuit has field defaults and no constructor arguments yet, so one constructor has nothing to delegate to another.")

E("inheriting constructors `using Base::Base`", "C++11", "open",
  "struct Derived : Base { using Base::Base; };",
  None,
  "Follows [M27](M27.md) and [M28](M28.md). A suit holds a copy of its superclass and gets its field defaults. Arguments would have to exist first.")

E("default member initialisers", "C++11", "says it",
  "struct S { int x = 3; };",
  "satellite.spacesuit s()\n{\n    satellite.protected\n    {\n        satellite.variable.number x = 3\n    }\n}",
  "A field's initialiser is written where it is declared. `example/class_test.satl` sets a binary field this way.")

E("member initialiser lists", "C++98", "open",
  "Point(int x, int y) : x_(x), y_(y) { }",
  None,
  "There are no constructor arguments to initialise from ([M27](M27.md)). Field defaults cover the constant case; everything else is a `call_set_` verb after declaration.")

E("copy constructors, copy assignment, the rule of three/five/zero", "C++98 / C++11", "never",
  "Thing(const Thing &);\nThing &operator=(Thing &&) noexcept;",
  None,
  """
**Satellite takes the rule of zero to its conclusion.** A spacesuit is a
handle, so assigning one shares the object, and a list or map copies itself
correctly on assignment (checked 2026-09-12). No type can get copying wrong,
because no type writes it.

**The one thing to learn is which is which.** A spacesuit is shared and a
container is copied. [M9](M9.md) describes the bug this causes.
""")

E("`= default` and `= delete`", "C++11", "never",
  "Thing(const Thing &) = delete;",
  None,
  "There are no special members to default or delete (above). The C++26 folder records the same answer for `= delete(\"reason\")`.")

E("conversion operators `operator bool()`", "C++98 / C++11", "never",
  "explicit operator bool() const;",
  None,
  "M38 limits user operators to `+`, `==` and `<` on purpose, and names conversion operators as where C++ overloading became unreadable. A suit answers `call_is_valid()` instead.")

E("`operator[]`, `operator()`, multidimensional `operator[]`", "C++98 / C++23", "never",
  "T &operator[](std::size_t);\nbool operator()(int) const;",
  None,
  "Excluded by M38 for the same reason as conversions. A function object's real use, passing behaviour around, is M32.")

E("abstract classes and pure `virtual`", "C++98", "M31",
  "struct Shape { virtual double area() const = 0; };",
  None,
  "An interface with no body is the same question as Java's `interface` and Rust's `trait`, which `foreign.cpp` sends to M31 and \"a decision after it\".")

E("`override`", "C++11", "partial",
  "double area() const override;",
  "satellite.capsule area() satellite.returns(satellite.variable.number)",
  """
A subclass declares a capsule with the same name, which is what `foreign.cpp`
tells a Java reader about `@Override`. **Partial, because what `override`
protects is dispatch through a base handle, and that does not exist yet**
([M28](M28.md)). There is also no check that the name matches something in the
superclass.
""")

E("`final`", "C++11", "never",
  "struct Leaf final : Base { };",
  None,
  "Satellite has no virtual dispatch to seal and no mechanism for preventing subclasses. If dispatch through a base arrives, this question arrives with it.")

E("multiple inheritance", "C++98", "open",
  "struct C : A, B { };",
  None,
  """
**Checked 2026-09-12: refused.** `satellite.spacesuit c(a, b)` is `S0201:
expected ')' to close the superclass`. One superclass only.

**Open. No decision is recorded anywhere**, as opposed to a refusal of
one. The deciding question is what "holds a copy of its superclass" means when
two superclasses share a field name.
""")

E("virtual inheritance and the diamond", "C++98", "never",
  "struct B : virtual A { };",
  None,
  "Only exists with multiple inheritance (above). If that stays refused, this does not arise.")

E("`std::enable_shared_from_this`", "C++11", "never",
  "struct Node : std::enable_shared_from_this<Node> { };",
  None,
  "Its satellite counterpart is being told your own handle from outside, which is written by hand. A suit has no `this` to make a shared handle from ([M9](M9.md)).")

E("`std::weak_ptr`", "C++11", "open",
  "std::weak_ptr<Node> parent;",
  None,
  """
**This is the one smart pointer satellite may actually need.** Refcounting
never frees a cycle (DESIGN §12), and `infinity_data_main.satl` has one on
purpose. §12 names a weak-reference field as the cheap partial fix and a
tracing collector as the complete one, and chooses neither. PLAN.md's M26
recorded that choice as its blocker.

**Open, and it is the author's decision.**
""")

E("object slicing", "C++98", "never",
  "Base b = derived;   // the Derived part is gone",
  None,
  "Slicing happens when an object is copied by value into its base type. A spacesuit is never copied by value, so there is nothing to slice.")

E("object layout, standard-layout types, EBO", "C++98 / C++11", "never",
  "std::is_standard_layout_v<T>",
  None,
  "Layout is the interpreter's. No satellite program can observe it, so no guarantee about it is needed.")

# ---------------------------------------------------------------------------
section("Templates, in detail")

E("function templates", "C++98", "M31",
  "template <typename T> T biggest(T a, T b);",
  None,
  "M31. Today a capsule takes one parameter type, so a generic helper is written once per type.")

E("class templates", "C++98", "M31",
  "template <typename T> class Stack { std::vector<T> items; };",
  None,
  "M31. Its page quotes the two identical spacesuits in `infinity_data_main.satl` that differ only in element type.")

E("template specialisation, full and partial", "C++98", "M31",
  "template <> struct Hash<std::string> { };",
  None,
  "Follows M31. **Full specialisation is the likely boundary.** Partial specialisation is a second pattern-matching language inside the type system, and M31 asks for a decision on monomorphisation before anything that deep.")

E("variable templates", "C++14", "M31",
  "template <typename T> constexpr T pi = T(3.14159);",
  None,
  "Needs M31 for the template and M34 for the constant.")

E("alias templates", "C++11", "M31",
  "template <typename T> using Grid = std::vector<std::vector<T>>;",
  None,
  "Needs M31, and it is the generic form of the type-alias question, which is open (see `typedef`).")

E("class template argument deduction (CTAD)", "C++17", "never",
  "std::vector v{1, 2, 3};   // vector<int>",
  None,
  "Every declaration names its type, the same decision as [M7](M7.md). When M31 lands, arguments are written out.")

E("SFINAE and `std::enable_if`", "C++98 / C++11", "never",
  "template <typename T, typename = std::enable_if_t<std::is_integral_v<T>>>",
  None,
  "SFINAE is an accident of overload resolution that became a technique. Constraining a generic is [M36](M36.md) (concepts), which follows M31. Satellite should never have the accident.")

E("`<type_traits>`", "C++11", "never",
  "std::is_same_v<T, int>",
  None,
  "Compile-time type computation is template metaprogramming, which satellite is not getting. A *program* asking what something is at run time is M41.")

E("non-type template parameters, `auto` template parameters", "C++98 / C++17", "M31",
  "template <std::size_t N> struct Buffer { };",
  None,
  "Follows M31. With no fixed-size containers, the common use (a length in the type) has less to do here.")

E("template template parameters", "C++98", "never",
  "template <template <typename> class C> struct Wrap { };",
  None,
  "Satellite has two containers, so there is no family of containers for a template to take.")

E("abbreviated function templates `void f(auto x)`", "C++20", "never",
  "void print(auto value);",
  None,
  "Every parameter names its type. A capsule over any type is M31, spelled with a type parameter.")

E("`requires` clauses", "C++20", "M31",
  "template <typename T> requires std::integral<T> T half(T);",
  None,
  "Constraints come after generics: [M36](M36.md) (concepts) is M31, after generics.")

E("explicit instantiation and `extern template`", "C++98 / C++11", "never",
  "extern template class std::vector<int>;",
  None,
  "These control where template code is compiled and linked. There is no object code or linker, and M31's monomorphisation question is internal.")

E("CRTP", "C++98", "never",
  "struct Derived : Base<Derived> { };",
  None,
  "The curiously recurring template pattern gets static dispatch out of templates. It needs M31 and a use for static dispatch, and satellite has no compile-time dispatch to want.")

E("perfect forwarding and `std::forward`", "C++11", "never",
  "template <typename... A> void make(A &&...args) { T(std::forward<A>(args)...); }",
  None,
  "Forwarding preserves value category. Satellite has no references or rvalues to preserve ([M31](M31.md) move semantics is `never` for the same reason).")

# ---------------------------------------------------------------------------
section("Functions, in detail")

E("pass by value and pass by reference", "C++98", "partial",
  "void fill(std::vector<int> &out);",
  None,
  """
**The rule differs by type, which makes this the single most important
entry in the catalogue for a C++ reader.**

- **a spacesuit is passed by handle.** The capsule changes the caller's object.
- **a list or a map is copied.** Checked 2026-09-12: `b = a` then `b.append(2)`
  left `a` holding one item. [M9](M9.md) measured the capsule case: five appends
  inside, zero seen outside, and no diagnostic.

So `void fill(std::vector<int> &out)` has no direct spelling. Wrap the list in a
spacesuit and write through a `call_` verb, or return the list.
""")

E("function pointers", "C++98", "M32",
  "int (*op)(int, int) = add;",
  None,
  "A capsule can be called and not passed. `satellite.variable.capsule` is numbered, and M32 is what lets it hold one.")

E("pointers to members", "C++98", "never",
  "int Point::*field = &Point::x;",
  None,
  "Field access is only through the suit's own verbs, which is DESIGN §12's rule. The *method* half, a callable bound to a suit, is M32.")

E("lambda captures and init-capture", "C++11 / C++14", "M32",
  "auto add = [base](int x) { return base + x; };\nauto g = [n = compute()] { };",
  None,
  "M32, and its page names the hard part: a capture that outlives the frame it was written in, which is exactly what DESIGN §7.2's frame-per-call must not lose.")

E("generic lambdas", "C++14", "M31",
  "auto twice = [](auto x) { return x + x; };",
  None,
  "Needs both M31 and M32.")

E("`std::bind`, `std::bind_front`, `std::invoke`", "C++11 / C++17 / C++20", "M32",
  "auto f = std::bind_front(&Suit::method, suit);",
  None,
  "Calling or partially applying a callable value. M32.")

E("`std::move_only_function`", "C++23", "M32",
  "std::move_only_function<void()> task;",
  None,
  "A callable value (M32). The move-only half does not apply, because nothing in satellite is move-only.")

E("C variadic functions `...`", "C++98", "never",
  "int sum(int count, ...);",
  None,
  """
The arity is part of a path's identity (PLAN.md §8 on `satellite.network`:
*the arity is the identity*), so a variable argument count is a different shape,
not a flag. To pass many values, pass a list. A type-safe variadic is M31's
parameter packs.
""")

E("`constinit`", "C++20", "M34",
  "constinit int table_size = compute();",
  None,
  "Guaranteeing a global is initialised before the program runs is M34. `satellite.library` values from literals are already resolved at parse time.")

E("`std::reference_wrapper` and `std::ref`", "C++11", "says it",
  "std::vector<std::reference_wrapper<Big>> refs;",
  "satellite.container.list<big_suit> refs = satellite.container.list()",
  "A list of spacesuits already holds handles, and each item refers to the same object as the original.")

# ---------------------------------------------------------------------------
section("Errors, in detail")

E("`assert` from `<cassert>`", "C++98", "M42",
  "assert(count >= 0);",
  None,
  """
M42 (contracts). **Assert-as-a-statement is the smaller half and could come
first.** It stops the run with the condition quoted. A C++ `assert` disappears
under `NDEBUG`; satellite's would not, because satellite has no build modes.
""")

E("custom exception types and `std::exception`", "C++98", "M33",
  "struct ParseError : std::runtime_error { using runtime_error::runtime_error; };",
  None,
  "M33, which is deliberately `std::expected` rather than `try`. A refusal would become a value you can inspect, not a class hierarchy you catch by type.")

E("`std::terminate`, `std::abort`", "C++98", "partial",
  "std::terminate();",
  None,
  """
**A refusal stops the run**, with a code, a caret and a call stack. **What is
missing is a program refusing on purpose.** `foreign.cpp` tells a Rust reader
about `panic!`: *there is no way to raise one on purpose yet*. M42 covers
the checked-condition form. A bare "stop here, with this message" has no
milestone.
""")

E("`std::error_code` and `errno`", "C++98 / C++11", "partial",
  "std::error_code ec;\nstd::filesystem::remove(p, ec);",
  "satellite.variable.file f = satellite.file.open(path, \"read\")\nsatellite.statement.if (f.ok() == satellite.bool.false) { satellite.console.display(f.error()) }",
  "A file answers `ok()` and `error()` rather than refusing, and M33's page names that as the pattern to generalise. `satellite.system.delete(x)` also answers whether the thing is gone.")

E("`std::exception_ptr`, `rethrow`, nested exceptions", "C++11", "M33",
  "std::exception_ptr e = std::current_exception();",
  None,
  "Carrying an error to another thread or another place is what M33's value-shaped refusal would allow. Before M33 there is nothing to carry.")

E("`std::stacktrace`", "C++23", "partial",
  "std::cout << std::stacktrace::current();",
  None,
  """
**Every refusal already prints one**, innermost first, under the caret
(`in save_checkpoint, called at line 187 / in satellite.main`). A program cannot
get one as a value. The CXX26 folder's `std::debugging` section records why
"watch a value without editing the loop" is the gap people actually hit.
""")

E("`std::unreachable`", "C++23", "never",
  "default: std::unreachable();",
  None,
  "Marking code unreachable is permission for undefined behaviour. The satellite version is to check it: M42, or M30's exhaustiveness check.")

# ---------------------------------------------------------------------------
section("Memory")

E("`malloc`, `calloc`, `realloc`, `free`", "C++98", "never",
  "int *p = (int *)malloc(n * sizeof(int));",
  None,
  "No manual allocation ([M29](M29.md), [M9](M9.md)). A list grows with `append` and `reserve(n)` takes a capacity hint.")

E("placement `new`", "C++98", "never",
  "new (buffer) Thing();",
  None,
  "No addresses and no manual construction. The CXX26 folder answers constexpr placement new the same way.")

E("allocators and `std::pmr`", "C++98 / C++17", "never",
  "std::pmr::vector<int> v{&arena};",
  None,
  "A container's storage is not the program's business (the CXX26 folder's `inplace_vector`/`hive` row). Memory *limits* are, and those are in the last entry of this section.")

E("`memcpy`, `memset`, `memcmp`", "C++98", "never",
  "std::memcpy(dst, src, n);",
  "satellite.container.list<satellite.variable.number> dst = src",
  "Assigning a container copies it, and `==` compares values.")

E("`std::make_unique`, `std::make_shared`", "C++11 / C++14", "never",
  "auto p = std::make_unique<Thing>();",
  "thing t",
  "Declaring a spacesuit constructs a counted handle. `foreign.cpp` gives this answer for both smart pointers.")

E("running out of memory, `std::bad_alloc`", "C++98", "partial",
  "try { big.resize(n); } catch (const std::bad_alloc &) { }",
  "satellite.statement.if (satellite.system.memory.main(\"mb\") > ceiling_mb) { ... }",
  """
**Satellite gives a program the numbers C++ never did.** `satellite.system.memory`
answers free, used and total for the machine, swap, and this process, in a unit
you pass. A watchdog stops `satl` when `MEMORY_MAX` is crossed.

**Partial, because allocation failure cannot be caught** (M33), so a program
has to check before it grows rather than recover afterwards.
""")

# ---------------------------------------------------------------------------
section("Standard library: utilities")

E("`std::swap`, `std::exchange`", "C++98 / C++14", "says it",
  "std::swap(a, b);",
  "satellite.variable.number spare = a\na = b\nb = spare",
  "Three lines with a spare. Spacesuit handles swap the same way and swap only the handles.")

E("`std::min`, `std::max`, `std::clamp`, `std::abs`", "C++98 / C++17", "says it",
  "int m = std::min(a, b);\nint c = std::clamp(v, lo, hi);",
  "satellite.variable.number m = a.min(b)\nsatellite.variable.number c = v.clamp(lo, hi)",
  "Methods on the first value. `a.max(b)`, `n.abs()`, `v.clamp(lo, hi)`. A list answers its own `min()`, `max()` and `sum()`.")

E("`std::hash` and user types as map keys", "C++11", "M38",
  "template <> struct std::hash<Point> { };",
  None,
  "A map key needs equality, and a spacesuit has no `==` until M38. Strings and numbers work as keys today.")

E("`std::optional` monadic `and_then`, `transform`, `or_else`", "C++23", "M32",
  "auto name = find(id).transform(&User::name).value_or(\"?\");",
  None,
  "Needs a callable to pass (M32) and chaining to write it (M29). Today it is a `holding()` check and a named value.")

E("`std::expected` monadic operations", "C++23", "M33",
  "parse(s).and_then(validate).or_else(report);",
  None,
  "M33 first (the value that holds an answer or a refusal), and M32 for the functions passed to it.")

E("`std::getenv`", "C++98", "says it",
  "const char *home = std::getenv(\"HOME\");",
  "satellite.variable.string home = satellite.system.environment(\"HOME\")",
  "Checked 2026-09-12: printed `/home/madness`.")

E("`std::system`", "C++98", "open",
  "std::system(\"ls -l\");",
  None,
  """
**There is no way to run another program.** `satellite.system` answers the
machine's facts, `satellite.directory` lists and changes directories, and
`satellite.system.delete` removes a file. None of them starts a process.

**Open, with a dependency question.** A shell escape is the widest door a
language can open. [../ASM/](../ASM/) names `satellite.system` and
`satellite.file` as "the ways out", and this would be a third.
""")

E("`std::exit` and the exit code", "C++98", "says it",
  "return 2;   // from main\nstd::exit(1);",
  "satellite.return(satellite)",
  "`satl`'s exit code is decided by whether the run refused, not by a number the program returns ([M1](M1.md)). A program cannot choose exit code 2. That is on purpose, and it will matter to shell scripts.")

E("signals `<csignal>`", "C++98", "never",
  "std::signal(SIGINT, on_interrupt);",
  None,
  "Ctrl-C belongs to satellite (DESIGN §10.2), which handles it for the program. There is no handler for a program to install.")

E("command-line option parsing", "C++98", "says it",
  "if (argc > 2 && std::string(argv[1]) == \"--fast\")",
  "satellite.statement.if (arguments.contains(\"--fast\")) { ... }",
  "`main` receives a `list<string>` ([M1](M1.md)), and `satellite.container.arguments` adds `has(k)`, `get(k)`, `keys`, `first` and `last`, so options do not need a parsing loop.")

# ---------------------------------------------------------------------------
section("Standard library: strings and text")

E("`find` and `substr`", "C++98", "says it",
  "auto at = s.find(\"tell\");\nauto part = s.substr(2, 2);",
  "satellite.variable.number at = s.find(\"tell\")\nsatellite.variable.string part = s.substring(2, 4)",
  """
**`substring` takes an END, not a LENGTH.** Checked 2026-09-12:
`"abcdef".substring(2, 4)` is `"cd"`. The C++ call `substr(2, 4)` would give
`"cdef"`. A C++ reader will get this wrong exactly once.
""")

E("`starts_with`, `ends_with`, `contains`", "C++20 / C++23", "says it",
  "if (path.starts_with(\"/\"))",
  "satellite.statement.if (path.starts_with(\"/\")) { ... }",
  "The same three names.")

E("`std::to_string`, `std::stoi`, `std::stod`", "C++11", "says it",
  "int n = std::stoi(\"42\");",
  "satellite.variable.string text = \"42\"\nsatellite.variable.number n = text.to_number()",
  "Checked 2026-09-12: `\"42\".to_number()` plus 1 printed `43`. One conversion serves both integers and decimals, because there is one number type.")

E("`std::to_chars`, `std::from_chars`", "C++17", "says it",
  "std::to_chars(buf, buf + 32, value);",
  "satellite.variable.string text = value.to_string()",
  "`to_chars` exists for speed and for being locale-free. Satellite's conversion does not use a locale, and there is no caller-owned buffer to write into.")

E("`toupper`, `tolower`", "C++98", "says it",
  "std::transform(s.begin(), s.end(), s.begin(), ::toupper);",
  "satellite.variable.string loud = s.upper()",
  "`upper()` and `lower()` convert the whole string. Only ASCII changes: strings are bytes, and M45 records that `upper()` on anything else does nothing, silently.")

E("split, trim and replace", "C++20 (`views::split`)", "says it",
  "for (auto part : s | std::views::split(','))",
  "satellite.container.list<satellite.variable.string> parts = s.split(\",\")",
  "`split(separator)`, `trim()` and `replace(a, b)` are string methods, so C++'s most hand-written helpers are built in.")

E("`std::stringstream`, `std::ostringstream`", "C++98", "never",
  "std::ostringstream out; out << \"x=\" << x;",
  "satellite.variable.string out = \"x=\" + x",
  "A line is built with `+` or with `append(x)` ([M3](M3.md)). To split text, `split()` does what the input half of a stringstream is usually used for.")

E("`std::regex`", "C++11", "open",
  "std::regex re(R\"(\\d+)\"); std::regex_search(s, re);",
  None,
  """
**There is no regex.** List and map both have `search(pattern)`.
Checked 2026-09-12: `["apple","banana"].search("an")` answered `[]`, so it is
not substring matching, and `satellite.help` should be asked what a pattern is
before anyone says it matches.

**Open.** Worth recording that C++'s own `std::regex` is widely considered a
mistake (slow, and stuck with its ABI), so "not that one" is part of any answer.
""")

E("indexing a string `s[i]`, `s.at(i)`", "C++98", "says it",
  "char c = s.at(3);",
  "satellite.variable.string c = s.at(3)",
  "Answers a one-byte string, not a character. It is a byte index, like C++'s, and M45 says what that means for UTF-8.")

E("ordering strings `<`, `compare`", "C++98", "open",
  "if (a < b)",
  None,
  """
**Checked 2026-09-12: the language disagrees with itself.** `a < "y"` is
refused with `S0712` (*string has no order*), yet `list.sort()` sorted `b, a`
into `a, b`. Something in satellite already orders strings. The operator is the
part that says they have no order.

**Open: pick one.** Either strings have an order and `<` should use the one
`sort()` uses, or they do not and `sort()` owes a sentence saying what it
compares.
""")

E("`std::locale` and `<codecvt>`", "C++98 / C++11", "M45",
  "std::locale::global(std::locale(\"en_US.UTF-8\"));",
  None,
  "Locale and encoding are M45. `<codecvt>` was deprecated in C++17, which is a warning about doing this at the library level.")

# ---------------------------------------------------------------------------
section("Standard library: containers")

E("`std::deque`", "C++98", "says it",
  "std::deque<int> d; d.push_front(1);",
  "d.insert(0, 1)\nd.remove_first()",
  "A list supports `insert(n, x)`, `first`, `last`, `remove_first()` and `remove_last()`. Its cost at the front has not been measured here.")

E("`std::list`, `std::forward_list`", "C++98 / C++11", "says it",
  "std::list<int> l;",
  "satellite.container.list<satellite.variable.number> l = satellite.container.list()",
  "Satellite has one list. What a linked list adds in C++, iterators that stay valid while you splice, has nothing to hold on to here ([M16](M16.md): no iterators).")

E("`std::set`, `std::unordered_set`", "C++98 / C++11", "partial",
  "std::unordered_set<std::string> seen;\nif (!seen.contains(k)) seen.insert(k);",
  "satellite.container.map<satellite.variable.string, satellite.variable.bool> seen = satellite.container.map()\nsatellite.statement.if (!seen.has(k)) { seen.set(k, satellite.bool.true) }",
  "A map with a throwaway value is a set, and `has(k)` is the membership test. `list.contains(x)` also works, but it searches the list. Partial because the value is noise.")

E("`std::multiset`, `std::multimap`", "C++98", "partial",
  "std::multimap<std::string, int> m;",
  "satellite.container.map<satellite.variable.string, satellite.container.list<satellite.variable.number>> m = satellite.container.map()",
  "A map of lists. Because a map value is a copied list, adding to one means `get`, append to the named copy, `set` it back.")

E("`std::stack`", "C++98", "says it",
  "s.push(x); s.top(); s.pop();",
  "s.append(x)\ns.last()\ns.remove_last()",
  None)

E("`std::queue`", "C++98", "says it",
  "q.push(x); q.front(); q.pop();",
  "q.append(x)\nq.first()\nq.remove_first()",
  None)

E("`std::priority_queue`", "C++98", "partial",
  "std::priority_queue<int> pq;",
  "pq.append(x)\npq.sort_down()\npq.first()",
  "No heap: sort, then read the front. Correct, and slower per insert. A user key requires `sort_up(key)`, and a comparator function requires M32.")

E("`std::flat_map`, `std::flat_set`", "C++23", "never",
  "std::flat_map<int, int> m;",
  None,
  "One map, and how it is stored is not the program's business. That is the CXX26 folder's answer for `inplace_vector` and `hive`, and it applies here.")

E("`push_back`, `emplace_back`", "C++98 / C++11", "says it",
  "v.emplace_back(1, 2);",
  "v.append(x)",
  "`append` is the word, the same as Python's (`foreign.cpp`: *the same word -- 1 4 2 1*).")

E("`insert` and `erase` at a position", "C++98", "says it",
  "v.insert(v.begin() + n, x);\nv.erase(v.begin() + n);",
  "v.insert(n, x)\nv.remove_at(n)",
  "An index instead of an iterator. `remove(x)` removes by value.")

E("`std::erase`, `std::erase_if`", "C++20", "partial",
  "std::erase(v, 0);\nstd::erase_if(v, is_blank);",
  "v.remove(0)",
  "`remove(x)` covers erasing by value (whether it removes every match or only the first has not been checked here). `erase_if` takes a predicate, so it is M32.")

E("map access: `operator[]`, `find`, `at`, `contains`, `insert_or_assign`", "C++98 / C++17 / C++20", "says it",
  "m[\"a\"] = 1;\nif (m.contains(\"a\")) use(m.at(\"a\"));",
  "m.set(\"a\", 1)\nsatellite.statement.if (m.has(\"a\")) { ... m.get(\"a\") ... }",
  """
**`get` on a missing key refuses, it does not insert.** Checked 2026-09-12:
`S0726: this map holds nothing under zz -- a missing key is an error because
nothing is a value an entry can legitimately hold, and has(k) is the question
that answers false instead of stopping`.

That is C++'s `at()`, not its `operator[]`. The silent insertion in `m["x"]` is
not possible here.
""")

E("walking a map's keys and values", "C++98 / C++17", "says it",
  "for (const auto &[k, v] : m)",
  "satellite.container.list<satellite.variable.string> ks = m.keys()\nsatellite.statement.for (satellite.variable.number i = 0; i < ks.size(); i = i + 1)\n{\n    satellite.variable.string k = ks[i]\n    satellite.variable.number v = m.get(k)\n}",
  "`keys()` and `values()` answer lists. The loop is range-for (open, above) and the binding is M35.")

E("`reserve`, `capacity`, `shrink_to_fit`", "C++98 / C++11", "partial",
  "v.reserve(1000);",
  "v.reserve(1000)",
  "`reserve(n)` exists as a capacity hint, and `list_methods.cpp` notes that it keeps the hint honest under copy-on-write. There is no `capacity()` to read back and no `shrink_to_fit`.")

E("`resize`", "C++98", "partial",
  "v.resize(10);",
  "v.truncate(10)",
  "`truncate(n)` shrinks. There is no growing counterpart that fills with a default, so growing is a loop of appends.")

E("`front`, `back`, `size`, `empty`, `clear`", "C++98", "says it",
  "v.front(); v.back(); v.empty();",
  "v.first()\nv.last()\nv.empty()",
  "`first` and `last` instead of `front` and `back`. The rest keep their names.")

# ---------------------------------------------------------------------------
section("Standard library: algorithms")

E("`std::find`, `std::find_if`", "C++98 / C++11", "partial",
  "auto it = std::find(v.begin(), v.end(), x);",
  "satellite.variable.number at = v.index_of(x)",
  "`index_of(x)` and `contains(x)` cover finding a value. `find_if` takes a predicate, which is M32.")

E("`std::count`, `std::count_if`", "C++98", "partial",
  "auto n = std::count_if(v.begin(), v.end(), is_even);",
  None,
  "An index loop and a counter. `count_if` needs M32.")

E("`std::accumulate`, `std::reduce`, `std::ranges::fold_left`", "C++98 / C++17 / C++23", "partial",
  "int total = std::accumulate(v.begin(), v.end(), 0);",
  "satellite.variable.number total = v.sum()",
  "`sum()` covers the common case exactly, because numbers are exact. A fold with any other function needs M32, and as a pipeline M36.")

E("`std::min_element`, `std::max_element`", "C++98", "says it",
  "auto it = std::max_element(v.begin(), v.end());",
  "satellite.variable.number biggest = v.max()",
  "Answers the value, not a position. Use `index_of` on the answer for the position.")

E("`std::reverse`", "C++98", "says it",
  "std::reverse(v.begin(), v.end());",
  "v.reverse()",
  None)

E("`std::sort` with a comparator, `std::stable_sort`", "C++98", "partial",
  "std::sort(v.begin(), v.end(), [](auto &a, auto &b) { return a.age < b.age; });",
  "v.sort_up(key)",
  """
`sort()`, `sort(direction)`, `sort_up(key)` and `sort_down(key)` exist. A
comparator *function* is M32, and sorting spacesuits by their own `<` is M38.
Whether `sort` is stable has not been checked, so rely on it only after asking
`satellite.help`.
""")

E("`std::binary_search`, `std::lower_bound`", "C++98", "partial",
  "bool found = std::binary_search(v.begin(), v.end(), x);",
  "satellite.variable.bool found = v.contains(x)",
  "`contains` and `index_of` answer the question without requiring a sorted list. Nothing here uses the fact that a list is sorted to answer faster.")

E("`std::transform`, `std::for_each`", "C++98", "M36",
  "std::transform(in.begin(), in.end(), std::back_inserter(out), twice);",
  None,
  "M36, which depends on M32. Its page shows today's index loop.")

E("`std::remove_if` and the erase–remove idiom", "C++98", "M32",
  "v.erase(std::remove_if(v.begin(), v.end(), bad), v.end());",
  None,
  "Takes a predicate, so M32. Satellite has no erase-remove idiom to learn, because `remove(x)` and `remove_at(n)` change the list directly.")

E("`std::all_of`, `std::any_of`, `std::none_of`", "C++11", "M32",
  "bool ok = std::all_of(v.begin(), v.end(), positive);",
  None,
  "Predicates, so M32. `contains(x)` is `any_of` for one exact value.")

E("`std::iota`, `std::fill`, `std::generate`", "C++98 / C++11", "says it",
  "std::iota(v.begin(), v.end(), 0);",
  "satellite.statement.for (satellite.variable.number i = 0; i < n; i = i + 1)\n{\n    v.append(i)\n}",
  "A loop of appends. `std::generate` with a function object is M32.")

E("`std::shuffle`, `std::sample`", "C++11 / C++17", "partial",
  "std::shuffle(v.begin(), v.end(), rng);",
  "satellite.variable.number pick = satellite.random.fast(0, last_index)",
  "`satellite.random` draws in a range, so a shuffle is a hand-written swap loop. `seeded(seed)` makes it repeatable ([M53](M53.md)).")

E("`std::copy`, `std::copy_if`", "C++98 / C++11", "partial",
  "std::copy(a.begin(), a.end(), std::back_inserter(b));",
  "satellite.container.list<satellite.variable.number> b = a",
  "Assigning a list copies it (checked 2026-09-12). `copy_if` needs a predicate, which is M32.")

E("`views::zip`, `enumerate`, `chunk`, `slide`, `ranges::to`", "C++23", "M36",
  "for (auto [i, x] : std::views::enumerate(v))",
  None,
  "M36, which is eager and follows M29 and M32. `enumerate` specifically is what an index loop already gives you.")

E("parallel algorithms and execution policies", "C++17", "M43",
  "std::sort(std::execution::par, v.begin(), v.end());",
  None,
  """
**Satellite already has a pool and no door to it.** `machine_limits/pool.cpp`
keeps worker threads warm, and PLAN.md §4.5.1.2 assumes a `parallel_for` that no
document defines. Composing parallel work is M43, and M43 says it must not start
before M40.
""")

E("`views::join_with` and joining strings", "C++23", "says it",
  "auto line = parts | std::views::join_with(',') | std::ranges::to<std::string>();",
  "satellite.variable.string line = parts.join(\",\")",
  "`join(separator)` is a list method.")

# ---------------------------------------------------------------------------
section("Standard library: numerics")

E("`sqrt`, `pow`, `floor`, `ceil`, `round`, `trunc`", "C++98 / C++11", "says it",
  "double r = std::sqrt(x);",
  "satellite.variable.number r = x.sqrt()",
  "Number methods: `sqrt`, `power(b)`, `floor`, `ceil`, `round` (half away from zero, DESIGN §8.6) and `truncate`. `power` takes one argument, the exponent (a two-argument call is `S0722`).")

E("`sin`, `cos`, `tan`, `exp`, `log`", "C++98", "open",
  "double y = std::sin(angle);",
  None,
  """
**Not in `satl --words`.** A number has `sqrt` and `power` and nothing
trigonometric or logarithmic.

**Open, and it touches a real design question.** Satellite numbers are exact,
and `sin(1)` has no exact answer. It is either a float method, or a number
method rounded to `satellite.library.system.division_digits` the way `1 / 3`
already is.
""")

E("`std::numbers::pi` and mathematical constants", "C++20", "open",
  "double c = 2 * std::numbers::pi * r;",
  None,
  "Goes with the trigonometry entry above. π to `division_digits` places is the constant that entry implies.")

E("`std::gcd`, `std::lcm`", "C++17", "open",
  "auto g = std::gcd(a, b);",
  None,
  "**Not in `satl --words`.** A short loop with `%` today. Open, and small: it is exact integer arithmetic, which is what satellite numbers are best at.")

E("`std::midpoint`, `std::lerp`", "C++20", "says it",
  "auto mid = std::midpoint(a, b);",
  "satellite.variable.number mid = (a + b) / 2",
  "`std::midpoint` exists because `(a + b) / 2` overflows or rounds in C++. **Here it is exact**, so the obvious expression is correct.")

E("`std::numeric_limits`", "C++98", "never",
  "std::numeric_limits<int>::max()",
  "satellite.variable.number digits = satellite.library.system.division_digits",
  "Numbers have no maximum. The limits a program can hit are precision (`division_digits`, `float_digits`) and call depth (`max_depth`), and all three are library values.")

E("`<bit>`: `popcount`, `countl_zero`, `rotl`, `bit_width`, `byteswap`", "C++20 / C++23", "open",
  "int ones = std::popcount(mask);",
  None,
  """
**Not in `satl --words`.** `binary` and `hex` have `width`, `digits` and
conversions, and `number` has shifts. There is no counting or rotation.

**Open, and it belongs with the bitwise operators entry**, since `binary` is
where both would live.
""")

E("`std::bit_cast`", "C++20", "partial",
  "auto bits = std::bit_cast<std::uint64_t>(value);",
  "satellite.variable.binary bits = f.binary()",
  "`binary` and `hex` exist on `float` and on `number`. What they answer for a float, its IEEE bits or its value in base 2, has not been checked here, and a C++ reader should ask `satellite.help` before relying on it.")

E("`std::endian`", "C++20", "says it",
  "if (std::endian::native == std::endian::little)",
  "satellite.variable.string order = satellite.library.main.arguments.machine.byte_order",
  "The machine's byte order is a machine fact.")

E("`std::complex`", "C++98", "never",
  "std::complex<double> z{1, 2};",
  None,
  "A library, not a language feature, which is the CXX26 folder's answer for `std::linalg`. A spacesuit with two numbers is the satellite version, and M38 would give it `+`.")

E("`std::valarray`", "C++98", "never",
  "std::valarray<double> v(10);",
  None,
  "Element-wise arithmetic on arrays is a feature C++ itself mostly abandoned. With no SIMD layer (CXX26 folder), it would only be a loop.")

E("`std::ratio`", "C++11", "never",
  "std::ratio<1, 1000>",
  None,
  "Compile-time fractions exist because C++ integers cannot hold `1/1000`. A satellite number holds it exactly at run time.")

E("NaN, infinity, `std::isnan`, rounding modes", "C++98 / C++11", "open",
  "if (std::isnan(x)) { }",
  None,
  """
**Checked 2026-09-12: satellite does not produce infinity.** `1.0 / 0.0` on
floats is `S0601`, the same refusal as for numbers. Whether a float can
ever hold NaN (from a conversion, or from the C++ side) was not found.

**Open: say it in DESIGN.** If floats never hold NaN or infinity, that is a
real guarantee C++ readers will not assume, and it should be written down.
""")

E("mathematical special functions", "C++17", "never",
  "std::riemann_zeta(2.0);",
  None,
  "A library, not a language feature, as with `std::complex`.")

E("`rand()` and `srand()`", "C++98", "says it",
  "srand(42); int r = rand() % 6;",
  "satellite.variable.number r = satellite.random.seeded(42)",
  """
`satellite.random.fast(min, max)` for a range, and `seeded(seed)` to repeat a
sequence. `fast`, `normal` and `ultra` are **tiers of generator, not
distributions**. `satellite.random.normal()` is not a normal distribution,
which is the next entry. [M53](M53.md) covers `<random>`.
""")

E("random distributions `normal_distribution`, `poisson_distribution`", "C++11", "open",
  "std::normal_distribution<double> d(0.0, 1.0);",
  None,
  """
**Satellite draws uniformly in a range.** No bell curve, no Poisson.

**Open, with a naming trap.** `satellite.random.normal` is already a tier, so
a normal *distribution* cannot use that word ("a word means one thing
everywhere", DESIGN §1).
""")

# ---------------------------------------------------------------------------
section("Standard library: input, output and files")

E("`std::cin >>`", "C++98", "says it",
  "int n; std::cin >> n;",
  "satellite.variable.number n = satellite.console.input(\"ENTER A NUMBER: \")",
  "`input(prompt)` answers the typed line, and `input(prompt, target)` writes into a variable (`example/console.satl`).")

E("`std::getline(std::cin, line)`", "C++98", "says it",
  "std::getline(std::cin, line);",
  "satellite.variable.string line = satellite.console.input()",
  "Input is line-at-a-time already. For files, `f.read_line()`.")

E("`std::cerr` and `std::clog`", "C++98", "open",
  "std::cerr << \"warning\\n\";",
  None,
  """
**A program writes to one place.** `satellite.console.display` is the output
path, and refusals go wherever `satl` sends diagnostics. A program cannot keep
its errors out of piped output.

**Open.** Anyone running satellite in a pipeline will need this.
""")

E("`std::endl`, `std::flush`, stream manipulators", "C++98", "says it",
  "std::cout << x << std::endl;",
  "satellite.console.display(x)",
  "`display` adds the newline. There are no manipulators, which is the format-string loss [M3](M3.md) records.")

E("binary file I/O, `seekg`, `tellg`", "C++98", "open",
  "in.seekg(128); in.read(buf, 16);",
  None,
  """
**Files are text lines and whole reads.** `read_line`, `read_all`, `write`
and `write_line` exist, and there is no seek, no byte count and no binary mode.

**Open.** Combined with M44 (a file's bytes at parse time), this is where
`satellite.variable.binary` would meet a file.
""")

E("`std::filesystem`: exists, current directory, listing", "C++17", "says it",
  "for (auto &e : std::filesystem::directory_iterator(\".\"))",
  "satellite.container.list<satellite.variable.string> here = satellite.directory.list()",
  "`satellite.file.exists(path)`, `satellite.directory.current()`, `.change(d)`, `.exists(d)`, `.list()` and `.list(d)`.")

E("`std::filesystem::remove`", "C++17", "says it",
  "std::filesystem::remove(p);",
  "satellite.variable.bool gone = satellite.system.delete(p)",
  "`satellite.help` (checked 2026-09-12): *removes a file, or an **empty** directory, and answers whether it is gone.* It is not `remove_all`.")

E("`create_directory`, `rename`, `copy`", "C++17", "open",
  "std::filesystem::create_directories(p);",
  None,
  "**Not in `satl --words`.** A program can list, change into and delete directories but not create one, or rename or copy a file. Open, and it rounds out `satellite.directory`.")

E("`std::osyncstream`: output from several threads", "C++20", "open",
  "std::osyncstream(std::cout) << \"line\\n\";",
  None,
  "Whether two threads calling `satellite.console.display` at once can interleave within a line has not been checked here. `satellite_console/console.cpp` does lock, so the answer may already be *says it*. Open until measured, next to M40.")

# ---------------------------------------------------------------------------
section("Standard library: time")

E("`std::this_thread::sleep_for`", "C++11", "says it",
  "std::this_thread::sleep_for(std::chrono::milliseconds(250));",
  "satellite.time.sleep(250)",
  "Milliseconds, per `satellite.help` (checked 2026-09-12).")

E("`system_clock::now()` and subtracting two times", "C++11", "partial",
  "auto elapsed = std::chrono::system_clock::now() - start;",
  None,
  """
`satellite.time.now` gives an instant, and today an instant can only be
displayed. Subtracting two, and whether the result is a number of seconds or a
`duration`, is **PLAN.md's M29 (the calendar)**. That is a build milestone in the
plan's own numbering, not a promise from this folder.
""")

E("`<chrono>` calendar and time zones", "C++20", "partial",
  "std::chrono::year_month_day ymd{std::chrono::floor<std::chrono::days>(now)};",
  None,
  "`satellite.variable.date` `1 6 7` is numbered and unbuilt. Its design is PLAN.md's M29, whose blockers are what a date *is* and what `time.new` takes. Time zones are not mentioned anywhere yet.")

E("`steady_clock` and measuring intervals", "C++11", "open",
  "auto t0 = std::chrono::steady_clock::now();",
  None,
  "DESIGN §13 settled that a time is `system_clock`, which can jump when the wall clock is set. A monotonic clock for timing work is not numbered. Open, and it belongs next to PLAN.md's M29.")

# ---------------------------------------------------------------------------
section("Standard library: concurrency, in detail")

E("`std::jthread` and `std::stop_token`", "C++20", "partial",
  "std::jthread t([](std::stop_token st) { while (!st.stop_requested()) { } });",
  "satellite.variable.thread t = satellite.thread.new(work(x))\nt.start()\nt.join()",
  "Threads start and join ([M45](M45.md)). **Asking a thread to stop is not in the word tree**, and the only shared signal is a `satellite.library` flag, which M40's open questions cover.")

E("`std::condition_variable`", "C++11", "M40",
  "cv.wait(lock, [] { return ready; });",
  None,
  "Waiting for a condition needs a lock to wait on, and there is none. M40.")

E("`std::shared_mutex`, `std::scoped_lock`", "C++17", "M40",
  "std::scoped_lock both(a_mutex, b_mutex);",
  None,
  "M40. Its third option, containers that refuse when shared across threads, is what would make reader-writer locks rarely needed.")

E("semaphores, `std::latch`, `std::barrier`", "C++20", "M40",
  "std::latch done{workers};",
  None,
  "M40. `join()` is the only synchronisation today, and it is enough for the common case of starting N workers and waiting for all of them.")

E("`std::call_once`", "C++11", "M40",
  "std::call_once(flag, init);",
  None,
  "M40. `satellite.library` values assigned from literals are resolved at parse time, which removes the most common use: lazily initialising a constant.")

E("`std::promise`, `std::packaged_task`", "C++11", "M40",
  "std::packaged_task<int()> task(work); auto f = task.get_future();",
  None,
  "Getting a value back from a thread. The CXX23 index sends futures to M40, and M43 is where composing them lives.")

E("`std::thread::hardware_concurrency`", "C++11", "says it",
  "unsigned n = std::thread::hardware_concurrency();",
  "satellite.variable.number n = satellite.library.main.arguments.machine.threads",
  "`machine.threads` and `machine.cores` are separate facts, which is more than C++ gives you.")

E("`std::atomic<std::shared_ptr>`", "C++20", "M40",
  "std::atomic<std::shared_ptr<Config>> current;",
  None,
  "Swapping a shared handle atomically is M40. Whether spacesuit refcounts are atomic today is exactly what the folder currently disagrees about. Settle it in M40 before writing this page any further.")

# ---------------------------------------------------------------------------
section("The C heritage")

E("C strings: `char *`, `strlen`, `strcpy`, `strcmp`", "C++98", "never",
  "char buf[64]; strcpy(buf, name);",
  "satellite.variable.string copy = name",
  "One string type that knows its size (`size()`) and cannot overrun. C++'s own reason for having `std::string`, taken as the only option.")

E("`printf` and `scanf`", "C++98", "says it",
  "printf(\"%d\\n\", x);\nscanf(\"%d\", &x);",
  "satellite.console.display(x)\nsatellite.variable.number x = satellite.console.input()",
  "No format string ([M3](M3.md), [M50](M50.md)). `foreign.cpp` already answers `printf(`.")

E("`setjmp` and `longjmp`", "C++98", "never",
  "if (setjmp(env) == 0) { longjmp(env, 1); }",
  None,
  "A non-local jump is `goto` across frames ([M23](M23.md): never). Recovering from failure is M33.")

E("plain `struct` with only data", "C++98", "says it",
  "struct Point { int x; int y; };",
  "satellite.spacesuit point()\n{\n    satellite.protected\n    {\n        satellite.variable.number x = 0\n        satellite.variable.number y = 0\n    }\n    satellite.public\n    {\n        satellite.capsule call_x() satellite.returns(satellite.variable.number) { satellite.return(x) }\n    }\n}",
  "**Fields are never public** (DESIGN §12: a field is reachable only from inside the spacesuit), so a C struct needs an accessor per field. That is the cost for a plain data type, and it is paid on purpose.")

E("`FILE *`, `fopen`, `fgets`, `fclose`", "C++98", "says it",
  "FILE *f = fopen(path, \"r\"); fgets(line, 256, f); fclose(f);",
  "satellite.variable.file f = satellite.file.open(path, \"read\")\nsatellite.variable.string line = f.read_line()\nf.close()",
  "Closing is explicit (`foreign.cpp` on Python's `with`: *there is no scope guard*). Check `f.ok()` after opening.")

E("`<ctime>`: `time()`, `strftime`", "C++98", "partial",
  "std::time_t t = std::time(nullptr);",
  "satellite.console.display(satellite.time.now())",
  "An instant can be read and displayed. Formatting it and doing arithmetic on it are PLAN.md's M29, as above.")

# ---------------------------------------------------------------------------

def render_page(n, e):
    std = e["std"]
    lines = [f"# CXX23/M{n} — {e['feature']}", ""]
    lines.append(f"**Introduced:** {std}. Present in C++23 (ISO/IEC 14882:2024).")
    lines.append(f"**Answer: {link(ANSWER_WORDS(e['answer']))}.**")
    lines.append("")
    if e["cxx"]:
        lines += ["## C++", "", "```cpp", e["cxx"], "```", ""]
    if e["sat"]:
        lines += ["## satellite", "", "```", e["sat"], "```", ""]
    if e["differs"]:
        head = "## What differs" if e["sat"] else "## Why"
        lines += [head, "", link(e["differs"]), ""]
    return "\n".join(lines).rstrip() + "\n"

def ANSWER_WORDS(a):
    return {
        "says it": "satellite says it",
        "partial": "partial — satellite has part of it",
        "never": "never",
        "open": "open — no milestone and no refusal yet; this needs a decision",
    }.get(a, f"satellite milestone {a}")

def main():
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
            rows.append(f"| {n} | {cell} | {e['std']} | {link(e['answer'])} | [M{n}](M{n}.md) |")
            m = re.match(r"M(\d+)$", e["answer"])
            if m:
                asks.setdefault(e["answer"], []).append(n)
            if e["answer"] == "open":
                opens.append((n, e["feature"]))
            n += 1
        index.append((heading, rows))
    with open(os.path.join(os.path.dirname(__file__), "index.md"), "w") as f:
        for heading, rows in index:
            f.write(f"### {heading}\n\n| # | C++ feature | since | satellite | file |\n|---|---|---|---|---|\n")
            f.write("\n".join(rows) + "\n\n")
        f.write("ASKS\n")
        for k in sorted(asks, key=lambda s: int(s[1:])):
            f.write(f"| {link(k)} | {', '.join(f'M{x}' for x in asks[k])} |\n")
        f.write("OPENS\n")
        for x, feat in opens:
            f.write(f"| [M{x}](M{x}.md) | {feat.replace('|', chr(92) + '|')} |\n")
    print("wrote", FIRST, "to", n - 1, "=", n - FIRST, "pages;", len(opens), "open")

main()
