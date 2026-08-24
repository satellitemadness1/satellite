*satellite design docs, §3 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§2](02-hello-world.md), On: [§4](04-the-reservation-rule.md).*

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

**Done — and the table's silence about every OTHER backslash turned out to
matter.** An escape this table does not know passes through with the backslash
still attached, which is not an error and therefore invisible: `"DOESN\'T"`
printed the backslash, 501 times in one real program's output. §19.8 adds `\'`
as a redundant spelling of `'` and explains why rejecting unknown escapes
outright was the wrong fix.

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
