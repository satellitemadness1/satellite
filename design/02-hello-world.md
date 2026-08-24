*satellite design docs, §2 of 19. Index: [DESIGN.md](../DESIGN.md).*
*Back: [§1](01-the-generating-rule.md), On: [§3](03-lexical-structure.md).*

---

## 2. Hello world

```satellite
satellite.include(satellite)

satellite.capsule satellite.main(satellite.container.list<satellite.variable.string> argz)
{
    satellite.console.display("hello, world!")

    satellite.return(satellite)
}
```

- **capsule** = function.
- **`satellite`** as a value is the *singleton runtime object*, not a zero sentinel. That
  is why `satellite.include(satellite)` and `satellite.return(satellite)` can use the same
  word to mean sensible things: include the runtime, return the runtime (i.e. success).
- **`satellite.console.display`** is ordinary stdout. The child process writes bytes to the
  PTY and VTE renders them. Implementation is `decode()` + `fwrite` + `fflush` — the flush
  is required because glibc line-buffers stdout on a tty. Nothing else is involved: no GTK,
  no IPC. **Verified**: "hello, world!" round-trips through `encode`/`decode` byte-identically.
- **`satellite.include`** works as of 2026-08-19 (§16). `satellite.include(satellite)` is
  the one form that does nothing, and that is not a leftover — it means "include the
  runtime", which a running program already has. Every other form names a **spaceship** and
  loads it: `satellite.include(my_parser)` merges the declarations in `my_parser.satl` into
  this program before it resolves. This line stays in hello world because the ceremony reads
  well, not because it is all the feature can do.
