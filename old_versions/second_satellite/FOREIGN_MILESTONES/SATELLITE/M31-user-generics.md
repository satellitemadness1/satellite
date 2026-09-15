# M31 — generics a program can write

**Named by** `foreign.cpp` row for `template<` (C++); also Java's `<T>`, Rust's
`fn f<T>`, and every `Vec<T>` a reader expects to be able to define themselves.

## What satellite makes you write instead

Nothing, and this is the gap with the ugliest workaround in the tree.
`satellite.container.list<T>` and `.map<K,V>` ARE generic — the language has the
machinery — but nothing a program declares can be. So a container of your own is
written once per element type:

```
satellite.spacesuit earth_shard()  { ... list<planet_earth_entity> ... }
satellite.spacesuit world_shard()  { ... list<world_citizen>       ... }
```

Those two spacesuits in `infinity_data_main.satl` are **the same spacesuit
written twice**, differing in one type name, and the file says so at the top of
the second one. Every method is duplicated; a fix to one is a fix owed to the
other.

## What it would cost to build

This is the largest milestone in this folder and should be sized honestly:

- a type parameter on `satellite.spacesuit` and `satellite.capsule` declarations
- the resolver carrying a binding from parameter to argument through a body
- `.satc` representing an unresolved type — today every type in the cache is a
  number, and `T` is not
- a decision on **monomorphisation vs one shared body**, which decides whether
  this costs code size or dispatch time

## What it must not break

The numbering. `list<T>` works today because `1 4 2` is one path whose element
type is checked by the resolver, not by the number. A user generic must resolve
the same way — the type parameter cannot mint numbered paths, or every
instantiation would renumber the language.
