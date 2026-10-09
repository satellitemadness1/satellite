# Building a programming language IN satellite

What satellite still needs before someone can write a programming language in it, and the
milestones that get there, in order. Written 2026-09-25 at your asking: *"can you build a set
of milestones that we could do so we CAN build a programming language in satellite? Or at least
a list of what satellite is still missing before we can build a programming language in it?"*

Every "have" and "missing" below was run on BUILD 0085 tonight, not guessed.

---

## What "a language in satellite" means

A program written in satellite that reads another language's source and runs it. That's an
interpreter, the same shape satl is in C++:

1. **The lexer** reads the text one character at a time and cuts it into tokens:
   `12`, `+`, `x`, `"hi"`.
2. **The parser** turns the tokens into a tree. `1 + 2 * 3` becomes a `+` node whose right
   child is a `*` node.
3. **The evaluator** walks the tree and works out the answer, keeping variables in a table of
   names.

Around those three: error messages that name a line, reading the file, and a prompt.

---

## What satellite already has (proved tonight)

| an interpreter needs | satellite has | proved by |
|---|---|---|
| the source file | `satellite.file.open(path)`, one line at a time as `f[1]`, `f.size()` | read a file's first line |
| keyboard input for a prompt | `satellite.console.input()` | read `hello` from stdin |
| a list of tokens | `satellite.container.list<satellite.variable.string>`, `.append`, `.size` | built a 2-token list |
| a table of variables | `satellite.container.index<satellite.variable.string, ...>`, `["x"]`, `.contains` | stored and read back `x = 5` |
| a value that can be any kind | `satellite.container.multiple` | `{1, "two"}` |
| tree nodes | a spacesuit with a `satellite.container.list<node>` of children | a `+` node over `2` and `3` evaluated to **5** |
| walking a tree | capsules calling themselves, **as deep as memory allows** (built tonight) | 1,000,000 levels deep |
| if / while / for, capsules, spacesuits, includes, namespaces | built | check.sh |
| reading a whole number out of text | `"42".number` | check.sh |

**So the parser and the evaluator can be written today.** A syntax tree and a recursive
evaluator work (the `+` tree above). What can't be written yet is the **lexer**, because
satellite can't yet look at one character of a string, plus a few control-flow words every
interpreter leans on.

---

## What satellite is missing (each probed tonight)

| missing | what an interpreter uses it for | tonight's answer | where it's planned |
|---|---|---|---|
| `s.size()` | how long the source line is | S210, not built | M16 |
| `s[n]` / `s.at(n)` | **one character at a time: the whole lexer** | S301 / S110 | M16 (`at`) |
| `s.substring(a, b)` | cutting a token out of the line | not built | M16 |
| `.contains`, `.starts_with`, `.split`, `.trim` | keywords, splitting, whitespace | not built | M16 |
| a character's code, or `.is_digit()` / `.is_letter()` / `.is_space()` | "is this a digit?" -- deciding what kind of token begins here | `"A".code()` is S110 | **not planned** |
| `"3.5"` read as a number | decimal literals in the language being written | S120, refused | M17 / M20 |
| `satellite.statement.break`, `continue` | leaving the lexer's loop at the end of a token | S110 | M20.E, M20.F |
| `satellite.statement.switch` | choosing what to do for each kind of node | S110 | M20.H (your design) |
| asking what kind a value is (`x.kind`) | the evaluator: is this a number or a string? | S110 | **not planned** |
| errors a program can catch (`try`) | an error in the interpreted program must not stop the interpreter | S110 | **not planned** |
| a capsule held in a variable | a table of built-in functions | S110 | **not planned** |
| `& | ~ << >>` | bytecode and flags, for a faster interpreter | ruled 2026-09-22, not built | operator spellings |

---

## The milestones, in order

Each one ends with a satellite program that proves it, the way check.sh proves everything
else.

### L1 — the string methods (this is M16, already next)

`size`, `at` and `s[n]`, `substring`, `contains`, `starts_with`, `ends_with`, `split`, `trim`,
`replace`, `append`. Every one is numbered already (`1 6 1 n`) and has a library ported from
003. **Done when** a satellite program reads `"12 + 34"` one character at a time and prints each
character on its own line.

### L2 — characters as things a program can ask about (NEW)

Either a character's number (`"A".code()` is 65, and a way back from 65 to `"A"`), or the three
questions a lexer asks: `.is_digit()`, `.is_letter()`, `.is_space()`. Both are your call:
the spelling, and whether "letter" means a–z or every language's letters (tonight's
`.upper()` already reads every language's). **Done when** a program sorts every character of
`"x1 + 22"` into digit, letter, space or sign.

### L3 — break, continue and switch (M20.E, M20.F, M20.H, your designs)

A lexer is a loop that stops at the end of each token, and an evaluator is a switch over the
kinds of node. Your switch design (a case can be any piece of a string, and code between cases
runs no matter what) is in MILESTONES M20.H. **Done when** a `while` over a string stops with
`break` at the first space, and a `switch` picks the case for `"+"`.

### L4 — reading every kind of number out of text (M17/M20)

`"3.5".number` is refused today. The language being written needs its decimal literals.
**Done when** `"3.5"` reads as 3.5 and `"1/3"` as a fraction.

### L5 — asking what a value is (NEW)

`x.kind` answering `"number"`, `"string"` and so on. It's the evaluator's first question about
anything it pulls out of a `satellite.container.multiple`. **Done when** a program prints the
kind of each item in `{1, "two", 3.5}`.

### L6 — errors a program can catch (NEW, your design)

Today every refusal ends the whole run. An interpreter written in satellite has to refuse the
program it's running without dying itself. Two shapes to choose between:
- **try/catch:** `satellite.statement.try { } satellite.statement.catch(e) { }`.
- **Errors as values:** a capsule hands back an error, and the caller asks `.ok`. The file
  object already works this way, with `f.ok()`.

**Done when** a satellite program runs `1 / 0` as the language it interprets, prints "division
by zero on line 1", and carries on.

### L7 — THE FIRST LANGUAGE: a calculator

`examples/lang/calc.satl`: reads `1 + 2 * (3 - 1)` from the prompt or a file. A lexer (L1–L3), a
recursive-descent parser building spacesuit nodes (possible today), and an evaluator (possible
today) print **5**. With precedence, brackets and an error message that points at the column.
**This is the proof that a language can be written in satellite.**

### L8 — a small language with variables, if and while

`examples/lang/tiny.satl`: `x = 5`, `if x > 3 { print x }`, `while`. The variable table is a
`satellite.container.index` (possible today); errors use L6.

### L9 — calling capsules by name (NEW, optional)

A capsule held in a value or a table, so a language's built-in functions can be a table and not
a long switch. It's optional: L3's switch does the job, only more slowly to write.

### L10 — speed

An interpreter in satellite runs inside satl, an interpreter too. Once L8 runs, measure it
against the same program in C++ and Python, the way `time_test/` does. Then the tools are
M12's batches (running independent work in parallel) and the bit operators for a bytecode
version.

### L11 — satellite in satellite (long term)

A satellite interpreter written in satellite, that runs check.sh's own programs. Every milestone
above is on its way there.

---

## The short version

- **Can be written today:** the tree and the evaluator.
- **Blocked by M16** (next anyway): the lexer, which needs a string's characters.
- **Small, and yours to design:** character questions (L2), a value's kind (L5), catching
  errors (L6), capsules as values (L9).
- **Designed and waiting:** break, continue and switch (M20.E/F/H).
- **The first language is L7:** a calculator, a few hundred lines of satellite once L1–L3 are in.
