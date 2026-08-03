# Next task: the .satl comment stripper

Agreed as the first thing to build after this point. It must be **callable**,
because every `satellite.include` will run source through it.

---

## Two decisions to make before writing code

### 1. The lexer already skips comments — so what is the stripper for?

`Lexer::scan()` consumes `//` and `/* */` and never emits a token for either.
Anything that lexes already gets comment-free input. So a stripper is **not**
needed to feed the lexer, and building it for that reason would be redundant
work.

There are two jobs it genuinely does, and they decide its shape:

- **Fast include scanning.** The include scanner wants to find
  `satellite.include("...")` in hundreds of files without paying to fully lex
  each one. Stripping comments first means a plain substring scan cannot be
  fooled by `// satellite.include("not_real")` in a comment. This is the real
  motivation, and it is a good one.
- **Canonical form for caching.** Hash the stripped source to decide whether a
  file changed in a way that matters. Editing a comment then leaves the cache
  valid.

Recommend building it for those two, and *not* inserting it in front of the
lexer, which would be a pointless second pass over every byte.

### 2. It must preserve line numbers

If a comment is deleted, every line after it shifts up, and every diagnostic
from that point on points at the wrong line. That turns good error messages into
misleading ones — a worse outcome than not stripping at all.

**Replace comment characters with spaces; keep every newline.** The output is
byte-for-byte the same length and the same line structure as the input, with
comment text blanked. A `/* ... */` spanning four lines becomes four lines of
spaces.

```
satellite.variable.int x = 5   // the count
satellite.variable.int x = 5
```
(the comment becomes trailing spaces on the same line, line count unchanged)

---

## What it must NOT strip

Three cases, and getting any of them wrong corrupts working programs:

**1. Inside string literals.**
```
satellite.console.display("https://example.com")
```
The `//` is data, not a comment. The scanner must track whether it is inside a
`"..."`, honouring `\"` escapes.

**2. Inside `satellite.cxx { ... }` blocks — leave the entire block alone.**
The contents are C++, they get handed to a C++ compiler, and that compiler
handles its own comments. Touching them risks corrupting raw strings
(`R"(...)"`), character literals, and preprocessor lines. Skip the block wholly,
by counting braces the way `Lexer::scan_cxx_block()` already does.

**3. A `/*` inside a `//` line comment, and vice versa.** Whichever opens first
wins; the other is just text.

---

## Proposed interface

```cpp
// include/satellite/source.hpp
namespace satellite {

// Blanks comments to spaces, preserving length and line structure.
// String literals and satellite.cxx { } blocks are left untouched.
std::string strip_comments(const std::string& source);

struct StripStats {
    uint32_t line_comments  = 0;
    uint32_t block_comments = 0;
    uint32_t cxx_blocks_skipped = 0;
    uint32_t bytes_blanked = 0;
    bool     unterminated_block_comment = false;
    bool     unterminated_cxx_block     = false;
};

std::string strip_comments(const std::string& source, StripStats* stats);

}  // namespace satellite
```

Returning stats makes it testable and gives the include scanner something to
report.

---

## Tests it must pass

- `a // b` → `a      ` (same length, comment blanked)
- `a /* b */ c` → `a         c` (same length)
- a block comment spanning 4 lines → 4 lines, all whitespace after the opener
- `"http://x"` → unchanged
- `"a \" // b"` → unchanged (escaped quote inside the string)
- `satellite.cxx { // real C++ comment\n int x = 1 }` → block **entirely**
  unchanged, including its comment
- nested braces inside a cxx block do not end it early
- `/*` with no `*/` → sets `unterminated_block_comment`, blanks to end of input
- input with no comments → output identical to input, byte for byte
- **line count of output == line count of input, always** (this is the property
  that protects diagnostics; assert it on every test case)
- stripping is idempotent: `strip(strip(x)) == strip(x)`

---

## Sizing

~150 lines of implementation, ~150 of tests. It shares its scanning logic with
`Lexer::scan_cxx_block()` in `src/lexer.cpp` — read that first; the string, char,
raw-string and comment skipping is already written there and should be factored
out rather than duplicated.

---

## Where it fits

Stage 5 in `docs/INTERPRETER.md` is where `satellite.include` actually loads
files. This is a prerequisite for the scanner half of it, and it is
independently testable today, which is why it is a good next task.
