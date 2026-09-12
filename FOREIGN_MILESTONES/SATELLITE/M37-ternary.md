# M37 — the conditional expression

**Named by** `?:` (C, C++, Java, Rust's `if` as an expression, Python's
`a if c else b`).

## What satellite makes you write instead

A declaration, then an `if` that assigns:

```
satellite.variable.number target_gb = wanted_gb
satellite.statement.if(spare_gb < wanted_gb)
{
    target_gb = spare_gb
}
```

Four lines for one. In `infinity_data_main.satl` this shape appears often enough
that `x.min(y)` was reached for instead wherever the two branches were values
rather than statements — which is the workaround, and it only covers comparison.

## What it would cost to build

`satellite.statement.if` is a statement, and this is an **expression** — a
different node, a different place in the grammar, and a type rule that both arms
answer the same type. Small, and entirely in the parser and resolver.

## What it must not break

Nesting. `a ? b : c ? d : e` is the reason this operator has a bad name; the
parser should make the right-associative reading explicit or require brackets.
