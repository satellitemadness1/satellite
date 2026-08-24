*satellite design docs, §5 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§4](04-the-reservation-rule.md), On: [§6](06-scope-model.md).*

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

### One bare name after a dot is not a method call

`a.cxx_compiler` — see §8.8. The postfix loop above already produces a `Member`
node for it, and always did; what changed is that the evaluator answers it for
one receiver instead of refusing it for all of them. The grammar is untouched,
which is the point of recording it here rather than in the productions.
