# WORD_NUMBERS.md as the user wrote it, 2026-08-27

**This file is scratch**, and it exists for one reason: `WORD_NUMBERS.md` was
never committed, and rewriting it in prose overwrote the only copy. This is that
copy. Delete it once the rewrite has been read and accepted.

Every number below is preserved unchanged in the rewritten `WORD_NUMBERS.md` §2.2.
Their two parenthesised comments and their two `//` notes are preserved as table
annotations and as §3's blockquote.

```text
satellite.include()          1 1 (0)
satellite.include(satellite) 1 1 1
satellite.capsule            1 2 (0)
satellite.main               1 3 (0)
satellite.container          1 4 (0)
satellite.container.map      1 4 1
satellite.container.list     1 4 2
satellite.console            1 5 (0)
satellite.console.display    1 5 1
satellite.console.input      1 5 2
satellite.variable           1 6 (0)
satellite.variable.string    1 6 1
satellite.variable.file      1 6 2
satellite.variable.time      1 6 3
satellite.variable.number    1 6 4
satellite.random.fast        1 7 1
satellite.random.normal      1 7 2
satellite.random.ultra       1 7 3
satellite.file.new           1 8 1
satellite.file.open          1 8 2
satellite.time.now           1 9 1
satellite.time.new           1 9 2 (set the arguments for a point in time)
satellite.file.clear         1 8 3 (empty the contents of a file;)
satellite.spacesuit          1 10 (0)
satellite.protected          1 11 (0)
satellite.public             1 12 (0)
satellite.statement          1 13 (0) // this one kinda doesn't do anything
satellite.statement.if       1 13 1
satellite.statement.for      1 13 2
satellite.statement.while    1 13 3
satellite.statement.else     1 13 4
satellite.library            1 14 (0)
satellite.library.main       1 14 1
satellite.library.x          1 14 ? // this needs to be built-in to grab the next available number, as x is just a capsule name; so this needs to grab 1 14 x where x is the next available number, and a similar thing needs to happen with user defined classes (spacesuits)
satellite.return()           1 15 ?
satellite.return(satellite)  1 15 1


there was one thing that when we programmed it, it took 4 numbers; I can't remember
what it was though;

so all user defined capsules and all user defined spacesuits need to grab the next
available number; the language needs to keep and always have ready the next available number;
```

## The one question in it that was answered

> *"there was one thing that when we programmed it, it took 4 numbers; I can't
> remember what it was though"*

Two families in the first satellite took four segments, both visible in
`old_versions/first_satellite/src/bytecode_format/format.def`:

    satellite.random.fast.range(low, high)     format.def:598
    satellite.random.normal.range(low, high)   format.def:599
    satellite.random.ultra.range(low, high)    format.def:600

    satellite.system.memory.used([unit])       format.def:552
    satellite.system.memory.free               format.def:553
    satellite.system.memory.total              format.def:554
    satellite.system.memory.swap               format.def:555
    satellite.system.memory.main               format.def:556
    satellite.system.memory.frequency          format.def:557
    satellite.system.memory.bit                format.def:558

`.range` is the likelier memory, since it hangs off the three tiers already
numbered at `1 7 1`, `1 7 2` and `1 7 3`.

It is also worth recording *why* four felt like a limit rather than a length: v1's
macro was `SAT_PATH(ident, s1, s2, s3, s4, arity)` — literally four segment slots
and no fifth. DESIGN §4.5 names that as a limit this language refuses. Under the
numbering here, four is not special.
