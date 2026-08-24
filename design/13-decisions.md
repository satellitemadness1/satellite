*satellite design docs, §13 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§12](12-deliberately-deferred.md), On: [§14](14-spacesuits.md).*

---

## 13. Decisions, and the one still open

This section held three genuine forks that needed an answer before M0. **M0 shipped, and each
was answered the way it was recommended here.** They are recorded rather than deleted, because
a design document that quietly loses the questions it once had cannot be trusted about the
ones it still has.

1. **Return type syntax** — optional `satellite.returns(TYPE)` after the parameter list,
   defaulting to the `satellite` type, which leaves hello world byte-identical. It is in §5's
   grammar and in `parse_signature`, and a constructor is the one member forbidden to declare
   one, since what a constructor produces is the object (§14).
2. **User capsules are bare** — `fact(3)`, with `satellite.main` the reserved exception, which
   is §1's invariant read straight.
3. **Statements are newline-terminated**, so §7's same-line rule for postfix `[` is load
   bearing rather than a nicety, and §14 reuses it for the bare spacesuit type.

The one question this section used to leave open — **§16's name for a unit of includable
code** — was answered on 2026-08-18: it is a **spaceship**. §16 carries the reasoning. With
that, this document has no open naming question left; what remains is unwritten code, not
undecided design.

### Resolved

**Control flow** is `satellite.statement.if(...)`, `satellite.statement.while(...)` and
`satellite.statement.for(...)`, each followed by a brace-delimited block. `statement` joins
`variable`, `container` and `library` as a segment-1 dispatch key, so §1's rule holds with no
exceptions anywhere in the language.

**Classes are `satellite.spacesuit`**, with `satellite.protected` / `satellite.public` blocks
and a bare-name type. See §14.

`for` is three-part and C-shaped —
`for(satellite.variable.number i = 0; i < 10; i = i + 1)` — which introduces no syntax the
language did not already have: `;` is an ordinary punctuation code, the init clause is the
same `VarDecl` a declaration statement produces, and the step clause is the same `Assign`.
All three parts are optional, so `for(;;)` is the infinite loop.

**`satellite.container.list` stays.** `variable` and `container` are both type namespaces
and §4's reservation rule covers both. Two type namespaces cost the parser one extra string
comparison in `at_type()`, and `container` earns it by saying something `variable` cannot:
this type holds other things and takes a generic parameter. A scalar and an aggregate
reading differently at a glance is the point, not an inconsistency.
