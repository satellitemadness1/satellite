*satellite design docs, §4 of 21. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§3](03-lexical-structure.md), On: [§5](05-grammar.md).*

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
